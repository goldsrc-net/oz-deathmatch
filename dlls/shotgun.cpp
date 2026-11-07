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
#include "gamerules.h"
#include "UserMessages.h"
#include "game.h"
#include "items.h"

// OZ's spreads (wide, flat), in place of VECTOR_CONE_DM_SHOTGUN and VECTOR_CONE_DM_DOUBLESHOTGUN
#define VECTOR_CONE_OZ_SHOTGUN Vector(0.17365, 0.02181, 0.00)
#define VECTOR_CONE_OZ_DOUBLESHOTGUN Vector(0.130238, 0.02181, 0.00)

LINK_ENTITY_TO_CLASS(weapon_shotgun, CShotgun); // orig 0x106070

// orig 0x106090
void CShotgun::Spawn()
{
	if (oz_disableshotgun.value == 1)
	{
		Kill();
		return;
	}

	Precache();
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = (int)oz_givewithshotgun.value;

	FallInit(); // get ready to fall
}


// orig 0x106130
void CShotgun::Precache()
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shotgunshell.mdl"); // shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/dbarrel1.wav"); //shotgun
	PRECACHE_SOUND("weapons/sbarrel1.wav"); //shotgun

	PRECACHE_SOUND("weapons/reload1.wav"); // shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav"); // shotgun reload

	//	PRECACHE_SOUND ("weapons/sshell1.wav");	// shotgun reload - played on client
	//	PRECACHE_SOUND ("weapons/sshell3.wav");	// shotgun reload - played on client

	PRECACHE_SOUND("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND("weapons/scock1.wav");	 // cock gun

	m_usSingleFire = PRECACHE_EVENT(1, "events/shotgun1.sc");
	m_usDoubleFire = PRECACHE_EVENT(1, "events/shotgun2.sc");
}

// orig 0x106260. SDK 2.3's WeapPickup, kept here as CPython::AddToPlayer explains.
void CShotgun::AddToPlayer(CBasePlayer* pPlayer)
{
	CBasePlayerWeapon::AddToPlayer(pPlayer);

	MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
	WRITE_BYTE(m_iId);
	MESSAGE_END();
}

// orig 0x1062f0
bool CShotgun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = (int)oz_maxshells.value;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = (int)oz_magazineshotgun.value;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = (int)oz_weightshotgun.value;

	return true;
}



// orig 0x1063e0
bool CShotgun::Deploy()
{
	return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun");
}

// Brass, as OZ's server-side fire ejects it (Valve's event does this on the client).
static void OzShotgunShell(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon, int iShell)
{
	// in the original's order: the right then the up random
	const float flRight = RANDOM_FLOAT(50, 70);
	const float flUp = RANDOM_FLOAT(100, 150);
	Vector vecShellVelocity = pPlayer->pev->velocity + gpGlobals->v_right * flRight + gpGlobals->v_up * flUp + gpGlobals->v_forward * 25;
	EjectBrass(pPlayer->pev->origin + pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 20 + gpGlobals->v_right * 4, vecShellVelocity, pWeapon->pev->angles.y, iShell, TE_BOUNCE_SHOTSHELL);
}

// orig 0x106420. OZ: pellets, spread, cycle and kick from cvars, fires underwater if
// oz_waterweapons. With the Haste Rune, underwater or a cycle under 0.3 s the anim, brass and sound
// come from the server, since Valve's client predicts the stock shotgun's timing.
void CShotgun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && oz_waterweapons.value == 0)
	{
		PlayEmptySound();
		// halflife-updated's fix, kept: GetNextAttackDelay evens out the fire-rate creep (seed; Steven
		// 2026-11-07: unclear calls go to halflife-updated). The original sets UTIL_WeaponTimeBase() + the cycle.
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		if (m_iClip == 0)
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecDir = m_pPlayer->FireBulletsPlayer((int)oz_pelletsshotgun.value, vecSrc, vecAiming, VECTOR_CONE_OZ_SHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	if (m_pPlayer->m_iRune == RUNE_HASTE || m_pPlayer->pev->waterlevel == 3 || oz_cycleshotgun.value < 0.3)
	{
		SendWeaponAnim(SHOTGUN_FIRE);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		OzShotgunShell(m_pPlayer, this, m_iShell);

		const int iPitch = 93 + RANDOM_LONG(0, 0x1f);
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sbarrel1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, iPitch);
	}
	else
	{
		// sic: no SetAnimation on this path; the shooter's model doesn't play the attack
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usSingleFire, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);
	}


	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	// halflife-updated's fix, kept: the original set this only with shells left, so the last shot
	// never pumped (a65846e, halflife#617; ItemPostFrame plays it)
	//if (m_iClip != 0)
	m_flPumpTime = gpGlobals->time + 0.5;

	// halflife-updated's fix, kept: GetNextAttackDelay evens out the fire-rate creep (seed; Steven
	// 2026-11-07: unclear calls go to halflife-updated). The original sets UTIL_WeaponTimeBase() + the cycle.
	if (m_pPlayer->m_iRune == RUNE_HASTE)
	{
		m_flNextPrimaryAttack = GetNextAttackDelay(oz_cycleshotgun.value * 0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cycleshotgun.value * 0.5;
	}
	else
	{
		m_flNextPrimaryAttack = GetNextAttackDelay(oz_cycleshotgun.value);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cycleshotgun.value;
	}
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	m_fInSpecialReload = 0;

	m_pPlayer->pev->punchangle.x -= oz_recoilshotgun.value;
}


// orig 0x106ac0. OZ: as PrimaryAttack, with the double-barrel cvars and two shells.
void CShotgun::SecondaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && oz_waterweapons.value == 0)
	{
		PlayEmptySound();
		// halflife-updated's fix, kept: GetNextAttackDelay evens out the fire-rate creep (seed; Steven
		// 2026-11-07: unclear calls go to halflife-updated). The original sets UTIL_WeaponTimeBase() + the cycle.
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 1)
	{
		Reload();
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecDir = m_pPlayer->FireBulletsPlayer((int)oz_pelletsdoubleshotgun.value, vecSrc, vecAiming, VECTOR_CONE_OZ_DOUBLESHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	if (m_pPlayer->m_iRune == RUNE_HASTE || m_pPlayer->pev->waterlevel == 3 || oz_cycleshotgun2.value < 0.3)
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		SendWeaponAnim(SHOTGUN_FIRE2);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		OzShotgunShell(m_pPlayer, this, m_iShell);
		OzShotgunShell(m_pPlayer, this, m_iShell);

		const int iPitch = 85 + RANDOM_LONG(0, 0x1f);
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/dbarrel1.wav", RANDOM_FLOAT(0.98, 1.0), ATTN_NORM, 0, iPitch);
	}
	else
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usDoubleFire, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);
	}

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	// halflife-updated's fix, kept: as PrimaryAttack's (a65846e)
	//if (m_iClip != 0)
	m_flPumpTime = gpGlobals->time + 0.95;

	// halflife-updated's fix, kept: GetNextAttackDelay evens out the fire-rate creep (seed; Steven
	// 2026-11-07: unclear calls go to halflife-updated). The original sets UTIL_WeaponTimeBase() + the cycle.
	if (m_pPlayer->m_iRune == RUNE_HASTE)
	{
		m_flNextPrimaryAttack = GetNextAttackDelay(oz_cycleshotgun2.value * 0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cycleshotgun2.value * 0.5;
	}
	else
	{
		m_flNextPrimaryAttack = GetNextAttackDelay(oz_cycleshotgun2.value);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cycleshotgun2.value;
	}
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInSpecialReload = 0;

	m_pPlayer->pev->punchangle.x -= oz_recoilshotgun2.value;
}


// orig 0x107410. OZ: oz_magazineshotgun shells, oz_reloadshotgun per insert.
void CShotgun::Reload()
{
	const int iMaxClip = (int)oz_magazineshotgun.value;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == iMaxClip)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(SHOTGUN_START_RELOAD);
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
		// halflife-updated's fix, kept: GetNextAttackDelay evens out the fire-rate creep (seed; Steven
		// 2026-11-07: unclear calls go to halflife-updated). The original sets UTIL_WeaponTimeBase() + the cycle.
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if (RANDOM_LONG(0, 1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

		SendWeaponAnim(SHOTGUN_RELOAD);

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip. sic: the reserve is charged oz_reloadshotgun shells whatever it held,
		// so it can go negative (ORIGINAL_BUGS)
		m_iClip = (int)((float)m_iClip + oz_reloadshotgun.value);
		if (m_iClip > iMaxClip)
		{
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] += m_iClip - iMaxClip;
			m_iClip = iMaxClip;
		}
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = (int)((float)m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] - oz_reloadshotgun.value);
		m_fInSpecialReload = 1;
	}
}


// orig 0x1076f0
void CShotgun::WeaponIdle()
{
	const int iMaxClip = (int)oz_magazineshotgun.value;

	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	// halflife-updated's fix, kept: moved to ItemPostFrame so the pump plays while attack is held
	// (a65846e, halflife#617)
	/*
	if ( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
	}
	*/

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && 0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != iMaxClip && 0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim(SHOTGUN_PUMP);

				// play cocking sound
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 0x1f));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.8)
			{
				iAnim = SHOTGUN_IDLE_DEEP;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (60.0 / 12.0); // * RANDOM_LONG(2, 5);
			}
			else if (flRand <= 0.95)
			{
				iAnim = SHOTGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0 / 9.0);
			}
			else
			{
				iAnim = SHOTGUN_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0 / 9.0);
			}
			SendWeaponAnim(iAnim);
		}
	}
}

// halflife-updated's (a65846e): the pump sound WeaponIdle played in the original
void CShotgun::ItemPostFrame()
{
	if (0 != m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		// play pumping sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 0x1f));
		m_flPumpTime = 0;
	}

	CBasePlayerWeapon::ItemPostFrame();
}


class CShotgunAmmo : public CBasePlayerAmmo
{
	// orig 0x141190
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_shotbox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	// orig 0x141150
	void Precache() override
	{
		PRECACHE_MODEL("models/w_shotbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	// orig 0x141090
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo((int)oz_giveshells.value, "buckshot", (int)oz_maxshells.value) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_buckshot, CShotgunAmmo); // orig 0x107970
