# Dossier: weapons-b

## 1. Scope

Original files: `rpg.cpp`, `gauss.cpp`, `egon.cpp`, `hornetgun.cpp`, `hornet.cpp`, `handgrenade.cpp`,
`satchel.cpp`, `tripmine.cpp`, `squeakgrenade.cpp`, `ggrenade.cpp` (all stock SDK files OZ changed),
plus the inline ammo classes the original compiles from them (`CRpgAmmo`, `CGaussAmmo`, `CEgonAmmo`,
funcfile's "(header)" rows 0x1409f0-0x140b20, 0x13c210-0x13c310, 0x13b960-0x13ba60).

Written here: those ten `dlls/*.cpp`. Shared files touched: `dlls/weapons.h` (only inside CGrenade,
CRpg, CGauss, CEgon, CHgun, CHandGrenade, CSatchel, CTripmine) and `dlls/player.h` (a
`// weapons-b needs:` block in CBasePlayer).

Every function was compared against the decompile (`recovered/decomp/`, extracted per file); where
Ghidra split or lost a function the disassembly was read: `CEgon::Fire` (FUN_000c3391),
`CGrenade::Explode(TraceResult*, int)` (FUN_000d2281), `CRpgRocket::FollowThink`'s homing branch,
`CHornet::Spawn`'s red branch (FUN_000d8fa0), `CTripmineGrenade::MakeBeam`'s switch,
`CTripmine::tripmodeset`, `CSatchel::AddToPlayer`, `CHandGrenade::WeaponIdle`'s throw end, the gauss
overcharge `TakeDamage`. The 1.7 source was used for names and intent. Cvar names are `game.h`'s.

## 2. Status

`scripts/check_file.py <file> --missing`: rpg 28/28, gauss 14/14, egon 21/21, hornetgun 14/14,
hornet 15/15, handgrenade 12/12, satchel 22/22, tripmine 26/26, squeakgrenade 22/22, ggrenade 21/21;
0 missing in every file. Builds on i386 and amd64; all 18 entity factories exported (`weapon_rpg`,
`laser_spot`, `rpg_rocket`, `ammo_rpgclip`, `weapon_gauss`, `ammo_gaussclip`, `weapon_egon`,
`ammo_egonclip`, `weapon_hornetgun`, `hornet`, `weapon_handgrenade`, `monster_satchel`,
`weapon_satchel`, `monster_tripmine`, `weapon_tripmine`, `monster_snark`, `weapon_snark`, `grenade`).
Nothing checked in an engine. The client library (`cl_dll`) compiles these files too and now needs
the OZ cvars; it is not built and OZ ships no client.

## 3. Function map

Every original function in scope, our function in the same file and class. "ok" with a note says what
the body carries from OZ or which halflife-updated (HLU) change was kept or dropped (section 5 has the
calls). Functions without a note match the SDK body already here.

`rpg.cpp`

```
0xfdaf0  weapon_rpg                               ok
0xfdb10  laser_spot                               ok
0xfdb30  CLaserSpot::CreateSpot                   ok
0xfdb80  CLaserSpot::Spawn                        ok
0xfdc10  CLaserSpot::Suspend                      ok
0xfdc60  CLaserSpot::Revive                       ok
0xfdc90  CLaserSpot::Precache                     ok
0xfdcc0  rpg_rocket                               ok
0xfdce0  CRpgRocket::CreateRpgRocket              ok
0xfdd70  CRpgRocket::Spawn                        ok: damage_rpg
0xfdf00  CRpgRocket::RocketTouch                  ok: launcher decrement moved to ~CRpgRocket (HLU fix)
0xfdf60  CRpgRocket::Precache                     ok
0xfdfc0  CRpgRocket::IgniteThink                  ok: color_*_rpg
0xfe1a0  CRpgRocket::FollowThink                  VERIFY: rpg_homing sphere search; unused GetClassPtr(owner) kept; speed_rpg_*; HLU launcher release/sky check dropped
0xfea50  CRpg::Reload                             ok: reload_rpg, haste halves
0xfeb80  CRpg::Spawn                              ok: disable/give_with
0xfec60  CRpg::Precache                           ok
0xfed20  CRpg::GetItemInfo                        ok: max/weight, flags 0
0xfedf0  CRpg::AddToPlayer                        ok: OZ-added, WeapPickup
0xfee80  CRpg::Deploy                             ok
0xfeee0  CRpg::CanHolster                         ok
0xfef10  CRpg::Holster                            ok
0xfefa0  CRpg::PrimaryAttack                      ok: recoil_rpg, empty 0.5 s
0xff2a0  CRpg::SecondaryAttack                    ok
0xff320  CRpg::WeaponIdle                         ok: ResetEmptySound on top; fidget 6.1 (HLU fix)
0xff450  CRpg::UpdateSpot                         ok: HLU reload check kept
0xff550  ammo_rpgclip                             ok
0xff570  CLaserSpot::ObjectCaps                   ok
0xff580  __tf10CLaserSpot                         ok
```

`gauss.cpp`

```
0xc0510  weapon_gauss                             ok
0xc0530  CGauss::GetFullChargeTime                ok
0xc0570  CGauss::Spawn                            ok: disable_gauss == 1
0xc0610  CGauss::Precache                         ok
0xc0750  CGauss::AddToPlayer                      ok: OZ-added
0xc07e0  CGauss::GetItemInfo                      ok
0xc08b0  CGauss::Deploy                           ok
0xc0900  CGauss::Holster                          ok
0xc09b0  CGauss::PrimaryAttack                    ok: water_weapons, cycle_gauss, haste
0xc0b40  CGauss::SecondaryAttack                  ok: gauss_maxchargetime, 90 DMG_BLAST, fade (255,0,20)
0xc1220  CGauss::StartFire                        ok: damage_gauss(_max), recoil, STOP_SOUND pulsemachine
0xc14c0  CGauss::Fire                             VERIFY: underwater host flag; MP radius 1.2; HLU self-hit hitgroup clear (Steven)
0xc1c80  CGauss::WeaponIdle                       ok
0xc1ef0  ammo_gaussclip                           ok
```

`egon.cpp`

```
0xb2b30  weapon_egon                              ok
0xb2b50  CEgon::Spawn                             ok: disable_egon == 1
0xb2bf0  CEgon::Precache                          ok: no events
0xb2cd0  CEgon::Deploy                            ok
0xb2d20  CEgon::AddToPlayer                       ok: OZ-added
0xb2db0  CEgon::Holster                           ok
0xb2e10  CEgon::GetItemInfo                       ok
0xb2ee0  CEgon::GetPulseInterval                  ok
0xb2f00  CEgon::GetDischargeInterval              ok
0xb2f20  CEgon::HasAmmo                           ok
0xb2f50  CEgon::UseAmmo                           ok: no busters
0xb2fb0  CEgon::Attack                            ok: old SDK sounds, water_weapons
0xb3320  CEgon::PrimaryAttack                     ok: narrow
0xb3350  CEgon::SecondaryAttack                   ok: OZ-added, wide
0xb3380  CEgon::Fire                              ok: read from disassembly; damage_egon_*
0xb3990  CEgon::UpdateEffect                      ok
0xb3c80  CEgon::CreateEffect                      ok: no owner/SKIPLOCALHOST
0xb3f20  CEgon::DestroyEffect                     ok
0xb3fd0  CEgon::WeaponIdle                        ok: gpGlobals->time test (bug kept)
0xb40c0  CEgon::EndAttack                         ok: sounds, no event
0xb4180  ammo_egonclip                            ok
```

`hornetgun.cpp`

```
0xca230  weapon_hornetgun                         ok
0xca250  CHgun::IsUseable                         ok
0xca260  CHgun::Spawn                             ok
0xca310  CHgun::Precache                          ok
0xca390  CHgun::AddToPlayer                       VERIFY: ammo set before AddPlayerItem extracts (differs only if max_hornets > 250)
0xca490  CHgun::GetItemInfo                       ok
0xca560  CHgun::Deploy                            ok
0xca5a0  CHgun::Holster                           ok
0xca640  CHgun::PrimaryAttack                     ok: cycle_hivehand, haste, 0.5 s recharge
0xca980  CHgun::SecondaryAttack                   ok: cycle_hivehand2, FIREMODE_FAST, idle from gpGlobals->time (bug kept)
0xcaf90  CHgun::Reload                            ok: max_hornets, haste 0.25
0xcb0a0  CHgun::WeaponIdle                        ok
0xcb190  CHgun::UseDecrement                      ok
0xcb1a0  CHgun::iItemSlot                         ok
0xcb1b0  __tf5CHgun                               ok
```

`hornet.cpp`

```
0xc8da0  hornet                                   ok
0xc8dc0  CHornet::Save                            ok
0xc8e10  CHornet::Restore                         ok
0xc8e60  CHornet::TakeDamage                      ok
0xc8ea0  CHornet::Spawn                           ok: speed_hornet_red/2, damage_hornet
0xc90b0  CHornet::Precache                        ok
0xc91c0  CHornet::IRelationship                   ok
0xc9210  CHornet::Classify                        ok
0xc9250  CHornet::StartTrack                      ok
0xc92c0  CHornet::StartDart                       ok
0xc9330  CHornet::IgniteTrail                     ok: color_*_hornet1/2
0xc9520  CHornet::TrackTarget                     ok
0xc9ec0  CHornet::TrackTouch                      ok
0xca070  CHornet::DartTouch                       ok
0xca0a0  CHornet::DieTouch                        ok: HLU owner guard kept
0xca1e0  __tf7CHornet                             ok
```

`handgrenade.cpp`

```
0xc78e0  weapon_handgrenade                       ok
0xc7900  CHandGrenade::Spawn                      ok
0xc79b0  CHandGrenade::Precache                   ok: w_flare.mdl
0xc7a10  CHandGrenade::GetItemInfo                ok
0xc7ae0  CHandGrenade::Deploy                     ok
0xc7b30  CHandGrenade::CanHolster                 ok
0xc7b60  CHandGrenade::Holster                    ok: HLU disarm kept
0xc7c50  CHandGrenade::PrimaryAttack              ok: Flare = false
0xc7cf0  CHandGrenade::SecondaryAttack            ok: OZ-added flare
0xc7da0  CHandGrenade::WeaponIdle                 ok: speed_hgrenade, 500 cap, grenade/flare_time; HLU release fix kept
0xc82d0  CHandGrenade::UseDecrement               ok
0xc82e0  CHandGrenade::iItemSlot                  ok
0xc82f0  __tf12CHandGrenade                       ok
```

`satchel.cpp`

```
0xff5d0  monster_satchel                          ok
0xff5f0  CSatchelCharge::Deactivate               ok
0xff620  CSatchelCharge::Spawn                    ok: damage_satchel, arm time, shootable
0xff780  CSatchelCharge::SatchelSlide             ok
0xff8e0  CSatchelCharge::SatchelThink             VERIFY: proximity fuse tests the first sphere hit only; dmgtime +5 each think
0xffab0  CSatchelCharge::Precache                 ok
0xffb10  CSatchelCharge::BounceSound              ok
0xffbb0  weapon_satchel                           ok
0xffbd0  CSatchel::AddDuplicate                   ok: no refusal
0xffc10  CSatchel::AddToPlayer                    ok: no m_chargeReady = 0; AddWeapon left to AddPlayerItem
0xffc80  CSatchel::Spawn                          ok
0xffd20  CSatchel::Precache                       ok
0xffda0  CSatchel::GetItemInfo                    ok
0xffe70  CSatchel::IsUseable                      ok
0xffec0  CSatchel::CanDeploy                      ok
0xfff10  CSatchel::Deploy                         ok: HLU idle fix kept
0xfffd0  CSatchel::Holster                        ok
0x1000d0  CSatchel::PrimaryAttack                  ok: SDK 2.3 controls
0x100230  CSatchel::SecondaryAttack                ok
0x100260  CSatchel::Throw                          ok: speed_satchel
0x100420  CSatchel::WeaponIdle                     ok
0x100630  DeactivateSatchels                       ok
0x100710  __tf14CSatchelCharge                     ok
```

`tripmine.cpp`

```
0x11e110  monster_tripmine                         ok
0x11e130  CTripmineGrenade::Save                   ok
0x11e180  CTripmineGrenade::Restore                ok
0x11e1d0  CTripmineGrenade::Spawn                  VERIFY: euser1 = owner (reader unknown); damage_tripmine
0x11e440  CTripmineGrenade::Precache               ok
0x11e4a0  CTripmineGrenade::WarningThink           ok
0x11e4f0  CTripmineGrenade::PowerupThink           ok
0x11e9e0  CTripmineGrenade::KillBeam               ok
0x11ea20  CTripmineGrenade::MakeBeam               VERIFY: tripmode colours; mode 0 uninitialized in the original
0x11ed50  CTripmineGrenade::BeamBreakThink         ok: HLU restore fix kept
0x11ef90  CTripmineGrenade::TakeDamage             ok
0x11f050  CTripmineGrenade::Killed                 ok
0x11f100  CTripmineGrenade::DelayDeathThink        ok
0x11f1f0  weapon_tripmine                          ok
0x11f210  CTripmine::Spawn                         ok
0x11f330  CTripmine::Precache                      ok
0x11f3a0  CTripmine::GetItemInfo                   ok
0x11f470  CTripmine::Deploy                        ok: HLU body fix kept
0x11f4b0  CTripmine::Holster                       ok
0x11f590  CTripmine::PrimaryAttack                 ok: tripmodeset; body in PlaceMine
0x11f8c0  CTripmine::SecondaryAttack               ok: OZ-added; uses 1 mine (bug kept)
0x11fbe0  CTripmine::WeaponIdle                    ok
0x11fd10  CTripmine::tripmodeset                   ok: OZ-added
0x11fde0  __tf16CTripmineGrenade                   ok
0x11fe30  CTripmine::UseDecrement                  ok
0x11fe40  CTripmine::SetObjectCollisionBox         ok
0x11ff10  CTripmine::iItemSlot                     ok
0x11ff20  __tf9CTripmine                           ok
```

`squeakgrenade.cpp`

```
0x10e4d0  monster_snark                            ok
0x10e4f0  CSqueakGrenade::Save                     ok
0x10e540  CSqueakGrenade::Restore                  ok
0x10e590  CSqueakGrenade::Classify                 ok
0x10e630  CSqueakGrenade::Spawn                    ok: snark_health/time, damage_snarkpop
0x10e820  CSqueakGrenade::Precache                 ok
0x10e8d0  CSqueakGrenade::Killed                   ok
0x10ea40  CSqueakGrenade::GibMonster               ok
0x10ea90  CSqueakGrenade::HuntThink                ok: snark_time pitch
0x10f260  CSqueakGrenade::SuperBounceTouch         ok: damage_snark*, no WORLDBRUSH test
0x10f5f0  weapon_snark                             ok
0x10f610  CSqueak::Spawn                           ok
0x10f6e0  CSqueak::Precache                        ok
0x10f780  CSqueak::GetItemInfo                     ok
0x10f850  CSqueak::Deploy                          ok: HLU idle fix kept
0x10f920  CSqueak::Holster                         ok
0x10fa00  CSqueak::PrimaryAttack                   ok: SDK 2.3 throw
0x10fd70  CSqueak::SecondaryAttack                 ok
0x10fd80  CSqueak::WeaponIdle                      ok
0x10ff00  CSqueakGrenade::BloodColor               ok
0x10ff10  __tf14CSqueakGrenade                     ok
0x10ff60  CSqueak::UseDecrement                    ok
0x10ff70  CSqueak::iItemSlot                       ok
0x10ff80  __tf7CSqueak                             ok
```

`ggrenade.cpp`

```
0xc21d0  grenade                                  ok
0xc21f0  CGrenade::Explode                        ok (Vector, Vector)
0xc2270  CGrenade::Explode                        ok: (TraceResult*) read from disassembly; HLU 0.6 pull-out (Steven), HLU -1 z kept
0xc26f0  CGrenade::Smoke                          ok
0xc2890  CGrenade::Killed                         ok
0xc28b0  CGrenade::DetonateUse                    ok
0xc28f0  CGrenade::PreDetonate                    ok
0xc2950  CGrenade::Detonate                       ok
0xc2a20  CGrenade::ExplodeTouch                   ok
0xc2be0  CGrenade::DangerSoundThink               ok
0xc2d20  CGrenade::BounceTouch                    ok: avelocity 300 (OZ); HLU sequence fixes kept
0xc3030  CGrenade::SlideTouch                     ok
0xc30c0  CGrenade::BounceSound                    ok
0xc3160  CGrenade::TumbleThink                    ok
0xc32d0  CGrenade::Spawn                          ok
0xc3390  CGrenade::ShootContact                   ok: "ARgrenade", damage_m203
0xc34b0  CGrenade::ShootTimed                     ok: damage_hgrenade / booby trap
0xc36b0  CGrenade::ShootFlare                     ok: OZ-added
0xc3870  CGrenade::ShootSatchelCharge             ok
0xc39d0  CGrenade::UseSatchelCharges              ok
0xc3b20  CGrenade::BloodColor                     ok
0xc3b30  __tf8CGrenade                            ok
```

## 4. Classes

Bases are `hierarchy.tsv`'s; virtual overrides were checked against `vtables.tsv` per class.

- CRpg (CBasePlayerWeapon): + `AddToPlayer`. Saved m_fSpotActive (0xd0, FIELD_INTEGER in the
  original, our bool/FIELD_BOOLEAN), m_cActiveRockets (0xd4); m_pSpot +0xcc unsaved.
- CRpgRocket (CGrenade): saved m_flIgniteTime (0x2b0), m_pLauncher (0x2b4, FIELD_CLASSPTR) which is
  our `m_hLauncher` EHANDLE (HLU 8092ab5, kept); m_iTrail +0x2ac.
- CLaserSpot (CBaseEntity): unchanged.
- CGauss (CBasePlayerWeapon): + `AddToPlayer`. m_iBalls +0xcc, m_iGlow +0xd0, m_iBeam +0xd4,
  m_iSoundState +0xd8. The charge state is CBasePlayer's (m_flStartCharge +0x688 .. m_flNextAmmoBurn
  +0x694), SDK names.
- CEgon (CBasePlayerWeapon): + `AddToPlayer`, `SecondaryAttack`, static `g_fireAnims1/2` (1.7 names,
  .data 0x15d7cc/0x15d7dc: {5,6,7,8}, {3}); dropped HLU's `ShouldWeaponIdle`, `CanHolster`,
  `m_usEgonFire`, `m_usEgonStop` (the original has none). m_pBeam +0xd0, m_pNoise +0xd4,
  m_pSprite +0xd8, m_deployed +0xec.
- CHgun (CBasePlayerWeapon): m_flRechargeTime +0xd0, m_iFirePhase +0xd4, m_usHornetFire +0xd8. The
  original has no save table (see section 6).
- CHornet (CBaseMonster): unchanged layout; save table matches (3 fields).
- CHandGrenade (CBasePlayerWeapon): + `SecondaryAttack`. m_flStartThrow/m_flReleaseThrow are
  CBaseEntity's (+0x4c/+0x50).
- CSatchelCharge (CGrenade, in satchel.cpp): + `m_flArmTime` (+0x2ac, unsaved; named from cfg's
  "armtime of fuse" comment, VERIFY name).
- CSatchel (CBasePlayerWeapon): HLU's `Detonate()` removed (the original inlines it in
  PrimaryAttack).
- CTripmineGrenade (CGrenade, tripmine.cpp): `MakeBeam(int)` as the original's signature (the int is
  unused; the body reads the global). Save table matches the original's 9 fields except m_pBeam,
  which HLU leaves out on purpose (recreated).
- CTripmine (CBasePlayerWeapon): + `SecondaryAttack`, `tripmodeset(int)`, and `PlaceMine()` (ours:
  the body both attacks repeat inline). Global `int tripmode` (orig .data 0x16d5a0, initial 0).
- CSqueakGrenade (CGrenade): m_iMyClass +0x2d8; save table matches (6 fields).
- CGrenade (CBaseMonster): + static `ShootFlare`. m_fRegisteredSound +0x2a8.

## 5. Cross-file needs and halflife-updated calls

Shared-file edits:
- `player.h`, CBasePlayer, `// weapons-b needs:` block: `int m_iRune` (+0x674; the items package's
  name, per the coordinator; read as `== 4`, haste, in rpg/gauss/hornetgun) and `bool Flare`
  (+0x640, 1.7's name; set by the hand grenade's attacks). Drop `m_iRune` when items merges.
- `weapons.h`: declarations inside my classes only (list in section 4).
- No temp stubs.

Needs from other packages:
- weapons-a (`weapons.cpp`): remove the `CHgun` save table (`m_flRechargeTime`), which the original
  doesn't have, together with CHgun's Save/Restore declarations (left in weapons.h because the
  definitions are in weapons.cpp). The CEgon table's order and the CRpg FIELD_INTEGER/BOOLEAN types
  are weapons-a's as well.
- player (`CBasePlayer::AddPlayerItem`): RPG, gauss, egon and hornetgun now send WeapPickup from
  their AddToPlayer (SDK 2.3's, the FIXES.md "design" row), so AddPlayerItem's own WeapPickup has to
  go or those four announce twice. Satchel, tripmine, snark and hand grenade send none, as in the
  original.
- weapons-a (`CBasePlayerWeapon::AddWeapon`): CSatchel::AddToPlayer's AddWeapon call is not wired,
  since AddPlayerItem extracts the ammo after AddToPlayer (HLU's flow); calling it would extract twice.
- items: `RUNE_*` from `items.h` can replace the literal 4 once merged.

halflife-updated calls (verdict, commit). "design" means the original's code is back.

```
rpg      ~CRpgRocket decrement + EHANDLE launcher           fix     40d3849 (halflife#3264), 8092ab5
rpg      FollowThink launcher release 8192/6 s, sky detonate design  9b9d6c3 (HL25)
rpg      PrimaryAttack/WeaponIdle ResetEmptySound placement design  c31c918, 9b9d6c3 (client prediction; server gets the original)
rpg      WeaponIdle fidget 6.1 s (was 3.0)                  fix     33385ad (halflife#2495)
rpg      UpdateSpot no laser while reloading               fix     9784339
rpg      Reload reads only this reload's result             fix     c82c3fa (#42)
rpg      GetItemInfo ITEM_FLAG_NOAUTOSWITCHTO               design  2353ca4 (HL25)
all      GetNextAttackDelay -> UTIL_WeaponTimeBase() + x    design  seed (as mod-rc2 and mod-rockcrowbar)
4 guns   AddToPlayer WeapPickup                             design  FIXES.md row 0x2ae70..
gauss    V_min(m_flStartCharge, time) sanitize               fix     8c5d4ab (HL25)
gauss    Fire: hitgroup cleared on self-hit                 fix     8c5d4ab (Steven 2026-11-07: unclear goes to HLU)
gauss    SendStopEvent in water/overcharge                  fix     fd35528 (halflife#3188)
gauss    stop event FEV_GLOBAL                              fix     52b6a8a (halflife#3233)
gauss    no ammo burn at 0                                  fix     0e8d3ad (halflife#3343)
egon     events, busters, ShouldWeaponIdle, CanHolster, owner/SKIPLOCALHOST, WeaponIdle button test
                                                            n/a     OZ's own pre-event egon replaces the body (not HLU calls)
hgun     0.3 s MP recharge, empty-delay additions           design  37ed1d9 (HL25)
hgun     iparam1 FIREMODE_FAST -> 0, ammo_hornets test      design  f347954 (Valve's client reads iparam1; server test on m_rgAmmo)
hgun     TabulateAmmo after recharge                        fix     f347954 (halflife#556)
hornet   DieTouch owner guard                               fix     e7fb734 (halflife#1598)
hgrenade Holster disarms (m_flStartThrow = 0)               fix     70dd419 (halflife#3251), as mod-rockcrowbar; mod-rc2 left it out
hgrenade m_flReleaseThrow kept after throw                  fix     4bb1f3c (halflife#2495)
hgrenade 6.5 multiplier, 1000 cap                           design  ae352a6 (HL25)
ggrenade BounceTouch ResetSequenceInfo, frame 0             fix     2ee36b4, b31de2b (FIXES.md)
ggrenade ShootTimed ResetSequenceInfo                       fix     2ee36b4 (FIXES.md)
ggrenade Explode z - 1                                      fix     d6accf1 (halflife#3244), as mod-rockcrowbar
ggrenade Explode pull-out 0.6 (was (dmg - 24) * 0.6)        fix     078e787 (halflife#3244; Steven 2026-11-07: unclear goes to HLU)
satchel  MODERN_SATCHEL_CONTROLS                            design  1c202f5 (HL25)
satchel  live charge limit in AddDuplicate                  design  fd97136 (HL25)
satchel  Deploy idle 2 s after DefaultDeploy                fix     33385ad
tripmine SF_BEAM_TEMPORARY, PointsInit                      fix     7d84906, 819a9ac
tripmine BeamBreakThink owner retrace                       fix     6f0ae55, 79f9e95
tripmine Deploy/WeaponIdle pev->body = 0                    fix     68dc517, c10b998
snark    aim-down throw, FL_WORLDBRUSH bite test            design  e9986c0 (HL25)
snark    movetype assignment                                fix     d6629ed (halflife#3175)
snark    Deploy idle 1.7 s                                  fix     33385ad
```

Mispredictions on Valve's client (server behaviour reconstructed, as in 2004): every cvar-driven
rate, damage, speed and colour; rpg's empty 0.5 s wait and homing; gauss underwater fire (the host
gets the events) and refire; egon primary narrow / secondary wide with server-played sounds and no
events (the shooter also sees the server's beam); hornetgun refire/recharge and haste; flares and
the 500 throw cap; satchel controls and throw speed; tripmine secondary and colours; snark throw.

## 6. VERIFY list

- rpg.cpp FollowThink: `GetClassPtr((CBasePlayer*)VARS(pev->owner))` with its result unused (0xfe214);
  1.7 tested `player->ishoming` there. Settle by a closer read of 0xfe1f0-0xfe240 (it is unused there).
- hornetgun.cpp AddToPlayer: ammo count above 250 max_hornets (needs AddPlayerItem's final flow).
- satchel.cpp `m_flArmTime` name; SatchelThink's dmgtime + 5 each think (reader unknown).
- tripmine.cpp `pev->euser1 = owner` (reader in another package?); MakeBeam with tripmode 0
  (original: uninitialized colour, ours 0,0,0).
- weapons.h CHgun save table (weapons-a).
- player.h `Flare` name.

8 VERIFY markers in the package's code (the eight files above plus weapons.h and player.h).

## 7. Behavior worth testing

- rpg_homing 1: rockets steer at any damageable thing within 1000 units in front, the shooter too.
- Gauss under water with water_weapons 1: shooter sees his own beam; overcharge does 90 and fades
  red; disable_gauss only at exactly 1.
- Egon: primary narrow, secondary wide; stop sound; double beam for the shooter.
- Hand grenade secondary throws a lit flare (flare_time), 5 damage pop.
- Satchel: proximity fuse with allow_proximity_satchel 1 (only the first entity in range counts);
  charges can be shot; picking up more with charges out is allowed.
- Tripmine secondary (black beam) and random_tripmine_color; two mines placed within 2.5 s of each
  other take the later one's colour.
- A dead Booby Trap Rune carrier's grenade: booby trap damage and "Booby Trap Rune" kill name; M203
  kills show "ARgrenade".
- Hornetgun secondary: idle animations stop (idle time from gpGlobals->time).

## 8. Original bugs

- CEgon::WeaponIdle compares the countdown idle timer with gpGlobals->time: the idle body runs on
  every idle frame (animation restarts for cl_lw 0 players). Kept.
- CHgun::SecondaryAttack sets m_flTimeWeaponIdle from gpGlobals->time: no idle animations after a
  secondary shot. Kept.
- CTripmine::SecondaryAttack needs more than ammo_used_per_secondary_mine - 1 mines but takes one.
  Kept.
- Tripmine beam colour comes from one global set at placement and read at power-up: crossed colours
  between mines. Kept. Mode 0 (map-placed mine before any player mine) used uninitialized colours;
  ours 0,0,0.
- CSatchelCharge proximity fuse tests only the first entity UTIL_FindEntityInSphere returns. Kept.
- CRpgRocket::FollowThink homing includes the shooter and dead-flag-1 (dying) entities. Kept.
- CGrenade::ShootTimed reads pevOwner->health with no null check. Kept.
- CGrenade::ShootFlare: dmg 5 gives a negative TE_EXPLOSION scale ((5 - 50) * 0.6) on detonation.
  Kept.
- CRpgRocket: the original decrements m_cActiveRockets only in RocketTouch (SDK 2.3), so a rocket
  that detonates in flight locks the launcher's reload while its laser is on. Fixed by HLU's
  destructor (kept).
- CGauss SecondaryAttack: the ammo burn can take uranium to -1 (SDK 2.3). Fixed by HLU (kept).
