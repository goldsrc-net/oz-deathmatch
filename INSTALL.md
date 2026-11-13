# Installing

The rebuild replaces one thing of an Oz Deathmatch 3.02 server: its game library. Everything else (the
`cfg/` files, per-map cfgs, bots, nav files, the models, sounds and maps) is 3.02's own and stays as it
is. Players need nothing new: OZ has no client library, so they join with Half-Life's client and
download nothing.

## A server

1. Install the 3.02 Steam server package (`oz3-02_for_steam.zip`) on the server: its `oz/` folder goes
   next to `valve/`, and its `botcycle.txt` next to `hlds_linux` (the bot cycle is read from the
   server's working folder, not the mod's).
2. Copy the `oz/` folder of a `reOZ-<rev>` package (CI's artifact or release, or
   `scripts/package.sh`, [BUILDING.md](BUILDING.md)) over the server's `oz/`. It holds the server under
   3.02's own names: `dlls/hl.so`, `hl_i386.so` and `hl_i686.so` for Linux (Steam's current engine
   loads `hl.so`, older ones `hl_i386.so`, Metamod-R `hl_i686.so`), `dlls/mp.dll` for Windows, and the
   64-bit servers.
3. On Linux, `oz/liblist.gam` needs `gamedll_linux "dlls/hl_i386.so"`. The Steam package's
   `liblist.gam` names only the Windows `mp.dll`; its `liblist.gam_OLD`, the one 3.02 shipped, has
   the line. A Windows server needs no change. A 64-bit ReHLDS loads that line as written, so on
   one, point it at `dlls/hl_amd64.so` (x86-64) or `dlls/hl_arm64.so` (ARM) instead; Xash3D finds
   the 64-bit name from the line by itself.
4. Start the server with `-game oz`.

`runfolder` in `oz/cfg/rhobot.cfg` (shipped as `oz`) is where the bots look for `bots/` and `nav/`.
The server writes nav files there at map change, so the folder must be writable by the server's user.

Going back: put the original `hl_i386.so` (the 3.02 Linux update, `oz_3.02.tar.gz`) and `mp.dll` back
in `oz/dlls/`, and delete `hl.so`, `hl_i686.so` and the 64-bit servers, which 3.02 never had (Steam's
engine would go on loading `hl.so`). On a current system the original only loads with egcs 1.1.2's C++
runtime, `libstdc++-libc6.1-1.so.2`, on its library path (`tools/rig/setup.sh` fetches it); the
rebuild needs nothing.

## Running the original and the rebuild side by side

`tools/rig/setup.sh` sets up two servers on one machine, the original on port 27041 and the rebuild on
27042, both as `oz`, and the client's mod folder; `tools/rig/run.sh original|ours` starts them. Join
with:

```
steam -applaunch 70 -game oz +connect 127.0.0.1:<port>
```

[tools/rig/README.md](tools/rig/README.md) has the details and how to undo it.
