# Bugs in the original Oz Deathmatch 3.02

Bugs and oddities in `hl_i386.so` found while reconstructing it. The rebuild keeps each one as the
original has it unless the **Ours** column says otherwise. The package's dossier
(`reference/dossiers/<package>.md`, its "Original bugs" section) has the full reasoning. Add new ones
here as they turn up.

Status: **kept** = same behavior as the original; **guarded** = we avoid a crash the original has
(each also in `DEVIATIONS.md`, "Crashes in the original", and marked at its spot with
`// The original ... (reference/DEVIATIONS.md, Crashes in the original)`); **changed** = we behave
differently for another reason, said in the column (mostly a halflife-updated fix kept, by
`../mod-rc2/reference/audit/FIXES.md`).

Policy (Steven, 2026-11-07): every crash or hang the original has gets the smallest guard that avoids
it, and nothing else changes. A crash is a null or out-of-range dereference, an unbounded write or
read past a buffer, unbounded recursion or an endless loop, or an entity leak without bound. Every
other bug is kept until the cleanup pass described in `CLAUDE.md` (one commit each, anything that
changes play put to Steven first).

Addresses are file addresses in `hl_i386.so` (`reference/funcfile.tsv`, the code's `// orig` tags).
Files are under `dlls/`.

## Crashes

```
Where                                   What                                                  Ours
items.cpp SpawnRunes (0xcbad0),         a map with no info_player_deathmatch: the chosen      guarded
  CRune::Move (0xcc9e0)                 start is NULL and dereferenced                        (no rune / stays)
grapple.cpp CGrappleBolt::BubbleThink   looks up its owner every think and never uses it;     guarded
  (0xc47f0)                             with no owner GetClassPtr(NULL) makes a new entity    (lookup dropped)
                                        each time
bot_ai.cpp CBaseBot::ActionChooseGoal   an owned "weapon*" entity (in practice a weaponbox    guarded
  (0x83700)                             with ammo) `continue`s without stepping: endless loop (steps on)
bot_ai.cpp CBaseBot::ActionChooseWeapon a bot with only a satchel or tripmine (or nothing     guarded
  (0x83b60), bot_fightstyle.cpp         else that can deploy) dispatches it again, and       (returns)
  UseSatchel (0x87700), UseTripMine     UseSatchel/UseTripMine call straight back:
  (0x87850)                             recursion until the stack runs out
bot_fightstyle.cpp UseCrossbow          read the enemy's origin unchecked; a bot aiming at a  guarded
  (0x87250), UseMP5 (0x87560),          tripmine with no enemy gets there                     (distance 0)
  UseShotgun (0x87730)
bot_ai.cpp CBaseBot::ThinkMood          reads the active item unchecked: a live bot has none  guarded
  (0x852b0)                             after player_weaponstrip, with no start weapons, or   (tests skipped)
                                        after its last grenade
bot_nav.cpp CGraph::SaveNavToFile       path formatted into 50 bytes (a long map name or      guarded
  (0x8d850)                             runfolder overruns it), fopen unchecked: no           (256 bytes, no
                                        <runfolder>/nav crashes the map change                file no write)
bot_stats.cpp CBaseBotStats::           strcpy's bot_name/bot_model into 64 bytes and the     guarded
  ReadBotFile (0x8e0b0)                 colour cvars into 4                                   (cut to fit)
bot_misc.cpp UseBotCycle (0x88310)      a bot cycle line over 82 characters overruns the      guarded
                                        100-byte exec command                                 (128 bytes)
bot_misc.cpp BotClientPutInServer       runfolder plus the cycle line into a 120-byte path    guarded
  (0x885e0)                                                                                   (256 bytes)
client.cpp StartFrame (0x97f20)         strcpy's the "bot" cvar into an 80-byte static        guarded
                                                                                              (cut to fit)
client.cpp ClientCommand (0x959e0)      removebot sprintf's the client's argument into 128    guarded
                                        bytes                                                 (cut to fit)
client.cpp PlayerPostThink (0x976b0)    calls through pPlayer before its null check           guarded
                                                                                              (returns first)
client.cpp PlayerPostThink (0x976b0)    sprintf's oz_link, and oz_line1 with oz_line2, into   guarded
                                        201 bytes on every join                               (cut to fit)
player.cpp CBasePlayer::TraceAttack     the hit message pointer is uninitialized for          guarded
  (0xf0bb0)                             HITGROUP_GENERIC and hitgroups above 7, then printed  (NULL, VERIFY)
player.cpp CBasePlayer::                the booby trap creates a CGrenade it never uses (the  guarded
  PlayerDeathThink (0xf2920)            static ShootTimed makes its own): one empty edict     (static call)
                                        per blast
player.cpp CBasePlayer::                `else if`: an item both active and last (SwitchWeapon guarded
  RemovePlayerItem (0xf8670)            doesn't update m_pLastItem) stays last after it goes, (`if`, as
                                        and lastinv deploys the removed item                  halflife-updated)
flyingcrowbar.cpp CFlyingCrowbar::      under the Super Crowbar Rune the thrown crowbar is    guarded
  SpinTouch (0xb5060)                   never removed (invisible, thinking every 0.25 s) and  (removed after
                                        the rune throws without limit                         0.1 s)
```

### Called a crash, but the path can't happen

Left as the original has it. The reason in each case is in a comment at the spot or in the dossier.

```
Where                                   Why it can't crash
multiplay_gamerules.cpp                 pAttacker is NULL only for a player whose Classify isn't CLASS_PLAYER;
  CHalfLifeMultiplay::IPointsForKill    CBasePlayer::Classify always returns it and nothing overrides it
  (0xdbf30)
player.cpp CBasePlayer::PreThink        hanging with no ground entity, pTrain is the world: Instance maps NULL
  (0xf4190)                             to it (SDK 2.3 did the same), so pTrain->pev is never a NULL read
weapons.cpp CBasePlayerWeapon::         reads m_pActiveItem unchecked, but every caller is the active weapon
  SendWeaponAnim (0x12a530)             (ItemPostFrame) or a Deploy/Holster made while it is set
rpg.cpp CRpgRocket::FollowThink         GetClassPtr(owner) with the result unused would make an entity per
  (0xfe1a0)                             think with no owner; the owner is always the shooter, and Explode
                                        clears it only after Smoke replaced the think
ggrenade.cpp CGrenade::ShootTimed       reads pevOwner->health unchecked; every caller passes its own pev or
  (0xc34b0)                             its player's
bot_nav.cpp CGraph::parse_nav_line      4/4/4/8-byte buffers and unchecked indices, but nothing calls
  (0x8cea0)                             FLoadBotNav (dead code, below)
```

## Logic bugs

```
Where                                   What                                                  Ours
---- bots
bot_ai.cpp ammo_check (0x85df0)         built at load from the max_* cvars before they are    kept
                                        registered: every max is 0, CanHaveAmmo refuses,
                                        bots never go for ammo
bot_ai.cpp BotAmmoCount (ActionOpenFire indexes m_rgAmmo with -1 for the glock's      changed (reads
  0x83560)                              secondary, reading +0x4d0 (m_pLastItem)               m_pLastItem
                                                                                              itself)
bot_fightstyle.cpp SetNextShootTime     adds m_flMSec (milliseconds, 1-100) to a time in      kept
  (0x87140)                             seconds: a tap-fire window up to 100 s
bot_move.cpp SteerGroupAlign            divide by the neighbour count unchecked (NaN          kept
  (0x8a610), SteerGroupCohesion         velocity with none); Separation seeks the summed
  (0x8a7a0), SteerGroupSeparation       away-vector as a point; evade/pursue lead by velocity
  (0x8aa20)                             times distance
bot_move.cpp CheckPathObstructedSides   adds 45 degrees to an angles vector's yaw, then       kept
  (0x89890)                             normalizes it as a direction
bot_move.cpp HandleObstacles (0x8a0b0)  an over-large turn clamps to the opposite side; the   kept
                                        left side trace reuses the front one
bot_ai.cpp ActionLook (0x83d20)         compares the whole pev->flags with FL_SPECTATOR       kept
bot_misc.cpp UseBotCycle (0x88310)      the line isn't trimmed: the shipped CRLF              kept
                                        botcycle.txt gives "name\r"
bot_misc.cpp BotConnect (0x88990)       the name filter skips the character after each one    kept
                                        it removes
bot_misc.cpp CBaseBotGlobals::          adds 12 s to the last time, not to now: after a long  kept
  HandleAutoAdjDifficulty (0x88050)     gap it runs every frame until it catches up
---- items, grapple, chargers
items.cpp CanHaveRune (0xcb940)         case 10 has no break: the booby trap rune is allowed  kept
                                        whenever the super glock rune is on
items.cpp CRune::Touch (0xcc670)        sets the rune's own maxspeed, not the player's        kept
                                        (ActivateRune sets the player's after); plays
                                        gunpickup2.wav twice (again in RuneHud)
items.cpp CRune::Touch                  the non-teamplay pickup log line prints the user id   kept
                                        as the team; the teamplay line has a double space
items.cpp CRune::Precache (0xcc560)     tests rune_scheme where rune_model was meant          kept
                                        (harmless: item_suit precaches every rune model)
grapple.cpp CGrappleBolt::BoltTouch     with the owner ducking, moves the hook 18 units up    kept
  (0xc4260)                             instead of back along its flight
h_battery.cpp CRecharge::Use (0xc5a50)  no floor on the juice: a capacity that isn't a        kept
                                        multiple of the amount goes negative and Off never
                                        recharges it
---- player
player.cpp CBasePlayer::                the booby trap renames the player's classname to      kept
  PlayerDeathThink (0xf2920)            "Booby Trap Rune" until respawn
player.cpp CBasePlayer::PostThink       one static `donethis` for all players: Identify runs  kept
  (0xf4f00)                             on alternate frames shared across players
player.cpp CBasePlayer::                with oz_nightops the flashlight time stays due, so    kept
  UpdateClientData (0xf8ae0)            FlashBat goes out every frame while the light is on
player.cpp CBasePlayer::DetectCheats    one global timer, so one player per 5 s round; the    kept
  (0xfc420)                             log prints the auth ID's address with %i
player.cpp CBasePlayer::MOTD (0xfb470)  a map MOTD file is loaded twice (both freed)          kept
---- rules
client.cpp ClientDisconnect (0x950d0)   frees a bot's private data, then GetClassPtr builds   kept
                                        a blank player: its rune isn't respawned, its hook
                                        isn't killed
client.cpp StartFrame (0x97f20)         addbot past 31 players returns before the "bot"       kept
                                        cvar is cleared, so it repeats every second
client.cpp ClientCommand (0x959e0)      removebot kicks any entity with that netname, human   kept
                                        or bot, and has the host run the kick
world.cpp CWorld::Precache (0x12d3f0)   with night ops the gDecals index lookup is skipped:   kept
                                        decals keep the last map's indices (0 on the first)
client.cpp Host_Say (0x95490)           logchat is inverted: 1 (the default) never logs       kept
client.cpp PlayerPostThink (0x976b0)    the default-name test reads isdigit(name[1]) for a    kept
                                        9-character name (VERIFY)
teamplay_gamerules.cpp                  calls TeamWithFewestPlayers and drops the result      kept
  CHalfLifeTeamplay::InitHUD (0x1147a0) (harmless: Spawn sets the team)
---- weapons-a
flyingcrowbar.cpp CFlyingCrowbar::      damage halved with the rune and doubled without; the  kept
  SpinTouch (0xb5060)                   inflictor is the crowbar, the attacker the owner
crowbar.cpp CCrowbar::SecondaryAttack   angles.z = v_forward.z - 90 (a vector component); no  kept
  (0xa54b0)                             UTIL_MakeVectors, so it throws along whatever
                                        v_forward last held
weapons.cpp CWeaponBox::Touch           a box with a crowbar, touched by a player who has     kept
  (0x12b450)                            one, returns after giving its ammo and earlier
                                        slots' weapons, and stays
hl_wpn_glock.cpp CGlock::PrimaryAttack  the fire anim is SHOOT or SHOOT_EMPTY at random       kept
  (0x130830)
hl_wpn_glock.cpp CGlock::Reload         idle time = gpGlobals->time + 10..15 on a 0-based     kept
  (0x130ff0)                            weapon clock: no idle for as long as the map has run
hl_wpn_glock.cpp CGlock::Reload         asks DefaultReload for clip_glock + 1 when the clip   changed (halflife-
  (0x130ff0)                            isn't empty (SDK 2.3's 18), which fills only to the   updated's: the
                                        clip: a full glock with reserve ammo replays the      clip, both
                                        reload on every press and gains nothing               branches)
python.cpp CPython::WeaponIdle          a 10-15 random compared with 0.5/0.7/0.9: always the  kept
  (0xfd8d0)                             fidget
mp5.cpp CMP5::PrimaryAttack (0xd9530)   oz_mp5spread outside 1-10 fires no bullet but spends  kept
                                        the round; unzoomed, the recoil is overwritten by
                                        the random punch; the silenced sound only at
                                        oz_silencemp5 1, the quiet volume at any nonzero;
                                        Spawn disables only at oz_disablemp5 1 (shotgun too)
mp5.cpp CMP5::SecondaryAttack           the sniper zoom sets pev->fov only, not m_iFOV        changed (one
  (0xda090)                                                                                   FOV, 6f8aac9)
mp5.cpp CMP5AmmoGrenade::Spawn          with oz_disablem203 >= 1 the entity is left           kept
  (0x13f180)                            unset-up, not removed
shotgun.cpp CShotgun::Reload            charges oz_reloadshotgun shells per insert whatever   kept
  (0x107410)                            the reserve holds: it can go negative
shotgun.cpp CShotgun::PrimaryAttack,    event branch: no SetAnimation, so no third-person     kept
  SecondaryAttack (0x106420, 0x106ac0)  attack anim
crossbow.cpp CCrossbow::Reload          the reload sound plays even when DefaultReload        kept
  (0xa4c20)                             refuses
weapons.cpp CBasePlayerWeapon::         resets m_iFOV but not pev->fov for a leftover zoom    changed (one
  DefaultDeploy (0x12a900)                                                                    FOV, 6f8aac9)
---- weapons-b
egon.cpp CEgon::WeaponIdle (0xb3fd0)    compares the countdown idle timer with                kept
                                        gpGlobals->time: the idle runs on every idle frame
hornetgun.cpp CHgun::SecondaryAttack    idle time from gpGlobals->time: no idle animations    kept
  (0xca980)                             after a secondary shot
tripmine.cpp CTripmine::SecondaryAttack needs more than ammo_used_per_secondary_mine - 1      kept
  (0x11f8c0)                            mines but takes one
tripmine.cpp CTripmineGrenade::MakeBeam beam colour from one global set at placement, read   kept
  (0x11ea20)                            at power-up: crossed colours between mines
tripmine.cpp CTripmineGrenade::MakeBeam mode 0 (a map mine before any player's) uses          changed (0,0,0)
                                        uninitialized colours
satchel.cpp CSatchelCharge::            the proximity fuse tests only the first entity in     kept
  SatchelThink (0xff8e0)                range
rpg.cpp CRpgRocket::FollowThink         homing takes the shooter and dying entities too       kept
  (0xfe1a0)
ggrenade.cpp CGrenade::ShootFlare       dmg 5 gives a negative TE_EXPLOSION scale             kept
  (0xc36b0)
rpg.cpp CRpgRocket::RocketTouch         m_cActiveRockets drops only on a touch: a rocket that changed (halflife-
  (0xfdf00)                             blew up in flight locks the reload while the laser is updated's
                                        on                                                    destructor)
gauss.cpp CGauss::SecondaryAttack       the ammo burn can take uranium to -1                  changed (halflife-
  (0xc0b40)                                                                                   updated's)
gauss.cpp CGauss::Fire (0xc14c0)        a reflected beam that hits its shooter keeps the last changed (halflife-
                                        hitgroup: a headshot on oneself                       updated's, Steven)
ggrenade.cpp CGrenade::Explode          pulls out of the wall by (dmg - 24) * 0.6: a big      changed (halflife-
  (0xc2270)                             explosion can sit on the far side of a thin wall      updated's 0.6,
                                                                                              Steven)
---- world
triggers.cpp PlayCDTrack (0x119910)     the "Track %d out of range" alert passes no argument  kept
talkmonster.cpp CTalkMonster::          no alive test: a dying talk monster can start a       changed (halflife-
  PlaySentence (0x113420)               sentence                                              updated's)
func_break.cpp CBreakable::Die          breaks again when hit twice before the remove; abs()  changed (halflife-
  (0xb7710)                             on the float health                                   updated's)
sound.cpp CEnvSound::Think (0x10a6b0)   sends the room type once and never resets it on a     changed (halflife-
                                        level change                                          updated's)
monsters.cpp CBaseMonster::ChangeYaw    turn rate follows the server's frame rate             changed (halflife-
  (0xd5870), flyingmonster.cpp                                                                updated's)
  CFlyingMonster::ChangeYaw (0xb5980)
talkmonster.cpp tasks, effects.cpp      read edict 1 even when the slot is free (stale        changed (halflife-
  CBlood::BloodPosition, CMessage::Use, origin, or a NULL Instance in CTriggerCamera::Use)    updated's)
  triggers.cpp ChangeLevelNow
```

## Dead code

`CGraph::FLoadBotNav` (its path ends in a space, so it never opens on Linux, and nothing calls it),
with `slurp_array` and `parse_nav_line`; `CBaseBot::TakeDamage` (its extra argument keeps it from
overriding, and nothing calls it); ThinkStart's `MarkLocationFavorable`, which runs only when the
steer precedence it just set to 0 is non-zero, so bots only mark cells unfavourable; BotConnect's
"no free client slots" branch (tests a pointer already known non-NULL); `UTIL_GetNextBestWeapon`
(no callers); UseBotCycle's copy of the last line (never read); CBaseBotFightStyle's attack mode and
CBaseBotGlobals' "boot_camp" (written, never read); `didhaveexploderune` (only ever cleared);
CGrappleBolt's `dragger` (written, never read) and its drag flags and `dragging` (unused); CRune's
+0x74 (allocated, never touched); StartObserver's FL_MONSTER and FL_FLY clears right after
`pev->flags = FL_SPECTATOR`.
