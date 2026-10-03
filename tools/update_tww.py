#!/usr/bin/env python3
"""Merge the latest official zeldaret/tww into the tww submodule's `pc` branch.

    py tools/update_tww.py [--rev zeldaret/main] [--manual]

The decomp lives in the `tww` submodule, a checkout of the private bct8925/tww-private repo:
  main    mirror of the official zeldaret/tww main
  pc      main + the PC port changes (TARGET_PC); this is what dusklight-ww builds and pins
  ghidra  pc + functions rewritten from the Ghidra export (personal use, never upstream)

This script, run from dusklight-ww with the submodule on a clean `pc`:
1. fetches the submodule's `zeldaret` remote (added from https://github.com/zeldaret/tww.git if
   missing; it must be the official repo, not a fork);
2. merges <rev> into `pc`, preferring upstream for conflicting hunks (`--manual` leaves them for
   you), re-runs the PC codemods on the files the merge touched, and lists the PC-layer lines the
   merge dropped so you can check that nothing important was lost.

Resolving conflicts (the official code wins once a function is decompiled):
  * A conflict inside a function that upstream has now filled in -> take THEIRS for that
    function (our PC_EMPTY_STUB there is obsolete). `git checkout --theirs <file>` is right only
    if we have no other PC edits in the file; otherwise resolve by hand.
  * Then re-apply the mechanical PC fixes to the new upstream code:
        py tools/jkr_new_codemod.py tww/<files>          (new/delete -> JKR_NEW/JKR_DELETE)
        py tools/pc_stub_empty.py --all tww/<files>      (stubs for functions still empty)
  * Rebuild; new upstream code can need the usual PC fixes (BE(), uintptr_t, ...).

Afterwards:
  git -C tww commit; git -C tww push origin pc
  git -C tww push origin zeldaret/main:main             (keep the mirror branch current)
  git -C tww checkout ghidra; git -C tww merge pc; git -C tww push origin ghidra
  py tools/gen_ww_files.py; py tools/gen_profile_list.py
  git add tww cmake/WWGameFiles.cmake src/dusk/ww_profile_list.cpp; git commit
"""

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TWW = ROOT / "tww"
OFFICIAL = "https://github.com/zeldaret/tww.git"
REMOTE = "zeldaret"
BRANCH = "pc"
# Removed on `pc`: aurora provides the Dolphin SDK, and the PowerPC runtime, debugger and REL glue
# have no PC meaning. Upstream changes to them are dropped (they stay deleted).
REMOVED = ("include/dolphin/", "src/dolphin/", "src/PowerPC_EABI_Support/", "include/TRK_MINNOW_DOLPHIN/",
           "src/TRK_MINNOW_DOLPHIN/", "include/OdemuExi2/", "src/OdemuExi2/", "include/REL/", "src/REL/",
           "src/amcstubs/", "src/odenotstub/")


def run(*args: str, cwd: Path = TWW, check: bool = True, capture: bool = False):
    result = subprocess.run(args, cwd=cwd, check=check, text=True, capture_output=capture)
    return result.stdout.strip() if capture else result


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rev", default=f"{REMOTE}/main", help=f"revision to merge (default: {REMOTE}/main)")
    ap.add_argument("--manual", action="store_true",
                    help="leave conflicts for hand resolution instead of preferring upstream")
    args = ap.parse_args()

    if not (TWW / ".git").exists():
        print("error: tww submodule is not checked out (git submodule update --init tww)", file=sys.stderr)
        return 1
    if run("git", "branch", "--show-current", capture=True) != BRANCH:
        print(f"error: check out the `{BRANCH}` branch in tww/ first (git -C tww checkout {BRANCH})",
              file=sys.stderr)
        return 1
    if run("git", "status", "--porcelain", "--untracked-files=no", capture=True):
        print("error: tww/ has uncommitted changes; commit or stash them first", file=sys.stderr)
        return 1

    remotes = run("git", "remote", "-v", capture=True)
    if not any(line.split()[:2] == [REMOTE, OFFICIAL] for line in remotes.splitlines()):
        if any(line.split()[:1] == [REMOTE] for line in remotes.splitlines()):
            print(f"error: remote '{REMOTE}' in tww/ is not {OFFICIAL}", file=sys.stderr)
            return 1
        run("git", "remote", "add", REMOTE, OFFICIAL)
    run("git", "fetch", REMOTE)
    sha = run("git", "rev-parse", f"{args.rev}^{{commit}}", capture=True)
    behind = run("git", "rev-list", "--count", f"HEAD..{sha}", capture=True)
    if behind == "0":
        print(f"{BRANCH} already contains {sha[:10]}")
        return 0
    print(f"merging {sha[:10]} ({behind} new upstream commits) into {BRANCH}")

    strategy = [] if args.manual else ["-X", "theirs"]
    print(run("git", "merge", "--no-commit", "--no-ff", *strategy, sha, check=False, capture=True))
    removed = [f for f in run("git", "diff", "--name-only", "--diff-filter=U", capture=True).splitlines()
               if f.startswith(REMOVED)]
    if removed:
        run("git", "rm", "-q", "--", *removed)
    added = [f for f in run("git", "diff", "--cached", "--name-only", "--diff-filter=A", capture=True).splitlines()
             if f.startswith(REMOVED)]
    if added:
        run("git", "rm", "-q", "--cached", "--", *added)
        run("git", "clean", "-q", "-f", "--", *added)
    conflicts = run("git", "diff", "--name-only", "--diff-filter=U", capture=True)
    if conflicts:
        print("\nconflicts (see the notes at the top of tools/update_tww.py):")
        print("\n".join("  tww/" + c for c in conflicts.splitlines()))
        return 2

    changed = [f for f in run("git", "diff", "--cached", "--name-only", "--diff-filter=AM", capture=True).splitlines()
               if f.startswith(("src/", "include/")) and f.endswith((".cpp", ".h", ".inc"))
               and not f.startswith(REMOVED)]
    if changed:
        # Upstream code knows nothing about the PC layer: redo the mechanical parts.
        run(sys.executable, "tools/jkr_new_codemod.py", *[f"tww/{f}" for f in changed], cwd=ROOT)
        sources = [f"tww/{f}" for f in changed if f.endswith(".cpp") and f.startswith("src/")]
        if sources:
            run(sys.executable, "tools/pc_stub_empty.py", "--all", *sources, cwd=ROOT)
        run("git", "add", "-A", *changed)

    # PC changes that the merge dropped: either upstream finished that code (intended) or a PC
    # fix was lost (re-apply it). Review these.
    marker = ("TARGET_PC", "PC_EMPTY_STUB", "uintptr_t", "BE(", "JKR_NEW", "JKR_DELETE", "OFFSET_PTR")
    dropped = {}
    current = ""
    for line in run("git", "diff", "--cached", "-U0", "HEAD", "--", "src", "include", capture=True).splitlines():
        if line.startswith("+++ b/"):
            current = line[6:]
        elif line.startswith("-") and not line.startswith("---") and any(m in line for m in marker):
            dropped[current] = dropped.get(current, 0) + 1
    if dropped:
        print("\nfiles where PC-layer lines were removed by the merge (count):")
        for f, n in sorted(dropped.items(), key=lambda kv: -kv[1]):
            print(f"  {n:4d}  tww/{f}")
    print("\nmerge staged in tww/; build, fix PC issues in the new code, then follow the "
          "'Afterwards' steps at the top of tools/update_tww.py")
    return 0


if __name__ == "__main__":
    sys.exit(main())
