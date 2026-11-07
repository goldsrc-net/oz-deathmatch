# Deviations from Oz Deathmatch 3.02

Everything this rebuild does differently from the original `hl_i386.so`, whether on purpose or
forced by the build, with its default and what it changes. Same convention as mod-rockcrowbar's
`reference/DEVIATIONS.md`.

## Build and layout

- **Member offsets and vtable slots differ.** We build with a modern compiler on halflife-updated.
  egcs put the vtable pointer after a class's base data (+0x60 in every entity here); every modern
  compiler puts it at 0, and halflife-updated adds virtuals of its own (the virtual destructor,
  `MyTogglePointer`, the HL25 sentence virtuals on `CBaseToggle`, the client-prediction virtuals
  on `CBasePlayerItem`, `DoRetireWeapon`). Anything that reads the original's memory by offset or
  hooks it by vtable slot (Metamod or AMX Mod X plugins written for OZ 3.02) needs our layout.
- **The glock is in `dlls/glock.cpp`.** The original's file is `hl_wpn_glock.cpp`, which is the
  client's file name in this tree.

## Cvars

The cvar set is checked by `scripts/cvars.py` (`reference/cvars.tsv`). `GameDLLInit` registers
OZ's 386 in the original's order and runs its `exec` lines (`skill.cfg`, then the seven
`cfg/*.cfg`).

- **`servercfgfile`, `lservercfgfile`, `mapcyclefile` and `motdfile` are not registered.** The
  original registers them (defaults `server.cfg`, `listenserver.cfg`, `mapcycle.txt`, `motd.txt`,
  no flags), as Valve's SDK 2.x did. The current engine defines all four itself, and
  halflife-updated's seed (Valve's 2013 SDK) already left them out. Behavior is the same.
- **Three cvars the original defines and never registers are left out:** a second
  `customrunemodel` and a second `rune_model` (the code reads the registered ones, by name through
  the engine or by symbol), and `jump_height` (`oz_jumpheight`), which no code reads.
- **Cvars the original does not have, kept from halflife-updated:** `allow_spectators`,
  `sv_allowbunnyhopping`, `sv_busters`, `sv_pushable_fixed_tick_fudge`. They are registered where
  halflife-updated registers them relative to the original's own.
- **OZ's own choices are kept** (they are the original, not deviations, listed here because they
  differ from halflife-updated): `mp_allowmonsters` defaults to 1 and is unlogged, and 216 of the
  SDK's `sk_*` skill cvars are not registered (every monster's health and damage, and the
  `sk_monster_*`/`sk_player_*` hitgroup multipliers, which OZ replaces with `head_damage` and the
  like). `skill.cfg`, from `valve/` since OZ ships none, sets those as unknown commands, and
  `GetSkillCvar` reads 0 for each, so any monster a map spawns gets 0 from the skill table, as in
  the original. OZ keeps the turret, sentry, bullet, hornet, charger, battery, healthkit and
  scientist-heal ones.

## Crashes in the original

- **Runes on a map without deathmatch starts** (`dlls/items.cpp` SpawnRunes, CRune::Move). The
  original picks a random `info_player_deathmatch` and uses it unchecked, so such a map crashes it.
  Ours spawns no rune there, and a rune due to move stays where it is.
- **The grapple bolt doesn't create entities** (`dlls/grapple.cpp` CGrappleBolt::BubbleThink). The
  original looks up its owner every think and never uses it; with no owner, `GetClassPtr(NULL)`
  creates a new entity each time, until the server runs out of edicts. Ours drops the lookup.
