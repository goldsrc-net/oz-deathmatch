# Dossier: bots (RhoBot)

## 1. Scope

Original files: `bot_ai.cpp`, `bot_chat.cpp`, `bot_fightstyle.cpp`, `bot_memory.cpp`, `bot_misc.cpp`,
`bot_move.cpp`, `bot_nav.cpp`, `bot_stats.cpp`, plus `GetClassPtr<CBaseBot>` (funcfile `(header)`,
the cbase.h template).

Written here: `dlls/bot.h` (new: the five classes, the three enums, the shared globals), the eight
`dlls/bot_*.cpp`. Shared file touched: `dlls/nodes.h` (a `// bots needs:` block in `CGraph`).

All eight are OZ's own files, so there are no stock bodies to compare and no halflife-updated
calls to make in this package.

## 2. Status

Built only (both arches); nothing has run in an engine.

```
file                 check_file --missing
bot_ai.cpp           65 of 65
bot_chat.cpp          1 of 1
bot_fightstyle.cpp   19 of 19
bot_memory.cpp        4 of 4
bot_misc.cpp          9 of 9
bot_move.cpp         40 of 40
bot_nav.cpp          13 of 15  (see below)
bot_stats.cpp         3 of 3
scripts/build.sh i386   ok, no warnings from these files
scripts/build.sh amd64  ok, no warnings from these files
```

The two bot_nav.cpp misses are `slurp_array__6CGraphPA255_A255_sPc` and
`parse_nav_line__6CGraphPA255_A255_sPc`: funcfile.tsv could not demangle them and lists them as
classless functions under their raw names. They are `CGraph` members (the disassembly passes
`this`, the array and the string), and our build has `CGraph::slurp_array(short (*)[256][256],
char*)` and `CGraph::parse_nav_line(...)`. Nothing to do.

The accessors (`Get*`/`Set*`, `IsNetClient`) are funcfile'd to bot_ai.cpp as out-of-line copies,
but bot_ai.cpp and bot_misc.cpp inline `SetEnemy`'s body, so they were inline in the class: they
are inline in bot.h and check_file counts them `inl`.

## 3. Function map

All `ok` unless listed: read from the decompile, and from the disassembly where the decompile was
cut or garbled (noted). Addresses are file addresses.

```
0x0832b0 CBaseBot::CBaseBot                  bot_ai.cpp
0x083560 ActionOpenFire                      bot_ai.cpp   disasm: vcall 0xfc = CanDeploy, compares
0x083700 ActionChooseGoal                    bot_ai.cpp   disasm: owned-weapon loop (bug 1); VERIFY weaponbox read
0x083b60 ActionChooseWeapon                  bot_ai.cpp   disasm: vcall 0x104 = CanHolster, ficom
0x083d20 ActionLook                          bot_ai.cpp   disasm: whole function
0x084850 ActionReload, 0x0848c0 ActionSpeak  bot_ai.cpp
0x084920 CheckBotKick                        bot_ai.cpp
0x084ac0 GetBotThinkDelay, 0x084b20 BotThink bot_ai.cpp
0x084ca0..0x085d50 CheckHasDecentWeapon, CheckNotice, CheckVisible, HandleTime, ThinkDead,
         ThinkMood, ThinkStart, ThinkSteering, ThinkValidateEnemy, ThinkValidateGoal,
         ActionCheckMercy, ActionCheckCloak  bot_ai.cpp
0x085df0 _GLOBAL_.I. (ammo_check table)      bot_ai.cpp   ammo_check[] initializer
0x0861a0..0x0866c0 IsNetClient, Get*/Set*    bot.h (inline)
0x086730 ActionChat                          bot_chat.cpp strings checked in .rodata
0x086bd0..0x087850 CBaseBotFightStyle (19)   bot_fightstyle.cpp  disasm: UseCrossbow, UseGauss args, SetNextShootTime
0x087880..0x087ba0 CBaseBotMemory (4)        bot_memory.cpp
0x087de0..0x088050 CBaseBotGlobals           bot_misc.cpp
0x0881d0 ChooseRandomName, 0x088270 ChooseRandomSkin   bot_misc.cpp  tables dumped from .data
0x088310 UseBotCycle                         bot_misc.cpp disasm: whitespace check
0x0885e0 BotClientPutInServer                bot_misc.cpp VERIFY: gamerules slot 19
0x088990 BotConnect                          bot_misc.cpp disasm: name filter
0x088bc0 CBaseBot::TakeDamage                bot_misc.cpp dead code in the original (no callers)
0x088e90/0x088ec0 BotGlobals ctor/dtor       bot_misc.cpp global definition
0x088ef0..0x08c6b0 bot_move.cpp (40)         bot_move.cpp disasm: AimAtBias, AimAtEnemy, AimWander,
         HandleObstacles, SteerGroupEscort/Follow, SteerPrimitiveWander, SteerSafe* callees
0x08c7f0 ActionChooseDirection               bot_nav.cpp
0x08cb30..0x08d850 CGraph nav methods (12)   bot_nav.cpp  disasm: FLoadBotNav, slurp_array,
         parse_nav_line, nextItem (decompile cut all three)
0x08d9a0..0x08e0b0 CBaseBotStats (3 + ctor)  bot_stats.cpp
```

## 4. Classes

No bot class has saved members (none in savefields.tsv); every name below is ours, from the
accessor names where one exists, else from use. Offsets are the original's.

- **CBaseBot** (base `CBasePlayer`, size 0x8c0). Virtuals: only `IsNetClient` (slot 40, returns
  false) and `BotThink` (slot 125, `CBaseMonster`'s). Members +0x6a0..+0x714 in bot.h, each named
  after its Get/Set pair; `m_flNextBotThink` (+0x6cc) has no accessor. Then `FightStyle` (+0x718),
  `Memory` (+0x734), `Stats` (+0x7e8). `TakeDamage` has an extra `CBasePlayer*` argument, so it is
  not the virtual; a `using CBasePlayer::TakeDamage` keeps the base one reachable through
  `CBaseBot*`.
- **CBaseBotFightStyle** (no base, 0x1c): aim point (+0), an attack mode only
  `OptimizeAttackMode` writes (+4), hold-attack (+8), secondary (+0xc), owner bot (+0x10), next
  and stop shoot times (+0x14, +0x18).
- **CBaseBotMemory** (no base, 0xb4): sighted time, in-sight flag, last seen position and time,
  then 4 frames of {enemy velocity, look direction, own velocity} (stride 0x24, shown by
  EndFrameUpdate's shifts), then the last origin. The frame struct `botmemoryframe_t` is ours.
- **CBaseBotStats** (no base, 0xd8): name[64], model[64], top/bottom colour char[4], 14 weapon
  preferences and 6 traits as floats, in the order the constructor fills them (the cvar order).
- **CBaseBotGlobals** (no base, 0xa0): next auto-adjust time, a 33-byte "boot_camp" nothing
  reads, the last bot cycle line (123 bytes to fill the object).
- **CGraph** additions (nodes.h block): `m_NavArray` short[256][256][256], at +0x24 in the
  original (after `m_nRouteInfo`), at the end of the class here. The original's FSaveGraph writes
  0x20020cc bytes of CGraph, nav array included, to every .nod file; ours copies `sizeof(CGraph)`
  too, so the array goes to and from .nod files the same way.

Enums `STEER`, `DIRECTION`, `TURNING_DIRECTION` are the original's type names (from the mangled
setters); their enumerator names are ours, values from the callers.

## 5. Cross-file needs

Provided by this package for others (declared in bot.h / nodes.h):
- client.cpp StartFrame: `BotGlobals.HandleAutoAdjDifficulty()`, `UseBotCycle(int)`,
  `BotConnect(int)`, `CBaseBot::BotThink` through the vtable, `WorldGraph.MarkLocationFavorable`.
- multiplay_gamerules.cpp ChangeLevel: `WorldGraph.SaveNavToFile()`.
- nodes.cpp InitGraph/AllocNodes: `CGraph::InitNavArray()`. The world package declared it in
  its own `// world needs:` block and left an empty TEMP STUB in nodes.cpp; both go at the merge.

Used from elsewhere:
- **TEMP STUB** `CBasePlayerByIndex(int)` at the bottom of `bot_misc.cpp`: real one belongs in
  client.cpp (orig 0x097eb0). The stub has the original's body (index range, `!free`,
  FL_CLIENT|FL_FAKECLIENT, GetClassPtr), so behaviour is right before the merge.
- `CBasePlayer::IsBot` (player.h), `CBaseMonster::BotThink` (basemonster.h), the bot cvars and
  `oz_max*` cvars (game.h): already there.
- Stock SDK: `CBasePlayer::TakeDamage/SwitchWeapon/IsOnLadder/GetGunPosition`, gamerules
  `CanHavePlayerItem/CanHaveAmmo/PlayerRelationship/PlayerThink`, `CSoundEnt`, `CBeam::GetStartPos`,
  `UTIL_SayTextAll`, `UTIL_EntitiesInBox`. No user messages.
- No members added to shared classes besides the CGraph block.

## 6. VERIFY list

31 markers: 27 are names we chose (members, enum values, the two structs); the rest:
1. `BotClientPutInServer` ends with gamerules vtable +0x54, which vtables.tsv names
   `PlayerThink` (slot 19). PlayerSpawn would be the obvious intent; we call PlayerThink as the
   binary does. A disassembly read of the original vtable around slot 18-19 would settle it.
2. `BotAmmoCount` (bot_ai.cpp): ActionOpenFire indexes `m_rgAmmo` with a weapon's ammo index,
   which is -1 when the weapon has no such ammo (the glock's secondary fire, 90% of the time). The
   original then reads +0x4d0, `m_pLastItem`. We return `m_pLastItem != NULL` for -1 rather than
   reading out of bounds in our layout. Runtime test: a bot with a glock fires secondary iff it
   has a last item.
3. `ActionChooseGoal` casts any "weapon*" entity to `CBasePlayerItem` and reads `m_pPlayer`; for a
   "weaponbox" the original reads `CWeaponBox::m_rgiszAmmo[1]` (+0x80), non-zero when the box
   holds ammo, which then hangs the server (bug 1). Our cast reads whatever our layout has there.
   Only reached by a bot with nothing in slots 1-3. Needs an engine test.

## 7. Behavior worth testing

- `bot_number`/`min_clients` with the shipped botcycle.txt (CRLF, in the half-life folder): the
  cycle line keeps its `\r`, so the exec of `bots/1st\r.cfg` and the ReadBotFile open should fail
  on Linux and bots get named "1st", "2nd"... with random skins and colours. Compare names,
  models and colours with the original.
- bot_skill_aa on: skill moves +5 when humans out-frag bots, -1 the other way, every 12 s.
- Bots facing a tripmine with no enemy and an MP5/shotgun/crossbow chosen (bug 3).
- The .nav file written at map change; the .nod file size (32 MB larger, as the original).
- ActionChat lines with bot_dontchat 0; mercy (bot_mercy 1) against a low-scoring human.
- A bot holding only a satchel or tripmine (bug 4).

## 8. Original bugs (all kept)

1. ActionChooseGoal (0x083700): an owned "weapon*" entity in the look list makes the loop
   `continue` without stepping to the next entity: infinite loop. Carried weapons have EF_NODRAW
   and are skipped first, so in practice only a weaponbox with ammo (VERIFY 3) reaches it.
2. ammo_check (0x085df0) is built at load time from the max_* cvars before they are registered,
   so every max is 0 and CanHaveAmmo always refuses: bots never go for ammo.
3. UseCrossbow/UseMP5/UseShotgun read the enemy unchecked; AimAtEntity on a tripmine with no
   enemy goes ActionOpenFire -> ActionChooseWeapon -> DispatchWeaponUse and can dereference NULL.
4. UseSatchel/UseTripMine call ActionChooseWeapon, which keeps the satchel (its desire is 0) when
   nothing else scores and dispatches to UseSatchel again: unbounded recursion.
5. SetNextShootTime adds `m_flMSec` (milliseconds, 1-100) to a time in seconds, so a tap-fire
   window lasts up to 100 s.
6. ThinkStart's MarkLocationFavorable runs only when the steer precedence is non-zero, right
   after setting it to 0: dead code. Bots only ever mark cells unfavourable.
7. SteerGroupAlign/Cohesion divide by the neighbour count with no check (NaN velocity with no
   neighbours); SteerGroupSeparation seeks the summed away-vector as a point (near the world
   origin); evade/pursue lead by velocity times distance.
8. CheckPathObstructedSides adds/subtracts 45 degrees to the yaw of an angles vector and then
   normalizes it as a direction.
9. HandleObstacles clamps an over-large turn to the opposite side (yaw + max when the turn went
   negative, yaw - max when positive). HandleObstacles also reuses the front trace for the left
   side trace.
10. ThinkMood dereferences `m_pActiveItem` unchecked.
11. ActionLook compares the whole `pev->flags` with FL_SPECTATOR instead of testing the bit.
12. FLoadBotNav's path has a trailing space ("%s/nav/%s.nav "), so it never opens; nothing calls
    it anyway. SaveNavToFile doesn't check fopen: no <runfolder>/nav directory crashes the map
    change.
13. parse_nav_line uses 4/4/4/8-byte buffers and doesn't range-check the indices.
14. ReadBotFile and BotClientPutInServer strcpy colours into char[4]; a longer cvar overruns.
15. UseBotCycle doesn't trim the line: CRLF botcycle.txt gives "name\r" (see section 7).
16. BotConnect's name filter skips the character after each one it removes; its
    "no free client slots" branch tests a pointer already known non-NULL and never runs.
17. HandleAutoAdjDifficulty adds 12 s to the last time, not to now: after a long gap it runs
    every frame until it catches up.
18. CBaseBot::TakeDamage is never called (its extra argument keeps it from overriding).
