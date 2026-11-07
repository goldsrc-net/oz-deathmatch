# mod-ozdm

**Mission: a functional, source-faithful reconstruction of the Oz Deathmatch 3.02 server game
library as modern C++ on the halflife-updated SDK, that builds for every arch the buildchain
targets, loads in the engine, and behaves identically to the original.**

Oz Deathmatch (AfterShock Productions, 1999-2003) is a server-only Half-Life deathmatch mod: runes,
the grappling hook, teamplay and the built-in RhoBot bots (1.7 had CTF; 3.02 has none). Clients run
Half-Life's own client library and download nothing, so there is no client to rebuild.

## Method

Same as `../mod-rockcrowbar` (Rocket Crowbar 1.9), whose original was built with the same compiler
(egcs 1.1.2): an unstripped Linux .so, so every function, vtable, save table and source file has its
name; `../DECOMPILE.md` section 3. Behavior parity on a modern compiler; the swap contract is the
external interface (entities, keyvalues, cvars, commands, user messages, the cfg/bot/nav files).

Against the newest SDK (Steven, 2026-11-07): halflife-updated's bug fixes stay, and a change that
isn't clearly design goes to halflife-updated's version; the original comes back only where
halflife-updated changed design or balance. `../mod-rc2/reference/audit/FIXES.md` has every call,
the same in mod-rc2 and mod-rockcrowbar.

Two sources besides the binary:
- the OZ 1.7 source (nekonomicon/OZDM on GitHub, AfterShock's own release): real names and
  code for what survived into 3.x; every OZ change in it is marked `// Oz`. Its licence asks for
  credit to Oz Deathmatch and no release of modifications under the Oz name.
- `reference/mp_i386.so` (3.0, pgcc 2.91.66): the release before 3.02, for diffs.

## This repo

A fork of `goldsrc-net/halflife-updated` branch `64bit-demo`, like mod-rockcrowbar.

- `upstream` = `git@github.com:goldsrc-net/halflife-updated.git`, tracked by `main`.
- `origin` = `git@github.com:goldsrc-net/oz-deathmatch.git`, private (`../REPOS.md`).

| Path | What |
|---|---|
| `game/oz/` | the mod's gamedir manifest (`liblist.gam`) |
| `reference/` | the original libraries and everything derived from them (`reference/README.md`); `RECONSTRUCTION.md` the work list, `DEVIATIONS.md`, `ORIGINAL_BUGS.md`, `dossiers/` one per package |
| `docs/` | `SERVER.md` (every cvar, command, impulse, file), `GAMEPLAY.md` (every change from Half-Life); `tutorials/` is Half-Life Updated's |
| `scripts/` | `build.sh`, `check_file.py`, `check_docs.py` (SERVER.md covers every cvar and command), `oz_touch.py`, the table generators from mod-rockcrowbar (`inventory`, `hierarchy`, `vtables`, `savefields`, `cvars`, `sdk_baseline`), `ghidra/` (`re.sh`; `ApplyRC.java` with VPTR 0x60) |
| `tools/rig/` | the original and ours side by side on the HLDS rig, and `smoke.py` (`tools/rig/README.md`) |

Original content: the 3.02 Steam server package (`oz3-02_for_steam.zip`), from the sparksonline.org
mirror the official download page links (`reference/README.md`). `tools/rig/setup.sh` takes its
unpacked `oz/` folder as `OZ_SRC`.

## Status (2026-11-07)

- Reconstructed: all seven packages (bots, player, rules, items, weapons-a, weapons-b, world) are
  merged. 2,762 of the original's 2,786 functions are matched by name in our build; the other 24
  are explained in their dossiers (statics today's compiler inlines, names the demangler misses,
  functions halflife-updated moved). Both arches build.
- Runs: on the rig, ours and the original boot, load their cfgs and RhoBot, and change map; their
  smoke logs differ only as `reference/DEVIATIONS.md` says (engine-registered cvars,
  halflife-updated's extra cvars, server.cfg run once). Steven has played ours on the rig.
- Built on SDK 2.3 with `CLIENT_WEAPONS`, like this repo: the vtable pointer sits at +0x60, after
  SDK 2.3's own CBaseEntity members. OZ's entity set is the original's: 20 SDK monster files are out
  of the CMake list, since OZ has none of those monsters.
- Crashes and hangs in the original are guarded (`reference/DEVIATIONS.md`, Crashes in the
  original). Its other bugs are kept and listed (`reference/ORIGINAL_BUGS.md`) for the cleanup
  pass, except the frame-rate dependent counters Steven had fixed early (the regeneration rune,
  spawn protection, the join HUD), each a `Fix original bug:` commit.
- A new cvar or command goes into `docs/SERVER.md` (`scripts/check_docs.py` fails otherwise), and
  a gameplay change into `docs/GAMEPLAY.md`.
- Next: the cleanup pass, once play testing against the original has run its course: OZ's own
  bugs, one commit each (`Fix original bug: ...`), with anything that changes play put to Steven
  first; the same pass runs in mod-rockcrowbar and mod-rc2.
