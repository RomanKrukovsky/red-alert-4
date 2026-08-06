"""Replace transliterated Russian in LOCTEXT literals with the real Cyrillic.

WHY THIS EXISTS
---------------
The UI source declares its Russian strings in Latin transliteration:

    LOCTEXT("Campaign", "KAMPANIYa")
    LOCTEXT("FactionLabel", "SOVETSKOE VERKhOVNOE KOMANDOVANIE")

so the game shows "KAMPANIYa" instead of "КАМПАНИЯ". The .locres files hold
correct Cyrillic and the fonts carry 255 Cyrillic glyphs each, so the defect is
purely in the source literals.

WHY IT USES THE .po AND NOT A TRANSLITERATION TABLE
---------------------------------------------------
A rule-based Latin->Cyrillic converter was tried first and produced wrong text,
because this transliteration is lossy and ambiguous:

    ISPYTANIYa  -> ИСПЙТАНИЯ   (Y is Ы here, but Й elsewhere)
    SOVETSKOE   -> СОВЕЦКОЕ    ("TS" spans a syllable boundary, it is not Ц)
    ENERGIYa    -> ЕНЕРГИЯ     (leading E is Э)
    VYYTI       -> ВЫТИ        (YY is Ы+Й, not one letter)

No ordering of rules fixes all four at once. Content/Localization/Game/ru/Game.po
already contains 244 correct Cyrillic strings keyed by namespace and key, so this
script looks the real text up instead of deriving it. Anything without a match is
reported and left alone rather than guessed at.

WHAT IT TOUCHES
---------------
Only the display text of LOCTEXT / NSLOCTEXT. Keys are left alone: they are the
identifiers the gathered manifest references, and rewriting them would orphan
every existing translation.

Usage:
    python3 Tools/Localization/fix_translit.py --check
    python3 Tools/Localization/fix_translit.py --apply
"""

import argparse
import glob
import os
import re
import sys

PO_PATH = "Content/Localization/Game/ru/Game.po"

LOCTEXT_RE = re.compile(
    r'(?P<head>(?P<macro>LOCTEXT|NSLOCTEXT)\s*\(\s*'
    r'(?P<ns>"(?:[^"\\]|\\.)*"\s*,\s*)?'
    r'"(?P<key>(?:[^"\\]|\\.)*)"\s*,\s*)'
    r'"(?P<text>(?:[^"\\]|\\.)*)"'
)

# Namespace declared per file via #define LOCTEXT_NAMESPACE "..."
NAMESPACE_RE = re.compile(r'#define\s+LOCTEXT_NAMESPACE\s+"([^"]+)"')

TRANSLIT_HINT = re.compile(r'(Ya|Yu|Kh|Zh|Ch|Sh|Ts|YO|EH|Shch)')


def is_cyrillic(text):
    return any("Ѐ" <= c <= "ӿ" for c in text)


def looks_transliterated(text):
    """Latin text carrying this scheme's multi-letter markers and mixed case."""
    if not text.strip() or is_cyrillic(text):
        return False
    if not TRANSLIT_HINT.search(text):
        return False
    return text.upper() != text.lower()


def load_po():
    """Map both (namespace, key) and bare key to the Cyrillic source string.

    msgctxt in this project's .po is "Namespace,Key". The bare-key map is a
    fallback for files whose namespace was renamed after the .po was gathered;
    it is only consulted when the qualified lookup misses, and only when the key
    is unambiguous across namespaces.
    """
    if not os.path.exists(PO_PATH):
        print(f"error: {PO_PATH} not found; cannot recover Cyrillic text")
        return {}, {}

    with open(PO_PATH, encoding="utf-8") as handle:
        blob = handle.read()

    entries = re.findall(
        r'msgctxt "([^"]*)"\nmsgid "((?:[^"\\]|\\.)*)"',
        blob,
    )

    qualified = {}
    by_key = {}
    clashes = set()
    for ctx, msgid in entries:
        if not is_cyrillic(msgid):
            continue
        if "," in ctx:
            namespace, _, key = ctx.partition(",")
            qualified[(namespace.strip(), key.strip())] = msgid
            bare = key.strip()
        else:
            bare = ctx.strip()
        if bare in by_key and by_key[bare] != msgid:
            clashes.add(bare)
        by_key[bare] = msgid

    for key in clashes:
        by_key.pop(key, None)      # ambiguous: refuse to guess

    return qualified, by_key


def process(path, qualified, by_key, apply_changes):
    with open(path, encoding="utf-8") as handle:
        source = handle.read()

    ns_match = NAMESPACE_RE.search(source)
    file_ns = ns_match.group(1) if ns_match else None

    fixed, missed = [], []

    def replace(match):
        text = match.group("text")
        if not looks_transliterated(text):
            return match.group(0)

        key = match.group("key")
        namespace = file_ns
        if match.group("ns"):
            namespace = match.group("ns").strip().strip(",").strip().strip('"')

        real = qualified.get((namespace, key)) or by_key.get(key)
        if real is None:
            missed.append((key, text))
            return match.group(0)

        fixed.append((text, real))
        return f'{match.group("head")}"{real}"'

    updated = LOCTEXT_RE.sub(replace, source)

    if apply_changes and fixed:
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(updated)

    return fixed, missed


def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--check", action="store_true")
    group.add_argument("--apply", action="store_true")
    parser.add_argument("--show", type=int, default=8)
    args = parser.parse_args()

    qualified, by_key = load_po()
    if not qualified and not by_key:
        return 1
    print(f"loaded {len(qualified)} qualified and {len(by_key)} bare keys from the .po")

    files = sorted(
        glob.glob("Source/**/*.cpp", recursive=True)
        + glob.glob("Source/**/*.h", recursive=True)
    )

    total_fixed = total_missed = 0
    for path in files:
        fixed, missed = process(path, qualified, by_key, args.apply)
        if not fixed and not missed:
            continue
        total_fixed += len(fixed)
        total_missed += len(missed)
        print(f"\n{os.path.basename(path)}: {len(fixed)} recovered, {len(missed)} unmatched")
        for before, after in fixed[: args.show]:
            print(f"    {before[:44]!r}\n -> {after[:44]!r}")
        for key, text in missed[: args.show]:
            print(f"    [no .po entry] {key}: {text[:44]!r}")

    verb = "rewrote" if args.apply else "would rewrite"
    print(f"\n{verb} {total_fixed} strings; {total_missed} had no .po entry")
    if total_missed:
        print("Unmatched strings were left untouched rather than guessed at.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
