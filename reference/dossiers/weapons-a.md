# Dossier: weapons-a

## 1. Scope

Original files: `flyingcrowbar.cpp` (OZ's), `hl_wpn_glock.cpp` (OZ's, our `dlls/glock.cpp`), and the
stock SDK files OZ changed: `weapons.cpp`, `crowbar.cpp`, `python.cpp`, `mp5.cpp`, `shotgun.cpp`,
`crossbow.cpp`. Also the inline ammo classes funcfile.tsv puts under "(header)" (CGlockAmmo,
CPythonAmmo, CMP5AmmoClip, CMP5Chainammo, CMP5AmmoGrenade, CShotgunAmmo, CCrossbowAmmo).

Files written or touched: `dlls/flyingcrowbar.cpp`, `dlls/glock.cpp`, `dlls/crowbar.cpp`,
`dlls/python.cpp`, `dlls/mp5.cpp`, `dlls/shotgun.cpp`, `dlls/crossbow.cpp`, `dlls/weapons.cpp`,
`dlls/weapons_shared.cpp` (halflife-updated moved part of the original's weapons.cpp there:
AddAmmoNameToAmmoRegistry, CanDeploy, DefaultReload, ResetEmptySound, CanAttack, ItemPostFrame),
`dlls/weapons.h` (shared), `dlls/player.h` (shared).

Compared function by function: every function of all eight files and the seven ammo classes, against
the decompile, and against the disassembly where the decompile was cut short or mangled (glock and
python PrimaryAttack/SecondaryAttack/WeaponIdle, every MP5 and shotgun attack, crowbar
SecondaryAttack, flying crowbar SpinTouch, crossbow FireSniperBolt/Reload/BoltCreate, CWeaponBox::Touch,
CBasePlayerAmmo::DefaultTouch, ItemPostFrame's tail). Nothing only skimmed.

## 2. Status

```
file                 check_file.py --missing     i386   amd64
flyingcrowbar.cpp    5 of 5, 0 missing           built  built
hl_wpn_glock.cpp     15 of 15, 0 missing         built  built
weapons.cpp          123 of 123, 0 missing       built  built
crowbar.cpp          14 of 14, 0 missing         built  built
python.cpp           15 of 15, 0 missing         built  built
mp5.cpp              20 of 20, 0 missing         built  built
shotgun.cpp          11 of 11, 0 missing         built  built
crossbow.cpp         24 of 24, 0 missing         built  built
```

All factories exported (`flying_crowbar`, `weapon_glock`, `weapon_9mmhandgun`, `ammo_glockclip`,
`ammo_9mmclip`, the rest unchanged). Nothing has run in an engine; built is all that is claimed. Only
the server library (`hl`) is built; `cl_dll` would no longer compile against the new CGlock (no
GlockFire, no glock events) and the MP5/python/crossbow changes, which doesn't matter for a
server-only mod.

## 3. Function map

Addresses are funcfile.tsv's (file addresses; Ghidra's are +0x10000), as in the code's `// orig`
comments. "ok" = written from the original as read. Ammo classes ("(header)" in funcfile.tsv), all ok,
AddAmmo/Precache/Spawn: CGlockAmmo 0x143410/0x1434d0/0x143510, CPythonAmmo 0x1405f0/0x1406b0/0x1406f0,
CMP5AmmoClip 0x13f3d0/0x13f490/0x13f4d0, CMP5Chainammo 0x13f230/0x13f2f0/0x13f330, CMP5AmmoGrenade
0x13f080/0x13f140/0x13f180, CShotgunAmmo 0x141090/0x141150/0x141190, CCrossbowAmmo
0x13aa40/0x13ab00/0x13ab40. Each AddAmmo gives OZ's give/max cvars.

flyingcrowbar.cpp -> dlls/flyingcrowbar.cpp

```
0xb4eb0  flying_crowbar                             flyingcrowbar.cpp    ok
0xb4ed0  CFlyingCrowbar::Spawn                      flyingcrowbar.cpp    ok
0xb5000  CFlyingCrowbar::Precache                   flyingcrowbar.cpp    ok
0xb5060  CFlyingCrowbar::SpinTouch                  flyingcrowbar.cpp    VERIFY runtime: rune-3 path leaks the entity (kept)
0xb5600  CFlyingCrowbar::BubbleThink                flyingcrowbar.cpp    ok
```

hl_wpn_glock.cpp -> dlls/glock.cpp

```
0x130360  weapon_glock                               glock.cpp            ok
0x130380  weapon_9mmhandgun                          glock.cpp            ok
0x1303a0  CGlock::Spawn                              glock.cpp            ok
0x130490  CGlock::Precache                           glock.cpp            ok
0x130580  CGlock::GetItemInfo                        glock.cpp            ok
0x130670  CGlock::Deploy                             glock.cpp            ok
0x1306f0  CGlock::Holster                            glock.cpp            ok
0x130760  CGlock::SecondaryAttack                    glock.cpp            ok
0x130830  CGlock::PrimaryAttack                      glock.cpp            rewritten from the disassembly (no GlockFire)
0x130ff0  CGlock::Reload                             glock.cpp            ok
0x1310e0  CGlock::WeaponIdle                         glock.cpp            ok
0x1311f0  ammo_glockclip                             glock.cpp            ok
0x131210  ammo_9mmclip                               glock.cpp            ok
0x131230  CGlock::UseDecrement                       glock.cpp            ok
0x131240  CGlock::iItemSlot                          glock.cpp            ok
```

crowbar.cpp -> dlls/crowbar.cpp

```
0xa4f70  weapon_crowbar                             crowbar.cpp          ok
0xa4f90  CCrowbar::Spawn                            crowbar.cpp          ok
0xa4ff0  CCrowbar::Precache                         crowbar.cpp          ok
0xa50e0  CCrowbar::GetItemInfo                      crowbar.cpp          ok
0xa5180  CCrowbar::Deploy                           crowbar.cpp          ok
0xa51c0  CCrowbar::Holster                          crowbar.cpp          ok
0xa5220  FindHullIntersection                       crowbar.cpp          ok
0xa5450  CCrowbar::PrimaryAttack                    crowbar.cpp          ok
0xa54b0  CCrowbar::SecondaryAttack                  crowbar.cpp          from the disassembly
0xa56f0  CCrowbar::Smack                            crowbar.cpp          ok
0xa5720  CCrowbar::SwingAgain                       crowbar.cpp          ok
0xa5750  CCrowbar::Swing                            crowbar.cpp          ok
0xa6040  CCrowbar::UseDecrement                     crowbar.cpp          ok
0xa6050  CCrowbar::iItemSlot                        crowbar.cpp          ok
```

python.cpp -> dlls/python.cpp

```
0xfce10  weapon_python                              python.cpp           ok
0xfce30  weapon_357                                 python.cpp           ok
0xfce50  CPython::GetItemInfo                       python.cpp           ok
0xfcf40  CPython::AddToPlayer                       python.cpp           ok
0xfcfd0  CPython::Spawn                             python.cpp           ok
0xfd090  CPython::Precache                          python.cpp           ok
0xfd170  CPython::Deploy                            python.cpp           ok
0xfd200  CPython::Holster                           python.cpp           ok
0xfd2a0  CPython::SecondaryAttack                   python.cpp           ok
0xfd3a0  CPython::PrimaryAttack                     python.cpp           from the disassembly
0xfd7a0  CPython::Reload                            python.cpp           ok
0xfd8d0  CPython::WeaponIdle                        python.cpp           ok
0xfda60  ammo_357                                   python.cpp           ok
0xfda80  CPython::UseDecrement                      python.cpp           ok
0xfda90  CPython::iItemSlot                         python.cpp           ok
```

mp5.cpp -> dlls/mp5.cpp

```
0xd9040  weapon_mp5                                 mp5.cpp              ok
0xd9060  weapon_9mmAR                               mp5.cpp              ok
0xd9080  CMP5::SecondaryAmmoIndex                   mp5.cpp              ok
0xd9090  CMP5::Spawn                                mp5.cpp              ok
0xd9150  CMP5::Precache                             mp5.cpp              ok
0xd92c0  CMP5::GetItemInfo                          mp5.cpp              ok
0xd93d0  CMP5::AddToPlayer                          mp5.cpp              ok
0xd9460  CMP5::Deploy                               mp5.cpp              ok
0xd94a0  CMP5::Holster                              mp5.cpp              ok
0xd9530  CMP5::PrimaryAttack                        mp5.cpp              rewritten from the disassembly
0xda090  CMP5::SecondaryAttack                      mp5.cpp              rewritten from the disassembly
0xda4f0  CMP5::Reload                               mp5.cpp              ok
0xda5d0  CMP5::WeaponIdle                           mp5.cpp              ok
0xda690  ammo_mp5clip                               mp5.cpp              ok
0xda6b0  ammo_9mmAR                                 mp5.cpp              ok
0xda6d0  ammo_9mmbox                                mp5.cpp              ok
0xda6f0  ammo_mp5grenades                           mp5.cpp              ok
0xda710  ammo_ARgrenades                            mp5.cpp              ok
0xda730  CMP5::UseDecrement                         mp5.cpp              ok
0xda740  CMP5::iItemSlot                            mp5.cpp              ok
```

shotgun.cpp -> dlls/shotgun.cpp

```
0x106070  weapon_shotgun                             shotgun.cpp          ok
0x106090  CShotgun::Spawn                            shotgun.cpp          ok
0x106130  CShotgun::Precache                         shotgun.cpp          ok
0x106260  CShotgun::AddToPlayer                      shotgun.cpp          ok
0x1062f0  CShotgun::GetItemInfo                      shotgun.cpp          ok
0x1063e0  CShotgun::Deploy                           shotgun.cpp          ok
0x106420  CShotgun::PrimaryAttack                    shotgun.cpp          rewritten from the disassembly
0x106ac0  CShotgun::SecondaryAttack                  shotgun.cpp          rewritten from the disassembly
0x107410  CShotgun::Reload                           shotgun.cpp          ok
0x1076f0  CShotgun::WeaponIdle                       shotgun.cpp          ok
0x107970  ammo_buckshot                              shotgun.cpp          ok
```

crossbow.cpp -> dlls/crossbow.cpp

```
0xa36c0  crossbow_bolt                              crossbow.cpp         ok
0xa36e0  CCrossbowBolt::BoltCreate                  crossbow.cpp         ok
0xa3730  CCrossbowBolt::Spawn                       crossbow.cpp         ok
0xa3830  CCrossbowBolt::Precache                    crossbow.cpp         ok
0xa38d0  CCrossbowBolt::Classify                    crossbow.cpp         ok
0xa38e0  CCrossbowBolt::BoltTouch                   crossbow.cpp         ok
0xa3e70  CCrossbowBolt::BubbleThink                 crossbow.cpp         ok
0xa3f20  CCrossbowBolt::ExplodeThink                crossbow.cpp         ok
0xa40d0  weapon_crossbow                            crossbow.cpp         ok
0xa40f0  CCrossbow::Spawn                           crossbow.cpp         ok
0xa4190  CCrossbow::AddToPlayer                     crossbow.cpp         ok
0xa4220  CCrossbow::Precache                        crossbow.cpp         ok
0xa42e0  CCrossbow::GetItemInfo                     crossbow.cpp         ok
0xa43c0  CCrossbow::Deploy                          crossbow.cpp         ok
0xa4420  CCrossbow::Holster                         crossbow.cpp         ok
0xa44b0  CCrossbow::PrimaryAttack                   crossbow.cpp         ok
0xa4500  CCrossbow::FireSniperBolt                  crossbow.cpp         ok
0xa47c0  CCrossbow::FireBolt                        crossbow.cpp         ok
0xa4b20  CCrossbow::SecondaryAttack                 crossbow.cpp         ok
0xa4c20  CCrossbow::Reload                          crossbow.cpp         ok
0xa4d50  CCrossbow::WeaponIdle                      crossbow.cpp         ok
0xa4e90  ammo_crossbow                              crossbow.cpp         ok
0xa4f00  CCrossbow::UseDecrement                    crossbow.cpp         ok
0xa4f10  CCrossbow::iItemSlot                       crossbow.cpp         ok
```

weapons.cpp -> dlls/weapons.cpp

```
0x128a40  MaxAmmoCarry                               weapons.cpp          ok
0x128b50  ClearMultiDamage                           weapons.cpp          ok
0x128b90  ApplyMultiDamage                           weapons.cpp          ok
0x128be0  AddMultiDamage                             weapons.cpp          ok
0x128c40  SpawnBlood                                 weapons.cpp          ok
0x128c90  DamageDecal                                weapons.cpp          ok
0x128ce0  DecalGunshot                               weapons.cpp          ok
0x128e40  EjectBrass                                 weapons.cpp          ok
0x128f70  AddAmmoNameToAmmoRegistry                  weapons_shared.cpp   ok
0x129000  UTIL_PrecacheOtherWeapon                   weapons.cpp          ok
0x129170  W_Precache                                 weapons.cpp          ok
0x1294d0  CBasePlayerItem::Save                      weapons.cpp          ok
0x129520  CBasePlayerItem::Restore                   weapons.cpp          ok
0x129570  CBasePlayerWeapon::Save                    weapons.cpp          ok
0x1295c0  CBasePlayerWeapon::Restore                 weapons.cpp          ok
0x129610  CBasePlayerItem::SetObjectCollisionBox     weapons.cpp          ok
0x1296e0  CBasePlayerItem::FallInit                  weapons.cpp          ok
0x1297b0  CBasePlayerItem::FallThink                 weapons.cpp          ok
0x129880  CBasePlayerItem::Materialize               weapons.cpp          ok
0x129920  CBasePlayerItem::AttemptToMaterialize      weapons.cpp          ok
0x129990  CBasePlayerItem::CheckRespawn              weapons.cpp          ok
0x1299e0  CBasePlayerItem::Respawn                   weapons.cpp          ok
0x129ae0  CBasePlayerItem::DefaultTouch              weapons.cpp          ok
0x129bc0  CanAttack                                  weapons_shared.cpp   ok
0x129c00  CBasePlayerWeapon::ItemPostFrame           weapons_shared.cpp   ok
0x12a0a0  CBasePlayerItem::DestroyItem               weapons.cpp          ok
0x12a0f0  CBasePlayerItem::AddToPlayer               weapons.cpp          ok
0x12a110  CBasePlayerItem::Drop                      weapons.cpp          ok
0x12a170  CBasePlayerItem::Kill                      weapons.cpp          ok
0x12a1d0  CBasePlayerItem::Holster                   weapons.cpp          ok
0x12a200  CBasePlayerItem::AttachToPlayer            weapons.cpp          ok
0x12a2b0  CBasePlayerWeapon::AddDuplicate            weapons.cpp          ok
0x12a300  CBasePlayerWeapon::AddToPlayer             weapons.cpp          no AddWeapon call (HLU fix, AddPlayerItem extracts)
0x12a3c0  CBasePlayerWeapon::UpdateClientData        weapons.cpp          ok
0x12a530  CBasePlayerWeapon::SendWeaponAnim          weapons.cpp          OZ always-send ids merged with HLU m_ForceSendAnimations
0x12a630  CBasePlayerWeapon::AddPrimaryAmmo          weapons.cpp          ok
0x12a730  CBasePlayerWeapon::AddSecondaryAmmo        weapons.cpp          ok
0x12a7b0  CBasePlayerWeapon::IsUseable               weapons.cpp          ok
0x12a820  CBasePlayerWeapon::CanDeploy               weapons_shared.cpp   ok
0x12a900  CBasePlayerWeapon::DefaultDeploy           weapons.cpp          OZ zoom resets added
0x12aa40  CBasePlayerWeapon::DefaultReload           weapons_shared.cpp   ok
0x12ab20  CBasePlayerWeapon::PlayEmptySound          weapons.cpp          ok
0x12ab90  CBasePlayerWeapon::ResetEmptySound         weapons_shared.cpp   ok
0x12abb0  CBasePlayerWeapon::PrimaryAmmoIndex        weapons.cpp          ok
0x12abc0  CBasePlayerWeapon::SecondaryAmmoIndex      weapons.cpp          ok
0x12abd0  CBasePlayerWeapon::Holster                 weapons.cpp          OZ zoom resets added
0x12ac70  CBasePlayerAmmo::Spawn                     weapons.cpp          ok
0x12ad00  CBasePlayerAmmo::Respawn                   weapons.cpp          ok
0x12ad90  CBasePlayerAmmo::Materialize               weapons.cpp          ok
0x12ae10  CBasePlayerAmmo::DefaultTouch              weapons.cpp          ok
0x12aef0  CBasePlayerWeapon::ExtractAmmo             weapons.cpp          ok
0x12afa0  CBasePlayerWeapon::ExtractClipAmmo         weapons.cpp          ok
0x12b020  CBasePlayerWeapon::RetireWeapon            weapons.cpp          ok
0x12b080  weaponbox                                  weapons.cpp          ok
0x12b0a0  CWeaponBox::Save                           weapons.cpp          ok
0x12b0f0  CWeaponBox::Restore                        weapons.cpp          ok
0x12b140  CWeaponBox::Precache                       weapons.cpp          ok
0x12b170  CWeaponBox::KeyValue                       weapons.cpp          ok
0x12b350  CWeaponBox::Spawn                          weapons.cpp          ok
0x12b3c0  CWeaponBox::Kill                           weapons.cpp          ok
0x12b450  CWeaponBox::Touch                          weapons.cpp          OZ crowbar check added
0x12b660  CWeaponBox::PackWeapon                     weapons.cpp          OZ crowbar model added
0x12b7c0  CWeaponBox::PackAmmo                       weapons.cpp          ok
0x12b850  CWeaponBox::GiveAmmo                       weapons.cpp          ok
0x12b970  CWeaponBox::HasWeapon                      weapons.cpp          ok
0x12ba00  CWeaponBox::IsEmpty                        weapons.cpp          ok
0x12ba40  CWeaponBox::SetObjectCollisionBox          weapons.cpp          ok
0x12bb00  CBasePlayerWeapon::PrintState              weapons.cpp          ok
0x12bbb0  CRpg::Save                                 weapons.cpp          ok
0x12bc00  CRpg::Restore                              weapons.cpp          ok
0x12bc50  CRpgRocket::Save                           weapons.cpp          ok
0x12bca0  CRpgRocket::Restore                        weapons.cpp          ok
0x12bcf0  CShotgun::Save                             weapons.cpp          ok
0x12bd40  CShotgun::Restore                          weapons.cpp          ok
0x12bd90  CGauss::Save                               weapons.cpp          ok
0x12bde0  CGauss::Restore                            weapons.cpp          ok
0x12be30  CEgon::Save                                weapons.cpp          ok
0x12be80  CEgon::Restore                             weapons.cpp          ok
0x12bed0  CSatchel::Save                             weapons.cpp          ok
0x12bf20  CSatchel::Restore                          weapons.cpp          ok
0x12bf70  CSatchel::UseDecrement                     weapons.cpp          ok
0x12bf80  CSatchel::iItemSlot                        weapons.cpp          ok
0x12bfe0  CEgon::UseDecrement                        weapons.cpp          ok
0x12bff0  CEgon::iItemSlot                           weapons.cpp          ok
0x12c050  CGauss::UseDecrement                       weapons.cpp          ok
0x12c060  CGauss::iItemSlot                          weapons.cpp          ok
0x12c110  CRpg::UseDecrement                         weapons.cpp          ok
0x12c120  CRpg::ShouldWeaponIdle                     weapons.cpp          ok
0x12c130  CRpg::iItemSlot                            weapons.cpp          ok
0x12c190  CShotgun::UseDecrement                     weapons.cpp          ok
0x12c1a0  CShotgun::iItemSlot                        weapons.cpp          ok
0x12c250  CBasePlayerAmmo::AddAmmo                   weapons.cpp          ok
0x12c2b0  CBasePlayerWeapon::GetWeaponPtr            weapons.cpp          ok
0x12c2c0  CBasePlayerWeapon::UseDecrement            weapons.cpp          ok
0x12c2d0  CBasePlayerWeapon::ShouldWeaponIdle        weapons.cpp          ok
0x12c2e0  CBasePlayerWeapon::WeaponIdle              weapons.cpp          ok
0x12c2f0  CBasePlayerWeapon::Reload                  weapons.cpp          ok
0x12c300  CBasePlayerWeapon::SecondaryAttack         weapons.cpp          ok
0x12c310  CBasePlayerWeapon::PrimaryAttack           weapons.cpp          ok
0x12c320  CBasePlayerWeapon::UpdateItemInfo          weapons.cpp          ok
0x12c330  CBasePlayerWeapon::AddWeapon               weapons.cpp          present, no caller on the pickup path
0x12c3b0  CBasePlayerItem::iFlags                    weapons.cpp          ok
0x12c3f0  CBasePlayerItem::iWeight                   weapons.cpp          ok
0x12c430  CBasePlayerItem::iMaxClip                  weapons.cpp          ok
0x12c470  CBasePlayerItem::pszName                   weapons.cpp          ok
0x12c4b0  CBasePlayerItem::iMaxAmmo2                 weapons.cpp          ok
0x12c4f0  CBasePlayerItem::pszAmmo2                  weapons.cpp          ok
0x12c530  CBasePlayerItem::iMaxAmmo1                 weapons.cpp          ok
0x12c570  CBasePlayerItem::pszAmmo1                  weapons.cpp          ok
0x12c5b0  CBasePlayerItem::iItemPosition             weapons.cpp          ok
0x12c5f0  CBasePlayerItem::iItemSlot                 weapons.cpp          ok
0x12c600  CBasePlayerItem::GetWeaponPtr              weapons.cpp          ok
0x12c610  CBasePlayerItem::UpdateClientData          weapons.cpp          ok
0x12c620  CBasePlayerItem::SecondaryAmmoIndex        weapons.cpp          ok
0x12c630  CBasePlayerItem::PrimaryAmmoIndex          weapons.cpp          ok
0x12c640  CBasePlayerItem::ItemPostFrame             weapons_shared.cpp   ok
0x12c650  CBasePlayerItem::ItemPreFrame              weapons.cpp          ok
0x12c660  CBasePlayerItem::UpdateItemInfo            weapons.cpp          ok
0x12c670  CBasePlayerItem::CanHolster                weapons.cpp          ok
0x12c680  CBasePlayerItem::Deploy                    weapons.cpp          ok
0x12c690  CBasePlayerItem::CanDeploy                 weapons_shared.cpp   ok
0x12c6a0  CBasePlayerItem::GetItemInfo               weapons.cpp          ok
0x12c6b0  CBasePlayerItem::AddDuplicate              weapons.cpp          ok
```

## 4. Classes

- **CFlyingCrowbar** (flyingcrowbar.cpp, declared there; base CBasePlayerWeapon per hierarchy.tsv, no
  new virtuals, Spawn and Precache overridden). `m_hOwner` EHANDLE at +0xcc, from code (Spawn
  assigns it from pev->owner, SpinTouch credits damage to it). VERIFY name. `m_pPlayer` (the base's,
  +0x80) is the thrower, set by CCrowbar::SecondaryAttack. No save table in the original.
- **CBasePlayerWeapon** (shared): `SuperGlockZoom` +0xc0 and `OzZoom` +0xc4, from code (DefaultDeploy,
  Holster, CMP5); names from the 1.7 source's weapons.h, which has them in the same place between
  m_fInReload and m_iDefaultAmmo. Not saved (the original's save table is SDK 2.3's 7 fields).
- **CGlock**: `m_fInZoom` +0xcc (its own zoom flag; VERIFY name, 1.7 used the base's OzZoom), `m_iShell`
  +0xd0. The SDK's m_usFireGlock1/2 and GlockFire are gone, as in the original.
- **CCrowbar**: m_iSwing +0xcc, m_trHit +0xd0, m_usCrowbar +0x108 (stock); SecondaryAttack added.
- **CPython**: `m_flSoundDelay` +0xcc and `m_fInZoom` +0xd0 back (SDK 2.3's, removed by halflife-updated),
  m_usFirePython +0xd4. AddToPlayer added.
- **CMP5**: m_flNextAnimTime +0xcc, m_iShell +0xd0 (stock); AddToPlayer, Holster, SecondaryAmmoIndex added.
  The SDK's m_usMP5/m_usMP52 stay declared, unused (OZ fires no MP5 events).
- **CCrossbow**: `m_fInZoom` +0xcc back, m_usCrossbow +0xd0, m_usCrossbow2 +0xd2; AddToPlayer added.
- **CShotgun**: +0xcc halflife-updated's unused m_fInReload, m_flNextReload +0xd0, m_iShell +0xd4,
  m_usDoubleFire +0xd8, m_usSingleFire +0xda (stock); AddToPlayer added. Save table unchanged (stock).
- **CBasePlayer** (shared): `m_iRune` +0x674, see section 5.

## 5. Cross-file needs

- `dlls/player.h`, CBasePlayer, `// weapons-a needs:` block: `int m_iRune` (+0x674, the active rune).
  The items package declares the same member under the same name in its own block; keep one at merge.
  Rune ids are literals with a comment (3 Super Crowbar, 4 Haste, 11 Super Glock), since items.h
  isn't in this worktree; items' `RUNE_*` names (RUNE_HASTE...) should replace them at merge.
- `dlls/weapons.h` `Bullet` enum, `// weapons-a:` comment: OZ's values in the 1.7 order (9MM_SNIPER 2,
  9MM_SUPERGLOCK 3, MP5 4, MP5_SNIPER 5, 357 6, 357_SNIPER 7, BUCKSHOT 8, CROWBAR 9, MONSTER_* 10-12).
  Same names and values as the items package's edit; the text differs, so take one at merge.
  FireBulletsPlayer (combat.cpp) must handle the sniper/superglock types; it isn't this package's.
- `dlls/weapons.h` CBasePlayerWeapon, `// weapons-a:` block: SuperGlockZoom, OzZoom.
- `dlls/weapons.h`: my classes' declarations only (CGlock, CCrowbar, CPython, CMP5, CCrossbow, CShotgun).
- CHgun (weapons-b's class): its save table in weapons.cpp and its Save/Restore/m_SaveData
  declarations in weapons.h removed, at the coordinator's word: the original's CHgun has none.
- WeapPickup: the AddToPlayer of python, mp5, shotgun, crossbow send it (SDK 2.3, FIXES.md design row
  for AddPlayerItem). **player.cpp's CBasePlayer::AddPlayerItem still sends it for every non-exhaustible
  weapon** (halflife-updated's); the player package must drop that, as mod-rc2 did, or these four
  announce twice. weapons-b (merged) sends it from the egon, gauss, rpg and hornetgun AddToPlayer the same way.
- AddWeapon: no caller on the pickup path, by FIXES.md's AddPlayerItem fix verdict (AddPlayerItem
  extracts after inserting). CSatchel::AddToPlayer (weapons-b) calls it in the original.
- pev->fov: OZ's zooms set `pev->fov` (and m_iFOV, except the MP5 sniper, which sets only pev->fov) and
  test it. halflife-updated's 6f8aac9 made client.cpp's UpdateClientData send `m_iFOV` as cd->fov and
  dropped the pev->fov resets in CBasePlayer::Killed, StartObserver and Spawn. For the MP5 sniper zoom
  to show and for the zoom checks to reset at death/spawn, client.cpp (`cd->fov = pev->fov`) and
  player.cpp need the original's. Not this package's files; flagged for the client and player packages.
- No temporary stubs.

## 6. VERIFY list

- `CFlyingCrowbar::m_hOwner` name (code only). Settles: nothing in the binary names it.
- `CGlock::m_fInZoom` name. Same.
- `CBasePlayerWeapon::SendWeaponAnim` dereferences `m_pPlayer->m_pActiveItem` without a null check, as
  the original does. halflife-updated's paths call SendWeaponAnim from Deploy/Holster with an active
  item set as far as read, but a runtime test should confirm no path reaches it with none (crash).
- Unclear halflife-updated call, not decided: **GetNextAttackDelay** (the 2013 SDK's fire-rate creep
  compensation, m_flPrevPrimaryAttack/m_flLastFireTime). The original has no such function; all its
  weapons set `UTIL_WeaponTimeBase() + cycle`. My files use the original's form; GetNextAttackDelay and
  its m_flLastFireTime resets (ItemPostFrame, DefaultDeploy) stay for weapons-b's callers. Whether the
  server should compensate (it changes the server's fire rate slightly against Valve's predicting
  client, which compensates) is for Steven; weapons-b has the same question.

## 7. Behavior worth testing

- Crowbar secondary: throw, damage (2x oz_damageflyingcrowbar without the rune), the weaponbox with a
  crowbar model left behind for 120 s, only pickable by a player without a crowbar.
- Super Crowbar Rune: 1000-damage swings, unlimited throws at half damage; the leaked invisible
  entities (edict count climbs).
- Glock zoom (secondary), zoomed fire quiet with no flash, Super Glock Rune zoomed: no ammo cost,
  tu_fire1 sound, 15-degree kick; the idle never returning after a reload.
- MP5: oz_mp5spread 0 or 11+ fires nothing; oz_silencemp5 2 quiet but loud sound; oz_mp5sniper 1 zoom
  (needs the client.cpp fov point above); haste triple kick.
- Shotgun: the server-side branch (haste, underwater with oz_waterweapons, or cycle < 0.3) against the
  event branch; oz_reloadshotgun > 1 draining the reserve below zero.
- Python: always the fidget idle; haste cycle 0.6/0.7.
- Crossbow: bolt classname "crossbow" in death notices; reload sound with nothing to reload never
  happens (guarded) but plays even when DefaultReload refuses.
- Prediction: glock, MP5 and shotgun anims always reach the shooter (SendWeaponAnim); Valve's client
  will still predict its own stock fire for them, as in 2004.

## 8. Original bugs

All kept.

- flyingcrowbar.cpp SpinTouch: with the Super Crowbar Rune the thrown crowbar is never removed
  (invisible, SOLID_NOT, BubbleThink every 0.25 s forever): an edict leak per throw.
- flyingcrowbar.cpp SpinTouch: damage is halved with the rune (oz_damageflyingcrowbar) and doubled
  without; the inflictor passed to TraceAttack is the crowbar, the attacker the owner.
- crowbar.cpp SecondaryAttack: angles.z = v_forward.z - 90 (a vector component, not an angle); no
  UTIL_MakeVectors, so it throws along whatever gpGlobals->v_forward last held.
- weapons.cpp CWeaponBox::Touch: a box holding a crowbar, touched by a player who has one, returns after
  handing out its ammo and any weapons in earlier slots, and stays.
- hl_wpn_glock.cpp PrimaryAttack: the fire anim is GLOCK_SHOOT or GLOCK_SHOOT_EMPTY at random.
- hl_wpn_glock.cpp Reload: m_flTimeWeaponIdle = gpGlobals->time + 10..15 on a 0-based weapon clock, so
  the glock stops idling for as long as the map has run.
- python.cpp WeaponIdle: UTIL_SharedRandomFloat(seed, 10, 15) compared against 0.5/0.7/0.9: always the
  fidget.
- mp5.cpp PrimaryAttack: oz_mp5spread outside 1..10 fires no bullet but spends the round; unzoomed the
  oz_recoilmp5 subtraction is overwritten by the RANDOM_FLOAT punch.
- mp5.cpp PrimaryAttack: the silenced sound only for oz_silencemp5 exactly 1, the quiet volume for any
  nonzero value. Spawn disables only at oz_disablemp5 exactly 1 (shotgun likewise).
- mp5.cpp SecondaryAttack (sniper): zooms via pev->fov only, m_iFOV untouched.
- mp5.cpp CMP5AmmoGrenade::Spawn: with oz_disablem203 >= 1 the entity is left unset-up, not removed.
- shotgun.cpp Reload: the reserve is charged oz_reloadshotgun shells per insert whatever it holds; can
  go negative.
- shotgun.cpp PrimaryAttack/SecondaryAttack, event branch: no SetAnimation (the third-person attack
  anim never plays).
- crossbow.cpp Reload: the reload sound plays even when DefaultReload refuses.
- CBasePlayerWeapon::DefaultDeploy resets m_iFOV but not pev->fov for a leftover zoom.

## halflife-updated calls (FIXES.md rule)

```
function                              commit        verdict  note
CCrowbar::Swing  event only on fFirst 6528126       fix      SwingAgain replayed the swing event
CCrowbar::Swing  next attack before   511c6ef       fix      corpse took swings with no delay
                 the dead-target return
CCrowbar::Swing  Smack think on time  c90351f       fix      UTIL_WeaponTimeBase()+0.2 decalled at once
CCrowbar::Swing  <= full swing        a542e4a       fix      inert in multiplayer
CPython::PrimaryAttack dry fire       75bc5af       fix      repeated dry-fire sound
CPython/CCrossbow m_fInZoom, pev->fov d4b0e77 6f8aac9 design  OZ's zoom code is built on them; the fix is
                                                             singleplayer save/restore
CCrossbow events' ammo int           (0 in HLU)    design   restored the original's reserve count
CShotgun pump sound in ItemPostFrame  a65846e       fix      pump never played while holding attack or
                 and on the last shot                        after the last shell
CBasePlayerItem::AttemptToMaterialize f14827a       fix      absolute time
CBasePlayerItem::AttachToPlayer SetThink(NULL) ad50584 fix   double pickup crash
CBasePlayerItem::FallThink busters    e0ba8ce       kept     busters mode kept per DEVIATIONS.md, inert
CBasePlayerWeapon::AddPrimaryAmmo     3d13792       fix      sound test (goes with 6951bf1)
CBasePlayerWeapon::IsUseable, PlayEmptySound, RetireWeapon, SendWeaponAnim m_ForceSendAnimations,
CBasePlayerWeapon::AddToPlayer (no extract)          fix      FIXES.md rows, as mod-rc2
DefaultReload forwards body           a3783be       kept     inert: weapons-a's callers pass 0
GetNextAttackDelay                    seed          unclear  see VERIFY list
```
