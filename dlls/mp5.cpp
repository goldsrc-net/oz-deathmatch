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
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "game.h"
#include "items.h"

// OZ's MP5 fires on the server only, like its glock: its own sounds and brass, no PLAYBACK_EVENT, an
// optional silencer (oz_silencemp5), a sniper zoom on secondary attack (oz_mp5sniper 1) in place of the
// grenade, and a spread cone picked by oz_mp5spread. Valve's client still predicts the stock MP5 for
// the shooter.

LINK_ENTITY_TO_CLASS(weapon_mp5, CMP5);	  // orig 0xd9040
LINK_ENTITY_TO_CLASS(weapon_9mmAR, CMP5); // orig 0xd9060


// orig 0xd9080. SDK 2.3's; halflife-updated dropped it for the base class's, which returns the same.
int CMP5::SecondaryAmmoIndex()
{
	return m_iSecondaryAmmoType;
}

//=========================================================
//=========================================================
// orig 0xd9090
void CMP5::Spawn()
{
	if (oz_disablemp5.value == 1)
	{
		Kill();
		return;
	}

	pev->classname = MAKE_STRING("weapon_9mmAR"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = (int)oz_givewithmp5.value;

	FallInit(); // get ready to fall down.
}


// orig 0xd9150
void CMP5::Precache()
{
	PRECACHE_MODEL("models/v_9mmAR.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl"); // grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/hks1.wav"); // H to the K
	PRECACHE_SOUND("weapons/hks2.wav"); // H to the K

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	PRECACHE_SOUND("weapons/bullet_hit1.wav");
	PRECACHE_SOUND("weapons/bullet_hit2.wav");
	PRECACHE_SOUND("weapons/pl_gun1.wav"); // silenced
	PRECACHE_SOUND("weapons/pl_gun2.wav"); // silenced
}

// orig 0xd92c0
bool CMP5::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = (int)oz_max9mm.value;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = (int)oz_maxm203.value;
	p->iMaxClip = (int)oz_magazinemp5.value;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = (int)oz_weightmp5.value;

	return true;
}

// orig 0xd93d0. SDK 2.3's WeapPickup, kept here as CPython::AddToPlayer explains.
void CMP5::AddToPlayer(CBasePlayer* pPlayer)
{
	CBasePlayerWeapon::AddToPlayer(pPlayer);

	MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
	WRITE_BYTE(m_iId);
	MESSAGE_END();
}

// orig 0xd9460
bool CMP5::Deploy()
{
	return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DEPLOY, "mp5");
}

// orig 0xd94a0. Unlike CBasePlayerWeapon::Holster it leaves the view and weapon models alone.
void CMP5::Holster()
{
	m_fInReload = false;

	if (0 != OzZoom)
		SecondaryAttack();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


// orig 0xd9530
void CMP5::PrimaryAttack()
{
	const int iSpread = (int)oz_mp5spread.value;

	// don't fire underwater
	if ((m_pPlayer->pev->waterlevel == 3 && oz_waterweapons.value == 0) || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (oz_silencemp5.value == 0)
	{
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}
	else
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}

	m_iClip--;

	if (oz_silencemp5.value == 0)
		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	SendWeaponAnim(MP5_FIRE1 + RANDOM_LONG(0, 2));
	m_flNextAnimTime = gpGlobals->time + 0.2;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	// sic: the silenced sound only at exactly 1; any other nonzero value is quiet with the loud sound
	if (oz_silencemp5.value == 1)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 0xf));
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun2.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 0xf));
			break;
		}
	}
	else
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 0xf));
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 0xf));
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

	if (m_pPlayer->m_iRune == RUNE_HASTE) // Haste Rune: a 10-degree cone and three times the kick
	{
		m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 8192, BULLET_PLAYER_MP5, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
		m_pPlayer->pev->punchangle.x -= oz_recoilmp5.value + oz_recoilmp5.value;
	}
	else if (0 != OzZoom)
	{
		m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_MP5_SNIPER, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// oz_mp5spread picks the cone in degrees; sic: outside 1-10 no bullet is fired, though the
		// round is spent (ORIGINAL_BUGS)
		Vector vecSpread;
		bool bFire = true;
		switch (iSpread)
		{
		case 1: vecSpread = VECTOR_CONE_1DEGREES; break;
		case 2: vecSpread = VECTOR_CONE_2DEGREES; break;
		case 3: vecSpread = VECTOR_CONE_3DEGREES; break;
		case 4: vecSpread = VECTOR_CONE_4DEGREES; break;
		case 5: vecSpread = VECTOR_CONE_5DEGREES; break;
		case 6: vecSpread = VECTOR_CONE_6DEGREES; break;
		case 7: vecSpread = VECTOR_CONE_7DEGREES; break;
		case 8: vecSpread = VECTOR_CONE_8DEGREES; break;
		case 9: vecSpread = VECTOR_CONE_9DEGREES; break;
		case 10: vecSpread = VECTOR_CONE_10DEGREES; break;
		default: bFire = false; break;
		}

		if (bFire)
			m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MP5, (int)oz_mp5tracers.value, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	// SDK 2.3's accumulating cycle, with OZ's times (halved by the Haste Rune)
	float flCycle = 0 != OzZoom ? oz_cyclemp5sniper.value : oz_cyclemp5.value;
	if (m_pPlayer->m_iRune == RUNE_HASTE)
		flCycle = flCycle * 0.5;

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + flCycle;
	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flCycle;

	m_pPlayer->pev->punchangle.x -= oz_recoilmp5.value;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	// sic: unzoomed, this replaces the kick above (SDK 2.3's RANDOM_FLOAT(-2, 2), scaled)
	if (0 == OzZoom)
		m_pPlayer->pev->punchangle.x = RANDOM_FLOAT(-oz_recoilmp5.value, oz_recoilmp5.value);
}



// orig 0xda090. OZ: with oz_mp5sniper 1 this zooms instead of firing the grenade.
void CMP5::SecondaryAttack()
{
	if (oz_mp5sniper.value == 1)
	{
		if (m_pPlayer->pev->fov != 0)
		{
			m_pPlayer->m_iFOV = 0;
			m_pPlayer->pev->fov = 0;
			OzZoom = 0;
		}
		else if (m_pPlayer->pev->fov != oz_zoomamountmp5.value)
		{
			// sic: only pev->fov, not m_iFOV, so the zoom shows through clientdata's fov alone
			m_pPlayer->pev->fov = oz_zoomamountmp5.value;
			OzZoom = 1;
		}

		pev->nextthink = gpGlobals->time + 0.1;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && oz_waterweapons.value == 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.2;

	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	SendWeaponAnim(MP5_LAUNCH);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	if (0 != RANDOM_LONG(0, 1))
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher2.wav", 0.8, ATTN_NORM);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	// we don't add in player velocity anymore.
	CGrenade::ShootContact(m_pPlayer->pev,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * oz_speedm203.value);

	if (m_pPlayer->m_iRune == RUNE_HASTE)
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclem203.value * 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cyclem203.value * 0.5;
	}
	else
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclem203.value;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cyclem203.value;
	}

	if (0 == m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_pPlayer->pev->punchangle.x -= oz_recoilm203.value;
}

// orig 0xda4f0
void CMP5::Reload()
{
	if (m_pPlayer->ammo_9mm <= 0 || (float)m_iClip >= oz_magazinemp5.value)
		return;

	if (0 != OzZoom)
		SecondaryAttack();

	float flDelay = oz_reloadmp5.value;
	if (m_pPlayer->m_iRune == RUNE_HASTE)
		flDelay = oz_reloadmp5.value * 0.5;

	DefaultReload((int)oz_magazinemp5.value, MP5_RELOAD, flDelay, 0);
}


// orig 0xda5d0
void CMP5::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = MP5_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}



class CMP5AmmoClip : public CBasePlayerAmmo
{
	// orig 0x13f4d0
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	// orig 0x13f490
	void Precache() override
	{
		PRECACHE_MODEL("models/w_9mmARclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	// orig 0x13f3d0
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo((int)oz_givemp5clip.value, "9mm", (int)oz_max9mm.value) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5clip, CMP5AmmoClip); // orig 0xda690
LINK_ENTITY_TO_CLASS(ammo_9mmAR, CMP5AmmoClip);	  // orig 0xda6b0



class CMP5Chainammo : public CBasePlayerAmmo
{
	// orig 0x13f330
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	// orig 0x13f2f0
	void Precache() override
	{
		PRECACHE_MODEL("models/w_chainammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	// orig 0x13f230
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo((int)oz_givechainbox.value, "9mm", (int)oz_max9mm.value) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_9mmbox, CMP5Chainammo); // orig 0xda6d0


class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	// orig 0x13f180. OZ: with oz_disablem203 1 or more the entity stays, never set up (no model, no
	// touch); the original doesn't remove it.
	void Spawn() override
	{
		if (oz_disablem203.value < 1)
		{
			Precache();
			SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
			CBasePlayerAmmo::Spawn();
		}
	}
	// orig 0x13f140
	void Precache() override
	{
		PRECACHE_MODEL("models/w_ARgrenade.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	// orig 0x13f080
	bool AddAmmo(CBaseEntity* pOther) override
	{
		bool bResult = (pOther->GiveAmmo((int)oz_givem203.value, "ARgrenades", (int)oz_maxm203.value) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mp5grenades, CMP5AmmoGrenade); // orig 0xda6f0
LINK_ENTITY_TO_CLASS(ammo_ARgrenades, CMP5AmmoGrenade);	 // orig 0xda710
