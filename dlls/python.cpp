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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "game.h"
#include "items.h"

LINK_ENTITY_TO_CLASS(weapon_python, CPython); // orig 0xfce10
LINK_ENTITY_TO_CLASS(weapon_357, CPython);	  // orig 0xfce30

// orig 0xfce50
bool CPython::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = (int)oz_max357rounds.value;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = (int)oz_magazine357.value;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = (int)oz_weight357.value;

	return true;
}

// orig 0xfcf40. SDK 2.3's: halflife-updated sends WeapPickup from CBasePlayer::AddPlayerItem instead,
// a design change by FIXES.md (as mod-rc2), so the original's message stays here.
void CPython::AddToPlayer(CBasePlayer* pPlayer)
{
	CBasePlayerWeapon::AddToPlayer(pPlayer);

	MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
	WRITE_BYTE(m_iId);
	MESSAGE_END();
}

// orig 0xfcfd0
void CPython::Spawn()
{
	if (oz_disable357.value > 0)
	{
		Kill();
		return;
	}

	pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_PYTHON;
	SET_MODEL(ENT(pev), "models/w_357.mdl");

	m_iDefaultAmmo = (int)oz_givewith357.value;

	FallInit(); // get ready to fall down.
}


// orig 0xfd090
void CPython::Precache()
{
	PRECACHE_MODEL("models/v_357.mdl");
	PRECACHE_MODEL("models/w_357.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/357_reload1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("weapons/357_shot1.wav");
	PRECACHE_SOUND("weapons/357_shot2.wav");

	m_usFirePython = PRECACHE_EVENT(1, "events/python.sc");
}

// orig 0xfd170
bool CPython::Deploy()
{
#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	return DefaultDeploy("models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", pev->body);
}


// orig 0xfd200
void CPython::Holster()
{
	m_fInReload = false; // cancel any reload in progress.

	if (0 != m_fInZoom)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(PYTHON_HOLSTER);
}

// orig 0xfd2a0. OZ: zoom to oz_zoomamount357 instead of 40. The zoom state is the original's m_fInZoom
// and pev->fov, which halflife-updated dropped (d4b0e77, 6f8aac9) for a singleplayer save/restore
// problem that a multiplayer-only mod doesn't have.
void CPython::SecondaryAttack()
{
#ifdef CLIENT_DLL
	if (!bIsMultiplayer())
#else
	if (!g_pGameRules->IsMultiplayer())
#endif
	{
		return;
	}

	if (m_pPlayer->pev->fov == oz_zoomamount357.value)
	{
		m_fInZoom = 0;
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_pPlayer->pev->fov = 0;
	}
	else if (m_pPlayer->pev->fov != oz_zoomamount357.value)
	{
		m_fInZoom = 1;
		m_pPlayer->m_iFOV = (int)oz_zoomamount357.value;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV;
	}

	m_flNextSecondaryAttack = 0.75;
}

// orig 0xfd3a0. OZ: fires underwater if oz_waterweapons, a 2-degree cone (1 degree zoomed, with the
// 357 sniper bullet and a tracer), the haste rune cycle and oz_recoil357 kick.
void CPython::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && oz_waterweapons.value == 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		// halflife-updated's fix, kept: the original's EMIT_SOUND of 357_cock1 (and a Reload when not
		// m_fFireOnEmpty, which ItemPostFrame always sets here) played the dry fire every 0.15 s while
		// held (75bc5af, halflife#617)
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);


	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecDir;
	if (0 != m_fInZoom)
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357_SNIPER, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	else
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	// OZ: underwater (oz_waterweapons) the event goes to the shooter too, since Valve's client doesn't
	// predict a shot underwater
	int flags = m_pPlayer->pev->waterlevel == 3 ? 0 : FEV_NOTHOST;

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFirePython, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	// the haste rune cuts the cycle to 0.7 zoomed and 0.6 otherwise, not 0.5 as elsewhere
	if (0 != m_fInZoom)
	{
		if (m_pPlayer->m_iRune == RUNE_HASTE)
			m_flNextPrimaryAttack = oz_cycle3572.value * 0.7;
		else
			m_flNextPrimaryAttack = oz_cycle3572.value;
	}
	else
	{
		if (m_pPlayer->m_iRune == RUNE_HASTE)
			m_flNextPrimaryAttack = oz_cycle357.value * 0.6;
		else
			m_flNextPrimaryAttack = oz_cycle357.value;
	}

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	m_pPlayer->pev->punchangle.x -= oz_recoil357.value;
}


// orig 0xfd7a0
void CPython::Reload()
{
	if (m_pPlayer->ammo_357 <= 0)
		return;

	if (m_pPlayer->pev->fov != 0)
	{
		m_fInZoom = 0;
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_pPlayer->pev->fov = 0;
	}

	// OZ passes body 0 where the SDK passed the multiplayer scope
	if (m_pPlayer->m_iRune == RUNE_HASTE)
	{
		if (DefaultReload((int)oz_magazine357.value, PYTHON_RELOAD, oz_reload357.value * 0.5, 0))
			m_flSoundDelay = 0.75;
	}
	else
	{
		if (DefaultReload((int)oz_magazine357.value, PYTHON_RELOAD, oz_reload357.value, 0))
			m_flSoundDelay = 1.5;
	}
}


// orig 0xfd8d0
void CPython::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if (0 != m_flSoundDelay && m_flSoundDelay <= UTIL_WeaponTimeBase())
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	// sic: OZ draws from 10 to 15, not 0 to 1, so the idle is always the fidget (ORIGINAL_BUGS)
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	if (flRand <= 0.5)
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = (70.0 / 30.0);
	}
	else if (flRand <= 0.7)
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = (60.0 / 30.0);
	}
	else if (flRand <= 0.9)
	{
		iAnim = PYTHON_IDLE3;
		m_flTimeWeaponIdle = (88.0 / 30.0);
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = (170.0 / 30.0);
	}

	SendWeaponAnim(iAnim, 0);
}


class CPythonAmmo : public CBasePlayerAmmo
{
	// orig 0x1406f0
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_357ammobox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	// orig 0x1406b0
	void Precache() override
	{
		PRECACHE_MODEL("models/w_357ammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	// orig 0x1405f0
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo((int)oz_give357rounds.value, "357", (int)oz_max357rounds.value) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_357, CPythonAmmo); // orig 0xfda60
