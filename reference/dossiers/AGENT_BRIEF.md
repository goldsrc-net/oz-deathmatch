# Brief: reconstructing an Oz Deathmatch 3.02 work package

You are rebuilding part of the original Oz Deathmatch 3.02 server game library (`hl_i386.so`,
2004, egcs 1.1.2, not stripped) as modern C++ on the halflife-updated SDK. Your package is a set of
the original's source files; other agents do the other files in parallel, each in its own git
worktree. The bar is identical behavior. Exact binary layout is NOT a goal.

## Where things are

- **Your worktree** (edit only here): `~/Projects/goldsrc.net/mod-ozdm-wt/<package>`, branch
  `wt/<package>`. The main checkout is `~/Projects/goldsrc.net/mod-ozdm`; don't edit it.
- **Read first, in full:** the worktree's `CLAUDE.md`, `reference/README.md`,
  `reference/DEVIATIONS.md`, `reference/dossiers/README.md` (the dossier format), and your
  files' sections of `reference/RECONSTRUCTION.md`.
- **The decompile** (typed, read-only, shared): `~/Projects/goldsrc.net/mod-ozdm/recovered/decomp/`
  - one `<Class>.c` per class, free functions in `_free.c`. Each function is headed
  `/* Class::Method  @ <ghidra addr>  (<bytes>) */` then `/* <original file>.cpp */`. Free
  functions keep their gcc2 names (`UTIL_IsValidEntity__FP7edict_s`). Do NOT run Ghidra
  (`scripts/ghidra/re.sh`): one project, one lock, several agents.
- **Tables** in `reference/`: `funcfile.tsv` (every original function: address, size, file, class,
  method, signature; filter on your files), `members.tsv`/`savefields.tsv` (saved members: real
  names, types, the original's offsets), `vtables.tsv` (class, slot, method, implementer, args),
  `hierarchy.tsv` (each class's base), `signatures-gcc2.txt`, `signatures-mp.txt` (the Windows
  build's exports), `cvars.tsv`. `python3 scripts/vtables.py --new <Class>` lists the virtuals a
  class introduces over its base.
- **The binary** for when the decompile is unclear: `objdump -d --no-show-raw-insn -C
  --start-address=0x<a> --stop-address=0x<b> reference/hl_i386.so`. Ghidra address = file address
  + 0x10000 (funcfile.tsv has file addresses).
- **The OZ 1.7 source** (`~/Packages/OZDM`, read-only): AfterShock's own release of an earlier
  version, every OZ change marked `// Oz`. Use it for names, comments and intent where 3.02 still
  has the same code; the 3.02 binary decides what the code does. 1.7 has no bots.
- **The 3.02 content** (cfg, bot and map files the code reads):
  `/media/Data3/Projects/mods/oz/x-oz3-02_for_steam/SteamApps/YOUR_steam_username/half-life/oz/`.

## Reading the decompile

- `this->vtbl->Method(this, ...)` is a virtual call, already resolved. Where it isn't (an untyped
  pointer): the vtable pointer sits at `+0x60` in every entity, and a call through `vptr + o` is
  slot `o/4 - 2` in `vtables.tsv` for the pointer's class.
- `this->field_0xNN` is an unsaved member; name it from what it does (the 1.7 source often has the
  name), keep the original offset in a comment (`// orig +0x2a4`), and add `VERIFY name` when it's
  a guess. Saved members have their real names already (`this->m_pGoalEnt`).
- `(*g_engfuncs_pfnXxx)(...)` is the engine call: write the SDK macro (`pfnMessageBegin` ->
  `MESSAGE_BEGIN`, `pfnPrecacheModel` -> `PRECACHE_MODEL`, `pfnEmitSound` via `EMIT_SOUND_DYN`...).
  `gpGlobals->pStringBase + x` is `STRING(x)`.
- `CONCAT31(extraout_var, b)` is a bool or byte return; take the low byte.
- An edict loop stepping `+ 0x324` is `pEdict++` from `INDEXENT(1)`; `*(int*)edict == 0` is
  `!pEdict->free`; `*(int*)(edict + 0x7c)` is `pvPrivateData` (use `CBaseEntity::Instance`).
- Pointer-to-member think/touch/use are two words: `this->m_pfnThink = X; field_0x10 = 0` is
  `SetThink(X)`.
- FP constants are labelled with their value (`dc_0_1` = 0.1).
- Ghidra sometimes splits a huge function into `FUN_` fragments or loses the end of one; read the
  disassembly for those, don't guess.

## How to write it

- Same file names as the original. OZ's own `.cpp` files exist as placeholders and are already in
  `dlls/CMakeLists.txt`, except the glock: OZ's `hl_wpn_glock.cpp` is our `dlls/glock.cpp`. Put
  class declarations in a header of your own when other files will need them (`dlls/<file>.h`),
  else in the `.cpp`.
- Modern SDK style, matching the surrounding halflife-updated code: `bool` returns, `override`,
  `SetThink`/`SetTouch`, `pev->`, `LINK_ENTITY_TO_CLASS`, `DEFINE_FIELD` save tables in the
  original's order with its field types (`savefields.tsv`), `IMPLEMENT_SAVERESTORE`.
- Original names everywhere they exist: classes, methods, saved members, entity classnames,
  keyvalue names, sound/model paths, cvar names, bot file keys. Their typos too.
- Each class's base from `hierarchy.tsv`; its new virtuals in the original's slot order.
- Every function gets a one-line provenance comment with its original address
  (`// orig 0x15a2f0`). Mark every uncertain spot `// VERIFY: <why>`.
- Keep the original's bugs and list each in the dossier's "Original bugs" section;
  `reference/ORIGINAL_BUGS.md` collects them.
- Never invent behavior. If the decompile and the disassembly don't settle it, write the most
  literal reading, mark it VERIFY, and put it in the dossier.

## Stock SDK files

Most of OZ is changes inside the SDK's own files. Those files already exist here as
halflife-updated's versions. For each one in your package:

1. **Add every function OZ added** (`check_file.py <file> --missing` lists them).
2. **Bring every stock function's body in line with the original.** `reference/oz_touch.tsv` lists
   the stock functions whose original body uses an OZ name; that's a lead list, not complete (a
   changed constant or a removed line references nothing new). Weapon, player, gamerules, client
   and item files: compare every function. Monster files: compare the ones in `oz_touch.tsv` and
   the ones OZ added, then read the rest of the file's decompiles quickly. Say in the dossier which
   you compared and which you only skimmed.
3. **halflife-updated's changes.** Where our stock body differs from the original because
   halflife-updated fixed a bug, keep the fix and comment it `// halflife-updated's fix, kept:
   <what>`. Where halflife-updated changed design or balance (damage, timings, a removed check
   that was deliberate, a new feature), the original's behavior wins. The rule and every call made
   so far are in `~/Projects/goldsrc.net/mod-rc2/reference/audit/FIXES.md`; the same function gets
   the same verdict here. `git log upstream/64bit-demo -S'<code>' -- <file>` finds the commit and
   its reason. Put each call you make in the dossier, with the commit; unclear ones are flagged,
   not decided.

OZ was built with `CLIENT_WEAPONS`, as this repo is: its `GetWeaponData` fills the weapon data and
`UTIL_WeaponTimeBase()` returns 0, so keep the SDK's prediction branches (`#if defined(
CLIENT_WEAPONS)`) as they are. OZ is server-only: players run Valve's own client, which predicts
stock Half-Life weapons, so an OZ weapon change mispredicts there as it did in 2004. Reconstruct
the server behavior and note each such change in the dossier.

## Shared files

You own only your package's files and any new header you create. Do not touch `CMakeLists.txt`,
`game.cpp`, `game.h` or other packages' files. `weapons.h`, `player.h` and `cbase.h` are edited by
several packages: keep each edit inside your own classes' declarations. When you need something
that isn't there:

- **A member on a shared class** (`CBasePlayer`, `CBaseEntity`, ...): add it in a block
  `// <package> needs:` right before the class's closing `};`, and list it in the dossier.
- **A function, global or user message from elsewhere**: declare it `extern` in your file and, if
  needed to link, give it a temporary definition at the bottom of your own `.cpp` in a block marked
  `// TEMP STUB (<package>): real one belongs in <file>`. List each in the dossier.

Don't reformat or reorder anything you don't change.

## Checking

```sh
cd ~/Projects/goldsrc.net/mod-ozdm-wt/<package>
scripts/build.sh i386            # Docker; a few minutes; run before amd64
scripts/build.sh amd64
python3 scripts/check_file.py <file>.cpp --missing   # must reach 0 missing, or explain each
nm -D build-i386/dlls/hl.so | grep -w <classname>    # each entity factory exported
```

Both arches must build. Several agents share this machine: one build at a time, no `make -j`
beyond what `build.sh` does. Nothing runs in an engine yet, so "built" is the most you can claim.

## Finishing

- **Don't commit** (commits are gpg-signed by Steven's key and would prompt him). Leave your
  changes uncommitted in the worktree.
- Write `reference/dossiers/<package>.md` in the worktree, in the format of the dossier README.
- Your final message: the worktree path, check_file results per file, both build results, the
  shared-file edits and stubs you made, the halflife-updated calls you made, and the VERIFY count.
  Keep it short; the dossier has the detail.
