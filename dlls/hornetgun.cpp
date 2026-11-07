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
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "game.h"

// No GetRechargeTime: halflife-updated's 0.3 s multiplayer recharge (37ed1d9, HL25) is a balance
// change; the original recharges a hornet every 0.5 s (0.25 s with the Haste Rune).

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};


LINK_ENTITY_TO_CLASS(weapon_hornetgun, CHgun); // orig 0xca230

// orig 0xca250
bool CHgun::IsUseable()
{
	return true;
}

// orig 0xca260. Oz: disable_hivehand removes it; give_with_hivehand hornets.
void CHgun::Spawn()
{
	if (oz_disablehivehand.value > 0)
	{
		Kill();
		return;
	}

	Precache();
	m_iId = WEAPON_HORNETGUN;
	SET_MODEL(ENT(pev), "models/w_hgun.mdl");

	// Oz variables (.value)
	m_iDefaultAmmo = (int)oz_givewithhivehand.value;
	m_iFirePhase = 0;

	FallInit(); // get ready to fall down.
}


// orig 0xca310
void CHgun::Precache()
{
	PRECACHE_MODEL("models/v_hgun.mdl");
	PRECACHE_MODEL("models/w_hgun.mdl");
	PRECACHE_MODEL("models/p_hgun.mdl");

	m_usHornetFire = PRECACHE_EVENT(1, "events/firehornet.sc");

	UTIL_PrecacheOther("hornet");
}

// orig 0xca390. SDK 2.3's: in multiplayer the player's hornets are set to max_hornets (capped at
// 250), then WeapPickup (design per mod-rc2's FIXES.md; halflife-updated sets the default ammo
// instead and announces the pickup from CBasePlayer::AddPlayerItem). VERIFY: here AddPlayerItem
// extracts the default ammo after this, capped at max_hornets, so the count matches the
// original's except when max_hornets is above 250.
void CHgun::AddToPlayer(CBasePlayer* pPlayer)
{
	int iMaxHornets = (int)oz_maxhornets.value;
	if (iMaxHornets > 250)
		iMaxHornets = 250;

	CBasePlayerWeapon::AddToPlayer(pPlayer);

#ifndef CLIENT_DLL
	if (g_pGameRules->IsMultiplayer())
	{
		// in multiplayer, all hivehands come full.
		pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = iMaxHornets;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
	WRITE_BYTE(m_iId);
	MESSAGE_END();
#endif
}

// orig 0xca490. Oz: max_hornets, weight_hivehand.
bool CHgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hornets";
	p->iMaxAmmo1 = (int)oz_maxhornets.value;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_HORNETGUN;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = (int)oz_weighthivehand.value; // Oz weight

	return true;
}


// orig 0xca560
bool CHgun::Deploy()
{
	return DefaultDeploy("models/v_hgun.mdl", "models/p_hgun.mdl", HGUN_UP, "hive");
}

// orig 0xca5a0
void CHgun::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(HGUN_DOWN);

	//!!!HACKHACK - can't select hornetgun if it's empty! no way to get ammo for it, either.
	if (0 == m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}


// orig 0xca640. Oz: cycle_hivehand's refire, halved with the Haste Rune. The ammo test is the
// original's on m_rgAmmo (halflife-updated reads ammo_hornets for its client, f347954).
void CHgun::PrimaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		return;
	}

#ifndef CLIENT_DLL
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	CBaseEntity* pHornet = CBaseEntity::Create("hornet", m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12, m_pPlayer->pev->v_angle, m_pPlayer->edict());
	pHornet->pev->velocity = gpGlobals->v_forward * 300;

	m_flRechargeTime = gpGlobals->time + 0.5;
#endif

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;


	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usHornetFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, FIREMODE_TRACK, 0, 0, 0);



	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Oz runes - haste
	// Oz variables (.value)
	if (m_pPlayer->m_iRune == 4) // Haste Rune
	{
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + oz_cyclehivehand.value * 0.5;

		if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclehivehand.value * 0.5;
		}
	}
	else
	{
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + oz_cyclehivehand.value;

		if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclehivehand.value;
		}
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}



// orig 0xca980. Oz: cycle_hivehand2's refire for both attacks, halved with the Haste Rune. The
// event's iparam1 is FIREMODE_FAST, as SDK 2.3's (Valve's client reads it); halflife-updated
// sends 0 (f347954). The idle time is set from gpGlobals->time, not UTIL_WeaponTimeBase(), so
// after a secondary shot the idle animations wait the server's uptime (original bug, kept).
void CHgun::SecondaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		return;
	}

	//Wouldn't be a bad idea to completely predict these, since they fly so fast...
#ifndef CLIENT_DLL
	CBaseEntity* pHornet;
	Vector vecSrc;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;

	m_iFirePhase++;
	switch (m_iFirePhase)
	{
	case 1:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		break;
	case 2:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 3:
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 4:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 5:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		break;
	case 6:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		break;
	case 7:
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		break;
	case 8:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		m_iFirePhase = 0;
		break;
	}

	pHornet = CBaseEntity::Create("hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict());
	pHornet->pev->velocity = gpGlobals->v_forward * 1200;
	pHornet->pev->angles = UTIL_VecToAngles(pHornet->pev->velocity);

	pHornet->SetThink(&CHornet::StartDart);

	m_flRechargeTime = gpGlobals->time + 0.5;
#endif

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usHornetFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, FIREMODE_FAST, 0, 0, 0);


	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Oz runes - haste
	// Oz variables (.value)
	if (m_pPlayer->m_iRune == 4) // Haste Rune
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cyclehivehand2.value * 0.5;
	else
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cyclehivehand2.value;

	m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


// orig 0xcaf90. Oz: up to max_hornets (compared as floats), twice as fast with the Haste Rune.
void CHgun::Reload()
{
#ifndef CLIENT_DLL
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= oz_maxhornets.value)
		return;

	while (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < oz_maxhornets.value && m_flRechargeTime < gpGlobals->time)
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		// Oz runes - haste
		if (m_pPlayer->m_iRune == 4) // Haste Rune
			m_flRechargeTime += 0.25;
		else
			m_flRechargeTime += 0.5;
	}

	// halflife-updated's fix, kept: ammo_hornets follows the recharge, for client prediction
	// (f347954, halflife#556); the original doesn't tabulate here
	m_pPlayer->TabulateAmmo();
#endif
}


// orig 0xcb0a0
void CHgun::WeaponIdle()
{
	Reload();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.75)
	{
		iAnim = HGUN_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = HGUN_FIDGETSWAY;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	else
	{
		iAnim = HGUN_FIDGETSHAKE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 35.0 / 16.0;
	}
	SendWeaponAnim(iAnim);
}
