#!/usr/bin/env python3
"""Port dusklight's big-endian field annotations onto the matching structs of a TWW header.

Data read in place from GameCube files is big-endian; dusklight marks those fields BE(T), BE(T)*
or OFFSET_PTR_V0 (a 32-bit file offset). TWW's JSystem headers declare the same structs, often
with different field names, so this lines up each struct with dusklight's by name and then each
field by its /* 0xNN */ offset comment (or by name), and rewrites TWW's plain type where
dusklight's is the endian-aware one. Every change is printed; review the diff afterwards.

    python tools/port_be_fields.py tww/include/JSystem/J3DGraphBase/J3DStruct.h [...]
        [--ref dusklight-upstream/main] [--dry-run]

The dusklight path is derived from the TWW one (include/JSystem/... -> libs/JSystem/include/
JSystem/..., otherwise the same path) and read from the ref with git show.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STRUCT = re.compile(r"^(struct|class)\s+(\w+)\b[^;{]*\{", re.M)
FIELD = re.compile(r"^(?P<indent>\s*)(?P<off>/\*\s*(?P<offv>0x[0-9A-Fa-f]+)\s*\*/\s*)?"
                   r"(?P<type>(?:const\s+)?[A-Za-z_][\w:]*(?:\([\w:]+\))?(?:<[^;{}()]*>)?\s*\**)\s*"
                   r"(?P<name>[A-Za-z_]\w*)(?P<arr>\s*\[[^\]]*\])*\s*;", re.M)
ENDIAN_AWARE = re.compile(r"\bBE\(|\bBE<|OFFSET_PTR")
# Runtime classes differ between TWW and TP, so offsets do not line up; --classes includes them.
INCLUDE_CLASSES = False
SKIP_TYPES = {"return", "typedef", "using", "friend", "static", "virtual", "enum"}


def dusklight_path(tww_path: str) -> str:
    tww_path = tww_path.removeprefix("tww/")
    if tww_path.startswith("include/JSystem/"):
        return "libs/JSystem/" + tww_path
    if tww_path.startswith("src/JSystem/"):
        return "libs/JSystem/src/" + tww_path[len("src/JSystem/"):]
    return tww_path


def struct_bodies(text: str) -> dict[str, tuple[int, int]]:
    """name -> (start, end) of the body of each top-level or nested struct/class."""
    out = {}
    for m in STRUCT.finditer(text):
        depth, i = 1, m.end()
        while depth and i < len(text):
            depth += {"{": 1, "}": -1}.get(text[i], 0)
            i += 1
        if m.group(1) == "struct" or INCLUDE_CLASSES:
            out.setdefault(m.group(2), (m.end(), i - 1))
    return out


def fields(body: str) -> list[re.Match]:
    result = []
    depth = 0
    pos = 0
    # Only direct members: skip nested braces (inline functions, nested types).
    for m in FIELD.finditer(body):
        depth += body.count("{", pos, m.start()) - body.count("}", pos, m.start())
        pos = m.start()
        if depth != 0:
            continue
        t = m.group("type").strip()
        if t.split()[0] in SKIP_TYPES or "(" in body[m.start():m.end()].replace(m.group("type"), ""):
            continue
        result.append(m)
    return result


def map_type(ours: str, theirs: str):
    """Apply dusklight's kind of annotation to our own type (TP's field types can differ)."""
    ours = ours.replace(" ", "")
    pointer = ours.endswith("*")
    base = ours.rstrip("*")
    if theirs.startswith("OFFSET_PTR(") or theirs.startswith("OFFSET_PTR_RAW"):
        # Relocated in place by OffsetPtr::setBase; keep it typed when ours is a pointer.
        if pointer:
            return f"OFFSET_PTR({base})" if base != "void" else "OFFSET_PTR_RAW"
        return theirs
    if theirs == "OFFSET_PTR_V0":
        return "OFFSET_PTR_V0"
    if theirs.rstrip(" *").startswith(("BE(", "BE<")):
        if theirs.endswith("*") != pointer:
            return None
        return f"BE({base})*" if pointer else f"BE({base})"
    return None


def port(tww_rel: str, ref: str, dry: bool) -> int:
    path = ROOT / tww_rel
    text = path.read_bytes().decode("utf-8")
    try:
        ref_text = subprocess.run(["git", "show", f"{ref}:{dusklight_path(tww_rel)}"], cwd=ROOT,
                                  check=True, capture_output=True).stdout.decode("utf-8")
    except subprocess.CalledProcessError:
        print(f"{tww_rel}: no dusklight counterpart at {dusklight_path(tww_rel)}", file=sys.stderr)
        return 0
    ref_structs = struct_bodies(ref_text)
    edits = []
    for name, (s, e) in struct_bodies(text).items():
        if name not in ref_structs:
            continue
        rs, re_ = ref_structs[name]
        ref_fields = fields(ref_text[rs:re_])
        by_off = {m.group("offv").lower(): m for m in ref_fields if m.group("offv")}
        by_name = {m.group("name"): m for m in ref_fields}
        for m in fields(text[s:e]):
            ref = None
            if m.group("offv") and m.group("offv").lower() in by_off:
                ref = by_off[m.group("offv").lower()]
            elif m.group("name") in by_name:
                ref = by_name[m.group("name")]
            if ref is None:
                continue
            ours, theirs = m.group("type").strip(), ref.group("type").strip()
            if ENDIAN_AWARE.search(ours) or not ENDIAN_AWARE.search(theirs):
                continue
            if bool(m.group("arr")) != bool(ref.group("arr")):
                # Same offset but array vs. scalar; leave it for a human.
                print(f"  ? {name}::{m.group('name')}: {ours}{m.group('arr') or ''} vs {theirs}{ref.group('arr') or ''}")
                continue
            new = map_type(ours, theirs)
            if new is None:
                print(f"  ? {name}::{m.group('name')}: {ours} vs {theirs}")
                continue
            ts, te = s + m.start("type"), s + m.end("type")
            edits.append((ts, te, new + (" " if not new.endswith("*") else ""), f"{name}::{m.group('name')}: {ours} -> {new}"))
    for *_, msg in edits:
        print("  " + msg)
    if edits and not dry:
        for ts, te, new, _ in sorted(edits, reverse=True):
            text = text[:ts] + new + text[te:].lstrip(" ")
        path.write_bytes(text.encode("utf-8"))
    return len(edits)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("headers", nargs="+")
    ap.add_argument("--ref", default="dusklight-upstream/main")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--classes", action="store_true", help="also match classes (review carefully)")
    args = ap.parse_args()
    global INCLUDE_CLASSES
    INCLUDE_CLASSES = args.classes
    total = 0
    for h in args.headers:
        print(h)
        total += port(Path(h).as_posix(), args.ref, args.dry_run)
    print(f"{total} fields {'would change' if args.dry_run else 'changed'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
