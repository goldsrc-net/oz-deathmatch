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


#define CROWBAR_BODYHIT_VOLUME 128
#define CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS(weapon_crowbar, CCrowbar); // orig 0xa4f70

// orig 0xa4f90
void CCrowbar::Spawn()
{
	Precache();
	m_iId = WEAPON_CROWBAR;
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit(); // get ready to fall down.
}


// orig 0xa4ff0
void CCrowbar::Precache()
{
	PRECACHE_MODEL("models/v_crowbar.mdl");
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	UTIL_PrecacheOther("flying_crowbar"); // OZ: what SecondaryAttack throws

	m_usCrowbar = PRECACHE_EVENT(1, "events/crowbar.sc");
}

// orig 0xa50e0
bool CCrowbar::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_CROWBAR;
	p->iWeight = (int)oz_weightcrowbar.value;
	return true;
}



// orig 0xa5180
bool CCrowbar::Deploy()
{
	return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
}

// orig 0xa51c0
void CCrowbar::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CROWBAR_HOLSTER);
}


// orig 0xa5220
void FindHullIntersection(const Vector& vecSrc, TraceResult& tr, const Vector& mins, const Vector& maxs, edict_t* pEntity)
{
	int i, j, k;
	float distance;
	const Vector* minmaxs[2] = {&mins, &maxs};
	TraceResult tmpTrace;
	Vector vecHullEnd = tr.vecEndPos;
	Vector vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace);
	if (tmpTrace.flFraction < 1.0)
	{
		tr = tmpTrace;
		return;
	}

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			for (k = 0; k < 2; k++)
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i]->x;
				vecEnd.y = vecHullEnd.y + minmaxs[j]->y;
				vecEnd.z = vecHullEnd.z + minmaxs[k]->z;

				UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace);
				if (tmpTrace.flFraction < 1.0)
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


// orig 0xa5450
void CCrowbar::PrimaryAttack()
{
	if (!Swing(true))
	{
		SetThink(&CCrowbar::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


// orig 0xa54b0. OZ: throws the crowbar (flying_crowbar). Without the Super Crowbar rune the thrower
// loses it; with it he keeps it and can throw again on the next cycle.
void CCrowbar::SecondaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3)
		return;

	// No UTIL_MakeVectors here: the original uses whatever gpGlobals->v_* hold (the last weapon frame's).
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_right * 8 + gpGlobals->v_forward * 16;
	Vector vecDir = gpGlobals->v_forward;
	Vector vecAngles = UTIL_VecToAngles(vecDir);
	vecAngles.z = vecDir.z - 90; // sic: the forward vector's z, not the angle's

	CBaseEntity* pCrowbar = CBaseEntity::Create("flying_crowbar", vecSrc, Vector(0, 0, 0), m_pPlayer->edict());
	pCrowbar->pev->velocity = vecDir * 1000 + m_pPlayer->pev->velocity;
	pCrowbar->pev->angles = vecAngles;
	pCrowbar->pev->avelocity.x = -1000;
	pCrowbar->pev->gravity = 0.5;
	static_cast<CBasePlayerWeapon*>(pCrowbar)->m_pPlayer = m_pPlayer; // a CFlyingCrowbar

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + oz_cyclecrowbar.value;

	if (m_pPlayer->m_iRune != 3) // Super Crowbar Rune
	{
		m_pPlayer->pev->weapons &= ~(1 << m_iId);
		DestroyItem();
	}
}


// orig 0xa56f0
void CCrowbar::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


// orig 0xa5720
void CCrowbar::SwingAgain()
{
	Swing(false);
}


// orig 0xa5750. OZ: cycle times from oz_cyclecrowbar/oz_cyclecrowbar2 (halved by the Haste Rune),
// damage from oz_damagecrowbar, 1000 with the Super Crowbar Rune.
bool CCrowbar::Swing(bool fFirst)
{
	bool fDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	// halflife-updated's fix, kept: the original played the swing event on every call, so SwingAgain
	// played it twice (6528126, halflife#3230)
	if (fFirst)
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usCrowbar,
			0.0, g_vecZero, g_vecZero, 0, 0, 0,
			0.0, 0, 0.0);
	}


	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			if (m_pPlayer->m_iRune == 4) // Haste Rune
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclecrowbar.value * 0.5;
			else
				m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclecrowbar.value;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(CROWBAR_ATTACK1HIT);
			break;
		case 1:
			SendWeaponAnim(CROWBAR_ATTACK2HIT);
			break;
		case 2:
			SendWeaponAnim(CROWBAR_ATTACK3HIT);
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = true;
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		// JoshA: Changed from < -> <= to fix the full swing logic since client weapon prediction.
		// -1.0f + 1.0f = 0.0f. UTIL_WeaponTimeBase is always 0 with client weapon prediction (0 time base vs curtime base)
		// halflife-updated's fix, kept: the original's < (a542e4a, HL25); in multiplayer every swing is full anyway
		if ((m_flNextPrimaryAttack + 1.0f <= UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
		{
			// first swing does full damage
			if (m_pPlayer->m_iRune == 3) // Super Crowbar Rune
				pEntity->TraceAttack(m_pPlayer->pev, 1000, gpGlobals->v_forward, &tr, DMG_CLUB);
			else
				pEntity->TraceAttack(m_pPlayer->pev, oz_damagecrowbar.value, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		else
		{
			// subsequent swings do half
			if (m_pPlayer->m_iRune == 3)
				pEntity->TraceAttack(m_pPlayer->pev, 1000, gpGlobals->v_forward, &tr, DMG_CLUB);
			else
				pEntity->TraceAttack(m_pPlayer->pev, oz_damagecrowbar.value * 0.5, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

#endif

		// halflife-updated's fix, kept: the original set this at the end, after the return for a dead
		// target, so a corpse took swings with no delay (511c6ef, halflife#2590)
		if (m_pPlayer->m_iRune == 4) // Haste Rune
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclecrowbar2.value * 0.5;
		else
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + oz_cyclecrowbar2.value;

#ifndef CLIENT_DLL
		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool fHitWorld = true;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM);
					break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM);
					break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return true;
				else
					flVol = 0.1;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		SetThink(&CCrowbar::Smack);
		// halflife-updated's fix, kept: the original's UTIL_WeaponTimeBase() + 0.2 is 0.2 here, so the
		// decal went on at once (c90351f, halflife#2296)
		pev->nextthink = gpGlobals->time + 0.2;
	}
	return fDidHit;
}
