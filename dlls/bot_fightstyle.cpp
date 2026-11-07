// Oz Deathmatch 3.02: bot_fightstyle.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).
//
// Per-weapon fire timing, aim point and fire mode. Called from CBaseBot::ActionChooseWeapon with
// the newly chosen weapon. UseCrossbow, UseMP5 and UseShotgun read the enemy without checking it
// in the original: a bot with no enemy that picks one of them (AimAtEntity on a tripmine leads
// there) dereferences NULL. Ours takes the distance as 0 then (BotEnemyDistance).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "bot.h"

CBaseBotFightStyle::CBaseBotFightStyle() // orig 0x086bd0
{
	m_iAimAt = BOT_AIM_BODY;
	m_fHoldAttack = 0;
	m_fSecondaryFire = 0;
	m_flNextShootTime = gpGlobals->time;
	// m_iAttackMode, m_pBot and m_flStopShootTime are not set here (orig); CBaseBot's
	// constructor sets m_pBot.
}

void CBaseBotFightStyle::DispatchWeaponUse(CBasePlayerItem* pActiveItem) // orig 0x086c40
{
	m_fHoldAttack = 0;

	const char* pszClassname = STRING(pActiveItem->pev->classname);
	if (0 == strcmp(pszClassname, "weapon_9mmhandgun"))
		UseGlock();
	else if (0 == strcmp(pszClassname, "weapon_shotgun"))
		UseShotgun();
	else if (0 == strcmp(pszClassname, "weapon_9mmAR"))
		UseMP5();
	else if (0 == strcmp(pszClassname, "weapon_rpg"))
		UseRPG();
	else if (0 == strcmp(pszClassname, "weapon_crossbow"))
		UseCrossbow();
	else if (0 == strcmp(pszClassname, "weapon_egon"))
		UseEgon();
	else if (0 == strcmp(pszClassname, "weapon_gauss"))
		UseGauss();
	else if (0 == strcmp(pszClassname, "weapon_hornetgun"))
		UseHornetGun();
	else if (0 == strcmp(pszClassname, "weapon_crowbar"))
		UseCrowbar();
	else if (0 == strcmp(pszClassname, "weapon_snark"))
		UseSnark();
	else if (0 == strcmp(pszClassname, "weapon_357"))
		UsePython();
	else if (0 == strcmp(pszClassname, "weapon_handgrenade"))
		UseHandGrenade();
	else if (0 == strcmp(pszClassname, "weapon_satchel"))
		UseSatchel();
	else if (0 == strcmp(pszClassname, "weapon_tripmine"))
		UseTripMine();
	else
		ALERT(at_aiconsole, "Unidentified Weapon!\n");
}

void CBaseBotFightStyle::OptimizeAttackMode() // orig 0x087010
{
	m_iAttackMode = 0;
}

// Aim at the head with iChance% of the (difficulty-adjusted) accuracy, always above 98.
void CBaseBotFightStyle::RandomizeAimAtHead(int iChance) // orig 0x087030
{
	const int iAccuracy = (int)m_pBot->Stats.FDifficultyAdjusted(m_pBot->Stats.m_flTraitAccuracy);

	if (RANDOM_LONG(0, 100) < (iChance * iAccuracy) / 100 || iAccuracy > 98)
		m_iAimAt = BOT_AIM_HEAD;
	else
		m_iAimAt = BOT_AIM_BODY;
}

void CBaseBotFightStyle::RandomizeSecondaryFire(int iChance) // orig 0x0870e0
{
	if (RANDOM_LONG(0, 100) < iChance)
		m_fSecondaryFire = 1;
	else
		m_fSecondaryFire = 0;
}

// Next shot after flDelay plus a whole number of seconds between the truncated bounds, scaled
// down by the reflexes. The bot keeps firing until m_flStopShootTime: 2 s when holding the
// button, else 0.1 s plus m_flMSec, which is in milliseconds (1-100) but added as seconds (orig).
void CBaseBotFightStyle::SetNextShootTime(float flDelay, float flMinRandom, float flMaxRandom) // orig 0x087140
{
	CBaseBot* pBot = m_pBot;
	const float flReflexScale = 1 - pBot->Stats.FDifficultyAdjusted(pBot->Stats.m_flTraitReflexes) / 100.0;

	float flBase = m_flNextShootTime;
	if (flBase <= gpGlobals->time)
		flBase = gpGlobals->time;

	const int iRandom = RANDOM_LONG((int)flMinRandom, (int)flMaxRandom);
	m_flNextShootTime = (flBase + flDelay) + flReflexScale * iRandom;

	if (0 != m_fHoldAttack)
		m_flStopShootTime = m_flNextShootTime + 2.0;
	else
		m_flStopShootTime = m_flNextShootTime + 0.1 + pBot->m_flMSec;
}

// The original reads m_hEnemy unchecked in UseCrossbow, UseMP5 and UseShotgun; with no enemy it
// dereferences NULL. Ours gives 0 (reference/DEVIATIONS.md, Crashes in the original).
static float BotEnemyDistance(CBaseBot* pBot)
{
	if (pBot->m_hEnemy == NULL)
		return 0;
	return (pBot->m_hEnemy->pev->origin - pBot->pev->origin).Length();
}

void CBaseBotFightStyle::UseCrossbow() // orig 0x087250
{
	const float flDist = BotEnemyDistance(m_pBot);

	int iChance;
	if (flDist > 700.0)
		iChance = 90;
	else if (flDist > 200.0)
		iChance = 60;
	else
		iChance = 20;
	RandomizeSecondaryFire(iChance);

	if (0 != m_fSecondaryFire)
	{
		RandomizeAimAtHead(60);
		SetNextShootTime(0.2, 0.0, 0.0);
	}
	else
	{
		m_iAimAt = BOT_AIM_ORIGIN;
		SetNextShootTime(0.75, 1.0, 1.3);
	}
}

void CBaseBotFightStyle::UseCrowbar() // orig 0x087360
{
	m_fSecondaryFire = 0;
	RandomizeAimAtHead(80);
	m_fHoldAttack = 1;
	SetNextShootTime(0.3, 0.6, 1.0);
}

void CBaseBotFightStyle::UseEgon() // orig 0x0873b0
{
	m_fSecondaryFire = 0;
	RandomizeAimAtHead(5);
	m_fHoldAttack = 1;
	SetNextShootTime(0.2, 0.0, 0.0);
}

void CBaseBotFightStyle::UseGauss() // orig 0x087400
{
	RandomizeSecondaryFire(70);

	if (0 != m_fSecondaryFire)
	{
		RandomizeAimAtHead(5);
		m_fHoldAttack = 1;
		SetNextShootTime(1.0, 1.2, 2.0);
	}
	else
	{
		RandomizeAimAtHead(20);
		SetNextShootTime(0.2, 1.0, 1.2);
	}
}

void CBaseBotFightStyle::UseGlock() // orig 0x087480
{
	RandomizeSecondaryFire(90);
	RandomizeAimAtHead(80);
	m_fHoldAttack = 1;
	SetNextShootTime(0.2, 0.2, 0.4);
}

void CBaseBotFightStyle::UseHandGrenade() // orig 0x0874d0
{
	m_fSecondaryFire = 0;
	m_iAimAt = BOT_AIM_ORIGIN;
	SetNextShootTime(0.1, 5.0, 7.0);
}

void CBaseBotFightStyle::UseHornetGun() // orig 0x087510
{
	RandomizeSecondaryFire(20);
	RandomizeAimAtHead(50);
	m_fHoldAttack = 1;
	SetNextShootTime(0.25, 1.0, 1.5);
}

void CBaseBotFightStyle::UseMP5() // orig 0x087560
{
	const float flDist = BotEnemyDistance(m_pBot);

	if (flDist < 200.0)
	{
		m_fSecondaryFire = 0;
		m_fHoldAttack = 1;
	}
	else
	{
		RandomizeSecondaryFire(10);
	}

	if (0 == m_fSecondaryFire)
	{
		RandomizeAimAtHead(30);
		m_fHoldAttack = 1;
		SetNextShootTime(0.1, 0.5, 0.8);
	}
	else
	{
		m_iAimAt = BOT_AIM_ORIGIN;
		SetNextShootTime(1.0, 1.4, 2.0);
	}
}

void CBaseBotFightStyle::UsePython() // orig 0x087670
{
	m_fSecondaryFire = 0;
	RandomizeAimAtHead(80);
	SetNextShootTime(0.75, 1.5, 2.2);
}

void CBaseBotFightStyle::UseRPG() // orig 0x0876c0
{
	m_fSecondaryFire = 0;
	m_iAimAt = BOT_AIM_ORIGIN;
	SetNextShootTime(1.5, 5.0, 7.0);
}

// Satchels and tripmines aren't used: choose another weapon.
void CBaseBotFightStyle::UseSatchel() // orig 0x087700
{
	m_pBot->ActionChooseWeapon();
}

void CBaseBotFightStyle::UseShotgun() // orig 0x087730
{
	const float flDist = BotEnemyDistance(m_pBot);

	RandomizeSecondaryFire(flDist > 700.0 ? 20 : 90);
	RandomizeAimAtHead(60);
	SetNextShootTime(0.75, 0.8, 2.0);
}

void CBaseBotFightStyle::UseSnark() // orig 0x087800
{
	m_fSecondaryFire = 0;
	m_iAimAt = BOT_AIM_ORIGIN;
	m_fHoldAttack = 1;
	SetNextShootTime(0.2, 0.6, 1.0);
}

void CBaseBotFightStyle::UseTripMine() // orig 0x087850
{
	m_pBot->ActionChooseWeapon();
}
