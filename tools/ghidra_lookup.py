#!/usr/bin/env python3
"""Print functions from a Ghidra C export of main.dol, by name or address.

The export (File > Export Program > C/C++ in Ghidra) is derived from Nintendo's binary: keep it
outside the repo. Point this at it with --export or the GHIDRA_EXPORT environment variable
(default: %USERPROFILE%\\ghidra\\WindWaker.rep\\main.1.c).

    python tools/ghidra_lookup.py dPa_control_c::draw          # functions whose name contains this
    python tools/ghidra_lookup.py --exact dCamera_c::Run       # exact qualified name
    python tools/ghidra_lookup.py 0x8007b3a0                   # by address (FUN_/LAB_ names)
    python tools/ghidra_lookup.py --list dEvent_manager_c::    # names only

The decompiled code is a reference for writing functional (non-matching) C++ on PC; it uses
Ghidra's field names (field30_0x30), so map offsets back to the decomp's member names.
"""

import argparse
import os
import pickle
import re
import sys
from pathlib import Path

DEFAULT = Path(os.environ.get("USERPROFILE", "~")) / "ghidra" / "WindWaker.rep" / "main.1.c"
# A function definition starts at column 0 with "<type> <name>(...)" and its body at "{".
DEF = re.compile(r"^(?!\s)(?!//)([^\n;{}]*?)\b([\w:~<>,\s*&]+?)\s*\(([^\n;]*)\)\s*$", re.M)
SIG_COMMENT = re.compile(r"^// (.*)$")


def build_index(path: Path):
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.split("\n")
    funcs = []  # (name, signature comment, start line, end line)
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if line and not line[0].isspace() and line.endswith(")") and not line.startswith("//") \
                and i + 2 < n and lines[i + 1] == "" and lines[i + 2] == "{":
            m = re.match(r"^(.*?)([\w:~]+(?:<[^()]*>)?(?:::[\w~]+)*)\s*\(", line)
            name = m.group(2) if m else line
            # Ghidra writes "// <signature>", a blank line, then the definition.
            c = i - 2 if i >= 2 and lines[i - 1] == "" else i - 1
            comment = lines[c][3:] if c >= 0 and lines[c].startswith("// ") else ""
            # The body ends at the first "}" in column 0 (inner braces are indented; brace
            # counting breaks on string literals).
            j = i + 3
            while j < n and lines[j] != "}":
                j += 1
            funcs.append((name, comment, c if comment else i, j))
            i = j + 1
            continue
        i += 1
    return funcs


def load(path: Path):
    cache = path.with_suffix(path.suffix + ".index")
    if cache.exists() and cache.stat().st_mtime >= path.stat().st_mtime:
        with open(cache, "rb") as f:
            return pickle.load(f)
    funcs = build_index(path)
    with open(cache, "wb") as f:
        pickle.dump(funcs, f)
    return funcs


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("query")
    ap.add_argument("--export", default=os.environ.get("GHIDRA_EXPORT", str(DEFAULT)))
    ap.add_argument("--exact", action="store_true")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--max", type=int, default=5)
    args = ap.parse_args()
    path = Path(args.export)
    funcs = load(path)

    q = args.query
    if re.fullmatch(r"(0x)?8[0-9a-fA-F]{7}", q):
        q = "_" + q[-8:].lower()
        hits = [f for f in funcs if f[0].lower().endswith(q)]
    elif args.exact:
        hits = [f for f in funcs if f[0] == q]
    else:
        hits = [f for f in funcs if q in f[0] or q in f[1]]
    if not hits:
        print("no match", file=sys.stderr)
        return 1
    if args.list:
        for name, comment, start, end in hits:
            print(f"{name}  ({end - start + 1} lines)  {comment}")
        return 0
    lines = path.read_text(encoding="utf-8", errors="replace").split("\n")
    for name, comment, start, end in hits[:args.max]:
        print("\n".join(lines[start:end + 1]))
        print()
    if len(hits) > args.max:
        print(f"... {len(hits) - args.max} more (use --list or --max)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
