#!/usr/bin/env python3
"""Architecture boundary guard for VMP-ui.

This is a soft P0 guard: existing boundary debt is explicitly baselined, while
new occurrences fail the check. The baseline should only shrink over time.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import sys


API_INCLUDE_RE = re.compile(r'#\s*include\s+"api/[^"\n]+"')
STATIC_RUNTIME_RE = re.compile(
    r"\b(?:"
    r"Registry::(?:TryGet|Get|View|AnyOf|AllOf|Clear|Emplace|Remove|Valid)|"
    r"Dispatcher::(?:Trigger|Enqueue|Sink|Update)"
    r")\b"
)
COMMON_RUNTIME_INCLUDE_RE = re.compile(r'#\s*include\s+"(?:core/RuntimeFacade\.hpp|singleton/Registry\.hpp)"')
PUBLIC_API_FORBIDDEN_INCLUDE_RE = re.compile(
    r'#\s*include\s+[<"](?:entt(?:/[^">]+)?|core/RuntimeFacade\.hpp|singleton/Registry\.hpp|singleton/Dispatcher\.hpp)'
)
ENTT_ENTITY_RE = re.compile(r"\bentt::entity\b")
ENTT_NAMESPACE_RE = re.compile(r"\bentt::")
RUNTIME_FACADE_CURRENT_RE = re.compile(r"RuntimeFacade::current\(\)")
RAW_ACCESS_RE = re.compile(r"\b(?:Registry|Dispatcher)::raw\(\)|\.raw\(\)")
UI_SOURCES_BLOCK_RE = re.compile(r"set\(UI_SOURCES\s+(.*?)\)", re.DOTALL)
DETAIL_CPP_ENTRY_RE = re.compile(r"\b(detail/[A-Za-z0-9_]+\.cpp)\b")

INTERNAL_API_INCLUDE_DIRS = (
    Path("src/core"),
    Path("src/detail"),
    Path("src/systems"),
    Path("src/renderers"),
    Path("src/services"),
)
STATIC_RUNTIME_DIRS = (Path("src/systems"),)
COMMON_DIRS = (Path("src/common"),)
PUBLIC_API_HEADER_DIRS = (Path("src/api"),)
API_CPP_DIRS = (Path("src/api"),)
RUNTIME_ACCESS_DIRS = (Path("src/systems"), Path("src/renderers"), Path("src/services"))
RAW_ACCESS_DIRS = (Path("src"),)
SOURCE_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"}
HEADER_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx"}
IMPLEMENTATION_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx"}

# Existing debt as of 2026-06-04. Keep exact occurrence counts so new usages fail
# even when a file already has one allowed include/call of the same kind.
ALLOWED_API_INCLUDE_COUNTS = Counter(
    {
    }
)

ALLOWED_STATIC_RUNTIME_COUNTS = Counter({})

ALLOWED_COMMON_RUNTIME_INCLUDE_COUNTS = Counter(
    {}
)

ALLOWED_API_CPP_RUNTIME_CURRENT_COUNTS = Counter(
    {
        ("src/api/Controls.cpp", "RuntimeFacade::current()"): 1,
        ("src/api/Factory.cpp", "RuntimeFacade::current()"): 1,
        ("src/api/Image.cpp", "RuntimeFacade::current()"): 1,
        ("src/api/Query.cpp", "RuntimeFacade::current()"): 3,
        ("src/api/Theme.cpp", "RuntimeFacade::current()"): 3,
        ("src/api/Utils.cpp", "RuntimeFacade::current()"): 3,
    }
)

ALLOWED_API_CPP_ENTT_ENTITY_COUNTS = Counter(
    {
        ("src/api/Factory.cpp", "entt::entity"): 6,
        ("src/api/Utils.cpp", "entt::entity"): 18,
    }
)

ALLOWED_RUNTIME_CURRENT_COUNTS = Counter(
    {
        ("src/renderers/ShapeRenderer.hpp", "RuntimeFacade::current()"): 1,
        ("src/services/TextEditingService.cpp", "RuntimeFacade::current()"): 2,
        ("src/systems/ActionSystem.hpp", "RuntimeFacade::current()"): 6,
        ("src/systems/HitTestSystem.cpp", "RuntimeFacade::current()"): 1,
        ("src/systems/InteractionSystem.hpp", "RuntimeFacade::current()"): 1,
        ("src/systems/PlatformWindowSystem.hpp", "RuntimeFacade::current()"): 3,
        ("src/systems/StateSystem.cpp", "RuntimeFacade::current()"): 14,
        ("src/systems/ThemeSystem.cpp", "RuntimeFacade::current()"): 4,
        ("src/systems/TimerSystem.cpp", "RuntimeFacade::current()"): 7,
        ("src/systems/TweenSystem.hpp", "RuntimeFacade::current()"): 1,
    }
)

ALLOWED_RAW_ACCESS_COUNTS = Counter(
    {
        ("src/core/RuntimeFacade.hpp", ".raw()"): 8,
        ("src/renderers/RendererRegistry.hpp", ".raw()"): 2,
    }
)

ALLOWED_UNLISTED_DETAIL_CPP_COUNTS = Counter(
    {
        ("src/detail/Factory.cpp", "detail-cpp-not-in-ui-sources"): 1,
        ("src/detail/Canvas.cpp", "detail-cpp-not-in-ui-sources"): 1,
        ("src/detail/Timer.cpp", "detail-cpp-not-in-ui-sources"): 1,
        ("src/detail/Shortcut.cpp", "detail-cpp-not-in-ui-sources"): 1,
        ("src/detail/Utils.cpp", "detail-cpp-not-in-ui-sources"): 1,
        ("src/detail/Log.cpp", "detail-cpp-not-in-ui-sources"): 1,
    }
)


class Violation:
    def __init__(self, rule: str, path: str, line_no: int, text: str) -> None:
        self.rule = rule
        self.path = path
        self.line_no = line_no
        self.text = text.strip()

    def __str__(self) -> str:
        return f"[{self.rule}] {self.path}:{self.line_no}: {self.text}"


def normalized_relative(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def is_under(path: Path, root: Path, prefixes: tuple[Path, ...]) -> bool:
    rel = path.relative_to(root)
    return any(rel == prefix or prefix in rel.parents for prefix in prefixes)


def iter_source_files(root: Path):
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in SOURCE_EXTENSIONS:
            continue
        rel = path.relative_to(root).as_posix()
        if rel.startswith("third_party/") or rel.startswith("build/"):
            continue
        yield path


def count_matches(root: Path):
    api_counts: Counter[tuple[str, str]] = Counter()
    runtime_counts: Counter[tuple[str, str]] = Counter()
    common_counts: Counter[tuple[str, str]] = Counter()
    public_api_forbidden_include_counts: Counter[tuple[str, str]] = Counter()
    public_api_entt_counts: Counter[tuple[str, str]] = Counter()
    api_cpp_runtime_counts: Counter[tuple[str, str]] = Counter()
    api_cpp_entt_entity_counts: Counter[tuple[str, str]] = Counter()
    runtime_current_counts: Counter[tuple[str, str]] = Counter()
    raw_access_counts: Counter[tuple[str, str]] = Counter()
    unlisted_detail_cpp_counts: Counter[tuple[str, str]] = Counter()
    locations: dict[tuple[str, str, str], list[tuple[int, str]]] = {}

    cmake_file = root / "src" / "CMakeLists.txt"
    ui_sources_entries: set[str] = set()
    if cmake_file.exists():
        try:
            cmake_text = cmake_file.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            cmake_text = cmake_file.read_text(encoding="utf-8-sig")
        if match := UI_SOURCES_BLOCK_RE.search(cmake_text):
            ui_sources_entries = set(DETAIL_CPP_ENTRY_RE.findall(match.group(1)))

    for path in root.glob("src/detail/*.cpp"):
        rel = normalized_relative(path, root)
        cmake_entry = rel.removeprefix("src/")
        if cmake_entry not in ui_sources_entries:
            key = (rel, "detail-cpp-not-in-ui-sources")
            unlisted_detail_cpp_counts[key] += 1
            locations.setdefault(("detail-cpp-not-in-ui-sources", *key), []).append((1, cmake_entry))

    for path in iter_source_files(root):
        rel = normalized_relative(path, root)
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            lines = path.read_text(encoding="utf-8-sig").splitlines()

        for line_no, line in enumerate(lines, start=1):
            if is_under(path, root, INTERNAL_API_INCLUDE_DIRS):
                if match := API_INCLUDE_RE.search(line):
                    key = (rel, match.group(0))
                    api_counts[key] += 1
                    locations.setdefault(("api-include", *key), []).append((line_no, line))

            if is_under(path, root, STATIC_RUNTIME_DIRS):
                for match in STATIC_RUNTIME_RE.finditer(line):
                    key = (rel, match.group(0))
                    runtime_counts[key] += 1
                    locations.setdefault(("static-runtime", *key), []).append((line_no, line))

            if is_under(path, root, COMMON_DIRS):
                if match := COMMON_RUNTIME_INCLUDE_RE.search(line):
                    key = (rel, match.group(0))
                    common_counts[key] += 1
                    locations.setdefault(("common-runtime-include", *key), []).append((line_no, line))

            if path.suffix in HEADER_EXTENSIONS and is_under(path, root, PUBLIC_API_HEADER_DIRS):
                if match := PUBLIC_API_FORBIDDEN_INCLUDE_RE.search(line):
                    key = (rel, match.group(0))
                    public_api_forbidden_include_counts[key] += 1
                    locations.setdefault(("public-api-forbidden-include", *key), []).append((line_no, line))
                for match in ENTT_NAMESPACE_RE.finditer(line):
                    key = (rel, match.group(0))
                    public_api_entt_counts[key] += 1
                    locations.setdefault(("public-api-entt", *key), []).append((line_no, line))

            if path.suffix in IMPLEMENTATION_EXTENSIONS and is_under(path, root, API_CPP_DIRS):
                for match in RUNTIME_FACADE_CURRENT_RE.finditer(line):
                    key = (rel, match.group(0))
                    api_cpp_runtime_counts[key] += 1
                    locations.setdefault(("api-cpp-runtime-current", *key), []).append((line_no, line))
                for match in ENTT_ENTITY_RE.finditer(line):
                    key = (rel, match.group(0))
                    api_cpp_entt_entity_counts[key] += 1
                    locations.setdefault(("api-cpp-entt-entity", *key), []).append((line_no, line))

            if is_under(path, root, RUNTIME_ACCESS_DIRS):
                for match in RUNTIME_FACADE_CURRENT_RE.finditer(line):
                    key = (rel, match.group(0))
                    runtime_current_counts[key] += 1
                    locations.setdefault(("runtime-current", *key), []).append((line_no, line))

            if is_under(path, root, RAW_ACCESS_DIRS):
                for match in RAW_ACCESS_RE.finditer(line):
                    key = (rel, match.group(0))
                    raw_access_counts[key] += 1
                    locations.setdefault(("raw-access", *key), []).append((line_no, line))

    return (
        api_counts,
        runtime_counts,
        common_counts,
        public_api_forbidden_include_counts,
        public_api_entt_counts,
        api_cpp_runtime_counts,
        api_cpp_entt_entity_counts,
        runtime_current_counts,
        raw_access_counts,
        unlisted_detail_cpp_counts,
        locations,
    )


def collect_extra(
    rule: str,
    actual: Counter[tuple[str, str]],
    allowed: Counter[tuple[str, str]],
    locations: dict[tuple[str, str, str], list[tuple[int, str]]],
) -> list[Violation]:
    violations: list[Violation] = []
    for key, count in sorted(actual.items()):
        extra = count - allowed.get(key, 0)
        if extra <= 0:
            continue

        path, token = key
        all_locations = locations.get((rule, path, token), [])
        for line_no, line in all_locations[-extra:]:
            violations.append(Violation(rule, path, line_no, line))

    return violations


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Check VMP-ui architecture boundary debt does not grow.")
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="Repository root")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    (
        api_counts,
        runtime_counts,
        common_counts,
        public_api_forbidden_include_counts,
        public_api_entt_counts,
        api_cpp_runtime_counts,
        api_cpp_entt_entity_counts,
        runtime_current_counts,
        raw_access_counts,
        unlisted_detail_cpp_counts,
        locations,
    ) = count_matches(root)

    violations: list[Violation] = []
    violations.extend(collect_extra("api-include", api_counts, ALLOWED_API_INCLUDE_COUNTS, locations))
    violations.extend(collect_extra("static-runtime", runtime_counts, ALLOWED_STATIC_RUNTIME_COUNTS, locations))
    violations.extend(
        collect_extra("common-runtime-include", common_counts, ALLOWED_COMMON_RUNTIME_INCLUDE_COUNTS, locations)
    )
    violations.extend(
        collect_extra("public-api-forbidden-include", public_api_forbidden_include_counts, Counter(), locations)
    )
    violations.extend(collect_extra("public-api-entt", public_api_entt_counts, Counter(), locations))
    violations.extend(
        collect_extra(
            "api-cpp-runtime-current", api_cpp_runtime_counts, ALLOWED_API_CPP_RUNTIME_CURRENT_COUNTS, locations
        )
    )
    violations.extend(
        collect_extra("api-cpp-entt-entity", api_cpp_entt_entity_counts, ALLOWED_API_CPP_ENTT_ENTITY_COUNTS, locations)
    )
    violations.extend(collect_extra("runtime-current", runtime_current_counts, ALLOWED_RUNTIME_CURRENT_COUNTS, locations))
    violations.extend(collect_extra("raw-access", raw_access_counts, ALLOWED_RAW_ACCESS_COUNTS, locations))
    violations.extend(
        collect_extra(
            "detail-cpp-not-in-ui-sources",
            unlisted_detail_cpp_counts,
            ALLOWED_UNLISTED_DETAIL_CPP_COUNTS,
            locations,
        )
    )

    if violations:
        print("Architecture boundary check failed: new boundary debt detected.", file=sys.stderr)
        print("Existing 2026-06-04 debt is baselined; remove entries from the baseline when debt is fixed.", file=sys.stderr)
        for violation in violations:
            print(str(violation), file=sys.stderr)
        return 1

    print("Architecture boundary check passed: no new P0 boundary debt.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
