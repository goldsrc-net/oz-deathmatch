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
#include "game.h"

// The original has SDK 2.3's controls: primary throws the first charge, then detonates; secondary
// throws more. halflife-updated's MODERN_SATCHEL_CONTROLS (1c202f5, HL25) is a design change, left out.

class CSatchelCharge : public CGrenade
{
	void Spawn() override;
	void Precache() override;
	void BounceSound() override;

	void EXPORT SatchelSlide(CBaseEntity* pOther);
	void EXPORT SatchelThink();

public:
	void Deactivate();

	float m_flArmTime; // Oz: when the proximity fuse arms (satchel_charge_time). orig +0x2ac, unsaved. VERIFY name
};
LINK_ENTITY_TO_CLASS(monster_satchel, CSatchelCharge); // orig 0xff5d0

//=========================================================
// Deactivate - do whatever it is we do to an orphaned
// satchel when we don't want it in the world anymore.
//=========================================================
// orig 0xff5f0
void CSatchelCharge::Deactivate()
{
	pev->solid = SOLID_NOT;
	UTIL_Remove(this);
}


// orig 0xff620. Oz: damage_satchel; the proximity fuse's arm time; a charge can be shot (1 health).
void CSatchelCharge::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_satchel.mdl");
	//UTIL_SetSize(pev, Vector( -16, -16, -4), Vector(16, 16, 32));	// Old box -- size of headcrab monsters/players get blocked by this
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4)); // Uses point-sized, and can be stepped over
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CSatchelCharge::SatchelSlide);
	SetUse(&CSatchelCharge::DetonateUse);
	SetThink(&CSatchelCharge::SatchelThink);
	pev->nextthink = gpGlobals->time + 0.1;

	// Oz variables (.value)
	m_flArmTime = gpGlobals->time + oz_satchelchargetime.value;

	pev->gravity = 0.5;
	pev->friction = 0.8;

	pev->dmg = oz_damagesatchel.value; // Oz variables (.value)
	// ResetSequenceInfo( );
	pev->sequence = 1;

	pev->takedamage = DAMAGE_YES;
	pev->health = 1;
}


// orig 0xff780
void CSatchelCharge::SatchelSlide(CBaseEntity* pOther)
{
	entvars_t* pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// pev->avelocity = Vector (300, 300, 300);
	pev->gravity = 1; // normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 10), ignore_monsters, edict(), &tr);

	if (tr.flFraction < 1.0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if ((pev->flags & FL_ONGROUND) == 0 && pev->velocity.Length2D() > 10)
	{
		BounceSound();
	}
	StudioFrameAdvance();
}


// orig 0xff8e0. Oz: with allow_proximity_satchel 1, an armed charge blows when the first entity
// UTIL_FindEntityInSphere returns within satchel_detection_area is a player (only that one is
// tested, the charge's owner included). Every think also sets dmgtime 5 s ahead.
void CSatchelCharge::SatchelThink()
{
	pev->dmgtime = gpGlobals->time + 5; // VERIFY: nothing in the package reads it for a satchel

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->waterlevel == 3)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == 0)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}

	// Oz proximity satchel
	// Oz variables (.value)
	CBaseEntity* pEntity;
	if (m_flArmTime <= gpGlobals->time && oz_allowproximity.value == 1 &&
		(pEntity = UTIL_FindEntityInSphere(NULL, pev->origin, oz_satchelareadetect.value)) != NULL &&
		pEntity->IsPlayer())
	{
		SetThink(&CSatchelCharge::Detonate);
	}
}

// orig 0xffab0
void CSatchelCharge::Precache()
{
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
}

// orig 0xffb10
void CSatchelCharge::BounceSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM);
		break;
	}
}


LINK_ENTITY_TO_CLASS(weapon_satchel, CSatchel); // orig 0xffbb0


//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
// orig 0xffbd0. Oz: SDK 2.3's refusal while charges are out is gone (its multiplayer test is
// left with nothing in it), and so is halflife-updated's live charge limit (fd97136, HL25: design).
bool CSatchel::AddDuplicate(CBasePlayerItem* pOriginal)
{
#ifndef CLIENT_DLL
	g_pGameRules->IsMultiplayer();
#endif

	return CBasePlayerWeapon::AddDuplicate(pOriginal);
}

//=========================================================
//=========================================================
// orig 0xffc10. SDK 2.3's without its m_chargeReady = 0: a satchel weapon picked up keeps its
// radio state (Oz). The original goes through CBasePlayerItem::AddToPlayer, sets the weapon bit and
// calls AddWeapon (ExtractAmmo); here CBasePlayerWeapon::AddToPlayer sets the bit (and the ammo
// indices) and CBasePlayer::AddPlayerItem extracts the ammo after it (halflife-updated's flow).
void CSatchel::AddToPlayer(CBasePlayer* pPlayer)
{
	CBasePlayerWeapon::AddToPlayer(pPlayer);
}

// orig 0xffc80. Oz: disable_satchel removes it; give_with_satchel charges.
void CSatchel::Spawn()
{
	if (oz_disablesatchel.value > 0)
	{
		Kill();
		return;
	}

	Precache();
	m_iId = WEAPON_SATCHEL;
	SET_MODEL(ENT(pev), "models/w_satchel.mdl");

	m_iDefaultAmmo = (int)oz_givewithsatchel.value; // Oz variables (.value)

	FallInit(); // get ready to fall down.
}


// orig 0xffd20
void CSatchel::Precache()
{
	PRECACHE_MODEL("models/v_satchel.mdl");
	PRECACHE_MODEL("models/v_satchel_radio.mdl");
	PRECACHE_MODEL("models/w_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel_radio.mdl");

	UTIL_PrecacheOther("monster_satchel");
}


// orig 0xffda0. Oz: max_satchels, weight_satchel.
bool CSatchel::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Satchel Charge";
	p->iMaxAmmo1 = (int)oz_maxsatchels.value;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_SATCHEL;
	p->iWeight = (int)oz_weightsatchel.value; // Oz weight

	return true;
}

//=========================================================
//=========================================================
// orig 0xffe70
bool CSatchel::IsUseable()
{
	if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0)
	{
		// player is carrying some satchels
		return true;
	}

	if (m_chargeReady != 0)
	{
		// player isn't carrying any satchels, but has some out
		return true;
	}

	return false;
}

// orig 0xffec0
bool CSatchel::CanDeploy()
{
	if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0)
	{
		// player is carrying some satchels
		return true;
	}

	if (m_chargeReady != 0)
	{
		// player isn't carrying any satchels, but has some out
		return true;
	}

	return false;
}

// orig 0xfff10. halflife-updated's fix, kept: the idle time is set after DefaultDeploy, which sets
// its own 1 s (33385ad, halflife#2495); the original sets 10-15 s before it, so the draw animation
// is cut after 1 s.
bool CSatchel::Deploy()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	bool result;

	if (0 != m_chargeReady)
		result = DefaultDeploy("models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", SATCHEL_RADIO_DRAW, "hive");
	else
		result = DefaultDeploy("models/v_satchel.mdl", "models/p_satchel.mdl", SATCHEL_DRAW, "trip");

	if (result)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	}

	return result;
}


// orig 0xfffd0
void CSatchel::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (0 != m_chargeReady)
	{
		SendWeaponAnim(SATCHEL_RADIO_HOLSTER);
	}
	else
	{
		SendWeaponAnim(SATCHEL_DROP);
	}
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if (0 == m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && 0 == m_chargeReady)
	{
		m_pPlayer->ClearWeaponBit(m_iId);
		SetThink(&CSatchel::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}



// orig 0x1000d0. SDK 2.3's: the first charge, then the detonator.
void CSatchel::PrimaryAttack()
{
	switch (m_chargeReady)
	{
	case 0:
	{
		Throw();
	}
	break;
	case 1:
	{
		SendWeaponAnim(SATCHEL_RADIO_FIRE);

		edict_t* pPlayer = m_pPlayer->edict();

		CBaseEntity* pSatchel = NULL;

		while ((pSatchel = UTIL_FindEntityInSphere(pSatchel, m_pPlayer->pev->origin, 4096)) != NULL)
		{
			if (FClassnameIs(pSatchel->pev, "monster_satchel"))
			{
				if (pSatchel->pev->owner == pPlayer)
				{
					pSatchel->Use(m_pPlayer, m_pPlayer, USE_ON, 0);
					m_chargeReady = 2;
				}
			}
		}

		m_chargeReady = 2;
		// halflife-updated's fix, kept (Steven 2026-11-07): GetNextAttackDelay evens out the fire-rate creep
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		break;
	}

	case 2:
		// we're reloading, don't allow fire
		{
		}
		break;
	}
}


// orig 0x100230
void CSatchel::SecondaryAttack()
{
	if (m_chargeReady != 2)
	{
		Throw();
	}
}


// orig 0x100260. Oz: speed_satchel's throw.
void CSatchel::Throw()
{
	if (0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		Vector vecSrc = m_pPlayer->pev->origin;

		// Oz variables (.value)
		Vector vecThrow = gpGlobals->v_forward * oz_speedsatchel.value + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
		CBaseEntity* pSatchel = Create("monster_satchel", vecSrc, Vector(0, 0, 0), m_pPlayer->edict());
		pSatchel->pev->velocity = vecThrow;
		pSatchel->pev->avelocity.y = 400;

		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel_radio.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel_radio.mdl");
#else
		LoadVModel("models/v_satchel_radio.mdl", m_pPlayer);
#endif

		SendWeaponAnim(SATCHEL_RADIO_DRAW);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		m_chargeReady = 1;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		// halflife-updated's fix, kept (Steven 2026-11-07): GetNextAttackDelay evens out the fire-rate creep
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}


// orig 0x100420
void CSatchel::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	switch (m_chargeReady)
	{
	case 0:
		SendWeaponAnim(SATCHEL_FIDGET1);
		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");
		break;
	case 1:
		SendWeaponAnim(SATCHEL_RADIO_FIDGET1);
		// use hivehand animations
		strcpy(m_pPlayer->m_szAnimExtention, "hive");
		break;
	case 2:
		if (0 == m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#ifndef CLIENT_DLL
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel.mdl");
#else
		LoadVModel("models/v_satchel.mdl", m_pPlayer);
#endif

		SendWeaponAnim(SATCHEL_DRAW);

		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");

		// halflife-updated's fix, kept (Steven 2026-11-07): GetNextAttackDelay evens out the fire-rate creep
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_chargeReady = 0;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}

//=========================================================
// DeactivateSatchels - removes all satchels owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
// orig 0x100630
void DeactivateSatchels(CBasePlayer* pOwner)
{
	edict_t* pFind;

	pFind = FIND_ENTITY_BY_CLASSNAME(NULL, "monster_satchel");

	while (!FNullEnt(pFind))
	{
		CBaseEntity* pEnt = CBaseEntity::Instance(pFind);
		CSatchelCharge* pSatchel = (CSatchelCharge*)pEnt;

		if (pSatchel)
		{
			if (pSatchel->pev->owner == pOwner->edict())
			{
				pSatchel->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME(pFind, "monster_satchel");
	}
}
