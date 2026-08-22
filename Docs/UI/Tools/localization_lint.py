#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Blocking localization lint for the Scarlet Horizon interface.

Catches the two defect classes that only surface once text is gathered, long
after the code that caused them was written:

1. Key collisions. In Unreal a namespace + key pair identifies one piece of text.
   When two different source strings share a pair, GatherText resolves the
   collision silently and one of the two labels renders the wrong words in a
   localized build. This is invisible while running from source, because there
   the literal in the macro is used directly.

2. Transliterated Russian. Latin spellings of Russian words ("ZVUK", "KREDITY",
   "PROIZVODSTVO") pass every compiler and every English spell check, and they
   look deliberate next to genuinely Latin names, so they survive review.

Usage:
    python3 Docs/UI/Tools/localization_lint.py [--root .]

Exit code 0 when clean, 1 when a defect is found.
"""

import argparse
import pathlib
import re
import sys
from collections import defaultdict

# NSLOCTEXT("ns", "key", "source") and LOCTEXT("key", "source"), source may span
# escaped quotes.
STR = r'"((?:[^"\\]|\\.)*)"'
NSLOCTEXT = re.compile(r'\bNSLOCTEXT\(\s*' + STR + r'\s*,\s*' + STR + r'\s*,\s*' + STR)
LOCTEXT = re.compile(r'(?<!NS)\bLOCTEXT\(\s*' + STR + r'\s*,\s*' + STR)
NAMESPACE = re.compile(r'#define\s+LOCTEXT_NAMESPACE\s+"([^"]+)"')

# Latin words that are Russian spelled in Latin letters. Genuine Latin content in
# this project is names, doctrine titles and technical acronyms, so the list
# targets syllable patterns that only appear in transliteration.
TRANSLIT = re.compile(
    r'\b('
    r'ZVUK|IGRA|VOYSKA|VOJSKA|UPRAVLENIE|KREDITY|PROIZVODSTVO|EFFEKTY|MUZYKA|'
    r'MASHTAB|MASHQ|INTERFEYSA|OPERATIVNYE|DANNYE|NASTROYKI|REDAKTOR|UROVEN|'
    r'SVODKA|NOVOSTEY|SOOBSHCHENIE|POBEDA|PORAZHENIE|OCHERED|BRONYA|TYAZH'
    r')\b',
    re.IGNORECASE,
)


def iter_sources(root: pathlib.Path):
    for path in sorted(root.rglob("*")):
        if path.suffix not in (".cpp", ".h"):
            continue
        parts = set(path.parts)
        if "Intermediate" in parts or "ThirdParty" in parts:
            continue
        yield path


def scan(root: pathlib.Path):
    """Returns (collisions, translit) findings."""
    # (namespace, key) -> {source: [locations]}
    entries = defaultdict(lambda: defaultdict(list))
    translit = []

    for path in iter_sources(root):
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue

        rel = path.relative_to(root)
        # LOCTEXT resolves against the namespace in force at that point.
        namespaces = [(m.start(), m.group(1)) for m in NAMESPACE.finditer(text)]

        def namespace_at(pos: int) -> str:
            current = ""
            for start, name in namespaces:
                if start < pos:
                    current = name
                else:
                    break
            return current

        for m in NSLOCTEXT.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            entries[(m.group(1), m.group(2))][m.group(3)].append(f"{rel}({line})")

        for m in LOCTEXT.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            ns = namespace_at(m.start())
            entries[(ns, m.group(1))][m.group(2)].append(f"{rel}({line})")

        for m in TRANSLIT.finditer(text):
            # Only report inside a localized string, not in identifiers.
            line_start = text.rfind("\n", 0, m.start()) + 1
            line_end = text.find("\n", m.end())
            line_text = text[line_start:line_end if line_end != -1 else len(text)]
            if "LOCTEXT(" in line_text:
                line = text.count("\n", 0, m.start()) + 1
                translit.append((f"{rel}({line})", m.group(1), line_text.strip()))

    collisions = {k: v for k, v in entries.items() if len(v) > 1}
    return collisions, translit


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=".", help="repository root")
    args = parser.parse_args()

    root = pathlib.Path(args.root).resolve()
    source_root = root / "Source"
    if not source_root.is_dir():
        print(f"localization-lint: no Source directory under {root}", file=sys.stderr)
        return 1

    collisions, translit = scan(source_root)

    for (ns, key), sources in sorted(collisions.items()):
        print(f"COLLISION  namespace '{ns}' key '{key}' names {len(sources)} different strings:")
        for source, locations in sources.items():
            print(f"           \"{source}\"  at {', '.join(locations)}")

    for location, word, line in translit:
        print(f"TRANSLIT   {location}: '{word}' is Russian spelled in Latin letters")
        print(f"           {line}")

    total = len(collisions) + len(translit)
    if total == 0:
        print("localization-lint: clean")
        return 0
    print(f"\nlocalization-lint: {len(collisions)} key collision(s), {len(translit)} transliteration(s)")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
