#!/usr/bin/env python3
"""Give the decomp's empty non-void functions a PC body.

Functions the decomp has not decompiled yet are emitted as

    RetType Class::func(args) {
        /* Nonmatching */
    }

On PowerPC that returned whatever was in r3/f1; MSVC rejects it (C4716). This adds, on PC only,
a call that logs the function the first time it runs and a value-initialized return:

    RetType Class::func(args) {
        /* Nonmatching */
    #if TARGET_PC
        PC_EMPTY_STUB();
        return {};
    #endif
    }

With --all, void functions, constructors and destructors get the logging call too, so a run
reports every undecompiled function it reaches (the log and --trace's trace.txt name them).
Running it again changes nothing.

    python tools/pc_stub_empty.py [--all] tww/src/d/d_camera.cpp [more files...]
"""

import re
import sys
from pathlib import Path

STUB = re.compile(
    r"^(?P<sig>[^\n]*\)[^\n;{]*\{)(?P<nl>\r?\n)(?P<indent>[ \t]*)/\* Nonmatching \*/\r?\n\}",
    re.M,
)


def returns_value(signature: str) -> bool:
    head = signature.split("(", 1)[0].strip()  # e.g. "bool dCamera_c::pauseEvCamera"
    words = head.replace("*", " * ").replace("&", " & ").split()
    if len(words) < 2:
        return False  # no return type: constructor, destructor or macro
    name = words[-1]
    if "~" in name or (("::" in name) and name.split("::")[-1] == name.split("::")[-2]):
        return False
    return_type = [w for w in words[:-1] if w not in ("static", "inline", "virtual", "extern", '"C"')]
    return bool(return_type) and return_type != ["void"]


def process(path: Path, all_functions: bool) -> int:
    with open(path, encoding="utf-8", errors="surrogateescape", newline="") as f:
        text = f.read()
    count = 0

    def fix(m):
        nonlocal count
        nl, ind = m.group("nl"), m.group("indent") or "    "
        if not returns_value(m.group("sig")):
            if not all_functions:
                return m.group(0)
            count += 1
            return (f"{m.group('sig')}{nl}{ind}/* Nonmatching */{nl}#if TARGET_PC{nl}"
                    f"{ind}PC_EMPTY_STUB();{nl}#endif{nl}}}")
        count += 1
        return (f"{m.group('sig')}{nl}{ind}/* Nonmatching */{nl}#if TARGET_PC{nl}"
                f"{ind}PC_EMPTY_STUB();{nl}{ind}return {{}};{nl}#endif{nl}}}")

    new = STUB.sub(fix, text)
    if count:
        with open(path, "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
            f.write(new)
    return count


def main() -> int:
    args = sys.argv[1:]
    all_functions = "--all" in args
    args = [a for a in args if a != "--all"]
    total = 0
    for arg in args:
        n = process(Path(arg), all_functions)
        if n:
            print(f"{arg}: {n}")
        total += n
    print(f"{total} empty functions given a PC body")
    return 0


if __name__ == "__main__":
    sys.exit(main())
