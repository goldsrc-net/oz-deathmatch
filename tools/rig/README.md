# tools/rig - running the original and the rebuild side by side

Two dedicated servers on this machine, both with gamedir `oz`, so one client folder joins
either:

```
original OZ 3.02 (reference/hl_i386.so)   tools/rig/run.sh original   port 27041
our rebuild (build-i386/dlls/hl.so)       tools/rig/run.sh ours       port 27042

steam -applaunch 70 -game oz +connect 127.0.0.1:<port>   (or "connect" in the console)
```

Set up or refresh with `tools/rig/setup.sh` after `scripts/build.sh i386`. It copies our latest
`hl.so` every time and creates the rest only if missing. The ports are clear of the RC rigs'
27031 and 27032.

## What it sets up

- `~/Containers/hlds/hlds-linux32-test/oz/`: the 3.02 content and the original library. The rig
  is a 32-bit ReHLDS install (goldsrc-net buildchain drop-in) shared with other work; only this
  gamedir and `ozours/` (smoke.py's copy of ours) are added.
- `~/Containers/hlds/hlds-linux32-ozours/`: symlinks to every entry of that rig, plus its own
  real `oz/` with our `hl.so`. Writes into `valve/` (logs, banned.cfg) land in the shared rig.
- `~/.local/share/Steam/steamapps/common/Half-Life/oz/`: the client's mod folder, the 3.02
  content with the player's own `valve/config.cfg` (OZ ships none).
- `tools/rig/lib/` (gitignored): `libstdc++-libc6.1-1.so.2`, below.
- Server-side change from stock 3.02: `sv_cheats 1` in `cfg/multiplayer.cfg`, which GameDLLInit
  runs after the command line. Everything else is the release as shipped.

The content comes from the 3.02 Steam server package's `oz/` folder (`OZ_SRC`, default
`oz3-02_for_steam.zip` unpacked in `~/Downloads/oz3-02_for_steam`; `reference/README.md` has where
it was downloaded). The rig paths above are setup.sh's defaults; `RIG`, `RIG2` and `CLIENT` override
them.

## The original's C++ runtime

The original was built with egcs 1.1.2 and needs its C++ runtime, `libstdc++-libc6.1-1.so.2`,
which no current system ships. setup.sh downloads Debian 2.2's package of it
(`libstdc++2.9-glibc2.1` 2.91.66-4, built from egcs 1.1.2) from archive.debian.org, checks its
sha256 (its md5 is the one in potato's Packages index), and unpacks the library into
`tools/rig/lib/`; run.sh puts that on the original's library path only. It is a runtime need of
the original alone: our build links only libc, libm and libdl (and libgcc_s on amd64).

`-nobreakpad` in run.sh: without it the engine's breakpad handler uploads a minidump to Valve's
crash server whenever the server crashes.

## Smoke test

`tools/rig/smoke.py` boots one server headless on port 27043, changes map twice, quits, and
saves the log; `--clean` strips run-specific noise so two logs diff:

```
tools/rig/smoke.py oz orig.log
tools/rig/smoke.py ozours ours.log
diff <(tools/rig/smoke.py --clean orig.log) <(tools/rig/smoke.py --clean ours.log)
```

## Undo

Move these away (nothing else is touched):

```
~/Containers/hlds/hlds-linux32-test/oz
~/Containers/hlds/hlds-linux32-test/ozours
~/Containers/hlds/hlds-linux32-ozours
~/.local/share/Steam/steamapps/common/Half-Life/oz
tools/rig/lib
```
