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

// OZ's hl_wpn_glock.cpp (reference/funcfile.tsv). OZ replaced the SDK's event-driven glock with a
// server-side one: its own sounds and brass, no PLAYBACK_EVENT, a zoom on secondary attack (the
// "glock sniper") and the Super Glock Rune. Valve's client still predicts the stock glock for the
// shooter, so the shooter's own view mispredicts, as it did in 2004.

LINK_ENTITY_TO_CLASS(weapon_glock, CGlock);		 // orig 0x130360
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CGlock); // orig 0x130380

// orig 0x1303a0
void CGlock::Spawn()
{
	if (oz_disableglock.value > 0)
	{
		Kill();
		return;
	}

	SET_MODEL(ENT(pev), oz_silenceglock.value > 0 ? "models/w_silencer.mdl" : "models/w_9mmhandgun.mdl");
	m_iDefaultAmmo = (int)oz_givewithglock.value;
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;

	FallInit(); // get ready to fall down.
}


// orig 0x130490
void CGlock::Precache()
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_silencer.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/pl_gun1.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun2.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/pl_gun3.wav"); //handgun
	PRECACHE_SOUND("turret/tu_fire1.wav"); // Super Glock Rune, zoomed
}

// orig 0x130580
bool CGlock::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = (int)oz_max9mm.value;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = (int)oz_magazineglock.value;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = (int)oz_weightglock.value;

	return true;
}

// orig 0x130670
bool CGlock::Deploy()
{
	// Oz silencer
	if (oz_silenceglock.value > 0)
		pev->body = 1;
	else
		pev->body = 0;

	return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded");
}

// orig 0x1306f0. Unlike CBasePlayerWeapon::Holster it leaves the view and weapon models alone.
void CGlock::Holster()
{
	m_fInReload = false;

	if (0 != m_fInZoom)
		SecondaryAttack();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

// orig 0x130760. OZ: zoom in to oz_zoomamountglock and back.
void CGlock::SecondaryAttack()
{
	if (m_pPlayer->pev->fov == oz_zoomamountglock.value)
	{
		m_pPlayer->m_iFOV = 0;
		m_pPlayer->pev->fov = 0;
		m_fInZoom = 0;
	}
	else if (m_pPlayer->pev->fov != oz_zoomamountglock.value)
	{
		m_pPlayer->m_iFOV = (int)oz_zoomamountglock.value;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV;
		m_fInZoom = 1;
	}

	m_flNextSecondaryAttack = 0.75;
}

// orig 0x130830. OZ: one function for both modes (the SDK's GlockFire is gone). Zoomed, the glock is
// quiet, has no muzzle flash and fires the sniper bullet; with the Super Glock Rune it also costs no
// ammo, fires the superglock bullet and has its own cycle and kick.
void CGlock::PrimaryAttack()
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	if (m_fInZoom == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

		if (m_pPlayer->m_iRune == 11) // Super Glock Rune
			m_iClip++;
	}
	else
	{
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	m_iClip--;

	if (m_fInZoom < 1)
		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// sic: GLOCK_SHOOT or GLOCK_SHOOT_EMPTY at random, whatever the clip holds
	SendWeaponAnim(GLOCK_SHOOT + RANDOM_LONG(0, 1));

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (0 != m_fInZoom && m_pPlayer->m_iRune == 11)
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, ATTN_NORM, 0, 80 + RANDOM_LONG(0, 15));
	}
	else if (m_fInZoom == 1)
	{
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
		case 2:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 15));
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun2.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 15));
			break;
		}
	}
	else
	{
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
		case 1:
		case 2:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 15));
			break;
		}
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	// in the original's order: the right then the up random
	const float flRight = RANDOM_FLOAT(50, 70);
	const float flUp = RANDOM_FLOAT(100, 150);
	Vector vecShellVelocity = m_pPlayer->pev->velocity + gpGlobals->v_right * flRight + gpGlobals->v_up * flUp + gpGlobals->v_forward * 25;
	EjectBrass(pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_fInZoom == 1)
	{
		if (m_pPlayer->m_iRune == 11)
			m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_9MM_SUPERGLOCK, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed);
		else
			m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_9MM_SNIPER, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	if (m_fInZoom == 1)
	{
		if (m_pPlayer->m_iRune == 4) // Haste Rune
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cycleglock2.value * 0.5;
		}
		else if (m_pPlayer->m_iRune == 11)
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclesglock.value;
			m_pPlayer->pev->punchangle.x -= 15;
		}
		else
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cycleglock2.value;
		}
	}
	else
	{
		if (m_pPlayer->m_iRune == 4)
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cycleglock.value * 0.5;
		else
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cycleglock.value;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	m_pPlayer->pev->punchangle.x -= oz_recoilglock.value;
}


// orig 0x130ff0
void CGlock::Reload()
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	if (m_fInZoom == 1)
		SecondaryAttack();

	bool iResult;

	if (m_iClip == 0)
		iResult = DefaultReload((int)oz_magazineglock.value, GLOCK_RELOAD, oz_reloadglock.value);
	else
		iResult = DefaultReload((int)(oz_magazineglock.value + 1), GLOCK_RELOAD_NOT_EMPTY, oz_reloadglock.value);

	if (iResult)
	{
		// sic: gpGlobals->time on a weapon whose timers run from 0 (UTIL_WeaponTimeBase), so after a
		// reload the glock doesn't idle again for as long as the map has been running (ORIGINAL_BUGS)
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(10, 15);
	}
}



// orig 0x1310e0
void CGlock::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim(iAnim);
	}
}








class CGlockAmmo : public CBasePlayerAmmo
{
	// orig 0x143510
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	// orig 0x1434d0
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	// orig 0x143410
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo((int)oz_giveglockclip.value, "9mm", (int)oz_max9mm.value) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_glockclip, CGlockAmmo); // orig 0x1311f0
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CGlockAmmo);	  // orig 0x131210
