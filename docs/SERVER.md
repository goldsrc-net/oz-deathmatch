# Server reference

Everything Oz Deathmatch 3.02 adds to a Half-Life server's console and config: its cvars, its
commands, its impulses and the files it reads and writes. What each feature does in play is in
[GAMEPLAY.md](GAMEPLAY.md). Where the rebuild differs from 3.02 itself,
[reference/DEVIATIONS.md](../reference/DEVIATIONS.md) says so.

`scripts/check_docs.py` fails if a cvar or command the code registers is missing from this page.

## Cvars

`GameDLLInit` registers 386 cvars, then runs `skill.cfg` and OZ's seven cfg files in this order:
`cfg/multiplayer.cfg`, `cfg/weapons.cfg`, `cfg/player.cfg`, `cfg/grapple.cfg`, `cfg/colors.cfg`,
`cfg/special.cfg`, `cfg/rhobot.cfg`. The cvars below are grouped by the file that sets them, in
registration order. "Default" is the value compiled in; "3.02" is the value the shipped cfg file
sets where it differs, and `-` means the shipped files don't set it. A server running the 3.02
content gets the 3.02 column. None of OZ's own cvars are server cvars (`FCVAR_SERVER`), so none
show in server browsers.

"Read by" names the function that reads a cvar where the code doesn't make its effect plain. "Not
read" means the cvar is registered and nothing reads it.

### multiplayer.cfg

```
cvar                 default  3.02                     what it does
show_player_message  0        -                        not read
GameDesc             0        -                        not read (the browser's game name is fixed:
                                                       "Half-Life ozdm", "Half-Life oztp" in teamplay)
mp_picture           0        -                        1: skip the join messages and the MOTD
bm_cheatdetect       1                                 1: every 5 s, probe one player for the HLHack
                                                       aimbot and the XQZ2 wallhack; three unanswered
                                                       probes name him to all and in the log
logchat              0        -                        1 (the default) logs no chat; 0 logs it (inverted,
                                                       reference/ORIGINAL_BUGS.md)
logrune              0        -                        1: log "Discarded the" lines for droprune
oz_spectators        1                                 1: the observer's jump toggles free roaming and
                                                       chasing a player; 0: roaming only
oz_advspec           1                                 1: send the observer mode to the client (needs
                                                       OZ's modified valve/delta.lst); 0: server side only
oz_link              " "      www.yourlink.com         second line of the join message
oz_line1             " "      Thank you for Playing    a line shown to each player after joining
oz_line2             " "      "  Oz DeathMatch 3"      the line after it
```

The join messages `mp_picture 1` skips are the "Oz Deathmatch v3.02" banner with `oz_link`, the
help hint, `oz_line1` and `oz_line2`, and the observer hint.

### weapons.cfg

Auto switching and weapon weights (higher wins when auto switching on pickup):

```
cvar             default  3.02  what it does
auto_switch      1              above 0: switch to a picked-up weapon that weighs more than the one
                                held (never with the Super Crowbar Rune)
weight_crowbar   0
weight_glock     10
weight_357       15
weight_mp5       15
weight_shotgun   0        15
weight_xbow      10
weight_rpg       20
weight_gauss     20
weight_egon      20
weight_hivehand  10
weight_hgrenade  5
weight_snark     5
weight_satchel   -10
weight_tripmine  -10
```

Disabling weapons. Each removes the weapon entity as it spawns, so it is neither in the map nor
given at spawn; `disable_m203` instead leaves AR grenade pickups inert:

```
cvar              default  3.02  what it does
disable_glock     0              above 0: no glock
disable_357       0              above 0: no 357
disable_mp5       0              exactly 1: no MP5
disable_m203      0              1 or more: ammo_ARgrenades pickups stay in the map but never set up
                                 (no model, no touch)
disable_shotgun   0              exactly 1: no shotgun
disable_xbow      0              above 0: no crossbow
disable_rpg       0              above 0: no RPG
disable_gauss     0              exactly 1: no gauss
disable_egon      0              exactly 1: no egon
disable_hivehand  0              above 0: no hivehand
disable_hgrenade  0              above 0: no hand grenades
disable_satchel   0              above 0: no satchels
disable_tripmine  0              above 0: no tripmines
disable_snark     0              above 0: no snarks
```

What a player spawns with. The crowbar always comes; a map's `game_player_equip` replaces all of
this:

```
cvar             default  3.02  what it does
start_glock      1              above 0: spawn with it
start_357        0
start_mp5        0        1
start_shotgun    0
start_xbow       0
start_rpg        0
start_gauss      0
start_egon       0
start_hivehand   0
active_weapon    4              the weapon deployed at spawn, by id: 1 crowbar, 2 glock, 3 357,
                                4 MP5, 6 crossbow, 7 shotgun, 8 RPG, 9 gauss, 10 egon, 11 hivehand,
                                12 hand grenade, 13 tripmine, 14 satchel, 15 snark
start_9mm        50       100   rounds, up to max_9mm
start_shells     0        24
start_357rounds  0        10
start_m203       0
start_uranium    0
start_rockets    0
start_xbowbolts  0
start_hgrenade   0              one weapon_handgrenade each
start_satchel    0              one weapon_satchel each
start_tripmine   0              one weapon_tripmine each
start_snark      0              one weapon_snark each
```

Ammo limits, what an ammo pickup gives, what a weapon pickup comes with, and magazine sizes:

```
cvar             default  3.02  what it does
max_uranium      100            most a player can carry
max_9mm          250
max_shells       125
max_357rounds    36
max_m203         10
max_rockets      5
max_satchels     5
max_tripmines    5
max_hgrenades    10
max_snarks       10
max_hornets      8              also where the hivehand recharges to
max_xbowbolts    50
item_uranium     20             ammo_gaussclip
item_chainbox    200            ammo_9mmbox
item_shells      12             ammo_buckshot
item_357rounds   6              ammo_357
item_m203        2              ammo_ARgrenades
item_glockclip   17             ammo_9mmclip
item_mp5clip     50             ammo_9mmAR
item_rpgclip     2              ammo_rpgclip
item_xbowclip    5              ammo_crossbow
ammo_mp5         50       20    with the weapon
ammo_rpg         2
ammo_glock       17
ammo_shotgun     12
ammo_gauss       20
ammo_egon        20
ammo_hgrenade    5
ammo_satchel     1
ammo_tripmine    1
ammo_xbow        5
ammo_m203        0        -     not read
ammo_snark       5
ammo_hivehand    8
ammo_357         6
clip_glock       17             magazine size; also what the Super Glock Rune gives
clip_357         6
clip_mp5         50       20
clip_shotgun     8
clip_xbow        5
```

Damage. Bullet damage is multiplied by the hitgroup cvars in player.cfg:

```
cvar                   default  3.02  what it does
damage_crowbar         25             a first swing; later swings in a row do half
damage_flying_crowbar  25       -     a thrown crowbar does twice this, or this with the Super
                                      Crowbar Rune (sic)
damage_glock           12             a glock round
damage_sglock          300            the zoomed glock with the Super Glock Rune (always gibs)
damage_glock_sniper    12       35    the zoomed glock
damage_357             25       55
damage_357_sniper      25       80    the zoomed 357
damage_mp5             12       20
damage_mp5_sniper      12       100   the zoomed MP5 (mp5_sniper 1)
damage_shotgun_pellet  20             each pellet
damage_gauss           20             the primary shot
damage_egonwide        20             the wide beam (secondary)
damage_rpg             120      100   the rocket
damage_rpg_homing      50       40    not read; a homing rocket does damage_rpg
damage_m203            100            an AR grenade
damage_xbow            20             a bolt that hits
damage_xbowexplode     40             the explosion of an unzoomed bolt
damage_xbow_sniper     120            a zoomed bolt
damage_gaussmax        200            a full gauss charge
damage_egonnarrow      10             the narrow beam (primary)
damage_hgrenade        100            a hand grenade
damage_hornet          10             a hornet
damage_satchel         120            a satchel charge
damage_tripmine        150            a tripmine
damage_snarkbite       10             a snark's bite
damage_snarkpop        30             a snark bursting
```

Recoil, the view kick up in degrees per shot:

```
cvar             default  3.02  what it does
recoil_glock     2        1
recoil_mp5       2        3     three times with the Haste Rune; zoomed, a random kick of up to
                                this either way
recoil_m203      10             an AR grenade
recoil_xbow      2
recoil_rpg       5
recoil_shotgun   5
recoil_shotgun2  10             the double shot
recoil_357       10             primary and zoom
recoil_gauss     2              the primary shot
recoil_hivehand  2              not read
```

Rates of fire, seconds between shots. The Haste Rune halves each of them (the egon has none):

```
cvar                 default  3.02  what it does
cycle_crowbar_first  .5             after a miss, and between throws
cycle_crowbar_after  .25            after a hit
cycle_glock          .3
cycle_sglock         1.5      -     the zoomed glock with the Super Glock Rune
cycle_glock2         .75      0.2   the zoomed glock
cycle_mp5            .1
cycle_mp5_sniper     .3       0.7   the zoomed MP5
cycle_357            .75
cycle_3572           1.25     1.0   the zoomed 357
cycle_shotgun        .75            under 0.3, the server plays the shot's animation and sound
cycle_shotgun2       1.5      1.2   the double shot
cycle_gauss          .2             between primary shots
cycle_m203           1              between AR grenades
cycle_xbow           .75
cycle_xbow_sniper    .75            the zoomed crossbow
cycle_hivehand       .25            primary
cycle_hivehand2      .1             secondary
```

Projectile speeds and reload times:

```
cvar                       default  3.02  what it does
speed_hgrenade             4              throw speed per degree of upward aim (capped at 500)
speed_m203                 800            AR grenade launch speed
speed_rpg_water            300            the rocket's top speed under water
speed_rpg_air              2000           the rocket's top speed in air
speed_satchel              274            satchel throw speed
speed_xbow_air             2000           bolt speed in air
speed_xbow_water           1000           bolt speed under water
speed_hornet_red           600      800   the red hornet (two in five, at random)
speed_hornet_orange        800      600   the orange hornet (the rest)
reload_glock               1.5            seconds (halved with the Haste Rune)
reload_357                 2
reload_mp5                 1.5      1.0
shotgun_shells_per_reload  1        4     shells put in per insert; the reserve is charged that many
                                          whatever it holds, so it can go negative
reload_xbow                4.5      1.5
reload_rpg                 2
```

Weapon modes and special weapons:

```
cvar                          default  3.02  what it does
mp5_tracers                   2              every Nth MP5 round is a tracer; 0 none
mp5_spread                    6        3     the MP5's cone, 1 (tight) to 10 (wide); outside 1-10 a
                                             shot fires nothing but spends the round
mp5_sniper                    0        1     exactly 1: secondary zooms (zoomed: 1-degree cone,
                                             damage_mp5_sniper, cycle_mp5_sniper) instead of the
                                             grenade launcher
mp5_silence                   0              exactly 1: silenced sound; any other non-zero: the quiet
                                             volume only
glock_sniper                  0              not read: the glock's secondary always zooms in 3.02
glock_silence                 0              above 0: the silencer model and quiet shots
shotgun_pellets               4        5     pellets per primary shot
shotgun2_pellets              8        10    pellets per double shot
snark_health                  10             a snark's health
snark_time                    15             seconds before a snark bursts on its own
gauss_maxchargetime           10       3     seconds of charging before the gauss overcharges and
                                             zaps its holder
grenade_time                  3              hand grenade fuse in seconds, from the pin pull
flare_time                    10             a flare's fuse; 0 disables the hand grenade's secondary
                                             (the flare)
water_weapons                 0        1     1: the 357, MP5, shotgun, gauss and egon fire under water
zoom_fov_glock                40       50    field of view when zoomed
zoom_fov_mp5                  30       10
zoom_fov_xbow                 20
zoom_fov_357                  40
satchel_charge_time           5        3     seconds before a satchel's proximity fuse arms
satchel_detection_area        128            the proximity fuse's radius
allow_proximity_satchel       0              1: an armed satchel blows when a player comes within
                                             satchel_detection_area (its owner too)
ammo_used_per_secondary_mine  5        2     the secondary (invisible beam) tripmine needs more than
                                             this minus one mines, and uses one
shake_enable                  1        0     1: every multiplayer blast that hurts a player shakes
                                             the screens around him
shake_amp                     25.0           shake strength
shake_freq                    150.0          shake frequency
shake_dur                     1.0            shake duration in seconds
shake_radius                  850            shake radius
rpg_homing                    0        -     1: rockets steer for anything that takes damage within
                                             1000 units instead of the laser spot
```

### player.cfg

```
cvar                        default  3.02  what it does
player_id                   1              above 0: aiming at a player center prints his name and
                                           rune; also needed by the Identify Rune
healthcharger_capacity      50       30    a health charger's juice
suitcharger_capacity        30             a suit charger's juice
healthcharger_rechargetime  60             not read (multiplayer chargers recharge after a fixed
                                           60 s)
suitcharger_rechargetime    30             not read (fixed 30 s)
healthcharger_amount        1        2     health per charger tick
suitcharger_amount          1        2     armor per charger tick
start_health                100            health at spawn
start_charge                0        10    armor at spawn
max_health                  100            the most health (and the regeneration rune's cap)
max_charge                  100            the most armor, from batteries, chargers and the rune
battery                     15             armor from item_battery
healthkit                   15             health from item_healthkit
damage_falling              0              1 or more: fall damage; otherwise a hard landing does 0
start_longjump              0              above 0: spawn with the long jump module
player_air                  12       15    seconds of air under water (item_airtank refills to this)
player_speed                300            a player's top speed; runes scale it
head_damage                 2        4     damage multiplier for a head hit, players and monsters
chest_damage                1        2
stomach_damage              1        2
arm_damage                  1
leg_damage                  1
blood_amount                2        1     blood drips in multiplayer are multiplied by this
```

### grapple.cfg

```
cvar                   default  3.02  what it does
grapple_airspeed       1500           hook flight speed in air (the shipped cfg says max 2000)
grapple_waterspeed     1000           hook flight speed under water
grapple_dragspeed      700            pull speed beyond 150 units of the hook (the cfg says max 1000)
damage_grapple         5              damage when the hook hits something that takes damage
disable_grapple        0              above 0: +hook does nothing
grapple_glow           0              exactly 1: a glowing white ball on the hook until hanging
enable_grapple_sounds  1              0: the hook hits silently
enable_skygrapple      0              1: the hook holds on the sky
```

### colors.cfg

```
cvar                      default  3.02  what it does
random_grapple_color      1              not 0: the hook's trail takes a random colour
random_tripmine_color     0              exactly 1: each primary tripmine's beam takes a random colour
color_red_grapple         50       100   the hook's trail, when not random
color_green_grapple       100
color_blue_grapple        200      255
color_red_rpg             224            the rocket's trail
color_green_rpg           255
color_blue_rpg            224
color_red_hornet1         179            the red hornet's trail
color_green_hornet1       39
color_blue_hornet1        14
color_red_hornet2         255            the orange hornet's trail
color_green_hornet2       128
color_blue_hornet2        0
color_red_tripmine        0              a primary tripmine's beam ("set all to 0 and you have
color_green_tripmine      214      55    invisible tripmines", the shipped cfg says)
color_blue_tripmine       198      50
color_red_gaussprimary      255    -     not read
color_green_gaussprimary    128    -     not read
color_blue_gaussprimary     0      -     not read
color_red_gausssecondary    255    -     not read
color_green_gausssecondary  255    -     not read
color_blue_gausssecondary   255    -     not read
```

### special.cfg

```
cvar                          default             3.02  what it does
number_of_crowbar_runes       1                         how many of each rune the map gets when the
number_of_haste_runes         1                         first player spawns; 0 disables that rune
number_of_superglock_runes    1                         (and keeps the random rune from turning
number_of_boobytrap_runes     1                         into it)
number_of_cloak_runes         0
number_of_protection_runes    1
number_of_regen_runes         1
number_of_vampire_runes       1
number_of_superhook_runes     1
number_of_lowgravity_runes    1
number_of_superspeed_runes    1
number_of_teleport_runes      0
number_of_frag_runes          0
number_of_identify_runes      0
number_of_random_runes        1
rune_model                    7                   1     the rune and item_suit model: 1 HEV suit,
                                                        2 chumtoad, 3 crystal, 4 long jump module,
                                                        5 oxygen pack, 6 headcrab, 7 customrunemodel
customrunemodel               oz_custom_rune.mdl        the model for rune_model 7, in models/
rune_scheme                   0                         1: the alternative rune colours
respawntime_rune              30                        seconds before an untaken rune moves to
                                                        another deathmatch start
frag_rune_value               1                   2     extra frags per kill with the Frag Rune
regen_rune_health             1                   2     health per regeneration tick
regen_rune_armor              1                         armor per regeneration tick
superhook_rune_damage         25                  50    the hook's damage with the Super Hook Rune
lowgravity_rune_value         .2                        gravity with the Low Gravity Rune
max_health_with_vampire_rune  150                       the Vampire Rune heals up to this
damage_boobytrap_rune         150                 250   the Booby Trap Rune's blast
night_ops                     0                         above 0: every light style at light_level,
                                                        and flashlights don't drain. Reset to 0 at
                                                        each map start, so only maps/<map>.cfg can
                                                        turn it on
light_level                   4                   5     0 to 4 give styles "a" to "e"; anything
                                                        else "f" (the cfg says 1 dark, 5 lighter)
respawntime_item              30                  15    seconds before an item respawns
respawntime_ammo              20                  15    seconds before ammo respawns
respawntime_weapon            20                  15    seconds before a weapon respawns
points_per_kill               1                         frags for a kill
points_per_suicide            1                   2     frags lost for a suicide
points_per_worldkill          1                         frags lost when the world kills you
points_per_teamkill           1                         frags lost for killing a teammate
points_per_death              1                         not read
```

The shipped special.cfg also sets `points_per_flag_capture`, `points_per_flag_steal` and
`poinst_per_flag_return`, leftovers from 1.7's CTF. 3.02 has no such cvars, so the server prints
an unknown-command line for each.

### rhobot.cfg

```
cvar                  default  3.02  what it does
bot_blood             0        3     bot blood colour: 0 random, 1 green, 2 blue, 3 red, 4 yellow
bot_speed             300            a bot's top speed
bot_number            0              bots that stay in the server whatever the player count
bot_skill             50       35    bot skill, 0-100
bot_skill_aa          1        0     1: every 12 s, raise bot_skill by 5 (to bot_aa_max) when humans
                                     have the better frag/death ratio, lower it by 1 when bots do
bot_jump              50             how often bots jump, 0-100
bot_mercy             1              1: bots spare a human with half their frags or fewer, and
                                     sometimes one with up to three quarters
bot_chat              30       -     not read
bot_enable            0        -     not read
bot_debug             0              not read
bot_dontchat          0        1     1: bots never chat
bot_observer          0              not 0: bots ignore human players
bot_name              Rho-Bot  -     the next bot's name, set by its bots/<name>.cfg
bot_model             robo     -     its player model
bot_top_color         255      -     its colours
bot_bottom_color      255      -
bot_trait_jump        50       -     its traits, 0-100, set by its bots/<name>.cfg
bot_trait_accuracy    50       -
bot_trait_aggression  50       -
bot_trait_chat        50       -
bot_trait_perception  70       -
bot_trait_reflexes    70       -
bot_wp_crossbow       50       -     its weapon preferences, 0-100, set by its bots/<name>.cfg
bot_wp_crowbar        50       -
bot_wp_egon           50       -
bot_wp_gauss          50       -
bot_wp_glock          50       -
bot_wp_handgrenade    50       -
bot_wp_hornetgun      50       -
bot_wp_mp5            50       -
bot_wp_python         50       -
bot_wp_rpg            50       -
bot_wp_satchel        50       -
bot_wp_shotgun        50       -
bot_wp_snark          50       -
bot_wp_tripmine       50       -
bot                   ""       -     a command channel: set it to "addbot", "removebot <name>" or
                                     "removebots" and the server runs it within a second, then
                                     clears it
min_clients           0              add bots until humans plus bots reach this; each joining human
                                     replaces one
min_bot               0        -     not read
runfolder             valve    oz    the folder holding bots/ and nav/
bot_aa_max            99       50    the highest bot_skill auto adjust goes
```

Bots join only when `bot_number` or `min_clients` is above 0, from 10 s into the map, one every
10 s.

### Stock Half-Life cvars

Registered as in Valve's SDK, some with OZ's defaults or meaning:

```
cvar              default            3.02   how
mp_teamplay       0                         teamplay; in teamplay the observer modes are the SDK's
                                            plain parked view
mp_fraglimit      0                  30
mp_timelimit      0                  30
mp_friendlyfire   0
mp_falldamage     0                         1: realistic fall damage, but only with damage_falling 1
                                            or more is any of it dealt
mp_weaponstay     0
mp_forcerespawn   1                  0
mp_flashlight     0                  1
mp_autocrosshair  1                  0
decalfrequency    30                 10
mp_teamlist       hgrunt;scientist
mp_teamoverride   1                  -
mp_defaultteam    0                  -
mp_allowmonsters  1                         OZ's default and unlogged (Valve's is 0)
displaysoundlist  0                  -
mp_fragsleft      0                  -      read-only, for server browsers
mp_timeleft       0                  -      read-only, for server browsers
mp_chattime       10                 -      seconds of intermission
```

Of the skill cvars, OZ registers only `sk_turret_health1-3`, `sk_miniturret_health1-3`,
`sk_sentry_health1-3`, `sk_12mm_bullet1-3`, `sk_9mmAR_bullet1-3`, `sk_9mm_bullet1-3`,
`sk_hornet_dmg1-3`, `sk_suitcharger1-3`, `sk_battery1-3`, `sk_healthcharger1-3`,
`sk_healthkit1-3` and `sk_scientist_heal1-3`, all defaulting to 0 and set by `valve/skill.cfg`.
Players' bullets, batteries, healthkits and chargers use OZ's cvars above instead. The other
`sk_*` lines in skill.cfg are unknown commands here (reference/DEVIATIONS.md, Cvars).

### Added by Half-Life Updated, not OZ

OZ 3.02 doesn't have these; the rebuild keeps them from its base.

```
cvar                          default  what it does
allow_spectators              0.0      not read: OZ's own observer mode is used
sv_allowbunnyhopping          0        bunny hopping (halflife-updated's speed cap applies at 0)
sv_busters                    0        Half-Life's "busters" mode, off
sv_pushable_fixed_tick_fudge  15       func_pushable's tick fudge
sv_alltalk                    0        the voice manager: 1 lets everyone hear everyone
voice_serverdebug             0        voice manager debug output
```

## Commands

Typed in the client console (or bound). Bots run none of them.

### Everyone

```
command         what it does
+hook, -hook    fire the grappling hook and hold on; release (disable_grapple, grapple.cfg)
droprune        drop your rune; it goes back into the map at a deathmatch start
runestatus      show what your rune does
userune         with the Teleport Rune, teleport to a spawn point (at most every 5 s); with the
                Booby Trap Rune, blow yourself up
help            the command list, in the console and on screen
more            a page of client tips (voice, fps_modem, net_graph)
cam             how the observer mode's buttons work
info            map name, time limit, frag limit and time remaining; also showtime,
                showmapname and timeleft
intN=1          on everyone's screen: "The Originator and Programmer of Oz Deathmatch has
                entered the server"
intN=2          on everyone's screen: "Attention! Player's attention is required."
intN=3          on everyone's screen: "NOTICE! This server is monitored For Inappropriate Conduct."
intN=4          on everyone's screen: "Behold...For a Paradigm Lozt Regular walks among you!"
bot_fov         the answer a clean client gives bm_cheatdetect's aimbot probe
enable_wallhack the answer a clean client gives its wallhack probe
```

The `intN=` messages need no rights; anyone can send them.

Stock commands kept: `say`, `say_team`, `drop`, `use`, `weapon_*`, `lastinv`, `give` and `fov`
(both with `sv_cheats`), and `menuselect` (teamplay's team menu).

### Listen server only

Refused from a dedicated server's clients ("not allowed from client"); there, set the `bot` cvar
instead.

```
command           what it does
addbot            add one to bot_number (at most 32 players)
removebot <name>  kick the player with that name, bot or not, and take one from bot_number
removebots        set bot_number to 0
```

### Server console

None of OZ's own besides the `bot` cvar. Half-Life Updated adds `sv_load_all_maps` and
`sv_stop_loading_all_maps` (load every map in turn, to build node graphs); 3.02 doesn't have them.

### Added by Half-Life Updated, not OZ

Answered so a current client works: `fullupdate` (re-sends the HUD), `closemenus` (ignored), and the
voice manager's `vban` and `VModEnable`.

## Impulses

OZ adds none; the SDK's are kept as they are. 99 (logo), 100 (flashlight), 201 (spray) and 204
(resend the HUD) work for everyone. With `sv_cheats 1`: 76 (a human grunt, after a restart), 101
(all weapons), 102 (gibs), 103 (monster AI report), 104 (global state dump), 105 (player makes no
sound for monsters), 106 (classname of what you look at), 107 (texture name), 195-197 and 199
(node graph displays), 202 (blood), 203 (remove what you look at).

## Files

In the game directory unless said otherwise:

```
file                      read or written
cfg/multiplayer.cfg       run once at startup, in this order, by GameDLLInit (Cvars, above)
cfg/weapons.cfg
cfg/player.cfg
cfg/grapple.cfg
cfg/colors.cfg
cfg/special.cfg
cfg/rhobot.cfg
skill.cfg                 run first; OZ ships none, so valve/'s is used
server.cfg                run by the engine at startup (servercfgfile)
maps/<map>.cfg            run at each map start, before the light styles and night_ops are read
mapcycle.txt              the map cycle (mapcyclefile, the engine's)
motd.txt                  the MOTD shown at join (motdfile); maps/<map>.motd replaces it on that map
player.txt                sent in the MOTD window to a player who kept a default name ("Player", or
                          some nine-letter names), who is also renamed "[PNP]<random name>"
botcycle.txt              in the server's working folder (next to hlds_linux), not the game
                          directory: one bot cfg name per line; the shipped file is CRLF
<runfolder>/bots/<n>.cfg  run for each bot that joins, in botcycle.txt order: its bot_name,
                          bot_model, colours, bot_trait_* and bot_wp_* values
<runfolder>/nav/<map>.nav written at each map change: the bots' navigation array, one "x.y.z=value"
                          line per 32-unit cell (the folder must exist and be writable)
maps/graphs/<map>.nod     the node graph, built and written by the engine's node code as in Half-Life
                          (<map>.nod64 on a 64-bit build); OZ's CGraph also carries the bots'
                          navigation array, so it is saved inside the .nod as well
```

The 3.02 package ships `nav/` and `bots/` in `oz/`, with `runfolder "oz"` in rhobot.cfg to match.
