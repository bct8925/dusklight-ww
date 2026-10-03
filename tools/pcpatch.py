"""Helpers for applying TARGET_PC edits to decomp files, keeping the original code.

Use from a script or `python -c`, with paths relative to the repo root:

    import sys; sys.path.insert(0, "tools")
    from pcpatch import pc, pc_range, stub_body, rep
    pc("tww/src/d/d_foo.cpp", "    old line;", "    new line;")

Edits keep each file's line endings and fail loudly if the old text is not found exactly.
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _load(path):
    full = os.path.join(ROOT, path)
    data = open(full, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in data else "\n"
    return full, data, nl


def rep(path, old, new, count=1):
    """Plain replacement (old/new use \\n; converted to the file's line endings)."""
    full, data, nl = _load(path)
    old, new = old.replace("\n", nl), new.replace("\n", nl)
    n = data.count(old)
    if n != count:
        raise SystemExit(f"{path}: expected {count} match(es), found {n}: {old[:80]!r}")
    open(full, "w", encoding="utf-8", newline="").write(data.replace(old, new))


def pc(path, old, new, count=1):
    """Replace whole lines `old` with `#if TARGET_PC / new / #else / old / #endif`."""
    old_block = old if old.endswith("\n") else old + "\n"
    new_block = new if new.endswith("\n") else new + "\n"
    rep(path, old_block, "#if TARGET_PC\n" + new_block + "#else\n" + old_block + "#endif\n", count)


def stub_body(path, signature, pc_body=None, reason="PowerPC-only"):
    """Compile a function's body out on PC. `signature` is the full line ending in '{'.
    `pc_body` (optional) is code used on PC instead, e.g. 'return false;'."""
    full, data, nl = _load(path)
    sig = signature + nl
    if data.count(sig) != 1:
        raise SystemExit(f"{path}: signature not unique/found: {signature!r}")
    start = data.index(sig) + len(sig)
    depth, i = 1, start
    while depth:
        c = data[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
        i += 1
    close = i - 1  # index of the function's closing brace
    body = data[start:close]
    if pc_body:
        new = (f"#if TARGET_PC{nl}    // {reason}{nl}    {pc_body}{nl}#else{nl}" + body + f"#endif{nl}")
    else:
        new = f"#if !TARGET_PC  // {reason}{nl}" + body + f"#endif{nl}"
    data = data[:start] + new + data[close:]
    open(full, "w", encoding="utf-8", newline="").write(data)


def pc_range(path, start, end, subs):
    """Lines start..end (1-based, inclusive): PC copy with regex `subs` applied, original in #else.
    Apply ranges in a file from the bottom up so line numbers stay valid."""
    import re
    full, data, nl = _load(path)
    lines = data.split(nl)
    orig = lines[start - 1:end]
    new = []
    for line in orig:
        for pat, rpl in subs:
            line = re.sub(pat, rpl, line)
        new.append(line)
    if new == orig:
        raise SystemExit(f"{path}:{start}-{end}: substitutions changed nothing")
    lines[start - 1:end] = ["#if TARGET_PC"] + new + ["#else"] + orig + ["#endif"]
    open(full, "w", encoding="utf-8", newline="").write(nl.join(lines))
