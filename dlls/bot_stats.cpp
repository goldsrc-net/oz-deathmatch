// Oz Deathmatch 3.02: bot_stats.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).
//
// A bot's personality. The constructor rolls random traits and weapon preferences; ReadBotFile
// replaces them with the bot_* cvars, which UseBotCycle set by exec'ing bots/<name>.cfg. The cfg
// file itself is only opened to see that it exists.

#include <fstream>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "bot.h"

CBaseBotStats::CBaseBotStats() // orig 0x08d9a0
{
	// One RANDOM_FLOAT per line, in this order (orig).
	m_flWpCrossbow = RANDOM_FLOAT(0, 20) + 70.0;
	m_flWpCrowbar = RANDOM_FLOAT(0, 20) + 10.0;
	m_flWpEgon = 90.0 + RANDOM_FLOAT(0, 10);
	m_flWpGauss = 55.0 + RANDOM_FLOAT(0, 20);
	m_flWpGlock = RANDOM_FLOAT(0, 10) + 0.0;
	m_flWpHandGrenade = RANDOM_FLOAT(0, 20) + 10.0;
	m_flWpHornetGun = RANDOM_FLOAT(0, 20) + 40.0;
	m_flWpMP5 = 80.0 + RANDOM_FLOAT(0, 20);
	m_flWpPython = RANDOM_FLOAT(0, 10) + 5.0;
	m_flWpRPG = RANDOM_FLOAT(0, 20) + 60.0;
	m_flWpSatchel = RANDOM_FLOAT(0, 14) + 0.0;
	m_flWpShotgun = RANDOM_FLOAT(0, 20) + 70.0;
	m_flWpSnark = RANDOM_FLOAT(0, 20) + 0.0;
	m_flWpTripMine = RANDOM_FLOAT(0, 10) + 0.0;
	m_flTraitAccuracy = 75.0 + RANDOM_FLOAT(0, 25);
	m_flTraitAggression = RANDOM_FLOAT(0, 70) + 10.0;
	m_flTraitChat = RANDOM_FLOAT(0, 60) + 0.0;
	m_flTraitJump = RANDOM_FLOAT(0, 94) + 5.0;
	m_flTraitPerception = 50.0 + RANDOM_FLOAT(0, 50);
	m_flTraitReflexes = 30.0 + RANDOM_FLOAT(0, 40);

	strcpy(m_szName, "Rho-Bot");
	strcpy(m_szModel, "gordon");
	strcpy(m_szTopColor, "255");
	strcpy(m_szBottomColor, "255");
}

// Move a 0-100 trait towards 100 (bot_skill above 50) or towards 0 (below 50); 50 leaves it.
float CBaseBotStats::FDifficultyAdjusted(float flTrait) // orig 0x08dcf0
{
	const float flSkill = bot_skill.value / 50.0 - 1.0;

	if (flSkill >= 0)
		return flTrait + flSkill * (100.0 - flTrait);
	return flTrait + flSkill * flTrait;
}

// How much the bot wants pItem at flDistance from its enemy; 0 for the satchel, the tripmine and
// anything unknown.
float CBaseBotStats::FindWeaponDesire(CBasePlayerItem* pItem, float flDistance) // orig 0x08dd50
{
	const char* pszClassname = STRING(pItem->pev->classname);

	if (0 == strcmp(pszClassname, "weapon_9mmhandgun"))
		return m_flWpGlock;
	if (0 == strcmp(pszClassname, "weapon_shotgun"))
		return m_flWpShotgun;
	if (0 == strcmp(pszClassname, "weapon_9mmAR"))
		return m_flWpMP5;
	if (0 == strcmp(pszClassname, "weapon_rpg"))
	{
		if (flDistance > 700.0)
			return m_flWpRPG;
	}
	else if (0 == strcmp(pszClassname, "weapon_crossbow"))
	{
		if (flDistance > 200.0)
			return m_flWpCrossbow;
	}
	else
	{
		if (0 == strcmp(pszClassname, "weapon_egon"))
			return m_flWpEgon;
		if (0 == strcmp(pszClassname, "weapon_gauss"))
			return m_flWpGauss;
		if (0 == strcmp(pszClassname, "weapon_hornetgun"))
			return m_flWpHornetGun;
		if (0 == strcmp(pszClassname, "weapon_crowbar"))
		{
			if (flDistance < 200.0)
				return m_flWpCrowbar;
		}
		else if (0 == strcmp(pszClassname, "weapon_snark"))
		{
			if (flDistance > 200.0)
				return m_flWpSnark;
		}
		else
		{
			if (0 == strcmp(pszClassname, "weapon_357"))
				return m_flWpPython;
			if (0 == strcmp(pszClassname, "weapon_handgrenade") && flDistance > 700.0)
				return m_flWpHandGrenade;
		}
	}

	return 0;
}

// True when pszFile opens. The values come from the cvars, truncated to whole numbers.
bool CBaseBotStats::ReadBotFile(char* pszFile) // orig 0x08e0b0
{
	std::ifstream file(pszFile);

	const bool fOpen = file.is_open();
	if (fOpen)
	{
		char szName[64];
		char szModel[64];
		char szTopColor[4];
		char szBottomColor[4];

		// The original strcpy's the cvars, so a name or model over 63 characters or a colour over 3
		// overruns these buffers (and the members after). Ours cuts them to fit
		// (reference/DEVIATIONS.md, Crashes in the original).
		snprintf(szName, sizeof(szName), "%s", CVAR_GET_STRING("bot_name"));
		snprintf(szModel, sizeof(szModel), "%s", CVAR_GET_STRING("bot_model"));
		snprintf(szTopColor, sizeof(szTopColor), "%s", CVAR_GET_STRING("bot_top_color"));
		snprintf(szBottomColor, sizeof(szBottomColor), "%s", CVAR_GET_STRING("bot_bottom_color"));

		strcpy(m_szName, szName);
		strcpy(m_szModel, szModel);
		strcpy(m_szTopColor, szTopColor);
		strcpy(m_szBottomColor, szBottomColor);

		m_flTraitJump = (int)CVAR_GET_FLOAT("bot_trait_jump");
		m_flTraitAccuracy = (int)CVAR_GET_FLOAT("bot_trait_accuracy");
		m_flTraitAggression = (int)CVAR_GET_FLOAT("bot_trait_aggression");
		m_flTraitChat = (int)CVAR_GET_FLOAT("bot_trait_chat");
		m_flTraitPerception = (int)CVAR_GET_FLOAT("bot_trait_perception");
		m_flTraitReflexes = (int)CVAR_GET_FLOAT("bot_trait_reflexes");
		m_flWpCrossbow = (int)CVAR_GET_FLOAT("bot_wp_crossbow");
		m_flWpCrowbar = (int)CVAR_GET_FLOAT("bot_wp_crowbar");
		m_flWpEgon = (int)CVAR_GET_FLOAT("bot_wp_egon");
		m_flWpGauss = (int)CVAR_GET_FLOAT("bot_wp_gauss");
		m_flWpGlock = (int)CVAR_GET_FLOAT("bot_wp_glock");
		m_flWpHandGrenade = (int)CVAR_GET_FLOAT("bot_wp_handgrenade");
		m_flWpHornetGun = (int)CVAR_GET_FLOAT("bot_wp_hornetgun");
		m_flWpMP5 = (int)CVAR_GET_FLOAT("bot_wp_mp5");
		m_flWpPython = (int)CVAR_GET_FLOAT("bot_wp_python");
		m_flWpRPG = (int)CVAR_GET_FLOAT("bot_wp_rpg");
		m_flWpSatchel = (int)CVAR_GET_FLOAT("bot_wp_satchel");
		m_flWpShotgun = (int)CVAR_GET_FLOAT("bot_wp_shotgun");
		m_flWpSnark = (int)CVAR_GET_FLOAT("bot_wp_snark");
		m_flWpTripMine = (int)CVAR_GET_FLOAT("bot_wp_tripmine");
	}

	return fOpen;
}
