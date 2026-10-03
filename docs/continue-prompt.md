# Prompt: continue the Wind Waker PC port

Paste the block below into a new Claude Code session opened on `C:\Users\brian\Dev\dusklight-ww`
(or on `dusklight-ww.code-workspace`). Update the "Where we left off" part when it goes stale.

---

We're continuing the Wind Waker PC port in C:\Users\brian\Dev\dusklight-ww (my fork of dusklight with the zeldaret/tww decomp swapped in).

Before doing anything, read docs/ww-port-notes.md. It has the ground rules, the repo layout, the build/run/debug commands, how the PC layer works, the recurring bug types, the Ghidra workflow, and the deferred items. The plan is at C:\Users\brian\.claude\plans\create-the-plan-for-generic-clarke.md (Milestone 1 = boot to the title screen).

Repo layout (four repos; details in the notes):
- zeldaret/tww: official decomp, read-only (remote `zeldaret` in the submodule).
- bct8925/tww-private (private): the decomp as the port builds it. `main` mirrors zeldaret, `pc` = main + every PC change to decomp files, `ghidra` = pc + functions rewritten from the Ghidra export. It is the `tww/` submodule of dusklight-ww.
- TwilitRealm/dusklight: the engine (remote `dusklight-upstream`), read-only.
- bct8925/dusklight-ww (public): engine fork + port layer only (src/dusk, sdk_compat, cmake, files.cmake, tools, docs). It never commits decomp sources; it pins a `pc` commit of tww/.

Where we left off (dusklight-ww main at 12aa9fb, tww/ at pc fea99f23de, all pushed):
- No crash: the game runs LOGO_SCENE, then OPENING_SCENE (title stage sea_T): Link, TITLE, METER, the sea, sky boxes, and the room actors (grass, trees, pots...), through the overlap fade and the scene change. Nothing visible is confirmed on screen yet.
- Reached empty functions ("stub hit:" in trace.txt): dMap_c::drawActorPointMiniMap and dMap_c::mapBufferSendAGB (on pc, also the dCamera/dPa_wave ones that the ghidra branch already rewrites).
- The log has ~560k "CP_REG_ARRAYBASE_ID is not supported. Use GX_AURORA_LOAD_ARRAYBASE instead." errors from aurora. Lead to check: display lists or GF/GD calls that write raw CP array-base registers (embedded DOL display lists, GFSetArray users like d_grass/d_tree), which aurora needs as sized array loads, the way J3DShape uses GDSetArraySized on PC.

Next steps:
1. Run the game and look at what actually draws (J2D title logo, J3D models, particles, the sea). Take screenshots or describe the window.
2. Track down the ARRAYBASE errors and fix them PC-side.
3. Keep working toward the title screen (M1.7–M1.10): J2D drawing, J3D drawing, toon/alpha paths, d_a_sea water, JPA v1 drawing, the title logo.

How to work:
- Build: cmd //c "$(cygpath -w tools/msvc.cmd)" cmake --build build/windows-msvc-relwithdebinfo --target dusklight -- -k 0 > build/x.log 2>&1, then py tools/build_errors.py build/x.log
- Run: py tools/crashtrace.py [--sample N] build/windows-msvc-relwithdebinfo/dusklight.exe --develop --trace --dvd "C:\\Users\\brian\\Dev\\tww\\orig\\GZLE01\\game.iso"
- After a crash, check the tail of %APPDATA%\bct8925\Dusklight-WW\trace.txt; the normal log loses its last lines on a fatal error.
- Use py, not python. Write patch scripts that contain backslashes to a file with the Write tool; Bash heredocs mangle them.
- Decomp files are under tww/ (a separate git repo). Commit PC fixes there on `pc`, push, merge `pc` into `ghidra`, then commit the submodule pointer in dusklight-ww. To run with the Ghidra rewrites: git -C tww checkout ghidra, rebuild, and never commit that pointer.
- Undecompiled functions the game reaches show up as "stub hit:" in trace.txt. Rewrite those from the Ghidra export (C:\Users\brian\ghidra\WindWaker.rep\main.1.c, looked up with py tools/ghidra_lookup.py <Class::method|0xaddress>) on tww-private's `ghidra` branch only.
- Other Claude sessions may be working in the same checkout; if you need to do side work, use a git worktree. C: is often nearly full, so check free space before adding another build directory.

Rules:
- Ask before every push to dusklight-ww; it is public. Pushing tww-private branches is fine.
- Never commit Nintendo data: the ISO, extracted files, generated assets/*.h, or the Ghidra export.
- Ghidra-derived code goes only on tww-private's `ghidra` branch; never on `pc`, never in dusklight-ww, never upstream.
