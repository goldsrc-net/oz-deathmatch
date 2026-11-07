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
| `reference/` | the original libraries and everything derived from them (`reference/README.md`) |

Original content: the 3.02 Steam server package at `/media/Data3/Projects/mods/oz/`
(`x-oz3-02_for_steam/.../half-life/oz/`), downloaded from the sparksonline.org mirror the official
site links (`/media/Data3/Projects/mods/oz/downloads/`).

## Status (2026-11-07)

Scaffolded: the gamedir manifest and the reference libraries. Next, DECOMPILE.md sections 1-6:
build.sh, the tables, Ghidra, the base classes, the packages.
