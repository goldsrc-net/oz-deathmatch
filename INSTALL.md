# Installing

The rebuild replaces one file of an Oz Deathmatch 3.02 server: its game library. Everything else (the
`cfg/` files, per-map cfgs, bots, nav files, the models, sounds and maps) is 3.02's own and stays as it
is. Players need nothing new: OZ has no client library, so they join with Half-Life's client and
download nothing.

## A server

1. Install the 3.02 Steam server package (`oz3-02_for_steam.zip`) on the server: its `oz/` folder goes
   next to `valve/`, and its `botcycle.txt` next to `hlds_linux` (the bot cycle is read from the
   server's working folder, not the mod's).
2. Copy the rebuild's `hl.so` (`build-i386/dlls/hl.so` for a 32-bit HLDS, `build-amd64/dlls/hl.so` for a
   64-bit one) to `oz/dlls/hl.so`.
3. In `oz/liblist.gam`, set `gamedll_linux "dlls/hl.so"`. The package's `liblist.gam` names only the
   Windows `mp.dll`; its `liblist.gam_OLD` has the original's `dlls/hl_i386.so`.
4. Start the server with `-game oz`.

`runfolder` in `oz/cfg/rhobot.cfg` (shipped as `oz`) is where the bots look for `bots/` and `nav/`.
The server writes nav files there at map change, so the folder must be writable by the server's user.

Going back: put the original `hl_i386.so` (the 3.02 Linux update, `oz_3.02.tar.gz`) in `oz/dlls/` and
point `gamedll_linux` at `dlls/hl_i386.so`. On a current system the original only loads with egcs
1.1.2's C++ runtime, `libstdc++-libc6.1-1.so.2`, on its library path (`tools/rig/setup.sh` fetches
it); the rebuild needs nothing.

## Running the original and the rebuild side by side

`tools/rig/setup.sh` sets up two servers on one machine, the original on port 27041 and the rebuild on
27042, both as `oz`, and the client's mod folder; `tools/rig/run.sh original|ours` starts them. Join
with:

```
steam -applaunch 70 -game oz +connect 127.0.0.1:<port>
```

[tools/rig/README.md](tools/rig/README.md) has the details and how to undo it.
