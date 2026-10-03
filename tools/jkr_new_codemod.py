#!/usr/bin/env python3
"""Rewrite the game's new/delete expressions to dusklight's JKR_NEW/JKR_DELETE macros.

On PC the global operator new/delete are the C runtime's; game allocations reach the JKR heaps
through overloads taking a JKRHeapToken (include/JSystem/JKernel/JKRHeap.h). On the GameCube the
macros expand back to plain new/delete, so the rewrite is made in place:

    new T(...)               -> JKR_NEW T(...)
    new (heap, align) T(...) -> JKR_NEW_ARGS(heap, align) T(...)
    new (0x20) T(...)        -> JKR_NEW_ARGS(0x20) T(...)
    new T[n]                 -> JKR_NEW_ARRAY(T, n)
    new (heap, align) T[n]   -> JKR_NEW_ARRAY_ARGS(T, n, heap, align)
    delete p                 -> JKR_DELETE(p)
    delete[] p               -> JKR_DELETE_ARRAY(p)

Left alone: comments and strings, `operator new/delete`, `= delete`, and placement new into
memory (`new (ptr) T`, a single non-numeric argument). Every game new and delete must be converted
together: JKR_DELETE frees memory it does not find in a JKR heap with _aligned_free.

    python tools/jkr_new_codemod.py [paths...]   (default: src include, minus src/dusk)
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXTS = {".cpp", ".h", ".hpp", ".inc", ".c"}
SKIP = ("src/dusk/", "include/dusk/", "src/JSystem/JKernel/JKRHeap.cpp", "include/JSystem/JKernel/JKRHeap.h",
        "include/JSystem/JKernel/JKRNew.h")
NEW = re.compile(r"\bnew\b")
DELETE = re.compile(r"\bdelete\b")


def mask(text: str) -> str:
    """Blank out comments and string/char literals, keeping offsets."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            j += 1
            for k in range(i + 1, min(j - 1, n)):
                out[k] = " "
            i = j
            continue
        else:
            i += 1
            continue
        for k in range(i, j):
            if out[k] != "\n":
                out[k] = " "
        i = j
    return "".join(out)


def skip_ws(m: str, i: int) -> int:
    while i < len(m) and m[i] in " \t\r\n":
        i += 1
    return i


def balanced(m: str, i: int, open_: str, close: str) -> int:
    """m[i] == open_; return the index just past its matching close."""
    depth = 0
    while i < len(m):
        if m[i] in "([{<" and m[i] == open_ or (open_ != "<" and m[i] in "([{"):
            depth += 1
        elif m[i] == close or (open_ != "<" and m[i] in ")]}"):
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    raise ValueError("unbalanced")


def split_args(s: str) -> list[str]:
    parts, depth, cur = [], 0, ""
    for c in s:
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += c
    if cur.strip():
        parts.append(cur.strip())
    return parts


def parse_type(m: str, i: int) -> int:
    """Return the end of the type-id starting at i (identifiers, ::, template args, *, const)."""
    while True:
        i = skip_ws(m, i)
        if m.startswith("::", i):
            i += 2
            continue
        mt = re.compile(r"[A-Za-z_]\w*").match(m, i)
        if mt:
            i = mt.end()
            j = skip_ws(m, i)
            if j < len(m) and m[j] == "<":
                i = balanced(m, j, "<", ">")
            continue
        if i < len(m) and m[i] == "*":
            i += 1
            continue
        return i


def is_placement(args: list[str]) -> bool:
    if len(args) != 1:
        return False
    a = args[0]
    return not re.fullmatch(r"-?(0x[0-9A-Fa-f]+|\d+)[uUlL]*", a)


def convert(text: str) -> tuple[str, int]:
    m = mask(text)
    edits = []  # (start, end, replacement)

    for mo in NEW.finditer(m):
        s = mo.start()
        if re.search(r"operator\s*$", m[:s]) or m[s - 1:s] in ("_",):
            continue
        i = skip_ws(m, mo.end())
        args = None
        if i < len(m) and m[i] == "(":
            end = balanced(m, i, "(", ")")
            args = split_args(text[i + 1:end - 1])
            if is_placement(args):
                continue
            i = skip_ws(m, end)
        tstart = i
        tend = parse_type(m, i)
        if tend == tstart:
            continue  # `new (T)` style or unparsable; leave for the compiler to flag
        j = skip_ws(m, tend)
        type_ = " ".join(text[tstart:tend].split())
        if j < len(m) and m[j] == "[":
            bend = balanced(m, j, "[", "]")
            count = text[j + 1:bend - 1].strip()
            if args:
                rep = f"JKR_NEW_ARRAY_ARGS({type_}, {count}, {', '.join(args)})"
            else:
                rep = f"JKR_NEW_ARRAY({type_}, {count})"
            edits.append((s, bend, rep))
        else:
            head = f"JKR_NEW_ARGS({', '.join(args)}) " if args else "JKR_NEW "
            edits.append((s, tstart, head))

    for mo in DELETE.finditer(m):
        s = mo.start()
        before = m[:s].rstrip()
        if before.endswith("operator") or before.endswith("="):
            continue
        i = skip_ws(m, mo.end())
        array = False
        if m.startswith("[", i):
            i = skip_ws(m, m.index("]", i) + 1)
            array = True
        # Operand: up to ';' or ',' at depth 0, or an unmatched closing bracket.
        j, depth = i, 0
        while j < len(m):
            c = m[j]
            if c in "([{":
                depth += 1
            elif c in ")]}":
                if depth == 0:
                    break
                depth -= 1
            elif c in ";," and depth == 0:
                break
            j += 1
        operand = text[i:j].rstrip()
        k = i + len(operand)
        macro = "JKR_DELETE_ARRAY" if array else "JKR_DELETE"
        edits.append((s, k, f"{macro}({operand})"))

    if not edits:
        return text, 0
    edits.sort()
    out, last = [], 0
    for s, e, r in edits:
        if s < last:
            raise ValueError(f"overlapping edits at {s}")
        out.append(text[last:s])
        out.append(r)
        last = e
    out.append(text[last:])
    return "".join(out), len(edits)


def main() -> int:
    roots = [Path(p) for p in sys.argv[1:]] or [ROOT / "tww" / "src", ROOT / "tww" / "include"]
    total = files = 0
    for root in roots:
        paths = [root] if root.is_file() else sorted(root.rglob("*"))
        for path in paths:
            rel = path.resolve().relative_to(ROOT).as_posix().removeprefix("tww/")
            if path.suffix not in EXTS or rel.startswith(SKIP):
                continue
            raw = path.read_bytes().decode("utf-8", errors="surrogateescape")
            try:
                new, n = convert(raw)
            except ValueError as e:
                print(f"error: {rel}: {e}", file=sys.stderr)
                return 1
            if n:
                path.write_bytes(new.encode("utf-8", errors="surrogateescape"))
                total += n
                files += 1
    print(f"rewrote {total} new/delete expressions in {files} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
