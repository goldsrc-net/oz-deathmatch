# items: runes, grapple, pickups, damage

## 1. Scope

Original source files: `grapple.cpp` (OZ's own), `items.cpp` (stock plus OZ's runes),
`healthkit.cpp`, `h_battery.cpp`, `airtank.cpp`, `combat.cpp` (stock, changed by OZ). items.cpp's
inline item classes (`CItemSuit`, `CItemBattery`, `CItemAntidote`, `CItemSecurity`,
`CItemLongJump`) are filed as "(header)" in `funcfile.tsv` (addresses 0x13da50-0x13e210) and were
compared too.

Files written or touched in `wt/items`:

| File | What |
|---|---|
| `dlls/grapple.cpp`, `dlls/grapple.h` (new) | `CGrappleBolt`, `grapple_bolt`; the header is for client.cpp/player.cpp |
| `dlls/items.cpp`, `dlls/items.h` | `CRune`, `oz_rune`, the five rune free functions, `CItem::RuneHud`, the `RUNE_*` enum, OZ's suit and battery |
| `dlls/healthkit.cpp`, `dlls/h_battery.cpp`, `dlls/airtank.cpp` | OZ's cvars |
| `dlls/combat.cpp` | hitgroup cvars, FireBullets back to SDK 2.3's, OZ's FireBulletsPlayer |
| `dlls/player.h` | `// items needs:` block (section 5) |
| `dlls/weapons.h` | the `Bullet` enum gains OZ's three types (section 5) |

## 2. Status

```
file            check_file.py --missing    note
grapple.cpp     11 of 11, 0 missing
items.cpp       27 of 27, 0 missing        + 15 "(header)" item methods, compared
healthkit.cpp   14 of 14, 0 missing
h_battery.cpp   10 of 10, 0 missing
airtank.cpp      9 of 9, 0 missing
combat.cpp      38 of 38, 0 missing
```

`scripts/build.sh i386` and `amd64` both build with no warnings from these files. Factories
exported (`nm -D build-i386/dlls/hl.so`): grapple_bolt, oz_rune, world_items, item_suit,
item_healthkit, func_healthcharger, func_recharge, item_airtank. Nothing has run in an engine.

Compared in full against the decompile and, where Ghidra fragmented a function, the disassembly
(`scripts/annot.py`): every function in all six files. Read from the disassembly: all of
grapple.cpp, every OZ function of items.cpp, CRune, FireBullets and FireBulletsPlayer (Ghidra lost
both switch tails).

## 3. Function map

grapple.cpp (all OZ):

```
orig      function                         status
0xc3bc0   grapple_bolt (factory)           ok
0xc3be0   CGrappleBolt::BoltCreate         ok
0xc3c30   CGrappleBolt::Init               ok   air speed out of the water (1.7 used the water speed twice)
0xc3e50   CGrappleBolt::Spawn              ok   no CTF colors in 3.02
0xc4180   CGrappleBolt::Killed             ok
0xc41b0   CGrappleBolt::Precache           ok   + common/null.wav
0xc4250   CGrappleBolt::Classify           ok
0xc4260   CGrappleBolt::BoltTouch          ok   sky test (oz_skygrapple), null.wav when oz_grapplesound 0, ducking offset
0xc47f0   CGrappleBolt::BubbleThink        ok
0xc48d0   CGrappleBolt::PlayerStoppedThink ok
0xc49c0   CGrappleBolt::DragPlayer         ok   >150 drag speed, 50-150 oz_playerspeed, else hang
```

items.cpp:

```
0xcb200   GetRuneName              oz    ok
0xcb370   GetRuneDescription       oz    ok   case 1 formats (int)frag_rune_value
0xcb510   GetRuneColor             oz    ok   table decoded from the jump table, both schemes
0xcb940   CanHaveRune              oz    ok   case 10 falls into 11 (section 8)
0xcbad0   SpawnRunes               oz    ok   third int unused (VERIFY name)
0xcbc20   world_items              stock ok
0xcbc40   CWorldItem::KeyValue     stock ok
0xcbe00   CWorldItem::Spawn        stock ok
0xcbf50   CItem::Spawn             stock ok
0xcc050   CItem::ItemTouch         stock ok
0xcc140   CItem::Respawn           stock ok
0xcc1d0   CItem::Materialize       stock ok
0xcc250.. item_suit/battery/antidote/security/longjump factories  ok
0xcc2f0   oz_rune                  oz    ok
0xcc310   CRune::Spawn             oz    ok
0xcc560   CRune::Precache          oz    ok
0xcc670   CRune::Touch             oz    ok
0xcc8b0   CRune::RuneThink         oz    ok
0xcc9e0   CRune::Move              oz    ok
0xccae0   CRune::SetColor          oz    ok
0xccb30   CRune::ActivateRune      oz    ok
0xccd90   CItem::RuneHud           oz    ok   takes const char* (original char*)
0xccf00   CItem::MyTouch           stock ok   inline in items.h
0x13e210  CItemSuit::Spawn         OZ    ok   rune_model picks the model, transTexture 128
0x13e140  CItemSuit::Precache      OZ    ok   gunpickup2 + all six rune models + custom
0x13e0c0  CItemSuit::MyTouch       stock ok
0x13de20  CItemBattery::MyTouch    OZ    ok   battery, max_charge
0x13dfe0/0x13e020 CItemBattery::Precache/Spawn  stock ok
0x13dd10/0x13dd50/0x13dd80 CItemAntidote::*     stock ok
0x13dc20/0x13dc40/0x13dc70 CItemSecurity::*     stock ok
0x13da50/0x13db50/0x13db80 CItemLongJump::*     stock ok
```

healthkit.cpp: CHealthKit::Spawn 0xc8360, Precache 0xc83b0 ok; MyTouch 0xc83f0 ok (healthkit);
CWallHealth Save/Restore 0xc8510/0xc8560, KeyValue 0xc85d0, Precache 0xc88e0, Off 0xc8c20,
ObjectCaps 0xc8cf0 ok (stock); Spawn 0xc8810 and Recharge 0xc8b90 ok (healthcharger_capacity); Use
0xc8930 ok (healthcharger_amount per tick, juice floored at 0).

h_battery.cpp: CRecharge Save/Restore 0xc5630/0xc5680, KeyValue 0xc56f0, Precache 0xc5a00, Off
0xc5dd0, ObjectCaps 0xc5ea0 ok (stock); Spawn 0xc5930 and Recharge 0xc5d60 ok
(suitcharger_capacity); Use 0xc5a50 ok (max_charge cap, suitcharger_amount step, no floor).

airtank.cpp: all stock except TankTouch 0x7b600 (player_air seconds of air). ok.

combat.cpp: every function stock except CBaseMonster::TraceAttack 0x9d940 (head_damage,
chest_damage, stomach_damage, arm_damage, leg_damage), CBaseEntity::FireBullets 0x9daa0 (SDK 2.3's,
section 6 of the brief) and CBaseEntity::FireBulletsPlayer 0x9e3b0 (OZ's). All ok. oz_touch.tsv's
`max_health` hits on BecomeDead and MakeDamageBloodDecal are pev->max_health, not the cvar named
max_health.

## 4. Classes

`CGrappleBolt` (base CBaseEntity; size 0x8c). Virtuals: Spawn, Precache, Classify, Killed. Members
from 1.7's grapple.h, offsets confirmed by the code and the 0x8c allocation: m_iTrail +0x64,
draggerDist +0x68, endOrigin +0x74, dragFinished/dragWasFinished/draggerMoving +0x80..0x82 (unused
in 3.02), dragger +0x84 (written, never read), dragging +0x88 (unused). No save table. 1.7's
ReleaseHook has no 3.02 function and is left out.

`CRune` (base CItem; size 0x78). Virtuals: Spawn, Precache, Touch (CBaseEntity's slot 44), so
CItem::Spawn's DropToFloor and ItemTouch never run for a rune. Members, recovered from code (no save
table; 1.7 had no rune entity, names ours, all VERIFY): m_iRuneType +0x64, m_flMoveTime +0x68,
m_fRandom +0x6c (int in the original), m_flNextColorTime +0x70. +0x74 is allocated and never
touched; no member for it.

`CItem` gains the non-virtual RuneHud. `RUNE_*` (items.h) names the original's bare numbers 0-15
after GetRuneName; ours.

## 5. Cross-file needs

- `dlls/player.h`, `// items needs:` block at the end of CBasePlayer:
  `PlayerReleaseGrapple()` (orig 0xfb3f0, player.cpp), `IsHanging` +0x63c (1.7's name),
  `m_iRuneHudTime` +0x65c, `m_iRune` +0x674, `m_iRandomRune` +0x678 (the last three named here,
  VERIFY). Other packages read the same offsets (crowbar, glock, gauss, hgun, crossbow, gamerules,
  flyingcrowbar all test +0x674); they should take these names.
- `dlls/weapons.h`: the `Bullet` enum gets OZ's BULLET_PLAYER_9MM_SNIPER (2),
  BULLET_PLAYER_9MM_SUPERGLOCK (3), BULLET_PLAYER_MP5_SNIPER (5), BULLET_PLAYER_357_SNIPER (7) in
  1.7's order, shifting MP5 to 4, 357 to 6, BUCKSHOT 8, CROWBAR 9, MONSTER_9MM 10, MONSTER_MP5 11,
  MONSTER_12MM 12. Checked against 3.02's callers (python 6/7, shotgun 8, mp5 5, crowbar decal 9,
  TEXTURETYPE_PlaySound's crowbar test 9). This is an edit outside our classes: FireBulletsPlayer
  can't be right without it, and the weapons packages need the same values. cl_dll uses only the
  names, so it is unaffected. Merge note: DecalGunshot (weapons.cpp) and TEXTURETYPE_PlaySound
  (sound.cpp) switch on these types and belong to other packages.
- TEMP STUB (items) in `dlls/grapple.cpp`: `CBasePlayer::PlayerReleaseGrapple`, empty; the real one
  belongs in player.cpp.
- TEMP STUB (items) in `dlls/items.cpp`: `int NumPlayerStarts`; ServerActivate (client.cpp) counts
  the deathmatch starts into it. The defining file is VERIFY (the .so's symbol table doesn't say).
- `szCommand` (char[256], a global in the original) is defined in items.cpp; only the suit and the
  rune use it.
- Declared for other files: `dlls/grapple.h` (client.cpp's grapple commands call BoltCreate/Init,
  player code calls Killed), items.h's GetRuneName/GetRuneDescription/GetRuneColor/CanHaveRune/
  SpawnRunes (player.cpp's RemoveRune and rune spawning, client.cpp's UpdateRuneInfo).
- Cvars used, all already in game.h: the grapple, rune and color cvars, oz_healthkit, oz_battery,
  oz_maxcharge, oz_health/suitcharger*, oz_playerair, oz_playerspeed, bot_speed, oz_max9mm,
  oz_magazineglock, the five hitgroup cvars and the eight player bullet damages.

halflife-updated calls (FIXES.md rule):

```
function                          halflife-updated change                    verdict
CBaseEntity::FireBullets          tracer flag removed, decals always         design: SDK 2.3's restored (as FIXES.md row 159)
                                  (b0e5d2d, #163)
CBaseEntity::FireBullets          BULLET_PLAYER_BUCKSHOT case (c73c240)      design: removed, as row 159 ("no player bullet cases")
CBaseEntity::FireBulletsPlayer    -                                          replaced whole by OZ's
CAirtank m_state                  int -> bool, FIELD_BOOLEAN (998469a)       kept: a type refactor, as mod-rc2 and mod-rockcrowbar
                                                                             keep it; savefields.tsv says FIELD_INTEGER. Flagged.
HasSuit via m_WeaponBits          64 weapons (c633af8)                       kept: same test, pev->weapons is mirrored
```

No other halflife-updated change in these files alters behavior (the rest are formatting, bool and
override refactors).

## 6. VERIFY list

- Names of CRune's four members, CBasePlayer's m_iRune/m_iRandomRune/m_iRuneHudTime, and
  SpawnRunes' third parameter: no source has them. Settled by whatever the player and rules
  packages chose, or by the 3.0 Windows build's strings (none found here).
- NumPlayerStarts' home file: settled when the rules package reconstructs ServerActivate.

## 7. Behavior worth testing

- Grapple: fire at a wall (drag at grapple_dragspeed beyond 150 units, slower at player_speed
  inside 150, hang inside 50 with noclip); fire at the sky with enable_skygrapple 0 and 1; fire
  while ducking (the hook sits 18 units up); hit a player (damage_grapple, superhook_rune_damage
  with the hook rune); enable_grapple_sounds 0 plays nothing; grapple_glow 1 lights the hook until
  hanging; random_grapple_color.
- Runes: each type's pickup (HUD text, the log line in teamplay and not), maxspeed per rune (haste
  slows to 0.8x, low gravity 1.25x and gravity lowgravity_rune_value, super speed sv_maxspeed),
  crowbar/glock given, the random rune changing color every 5 s and the held rune resolving to the
  type at pickup, the rune jumping to a new start every respawntime_rune seconds, rune_scheme 1
  colors, rune_model 1-7 for runes and for item_suit.
- Pickups: healthkit, battery up to max_charge, chargers with *_amount > 1, airtank with player_air.
- Damage: hitgroup multipliers on players and monsters; glock/mp5/357 sniper and super glock damage,
  the super glock's explosion sprite, server tracers and decals (mp5 decals only on non-tracer
  shots).

## 8. Original bugs

All kept, except the two crashes the coordinator guarded at the merge (`reference/DEVIATIONS.md`,
Crashes in the original): the runes without deathmatch starts, and BubbleThink's stray entities.

- `CanHaveRune` case 10 has no break: with the booby trap rune off, it is still allowed whenever
  the super glock rune is on (items.cpp).
- `CRune::Touch` sets the rune's own pev->maxspeed (oz_playerspeed or bot_speed), not the player's;
  harmless since ActivateRune sets the player's right after.
- `CRune::Touch` plays items/gunpickup2.wav twice (itself and again in RuneHud).
- The non-teamplay rune pickup log line prints the user id in the team field
  (`"%s<%i><%s><%i>" Picked up the "%s".`); the teamplay line has a double space before the quote.
- `SpawnRunes` and `CRune::Move` dereference the chosen info_player_deathmatch without a check: a
  map with none (NumPlayerStarts 0, RANDOM_LONG(1, 0)) crashes.
- `CRune::Precache` tests rune_scheme where rune_model was meant; harmless because W_Precache
  precaches item_suit, whose Precache takes every rune model.
- `CGrappleBolt::BubbleThink` still looks up the owner (1.7's CTF colors used it) and never uses
  it; with no owner, GetClassPtr(NULL) creates a stray entity every think.
- `CGrappleBolt::BoltTouch` moves the hook 18 units up instead of back along its flight when the
  owner is ducking (probably meant to offset the owner, not the hook).
- `CRecharge::Use` subtracts suitcharger_amount from the juice with no floor, so a capacity that
  isn't a multiple of the amount goes negative and `Off`'s `m_iJuice == 0` test never recharges it
  (CWallHealth floors at 0 and doesn't have this).
