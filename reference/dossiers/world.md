# Dossier: world (map entities and monsters)

## 1. Scope

The original's source files, all stock SDK files: `effects.cpp`, `nodes.cpp`, `plats.cpp`,
`monsters.cpp`, `triggers.cpp`, `sound.cpp`, `func_tank.cpp`, `controller.cpp`, `buttons.cpp`,
`maprules.cpp`, `func_break.cpp`, `talkmonster.cpp`, `schedule.cpp`, `tentacle.cpp`, `xen.cpp`,
`squadmonster.cpp`, `h_cycler.cpp`, `flyingmonster.cpp`, `soundent.cpp`, `playermonster.cpp`.

Touched here (worktree `~/Projects/goldsrc.net/mod-ozdm-wt/world`, branch `wt/world`, uncommitted):
`dlls/basemonster.h`, `dlls/buttons.cpp`, `dlls/effects.cpp`, `dlls/flyingmonster.cpp`,
`dlls/func_break.cpp`, `dlls/func_tank.cpp`, `dlls/monsters.cpp`, `dlls/nodes.cpp`, `dlls/nodes.h`,
`dlls/plats.cpp`, `dlls/playermonster.cpp`, `dlls/sound.cpp`, `dlls/squadmonster.cpp`,
`dlls/talkmonster.cpp`, `dlls/tentacle.cpp`, `dlls/triggers.cpp`. The other files needed no change.

OZ changed almost nothing here: the 1.7 source has no `// Oz` marker in any of these files, and the
only OZ code found in the 3.02 binary is the two `CGraph::InitNavArray` calls (bots) and the
`monster_player` entity (the SDK's `playermonster.cpp`, unbuilt in halflife-updated). The rest is
halflife-updated's changes, each given a verdict below.

## 2. Status

`scripts/check_file.py <file> --missing` after `scripts/build.sh i386`:

```
file               present  missing  the missing ones
effects.cpp        181/181  0
nodes.cpp           57/57   0
plats.cpp          109/111  2        Fix, FixupAngles: static in plats.cpp, inlined by today's gcc
monsters.cpp       116/119  3        CBaseMonster::PlaySentence, PlayScriptedSentence, SentenceStop:
                                     halflife-updated's HL25 sentence rework (3d8b933) moved them to
                                     CBaseToggle (subs.cpp, not this package); see section 5
triggers.cpp       133/133  0
sound.cpp           43/44   1        memfgets: static in sound.cpp, inlined
func_tank.cpp       49/49   0
controller.cpp      46/46   0
buttons.cpp         65/65   0
maprules.cpp        75/75   0
func_break.cpp      33/33   0
talkmonster.cpp     43/44   1        CTalkMonster::PlaySentence: the same rework made it
                                     CTalkMonster::PlaySentenceCore (here), called from
                                     CBaseToggle::PlaySentence
schedule.cpp        13/13   0
tentacle.cpp        27/27   0
xen.cpp             46/46   0
squadmonster.cpp    26/26   0
h_cycler.cpp        36/36   0
flyingmonster.cpp   16/16   0
soundent.cpp        19/19   0
playermonster.cpp    7/7    0        monster_player exported (nm -D)
```

Both `scripts/build.sh i386` and `scripts/build.sh amd64` build (no new warnings in these files).
Nothing has been run in an engine: built only.

## 3. Function map

About 1,400 original functions are in scope (`reference/funcfile.tsv`, filtered on the files above).
Every function not named below is `ok`: its stock body matches the original's, and where it differs
the difference is a halflife-updated change listed in section 3.2 as kept.

How each was checked:
- Compared in full against the decompile: every `reference/oz_touch.tsv` row for these files
  (`CController::Move`, `StartTask`; `CLightning::KeyValue`; `CBreakable::Die`;
  `CBaseMonster::FCheckAITrigger`, `HandleAnimEvent`, `MonsterInit`, `MonsterInitDead`, `Move`,
  `MovementComplete`, `RunTask`, `StartTask`; `CGraph::AllocNodes`, `InitGraph`; `CGunTarget::Spawn`,
  `Use`; `EMIT_GROUPNAME_SUIT`; `CTalkMonster::GetScheduleOfType`, `RunTask`, `StartTask`), every
  funcfile "oz" row, all of `playermonster.cpp`, the save tables of every class (script below), and
  the functions any halflife-updated commit to these files touched (`git log upstream/64bit-demo` on
  the 19 files, 108 commits, each read).
- Skimmed: everything else, by `scripts/skim.py` (strings, float constants and called names in the
  decompile missing from our body, and the reverse with `REV=1`) and a read of each flagged lead.
  Every lead was noise (macros the decompile shows expanded, `/2` as `*0.5`, `_DEBUG` strings) or one
  of the changes below.

The `oz_touch.tsv` rows were false leads: `max_health` matched OZ's cvar `max_health` (the bodies use
`pev->max_health`, stock), `HasConditions`/`SetConditions`/`InitNavArray`/`memfgets`/`Radius`/
`ExplosionMagnitude` are inline or static helpers. Only `InitNavArray` is OZ's.

### 3.1 The funcfile "oz" rows

All but nine are gcc2's out-of-line copies of inline functions our headers or the files' own class
bodies already have (egcs emitted one wherever an inline wasn't inlined). Spot-checked against the
decompile (`CLightning::ServerSide`, `CTalkMonster::IsFollowing`, `CGameCounter::HitLimit`,
`CSquadMonster::IsLeader`, `CMultiManager::ShouldClone`, `CFuncTank::Use`'s `TankActivate`): same.

```
file               oz rows  real additions
effects.cpp        50       none: CBeam (25), CSprite (8), CShake (8), CFade (4), CBlood (4),
                            CLightning::ServerSide (1), all inline in effects.h / effects.cpp
maprules.cpp       28       none: CGameCounter, CGameText, CGameScore... accessors, inline in the classes
monsters.cpp       18       BotThink (OZ's base virtual, section 5 of the coordinator, already in);
                            PlaySentence, PlayScriptedSentence, SentenceStop (stock, moved, section 2);
                            the rest inline in basemonster.h (HasConditions, Remember, TaskComplete...)
func_tank.cpp       5       none: inline in CFuncTank
plats.cpp           6       none: CFuncTrackChange/CFuncTrackTrain inlines; Fix, FixupAngles static
playermonster.cpp   7       the whole file: monster_player (stock SDK code OZ built, our CMake didn't)
talkmonster.cpp     3       PlaySentence (stock, moved); IsFollowing, StopTalking inline
squadmonster.cpp    4       none: inline in squadmonster.h
triggers.cpp        4       CLadder::KeyValue, CTriggerPush::KeyValue: SDK 2.3's pass-through
                            overrides, which halflife-updated's seed dropped; restored here.
                            CMultiManager::IsClone, ShouldClone inline
func_break.cpp      4       none: inline
flyingmonster.cpp   3       none: inline setters
sound.cpp           1       memfgets: static, stock
h_cycler, xen (2), soundent, buttons, tentacle: 1 each, all inline
```

### 3.2 Functions changed here

```
orig      file               function                         change                                               state
0x0fcc00  playermonster.cpp  monster_player, CPlayerMonster   modern SDK style; bodies already the original's      ok
          (all 7)                                             (CLASS_PLAYER_ALLY, yaw 90, health 8, notsolid 4)
0x0de0c0  nodes.cpp          CGraph::InitGraph                Oz: InitNavArray() first                             ok
0x0de1b0  nodes.cpp          CGraph::AllocNodes               Oz: InitNavArray() first                             ok
0x0e0b60  nodes.cpp          CTestHull::DropDelay             center print "Node Graph out of Date. Rebuilding..." ok (design, as mod-rc2)
                                                              restored (seed had it commented out)
0x0d6740  monsters.cpp       CBaseMonster::KeyValue           "allow_item_dropping" removed (b624a61, a feature)   ok (design)
-         monsters.cpp       CBaseMonster::DropItem           its m_AllowItemDropping test removed                 ok (design)
-         monsters.cpp       CBaseMonster::m_SaveData         original's order (m_afConditions first); the         ok
                                                              m_AllowItemDropping field removed
0x0d31f0  monsters.cpp       CBaseMonster::SetActivity        oldActivity = m_Activity (Steven's decision)         ok
0x0d5870  monsters.cpp       CBaseMonster::ChangeYaw          kept halflife-updated's time base, commented          ok (fix)
0x0d4fa0  monsters.cpp       CBaseMonster::IRelationship      HUMANPASSIVE row: halflife-updated's HT/DL for AMIL  ok (fix, Steven)
                                                              and APRED kept (75bf517), commented
0x119910  triggers.cpp       PlayCDTrack                      INDEXENT(1) and "cd pause" (Steven's decision)       ok
0x119c80  triggers.cpp       CTargetCDAudio::Think            INDEXENT(1) (design, as FIXES.md)                    ok
0x11aed0  triggers.cpp       CChangeLevel::Spawn              the "points to the current map" error removed        ok (design)
0x11bc70  triggers.cpp       CLadder::KeyValue                restored (pass-through)                              ok
0x11bda0  triggers.cpp       CTriggerPush::KeyValue           restored (pass-through)                              ok
0x0b8850  func_break.cpp     CPushable::Move                  the original's (HL25 rework 4f0d502 is design)       ok
0x0b7710  func_break.cpp     CBreakable::Die                  gibs along +g_vecAttackDir (Steven's decision);      ok
                                                              re-break guard and fabs kept, commented
0x0b5980  flyingmonster.cpp  CFlyingMonster::ChangeYaw        roll step 220 * delta (Steven's decision)            ok
0x10a250  sound.cpp          CEnvSound::KeyValue, m_SaveData  member back to float m_flRoomtype, FIELD_FLOAT, atof  ok
0x10a6b0  sound.cpp          CEnvSound::Think                 writes (int)m_flRoomtype to the player; the rest is  ok (fix)
                                                              halflife-updated's (bc3add9), commented
-         sound.cpp          CAmbientGeneric::m_SaveData      original's order (m_dpv before m_fActive)            ok
-         plats.cpp          CFuncTrain::m_SaveData           original's order                                     ok
-         func_tank.cpp      CFuncTank::m_SaveData            original's order                                     ok
-         buttons.cpp        CBaseButton::m_SaveData          original's order                                     ok
-         tentacle.cpp       CTentacle::m_SaveData            original's order                                     ok
-         squadmonster.cpp   CSquadMonster::m_SaveData        original's order                                     ok
```

Comments only (halflife-updated's fix kept, now marked): `CBaseButton::ButtonSpark`,
`ButtonBackHome`, `CMomentaryRotButton::Use`'s predicted value, `CFuncTrackTrain::UpdateSound`,
`CTriggerCamera::FollowTarget`, `CTriggerCamera::Use`, `CChangeLevel::ChangeLevelNow`,
`CBaseTrigger::HurtTouch`, `CBaseMonster::MonsterUse`, `ReportAIState`, `CFuncTank::StartControl`,
`StopControl`, `CTentacle::Spawn`, `CTalkMonster::TakeDamage`, `RunTask`/`StartTask`/
`GetScheduleOfType` (local player), `CBlood::BloodPosition`, `CMessage::Use`, `CBreakable::KeyValue`.

### 3.3 halflife-updated's changes in these files and their verdicts

Rule and earlier calls: `~/Projects/goldsrc.net/mod-rc2/reference/audit/FIXES.md`. Same function,
same verdict; new calls marked "new".

```
commit    file(s)                  what                                                     verdict
de70fad   nodes.cpp                DropDelay center print commented out (seed)               design (FIXES), restored
de70fad   triggers.cpp             CLadder/CTriggerPush KeyValue pass-throughs dropped       new: restored (no behavior)
b624a61   monsters.cpp             allow_item_dropping keyvalue + save field                 new: design (feature), removed
75bf517   monsters.cpp             HUMANPASSIVE vs AMIL/APRED: FR -> HT/DL                   new: fix (Steven), kept; R_FR is
                                                                                            never picked as an enemy
ec1f016   monsters.cpp             SetActivity order                                         decided (oldActivity = m_Activity)
343bbb2,  monsters.cpp,            yaw/roll from the time since the last turn                fix (FIXES); roll as decided
95fdebe   flyingmonster.cpp
08975b8   monsters.cpp             MonsterUse no longer sets alert (resurrection)            fix (as mod-rockcrowbar)
3dc5750   monsters.cpp             ReportAIState names                                       fix (FIXES)
3d8b933,  monsters, talkmonster,   sentence virtuals on CBaseToggle; IsAllowedToSpeak        fix: the original's PlaySentence
85467c5   subs.cpp, basemonster.h  instead of IsAlive (no talking while dying)                    tests IsAlive, the talk
                                                                                            monster's tests nothing
f2c8e23   triggers.cpp             PlayCDTrack, CTargetCDAudio::Think: UTIL_GetLocalPlayer   design (FIXES), INDEXENT(1) back
f2c8e23   effects, triggers,       UTIL_GetLocalPlayer elsewhere (BloodPosition, CMessage,   fix (FIXES for BloodPosition; new
          talkmonster              changelevel, camera, three talk tasks)                    for the rest, same reason: a free
                                                                                            slot 1's stale edict, or a crash)
de70fad   triggers.cpp             PlayCDTrack "cd stop"                                     decided: "cd pause"
b9adea9   triggers.cpp             changelevel-to-self error                                 design (FIXES), removed
6ae5366   triggers.cpp             trigger_hurt doesn't heal the dead (MP)                   fix (FIXES)
91944d1,  triggers.cpp             trigger_camera avelocity; 66b1f30's delta time was        fix (FIXES); net code is the
66b1f30,                           reverted by 95741e9                                       original's frametime moves
95741e9
4f0d502   func_break.cpp           HL25 pushable rework                                      design (FIXES), original's Move
50e1811   func_break.cpp           "explosion" "1"; gib direction negated                    fix; direction decided: original
473e9ea   func_break.cpp           no second Die                                             fix (FIXES)
e19fe47   func_break, plats        fabs                                                      fix (FIXES)
484911d   func_break.cpp           CPushable::DamageDecal (no glass decal unless breakable)  new: fix (a wrong decal)
fdc159c   sound.cpp                env_sound room type int                                   reverted: the original's float
bc3add9   sound.cpp, player.*      room type sent from UpdateClientData, saved, reset on      new: fix (halflife#1144 #2936)
                                   level change
51a1fc2   sound.cpp                SENTENCEG_Init off by one                                 new: fix
a8824ba   sound.cpp                speaker invalid preset                                    new: fix
1f2f27a,  nodes.cpp                node graphs through the engine filesystem, GAMECONFIG     new: fix (port); same paths
88df0a2,                           only, out-of-date check                                   (maps/graphs/<map>.nod)
609eafd,
04c63d0
870fe1c   nodes.cpp                64-bit builds use .nod64                                  new: kept (64-bit only, the
                                                                                            graph layout differs)
ecd7ef9,  buttons.cpp              button sparks (ltime, absmin, rot button)                 fix (FIXES)
d7dd2a8,
0d101ca,
2ca64e1
-         buttons.cpp              CMomentaryRotButton predicted value (SDK 2.2+)            fix (FIXES)
3c1e84a,  func_tank.cpp            view model, EquipWeapon                                   fix (FIXES)
(seed)
90500cf   func_tank.cpp            acos domain clamp                                         new: fix
1315fd4   func_tank.cpp            func_tank usable again (its own regression)               new: fix
a02eb83   tentacle.cpp             EFLAG_SLERP                                               decided: kept
e306d7a,  talkmonster.cpp          dying friend, +use during script, null m_pCine,           new (except e306d7a): fix
3f68a77,                           facing only when interruptible, RunTask init
3c7d4ed,
d667a24,
55a1e89
271d503   controller.cpp           no facing without an enemy                                new: fix (faced a stale LKP)
70f3b23   effects.cpp              env_spark no transition                                   new: fix
497b213   effects.cpp              beam attachments restored                                 new: fix (save/restore)
5831d2e   effects.cpp              trip beam touch reference                                 new: same function either way
6b649da,  maprules.cpp             game_player_equip null activator, KeyValue, save table    new: fix
4122fcd,
c20feca
fcb7096   maprules.cpp             game_score null deref                                     new: fix
61e18e3   maprules.cpp             game_zone_player no transition                            new: fix
cb51706   h_cycler.cpp             cycler_wreckage time as float                             new: fix
d02b687   h_cycler.cpp             cycler_sprite saved                                       new: fix
ffb0716   plats.cpp                func_plat trigger after the plat is removed               new: fix
c5b1932   (all)                    BOOL members are bool, FIELD_BOOLEAN                      kept: the type must follow the
                                                                                            member (CBubbling, CLightning,
                                                                                            CFuncTrackChange::m_use,
                                                                                            CTriggerCamera, CMomentaryRotButton,
                                                                                            CCycler, CCyclerSprite)
```

Refactors, formatting, warnings and build commits in the same log are not listed.

## 4. Classes

No new classes. `CPlayerMonster` (base `CBaseMonster`, from `hierarchy.tsv`) has no members of its
own and no new virtuals (`vtables.py --new`: none). Saved members: every save table in these files
now matches `reference/savefields.tsv` in names, order, counts and global flags (checked by
`~/stignored/ozdm-world/savecmp.py`), except the bool fields above (FIELD_BOOLEAN for halflife-updated's
bool members where the original has FIELD_INTEGER) and `CGamePlayerEquip`, which the original doesn't
save (halflife-updated's c20feca, kept). `CEnvSound::m_flRoomtype` is a float again (orig +0x68, the
original's name). No unsaved members were added.

## 5. Cross-file needs

- **`dlls/nodes.h`, `CGraph`:** a `// world needs:` block declaring `void InitNavArray();` (bot_nav.cpp,
  orig 0x08d5e0). The bots package declares it in its own block, so this one goes at the merge.
- **TEMP STUB (world)** at the bottom of `dlls/nodes.cpp`: an empty `CGraph::InitNavArray()`; the real
  one belongs in `bot_nav.cpp`.
- **`dlls/basemonster.h`, `CBaseMonster`:** `m_AllowItemDropping` removed (with its keyvalue and save
  field). No other file used it.
- **subs.cpp (not this package):** the original's `CBaseMonster::PlaySentence`/`PlayScriptedSentence`/
  `SentenceStop` (monsters.cpp, orig 0x0d70d0, 0x0d7160, 0x0d71b0) and `CTalkMonster::PlaySentence`
  (orig 0x113420) live as halflife-updated's `CBaseToggle::PlaySentence`/`PlaySentenceCore`. Bodies
  compared: same, except `IsAllowedToSpeak()` (alive and not dying, or scripted) where the original
  tests `IsAlive()` for monsters and nothing for talk monsters (85467c5, fix).
- **`sv_pushable_fixed_tick_fudge`** (game.cpp, `reference/DEVIATIONS.md` lists it as kept): with
  `CPushable::Move` back to the original's, nothing reads it. 4f0d502 also changed `pm_shared.cpp`
  (pushables in player movement); that file isn't in this package and wasn't touched.
- **`CBasePlayer::m_SndRoomtype` / `UpdateClientData`** (player package): env_sound relies on
  halflife-updated's sending of the room type from `UpdateClientData` (bc3add9). If the player package
  reverts that, `CEnvSound::Think` must send `SVC_ROOMTYPE` itself again as the original did.
- No user messages, no OZ cvars, no other stubs.

## 6. VERIFY list

None open. `CBaseMonster::IRelationship`'s HUMANPASSIVE row, listed here first, was decided by
Steven 2026-11-07: halflife-updated's 75bf517 is a fix. The original's R_FR (from `iEnemy.1301` at
0x163ba0) is never picked by `BestVisibleEnemy`, which takes only relationships above R_NO, so
human-passive monsters ignored alien grunts and bullsquids instead of fleeing them.

## 7. Behavior worth testing

- `monster_player` on a map (spawnflag 4 makes it not solid and undamageable); with OZ's
  `mp_allowmonsters 1` default it spawns.
- Node graph build on a map with info_nodes: the "Node Graph out of Date. Rebuilding..." center
  print, and (with the bots package merged) that the bot nav array is reset with the graph.
- func_pushable: push and pull speeds and the use-to-pull feel against the original.
- func_breakable with "explosion" "1" broken by gunfire: gibs fly away from the shooter? (+attack dir,
  the original's).
- env_sound across a level change and reconnect (halflife-updated's room type handling).
- target_cdaudio / trigger_cdaudio with track -1 ("cd pause").

## 8. Original bugs

- `PlayCDTrack` (triggers.cpp, orig 0x119910): the "Track %d out of range" alert passes no argument.
  Kept (halflife-updated has it too).
- `CTalkMonster::PlaySentence` (orig 0x113420): no alive test, so a dying talk monster can start a
  sentence. Changed: halflife-updated's IsAllowedToSpeak (85467c5).
- `CBreakable::Die` (orig 0x0b7710): breaks again when hit twice before the remove (double gibs and
  items), and `abs()` on the float health. Changed: halflife-updated's fixes kept.
- `CEnvSound::Think` (orig 0x10a6b0): sends the room type once on change and never resets it on a
  level change, so a client keeps a stale room type. Changed: halflife-updated's fix kept.
- `CBaseMonster::ChangeYaw` (orig 0x0d5870) and `CFlyingMonster::ChangeYaw` (orig 0x0b5980): turn rate
  follows the server's frame rate. Changed: halflife-updated's time base kept.
- talk monster tasks, `CBlood::BloodPosition`, `CMessage::Use`, `CChangeLevel::ChangeLevelNow`: read
  edict 1 even when the slot is free (stale origin, or a null `Instance` in `CTriggerCamera::Use`).
  Changed: halflife-updated's fix kept.
