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
#include "game.h"


#define HANDGRENADE_PRIMARY_VOLUME 450

LINK_ENTITY_TO_CLASS(weapon_handgrenade, CHandGrenade); // orig 0xc78e0


// orig 0xc7900. Oz: disable_hgrenade removes it; damage_hgrenade, give_with_hgrenade.
void CHandGrenade::Spawn()
{
	if (oz_disablehgrenade.value > 0)
	{
		Kill();
		return;
	}

	Precache();
	m_iId = WEAPON_HANDGRENADE;
	SET_MODEL(ENT(pev), "models/w_grenade.mdl");

#ifndef CLIENT_DLL
	// Oz variables (.value)
	pev->dmg = oz_damagehgrenade.value;
#endif

	m_iDefaultAmmo = (int)oz_givewithhgrenade.value;

	FallInit(); // get ready to fall down.
}


// orig 0xc79b0. Oz: the flare's model.
void CHandGrenade::Precache()
{
	PRECACHE_MODEL("models/w_grenade.mdl");
	PRECACHE_MODEL("models/v_grenade.mdl");
	PRECACHE_MODEL("models/p_grenade.mdl");
	PRECACHE_MODEL("models/w_flare.mdl");
}

// orig 0xc7a10. Oz: max_hgrenades, weight_hgrenade.
bool CHandGrenade::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = (int)oz_maxhgrenades.value;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_HANDGRENADE;
	p->iWeight = (int)oz_weighthgrenade.value; // Oz weight
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return true;
}


// orig 0xc7ae0
bool CHandGrenade::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy("models/v_grenade.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "crowbar");
}

// orig 0xc7b30
bool CHandGrenade::CanHolster()
{
	// can only holster hand grenades when not primed!
	return (m_flStartThrow == 0);
}

// orig 0xc7b60
void CHandGrenade::Holster()
{
	// halflife-updated's fix, kept: a primed grenade is disarmed when the weapon goes away
	// (70dd419, halflife#3251); the original keeps m_flStartThrow (CanHolster stops a normal
	// switch while primed, so only a forced holster or a drop is affected)
	//Stop any throw that was in process so players don't blow themselves or somebody else up when the weapon is deployed again.
	m_flStartThrow = 0;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		SendWeaponAnim(HANDGRENADE_HOLSTER);
	}
	else
	{
		// no more grenades!
		m_pPlayer->ClearWeaponBit(m_iId);
		SetThink(&CHandGrenade::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

// orig 0xc7c50. Oz: a grenade, not a flare.
void CHandGrenade::PrimaryAttack()
{
	if (0 == m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_pPlayer->Flare = false; // Oz flare
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim(HANDGRENADE_PINPULL);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}


// orig 0xc7cf0. Oz flare: primes a flare, when flare_time isn't 0.
void CHandGrenade::SecondaryAttack()
{
	// Oz variables (.value)
	if (oz_flaretime.value != 0 && 0 == m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_pPlayer->Flare = true; // Oz flare
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim(HANDGRENADE_PINPULL);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}


// orig 0xc7da0. Oz: speed_hgrenade's throw speed (SDK 2.3's 500 cap; halflife-updated's 6.5 and
// 1000 are HL25's, ae352a6: design); grenade_time or, for a flare, flare_time.
void CHandGrenade::WeaponIdle()
{
	if (m_flReleaseThrow == 0 && 0 != m_flStartThrow)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (0 != m_flStartThrow)
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		// Oz variables (.value)
		float flVel = (90 - angThrow.x) * oz_speedhgrenade.value;
		if (flVel > 500)
			flVel = 500;

		UTIL_MakeVectors(angThrow);

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		// Oz flare
		if (!m_pPlayer->Flare)
		{
			// Oz variables (.value)
			float time = m_flStartThrow - gpGlobals->time + oz_grenadetime.value;
			if (time < 0)
				time = 0;

			CGrenade::ShootTimed(m_pPlayer->pev, vecSrc, vecThrow, time);
		}
		else
		{
			// Oz variables (.value)
			float time = m_flStartThrow - gpGlobals->time + oz_flaretime.value;
			if (time < 0)
				time = 0;

			CGrenade::ShootFlare(m_pPlayer->pev, vecSrc, vecThrow, time);
		}

		if (flVel < 500)
		{
			SendWeaponAnim(HANDGRENADE_THROW1);
		}
		else if (flVel < 1000)
		{
			SendWeaponAnim(HANDGRENADE_THROW2);
		}
		else
		{
			SendWeaponAnim(HANDGRENADE_THROW3);
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		// halflife-updated's fix, kept: m_flReleaseThrow stays set, so the draw animation (or the
		// retire after the last grenade) follows the throw (4bb1f3c, halflife#2495); the original
		// zeroes it here as SDK 2.3 does
		//m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		// halflife-updated's fix, kept (Steven 2026-11-07): GetNextAttackDelay evens out the fire-rate creep
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if (0 == m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			// halflife-updated's fix, kept (Steven 2026-11-07): GetNextAttackDelay evens out the fire-rate creep
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5); // ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim(HANDGRENADE_DRAW);
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		m_flReleaseThrow = -1;
		return;
	}

	if (0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
		}
		else
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim(iAnim);
	}
}
