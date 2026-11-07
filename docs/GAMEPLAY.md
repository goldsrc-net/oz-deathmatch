# How Oz Deathmatch differs from Half-Life

Everything Oz Deathmatch 3.02 changes about Half-Life deathmatch, as the rebuild plays it. The cvars
and commands named here are in [SERVER.md](SERVER.md), with their defaults and the values 3.02's
cfg files ship. Where the rebuild differs from 3.02 itself,
[reference/DEVIATIONS.md](../reference/DEVIATIONS.md) says so, and
[reference/ORIGINAL_BUGS.md](../reference/ORIGINAL_BUGS.md) records OZ's own bugs and which were
kept. The per-file detail is in [reference/dossiers/](../reference/dossiers/).

OZ has no client library: it runs on Half-Life's own client, and players download nothing. All of
the below is server side. Nearly every number in it is a cvar, set in OZ's seven `cfg/*.cfg` files
and changeable mid-game.

## Runes

Runes are items that give the player who carries one a power until he dies, drops it (`droprune`)
or leaves. They look like the HEV suit (or the model `rune_model` picks) in a coloured shell, and
bounce around a little when they appear. A player carries one rune at a time.

- The map gets its runes when the first player spawns: `number_of_<rune>_runes` of each, every one
  at a random deathmatch start. 0 turns that rune off.
- A rune nobody takes moves to another deathmatch start every `respawntime_rune` seconds.
- A dropped rune, or the rune of a player who dies, goes back to a random deathmatch start.
- Picking one up shows its name and what it does; `runestatus` shows it again. The rune is in the
  HUD and in the log ("Picked up the", "Died while wearing the"; "Discarded the" with `logrune 1`).
- `rune_scheme 1` swaps in a second set of colours, to confuse players who know the first.

```
rune          shell (scheme 0)  what it does
Frag          white             frag_rune_value extra frags per kill (not for teamkills)
Regeneration  green             health and armor rise by regen_rune_health and regen_rune_armor
                                every 30/72 s, up to max_health and max_charge
Super Crowbar red               the crowbar (given if missing) kills with any swing (1000 damage);
                                throwing it doesn't lose it; no auto switch away from it
Haste         blue              every weapon's refire time halved, and most reloads; the MP5
                                spreads to 10 degrees with three times the kick; speed 0.8 times
                                player_speed
Cloaking      cyan              nearly invisible, more visible the faster you move (up to a set
                                limit); firing shows you for 10 s. Bots can't see a cloaked player
                                who moves slowly
Super Hook    orange            the hook flies and pulls at 2000 and hits for superhook_rune_damage
Protection    pale green        half damage, after armor
Low Gravity   yellow            gravity lowgravity_rune_value, speed 1.25 times player_speed
Vampire       purple            half the damage you deal to others comes back as health, up to
                                max_health_with_vampire_rune
Booby Trap    pink              you explode when killed (damage_boobytrap_rune), unless you killed
                                yourself; userune blows you up on purpose
Super Glock   dark grey         the glock (given if missing) and a clip of ammo; its shots cost
                                nothing, and zoomed it fires an exploding round (damage_sglock)
                                that always gibs, every cycle_sglock seconds with a heavy kick
Super Speed   light blue        speed sv_maxspeed
Identify      lime              aiming at a player shows his name, health, armor, distance and
                                rune (needs player_id on)
Teleport      violet            userune teleports you to a spawn point, at most every 5 s (the
                                rune's own text says 10)
Random        changing          its colour changes every 5 s; it becomes one of the enabled runes
                                when picked up, and goes back as a random rune when dropped
```

## The grappling hook

`+hook` fires an offhand hook (bind it: `bind x +hook`); `-hook` lets go. `disable_grapple 1`
turns it off.

- It flies at `grapple_airspeed` (`grapple_waterspeed` under water). On a wall it pulls you in at
  `grapple_dragspeed`, slows to `player_speed` inside 150 units, and inside 50 you hang there.
- It holds on the sky only with `enable_skygrapple 1`.
- A hook that hits something hurts it (`damage_grapple`).
- Its trail is a random colour by default (`random_grapple_color`), or `color_*_grapple`.
  `grapple_glow 1` lights the hook itself.
- Fired while ducking, the hook sits 18 units higher than where it hit (an original bug, kept).

## Weapons

Every weapon keeps its Half-Life model and slot. Damage, refire times, reload times, magazine sizes,
ammo limits, what pickups give, recoil and projectile speeds are all cvars (SERVER.md,
weapons.cfg), and any weapon can be taken out of the game (`disable_*`). With
`water_weapons 1` the 357, MP5, shotgun, gauss and egon fire under water.

```
weapon        primary                          secondary
crowbar       swing (damage_crowbar; half for  throw it (twice damage_flying_crowbar). You lose
              later hits in a row)             it, and a new one lies where it lands; with the
                                               Super Crowbar Rune you keep it
glock         fire, 4-degree cone              zoom (zoom_fov_glock). Zoomed: quiet, no muzzle
                                               flash, a 1-degree sniper round
                                               (damage_glock_sniper) every cycle_glock2 s
357           fire, 2-degree cone              zoom (zoom_fov_357). Zoomed: 1 degree, a tracer,
                                               damage_357_sniper every cycle_3572 s
MP5           fire, the cone set by            AR grenade; with mp5_sniper 1 a zoom instead
              mp5_spread; a tracer every       (zoom_fov_mp5): 1-degree rounds of
              mp5_tracers rounds; silenced     damage_mp5_sniper
              with mp5_silence 1
shotgun       shotgun_pellets pellets in a     shotgun2_pellets pellets, two shells
              wide, flat spread
crossbow      a bolt that explodes             zoom (zoom_fov_xbow); zoomed it fires the sniper
              (damage_xbowexplode)             bolt (damage_xbow_sniper)
RPG           a laser-guided rocket; with      the laser on or off
              rpg_homing 1 it homes on
              anything that can be hurt within
              1000 units instead
gauss         a shot (damage_gauss)            charge, to damage_gaussmax; held past
                                               gauss_maxchargetime it zaps its holder. No charge
                                               under water
egon          the narrow beam (the SDK's       the wide beam
              secondary)
hivehand      hornets, as in Half-Life         the fast volley
hand grenade  a grenade (grenade_time fuse)    a flare: a weak grenade with a light that burns
                                               for flare_time; flare_time 0 disables it
satchel       throw a charge                   the radio
tripmine      a mine with a coloured beam      a mine with an invisible beam, needing more mines
              (color_*_tripmine, or random)    (ammo_used_per_secondary_mine)
snark         snarks, as in Half-Life          -
```

Also:

- **The glock, MP5 and egon fire on the server.** OZ replaced their client-predicted code with
  its own (sounds, brass, the egon's old beams). Half-Life's client still predicts the stock
  weapons for the shooter, so his own view of them is a little off, as it was in 2004. The shotgun,
  357 and gauss send their effects to the shooter as well under water or at short refire times.
- **Satchels** can be shot (1 health). With `allow_proximity_satchel 1` an armed charge (after
  `satchel_charge_time`) blows when a player, its owner included, comes within
  `satchel_detection_area`. A player can pick up more satchels while some are out.
- **Tripmine beams** take their colour when they power up, from the last mine anyone placed, so
  two mines placed close together can swap colours (an original bug, kept).
- **Blasts shake the screen** of nearby players with `shake_enable 1`.
- **Auto switching** goes by `weight_*`, and only with `auto_switch` on.

## Players

- **Spawning.** A player spawns with `start_health` and `start_charge`, the crowbar and the
  `start_*` weapons and ammo, holding `active_weapon`, at `player_speed`. A map's
  `game_player_equip` replaces the weapons.
- **Spawn protection.** For 120 ticks at 72 Hz (1.7 s) after spawning a player takes no damage
  and glows red. Pressing attack ends it at once.
- **Joining.** In deathmatch a new player starts as an invisible observer, chasing another player,
  with "Observer Mode - Fire or Use to escape - Jump to change view - Alt Fire changes Players" on
  screen. Attack or use spawns him. Then come the "Oz Deathmatch v3.02" banner with `oz_link`, a
  hint to type `help`, the server's `oz_line1` and `oz_line2`, and the MOTD (`mp_picture 1` skips
  all of that). The server also sets his console on and his forward and back speeds to
  `sv_maxspeed`.
- **Observer mode.** In deathmatch a dead player who doesn't respawn within 6 s becomes an
  observer. Jump switches between free roaming and chasing (chasing only with `oz_spectators 1`),
  attack2 picks the next player, and attack or use goes back into the game. `oz_advspec 1` sends
  the mode to the client (OZ's modified `delta.lst` was meant to go with it). In teamplay the
  observer is the SDK's plain parked view.
- **Player ID.** With `player_id 1`, aiming at a player prints his name and rune.
- **Locational damage.** Bullets are multiplied by `head_damage`, `chest_damage`,
  `stomach_damage`, `arm_damage` and `leg_damage`, on players and monsters, and the shooter gets
  "Head Shot!", "Chest Shot!" and so on.
- **No fall damage** unless `damage_falling 1`.
- **Health and armor.** `max_health`, `max_charge`, `healthkit`, `battery`, the chargers'
  capacity and rate, `player_air` (and the air tank) are cvars. Blood in multiplayer is scaled by
  `blood_amount`.
- **Default names.** A player still called "Player" is renamed "[PNP]" plus a random joke name, and
  sent `player.txt`.
- **Cheat detection** (`bm_cheatdetect 1`) probes clients for two 2003 cheats and names anyone
  who fails three times in a row.
- **Night ops.** A map cfg that sets `night_ops 1` turns every light down to `light_level`, and
  flashlights don't run down.

## Scoring

A kill is worth `points_per_kill` (plus `frag_rune_value` with the Frag Rune). A suicide costs
`points_per_suicide`, a death to the world `points_per_worldkill`, and in teamplay a teamkill
`points_per_teamkill`. `kill` doesn't trigger the Booby Trap Rune.

## Bots

OZ has RhoBot built in. Bots join when `bot_number` (bots that always stay) or `min_clients` (fill
the server to this many players; each human who joins replaces a bot) is above 0, from 10 s into
the map, one every 10 s. On a listen server `addbot`, `removebot <name>` and `removebots` work from
the console; on a dedicated server set the `bot` cvar to the same.

- **Personalities.** Each bot takes the next line of `botcycle.txt` (in the server's working
  folder) and runs `bots/<line>.cfg` from `runfolder`, which sets its name, model, colours,
  traits (jump, accuracy, aggression, perception, reflexes, chat) and weapon preferences.
- **Skill.** `bot_skill`, 0-100. With `bot_skill_aa 1` it goes up 5 every 12 s while humans have
  the better frag/death ratio (to `bot_aa_max`), and down 1 while bots do.
- **Behaviour.** Bots roam by a navigation array of 32-unit cells that grows where human players
  walk and shrinks where bots get stuck; it is saved to `nav/<map>.nav` at each map change. They
  pick up weapons and use every one, chat in combat unless `bot_dontchat 1`, spare weak humans with
  `bot_mercy 1`, and ignore humans altogether with `bot_observer`. `bot_blood` colours their blood.
- In 3.02 bots never go for ammo: the table that says which ammo they can carry is built before
  the `max_*` cvars exist, so every limit reads 0 (an original bug, kept).

## Maps

- **Per-map configs:** `maps/<map>.cfg` runs at each map start; the shipped ones are examples.
- **Map MOTD:** `maps/<map>.motd` replaces `motd.txt` on that map.
- **Monsters.** OZ 3.02 has none of the monsters in 20 of the SDK's files (barnacle, headcrab,
  grunts, scientists and the rest; reference/DEVIATIONS.md, Entities), so a map that places one
  gets "Can't init" for it.

## Entities OZ adds

`oz_rune` (made by the game, not placeable in practice), `grapple_bolt` (the hook) and
`flying_crowbar` (a thrown crowbar).

## Where the rebuild plays differently

From [reference/DEVIATIONS.md](../reference/DEVIATIONS.md), the changes a player can notice:

- **Frame rate no longer matters** for the regeneration rune, spawn protection or the join
  messages. 3.02 counted them per client command, so they ran faster the higher a player's frame
  rate; the rebuild runs them at 72 Hz, what a 2004 client got.
- **Weapons fire through `GetNextAttackDelay`** (Half-Life Updated's), which keeps a weapon's rate
  of fire even when a shot's due time falls between frames.
- **The glock reloads to its clip.** 3.02 asked for one more round than the clip, so a full glock
  with spare ammo replayed its reload on every press.
- **A reflected gauss beam that hits its shooter** does plain damage, not the last hitgroup's.
- **Explosions pull out of a wall by 0.6 units**, where 3.02 pulled big ones far enough to land
  on the other side of a thin wall.
- **server.cfg runs once,** at startup, where 3.02 also ran it at every map change.
- **The crashes 3.02 had** (runes on a map without deathmatch starts, bots stuck on a weaponbox,
  long bot names and the rest) are guarded.
