# mod-ozdm

**Mission: a functional, source-faithful reconstruction of the Oz Deathmatch 3.02 server game
library as modern C++ on the halflife-updated SDK, that builds for every arch the buildchain
targets, loads in the engine, and behaves identically to the original.**

Oz Deathmatch (AfterShock Productions, 1999-2003) is a server-only Half-Life deathmatch mod: runes,
the grappling hook, CTF (Fire & Ice), teamplay and the built-in RhoBot bots. Clients run Half-Life's
own client library and download nothing, so there is no client to rebuild.

## Method

Same as `../mod-rockcrowbar` (Rocket Crowbar 1.9), whose original was built with the same compiler
(egcs 1.1.2): an unstripped Linux .so, so every function, vtable, save table and source file has its
name; `../DECOMPILE.md` section 3. Behavior parity on a modern compiler; the swap contract is the
external interface (entities, keyvalues, cvars, commands, user messages, the cfg/bot/nav files).
halflife-updated's bug fixes stay; the original comes back only where halflife-updated changed
design or balance (`../mod-rc2/reference/audit/FIXES.md` has the rule as applied).

Two sources besides the binary:
- the OZ 1.7 source (`~/Packages/OZDM`, nekonomicon/OZDM, AfterShock's own release): real names and
  code for what survived into 3.x; every OZ change in it is marked `// Oz`. Its licence asks for
  credit to Oz Deathmatch and no release of modifications under the Oz name.
- `reference/mp_i386.so` (3.0, pgcc 2.91.66): the release before 3.02, for diffs.

## This repo

A fork of `goldsrc-net/halflife-updated` branch `64bit-demo`, like mod-rockcrowbar.

- `upstream` = `git@github.com:goldsrc-net/halflife-updated.git`, tracked by `main`.
- No `origin` yet. When it gets one it is a private goldsrc-net repo (`../REPOS.md`).

| Path | What |
|---|---|
| `game/oz/` | the mod's gamedir manifest (`liblist.gam`) |
| `reference/` | the original libraries and everything derived from them (`reference/README.md`); `RECONSTRUCTION.md` is the work list |
| `scripts/` | `build.sh`, the table generators from mod-rockcrowbar (`inventory`, `hierarchy`, `vtables`, `savefields`, `cvars`, `sdk_baseline`), `ghidra/` (`re.sh`; `ApplyRC.java` with VPTR 0x60) |

Original content: the 3.02 Steam server package at `/media/Data3/Projects/mods/oz/`
(`x-oz3-02_for_steam/.../half-life/oz/`), downloaded from the sparksonline.org mirror the official
site links (`/media/Data3/Projects/mods/oz/downloads/`).

## Status (2026-11-07)

- Scaffolded per `../DECOMPILE.md` sections 1-4: the gamedir manifest, the reference libraries,
  build.sh (stock SDK builds on i386 and amd64), the tables (`reference/RECONSTRUCTION.md`: 2,964
  functions, 246 in OZ's 12 own files; 391 cvars, 329 not in our build; 83 OZ-only entities).
- Built on SDK 2.3 with `CLIENT_WEAPONS`, like this repo: the vtable pointer sits at +0x60, after
  SDK 2.3's own CBaseEntity members (the `ammo_*` and throw/charge fields RC 1.9's SDK 2.1 lacks).
- Section 5 done: OZ's 386 cvars in `game.cpp`/`game.h` in the original's order (decisions in
  `reference/DEVIATIONS.md`), OZ's base virtuals (`CBaseMonster::BotThink`,
  `CBasePlayer::IsBot`/`UpdateRuneInfo`, SDK 2.x's `CBasePlayerWeapon::AddWeapon`), placeholders
  for OZ's own files. OZ has no shared base classes of its own; its four classes with vtables
  (`CBaseBot`, `CGrappleBolt`, `CFlyingCrowbar`, `CRune`) each go with their file.
- Next: section 6, the packages (`reference/dossiers/AGENT_BRIEF.md`). The 1.7 source's `// Oz`
  markers say which stock functions OZ changed.
- Later (Steven, 2026-11-07): once it runs against the original, a cleanup pass fixes OZ's own
  bugs, one commit each (`Fix original bug: ...`), with anything that changes play put to Steven
  first; the same pass runs in mod-rockcrowbar and mod-rc2. Until then original bugs are kept and
  listed (`reference/ORIGINAL_BUGS.md`), except crashes, which are guarded (`reference/DEVIATIONS.md`).
- Steven, 2026-11-07: a halflife-updated change neither clearly a fix nor clearly design goes to
  halflife-updated's version; design reverts already made stay (`../mod-rc2/reference/audit/FIXES.md`).
