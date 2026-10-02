#!/usr/bin/env python3
"""Summarize an MSVC/Ninja build log: failed files, the first error of each, and the most common
error sites. Run the build with `-- -k 0` so every failure is in the log.

    python tools/build_errors.py build.log [count]
"""
import collections
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__))).replace("\\", "/") + "/"
ERROR = r"([\w./:-]+\.(?:h|hpp|cpp|c|inc|pch))\((\d+)\)\s?: (?:fatal )?error (C\d+): (.*)"

log = open(sys.argv[1], encoding="utf-8", errors="replace").read().replace("\\", "/")
top = int(sys.argv[2]) if len(sys.argv) > 2 else 30
failed = sorted(set(re.findall(r"FAILED: .*?((?:src|libs)/\S+?\.(?:cpp|c))\.obj", log)))
built = len(re.findall(r"Building (?:CXX|C) object", log))
errs = re.findall(ERROR, log)
print(f"compile steps {built}, failed TUs {len(failed)}, error lines {len(errs)}")


def norm(p):
    return p[len(ROOT):] if p.lower().startswith(ROOT.lower()) else p


byloc = collections.Counter(f"{norm(f)}({l}) {c}: {m[:95]}" for f, l, c, m in errs)
byfile = collections.Counter(norm(f) for f, l, c, m in errs)
firsts = collections.Counter()
for chunk in log.split("FAILED:")[1:]:
    m = re.search(ERROR, chunk)
    if m:
        firsts[f"{norm(m.group(1))}({m.group(2)}) {m.group(3)}: {m.group(4)[:95]}"] += 1
print("--- first error per failed TU (by count)")
for k, n in firsts.most_common(top):
    print(f"{n:5} {k}")
print("--- top error sites")
for k, n in byloc.most_common(15):
    print(f"{n:5} {k}")
print("--- top files")
for k, n in byfile.most_common(15):
    print(f"{n:5} {k}")
print("--- failed files")
for f in failed:
    print(f"      {f}")
