#!/usr/bin/env python3
"""Pull the latest zeldaret/tww into the current branch.

    py tools/update_tww.py [--tww ../tww] [--rev zeldaret/main] [--no-merge]

1. fetches the `zeldaret` remote of the tww checkout (added from
   https://github.com/zeldaret/tww.git if missing; it must be the official repo, not a fork);
2. imports <rev> onto branch `vendor/tww` through a temporary git worktree, so the current
   branch and working tree are left alone (the tree must be clean, though, for the merge);
3. merges `vendor/tww` into the current branch and lists the conflicts.

Resolving conflicts (the official code wins once a function is decompiled):
  * A conflict inside a function that upstream has now filled in -> take THEIRS for that
    function (our PC_EMPTY_STUB / Ghidra rewrite there is obsolete). `git checkout --theirs
    <file>` is right only if we have no other PC edits in the file; otherwise resolve by hand.
  * Then re-apply the mechanical PC fixes to the new upstream code:
        py tools/jkr_new_codemod.py <files>          (new/delete -> JKR_NEW/JKR_DELETE)
        py tools/pc_stub_empty.py --all <files>      (stubs for functions still empty)
        py tools/gen_ww_files.py && py tools/gen_profile_list.py   (if files/actors changed)
  * Rebuild; new upstream code can need the usual PC fixes (BE(), uintptr_t, ...).
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OFFICIAL = "https://github.com/zeldaret/tww.git"
REMOTE = "zeldaret"


def run(*args: str, cwd: Path = ROOT, check: bool = True, capture: bool = False):
    result = subprocess.run(args, cwd=cwd, check=check, text=True, capture_output=capture)
    return result.stdout.strip() if capture else result


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tww", type=Path, default=ROOT.parent / "tww", help="tww checkout (default: ../tww)")
    ap.add_argument("--rev", default=f"{REMOTE}/main", help=f"revision to import (default: {REMOTE}/main)")
    ap.add_argument("--no-merge", action="store_true", help="only update vendor/tww")
    args = ap.parse_args()
    tww = args.tww.resolve()

    if run("git", "status", "--porcelain", "--untracked-files=no", capture=True):
        print("error: working tree has uncommitted changes; commit or stash them first", file=sys.stderr)
        return 1

    remotes = run("git", "remote", "-v", cwd=tww, capture=True)
    if not any(line.split()[:2] == [REMOTE, OFFICIAL] for line in remotes.splitlines()):
        if any(line.split()[:1] == [REMOTE] for line in remotes.splitlines()):
            print(f"error: remote '{REMOTE}' in {tww} is not {OFFICIAL}", file=sys.stderr)
            return 1
        run("git", "remote", "add", REMOTE, OFFICIAL, cwd=tww)
    run("git", "fetch", REMOTE, cwd=tww)
    sha = run("git", "rev-parse", f"{args.rev}^{{commit}}", cwd=tww, capture=True)

    recorded = ""
    shown = subprocess.run(["git", "show", "vendor/tww:UPSTREAM_COMMIT"], cwd=ROOT, text=True, capture_output=True)
    for line in shown.stdout.splitlines():
        if line.startswith("commit:"):
            recorded = line.split(":", 1)[1].strip()
    if recorded == sha:
        print(f"vendor/tww is already at {sha[:10]}")
    else:
        if recorded:
            behind = run("git", "rev-list", "--count", f"{recorded}..{sha}", cwd=tww, check=False, capture=True)
            print(f"importing {sha[:10]} ({behind or '?'} commits after {recorded[:10]})")
        with tempfile.TemporaryDirectory() as tmp:
            wt = Path(tmp) / "vendor"
            run("git", "worktree", "add", str(wt), "vendor/tww")
            try:
                run(sys.executable, str(ROOT / "tools" / "import_tww.py"), "--tww", str(tww), "--rev", sha,
                    "--remote", REMOTE, "--root", str(wt))
                run("git", "add", "-A", cwd=wt)
                if run("git", "status", "--porcelain", cwd=wt, capture=True):
                    subject = run("git", "show", "-s", "--format=%s", sha, cwd=tww, capture=True)
                    run("git", "commit", "-q", "-m", f"Import zeldaret/tww {sha[:10]}\n\n{subject}", cwd=wt)
            finally:
                run("git", "worktree", "remove", "--force", str(wt))

    if args.no_merge:
        return 0
    merged = run("git", "merge", "--no-commit", "--no-ff", "vendor/tww", check=False, capture=True)
    print(merged)
    conflicts = run("git", "diff", "--name-only", "--diff-filter=U", capture=True)
    if conflicts:
        print("\nconflicts (see the notes at the top of tools/update_tww.py):")
        print("\n".join("  " + c for c in conflicts.splitlines()))
        return 2
    print("\nmerge staged without conflicts; build, then commit")
    return 0


if __name__ == "__main__":
    sys.exit(main())
