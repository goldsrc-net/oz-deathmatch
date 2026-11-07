# rules

Client, game rules, world, shared helpers and player movement.

## 1. Scope

Original files: client.cpp, multiplay_gamerules.cpp, teamplay_gamerules.cpp, gamerules.cpp, world.cpp,
util.cpp, cbase.cpp, game.cpp, voice_gamemgr.cpp, pm_shared.c (our pm_shared/pm_shared.cpp). LinkUserMessages
(the original's player.cpp) is here too, because the user messages belong to this package.

Edited: those files (cbase.cpp and game.cpp need no change), plus `gamerules.h`, `teamplay_gamerules.h`,
`UserMessages.h/.cpp`, `util.h` (UTIL_GetNextBestWeapon), `player.h` and `nodes.h` ("rules needs:" blocks),
and `items.cpp` (its NumPlayerStarts TEMP STUB removed; client.cpp defines it now, as the coordinator asked).

The worktree was fast-forwarded to main (4a5a5ee, the items merge) without a new commit, so client.cpp
can use `grapple.h`, `items.h` (RUNE_*) and the items package's CBasePlayer members.

There is no CTF in 3.02. OZ 1.7's ctf_gamerules.cpp is gone, and neither 3.02 nor 3.0 has a CTF string,
entity or cvar. Teamplay is SDK 2.3's with OZ's scoring.

## 2. Status

Built, not run in an engine. i386 and amd64 both build.

check_file.py --missing:

```
client.cpp               44/44
multiplay_gamerules.cpp  59/59
teamplay_gamerules.cpp   21/21
gamerules.cpp            22/22
game.cpp                 1/1
pm_shared.c              53/53
world.cpp                25/26   InitBodyQue: static in both (t in the original), inlined here
util.cpp                 126/128 FixedSigned16, FixedUnsigned16: static in both, inlined here
cbase.cpp                84/90   four EHANDLE operators (gcc2 names the script can't match; the
                                 operators are there, and halflife-updated folds operator int into
                                 operator CBaseEntity*), operator new (inline in cbase.h), eoffset
                                 (halflife-updated removed it, f2c8e23; nothing in the original calls it)
voice_gamemgr.cpp        8/9     FindPlayerByName: static in both, unused, so no symbol
```

How each file was checked:
- Compared every function: client.cpp, the three gamerules files, game.cpp (GameDLLInit only; the base
  commit already did it), voice_gamemgr.cpp, pm_shared.
- Read the disassembly where Ghidra lost code: ClientCommand, PlayerPostThink, StartFrame, Host_Say's
  log branch, ClientKill, FShouldSwitchWeapon, ChangeLevel, InstallGameRules, CHalfLifeTeamplay::InitHUD
  and IPointsForKill, LinkUserMessages, and pm_shared's FlyMove, LadderMove, PreventMegaBunnyJumping,
  PlayerMove and CreateStuckTable.
- world.cpp: CWorld::Precache, Spawn and KeyValue compared; the rest read and stock.
- util.cpp: about 20 functions compared; the rest skimmed by strings and float constants, all already in ours.
- cbase.cpp: the dispatch functions, TakeHealth, TakeDamage, Killed, IsInWorld and Create compared;
  the rest skimmed. It is stock in 3.02. The oz_touch lead on TakeHealth was a false positive (max_health
  is a stock field).

## 3. Function map

OZ additions:

| orig | function | note |
|---|---|---|
| 0x975f0 | CBasePlayer::UpdateRuneInfo (client.cpp) | ok; replaces base's TEMP STUB |
| 0x97eb0 | CBasePlayerByIndex (client.cpp) | ok |
| 0xdadd0 | CHalfLifeMultiplay::GetNextBestWeapon | ok, SDK 2.x body; alwaysSearch is ignored |
| 0x123a40 | UTIL_GetNextBestWeapon (util.cpp) | ok, wraps g_pGameRules->GetNextBestWeapon; nothing calls it |
| 0xf0350 | LinkUserMessages (UserMessages.cpp) | SelAmmo restored; see 6 |

Stock functions changed to the original:

- **client.cpp**
  - ClientDisconnect:
    - frees a bot's private data first (FL_FAKECLIENT), so GetClassPtr builds a blank player (see 8);
    - releases the grappling hook and calls RemoveRune;
    - clears FL_CLIENT and FL_FAKECLIENT.
  - respawn: no corpse copy in noclip (the observer).
  - ClientKill: does nothing at 0 health or with FL_SPECTATOR; sets wassuicide with RUNE_TRAP.
  - ClientPutInServer: no iuser1/iuser2 reset.
  - Host_Say: SDK 2.3's. logchat 0 logs the line; anything else prints it to the server console only.
  - ClientCommand: OZ's whole command set, read from the disassembly:
    - player and info: bot_fov, enable_wallhack, userune, showtime, showmapname, info, timeleft, help,
      more, cam, droprune, runestatus;
    - grapple: +hook, -hook;
    - bots (listen server only): addbot, removebots, removebot;
    - intN=1 to 4, the author's announcements;
    - bots don't run commands (IsNetClient).
  - ServerActivate: counts info_player_deathmatch into NumPlayerStarts.
  - PlayerPostThink: OZ's join-time HUD lines (version with oz_link, help, oz_line1/oz_line2, MOTD,
    observer hint), the rune HUD refresh, the default-name PlayerMessage, spawn protection.
  - StartFrame:
    - RhoBot: BotThink, nav marking, min_clients/bot_number upkeep, the "bot" cvar's
      addbot/removebot/removebots;
    - then the game rules think.
  - ClientPrecache: adds ambience/particle_suck1.wav.
  - GetGameDescription: "Oz_DM 3.0" before the rules exist.
  - SetupVisibility: sees from the observed player (SDK 2.3).
  - AddToFullPack: no flesh-sound flag.
  - UpdateClientData: iuser1/iuser2 only with oz_advspec 1; no OBS_IN_EYE override.
  - AllowLagCompensation: 0.
- **multiplay**
  - Think and GoToIntermission: mp_chattime clamped to 10-45 s, maximum intermission 45 s.
  - RefreshSkillData: 357 at 40.
  - FShouldSwitchWeapon: auto_switch and RUNE_CROWBAR; no cl_autowepswitch.
  - ClientUserInfoChanged: removed (it is halflife-updated's prefs).
  - InitHUD: clears the cheat probes.
  - ClientDisconnected: SDK 2.3's observer reset restored.
  - PlayerSpawn: first client spawn enters OZ's observer intro; later spawns get the start_* weapons and
    ammo, then active_weapon.
  - IPointsForKill: points_per_kill + frag_rune_value.
  - PlayerKilled: points_per_suicide and points_per_worldkill; no func_vehicle credit.
  - Respawn times: respawntime_weapon, _item and _ammo.
  - ChangeLevel: saves the nav array first.
- **teamplay**
  - IPointsForKill: OZ's teamkill penalty and frag rune.
  - InitHUD: calls TeamWithFewestPlayers, not SetDefaultPlayerTeam (VERIFY).
  - GetGameDescription: "Half-Life oztp".
- **gamerules**
  - plrDmgHornet: 7, not a cvar.
  - InstallGameRules: no game.cfg; mp_teamplay == 1, read by name.
  - CGameRules::GetGameDescription: "Half-Life ozdm".
- **world, util, voice, pm_shared** (forks of this package; detail follows)
  - CWorld::Precache:
    - sets night_ops 0 after deleting the rules;
    - logs "Executing %s config file\n" and queues `exec maps/<map>.cfg\n` before InstallGameRules,
      whose SERVER_EXECUTE runs it;
    - night_ops > 0 sets styles 0-12 and 63 to one letter from light_level (0-4 gives a-e, else f);
    - clears g_runes_exist.
  - UTIL_BloodDrips: `(int)(amount * blood_amount)` in multiplayer.
  - CVoiceGameMgr::UpdateMasks: CVAR_GET_FLOAT("sv_alltalk") and the inner IsPlayer test (SDK 2.3).
  - pm_shared: OZ changed no movement. PM_LadderMove gets back the original's ladder speed (no maxspeed
    cap, no ducked climb speed), and UnDuck, Duck and CheckParamters write view_ofs z only. See 7 for the
    misprediction this brings.

Every function has its `// orig` address in the code.

## 4. Classes

No new classes. CHalfLifeMultiplay keeps halflife-updated's m_flIntermissionStartTime/EndTime members for
the original's g_flIntermissionStartTime and +0x4. CHalfLifeTeamplay's members are SDK 2.3's (+0xc
m_DisableDeathMessages, +0x10 m_DisableDeathPenalty, +0x14 m_teamLimit, +0x18 m_szTeamList).

CBasePlayer members recovered from code (player.h "rules needs:" block; names from 1.7 where it has them,
the rest VERIFY):

```
+0x638 pGrappleBolt      1.7
+0x644 m_iStart          observer hint timer (1.7 name, role guessed)
+0x648 m_iCommands       help hint, then oz_line1/2 (1.7)
+0x64c m_iOz             version line (1.7)
+0x650 m_iMOTDTimer      VERIFY
+0x654 m_iNameCheckTimer VERIFY
+0x658 m_iObserver       VERIFY, set by StartObserver/StopObserver
+0x660 m_iSpawnProtect   VERIFY
+0x668 m_iFirst          1.7
+0x670 wassuicide        1.7
+0x698 m_iBotFovCheck    VERIFY
+0x69c m_iWallhackCheck  VERIFY
```

IsHanging (+0x63c), m_iRuneHudTime (+0x65c) and m_iRune (+0x674) are the items package's (main). +0x308 is
SDK's m_fNextSuicideTime, +0x5e4 m_iDeaths, +0x5fc m_szTeamName, +0x62c m_hObserverTarget, +0x68c/+0x694
m_flAmmoStartCharge/m_flNextAmmoBurn.

## 5. Cross-file needs

Defined here for others: `NumPlayerStarts` (client.cpp; the items TEMP STUB is removed),
CBasePlayer::UpdateRuneInfo, UTIL_GetNextBestWeapon (util.h), gmsgSelAmmo (UserMessages.h).

Members added to shared classes: CBasePlayer (section 4, plus RemoveRune, Teleport, MOTD and PlayerMessage
declarations); CGraph in nodes.h (MarkLocationFavorable, SaveNavToFile).

TEMP DECL/STUB (drop at merge):

| what | where | real one |
|---|---|---|
| class CBaseBotGlobals { HandleAutoAdjDifficulty } and BotGlobals | client.cpp top / bottom | bot_misc.cpp |
| UseBotCycle, BotConnect, bInitBotCycle, TimeNextPossibleBotConnect | client.cpp bottom | bot_misc.cpp (bot globals) |
| CGraph::MarkLocationFavorable, SaveNavToFile | client.cpp bottom | bot_nav.cpp |
| CBasePlayer::RemoveRune, Teleport, MOTD, PlayerMessage | client.cpp bottom | player.cpp |
| bool g_runes_exist | world.cpp bottom | OZ's globals.cpp; whoever owns CRune |

Bot commands call into the bots package only through bot_number/"bot" cvars and the functions above.
`bot_number` and the other cvars are game.cpp's.

## 6. halflife-updated calls

Kept as fixes:
- sv_cheats read on demand, for give/fov and CWorld::Spawn (04f8b91, halflife#2511).
- ClientDisconnect lets go of a func_tank (de2751c, halflife#2594).
- Host_Say's Q_UnicodeValidate replaces the isprint loop (seed).
- The name null guards in ClientDisconnect and InitHUD (seed).
- fullupdate, closemenus and the % filter on "Unknown command" (seed; as mod-rockcrowbar).
- AddToFullPack: the MAX_EDICTS test, and the FL_FLY/m_EFlags SLERP rule (decided per FIXES.md).
- GetHullBounds fills the bounds (c3c5576).
- ServerActivate's loop bounds.
- UpdateClientData: the fov from m_iFOV (6f8aac9 removed pev->fov), and this repo's m_WeaponBits sync.
- The multiplay constructor doesn't exec servercfgfile (Steven 2026-11-07: left to the engine).
- Health message 2 bytes (5a0c6c1, halflife#2149).
- In the forks' files: CWorld's duplicate-worldspawn guard; CopyToBodyQue's modelindex 0 test (ac6b664,
  flagged below); vban's sscanf check (3e9a8f0); pm_shared's FlyMove first-plane reflect, duck-unstick,
  +use maxspeed cut, MP friction reset, frozen jump sound, PM_Init hull bounds, TraceModel clear.

Reverted to the original (design, balance or added feature):
- 357 damage 50 (ebaf86d; FIXES.md).
- sk_plr_hornet_dmg (460802c).
- cl_autowepswitch and prefs (e7bb9d7).
- The chat flood timer, "(SPEC)" chat and voice mute in say.
- spectate, specmode and follownext.
- ClientPutInServer's iuser reset.
- OBS_IN_EYE data in UpdateClientData.
- EFLAG_FLESH_SOUND.
- game.cfg exec.
- The func_vehicle kill credit.
- mp_chattime 1-120.
- `teamplay.value > 0` for mp_teamplay == 1.
- Lag compensation on.
- pm_shared's ladder speed caps (seed; as mod-rc2 p6).

Kept, not decided here: the busters branch and sv_busters (DEVIATIONS); the "bj" gate, whose default 0
equals the original; plats/vehicle_ignition.wav, for this build's func_vehicle.

Flagged for Steven:
- pm_shared's FlyMove reflect, duck-unstick and +use move, and CopyToBodyQue's modelindex test: kept as
  fixes. Steven 2026-11-07: halflife-updated's in all three projects (mod-rc2 and mod-rockcrowbar are
  being brought in line).
- allow_spectators is still registered (DEVIATIONS) but nothing reads it now that spectate is gone.
- StatusText/StatusValue stay registered because player.cpp's status bar sends them. The original
  registers neither; if the player package drops the status bar, they can go.

## 7. VERIFY list (9)

Code (2):
- PlayerPostThink's default-name test reads isdigit(name[1]) for a 9-character name (0x97847). Literal.
- CHalfLifeTeamplay::InitHUD calls TeamWithFewestPlayers and drops the result (0x1147bc). Literal;
  harmless, since Spawn sets the team.

Decision (0): the multiplay constructor's servercfgfile exec is left to the engine (Steven 2026-11-07).

Names in player.h (7): m_iStart (role), m_iMOTDTimer, m_iNameCheckTimer, m_iObserver, m_iSpawnProtect,
m_iBotFovCheck, m_iWallhackCheck. The player package settles the names.

## 8. Behavior worth testing

- Join as a client. Expected:
  - You start in OZ's observer mode (DM: chase-free, no HUD; teamplay: noclip).
  - The HUD lines and MOTD come in order, and mp_picture 1 skips them.
  - Fire leaves observer mode.
  - cl_forwardspeed/backspeed are set to sv_maxspeed.
- Commands: help, more, cam, info/timeleft, runestatus, droprune with logrune 0/1, userune with the
  booby trap and teleport runes, +hook/-hook (disable_grapple 1; low-gravity rune keeps its gravity).
- The bots:
  - addbot/removebot/removebots from a listen-server client, and refused on a dedicated server;
  - `bot "addbot"` and `bot "removebot Name With Spaces"`;
  - min_clients and bot_number filling and kicking.
- logchat 0 (logged, no console echo) and 1 (console echo only).
- Scoring: points_per_*, frag_rune_value, teamkill penalty in teamplay, auto_switch 0, the Super Crowbar
  rune keeping the crowbar.
- respawntime_* cvars; mp_chattime 5 and 90 (clamped to 10 and 45).
- Spawn protection: glow and god mode until 120 frames or the first attack.
- The rune name refresh at the bottom of the screen every 900 frames.
- A map whose maps/<map>.cfg sets night_ops and light_level.
- blood_amount; sv_alltalk.
- Ladder climbing ducked or slow (the misprediction in 3); bunny-hop cap.

## 9. Original bugs

All kept, except the crashes guarded since (`reference/ORIGINAL_BUGS.md`):
- **ClientDisconnect frees a bot's private data, then GetClassPtr builds a blank CBasePlayer** for it.
  The bot's hook and rune cleanup see zeroed members, so a disconnecting bot's rune is not respawned and
  its hook is not killed.
- **CHalfLifeMultiplay::IPointsForKill dereferences pAttacker unchecked.** That is NULL when the killer is
  a player whose Classify isn't CLASS_PLAYER, which no player is, so it can't crash.
- **StartFrame's "bot" cvar handling:**
  - addbot past 31 players returns before the cvar is cleared, so it repeats every second;
  - strcpy into an 80-byte static with no length check.
- **ClientCommand's removebot kicks any entity with that netname, human or bot,** and has the host run the
  kick (CLIENT_COMMAND).
- **CWorld::Precache with night ops skips the gDecals index lookup.** Decals keep the previous map's
  indices, or 0 on the first map.
- **Host_Say's logchat is inverted** from what its name suggests: logchat 1 (the default) means the line
  is never logged.
- **PlayerPostThink calls through pPlayer before its null check.**
