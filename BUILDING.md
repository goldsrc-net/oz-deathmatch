# Building

The server library builds on Linux in the [goldsrc-net/buildchain](https://github.com/goldsrc-net/buildchain)
Docker image (Debian 10, glibc 2.28), so the result runs on any Linux with glibc 2.28 or newer. Build the
image once from a buildchain checkout:

```sh
( cd ../hlds64-buildchain && make image )      # -> hlds64-buildchain:debian10
```

Then, from this repository:

```sh
scripts/build.sh i386      # -> build-i386/dlls/hl.so, for a 32-bit HLDS (and the original's engine)
scripts/build.sh amd64     # -> build-amd64/dlls/hl.so, for a 64-bit HLDS
BUILDTYPE=Debug scripts/build.sh i386   # -O0 with debug info
```

`scripts/build.sh` only consumes the image; this repository isn't a buildchain submodule. CMake is the
build system (`dlls/CMakeLists.txt`). The server is built with `CLIENT_WEAPONS`, as OZ 3.02's was on
SDK 2.3. The 20 SDK monster files OZ has no monsters from are left out of the CMake list
([reference/DEVIATIONS.md](reference/DEVIATIONS.md), Entities).

Oz Deathmatch has no client library of its own, so there is nothing client-side to build; the `cl_dll`
tree is Half-Life Updated's and isn't part of the mod.

## Checking a build

```sh
python3 scripts/check_file.py <original source file>.cpp --missing   # every original function present
python3 scripts/cvars.py --ours build-i386/dlls/hl.so              # regenerate reference/cvars.tsv
scripts/check_docs.py                                               # every cvar and command is in docs/SERVER.md
tools/rig/smoke.py ozours out.log                                   # boot, change maps, quit (tools/rig/README.md)
```

`scripts/` also regenerates every table in `reference/` from the original library (see each script's
header, and the parent goldsrc.net folder's `DECOMPILE.md` for the order).
