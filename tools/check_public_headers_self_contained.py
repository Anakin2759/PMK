#!/usr/bin/env python3
"""Public header self-containment guard for VMP-ui.

Guarantees that every public header under ``include/ui/`` (and the umbrella
``include/ui.hpp``) has zero third-party library dependencies:

- No includes of Eigen / EnTT / SDL3 / spdlog / yoga / freetype / harfbuzz /
  cmrc headers.
- No includes of internal ``src/`` paths or ``../`` escapes.

This is the header-level half of the self-contained distribution contract:
consumers only need the shipped ``include/ui/**`` headers plus one VMPUI
library, and must never be required to add a third-party include directory.
Platform SDK headers (e.g. ``<Windows.h>``) and the C/C++ standard library
are allowed.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path
import sys

# Headers that are allowed: C/C++ standard library and platform SDK only.
ALLOWED_SYSTEM_PREFIXES = (
    "algorithm",
    "array",
    "atomic",
    "bit",
    "cassert",
    "cctype",
    "cerrno",
    "charconv",
    "chrono",
    "cmath",
    "compare",
    "concepts",
    "cstddef",
    "cstdint",
    "cstdio",
    "cstdlib",
    "cstring",
    "ctime",
    "cwchar",
    "deque",
    "exception",
    "expected",
    "filesystem",
    "format",
    "functional",
    "initializer_list",
    "iomanip",
    "ios",
    "iosfwd",
    "iostream",
    "iterator",
    "limits",
    "list",
    "map",
    "memory",
    "mutex",
    "new",
    "numeric",
    "optional",
    "ranges",
    "ratio",
    "set",
    "source_location",
    "span",
    "sstream",
    "stack",
    "stdexcept",
    "string",
    "string_view",
    "system_error",
    "thread",
    "tuple",
    "type_traits",
    "typeinfo",
    "unordered_map",
    "unordered_set",
    "utility",
    "variant",
    "vector",
    "version",
    # Platform SDK (Windows)
    "windows.h",
    "windowsx.h",
    "commctrl.h",
    "dwmapi.h",
    "uxtheme.h",
    "shellapi.h",
    "hidapi.h",
)

# Third-party include prefixes that are forbidden in public headers.
FORBIDDEN_PREFIXES = (
    "Eigen",
    "Eigen3",
    "entt",
    "SDL",
    "SDL3",
    "spdlog",
    "fmt",
    "yoga",
    "freetype",
    "ft2build.h",
    "harfbuzz",
    "hb.h",
    "hb-",
    "cmrc",
    "gtest",
    "benchmark",
    "HFSM2",
    "hfsm2",
)

FORBIDDEN_PREFIX_RE = re.compile(
    r"^\s*#\s*include\s*[<\"]({})[/>]".format("|".join(re.escape(p) for p in FORBIDDEN_PREFIXES)),
    re.IGNORECASE,
)

# Internal source tree escapes: "src/...", "../...", "helper/...", "common/..." etc.
INTERNAL_INCLUDE_RE = re.compile(
    r"^\s*#\s*include\s*[\"](?:\.\./|src/|helper/|detail/|managers/|systems/|core/|api/|common/|utils/|traits/|interface/|renderers/|services/)",
)

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')

HEADER_EXTENSIONS = {".h", ".hh", ".hpp", ".hxx"}


class Violation:
    def __init__(self, rule: str, path: str, line_no: int, text: str) -> None:
        self.rule = rule
        self.path = path
        self.line_no = line_no
        self.text = text.strip()

    def __str__(self) -> str:
        return f"[{self.rule}] {self.path}:{self.line_no}: {self.text}"


def check_public_headers(root: Path) -> list[Violation]:
    violations: list[Violation] = []

    public_root = root / "include"
    candidates: list[Path] = []
    candidates.append(root / "include" / "ui.hpp")
    for path in (public_root / "ui").rglob("*"):
        if path.is_file() and path.suffix in HEADER_EXTENSIONS:
            candidates.append(path)

    for path in sorted(candidates):
        rel = path.relative_to(root).as_posix()
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except UnicodeDecodeError:
            lines = path.read_text(encoding="utf-8-sig").splitlines()

        for line_no, line in enumerate(lines, start=1):
            stripped = line.strip()
            if not stripped.startswith("#include"):
                continue

            if FORBIDDEN_PREFIX_RE.match(line):
                violations.append(Violation("third-party-include", rel, line_no, stripped))

            match = INCLUDE_RE.match(line)
            if match is None:
                continue
            header = match.group(1)

            if INTERNAL_INCLUDE_RE.match(line):
                violations.append(Violation("internal-include", rel, line_no, stripped))
                continue

            # Quoted includes must stay within include/ (they reference ui/*).
            if stripped.startswith('#include "'):
                if not header.startswith("ui/"):
                    violations.append(Violation("non-ui-quoted-include", rel, line_no, stripped))
                continue

            # Angle includes: allow standard library and platform SDK only.
            base = header.split("/")[0].lower()
            if base not in ALLOWED_SYSTEM_PREFIXES:
                violations.append(Violation("unknown-system-include", rel, line_no, stripped))

    return violations


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Check VMP-ui public headers have zero third-party dependencies.")
    parser.add_argument("--root", type=Path, default=Path.cwd(), help="Repository root")
    args = parser.parse_args(argv)

    root = args.root.resolve()
    violations = check_public_headers(root)

    print(f"Public headers checked: include/ui/** + include/ui.hpp ({len(violations)} violations)")
    if violations:
        print("Public header self-containment check failed.", file=sys.stderr)
        print("Public headers must not include third-party or internal src/ headers.", file=sys.stderr)
        for violation in violations:
            print(str(violation), file=sys.stderr)
        return 1

    print("Public header self-containment check passed: zero third-party dependencies.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
