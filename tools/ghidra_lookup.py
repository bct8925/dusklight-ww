#!/usr/bin/env python3
"""Print functions from a Ghidra C export of main.dol, by name or address.

The export (File > Export Program > C/C++ in Ghidra) is derived from Nintendo's binary: keep it
outside the repo. Point this at it with --export or the GHIDRA_EXPORT environment variable
(default: %USERPROFILE%\\ghidra\\WindWaker.rep\\main.1.c).

    python tools/ghidra_lookup.py dPa_control_c::draw          # functions whose name contains this
    python tools/ghidra_lookup.py --exact dCamera_c::Run       # exact qualified name
    python tools/ghidra_lookup.py 0x8007de94                   # by address (via the decomp's symbols.txt)
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
# Named functions carry no address in the export; map addresses through the decomp's symbols.
SYMBOLS = Path(os.environ.get("TWW_SYMBOLS", r"C:\Users\brian\Dev\tww\config\GZLE01\symbols.txt"))


def symbol_name(address: int):
    """Class::method (or plain name) for a main.dol address, from the decomp's symbols.txt."""
    if not SYMBOLS.exists():
        return None
    for line in SYMBOLS.read_text(encoding="utf-8", errors="replace").splitlines():
        m = re.match(r"(\S+) = \.text:0x([0-9A-Fa-f]+);", line)
        if m and int(m.group(2), 16) == address:
            mangled = m.group(1)
            cm = re.match(r"(\w+?)__(\d+)(\w+)", mangled)
            if cm:
                method, length, rest = cm.group(1), int(cm.group(2)), cm.group(3)
                return f"{rest[:length]}::{method}"
            return mangled.split("__")[0]
    return None
# A function definition starts at column 0 with "<type> <name>(...)" and its body at "{".
DEF = re.compile(r"^(?!\s)(?!//)([^\n;{}]*?)\b([\w:~<>,\s*&]+?)\s*\(([^\n;]*)\)\s*$", re.M)
SIG_COMMENT = re.compile(r"^// (.*)$")


def build_index(path: Path):
    """(name, signature comment, first line, last line) for every function in the export.

    Ghidra writes an optional "// <signature>" comment (possibly wrapped over several "//" lines),
    a blank line, the definition header (long ones wrap onto indented lines), a blank line, then
    "{" in column 0; the body ends at the next "}" in column 0.
    """
    lines = path.read_text(encoding="utf-8", errors="replace").split("\n")
    funcs = []
    n = len(lines)
    i = 0
    while i < n:
        if lines[i] != "{" or i < 2 or lines[i - 1] != "":
            i += 1
            continue
        # Header: the non-blank lines above the blank line before "{".
        h = i - 2
        while h > 0 and lines[h - 1] != "" and not lines[h - 1].startswith("//"):
            h -= 1
        header = " ".join(l.strip() for l in lines[h:i - 1])
        m = re.search(r"([\w~]+(?:::[\w~]+)*)\s*\(", header)
        name = m.group(1) if m else header
        # Comment block above the header (skipping one blank line).
        c = h - 1 if h > 0 and lines[h - 1] == "" else h
        first = h
        comment_lines = []
        while c - 1 >= 0 and lines[c - 1].startswith("//"):
            c -= 1
            comment_lines.insert(0, lines[c][2:].strip())
        if comment_lines:
            first = c
        comment = " ".join(l for l in comment_lines if not l.startswith("WARNING"))
        j = i + 1
        while j < n and lines[j] != "}":
            j += 1
        funcs.append((name, comment, first, j))
        i = j + 1
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
        name = symbol_name(int(q, 16))
        if name:
            print(f"// {q} is {name} in symbols.txt", file=sys.stderr)
            hits = [f for f in funcs if f[0] == name]
        else:
            hits = [f for f in funcs if f[0].lower().endswith("_" + q[-8:].lower())]
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
