#!/usr/bin/env python3
"""Import the zeldaret/tww decompilation into this tree.

Copies the committed state of a tww checkout (never its working tree) at a
given commit, replacing every managed path so files removed upstream disappear
here too. Run this only on the `vendor/tww` branch, commit the result, then
merge `vendor/tww` into `main`; all PC changes live on `main`.

    python tools/import_tww.py --tww ../tww [--rev HEAD]
"""

import argparse
import io
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

# Paths copied from tww, relative to both repo roots. Anything not listed here
# is never touched by the import.
MANAGED_PATHS = [
    # Game, framework and JSystem sources
    "src/DynamicLink.cpp",
    "src/JAZelAudio",
    "src/JSystem",
    "src/SSystem",
    "src/c",
    "src/d",
    "src/f_ap",
    "src/f_op",
    "src/f_pc",
    "src/m_Do",
    # Their headers
    "include/DynamicLink.h",
    "include/JAZelAudio",
    "include/JSystem",
    "include/SSystem",
    "include/c",
    "include/d",
    "include/f_ap",
    "include/f_op",
    "include/f_pc",
    "include/m_Do",
    "include/global.h",
    "include/weak_bss_3569.h",
    "include/weak_bss_936_to_1036.h",
    "include/weak_data.h",
    # Resource-archive index enums (file names only, no game data)
    "assets/D44J01",
    "assets/GZLE01",
    "assets/GZLJ01",
    "assets/GZLP01",
]
# Deliberately not imported: src/dolphin and include/dolphin (aurora provides
# the Dolphin SDK), PowerPC_EABI_Support, TRK_MINNOW_DOLPHIN, OdemuExi2, REL,
# amcstubs and odenotstub (PowerPC runtime, debugger and REL glue).

# Copied into upstream/tww/ for reference (build lists, license).
METADATA_FILES = ["configure.py", "LICENSE", "README.md"]


def git(tww: Path, *args: str) -> bytes:
    return subprocess.run(["git", "-C", str(tww), *args], check=True, capture_output=True).stdout


def remove(path: Path) -> None:
    if path.is_dir():
        shutil.rmtree(path)
    elif path.exists():
        path.unlink()


def extract(tww: Path, rev: str, paths: list[str], dest: Path) -> int:
    archive = git(tww, "archive", "--format=tar", rev, "--", *paths)
    count = 0
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        for member in tar.getmembers():
            if not member.isfile():
                continue
            target = dest / member.name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(tar.extractfile(member).read())
            count += 1
    return count


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tww", type=Path, required=True, help="path to a zeldaret/tww checkout")
    parser.add_argument("--rev", default="HEAD", help="commit to import (default: HEAD)")
    parser.add_argument("--remote", default="origin",
                        help="tww remote whose URL is recorded in UPSTREAM_COMMIT (default: origin)")
    parser.add_argument("--root", type=Path, default=None,
                        help="tree to import into (default: the repo containing this script)")
    args = parser.parse_args()

    root = (args.root or Path(__file__).resolve().parent.parent).resolve()
    tww = args.tww.resolve()
    sha = git(tww, "rev-parse", f"{args.rev}^{{commit}}").decode().strip()
    date = git(tww, "show", "-s", "--format=%cI", sha).decode().strip()
    try:
        origin = git(tww, "remote", "get-url", args.remote).decode().strip()
    except subprocess.CalledProcessError:
        origin = "unknown"

    missing = [p for p in MANAGED_PATHS
               if subprocess.run(["git", "-C", str(tww), "cat-file", "-e", f"{sha}:{p}"],
                                 capture_output=True).returncode != 0]
    if missing:
        print(f"error: {sha[:10]} lacks managed paths: {', '.join(missing)}", file=sys.stderr)
        return 1

    for p in MANAGED_PATHS:
        remove(root / p)
    count = extract(tww, sha, MANAGED_PATHS, root)

    meta = root / "upstream" / "tww"
    remove(meta)
    count += extract(tww, sha, METADATA_FILES, meta)

    (root / "UPSTREAM_COMMIT").write_text(
        f"repo: {origin}\n"
        f"commit: {sha}\n"
        f"date: {date}\n",
        encoding="utf-8",
    )
    print(f"imported {count} files from {origin} @ {sha[:10]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
