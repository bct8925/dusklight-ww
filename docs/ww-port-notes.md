# Wind Waker PC port — working notes

Context for resuming the port. Last updated 2026-10-03, at commit `05f1e6d`.

## Goal and ground rules

- A native PC port of *The Legend of Zelda: The Wind Waker* (GZLE01, USA rev 0), built the way
  dusklight ports Twilight Princess. This repo, `bct8925/dusklight-ww`, is a **fork of
  TwilitRealm/dusklight** (CC0) with TP's game code replaced by the **zeldaret/tww decomp**,
  which comes in as the `tww/` submodule (see "Repositories").
- **Never use melee-pc code** (`bct8925/melee-pc`, GPL). Dusklight's conventions and code only.
- **The fork is public. Ask before every push.**
- **Never commit Nintendo data**:
  - generated `assets/*.h`, extracted files, the ISO;
  - binaries built with `WW_LOCAL_ASSETS`.
- zeldaret/tww and dusklight upstream reject primarily AI-generated PRs. Keep AI-written code
  here, or on a private tww branch. Anything sent upstream must be the user's own work.
- **Ghidra-derived code is for personal use.** Functions rewritten from the Wind Waker Ghidra
  project make the port work. They are never submitted to zeldaret/tww or dusklight, and live
  only on the `ghidra` branch of the private `bct8925/tww-private` repo.
- **Never commit the Ghidra export** (or anything copied wholesale from it). It is derived from
  Nintendo's binary. Only the lookup tool is in the repo.
- Plan file: `C:\Users\brian\.claude\plans\create-the-plan-for-generic-clarke.md`. It covers
  Milestone 1 "boot to title screen" (M1.0–M1.10) and M2–M6.

## Repositories

Four repos are involved:

| Repo | Visibility | What it holds |
|---|---|---|
| `zeldaret/tww` | public, official | The Wind Waker decomp. Never pushed to. |
| `bct8925/tww-private` | **private** | The decomp as the port builds it. Branches: `main` (mirror of `zeldaret/tww` main), `pc` (main + every PC port change to decomp files: `TARGET_PC` edits, `BE()`, pointer fixes, `JKR_NEW`, `PC_EMPTY_STUB` bodies, `JKRNew.h`; also drops the decomp's Dolphin SDK and PowerPC runtime, which aurora replaces), `ghidra` (pc + functions rewritten from the Ghidra export). |
| `TwilitRealm/dusklight` | public, official | The engine (Twilight Princess port). Remote `dusklight-upstream`; never pushed to. |
| `bct8925/dusklight-ww` | **public** | This repo: the engine fork plus the Wind Waker port layer (`src/dusk`, `sdk_compat`, `cmake`, `files.cmake`, `tools`, `docs`). It **does not commit decomp sources**; it pins a `tww-private` `pc` commit as the `tww/` submodule. |

Consequences:
- Changes to decomp files (`tww/src`, `tww/include`, `tww/assets`) are commits in the `tww/`
  submodule, on `pc` (or `ghidra` for Ghidra-derived code), pushed to `tww-private`; then bump
  the submodule pointer here (`git add tww`). Never commit Ghidra-derived code on `pc`.
- dusklight-ww's committed submodule pointer is always a `pc` commit. To run with the Ghidra
  rewrites locally, check out `ghidra` in `tww/` (`git -C tww checkout ghidra`) and rebuild;
  don't commit that pointer.
- Others can read dusklight-ww but can't build it without access to `tww-private`.

## Where things are

| What | Path |
|---|---|
| This repo | `C:\Users\brian\Dev\dusklight-ww` (branch `main`, remote `origin`) |
| Decomp, as built | `tww/` submodule here (`bct8925/tww-private`; remote `zeldaret` for the official repo) |
| Standalone tww-private checkout | `C:\Users\brian\Dev\tww-private` (for work outside the port, e.g. on `ghidra`) |
| Dusklight upstream | remote `dusklight-upstream` (read TP's PC fixes: `git show dusklight-upstream/main:<path>`) |
| Local tww build (DOL assets for `WW_LOCAL_ASSETS`, `symbols.txt`) | `C:\Users\brian\Dev\tww` (GitHub `bct8925/tww`, public fork) |
| Disc image | `C:\Users\brian\Dev\tww\orig\GZLE01\game.iso` (trimmed dump; `files/` holds only RELs) |
| User data, logs, `trace.txt` | `%APPDATA%\bct8925\Dusklight-WW\` (`logs\dusklight-*.log`) |
| Ghidra C export of `main.dol` | `C:\Users\brian\ghidra\WindWaker.rep\main.1.c` (+ `main.1.h` types). Local only; the lookup cache `main.1.c.index` sits beside it |

In the dusklight tree, JSystem lives in `libs/JSystem/...`. In ours it mirrors TWW: `tww/include/JSystem/...` and `tww/src/JSystem/...`.

## Build, run, debug

VS Code: open `dusklight-ww.code-workspace` (repo + `tww/` submodule as two folders; tasks for configure, build, run, crashtrace and switching `tww/` between `pc` and `ghidra`; clangd reads `build/windows-msvc-relwithdebinfo/compile_commands.json`, written by the configure task). A prompt for starting a new session is in `docs/continue-prompt.md`.

All commands run from the repo root in Git Bash.

```bash
# Build (MSVC environment via tools/msvc.cmd; -k 0 keeps going past errors)
cmd //c "$(cygpath -w tools/msvc.cmd)" cmake --build build/windows-msvc-relwithdebinfo --target dusklight -- -k 0 > build/x.log 2>&1
py tools/build_errors.py build/x.log            # tally of compile/link errors

# Run with crash reporting (prints the crashing function and probable callers from the PDB)
py tools/crashtrace.py build/windows-msvc-relwithdebinfo/dusklight.exe --develop --trace --dvd "C:\\Users\\brian\\Dev\\tww\\orig\\GZLE01\\game.iso"
# Hangs: print where the main thread is after N seconds, then stop the game
py tools/crashtrace.py --sample 25 build/windows-msvc-relwithdebinfo/dusklight.exe --develop --trace --dvd "..."

# Launch for the user to watch (detached)
cd build/windows-msvc-relwithdebinfo && cmd //c start "" dusklight.exe --develop --dvd "..."
```

**Game options:**
- `--dvd <iso>`: the disc path. It's remembered after the first run.
- `--develop`: sets `mDoMain::developmentMode = 1` and forces the game's `OSReport` output into the log.
- `--trace`: logs the following:
  - process creation, by profile name;
  - loading-phase steps, by function name;
  - DVD commands;
  - logo scene steps;
  - scene change requests;
  - resource load failures.

  Trace lines also go to `trace.txt`, flushed per line. **Read `trace.txt` after a crash.** The normal log loses its last lines.
- `--backend d3d12|vulkan|...`: forces a graphics backend.

**Reading a crash:** `crashtrace.py` output plus the `trace.txt` tail. A `FATAL ... Halt` in the log means an `OSPanic`/`JUT_ASSERT`, and the line before it names the failed assertion.

## Environment gotchas

- **Stale aurora pipeline cache.** `%APPDATA%/bct8925/Dusklight-WW/pipeline_cache.db*` stores every GX shader config the game has drawn, and aurora recompiles them at startup. Configs recorded while the port was broken (garbage texgens, TEV state) make aurora `FATAL` at startup (`unhandled tcg src 21`, `GX_TG_BINRM/TANGENT requires NBT`) before the game draws anything, even after the cause is fixed. If a GX FATAL looks impossible, delete the `pipeline_cache.db*` files first.

- `python` is the Microsoft Store stub here. **Use `py`.**
- **Bash heredocs mangle backslashes** (`'\n'`, `[\\/]`, `\\` macro continuations, `\(` in regexes).
  For any patch script that contains backslashes, write it with the Write tool into the
  scratchpad and run it with `py <file>`, or use the Edit tool.
- `tools/pcpatch.py` helpers:
  - `rep(path, old, new, count=1)`;
  - `pc(path, old, new, count=1)`, which wraps the change in `#if TARGET_PC` / `#else` original / `#endif`. Use it **only on whole statements**, never part of a function;
  - `pc_range`;
  - `stub_body`.

  They preserve CRLF/LF and fail loudly if the text isn't found. A failure part-way through a script leaves the earlier edits applied.
- The decomp's source text sometimes has trailing whitespace on blank lines, which breaks exact matches.
- LF→CRLF warnings from git on commit are harmless.
- Full rebuilds take a few minutes. Run long builds in the background if needed.
- Copyright: the ISO and extracted data stay local.

## Ghidra: rewriting functions the decomp has not decompiled

The decomp still has about 3,300 empty (`/* Nonmatching */`) functions, mostly in actors. On PC
an empty function does nothing, so if the game reaches one, behaviour silently differs. The
workflow:

1. **Find out what is actually reached.** Every empty function in the built code calls
   `PC_EMPTY_STUB()` on PC. The first hit of each is logged ("… is a stub") and, with `--trace`,
   written to `trace.txt` as `stub hit: <function>`:

   ```bash
   grep "stub hit" "$APPDATA/bct8925/Dusklight-WW/trace.txt"
   ```

   After enabling more actors, run `py tools/pc_stub_empty.py --all tww/src/d/actor/<new>.cpp`.
2. **Look the function up:**
   - by name: `py tools/ghidra_lookup.py <Class::method>`. Substring by default, `--exact`,
     `--list` for names only;
   - by address: use the `/* 8xxxxxxx-8xxxxxxx */` comment above the decomp function, e.g.
     `py tools/ghidra_lookup.py 0x8007de94`. Named functions carry no address in the export, so
     the tool maps the address to a name through the decomp's `symbols.txt`
     (`C:\Users\brian\Dev\tww\config\GZLE01\symbols.txt`, or `TWW_SYMBOLS`). Unnamed ones are
     `FUN_<address>`.

   Ghidra wraps long signatures over several lines; the indexer handles that (39,333
   functions). The index cache is rebuilt automatically when the export changes.
3. **Rewrite it** inside the existing empty body, under `#if TARGET_PC` (the empty original
   stays under `#else`). Write readable, functional C++, not a matching decompilation:
   - map Ghidra's `field30_0x30`-style names and raw offsets to the decomp's member names
     (check the class's `/* 0xNN */` offset comments);
   - replace Ghidra's `FUN_8xxxxxxx` calls with the decomp's named functions (look up the
     address in `symbols.txt`);
   - apply the PC rules from this doc: `JKR_NEW`/`JKR_DELETE`, `BE()` for file data, no 32-bit
     pointer casts;
   - remove the `PC_EMPTY_STUB()` call.
4. Verify by running past the point that needed it, and compare behaviour with Dolphin where it
   matters.

**Coverage:** the export only has `main.dol`. Actor code (`d_a_*`, RELs on the GameCube) is not
in it; export a REL's program from Ghidra the same way if an actor function is needed.

**Status 2026-10-03:** a full run to the current blocker hits **no** stubs, so nothing has needed
rewriting yet. Empty functions the title is likely to reach soon:
- `d_particle`: four draw callbacks:
  - `dPa_smokePcallBack::draw` (213 lines in the export);
  - `dPa_waveEcallBack::draw` (78);
  - `dPa_stripesEcallBack::draw` (129);
  - `dPa_ripplePcallBack::draw` (153).
- `d_ev_camera`: entirely empty; needed if the title uses an event/demo camera.
- `d_camera`: partly matched (38%) at the plan's last count; check the stub hits.

The other empty functions in built files (menus, map, message paper, minigame, name entry) are
off the title path.

## Repo workflow

- **Clone:** `git clone --recursive https://github.com/bct8925/dusklight-ww.git` (needs access to
  `tww-private` for the `tww/` submodule).
- **Changing decomp code:** edit under `tww/`, commit in the submodule on `pc` (Ghidra-derived
  code on `ghidra` only), `git -C tww push`, merge `pc` into `ghidra`, then `git add tww` here and
  commit the pointer. `tools/pcpatch.py` and the codemods take `tww/...` paths.
- **Updating from the official decomp:** `py tools/update_tww.py` (submodule on a clean `pc`).
  It fetches the submodule's `zeldaret` remote (https://github.com/zeldaret/tww.git, added if
  missing; never a fork) and merges `zeldaret/main` into `pc` with `-X theirs`: **upstream wins
  every conflicting hunk**, so a function they have now decompiled replaces our `PC_EMPTY_STUB`
  body. Upstream changes to the removed Dolphin SDK / PowerPC runtime paths are dropped. It then
  re-runs `jkr_new_codemod.py` and `pc_stub_empty.py --all` on the touched files and prints the
  files where PC-layer lines (`TARGET_PC`, `uintptr_t`, `BE(`, `JKR_*`) were dropped. **Review
  that list**: a dropped line is either obsolete (upstream finished the code) or a lost PC fix to
  re-apply (the first update lost the `uintptr_t` callback parameter in `d_a_player_main.cpp`).
  `--manual` leaves conflicts for hand resolution. Then follow the "Afterwards" steps in the
  script's docstring: commit and push `pc`, push `zeldaret/main` to tww-private `main`, merge `pc`
  into `ghidra`, run `gen_ww_files.py` and `gen_profile_list.py`, and commit the submodule pointer.
  Current base: `f5234ec8f4`.
- After a decomp update, run:
  - `py tools/gen_ww_files.py`, which writes `cmake/WWGameFiles.cmake` from `config.libs` in
    `tww/configure.py` and skips `DEBUG_ONLY` objects;
  - `py tools/gen_profile_list.py`.
- **Which actors are built:** `files.cmake` → `WW_ENABLED_RELS`. There are no RELs on PC; enabled actors are linked into the exe. After changing the list, run `py tools/gen_profile_list.py`, which writes `src/dusk/ww_profile_list.cpp` plus a name table for `--trace`. Actors that aren't built are `nullptr` and `fpcBs_Create` fails cleanly ("not built" in the trace).
- **Missing audio symbols at link:** `py tools/gen_audio_null.py build/x.log` adds no-op definitions (`src/dusk/ww_audio_null.cpp`). The wave-load checks *must* return 0, which means "ready". Returning 1 hangs the logo scene.
- **Which actors a room needs:** `py tools/list_stage_actors.py <iso> <stage> <room>` reads the disc's FST, RARC archives and stage chunks.

## Tools in `tools/`

| Tool | Purpose |
|---|---|
| `update_tww.py` | Merge the official decomp into the `tww/` submodule's `pc` branch (see Repo workflow) |
| `gen_ww_files.py` | `cmake/WWGameFiles.cmake` from `tww/configure.py` |
| `gen_profile_list.py` | Typed profile list (MSVC mangles variable types) + `--trace` name table |
| `gen_audio_null.py` | Null audio from link errors (`OVERRIDES` table for non-zero bodies) |
| `gen_gx_compat.py` | `sdk_compat/pc_gx_hw_enums.h` |
| `pc_stub_empty.py` | PC bodies for empty decomp functions: `PC_EMPTY_STUB()` logs the first hit. Default: non-void only (adds `return {};`). `--all` covers void functions, constructors and destructors too. Already applied with `--all` to every built file; rerun on newly enabled actors |
| `ghidra_lookup.py` | Print functions from the Ghidra export by name (`dCamera_c::Run`), substring, or address (`0x8007de94`, resolved through the decomp's `symbols.txt`); `--list` for names only. Export path from `--export` / `GHIDRA_EXPORT` / the default above |
| `jkr_new_codemod.py` | Rewrote game `new`/`delete` to `JKR_NEW`/`JKR_DELETE` (already applied; rerun on new code) |
| `port_be_fields.py` | Copy dusklight's `BE()`/`OFFSET_PTR` field annotations onto same-named TWW **structs**, matched by `/* 0xNN */` offset. Use `--dry-run` first. It wraps *our* types. Skip runtime classes and TP-only formats (JPA v2 ≠ our JPA v1) |
| `pdb_sym.py` | Resolve image offsets to symbols through the PDB (e.g. which static a stray pointer in a log belongs to; log `ptr - GetModuleHandle(NULL)`) |
| `crashtrace.py` | Debug-API crash reporter / `--sample N` hang sampler (dbghelp, no debugger needed) |
| `list_stage_actors.py` | Actor RELs a stage room places, from the ISO |
| `build_errors.py`, `pcpatch.py`, `msvc.cmd` | Build-log tally, patch helpers, VS environment |

## Status

M1.0–M1.5 are done. M1.6 (archives) is done apart from its exit check. M1.7–M1.10 are working: **the title screen renders and animates** (2026-10-03): the demo camera flies over the sea, the King of Red Lions, Link on the cliff, gulls, clouds, the logo with "the Wind Waker" and PRESS START. The cinemascope bars are the game's own (`dCamera_c` trim); at wide window aspects aurora crops the 4:3 image vertically, so the bars look uneven. Frame rate is about 20 fps in a RelWithDebInfo build.

**Boot sequence that works now** (see `trace.txt`):
1. `LOGO_SCENE`:
   - mounts System, Logo, Always, Link, Agb and LkAnm, plus about 25 resident archives;
   - loads every J3D model, animation and material in them;
   - plays the Nintendo/Dolby steps;
   - sets up particles (`common.jpc`) and `ActorDat.bin`.
2. `OPENING_SCENE`, the title-screen play scene on `sea_T` room 44:
   - creates `KANKYO`, `KYEFF`(2), `ENVSE`, `CAMERA`, `SEA`, `VRBOX`(2), `ROOM_SCENE` and `PLAYER` (Link), then `TITLE` and `METER`;
   - `SHIP` fails its create legitimately (save flag `MET_KORL` unset), and the cleanup path works;
   - event 80 runs the `title.stb` demo (JStudio, `Demo51.arc`), which creates the demo actors and drives the camera. `daSea` culls itself in room 44 on purpose (Outset has its own water).

**Found while bringing the title up** (all in tww-private `pc`; each is a class of bug worth checking elsewhere):
- J3D joint init data and its index table were never byte-swapped, so every unanimated model had garbage joint matrices (sky box, island).
- `GFSetVtxDescv` must replace the whole vertex descriptor (the GF call writes the raw CP registers); aurora's `GXSetVtxDescv` only updates the listed attributes.
- J3D materials bind textures by physical address inside their display lists. On PC `J3DMaterial::load*` first calls `J3DTevBlock::loadTexture()`, which binds real `GXTexObj`s (`J3DTexture::loadGX`, in `J3DTevs.cpp`). `J3DTexture::setResTIMG` keeps data pointers on the side because the relative-offset patching overflows the 32-bit offset fields.
- `J3DLoadArrayBasePtr` and the model matrix arrays go to aurora as sized commands (`J3DSys::setModelDrawMtx(ptr, count)`).
- JStudio: stb/fvb data is big-endian (`BE(u32)` reads, swapped copies of list/hermite data), `'STB '`/`'MESG'` multi-character signatures are little-endian on MSVC (use byte arrays), `TLinkList` node offsets are `-offsetof` (the vtable is 8 bytes), and the null audio has to create a `JSND` object.
- `JKRHeap::alloc` returns zeroed memory on PC: a fresh GameCube heap is zero and the game reads members of new objects before writing them (demo camera, `TVariableValue`, the fast-create request size).
- Hard-coded structure sizes break with 64-bit pointers (`fpcFCtRq_Request` allocated 0x50 bytes for a larger request).
- Aurora's cached `GXTexObj`s must outlive the call (`J3DSys::reinitTexture`'s texture object is static now).

Stubs reached by a run to the title (see Ghidra section): `dMap_c::drawActorPointMiniMap`, `dMap_c::mapBufferSendAGB`. The `ghidra` branch also rewrites `dCamera_c::getEvStringData`/`getEvIntData`/`pauseEvCamera` and `dPa_waveEcallBack::draw`; running with `pc` only gives the same picture so far. Not yet checked: pressing Start (the title's next scene), audio (null), and the remaining visual differences from the GameCube (Link's lighting, particle/wave details).

**Commits since the fork, in order:**

| Commit | Change |
|---|---|
| `3e4fde4` | Remove the TP game tree |
| `5f1c58c` / `cf1c1e3` | Import tww `d9672fa` |
| `0920a43`, `8b3174d` | The shell builds alone |
| `a6d300f`, `4a3bf9b`, `6fe2d4b`, `43448c8`, `738b096` | The whole tree compiles |
| `4d34e99`, `cf3387d` | Links: profile list, null audio, entry point |
| `9e8190d` | `JKR_NEW` scheme + `crashtrace` |
| `8da33ff` | Boot to the frame loop (font, heaps, thread-local heap, main-thread asserts) |
| `c4a2fd2` | Archives: big-endian RARC, `--trace`, null-audio fix |
| `9af252a` | Big-endian `ResTIMG` |
| `b37b9cf` | J3D models and animations |
| `eaf6f38` | JParticle v1 |
| `446e9ad` | `ActorDat.bin` |
| `11732c8` | Stage chunks, title actors, process fixes |
| `c7f9bfe` | Process vtable layout, DZB collision, Link |
| `d4aacf4` | These notes |
| `05f1e6d` + follow-up | Every reached empty function reports itself; Ghidra export lookup |

## How the PC layer is built (things to know before changing code)

### Conventions (from dusklight)

- Game-code changes go in `#if TARGET_PC` … `#else` (original) … `#endif`.
- Macros that are no-ops off-PC can be used in place: `BE(T)`, `JKR_NEW`, `OFFSET_PTR*`, `JKR_HEAP_TOKEN`.
- Use `AVOID_UB` for undefined-behaviour fixes.
- `VERSION=2` means USA. **`VERSION=0` is the demo in TWW**, unlike dusklight.

### SDK

- The decomp's Dolphin SDK headers aren't imported. `sdk_compat/` forwards those paths to aurora's headers.
- `global.h` is force-included (`/FIglobal.h`) and pulls in `pc_sdk_extras.h`. MSVC flag: `/Zc:strictStrings-`.
- On aurora, `u32` is `unsigned int`; in the decomp it's `unsigned long`.
- `GXSetArray` on PC needs a byte size plus an endianness flag (`GXSETARRAY` macro).
- GF functions map to GX in `src/dusk/gf_compat.cpp`. `GFEnd` must call `GXEnd`.

### Entry point and frame loop

- `src/dusk/ww_game_main.cpp` defines `game_main`: settings, logging, aurora, opening the disc.
- It then calls `mDoMain_run` in `m_Do_main.cpp`. On PC, `main01` runs `mDoMain_pcLoop`:
  1. aurora events;
  2. `aurora_begin_frame`;
  3. `VIWaitForRetrace`;
  4. pad;
  5. audio;
  6. `fapGm_Execute`;
  7. `aurora_end_frame`.
- There is no main `OSThread`. The two `&mainThread` asserts compare against `JFWSystem::mainThread`.
- `JFWDisplay::waitForTick` uses a time-based limiter.

### Memory

- **Allocations:** global `new`/`delete` are the C runtime's. Game code uses `JKR_NEW`/`JKR_NEW_ARGS(heap, align)`/`JKR_NEW_ARRAY(T, n)`/`JKR_DELETE` (`include/JSystem/JKernel/JKRNew.h`, included by `global.h`). `JKR_DELETE` frees non-JKR pointers with `_aligned_free`, so **every game new/delete must go through the macros**. New or imported game code needs `tools/jkr_new_codemod.py`.
- **Heap sizes** (`m_Do_machine.cpp`): system heap 32 MB, archive and command heaps ×2, game heap ×20. `mem1Size` is 256 MB.
- `JKRHeap::sCurrentHeap` is **thread_local** on PC, accessed through `getCurrentHeap()`. Worker threads start with no current heap.
- `getMaxAllocatableSize` reserves worst-case alignment.

### Big-endian file data

This is the biggest recurring task. Data read in place from disc files keeps its byte order:
- **Multi-byte fields:** declare them `BE(T)`, which is `BE<T>` on PC with conversion operators.
- **Vectors:** `BE(Vec)`/`BE(cXyz)`/`BE(csXyz)`, and `BE_TVEC3(T)` for JGeometry vectors. `cXyz` converts from `BE<Vec>` on PC.
- **Offsets the game relocates into pointers in place:**
  - with `JSUConvertOffsetToPtr` later: `OFFSET_PTR_V0` (`BE(u32)`);
  - relocated in place: `OFFSET_PTR(T)`/`OFFSET_PTR_RAW` (`OffsetPtr`, self-relative with a "relocated" flag, `setBase(base, zeroIsNull)`).
- **Where 64-bit pointers don't fit at all, use side tables:**
  - archive file data: `JKRArchive::mFileData` via `JKAR_DATA(entry)`;
  - actor data string tables: host `char*` arrays.
- **Swapped once at load** (dusklight's approach): J3D vertex arrays (`FixArrayEndian`), vertex format and descriptor lists, DZB vertices.

  Watch for **double-loading** of anything swapped in place.
- **Converted so far:** `JUTDataHeader` block headers, `ResFONT`, `ResTIMG`, `ResNTAB`, `ResTLUT`, RARC tables, J3D (animation, model, joint, shape and material blocks, init data, hierarchy, envelopes, draw matrices), JPA v1, J2D block headers (only the headers so far), JStudio stb/fvb headers, `d_stage` chunks (including the TWW-only MULT, LBNK, SHIP, EVNT and memory map), `d_path`, DZB, and the actor spawn parameters `fopAcM_prmBase_class` (BE everywhere, as in dusklight).

### Processes

- The process classes inherit their base on PC: leaf, node, scene, view, kankyo, sub-kankyo, msg, overlap, actor and camera.
- `base_process_class` has a virtual destructor on PC. This is because MSVC places a vtable pointer *before* a non-polymorphic base, which shifted every process field of actors with virtuals.
- `PC_BASE_MEMBER(T, name)` (in `global.h`) keeps `x->base.` (and the camera's `view.`) working as an MSVC `__declspec(property)`.
- `fopAcM_ct` and in-place `new (this) X` use **default-initialization** (no `()`) on PC. Value-initialization zeroes the process fields.

### Other

- `fopScnM_CreateReq`/`ReRequest` take `uintptr_t` user data.
- Null audio has no JAudio/JAZelAudio. `mDoAud_Create` only sets `onInitFlag`.

## Bug classes to check first

When something crashes, it's almost always one of these:
1. **Big-endian data read raw.**

   Symptoms: huge counts, endless loops (searching for an end marker), wrong indices, `0x...` offsets that look byte-swapped.

   Fix: annotate the struct (`port_be_fields.py` if dusklight has it), or read through `BE(T)*`.
2. **32-bit pointer truncation.**

   Symptoms: `(u32)ptr`, `(int)ptr`, `(s32)ptr`, `u32` fields or params holding pointers, a crash address like `0x7FBB...` or `0x2xx` truncated.

   Fix: `uintptr_t`, `OffsetPtr`, or side tables.
3. **GameCube address assumptions.**

   Symptoms: a crash address like `0x81xxxxxx`, or comparisons against `0x80000000`/`0xC0000000`.
4. **Struct layout assumptions.**

   Symptoms: hard-coded offsets (`+0x3c`), `sizeof` assumptions, vtable placement, MSVC value-init zeroing in placement new.
5. **PowerPC-only code.** `asm{}` under `__MWERKS__` with no C fallback, which leaves a return value unset.
6. **Threads.** Real OS threads instead of the GameCube's cooperative ones: shared globals like the current heap.
7. **Failure paths that never run on the GameCube.** A failed actor create or a missing resource exercises code that was never exercised on GameCube. Fix the root cause rather than the cleanup.
8. **Process field corruption.** If process fields look like garbage (for example `mpProf = 0x00A9…` where `0xA9` is a process number), suspect a layout shift or zeroing.

## Known gaps and deferred items

- **J2D/BLO loading** (current blocker). Then everything visual (M1.7–M1.10):
  - J2D drawing;
  - J3D drawing: dusklight passes the vertex descriptors to `J3DShapeDraw` on PC and uses `GDSetArraySized`; display lists are parsed by aurora in big-endian;
  - toon/alpha paths, `d_a_sea` water, LOD terrain, trees/grass/flowers, JPA v1 drawing, the title logo.
- **J3D cluster (blend-shape `BLS`) loading isn't ported.** It needs dusklight's `J3DClusterLoader`/`J3DDeformer` rework, because clusters are copied with 32-bit pointers inside. Nothing loaded so far uses it.
- **J3D `J3DAnmVtxColorIndexData::mpData`** (vertex-colour animations) is still a raw pointer field.
- **M1.6 exit check:** compare `item_table.bin` and `ActorDat.bin` contents with a Dolphin memory dump.
- **ISO validation:** `iso_validate.cpp` has no GZLE01 hash. The user's ISO is trimmed (1,086,171,900 bytes), so a full dump is needed for the hash.
- **M1.9 asset task:** replace the `WW_LOCAL_ASSETS` generated `assets/*.h` includes with runtime reads from `main.dol` (`dvd_asset.cpp`), so no local decomp build is needed.
- **Camera risk (from the plan):**
  - `d_camera` is 38% matched and `d_ev_camera` 0% in the decomp;
  - the stub hits will show what's actually called;
  - rewrite from the Ghidra export (see the Ghidra section).
- **Optional `--trace` instrumentation** added during debugging and left in:
  - logo steps (`d_s_logo.cpp`);
  - scene change requests (`f_op_scene_mng.cpp`);
  - resource failures (`d_resorce.cpp`).

  It's cheap, so leave it.
- **JAudio** still reads `JKRHeap::sCurrentHeap` directly (`JAIBasic`, `JASBank`, `JASWaveBank`). Switch those to `getCurrentHeap()` when audio is ported (M3).
- **The opening scene requests `SHIP`**, but `MET_KORL` isn't set, so it errors out on purpose. Verify against Dolphin that the GameCube does the same.
- **`dvdWaitDraw` repeats the scene-change request** every frame ("refused" in the trace). That's expected.
