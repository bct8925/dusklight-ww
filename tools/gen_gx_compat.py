#!/usr/bin/env python3
"""Generate sdk_compat/pc_gx_hw_enums.h from a zeldaret/tww checkout's include/dolphin/gx/GXEnum.h.

The decomp's own Dolphin SDK headers are not imported (aurora provides the SDK), so this pulls
out the GX enums aurora lacks, plus aliases for enumerators aurora spells differently. Re-run if
aurora or the decomp's GXEnum.h changes:

    python tools/gen_gx_compat.py ../tww
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TWW = os.path.join(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "..", "tww"),
                   "include", "dolphin", "gx", "GXEnum.h")
AURORA = os.path.join(ROOT, "extern", "aurora", "include", "dolphin")
OUT = os.path.join(ROOT, "sdk_compat", "pc_gx_hw_enums.h")

aur = "".join(open(f, encoding="utf-8", errors="replace").read()
              for f in glob.glob(AURORA + "/**/*.h", recursive=True))
aur_macros = set(re.findall(r"#define\s+(\w+)", aur))
src = open(TWW, encoding="utf-8").read()

out = [
    "// Generated from the zeldaret/tww decomp's include/dolphin/gx/GXEnum.h. Do not edit.",
    "// GX hardware register enums (BP/CP/XF register fields, FIFO commands, dirty flags) that the",
    "// decomp's JSystem code uses and aurora's SDK does not declare. Names aurora defines as macros",
    "// are skipped by #ifndef; names it declares as enumerators are left out entirely.",
    "#ifndef SDK_COMPAT_PC_GX_HW_ENUMS_H",
    "#define SDK_COMPAT_PC_GX_HW_ENUMS_H",
    "",
    "#include <dolphin/gx.h>",
    "",
]
count = 0
for m in re.finditer(r"typedef\s+enum\s*(\w*)\s*\{(.*?)\}\s*(\w+)\s*;", src, re.S):
    tag, body, tname = m.groups()
    entries = re.findall(r"^\s*(\w+)\s*(=\s*[^,\n/]*)?,?", body, re.M)
    keep = []
    for name, value in entries:
        in_aurora = re.search(r"\b" + name + r"\b", aur)
        if in_aurora and name not in aur_macros:
            continue  # aurora declares it as an enumerator already
        keep.append((name, value.strip()))
    if not keep:
        continue
    type_exists = re.search(r"\b" + tname + r"\b", aur)
    if type_exists:
        # The enum type exists in aurora; add the missing names as constants of that type.
        for name, value in keep:
            out += [f"#ifndef {name}", f"#define {name} (({tname})({value.lstrip('= ').strip() or 0}))",
                    "#endif"]
            count += 1
        continue
    out.append(f"typedef enum {tag} {{")
    for name, value in keep:
        guard = name in aur_macros
        if guard:
            out.append(f"#ifndef {name}")
        out.append(f"    {name} {value},".replace("  ,", ",").replace(" ,", ","))
        if guard:
            out.append("#endif")
        count += 1
    out.append(f"}} {tname};")
    out.append("")
out += ["#endif", ""]
open(OUT, "w", newline="\n").write("\n".join(out))
print(f"{count} names")


# Second pass: enumerators the decomp spells differently from aurora (same enum type, same value).
def enum_values(text):
    result = {}
    for m in re.finditer(r"typedef\s+enum\s*\w*\s*\{(.*?)\}\s*(\w+)\s*;", text, re.S):
        body, tname = m.groups()
        body = re.sub(r"//.*", "", body)
        body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
        vals, nxt, env = {}, 0, {}
        for item in [x.strip() for x in body.split(",") if x.strip()]:
            if item.startswith("#"):
                continue
            name, _, expr = item.partition("=")
            name = name.strip()
            if not re.fullmatch(r"\w+", name):
                continue
            if expr.strip():
                try:
                    nxt = int(eval(expr.strip().rstrip("uUlL"), {}, dict(env)))
                except Exception:
                    nxt = None
            if nxt is None:
                break
            vals[name] = nxt
            env[name] = nxt
            nxt += 1
        result[tname] = vals
    return result


aur_enum_text = "".join(open(f, encoding="utf-8", errors="replace").read()
                        for f in glob.glob(AURORA + "/gx/*.h"))
dec_vals = enum_values(src)
aur_vals = enum_values(aur_enum_text)
aliases = []
for tname, vals in dec_vals.items():
    if tname not in aur_vals:
        continue
    by_value = {}
    for n, v in aur_vals[tname].items():
        by_value.setdefault(v, []).append(n)
    for n, v in vals.items():
        if re.search(r"\b" + n + r"\b", aur) or len(by_value.get(v, [])) == 0:
            continue
        aliases.append((n, by_value[v][0], tname))

text = open(OUT).read().rstrip()
assert text.endswith("#endif")
block = ["", "// Enumerators the decomp spells differently from aurora (same enum type and value)."]
for n, target, tname in aliases:
    block += [f"#ifndef {n}", f"#define {n} {target}", "#endif"]
text = text[: -len("#endif")] + "\n".join(block) + "\n\n#endif\n"
open(OUT, "w", newline="\n").write(text)
print(f"{len(aliases)} aliases")
