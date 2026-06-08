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

INTERNAL_API_INCLUDE_DIRS = (
    Path("src/core"),
    Path("src/detail"),
    Path("src/systems"),
    Path("src/renderers"),
    Path("src/services"),
)
STATIC_RUNTIME_DIRS = (Path("src/systems"),)
COMMON_DIRS = (Path("src/common"),)
SOURCE_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx", ".c", ".cc", ".cpp", ".cxx"}

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
    locations: dict[tuple[str, str, str], list[tuple[int, str]]] = {}

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

    return api_counts, runtime_counts, common_counts, locations


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
    api_counts, runtime_counts, common_counts, locations = count_matches(root)

    violations: list[Violation] = []
    violations.extend(collect_extra("api-include", api_counts, ALLOWED_API_INCLUDE_COUNTS, locations))
    violations.extend(collect_extra("static-runtime", runtime_counts, ALLOWED_STATIC_RUNTIME_COUNTS, locations))
    violations.extend(
        collect_extra("common-runtime-include", common_counts, ALLOWED_COMMON_RUNTIME_INCLUDE_COUNTS, locations)
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
