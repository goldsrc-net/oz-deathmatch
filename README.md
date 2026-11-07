# Oz Deathmatch 3.02 (RE)

A fan-made reconstruction of Oz Deathmatch 3.02, the Half-Life deathmatch mod by AfterShock
Productions, as C++ source on the Half-Life SDK. It rebuilds the mod's server library from 3.02's
shipped binaries, so that it drops into a stock 3.02 install and plays the same: the runes, the
offhand grappling hook, the built-in RhoBot bots, teamplay and the cfg files that set nearly
everything. 3.02 has no CTF (1.7 had it). OZ is server-only; players use Half-Life's own client and
download nothing.

This reconstruction is not affiliated with or endorsed by AfterShock Productions. Oz Deathmatch, its
name, models, sounds and configs are theirs; this replaces only the code. AfterShock released the
OZ 1.7 source on the terms that modifications are not released under the Oz name and that what is
used is credited, so this repository is a private reconstruction, not a release of OZ. See
[CREDITS.md](CREDITS.md).

## Installing

The rebuild is a server library for a stock 3.02 install: put `hl.so` in `oz/dlls/` and point
`gamedll_linux` in `oz/liblist.gam` at it. [INSTALL.md](INSTALL.md) covers servers, the test setup
that runs the original and the rebuild side by side, and going back.

## Servers

The rebuild reads 3.02's cvars, `cfg/` files, per-map cfgs, bot cfgs, `botcycle.txt` and nav files
unchanged. [docs/SERVER.md](docs/SERVER.md) lists every cvar, command, impulse and file OZ adds to
Half-Life, and [docs/GAMEPLAY.md](docs/GAMEPLAY.md) what OZ changes in play. Where it deliberately
does something 3.02 does not, it is listed in
[reference/DEVIATIONS.md](reference/DEVIATIONS.md); the bugs found in the original, and which ones it
keeps, are in [reference/ORIGINAL_BUGS.md](reference/ORIGINAL_BUGS.md).

## Building

The Linux libraries build in the [goldsrc-net/buildchain](https://github.com/goldsrc-net/buildchain)
Docker image (`scripts/build.sh i386`, `scripts/build.sh amd64`). [BUILDING.md](BUILDING.md) has the
steps.

## How it was made

The original `hl_i386.so` was built with egcs 1.1.2 and kept its symbols, which gives every
function's name, its class hierarchy, vtables and save tables, and the names of the source files it
was built from. The whole library was decompiled with Ghidra, typed from those tables, and rewritten
on top of [Half-Life Updated](https://github.com/twhl-community/halflife-updated). AfterShock's own
OZ 1.7 source gave names and intent for the code that survived into 3.x; the 3.02 binary decided
what the code does. The rebuild was then checked by building, running and comparing against the
original running in the same engine. `reference/` holds the originals and the derived inventories;
`scripts/` builds and checks them, and `tools/rig/` runs the two side by side.

## License

The rebuild is built on the Half-Life SDK and is distributed under its license ([LICENSE](LICENSE)): free
of charge only. Half-Life Updated's own changes are in [CHANGELOG.md](CHANGELOG.md) and
[FULL_UPDATED_CHANGELOG.md](FULL_UPDATED_CHANGELOG.md), and its contributors in [CREDITS.md](CREDITS.md).
