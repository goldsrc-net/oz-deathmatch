/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "client.h"
#include "game.h"
#include "filesystem_utils.h"

cvar_t displaysoundlist = {"displaysoundlist", "0"};

// multiplayer server rules
cvar_t fragsleft = {"mp_fragsleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED}; // Don't spam console/log files/users with this changing
cvar_t timeleft = {"mp_timeleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED};	 // "      "

// multiplayer server rules
cvar_t teamplay = {"mp_teamplay", "0", FCVAR_SERVER};
cvar_t fraglimit = {"mp_fraglimit", "0", FCVAR_SERVER};
cvar_t timelimit = {"mp_timelimit", "0", FCVAR_SERVER};
cvar_t friendlyfire = {"mp_friendlyfire", "0", FCVAR_SERVER};
cvar_t falldamage = {"mp_falldamage", "0", FCVAR_SERVER};
cvar_t weaponstay = {"mp_weaponstay", "0", FCVAR_SERVER};
cvar_t forcerespawn = {"mp_forcerespawn", "1", FCVAR_SERVER};
cvar_t flashlight = {"mp_flashlight", "0", FCVAR_SERVER};
cvar_t aimcrosshair = {"mp_autocrosshair", "1", FCVAR_SERVER};
cvar_t decalfrequency = {"decalfrequency", "30", FCVAR_SERVER};
cvar_t teamlist = {"mp_teamlist", "hgrunt;scientist", FCVAR_SERVER};
cvar_t teamoverride = {"mp_teamoverride", "1"};
cvar_t defaultteam = {"mp_defaultteam", "0"};
cvar_t allowmonsters = {"mp_allowmonsters", "1", FCVAR_SERVER | FCVAR_UNLOGGED}; // Oz: 1, and unlogged

cvar_t allow_spectators = {"allow_spectators", "0.0", FCVAR_SERVER}; // 0 prevents players from being spectators

cvar_t mp_chattime = {"mp_chattime", "10", FCVAR_SERVER};

cvar_t sv_allowbunnyhopping = {"sv_allowbunnyhopping", "0", FCVAR_SERVER};

//CVARS FOR SKILL LEVEL SETTINGS
//Turret
cvar_t sk_turret_health1 = {"sk_turret_health1", "0"};
cvar_t sk_turret_health2 = {"sk_turret_health2", "0"};
cvar_t sk_turret_health3 = {"sk_turret_health3", "0"};

// MiniTurret
cvar_t sk_miniturret_health1 = {"sk_miniturret_health1", "0"};
cvar_t sk_miniturret_health2 = {"sk_miniturret_health2", "0"};
cvar_t sk_miniturret_health3 = {"sk_miniturret_health3", "0"};

// Sentry Turret
cvar_t sk_sentry_health1 = {"sk_sentry_health1", "0"};
cvar_t sk_sentry_health2 = {"sk_sentry_health2", "0"};
cvar_t sk_sentry_health3 = {"sk_sentry_health3", "0"};

// WORLD WEAPONS
cvar_t sk_12mm_bullet1 = {"sk_12mm_bullet1", "0"};
cvar_t sk_12mm_bullet2 = {"sk_12mm_bullet2", "0"};
cvar_t sk_12mm_bullet3 = {"sk_12mm_bullet3", "0"};

cvar_t sk_9mmAR_bullet1 = {"sk_9mmAR_bullet1", "0"};
cvar_t sk_9mmAR_bullet2 = {"sk_9mmAR_bullet2", "0"};
cvar_t sk_9mmAR_bullet3 = {"sk_9mmAR_bullet3", "0"};

cvar_t sk_9mm_bullet1 = {"sk_9mm_bullet1", "0"};
cvar_t sk_9mm_bullet2 = {"sk_9mm_bullet2", "0"};
cvar_t sk_9mm_bullet3 = {"sk_9mm_bullet3", "0"};

// HORNET
cvar_t sk_hornet_dmg1 = {"sk_hornet_dmg1", "0"};
cvar_t sk_hornet_dmg2 = {"sk_hornet_dmg2", "0"};
cvar_t sk_hornet_dmg3 = {"sk_hornet_dmg3", "0"};

// HEALTH/CHARGE
cvar_t sk_suitcharger1 = {"sk_suitcharger1", "0"};
cvar_t sk_suitcharger2 = {"sk_suitcharger2", "0"};
cvar_t sk_suitcharger3 = {"sk_suitcharger3", "0"};

cvar_t sk_battery1 = {"sk_battery1", "0"};
cvar_t sk_battery2 = {"sk_battery2", "0"};
cvar_t sk_battery3 = {"sk_battery3", "0"};

cvar_t sk_healthcharger1 = {"sk_healthcharger1", "0"};
cvar_t sk_healthcharger2 = {"sk_healthcharger2", "0"};
cvar_t sk_healthcharger3 = {"sk_healthcharger3", "0"};

cvar_t sk_healthkit1 = {"sk_healthkit1", "0"};
cvar_t sk_healthkit2 = {"sk_healthkit2", "0"};
cvar_t sk_healthkit3 = {"sk_healthkit3", "0"};

cvar_t sk_scientist_heal1 = {"sk_scientist_heal1", "0"};
cvar_t sk_scientist_heal2 = {"sk_scientist_heal2", "0"};
cvar_t sk_scientist_heal3 = {"sk_scientist_heal3", "0"};

// END Cvars for Skill Level settings

cvar_t sv_pushable_fixed_tick_fudge = {"sv_pushable_fixed_tick_fudge", "15"};

cvar_t sv_busters = {"sv_busters", "0", FCVAR_SERVER};

// Oz Deathmatch 3.02: the cvars it adds, in GameDLLInit's order (reference/cvars.tsv: names, defaults
// and flags read from the original).
cvar_t oz_randomgrapplecolor = {"random_grapple_color", "1"};
cvar_t oz_randomtripminecolor = {"random_tripmine_color", "0"};
cvar_t oz_colorredgrapple = {"color_red_grapple", "50"};
cvar_t oz_colorgreengrapple = {"color_green_grapple", "100"};
cvar_t oz_colorbluegrapple = {"color_blue_grapple", "200"};
cvar_t oz_colorredrpg = {"color_red_rpg", "224"};
cvar_t oz_colorgreenrpg = {"color_green_rpg", "255"};
cvar_t oz_colorbluerpg = {"color_blue_rpg", "224"};
cvar_t oz_colorredhornet1 = {"color_red_hornet1", "179"};
cvar_t oz_colorgreenhornet1 = {"color_green_hornet1", "39"};
cvar_t oz_colorbluehornet1 = {"color_blue_hornet1", "14"};
cvar_t oz_colorredhornet2 = {"color_red_hornet2", "255"};
cvar_t oz_colorgreenhornet2 = {"color_green_hornet2", "128"};
cvar_t oz_colorbluehornet2 = {"color_blue_hornet2", "0"};
cvar_t oz_colorredtripmine = {"color_red_tripmine", "0"};
cvar_t oz_colorgreentripmine = {"color_green_tripmine", "214"};
cvar_t oz_colorbluetripmine = {"color_blue_tripmine", "198"};
cvar_t oz_colorredgaussprimary = {"color_red_gaussprimary", "255"};
cvar_t oz_colorgreengaussprimary = {"color_green_gaussprimary", "128"};
cvar_t oz_colorbluegaussprimary = {"color_blue_gaussprimary", "0"};
cvar_t oz_colorredgausssecondary = {"color_red_gausssecondary", "255"};
cvar_t oz_colorgreengausssecondary = {"color_green_gausssecondary", "255"};
cvar_t oz_colorbluegausssecondary = {"color_blue_gausssecondary", "255"};
cvar_t oz_grappleairspeed = {"grapple_airspeed", "1500"};
cvar_t oz_grapplewaterspeed = {"grapple_waterspeed", "1000"};
cvar_t oz_grappledragspeed = {"grapple_dragspeed", "700"};
cvar_t oz_grappledamage = {"damage_grapple", "5"};
cvar_t oz_grappledisable = {"disable_grapple", "0"};
cvar_t oz_grappleglow = {"grapple_glow", "0"};
cvar_t oz_grapplesound = {"enable_grapple_sounds", "1"};
cvar_t oz_skygrapple = {"enable_skygrapple", "0"};
cvar_t oz_Display_Player_Message = {"show_player_message", "0"};
cvar_t oz_GameDesc = {"GameDesc", "0"};
cvar_t oz_picture = {"mp_picture", "0"};
cvar_t oz_cheatdetect = {"bm_cheatdetect", "1"};
cvar_t oz_logchat = {"logchat", "1"};
cvar_t oz_logrune = {"logrune", "0"};
cvar_t oz_spectators = {"oz_spectators", "1"};
cvar_t oz_advspec = {"oz_advspec", "1"};
cvar_t oz_link = {"oz_link", " "};
cvar_t oz_line1 = {"oz_line1", " "};
cvar_t oz_line2 = {"oz_line2", " "};
cvar_t oz_playerid = {"player_id", "1"};
cvar_t oz_healthchargercapacity = {"healthcharger_capacity", "50"};
cvar_t oz_suitchargercapacity = {"suitcharger_capacity", "30"};
cvar_t oz_healthchargerrecharge = {"healthcharger_rechargetime", "60"};
cvar_t oz_suitchargerrecharge = {"suitcharger_rechargetime", "30"};
cvar_t oz_healthchargeramount = {"healthcharger_amount", "1"};
cvar_t oz_suitchargeramount = {"suitcharger_amount", "1"};
cvar_t oz_starthealth = {"start_health", "100"};
cvar_t oz_startcharge = {"start_charge", "0"};
cvar_t oz_maxhealth = {"max_health", "100"};
cvar_t oz_maxcharge = {"max_charge", "100"};
cvar_t oz_healthkit = {"healthkit", "15"};
cvar_t oz_battery = {"battery", "15"};
cvar_t oz_fallingdamage = {"damage_falling", "0"};
cvar_t oz_startlongjump = {"start_longjump", "0"};
cvar_t oz_playerair = {"player_air", "12"};
cvar_t oz_playerspeed = {"player_speed", "300"};
cvar_t oz_headdamage = {"head_damage", "2"};
cvar_t oz_chestdamage = {"chest_damage", "1"};
cvar_t oz_stomachdamage = {"stomach_damage", "1"};
cvar_t oz_armdamage = {"arm_damage", "1"};
cvar_t oz_legdamage = {"leg_damage", "1"};
cvar_t oz_bloodamount = {"blood_amount", "2"};
cvar_t oz_crowbarrunenumber = {"number_of_crowbar_runes", "1"};
cvar_t oz_hasterunenumber = {"number_of_haste_runes", "1"};
cvar_t oz_superglockrunenumber = {"number_of_superglock_runes", "1"};
cvar_t oz_traprunenumber = {"number_of_boobytrap_runes", "1"};
cvar_t oz_cloakrunenumber = {"number_of_cloak_runes", "0"};
cvar_t oz_protectrunenumber = {"number_of_protection_runes", "1"};
cvar_t oz_regenrunenumber = {"number_of_regen_runes", "1"};
cvar_t oz_vamprunenumber = {"number_of_vampire_runes", "1"};
cvar_t oz_hookrunenumber = {"number_of_superhook_runes", "1"};
cvar_t oz_lowgravrunenumber = {"number_of_lowgravity_runes", "1"};
cvar_t oz_superspeedrunenumber = {"number_of_superspeed_runes", "1"};
cvar_t oz_teleportrunenumber = {"number_of_teleport_runes", "0"};
cvar_t oz_fragrunenumber = {"number_of_frag_runes", "0"};
cvar_t oz_identifyrunenumber = {"number_of_identify_runes", "0"};
cvar_t oz_randomrunenumber = {"number_of_random_runes", "1"};
cvar_t oz_runemodel = {"rune_model", "7"};
cvar_t oz_customrunemodel = {"customrunemodel", "oz_custom_rune.mdl"};
cvar_t oz_runescheme = {"rune_scheme", "0"};
cvar_t oz_respawntimerune = {"respawntime_rune", "30"};
cvar_t oz_fragruneamount = {"frag_rune_value", "1"};
cvar_t oz_regenrunehealth = {"regen_rune_health", "1"};
cvar_t oz_regenrunearmor = {"regen_rune_armor", "1"};
cvar_t oz_superhookrunedamage = {"superhook_rune_damage", "25"};
cvar_t oz_lowgravruneamount = {"lowgravity_rune_value", ".2"};
cvar_t oz_maxhealthwithvamprune = {"max_health_with_vampire_rune", "150"};
cvar_t oz_boobytrapdamage = {"damage_boobytrap_rune", "150"};
cvar_t oz_nightops = {"night_ops", "0"};
cvar_t oz_lightlevel = {"light_level", "4"};
cvar_t oz_itemrespawntime = {"respawntime_item", "30"};
cvar_t oz_ammorespawntime = {"respawntime_ammo", "20"};
cvar_t oz_weaponrespawntime = {"respawntime_weapon", "20"};
cvar_t oz_pointsperkill = {"points_per_kill", "1"};
cvar_t oz_pointspersuicide = {"points_per_suicide", "1"};
cvar_t oz_pointsperworldkill = {"points_per_worldkill", "1"};
cvar_t oz_pointsperteamkill = {"points_per_teamkill", "1"};
cvar_t oz_pointsperdeath = {"points_per_death", "1"};
cvar_t oz_autoswitch = {"auto_switch", "1"};
cvar_t oz_weightcrowbar = {"weight_crowbar", "0"};
cvar_t oz_weightglock = {"weight_glock", "10"};
cvar_t oz_weight357 = {"weight_357", "15"};
cvar_t oz_weightmp5 = {"weight_mp5", "15"};
cvar_t oz_weightshotgun = {"weight_shotgun", "0"};
cvar_t oz_weightxbow = {"weight_xbow", "10"};
cvar_t oz_weightrpg = {"weight_rpg", "20"};
cvar_t oz_weightgauss = {"weight_gauss", "20"};
cvar_t oz_weightegon = {"weight_egon", "20"};
cvar_t oz_weighthivehand = {"weight_hivehand", "10"};
cvar_t oz_weighthgrenade = {"weight_hgrenade", "5"};
cvar_t oz_weightsnark = {"weight_snark", "5"};
cvar_t oz_weightsatchel = {"weight_satchel", "-10"};
cvar_t oz_weighttripmine = {"weight_tripmine", "-10"};
cvar_t oz_disableglock = {"disable_glock", "0"};
cvar_t oz_disable357 = {"disable_357", "0"};
cvar_t oz_disablemp5 = {"disable_mp5", "0"};
cvar_t oz_disablem203 = {"disable_m203", "0"};
cvar_t oz_disableshotgun = {"disable_shotgun", "0"};
cvar_t oz_disablexbow = {"disable_xbow", "0"};
cvar_t oz_disablerpg = {"disable_rpg", "0"};
cvar_t oz_disablegauss = {"disable_gauss", "0"};
cvar_t oz_disableegon = {"disable_egon", "0"};
cvar_t oz_disablehivehand = {"disable_hivehand", "0"};
cvar_t oz_disablehgrenade = {"disable_hgrenade", "0"};
cvar_t oz_disablesatchel = {"disable_satchel", "0"};
cvar_t oz_disabletripmine = {"disable_tripmine", "0"};
cvar_t oz_disablesnark = {"disable_snark", "0"};
cvar_t oz_startglock = {"start_glock", "1"};
cvar_t oz_start357 = {"start_357", "0"};
cvar_t oz_startmp5 = {"start_mp5", "0"};
cvar_t oz_startshotgun = {"start_shotgun", "0"};
cvar_t oz_startxbow = {"start_xbow", "0"};
cvar_t oz_startrpg = {"start_rpg", "0"};
cvar_t oz_startgauss = {"start_gauss", "0"};
cvar_t oz_startegon = {"start_egon", "0"};
cvar_t oz_starthivehand = {"start_hivehand", "0"};
cvar_t oz_activeweapon = {"active_weapon", "4"};
cvar_t oz_start9mm = {"start_9mm", "50"};
cvar_t oz_startshells = {"start_shells", "0"};
cvar_t oz_start357rounds = {"start_357rounds", "0"};
cvar_t oz_startm203 = {"start_m203", "0"};
cvar_t oz_starturanium = {"start_uranium", "0"};
cvar_t oz_startrockets = {"start_rockets", "0"};
cvar_t oz_startxbowbolts = {"start_xbowbolts", "0"};
cvar_t oz_starthgrenades = {"start_hgrenade", "0"};
cvar_t oz_startsatchels = {"start_satchel", "0"};
cvar_t oz_starttripmines = {"start_tripmine", "0"};
cvar_t oz_startsnarks = {"start_snark", "0"};
cvar_t oz_maxuranium = {"max_uranium", "100"};
cvar_t oz_max9mm = {"max_9mm", "250"};
cvar_t oz_maxshells = {"max_shells", "125"};
cvar_t oz_max357rounds = {"max_357rounds", "36"};
cvar_t oz_maxm203 = {"max_m203", "10"};
cvar_t oz_maxrockets = {"max_rockets", "5"};
cvar_t oz_maxsatchels = {"max_satchels", "5"};
cvar_t oz_maxtripmines = {"max_tripmines", "5"};
cvar_t oz_maxhgrenades = {"max_hgrenades", "10"};
cvar_t oz_maxsnarks = {"max_snarks", "10"};
cvar_t oz_maxhornets = {"max_hornets", "8"};
cvar_t oz_maxxbowbolts = {"max_xbowbolts", "50"};
cvar_t oz_giveuranium = {"item_uranium", "20"};
cvar_t oz_givechainbox = {"item_chainbox", "200"};
cvar_t oz_giveshells = {"item_shells", "12"};
cvar_t oz_give357rounds = {"item_357rounds", "6"};
cvar_t oz_givem203 = {"item_m203", "2"};
cvar_t oz_giveglockclip = {"item_glockclip", "17"};
cvar_t oz_givemp5clip = {"item_mp5clip", "50"};
cvar_t oz_giverpgclip = {"item_rpgclip", "2"};
cvar_t oz_givexbowbolts = {"item_xbowclip", "5"};
cvar_t oz_givewithmp5 = {"ammo_mp5", "50"};
cvar_t oz_givewithrpg = {"ammo_rpg", "2"};
cvar_t oz_givewithglock = {"ammo_glock", "17"};
cvar_t oz_givewithshotgun = {"ammo_shotgun", "12"};
cvar_t oz_givewithgauss = {"ammo_gauss", "20"};
cvar_t oz_givewithegon = {"ammo_egon", "20"};
cvar_t oz_givewithhgrenade = {"ammo_hgrenade", "5"};
cvar_t oz_givewithsatchel = {"ammo_satchel", "1"};
cvar_t oz_givewithtripmine = {"ammo_tripmine", "1"};
cvar_t oz_givewithxbow = {"ammo_xbow", "5"};
cvar_t oz_givewithm203 = {"ammo_m203", "0"};
cvar_t oz_givewithsnark = {"ammo_snark", "5"};
cvar_t oz_givewithhivehand = {"ammo_hivehand", "8"};
cvar_t oz_givewith357 = {"ammo_357", "6"};
cvar_t oz_magazineglock = {"clip_glock", "17"};
cvar_t oz_magazine357 = {"clip_357", "6"};
cvar_t oz_magazinemp5 = {"clip_mp5", "50"};
cvar_t oz_magazineshotgun = {"clip_shotgun", "8"};
cvar_t oz_magazinexbow = {"clip_xbow", "5"};
cvar_t oz_damagecrowbar = {"damage_crowbar", "25"};
cvar_t oz_damageflyingcrowbar = {"damage_flying_crowbar", "25"};
cvar_t oz_damageglock = {"damage_glock", "12"};
cvar_t oz_damagesglock = {"damage_sglock", "300"};
cvar_t oz_damageglocksniper = {"damage_glock_sniper", "12"};
cvar_t oz_damage357 = {"damage_357", "25"};
cvar_t oz_damage357sniper = {"damage_357_sniper", "25"};
cvar_t oz_damagemp5 = {"damage_mp5", "12"};
cvar_t oz_damagemp5sniper = {"damage_mp5_sniper", "12"};
cvar_t oz_damageshotgun = {"damage_shotgun_pellet", "20"};
cvar_t oz_damagegauss = {"damage_gauss", "20"};
cvar_t oz_damageegonwide = {"damage_egonwide", "20"};
cvar_t oz_damagerpg = {"damage_rpg", "120"};
cvar_t oz_damagehomingrpg = {"damage_rpg_homing", "50"};
cvar_t oz_damagem203 = {"damage_m203", "100"};
cvar_t oz_damagexbow = {"damage_xbow", "20"};
cvar_t oz_damagexbowexplode = {"damage_xbowexplode", "40"};
cvar_t oz_damagexbowsniper = {"damage_xbow_sniper", "120"};
cvar_t oz_damagegaussmax = {"damage_gaussmax", "200"};
cvar_t oz_damageegonnarrow = {"damage_egonnarrow", "10"};
cvar_t oz_damagehgrenade = {"damage_hgrenade", "100"};
cvar_t oz_damagehornet = {"damage_hornet", "10"};
cvar_t oz_damagesatchel = {"damage_satchel", "120"};
cvar_t oz_damagetripmine = {"damage_tripmine", "150"};
cvar_t oz_damagesnarkbite = {"damage_snarkbite", "10"};
cvar_t oz_damagesnarkpop = {"damage_snarkpop", "30"};
cvar_t oz_recoilglock = {"recoil_glock", "2"};
cvar_t oz_recoilmp5 = {"recoil_mp5", "2"};
cvar_t oz_recoilm203 = {"recoil_m203", "10"};
cvar_t oz_recoilxbow = {"recoil_xbow", "2"};
cvar_t oz_recoilrpg = {"recoil_rpg", "5"};
cvar_t oz_recoilshotgun = {"recoil_shotgun", "5"};
cvar_t oz_recoilshotgun2 = {"recoil_shotgun2", "10"};
cvar_t oz_recoil357 = {"recoil_357", "10"};
cvar_t oz_recoilgauss = {"recoil_gauss", "2"};
cvar_t oz_recoilhivehand = {"recoil_hivehand", "2"};
cvar_t oz_cyclecrowbar = {"cycle_crowbar_first", ".5"};
cvar_t oz_cyclecrowbar2 = {"cycle_crowbar_after", ".25"};
cvar_t oz_cycleglock = {"cycle_glock", ".3"};
cvar_t oz_cyclesglock = {"cycle_sglock", "1.5"};
cvar_t oz_cycleglock2 = {"cycle_glock2", ".75"};
cvar_t oz_cyclemp5 = {"cycle_mp5", ".1"};
cvar_t oz_cyclemp5sniper = {"cycle_mp5_sniper", ".3"};
cvar_t oz_cycle357 = {"cycle_357", ".75"};
cvar_t oz_cycle3572 = {"cycle_3572", "1.25"};
cvar_t oz_cycleshotgun = {"cycle_shotgun", ".75"};
cvar_t oz_cycleshotgun2 = {"cycle_shotgun2", "1.5"};
cvar_t oz_cyclegauss = {"cycle_gauss", ".2"};
cvar_t oz_cyclem203 = {"cycle_m203", "1"};
cvar_t oz_cyclexbow = {"cycle_xbow", ".75"};
cvar_t oz_cyclexbow2 = {"cycle_xbow_sniper", ".75"};
cvar_t oz_cyclehivehand = {"cycle_hivehand", ".25"};
cvar_t oz_cyclehivehand2 = {"cycle_hivehand2", ".1"};
cvar_t oz_speedhgrenade = {"speed_hgrenade", "4"};
cvar_t oz_speedm203 = {"speed_m203", "800"};
cvar_t oz_speedrpgwater = {"speed_rpg_water", "300"};
cvar_t oz_speedrpgair = {"speed_rpg_air", "2000"};
cvar_t oz_speedsatchel = {"speed_satchel", "274"};
cvar_t oz_speedxbowair = {"speed_xbow_air", "2000"};
cvar_t oz_speedxbowwater = {"speed_xbow_water", "1000"};
cvar_t oz_speedhornet = {"speed_hornet_red", "600"};
cvar_t oz_speedhornet2 = {"speed_hornet_orange", "800"};
cvar_t oz_reloadglock = {"reload_glock", "1.5"};
cvar_t oz_reload357 = {"reload_357", "2"};
cvar_t oz_reloadmp5 = {"reload_mp5", "1.5"};
cvar_t oz_reloadshotgun = {"shotgun_shells_per_reload", "1"};
cvar_t oz_reloadxbow = {"reload_xbow", "4.5"};
cvar_t oz_reloadrpg = {"reload_rpg", "2"};
cvar_t oz_mp5tracers = {"mp5_tracers", "2"};
cvar_t oz_mp5spread = {"mp5_spread", "6"};
cvar_t oz_mp5sniper = {"mp5_sniper", "0"};
cvar_t oz_silencemp5 = {"mp5_silence", "0"};
cvar_t oz_glocksniper = {"glock_sniper", "0"};
cvar_t oz_silenceglock = {"glock_silence", "0"};
cvar_t oz_pelletsshotgun = {"shotgun_pellets", "4"};
cvar_t oz_pelletsdoubleshotgun = {"shotgun2_pellets", "8"};
cvar_t oz_snarkhealth = {"snark_health", "10"};
cvar_t oz_snarktime = {"snark_time", "15"};
cvar_t oz_gaussmaxchargetime = {"gauss_maxchargetime", "10"};
cvar_t oz_grenadetime = {"grenade_time", "3"};
cvar_t oz_flaretime = {"flare_time", "10"};
cvar_t oz_waterweapons = {"water_weapons", "0"};
cvar_t oz_zoomamountglock = {"zoom_fov_glock", "40"};
cvar_t oz_zoomamountmp5 = {"zoom_fov_mp5", "30"};
cvar_t oz_zoomamountxbow = {"zoom_fov_xbow", "20"};
cvar_t oz_zoomamount357 = {"zoom_fov_357", "40"};
cvar_t oz_satchelchargetime = {"satchel_charge_time", "5"};
cvar_t oz_satchelareadetect = {"satchel_detection_area", "128"};
cvar_t oz_allowproximity = {"allow_proximity_satchel", "0"};
cvar_t oz_sectripammoused = {"ammo_used_per_secondary_mine", "5"};
cvar_t oz_shake_enable = {"shake_enable", "1"};
cvar_t oz_shake_amp = {"shake_amp", "25.0"};
cvar_t oz_shake_freq = {"shake_freq", "150.0"};
cvar_t oz_shake_dur = {"shake_dur", "1.0"};
cvar_t oz_shake_radius = {"shake_radius", "850"};
cvar_t oz_homing = {"rpg_homing", "0"};
cvar_t bot_blood = {"bot_blood", "0"};
cvar_t bot_speed = {"bot_speed", "300"};
cvar_t bot_number = {"bot_number", "0"};
cvar_t bot_skill = {"bot_skill", "50"};
cvar_t bot_skill_aa = {"bot_skill_aa", "1"};
cvar_t bot_jump = {"bot_jump", "50"};
cvar_t bot_mercy = {"bot_mercy", "1"};
cvar_t bot_chat = {"bot_chat", "30"};
cvar_t bot_enable = {"bot_enable", "0"};
cvar_t bot_debug = {"bot_debug", "0"};
cvar_t bot_dontchat = {"bot_dontchat", "0"};
cvar_t bot_observer = {"bot_observer", "0"};
cvar_t bot_name = {"bot_name", "Rho-Bot"};
cvar_t bot_model = {"bot_model", "robo"};
cvar_t bot_top_color = {"bot_top_color", "255"};
cvar_t bot_bottom_color = {"bot_bottom_color", "255"};
cvar_t bot_trait_jump = {"bot_trait_jump", "50"};
cvar_t bot_trait_accuracy = {"bot_trait_accuracy", "50"};
cvar_t bot_trait_aggression = {"bot_trait_aggression", "50"};
cvar_t bot_trait_chat = {"bot_trait_chat", "50"};
cvar_t bot_trait_perception = {"bot_trait_perception", "70"};
cvar_t bot_trait_reflexes = {"bot_trait_reflexes", "70"};
cvar_t bot_wp_crossbow = {"bot_wp_crossbow", "50"};
cvar_t bot_wp_crowbar = {"bot_wp_crowbar", "50"};
cvar_t bot_wp_egon = {"bot_wp_egon", "50"};
cvar_t bot_wp_gauss = {"bot_wp_gauss", "50"};
cvar_t bot_wp_glock = {"bot_wp_glock", "50"};
cvar_t bot_wp_handgrenade = {"bot_wp_handgrenade", "50"};
cvar_t bot_wp_hornetgun = {"bot_wp_hornetgun", "50"};
cvar_t bot_wp_mp5 = {"bot_wp_mp5", "50"};
cvar_t bot_wp_python = {"bot_wp_python", "50"};
cvar_t bot_wp_rpg = {"bot_wp_rpg", "50"};
cvar_t bot_wp_satchel = {"bot_wp_satchel", "50"};
cvar_t bot_wp_shotgun = {"bot_wp_shotgun", "50"};
cvar_t bot_wp_snark = {"bot_wp_snark", "50"};
cvar_t bot_wp_tripmine = {"bot_wp_tripmine", "50"};
cvar_t cvar_bot = {"bot", ""};
cvar_t oz_minclients = {"min_clients", "0"};
cvar_t oz_minbot = {"min_bot", "0"};
cvar_t runfolder = {"runfolder", "valve"};
cvar_t bot_aa_max = {"bot_aa_max", "99"};

static bool SV_InitServer()
{
	if (!FileSystem_LoadFileSystem())
	{
		return false;
	}

	return true;
}

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit()
{
	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_psv_allow_autoaim = CVAR_GET_POINTER("sv_allow_autoaim");
	g_footsteps = CVAR_GET_POINTER("mp_footsteps");
	g_psv_cheats = CVAR_GET_POINTER("sv_cheats");

	if (!SV_InitServer())
	{
		g_engfuncs.pfnServerPrint("Error initializing server\n");
		//Shut the game down as soon as possible.
		SERVER_COMMAND("quit\n");
		return;
	}

	// in the original's order
	CVAR_REGISTER(&displaysoundlist);
	CVAR_REGISTER(&allow_spectators); // halflife-updated's
	CVAR_REGISTER(&teamplay);
	CVAR_REGISTER(&fraglimit);
	CVAR_REGISTER(&timelimit);
	CVAR_REGISTER(&fragsleft);
	CVAR_REGISTER(&timeleft);
	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&falldamage);
	CVAR_REGISTER(&weaponstay);
	CVAR_REGISTER(&forcerespawn);
	CVAR_REGISTER(&flashlight);
	CVAR_REGISTER(&aimcrosshair);
	CVAR_REGISTER(&decalfrequency);
	CVAR_REGISTER(&teamlist);
	CVAR_REGISTER(&teamoverride);
	CVAR_REGISTER(&defaultteam);
	CVAR_REGISTER(&allowmonsters);
	CVAR_REGISTER(&mp_chattime);
	CVAR_REGISTER(&sv_busters); // halflife-updated's
	CVAR_REGISTER(&sv_allowbunnyhopping); // halflife-updated's
	CVAR_REGISTER(&sk_turret_health1);
	CVAR_REGISTER(&sk_turret_health2);
	CVAR_REGISTER(&sk_turret_health3);
	CVAR_REGISTER(&sk_miniturret_health1);
	CVAR_REGISTER(&sk_miniturret_health2);
	CVAR_REGISTER(&sk_miniturret_health3);
	CVAR_REGISTER(&sk_sentry_health1);
	CVAR_REGISTER(&sk_sentry_health2);
	CVAR_REGISTER(&sk_sentry_health3);
	CVAR_REGISTER(&sk_12mm_bullet1);
	CVAR_REGISTER(&sk_12mm_bullet2);
	CVAR_REGISTER(&sk_12mm_bullet3);
	CVAR_REGISTER(&sk_9mmAR_bullet1);
	CVAR_REGISTER(&sk_9mmAR_bullet2);
	CVAR_REGISTER(&sk_9mmAR_bullet3);
	CVAR_REGISTER(&sk_9mm_bullet1);
	CVAR_REGISTER(&sk_9mm_bullet2);
	CVAR_REGISTER(&sk_9mm_bullet3);
	CVAR_REGISTER(&sk_hornet_dmg1);
	CVAR_REGISTER(&sk_hornet_dmg2);
	CVAR_REGISTER(&sk_hornet_dmg3);
	CVAR_REGISTER(&sk_suitcharger1);
	CVAR_REGISTER(&sk_suitcharger2);
	CVAR_REGISTER(&sk_suitcharger3);
	CVAR_REGISTER(&sk_battery1);
	CVAR_REGISTER(&sk_battery2);
	CVAR_REGISTER(&sk_battery3);
	CVAR_REGISTER(&sk_healthcharger1);
	CVAR_REGISTER(&sk_healthcharger2);
	CVAR_REGISTER(&sk_healthcharger3);
	CVAR_REGISTER(&sk_healthkit1);
	CVAR_REGISTER(&sk_healthkit2);
	CVAR_REGISTER(&sk_healthkit3);
	CVAR_REGISTER(&sk_scientist_heal1);
	CVAR_REGISTER(&sk_scientist_heal2);
	CVAR_REGISTER(&sk_scientist_heal3);
	CVAR_REGISTER(&oz_randomgrapplecolor);
	CVAR_REGISTER(&oz_randomtripminecolor);
	CVAR_REGISTER(&oz_colorredgrapple);
	CVAR_REGISTER(&oz_colorgreengrapple);
	CVAR_REGISTER(&oz_colorbluegrapple);
	CVAR_REGISTER(&oz_colorredrpg);
	CVAR_REGISTER(&oz_colorgreenrpg);
	CVAR_REGISTER(&oz_colorbluerpg);
	CVAR_REGISTER(&oz_colorredhornet1);
	CVAR_REGISTER(&oz_colorgreenhornet1);
	CVAR_REGISTER(&oz_colorbluehornet1);
	CVAR_REGISTER(&oz_colorredhornet2);
	CVAR_REGISTER(&oz_colorgreenhornet2);
	CVAR_REGISTER(&oz_colorbluehornet2);
	CVAR_REGISTER(&oz_colorredtripmine);
	CVAR_REGISTER(&oz_colorgreentripmine);
	CVAR_REGISTER(&oz_colorbluetripmine);
	CVAR_REGISTER(&oz_colorredgaussprimary);
	CVAR_REGISTER(&oz_colorgreengaussprimary);
	CVAR_REGISTER(&oz_colorbluegaussprimary);
	CVAR_REGISTER(&oz_colorredgausssecondary);
	CVAR_REGISTER(&oz_colorgreengausssecondary);
	CVAR_REGISTER(&oz_colorbluegausssecondary);
	CVAR_REGISTER(&oz_grappleairspeed);
	CVAR_REGISTER(&oz_grapplewaterspeed);
	CVAR_REGISTER(&oz_grappledragspeed);
	CVAR_REGISTER(&oz_grappledamage);
	CVAR_REGISTER(&oz_grappledisable);
	CVAR_REGISTER(&oz_grappleglow);
	CVAR_REGISTER(&oz_grapplesound);
	CVAR_REGISTER(&oz_skygrapple);
	// mapcyclefile: the engine registers it (reference/DEVIATIONS.md)
	// servercfgfile: the engine registers it (reference/DEVIATIONS.md)
	// lservercfgfile: the engine registers it (reference/DEVIATIONS.md)
	// motdfile: the engine registers it (reference/DEVIATIONS.md)
	CVAR_REGISTER(&oz_Display_Player_Message);
	CVAR_REGISTER(&oz_GameDesc);
	CVAR_REGISTER(&oz_picture);
	CVAR_REGISTER(&oz_cheatdetect);
	CVAR_REGISTER(&oz_logchat);
	CVAR_REGISTER(&oz_logrune);
	CVAR_REGISTER(&oz_spectators);
	CVAR_REGISTER(&oz_advspec);
	CVAR_REGISTER(&oz_link);
	CVAR_REGISTER(&oz_line1);
	CVAR_REGISTER(&oz_line2);
	CVAR_REGISTER(&oz_playerid);
	CVAR_REGISTER(&oz_healthchargercapacity);
	CVAR_REGISTER(&oz_suitchargercapacity);
	CVAR_REGISTER(&oz_healthchargerrecharge);
	CVAR_REGISTER(&oz_suitchargerrecharge);
	CVAR_REGISTER(&oz_healthchargeramount);
	CVAR_REGISTER(&oz_suitchargeramount);
	CVAR_REGISTER(&oz_starthealth);
	CVAR_REGISTER(&oz_startcharge);
	CVAR_REGISTER(&oz_maxhealth);
	CVAR_REGISTER(&oz_maxcharge);
	CVAR_REGISTER(&oz_healthkit);
	CVAR_REGISTER(&oz_battery);
	CVAR_REGISTER(&oz_fallingdamage);
	CVAR_REGISTER(&oz_startlongjump);
	CVAR_REGISTER(&oz_playerair);
	CVAR_REGISTER(&oz_playerspeed);
	CVAR_REGISTER(&oz_headdamage);
	CVAR_REGISTER(&oz_chestdamage);
	CVAR_REGISTER(&oz_stomachdamage);
	CVAR_REGISTER(&oz_armdamage);
	CVAR_REGISTER(&oz_legdamage);
	CVAR_REGISTER(&oz_bloodamount);
	CVAR_REGISTER(&oz_crowbarrunenumber);
	CVAR_REGISTER(&oz_hasterunenumber);
	CVAR_REGISTER(&oz_superglockrunenumber);
	CVAR_REGISTER(&oz_traprunenumber);
	CVAR_REGISTER(&oz_cloakrunenumber);
	CVAR_REGISTER(&oz_protectrunenumber);
	CVAR_REGISTER(&oz_regenrunenumber);
	CVAR_REGISTER(&oz_vamprunenumber);
	CVAR_REGISTER(&oz_hookrunenumber);
	CVAR_REGISTER(&oz_lowgravrunenumber);
	CVAR_REGISTER(&oz_superspeedrunenumber);
	CVAR_REGISTER(&oz_teleportrunenumber);
	CVAR_REGISTER(&oz_fragrunenumber);
	CVAR_REGISTER(&oz_identifyrunenumber);
	CVAR_REGISTER(&oz_randomrunenumber);
	CVAR_REGISTER(&oz_runemodel);
	CVAR_REGISTER(&oz_customrunemodel);
	CVAR_REGISTER(&oz_runescheme);
	CVAR_REGISTER(&oz_respawntimerune);
	CVAR_REGISTER(&oz_fragruneamount);
	CVAR_REGISTER(&oz_regenrunehealth);
	CVAR_REGISTER(&oz_regenrunearmor);
	CVAR_REGISTER(&oz_superhookrunedamage);
	CVAR_REGISTER(&oz_lowgravruneamount);
	CVAR_REGISTER(&oz_maxhealthwithvamprune);
	CVAR_REGISTER(&oz_boobytrapdamage);
	CVAR_REGISTER(&oz_nightops);
	CVAR_REGISTER(&oz_lightlevel);
	CVAR_REGISTER(&oz_itemrespawntime);
	CVAR_REGISTER(&oz_ammorespawntime);
	CVAR_REGISTER(&oz_weaponrespawntime);
	CVAR_REGISTER(&oz_pointsperkill);
	CVAR_REGISTER(&oz_pointspersuicide);
	CVAR_REGISTER(&oz_pointsperworldkill);
	CVAR_REGISTER(&oz_pointsperteamkill);
	CVAR_REGISTER(&oz_pointsperdeath);
	CVAR_REGISTER(&oz_autoswitch);
	CVAR_REGISTER(&oz_weightcrowbar);
	CVAR_REGISTER(&oz_weightglock);
	CVAR_REGISTER(&oz_weight357);
	CVAR_REGISTER(&oz_weightmp5);
	CVAR_REGISTER(&oz_weightshotgun);
	CVAR_REGISTER(&oz_weightxbow);
	CVAR_REGISTER(&oz_weightrpg);
	CVAR_REGISTER(&oz_weightgauss);
	CVAR_REGISTER(&oz_weightegon);
	CVAR_REGISTER(&oz_weighthivehand);
	CVAR_REGISTER(&oz_weighthgrenade);
	CVAR_REGISTER(&oz_weightsnark);
	CVAR_REGISTER(&oz_weightsatchel);
	CVAR_REGISTER(&oz_weighttripmine);
	CVAR_REGISTER(&oz_disableglock);
	CVAR_REGISTER(&oz_disable357);
	CVAR_REGISTER(&oz_disablemp5);
	CVAR_REGISTER(&oz_disablem203);
	CVAR_REGISTER(&oz_disableshotgun);
	CVAR_REGISTER(&oz_disablexbow);
	CVAR_REGISTER(&oz_disablerpg);
	CVAR_REGISTER(&oz_disablegauss);
	CVAR_REGISTER(&oz_disableegon);
	CVAR_REGISTER(&oz_disablehivehand);
	CVAR_REGISTER(&oz_disablehgrenade);
	CVAR_REGISTER(&oz_disablesatchel);
	CVAR_REGISTER(&oz_disabletripmine);
	CVAR_REGISTER(&oz_disablesnark);
	CVAR_REGISTER(&oz_startglock);
	CVAR_REGISTER(&oz_start357);
	CVAR_REGISTER(&oz_startmp5);
	CVAR_REGISTER(&oz_startshotgun);
	CVAR_REGISTER(&oz_startxbow);
	CVAR_REGISTER(&oz_startrpg);
	CVAR_REGISTER(&oz_startgauss);
	CVAR_REGISTER(&oz_startegon);
	CVAR_REGISTER(&oz_starthivehand);
	CVAR_REGISTER(&oz_activeweapon);
	CVAR_REGISTER(&oz_start9mm);
	CVAR_REGISTER(&oz_startshells);
	CVAR_REGISTER(&oz_start357rounds);
	CVAR_REGISTER(&oz_startm203);
	CVAR_REGISTER(&oz_starturanium);
	CVAR_REGISTER(&oz_startrockets);
	CVAR_REGISTER(&oz_startxbowbolts);
	CVAR_REGISTER(&oz_starthgrenades);
	CVAR_REGISTER(&oz_startsatchels);
	CVAR_REGISTER(&oz_starttripmines);
	CVAR_REGISTER(&oz_startsnarks);
	CVAR_REGISTER(&oz_maxuranium);
	CVAR_REGISTER(&oz_max9mm);
	CVAR_REGISTER(&oz_maxshells);
	CVAR_REGISTER(&oz_max357rounds);
	CVAR_REGISTER(&oz_maxm203);
	CVAR_REGISTER(&oz_maxrockets);
	CVAR_REGISTER(&oz_maxsatchels);
	CVAR_REGISTER(&oz_maxtripmines);
	CVAR_REGISTER(&oz_maxhgrenades);
	CVAR_REGISTER(&oz_maxsnarks);
	CVAR_REGISTER(&oz_maxhornets);
	CVAR_REGISTER(&oz_maxxbowbolts);
	CVAR_REGISTER(&oz_giveuranium);
	CVAR_REGISTER(&oz_givechainbox);
	CVAR_REGISTER(&oz_giveshells);
	CVAR_REGISTER(&oz_give357rounds);
	CVAR_REGISTER(&oz_givem203);
	CVAR_REGISTER(&oz_giveglockclip);
	CVAR_REGISTER(&oz_givemp5clip);
	CVAR_REGISTER(&oz_giverpgclip);
	CVAR_REGISTER(&oz_givexbowbolts);
	CVAR_REGISTER(&oz_givewithmp5);
	CVAR_REGISTER(&oz_givewithrpg);
	CVAR_REGISTER(&oz_givewithglock);
	CVAR_REGISTER(&oz_givewithshotgun);
	CVAR_REGISTER(&oz_givewithgauss);
	CVAR_REGISTER(&oz_givewithegon);
	CVAR_REGISTER(&oz_givewithhgrenade);
	CVAR_REGISTER(&oz_givewithsatchel);
	CVAR_REGISTER(&oz_givewithtripmine);
	CVAR_REGISTER(&oz_givewithxbow);
	CVAR_REGISTER(&oz_givewithm203);
	CVAR_REGISTER(&oz_givewithsnark);
	CVAR_REGISTER(&oz_givewithhivehand);
	CVAR_REGISTER(&oz_givewith357);
	CVAR_REGISTER(&oz_magazineglock);
	CVAR_REGISTER(&oz_magazine357);
	CVAR_REGISTER(&oz_magazinemp5);
	CVAR_REGISTER(&oz_magazineshotgun);
	CVAR_REGISTER(&oz_magazinexbow);
	CVAR_REGISTER(&oz_damagecrowbar);
	CVAR_REGISTER(&oz_damageflyingcrowbar);
	CVAR_REGISTER(&oz_damageglock);
	CVAR_REGISTER(&oz_damagesglock);
	CVAR_REGISTER(&oz_damageglocksniper);
	CVAR_REGISTER(&oz_damage357);
	CVAR_REGISTER(&oz_damage357sniper);
	CVAR_REGISTER(&oz_damagemp5);
	CVAR_REGISTER(&oz_damagemp5sniper);
	CVAR_REGISTER(&oz_damageshotgun);
	CVAR_REGISTER(&oz_damagegauss);
	CVAR_REGISTER(&oz_damageegonwide);
	CVAR_REGISTER(&oz_damagerpg);
	CVAR_REGISTER(&oz_damagehomingrpg);
	CVAR_REGISTER(&oz_damagem203);
	CVAR_REGISTER(&oz_damagexbow);
	CVAR_REGISTER(&oz_damagexbowexplode);
	CVAR_REGISTER(&oz_damagexbowsniper);
	CVAR_REGISTER(&oz_damagegaussmax);
	CVAR_REGISTER(&oz_damageegonnarrow);
	CVAR_REGISTER(&oz_damagehgrenade);
	CVAR_REGISTER(&oz_damagehornet);
	CVAR_REGISTER(&oz_damagesatchel);
	CVAR_REGISTER(&oz_damagetripmine);
	CVAR_REGISTER(&oz_damagesnarkbite);
	CVAR_REGISTER(&oz_damagesnarkpop);
	CVAR_REGISTER(&oz_recoilglock);
	CVAR_REGISTER(&oz_recoilmp5);
	CVAR_REGISTER(&oz_recoilm203);
	CVAR_REGISTER(&oz_recoilxbow);
	CVAR_REGISTER(&oz_recoilrpg);
	CVAR_REGISTER(&oz_recoilshotgun);
	CVAR_REGISTER(&oz_recoilshotgun2);
	CVAR_REGISTER(&oz_recoil357);
	CVAR_REGISTER(&oz_recoilgauss);
	CVAR_REGISTER(&oz_recoilhivehand);
	CVAR_REGISTER(&oz_cyclecrowbar);
	CVAR_REGISTER(&oz_cyclecrowbar2);
	CVAR_REGISTER(&oz_cycleglock);
	CVAR_REGISTER(&oz_cyclesglock);
	CVAR_REGISTER(&oz_cycleglock2);
	CVAR_REGISTER(&oz_cyclemp5);
	CVAR_REGISTER(&oz_cyclemp5sniper);
	CVAR_REGISTER(&oz_cycle357);
	CVAR_REGISTER(&oz_cycle3572);
	CVAR_REGISTER(&oz_cycleshotgun);
	CVAR_REGISTER(&oz_cycleshotgun2);
	CVAR_REGISTER(&oz_cyclegauss);
	CVAR_REGISTER(&oz_cyclem203);
	CVAR_REGISTER(&oz_cyclexbow);
	CVAR_REGISTER(&oz_cyclexbow2);
	CVAR_REGISTER(&oz_cyclehivehand);
	CVAR_REGISTER(&oz_cyclehivehand2);
	CVAR_REGISTER(&oz_speedhgrenade);
	CVAR_REGISTER(&oz_speedm203);
	CVAR_REGISTER(&oz_speedrpgwater);
	CVAR_REGISTER(&oz_speedrpgair);
	CVAR_REGISTER(&oz_speedsatchel);
	CVAR_REGISTER(&oz_speedxbowair);
	CVAR_REGISTER(&oz_speedxbowwater);
	CVAR_REGISTER(&oz_speedhornet);
	CVAR_REGISTER(&oz_speedhornet2);
	CVAR_REGISTER(&oz_reloadglock);
	CVAR_REGISTER(&oz_reload357);
	CVAR_REGISTER(&oz_reloadmp5);
	CVAR_REGISTER(&oz_reloadshotgun);
	CVAR_REGISTER(&oz_reloadxbow);
	CVAR_REGISTER(&oz_reloadrpg);
	CVAR_REGISTER(&oz_mp5tracers);
	CVAR_REGISTER(&oz_mp5spread);
	CVAR_REGISTER(&oz_mp5sniper);
	CVAR_REGISTER(&oz_silencemp5);
	CVAR_REGISTER(&oz_glocksniper);
	CVAR_REGISTER(&oz_silenceglock);
	CVAR_REGISTER(&oz_pelletsshotgun);
	CVAR_REGISTER(&oz_pelletsdoubleshotgun);
	CVAR_REGISTER(&oz_snarkhealth);
	CVAR_REGISTER(&oz_snarktime);
	CVAR_REGISTER(&oz_gaussmaxchargetime);
	CVAR_REGISTER(&oz_grenadetime);
	CVAR_REGISTER(&oz_flaretime);
	CVAR_REGISTER(&oz_waterweapons);
	CVAR_REGISTER(&oz_zoomamountglock);
	CVAR_REGISTER(&oz_zoomamountmp5);
	CVAR_REGISTER(&oz_zoomamountxbow);
	CVAR_REGISTER(&oz_zoomamount357);
	CVAR_REGISTER(&oz_satchelchargetime);
	CVAR_REGISTER(&oz_satchelareadetect);
	CVAR_REGISTER(&oz_allowproximity);
	CVAR_REGISTER(&oz_sectripammoused);
	CVAR_REGISTER(&oz_shake_enable);
	CVAR_REGISTER(&oz_shake_amp);
	CVAR_REGISTER(&oz_shake_freq);
	CVAR_REGISTER(&oz_shake_dur);
	CVAR_REGISTER(&oz_shake_radius);
	CVAR_REGISTER(&oz_homing);
	CVAR_REGISTER(&bot_blood);
	CVAR_REGISTER(&bot_speed);
	CVAR_REGISTER(&bot_number);
	CVAR_REGISTER(&bot_skill);
	CVAR_REGISTER(&bot_skill_aa);
	CVAR_REGISTER(&bot_jump);
	CVAR_REGISTER(&bot_mercy);
	CVAR_REGISTER(&bot_chat);
	CVAR_REGISTER(&bot_enable);
	CVAR_REGISTER(&bot_debug);
	CVAR_REGISTER(&bot_dontchat);
	CVAR_REGISTER(&bot_observer);
	CVAR_REGISTER(&bot_name);
	CVAR_REGISTER(&bot_model);
	CVAR_REGISTER(&bot_top_color);
	CVAR_REGISTER(&bot_bottom_color);
	CVAR_REGISTER(&bot_trait_jump);
	CVAR_REGISTER(&bot_trait_accuracy);
	CVAR_REGISTER(&bot_trait_aggression);
	CVAR_REGISTER(&bot_trait_chat);
	CVAR_REGISTER(&bot_trait_perception);
	CVAR_REGISTER(&bot_trait_reflexes);
	CVAR_REGISTER(&bot_wp_crossbow);
	CVAR_REGISTER(&bot_wp_crowbar);
	CVAR_REGISTER(&bot_wp_egon);
	CVAR_REGISTER(&bot_wp_gauss);
	CVAR_REGISTER(&bot_wp_glock);
	CVAR_REGISTER(&bot_wp_handgrenade);
	CVAR_REGISTER(&bot_wp_hornetgun);
	CVAR_REGISTER(&bot_wp_mp5);
	CVAR_REGISTER(&bot_wp_python);
	CVAR_REGISTER(&bot_wp_rpg);
	CVAR_REGISTER(&bot_wp_satchel);
	CVAR_REGISTER(&bot_wp_shotgun);
	CVAR_REGISTER(&bot_wp_snark);
	CVAR_REGISTER(&bot_wp_tripmine);
	CVAR_REGISTER(&cvar_bot);
	CVAR_REGISTER(&oz_minclients);
	CVAR_REGISTER(&oz_minbot);
	CVAR_REGISTER(&runfolder);
	CVAR_REGISTER(&bot_aa_max);
	CVAR_REGISTER(&sv_pushable_fixed_tick_fudge); // halflife-updated's

	InitMapLoadingUtils();

	SERVER_COMMAND("exec skill.cfg\n");
	SERVER_COMMAND("exec cfg/multiplayer.cfg\n");
	SERVER_COMMAND("exec cfg/weapons.cfg\n");
	SERVER_COMMAND("exec cfg/player.cfg\n");
	SERVER_COMMAND("exec cfg/grapple.cfg\n");
	SERVER_COMMAND("exec cfg/colors.cfg\n");
	SERVER_COMMAND("exec cfg/special.cfg\n");
	SERVER_COMMAND("exec cfg/rhobot.cfg\n");
}

void GameDLLShutdown()
{
	FileSystem_FreeFileSystem();
}
