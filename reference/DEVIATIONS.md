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

## Entities

- **The same entity set as the original, plus `func_vehicle` and `func_vehiclecontrols`.** OZ
  3.02 has none of the SDK's monsters in 20 of its files (aflock, agrunt, barnacle, barney,
  bigmomma, bullsquid, gargantua, gman, hassassin, headcrab, hgrunt, houndeye, ichthyosaur, islave,
  leech, nihilanth, rat, roach, scientist, zombie): they are out of `dlls/CMakeLists.txt`, so a map
  that places one gets the engine's "Can't init" as on the original. The two vehicle entities are
  halflife-updated's, kept as in mod-rockcrowbar.

## halflife-updated's calls that were unclear

Steven, 2026-11-07: a halflife-updated change that is neither clearly a fix nor clearly design goes
to halflife-updated's version (`../mod-rc2/reference/audit/FIXES.md`, mod-ozdm section).

- **Weapon fire rates go through `GetNextAttackDelay`** (`dlls/crowbar.cpp`, `glock.cpp`, `mp5.cpp`,
  `shotgun.cpp`, `crossbow.cpp`), at OZ's cycle cvars. It evens out the creep between a shot's due
  time and the frame it fires on, as Valve's predicting client does. The original sets
  `UTIL_WeaponTimeBase() + cycle` (the mp5 SDK 2.3's accumulating cycle). weapons-b's weapons keep
  the original's timing (a design call made before the rule).
- **A gauss beam that hits its own shooter does no hitgroup damage** (`dlls/gauss.cpp`
  CGauss::Fire). The original keeps the last hitgroup, so a reflected beam could headshot the
  shooter at `head_damage`.
- **Explosions pull out of a wall by 0.6 units** (`dlls/ggrenade.cpp` CGrenade::Explode). The
  original pulls out by (dmg - 24) * 0.6, 57.6 units for a 120-damage rocket, which could put a big
  explosion on the far side of a thin wall.
- **The zooms set `m_iFOV` alone** (`dlls/glock.cpp`, `python.cpp`, `mp5.cpp`, `crossbow.cpp`,
  `weapons.cpp` DefaultDeploy; `dlls/client.cpp` UpdateClientData sends it as the client's fov).
  halflife-updated's 6f8aac9 (halflife #3044). The original keeps `pev->fov` beside it and sends
  that; its MP5 sniper zoom set only `pev->fov`, and a weapon switch reset only `m_iFOV`.
- **env_sound's room type is an int** (`dlls/sound.cpp`, halflife-updated's fdc159c), as in both
  RC projects; the original's member is the float `m_flRoomtype`. Same values in practice.
- **server.cfg runs once, at startup, from the engine** (`dlls/multiplay_gamerules.cpp`). The
  original's multiplay rules also exec it at every map change (twice on the first, since the
  engine runs it too); halflife-updated's seed left that to the engine. A cvar changed by hand
  during a game is no longer reset at the next map.
- **`ForceClientDllUpdate` resends the whole HUD state** (`dlls/player.cpp`): hide HUD, FOV, weapon
  bits, room type and ammo, besides SDK 2.3's health and battery.

## Crashes in the original

- **Runes on a map without deathmatch starts** (`dlls/items.cpp` SpawnRunes, CRune::Move). The
  original picks a random `info_player_deathmatch` and uses it unchecked, so such a map crashes it.
  Ours spawns no rune there, and a rune due to move stays where it is.
- **The grapple bolt doesn't create entities** (`dlls/grapple.cpp` CGrappleBolt::BubbleThink). The
  original looks up its owner every think and never uses it; with no owner, `GetClassPtr(NULL)`
  creates a new entity each time, until the server runs out of edicts. Ours drops the lookup.
- **A bot's goal search always steps to the next entity** (`dlls/bot_ai.cpp`
  CBaseBot::ActionChooseGoal). The original `continue`s past an owned "weapon*" entity without
  stepping, so a weaponbox in view of a bot with nothing in slots 1-3 hangs the server.
- **A bot holding only a satchel or tripmine stops choosing** (`dlls/bot_ai.cpp`
  CBaseBot::ActionChooseWeapon). UseSatchel and UseTripMine call ActionChooseWeapon, which in the
  original dispatches the same weapon again when nothing can be switched to: recursion until the
  stack runs out. Ours returns there when no carried weapon has a desire and can deploy.
- **A bot's fight style works without an enemy** (`dlls/bot_fightstyle.cpp` UseCrossbow, UseMP5,
  UseShotgun). The original reads the enemy's origin unchecked (a bot aiming at a tripmine with no
  enemy gets there). Ours takes the distance as 0.
- **A bot with no weapon out has a mood** (`dlls/bot_ai.cpp` CBaseBot::ThinkMood). The original
  reads the active item unchecked; a live bot has none after a player_weaponstrip, with no start
  weapons, or after its last grenade. Ours skips the reload and crowbar tests then.
- **The nav file is written only when it opens** (`dlls/bot_nav.cpp` CGraph::SaveNavToFile). The
  original formats the path into 50 bytes and writes through an unchecked fopen, so a long map
  name, or a server with no `<runfolder>/nav` directory, crashes the map change. Ours has a
  256-byte path and skips the file.
- **Bot strings fit their buffers** (`dlls/bot_stats.cpp` ReadBotFile, `dlls/bot_misc.cpp`
  UseBotCycle and BotClientPutInServer). The original strcpy's `bot_name`, `bot_model` and the two
  colour cvars into 64- and 4-byte buffers, a bot cycle line into a 100-byte exec command, and
  runfolder plus that line into a 120-byte path. Ours cuts the cvars to fit and gives the command
  128 bytes and the path 256.
- **The "bot" cvar and `removebot`'s reply fit their buffers** (`dlls/client.cpp` StartFrame,
  ClientCommand). The original strcpy's the cvar into an 80-byte static and sprintf's the
  client's argument into 128 bytes. Ours cuts both to fit.
- **PlayerPostThink tests the player first** (`dlls/client.cpp`). The original calls through it
  before its own null check at the end. Ours returns at the top.
- **The join HUD lines fit their buffer** (`dlls/client.cpp` PlayerPostThink). The original
  sprintf's `oz_link`, and `oz_line1` with `oz_line2`, into 201 bytes on every join. Ours cuts the
  text to fit.
- **A generic-hitgroup hit prints an empty center message** (`dlls/player.cpp`
  CBasePlayer::TraceAttack). The original leaves the message pointer uninitialized for
  HITGROUP_GENERIC and any hitgroup above 7.
- **The booby trap makes one grenade** (`dlls/player.cpp` CBasePlayer::PlayerDeathThink). The
  original also creates a CGrenade it never uses, one empty edict per blast for the rest of the
  map. Ours calls the static ShootTimed alone.
- **Removing an item clears the last item too** (`dlls/player.cpp`
  CBasePlayer::RemovePlayerItem). The original's `else if` leaves m_pLastItem on a removed item that
  was both active and last (SwitchWeapon doesn't update it), and `lastinv` then deploys it.
  Ours has halflife-updated's `if`, as mod-rockcrowbar.
- **A thrown crowbar under the Super Crowbar Rune is removed** (`dlls/flyingcrowbar.cpp`
  CFlyingCrowbar::SpinTouch). The original leaves each one invisible and thinking for the rest of
  the map, and the rune throws without limit. Ours removes it 0.1 s after the hit, as a throw
  without the rune does.
