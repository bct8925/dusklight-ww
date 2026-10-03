#!/usr/bin/env python3
"""List the actor RELs a stage room needs, from the disc image.

Reads <stage>/Stage.arc and Room<N>.arc from a GameCube ISO, collects the actor names placed in
their actor chunks (ACTR/ACT0-b, SCOB/SCO0-b, TGOB, TGSC, TGDR, DOOR, PLYR, TRES/TRE0-b), maps each
name to its process through l_objectName in src/d/d_stage.cpp, and each process to the source
file that defines its profile. Prints the actor RELs (WW_ENABLED_RELS entries) that are needed,
and which are already enabled. Actors that the game creates from code are not listed; --trace
reports those as "not built".

    python tools/list_stage_actors.py <game.iso> sea_T 44
"""

import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


# --- GameCube disc (FST) ---

class Disc:
    def __init__(self, path):
        self.f = open(path, "rb")
        self.f.seek(0x424)
        fst_off, fst_size = struct.unpack(">II", self.f.read(8))
        self.f.seek(fst_off)
        fst = self.f.read(fst_size)
        count = struct.unpack_from(">I", fst, 8)[0]
        strings = fst[count * 12:]
        self.files = {}

        def name_at(off):
            return strings[off:strings.index(b"\0", off)].decode("shift_jis")

        # Walk the directory tree: (index of directory end, path) stack.
        stack = [(count, "")]
        for i in range(1, count):
            while i >= stack[-1][0]:
                stack.pop()
            flags_name, a, b = struct.unpack_from(">III", fst, i * 12)
            name = name_at(flags_name & 0xFFFFFF)
            path = f"{stack[-1][1]}/{name}"
            if flags_name >> 24:
                stack.append((b, path))
            else:
                self.files[path.lower()] = (a, b)

    def read(self, path):
        off, size = self.files[path.lower()]
        self.f.seek(off)
        return self.f.read(size)


def yaz0(data):
    if data[:4] != b"Yaz0":
        return data
    size = struct.unpack_from(">I", data, 4)[0]
    out = bytearray()
    src = 16
    while len(out) < size:
        code = data[src]
        src += 1
        for bit in range(7, -1, -1):
            if len(out) >= size:
                break
            if code & (1 << bit):
                out.append(data[src])
                src += 1
            else:
                b1, b2 = data[src], data[src + 1]
                src += 2
                dist = ((b1 & 0xF) << 8 | b2) + 1
                n = b1 >> 4
                if n == 0:
                    n = data[src] + 0x12
                    src += 1
                else:
                    n += 2
                for _ in range(n):
                    out.append(out[-dist])
    return bytes(out)


def rarc_files(data):
    """name -> bytes for every file in a RARC archive."""
    data = yaz0(data)
    if data[:4] != b"RARC":
        raise ValueError("not a RARC archive")
    data_off = struct.unpack_from(">I", data, 0x0C)[0] + 0x20
    info = 0x20
    num_files, file_off = struct.unpack_from(">II", data, info + 8)
    str_off = struct.unpack_from(">I", data, info + 0x14)[0]
    files = {}
    for i in range(num_files):
        e = info + file_off + i * 0x14
        _, _, type_name, d_off, d_size = struct.unpack_from(">HHIII", data, e)
        if (type_name >> 24) & 0x02:
            continue  # directory
        s = info + str_off + (type_name & 0xFFFFFF)
        name = data[s:data.index(b"\0", s)].decode()
        files[name.lower()] = data[data_off + d_off:data_off + d_off + d_size]
    return files


# --- Stage chunks ---

ENTRY_SIZE = {"ACT": 0x20, "SCO": 0x24, "TGO": 0x20, "TGS": 0x24, "TGD": 0x24, "DOO": 0x24, "PLY": 0x20,
              "TRE": 0x20}


def actor_names(dz):
    count = struct.unpack_from(">I", dz, 0)[0]
    names = []
    for i in range(count):
        tag, num, off = struct.unpack_from(">4sII", dz, 4 + i * 12)
        tag = tag.decode()
        size = ENTRY_SIZE.get(tag[:3])
        if size is None or tag[:3] == "TRE" and tag not in ("TRES",) + tuple(f"TRE{c}" for c in "0123456789ab"):
            continue
        for j in range(num):
            raw = dz[off + j * size:off + j * size + 8]
            names.append((tag, raw.split(b"\0")[0].decode("ascii", "replace")))
    return names


def main() -> int:
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    iso, stage, room = sys.argv[1], sys.argv[2], int(sys.argv[3])
    disc = Disc(iso)
    placed = []
    for arc, dz in [(f"/res/Stage/{stage}/Stage.arc", "stage.dzs"), (f"/res/Stage/{stage}/Room{room}.arc", "room.dzr")]:
        files = rarc_files(disc.read(arc))
        placed += [(arc.rsplit("/", 1)[1], *n) for n in actor_names(files[dz])]

    d_stage = (ROOT / "src/d/d_stage.cpp").read_text(encoding="utf-8")
    objname = {m.group(1): m.group(2) for m in re.finditer(r'OBJNAME\("([^"]+)",\s*fpcNm_(\w+)_e', d_stage)}
    defs = {}
    for path in (ROOT / "src").rglob("*.cpp"):
        for m in re.finditer(r"^\w+\s+g_profile_(\w+)\s*=", path.read_text(encoding="utf-8", errors="replace"), re.M):
            defs.setdefault(m.group(1), path.relative_to(ROOT).as_posix())
    files_cmake = (ROOT / "files.cmake").read_text(encoding="utf-8")
    enabled = set(re.search(r"set\(WW_ENABLED_RELS\s*\n(.*?)\n\)", files_cmake, re.S).group(1).split())
    ww_files = (ROOT / "cmake/WWGameFiles.cmake").read_text(encoding="utf-8")
    in_dol = set(re.search(r"set\(WW_DOL_FILES\s*\n(.*?)\n\)", ww_files, re.S).group(1).split())

    needed = {}
    for arc, tag, name in placed:
        proc = objname.get(name)
        src = defs.get(proc) if proc else None
        if src is None:
            print(f"  ? {arc} {tag} {name}: {'no l_objectName entry' if proc is None else 'no profile for ' + proc}")
            continue
        if src.startswith("src/d/actor/") and src not in in_dol:
            needed.setdefault(Path(src).stem, set()).add(name)
    print(f"{len(placed)} placed actors in {stage} room {room}; actor RELs:")
    for rel in sorted(needed):
        mark = "enabled" if rel in enabled else "NEEDED"
        print(f"  {mark:8} {rel:24} {', '.join(sorted(needed[rel]))}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
