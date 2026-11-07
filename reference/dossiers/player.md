# Dossier: player

## 1. Scope

Original file: `player.cpp` (131 functions, 21 of them OZ's). Written or touched here:

- `dlls/player.cpp`: every function compared with the decompile (and the disassembly where Ghidra
  split or truncated it: TraceAttack, PlayerDeathThink, PostThink, Identify, RemoveRune,
  DetectCheats, PlayerMessage, StartObserver, ImpulseCommands).
- `dlls/player.h`: CBasePlayer's OZ members and methods, status bar members removed.
- `dlls/observer.cpp`: emptied (comment only). The original has no observer.cpp; its observer
  (SDK 2.2's) is in player.cpp. The file stays in CMakeLists.txt.
- Not touched: `UserMessages.cpp` (the original's LinkUserMessages, orig 0xf0350, lives there; see
  section 5), `weapons_shared.cpp` (SelectLastItem, orig 0xf6e00, already the original's body plus
  the m_ForceSendAnimations fix).

## 2. Status

`scripts/check_file.py player.cpp --missing`: 131 of 131 present, 0 missing. Builds on i386 and
amd64 (`scripts/build.sh`). The five entity factories (`player`, `monster_hevsuit_dead`,
`player_weaponstrip`, `player_loadsaved`, `info_intermission`) are exported. Nothing is checked in
an engine yet: built only.

## 3. Function map

All compared (player file: every function). `ok` = matches the original (with the kept
halflife-updated fixes of section 5b); OZ changes are in each function's comment.

```
orig      function                                  state
0xf0350   LinkUserMessages (UserMessages.cpp)        not changed here (rules package; see 5)
0xf07f0   Pain, 0xf0a50 DeathSound, TakeHealth,     ok, stock
          GetGunPosition, VecVelocityForDamage,
          TrainSpeed, TabulateAmmo, IsOnLadder
0xf0bb0   TraceAttack                                OZ: oz_*damage, hit center print. VERIFY (generic hitgroup text)
0xf0db0   TakeDamage                                 OZ: shake, protection, vampire, wassuicide. ok
0xf15e0   PackDeadPlayerItems                        ok (reload out)
0xf1940   RemoveAllItems                             ok (CurWeapon back)
0xf1af0   Killed                                     OZ: flashlight off; pev->fov. ok
0xf1e10   SetAnimation                               ok, stock
0xf23f0   WaterMove                                  OZ: oz_playerair. ok
0xf2920   PlayerDeathThink                           OZ: grapple, booby trap, rune log/drop, observer respawn. ok
0xf2fe0   StartDeathCam                              OZ form (no intermission spots, "Start Deathcam"). ok
0xf3120   StartObserver                              OZ/SDK 2.2 spectator. VERIFY (two no-op flag clears)
0xf3500   StopObserver                               OZ. ok
0xf35a0   Observer_FindNextPlayer                    SDK 2.2. ok
0xf3780   Observer_HandleButtons                     OZ (oz_spectators, attack/use leaves). ok
0xf3890   Observer_SetMode                           SDK 2.2. ok
0xf39c0   PlayerUse                                  ok (observer early-out out)
0xf3da0   Jump, FixPlayerCrouchStuck, Duck,          ok
          Classify, AddPoints, AddPointsToTeam
0xf4190   PreThink                                   OZ: DetectCheats, observer, IsHanging. VERIFY (NULL pTrain)
0xf4600   CheckTimeBasedDamage .. SetSuitUpdate      ok
0xf4c90   CheckPowerups                              ok (made non-static so the symbol exists)
0xf4cd0   UpdatePlayerSound                          ok
0xf4f00   PostThink                                  OZ: fall damage cvar, first-join exit, Identify,
                                                     regen and cloak runes. VERIFY (cloak renderfx OR)
0xf5950   IsSpawnPointValid                          ok
0xf59d0   EntSelectSpawnPoint                        ok (RANDOM_LONG(1,5) back)
0xf5e70   Spawn                                      OZ: oz_* cvars, bot blood/speed, counters, spawn
                                                     protection, rune placement. ok
0xf6880   Precache                                   ok (node graph pointers here, not PreThink)
0xf6950   Save, RenewItems, Restore                  ok
0xf6c20   SelectNextItem, SelectItem, HasWeapons,    ok
          SelectPrevItem, TeamID
0xf6f20   CSprayCan::Spawn/Think/ObjectCaps,         ok, stock
          CBloodSplat::Spawn/Spray
0xf7340   GiveNamedItem                              ok
0xf7430   FindEntityForward                          ok (added: SDK's free function)
0xf7590   Flashlight*, ForceClientDllUpdate          ok
0xf77d0   ImpulseCommands                            ok (impulse 204 back)
0xf7a90   CheatImpulseCommands                       ok (plain "Texture:" line)
0xf84a0   AddPlayerItem                              ok (no WeapPickup here)
0xf8670   RemovePlayerItem                           VERIFY (original's else-if)
0xf8780   GiveAmmo .. SendAmmoUpdate                 ok
0xf8ae0   UpdateClientData                           OZ: oz_nightops. ok
0xf9440   FBecomeProne .. EnableControl              ok
0xf9550   GetAutoaimVector, AutoaimDeflection,       ok (no sv_allow_autoaim)
          ResetAutoaim
0xfa0a0   Set/GetCustomDecalFrames                   ok
0xfa0e0   DropPlayerItem                             ok (no-next-weapon refusal out)
0xfa440   HasPlayerItem, HasNamedPlayerItem,         ok
          SwitchWeapon
0xfa5c0   CDeadHEV, CStripWeapons, CRevertSaved      ok (CRevertSaved's eight accessors are inline: "OZ added"
          (+ accessors), CInfoIntermission           in funcfile only because egcs emitted them)
0xfb3f0   PlayerReleaseGrapple                       OZ. ok
0xfb470   MOTD                                       OZ. ok
0xfb8c0   Identify                                   OZ. ok
0xfbc80   RemoveRune                                 OZ. ok
0xfc150   ChooseRandomPlayerName                     OZ (85 names, from .data 0x166280). ok
0xfc1f0   PlayerMessage                              OZ. ok
0xfc420   DetectCheats                               OZ. ok (log %i bug kept, see 8)
0xfc630   Teleport                                   OZ. ok
0xfc9c0   IsObserver .. BodyTarget, IsBot            inline in player.h. IsBot VERIFY (returns the masked flag)
```

## 4. Classes

CBasePlayer, base CBaseMonster. sizeof 0x6a0 in the original (GetClassPtr allocation). Saved
members: the save table is SDK 2.3's 38 fields (savefields.tsv); halflife-updated's table is kept
as it is (its extra m_WeaponBits, m_hViewEntity, m_SndRoomtype, m_flStartCharge only matter for
single-player saves). Stock unsaved members placed by code: m_szAnimExtention +0x60c,
m_hObserverTarget +0x62c, m_flNextObserverInput +0x634 (SDK 2.2 puts them after m_szAnimExtention),
m_flStartCharge/AmmoStartCharge/PlayAftershock/NextAmmoBurn +0x688..+0x694.

OZ's members (all unsaved, in player.h with offsets):

```
+0x638 pGrappleBolt      1.7 name                  +0x664 m_iTeleportTime   VERIFY
+0x63c IsHanging         1.7 name                  +0x668 m_iFirst          1.7 name
+0x640 Flare             1.7 name                  +0x66c didhaveexploderune 1.7 name (only cleared)
+0x644 m_iStart          VERIFY (rules' name)      +0x670 wassuicide        1.7 name
+0x648 m_iCommands       1.7 name                  +0x674 m_iRune           VERIFY (items' name)
+0x64c m_iOz             1.7 name                  +0x678 m_iRandomRune     VERIFY (items' name)
+0x650 m_iMOTDTimer      VERIFY (rules' name)      +0x67c regencounter      1.7 name
+0x654 m_iNameCheckTimer VERIFY (rules' name)      +0x680 (4 bytes, never accessed)
+0x658 m_iObserver       VERIFY (rules' name)      +0x684 m_flCloakTime     VERIFY
+0x65c m_iRuneHudTime    VERIFY (items' name)      +0x698 m_iBotFovCheck    VERIFY (rules' name)
+0x660 m_iSpawnProtect   VERIFY (rules' name)      +0x69c m_iWallhackCheck  VERIFY (rules' name)
```

The +0x644..+0x654 counters were named by what client.cpp's PlayerPostThink prints at 25 (strings
read from the binary): +0x64c the "Oz Deathmatch v3.02" title (1.7's m_iOz), +0x648 "Type help in
the console" then oz_line1/oz_line2 (1.7's m_iCommands), +0x644 the "- Observer Mode -" prompt
(1.7's m_iStart showed the "Fire ... to begin" prompt), +0x650 the MOTD, +0x654 the "Player" name
check. The items, rules and weapons-b packages' `needs:` names were adopted; my reading agrees with
all of them.

ctf_team/captures: 3.02 has no CTF at all (no ctf cvar, entity, string or code in hl_i386.so or the
3.0 mp_i386.so), so they went nowhere; 1.7's ctf_has* rune booleans became the single m_iRune.

Globals in player.cpp (orig .data): player_names[85][64] (0x166280), player_name_used[85]
(0x1677c0), `int timer` (0x167914, DetectCheats' shared probe time), PostThink's static donethis
(0x166234).

## 5. Cross-file needs

a. From other files:
- items.cpp: `GetRuneName(int)`, `SpawnRunes(edict_t*, int, int)` (main has them; TEMP STUBs at the
  bottom of player.cpp). Rune numbers are written as literals with the rune's name; items.h's
  RUNE_* enum can replace them at merge.
- `g_runes_exist` (the original's globals.cpp, which this tree lacks; reset by world.cpp in 1.7):
  TEMP STUB definition at the bottom of player.cpp.
- grapple.h (main): CGrappleBolt. Not in this worktree, so PlayerDeathThink and
  PlayerReleaseGrapple call Killed through `reinterpret_cast<CBaseEntity*>` (marked MERGE);
  include grapple.h and call it directly at merge. items' TEMP STUB of PlayerReleaseGrapple in
  grapple.cpp must go.
- client.cpp (rules): calls MOTD, PlayerMessage, Teleport, RemoveRune, StopObserver; zeroes
  m_iBotFovCheck/m_iWallhackCheck; owns PlayerPostThink's counters and spawn protection end.
  CHalfLifeMultiplay::PlayerSpawn uses m_iFirst and the counters.
- Weapons: WeapPickup is sent by the weapons' own AddToPlayer (AddPlayerItem no longer sends it).
  gauss uses m_flStartCharge etc.; the hand grenade uses Flare; weapons read m_iRune.

b. halflife-updated calls made here (the rest of halflife-updated's code in player.cpp is kept):
```
function                 change                                          verdict  note
PackDeadPlayerItems      reload before packing (9d4ce15)                 design   as FIXES
                         empty satchel check (e0ba8ce)                   fix      as FIXES
RemoveAllItems           func_tank release                               fix      as FIXES
                         CurWeapon moved to UpdateClientData (83c18d3)   design   restored here
Killed                   non-solid corpse in MP (df5605d, HL25)          design   removed
                         respawn count reset                             fix      as FIXES
PlayerDeathThink         body copy at animation end (ac6b664, HL25)      design   removed
                         time-based wait (ac6b664)                       kept     as mod-rc2 (frames in original)
                         iuser1 spectator return                         design   removed: OZ's observer is SDK 2.2's
StartObserver, Observer_* halflife-updated's spectator                   design   OZ's own reconstructed
PreThink                 view entity reset, func_vehicle                 kept     as mod-rc2/rockcrowbar
                         vehicle Classify on NULL pTrain                 fixed    NULL check added (crash)
                         node graph pointers here                        moved    original has them in Precache
PostThink                m_afButtonLast on every path                    fix      kept on the early exits only
WaterMove                drown damage taken, swim sound head-under       fix      as FIXES
CheckTimeBasedDamage     fabs (e19fe47)                                  fix      as FIXES
EntSelectSpawnPoint      spot-count random walk (b61c0be, HL25)          design   RANDOM_LONG(1,5) restored
ImpulseCommands          impulse 204 dropped                             design   restored
CheatImpulseCommands     texture type in impulse 107                     design   removed; live sv_cheats kept (as mod-rc2)
AddPlayerItem            insert before ExtractAmmo                       fix      as FIXES
                         WeapPickup for every weapon                     design   removed
GiveAmmo                 exhaustible weapon give                         fix      as FIXES
ItemPostFrame            tank started in PlayerUse                       fix      as FIXES
GetAutoaimVector         ResetAutoaim with sv_aim 0                      fix      as FIXES
Get/AutoaimDeflection    sv_allow_autoaim                                design   removed
DropPlayerItem           refusal with no next weapon                     design   removed
SelectItem, SwitchWeapon m_ForceSendAnimations                           fix      as FIXES
UpdateClientData         status bar, Weapons message, empty CurWeapon    design   removed
                         one-health rounding, restore resend, room type  fix      kept (room type: bc3add9, world)
                         Health as a short (5a0c6c1)                     fix      kept per the rules package
ForceClientDllUpdate     extra resends                                   fix      kept (Steven 2026-11-07)
Spawn                    m_flStartCharge, "bj", room type, chat time     kept
```
Flag for Steven: the original's user messages differ from halflife-updated's (SelAmmo registered,
Health 1 byte, no StatusText/StatusValue/Weapons). The coordinator has the rules package keep the
2-byte Health, so every Health writer here writes a short; Weapons is registered but no longer sent.

c. Shared-file edits: player.h (CBasePlayer only: observer comments, status bar members removed,
OZ methods and members added); observer.cpp emptied. TEMP STUBs at the bottom of player.cpp:
`g_runes_exist`, `GetRuneName`, `SpawnRunes`.

## 6. VERIFY list

- TraceAttack: the original prints an uninitialized pointer for HITGROUP_GENERIC; we print NULL
  (empty center text). Runtime check of what 3.02 shows.
- StartObserver: clears FL_MONSTER and FL_FLY right after setting pev->flags = FL_SPECTATOR (no-ops;
  disassembly confirms the bits).
- PreThink: hanging on the grapple with no ground entity: pTrain is the world (Instance maps NULL to
  it, in SDK 2.3 too), so pTrain->pev is never a NULL read. Settled 2026-11-07.
- PostThink: the cloak rune ORs kRenderFxGlowShell into renderfx (literal).
- RemovePlayerItem: the binary's `else if` for the last item is now `if` (crash guard,
  `reference/DEVIATIONS.md`).
- IsBot returns the masked FL_FAKECLIENT bit (player.h).
- All VERIFY member names above.

## 7. Behavior worth testing

- Hitgroup center prints and oz_*damage multipliers; protection/vampire rune damage.
- Booby trap death: blast, death notice reads "Booby Trap Rune", an empty edict leaks per blast.
- After 6 s dead in DM: respawn into spectator at a spawn point (chase, jump/attack2/attack/use);
  in teamplay: PlayerSpawn's first-join observer.
- First join: hidden until attack/use, then spawns; spawn protection glow.
- Identify with oz_playerid and with the identify rune; teleport rune cooldown.
- Default name "Player": renamed "[PNP]..." and player.txt shown.
- oz_cheatdetect probes; oz_nightops flashlight; oz_fallingdamage 0.

## 8. Original bugs

- TraceAttack: uninitialized message pointer for an unknown/generic hitgroup (guarded: NULL).
- PlayerDeathThink: the booby trap creates a CGrenade entity it never uses (ShootTimed is static):
  one leaked edict per blast. Guarded (the static call alone).
- PlayerDeathThink: the booby trap renames the player's classname to "Booby Trap Rune" until
  respawn (deliberate for the death notice, odd for anything that tests classname "player"). Kept.
- PreThink: the NULL pTrain read while hanging can't happen (see 6).
- RemovePlayerItem: an item both active and last stays m_pLastItem after removal, for lastinv to
  deploy. Guarded.
- PostThink: one static `donethis` for all players, so Identify runs on alternate frames shared
  across players. Kept.
- UpdateClientData: with oz_nightops the flashlight time stays due, so FlashBat is sent every frame
  while the light is on. Kept.
- DetectCheats: one global timer, so one player per 5 s round; the log prints the auth ID string's
  address with %i (cast to int here, prints the same on i386). Kept.
- MOTD: a map MOTD file is loaded twice (both freed). Kept.
- Spawn: 3.02 never sets didhaveexploderune (only clears it). Kept.
