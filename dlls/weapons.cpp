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
/*

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"
#include "UserMessages.h"

#define NOT_USED 255

#define TRACER_FREQ 4 // Tracers fire every fourth bullet

extern bool IsBustingGame();
extern bool IsPlayerBusting(CBaseEntity* pPlayer);

//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a
// player can carry.
//=========================================================
// orig 0x128a40
int MaxAmmoCarry(int iszName)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (CBasePlayerItem::ItemInfoArray[i].pszAmmo1 && 0 == strcmp(STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo1))
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo1;
		if (CBasePlayerItem::ItemInfoArray[i].pszAmmo2 && 0 == strcmp(STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo2))
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo2;
	}

	ALERT(at_console, "MaxAmmoCarry() doesn't recognize '%s'!\n", STRING(iszName));
	return -1;
}


/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
// orig 0x128b50
void ClearMultiDamage()
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}


//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage

// orig 0x128b90
void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker)
{
	Vector vecSpot1; //where blood comes from
	Vector vecDir;	 //direction blood should go
	TraceResult tr;

	if (!gMultiDamage.pEntity)
		return;

	gMultiDamage.pEntity->TakeDamage(pevInflictor, pevAttacker, gMultiDamage.amount, gMultiDamage.type);
}


// GLOBALS USED:
//		gMultiDamage

// orig 0x128be0
void AddMultiDamage(entvars_t* pevInflictor, CBaseEntity* pEntity, float flDamage, int bitsDamageType)
{
	if (!pEntity)
		return;

	gMultiDamage.type |= bitsDamageType;

	if (pEntity != gMultiDamage.pEntity)
	{
		ApplyMultiDamage(pevInflictor, pevInflictor); // UNDONE: wrong attacker!
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.amount = 0;
	}

	gMultiDamage.amount += flDamage;
}

/*
================
SpawnBlood
================
*/
// orig 0x128c40
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage)
{
	UTIL_BloodDrips(vecSpot, g_vecAttackDir, bloodColor, (int)flDamage);
}


// orig 0x128c90
int DamageDecal(CBaseEntity* pEntity, int bitsDamageType)
{
	if (!pEntity)
		return (DECAL_GUNSHOT1 + RANDOM_LONG(0, 4));

	return pEntity->DamageDecal(bitsDamageType);
}

// orig 0x128ce0
void DecalGunshot(TraceResult* pTrace, int iBulletType)
{
	// Is the entity valid
	if (!UTIL_IsValidEntity(pTrace->pHit))
		return;

	if (VARS(pTrace->pHit)->solid == SOLID_BSP || VARS(pTrace->pHit)->movetype == MOVETYPE_PUSHSTEP)
	{
		CBaseEntity* pEntity = NULL;
		// Decal the wall with a gunshot
		if (!FNullEnt(pTrace->pHit))
			pEntity = CBaseEntity::Instance(pTrace->pHit);

		switch (iBulletType)
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			UTIL_GunshotDecalTrace(pTrace, DamageDecal(pEntity, DMG_BULLET));
			break;
		case BULLET_MONSTER_12MM:
			// smoke and decal
			UTIL_GunshotDecalTrace(pTrace, DamageDecal(pEntity, DMG_BULLET));
			break;
		case BULLET_PLAYER_CROWBAR:
			// wall decal
			UTIL_DecalTrace(pTrace, DamageDecal(pEntity, DMG_CLUB));
			break;
		}
	}
}



//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
// orig 0x128e40
void EjectBrass(const Vector& vecOrigin, const Vector& vecVelocity, float rotation, int model, int soundtype)
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
	WRITE_BYTE(TE_MODEL);
	WRITE_COORD(vecOrigin.x);
	WRITE_COORD(vecOrigin.y);
	WRITE_COORD(vecOrigin.z);
	WRITE_COORD(vecVelocity.x);
	WRITE_COORD(vecVelocity.y);
	WRITE_COORD(vecVelocity.z);
	WRITE_ANGLE(rotation);
	WRITE_SHORT(model);
	WRITE_BYTE(soundtype);
	WRITE_BYTE(25); // 2.5 seconds
	MESSAGE_END();
}


#if 0
// UNDONE: This is no longer used?
void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE ( TE_EXPLODEMODEL );
		WRITE_COORD( vecOrigin.x );
		WRITE_COORD( vecOrigin.y );
		WRITE_COORD( vecOrigin.z );
		WRITE_COORD( speed );
		WRITE_SHORT( model );
		WRITE_SHORT( count );
		WRITE_BYTE ( 15 );// 1.5 seconds
	MESSAGE_END();
}
#endif

// Precaches the weapon and queues the weapon info for sending to clients
// orig 0x129000
void UTIL_PrecacheOtherWeapon(const char* szClassname)
{
	edict_t* pent;

	pent = CREATE_NAMED_ENTITY(MAKE_STRING(szClassname));
	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n");
		return;
	}

	CBaseEntity* pEntity = CBaseEntity::Instance(VARS(pent));

	if (pEntity)
	{
		ItemInfo II;
		pEntity->Precache();
		memset(&II, 0, sizeof II);
		if (((CBasePlayerItem*)pEntity)->GetItemInfo(&II))
		{
			CBasePlayerItem::ItemInfoArray[II.iId] = II;

			const char* weaponName = ((II.iFlags & ITEM_FLAG_EXHAUSTIBLE) != 0) ? STRING(pEntity->pev->classname) : nullptr;

			if (II.pszAmmo1 && '\0' != *II.pszAmmo1)
			{
				AddAmmoNameToAmmoRegistry(II.pszAmmo1, weaponName);
			}

			if (II.pszAmmo2 && '\0' != *II.pszAmmo2)
			{
				AddAmmoNameToAmmoRegistry(II.pszAmmo2, weaponName);
			}

			memset(&II, 0, sizeof II);
		}
	}

	REMOVE_ENTITY(pent);
}

// called by worldspawn
// orig 0x129170
void W_Precache()
{
	memset(CBasePlayerItem::ItemInfoArray, 0, sizeof(CBasePlayerItem::ItemInfoArray));
	memset(CBasePlayerItem::AmmoInfoArray, 0, sizeof(CBasePlayerItem::AmmoInfoArray));
	giAmmoIndex = 0;

	// custom items...

	// common world objects
	UTIL_PrecacheOther("item_suit");
	UTIL_PrecacheOther("item_battery");
	UTIL_PrecacheOther("item_antidote");
	UTIL_PrecacheOther("item_security");
	UTIL_PrecacheOther("item_longjump");

	// shotgun
	UTIL_PrecacheOtherWeapon("weapon_shotgun");
	UTIL_PrecacheOther("ammo_buckshot");

	// crowbar
	UTIL_PrecacheOtherWeapon("weapon_crowbar");

	// glock
	UTIL_PrecacheOtherWeapon("weapon_9mmhandgun");
	UTIL_PrecacheOther("ammo_9mmclip");

	// mp5
	UTIL_PrecacheOtherWeapon("weapon_9mmAR");
	UTIL_PrecacheOther("ammo_9mmAR");
	UTIL_PrecacheOther("ammo_ARgrenades");

	// python
	UTIL_PrecacheOtherWeapon("weapon_357");
	UTIL_PrecacheOther("ammo_357");

	// gauss
	UTIL_PrecacheOtherWeapon("weapon_gauss");
	UTIL_PrecacheOther("ammo_gaussclip");

	// rpg
	UTIL_PrecacheOtherWeapon("weapon_rpg");
	UTIL_PrecacheOther("ammo_rpgclip");

	// crossbow
	UTIL_PrecacheOtherWeapon("weapon_crossbow");
	UTIL_PrecacheOther("ammo_crossbow");

	// egon
	UTIL_PrecacheOtherWeapon("weapon_egon");

	// tripmine
	UTIL_PrecacheOtherWeapon("weapon_tripmine");

	// satchel charge
	UTIL_PrecacheOtherWeapon("weapon_satchel");

	// hand grenade
	UTIL_PrecacheOtherWeapon("weapon_handgrenade");

	// squeak grenade
	UTIL_PrecacheOtherWeapon("weapon_snark");

	// hornetgun
	UTIL_PrecacheOtherWeapon("weapon_hornetgun");

	if (g_pGameRules->IsDeathmatch())
	{
		UTIL_PrecacheOther("weaponbox"); // container for dropped deathmatch weapons
	}

	g_sModelIndexFireball = PRECACHE_MODEL("sprites/zerogxplode.spr");	// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL("sprites/WXplo1.spr");		// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL("sprites/steam1.spr");			// smoke
	g_sModelIndexBubbles = PRECACHE_MODEL("sprites/bubble.spr");		//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL("sprites/bloodspray.spr"); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL("sprites/blood.spr");		// splattered blood

	g_sModelIndexLaser = PRECACHE_MODEL((char*)g_pModelNameLaser);
	g_sModelIndexLaserDot = PRECACHE_MODEL("sprites/laserdot.spr");


	// used by explosions
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_MODEL("sprites/explode1.spr");

	PRECACHE_SOUND("weapons/debris1.wav"); // explosion aftermaths
	PRECACHE_SOUND("weapons/debris2.wav"); // explosion aftermaths
	PRECACHE_SOUND("weapons/debris3.wav"); // explosion aftermaths

	PRECACHE_SOUND("weapons/grenade_hit1.wav"); //grenade
	PRECACHE_SOUND("weapons/grenade_hit2.wav"); //grenade
	PRECACHE_SOUND("weapons/grenade_hit3.wav"); //grenade

	PRECACHE_SOUND("weapons/bullet_hit1.wav"); // hit by bullet
	PRECACHE_SOUND("weapons/bullet_hit2.wav"); // hit by bullet

	PRECACHE_SOUND("items/weapondrop1.wav"); // weapon falls to the ground
}




TYPEDESCRIPTION CBasePlayerItem::m_SaveData[] =
	{
		DEFINE_FIELD(CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayerItem, m_pNext, FIELD_CLASSPTR),
		//DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
		DEFINE_FIELD(CBasePlayerItem, m_iId, FIELD_INTEGER),
		// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
		// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE(CBasePlayerItem, CBaseAnimating);


TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
	{
#if defined(CLIENT_WEAPONS)
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_FLOAT),
#else  // CLIENT_WEAPONS
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME),
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME),
		DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME),
#endif // CLIENT_WEAPONS
		DEFINE_FIELD(CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayerWeapon, m_iClip, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER),
		//	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER )	 , reset to zero on load so hud gets updated correctly
		//  DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
};

IMPLEMENT_SAVERESTORE(CBasePlayerWeapon, CBasePlayerItem);


// orig 0x129610
void CBasePlayerItem::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16);
}


//=========================================================
// Sets up movetype, size, solidtype for a new weapon.
//=========================================================
// orig 0x1296e0
void CBasePlayerItem::FallInit()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0)); //pointsize until it lands on the ground.

	SetTouch(&CBasePlayerItem::DefaultTouch);
	SetThink(&CBasePlayerItem::FallThink);

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
// orig 0x1297b0
void CBasePlayerItem::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if (!FNullEnt(pev->owner))
		{
			int pitch = 95 + RANDOM_LONG(0, 29);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch);
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize();
	}
	else if (m_pPlayer != NULL)
	{
		// halflife-updated's, kept with its busters mode (e0ba8ce): inert with sv_busters 0
		SetThink(NULL);
	}

	// This weapon is an egon, it has no owner and we're in busting mode, so just remove it when it hits the ground
	if (IsBustingGame() && FNullEnt(pev->owner))
	{
		if (!strcmp("weapon_egon", STRING(pev->classname)))
		{
			UTIL_Remove(this);
		}
	}
}

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
// orig 0x129880
void CBasePlayerItem::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin(pev, pev->origin); // link into world.
	SetTouch(&CBasePlayerItem::DefaultTouch);
	SetThink(NULL);
}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
// orig 0x129920
void CBasePlayerItem::AttemptToMaterialize()
{
	float time = g_pGameRules->FlWeaponTryRespawn(this);

	if (time == 0)
	{
		Materialize();
		return;
	}

	// halflife-updated's fix, kept: FlWeaponTryRespawn returns an absolute time, which the original
	// added to gpGlobals->time (f14827a)
	pev->nextthink = time;
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should
// it respawn?
//=========================================================
// orig 0x129990
void CBasePlayerItem::CheckRespawn()
{
	switch (g_pGameRules->WeaponShouldRespawn(this))
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		return;
		break;
	}
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
// orig 0x1299e0
CBaseEntity* CBasePlayerItem::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity* pNewWeapon = CBaseEntity::Create((char*)STRING(pev->classname), g_pGameRules->VecWeaponRespawnSpot(this), pev->angles, pev->owner);

	if (pNewWeapon)
	{
		pNewWeapon->pev->effects |= EF_NODRAW; // invisible for now
		pNewWeapon->SetTouch(NULL);			   // no touch
		pNewWeapon->SetThink(&CBasePlayerItem::AttemptToMaterialize);

		DROP_TO_FLOOR(ENT(pev));

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime(this);
	}
	else
	{
		ALERT(at_console, "Respawn failed to create %s!\n", STRING(pev->classname));
	}

	return pNewWeapon;
}

// orig 0x129ae0
void CBasePlayerItem::DefaultTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
		return;

	if (IsPlayerBusting(pOther))
		return;

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// can I have this?
	if (!g_pGameRules->CanHavePlayerItem(pPlayer, this))
	{
		if (gEvilImpulse101)
		{
			UTIL_Remove(this);
		}
		return;
	}

	if (pOther->AddPlayerItem(this))
	{
		AttachToPlayer(pPlayer);
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	}

	SUB_UseTargets(pOther, USE_TOGGLE, 0); // UNDONE: when should this happen?
}

// orig 0x12a0a0
void CBasePlayerItem::DestroyItem()
{
	if (m_pPlayer)
	{
		// if attached to a player, remove.
		m_pPlayer->RemovePlayerItem(this);
	}

	Kill();
}

// orig 0x12a0f0
void CBasePlayerItem::AddToPlayer(CBasePlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

// orig 0x12a110
void CBasePlayerItem::Drop()
{
	SetTouch(NULL);
	SetThink(&CBasePlayerItem::SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

// orig 0x12a170
void CBasePlayerItem::Kill()
{
	SetTouch(NULL);
	SetThink(&CBasePlayerItem::SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

// orig 0x12a1d0
void CBasePlayerItem::Holster()
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

// orig 0x12a200
void CBasePlayerItem::AttachToPlayer(CBasePlayer* pPlayer)
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0;	  // server won't send down to clients if modelindex == 0
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();
	pev->nextthink = gpGlobals->time + .1;
	SetTouch(NULL);
	// halflife-updated's fix, kept: FallThink could pick the weapon up twice and crash (ad50584)
	SetThink(NULL); // Clear FallThink function so it can't run while attached to player.
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
// orig 0x12a2b0
bool CBasePlayerWeapon::AddDuplicate(CBasePlayerItem* pOriginal)
{
	if (0 != m_iDefaultAmmo)
	{
		return ExtractAmmo((CBasePlayerWeapon*)pOriginal);
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo((CBasePlayerWeapon*)pOriginal);
	}
}


// orig 0x12a300
void CBasePlayerWeapon::AddToPlayer(CBasePlayer* pPlayer)
{
	/*
	if ((iFlags() & ITEM_FLAG_EXHAUSTIBLE) != 0 && m_iDefaultAmmo == 0 && m_iClip <= 0)
	{
		//This is an exhaustible weapon that has no ammo left. Don't add it, queue it up for destruction instead.
		SetThink(&CSatchel::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
		return false;
	}
	*/

	CBasePlayerItem::AddToPlayer(pPlayer);

	pPlayer->SetWeaponBit(m_iId);

	if (0 == m_iPrimaryAmmoType)
	{
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex(pszAmmo1());
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex(pszAmmo2());
	}

	// The original ends with return AddWeapon(), which extracts the ammo here. halflife-updated's fix,
	// kept: CBasePlayer::AddPlayerItem extracts it once the weapon is in the player's list (6951bf1,
	// #153 #157; FIXES.md, as mod-rc2), so AddWeapon has no caller on the pickup path.
}

// orig 0x12a3c0
bool CBasePlayerWeapon::UpdateClientData(CBasePlayer* pPlayer)
{
	bool bSend = false;
	int state = 0;
	if (pPlayer->m_pActiveItem == this)
	{
		if (pPlayer->m_fOnTarget)
			state = WEAPON_IS_ONTARGET;
		else
			state = 1;
	}

	// Forcing send of all data!
	if (!pPlayer->m_fWeapon)
	{
		bSend = true;
	}

	// This is the current or last weapon, so the state will need to be updated
	if (this == pPlayer->m_pActiveItem ||
		this == pPlayer->m_pClientActiveItem)
	{
		if (pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem)
		{
			bSend = true;
		}
	}

	// If the ammo, state, or fov has changed, update the weapon
	if (m_iClip != m_iClientClip ||
		state != m_iClientWeaponState ||
		pPlayer->m_iFOV != pPlayer->m_iClientFOV)
	{
		bSend = true;
	}

	if (bSend)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev);
		WRITE_BYTE(state);
		WRITE_BYTE(m_iId);
		WRITE_BYTE(m_iClip);
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = true;
	}

	if (m_pNext)
		m_pNext->UpdateClientData(pPlayer);

	return true;
}


// orig 0x12a530
void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	// halflife-updated's fix, kept: m_ForceSendAnimations (42f45bd, #154; FIXES.md)
	const bool skiplocal = !m_ForceSendAnimations && UseDecrement() != false;

	m_pPlayer->pev->weaponanim = iAnim;

#if defined(CLIENT_WEAPONS)
	// OZ: always sent to the shooter for the glock, MP5 and shotgun, whose server-side firing Valve's
	// client doesn't predict. sic: no null check on m_pActiveItem.
	const int iActiveId = m_pPlayer->m_pActiveItem->m_iId;
	if (iActiveId != WEAPON_GLOCK && iActiveId != WEAPON_MP5 && iActiveId != WEAPON_SHOTGUN)
	{
		if (skiplocal && ENGINE_CANSKIP(m_pPlayer->edict()))
			return;
	}
#endif

	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev);
	WRITE_BYTE(iAnim);	   // sequence number
	WRITE_BYTE(pev->body); // weaponmodel bodygroup.
	MESSAGE_END();
}

// orig 0x12a630
bool CBasePlayerWeapon::AddPrimaryAmmo(CBasePlayerWeapon* origin, int iCount, char* szName, int iMaxClip, int iMaxCarry)
{
	int iIdAmmo;

	if (iMaxClip < 1)
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMaxCarry);
	}
	else if (m_iClip == 0)
	{
		int i;
		i = V_min(m_iClip + iCount, iMaxClip) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount - i, szName, iMaxCarry);
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMaxCarry);
	}

	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iPrimaryAmmoType = iIdAmmo;
		// halflife-updated's fix, kept: the original tested m_pPlayer->HasPlayerItem(this) (3d13792,
		// with the AddPlayerItem order of 6951bf1)
		if (this != origin)
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
	}

	return iIdAmmo > 0 ? true : false;
}


// orig 0x12a730
bool CBasePlayerWeapon::AddSecondaryAmmo(int iCount, char* szName, int iMax)
{
	int iIdAmmo;

	iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMax);

	//m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] = iMax; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iSecondaryAmmoType = iIdAmmo;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	}
	return iIdAmmo > 0 ? true : false;
}

//=========================================================
// IsUseable - this function determines whether or not a
// weapon is useable by the player in its current state.
// (does it have ammo loaded? do I have any ammo for the
// weapon?, etc)
//=========================================================
// orig 0x12a7b0
// halflife-updated's fix, kept: secondary ammo, WEAPON_NOCLIP, CanDeploy (a9b1d9c, c4a67d4; FIXES.md)
bool CBasePlayerWeapon::IsUseable()
{
	if (m_iClip > 0)
	{
		return true;
	}

	//Player has unlimited ammo for this weapon or does not use magazines
	if (iMaxAmmo1() == WEAPON_NOCLIP)
	{
		return true;
	}

	if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0)
	{
		return true;
	}

	if (pszAmmo2())
	{
		//Player has unlimited ammo for this weapon or does not use magazines
		if (iMaxAmmo2() == WEAPON_NOCLIP)
		{
			return true;
		}

		if (m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] > 0)
		{
			return true;
		}
	}

	// clip is empty (or nonexistant) and the player has no more ammo of this type.
	return CanDeploy();
}

// orig 0x12a900
bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
		return false;

	// OZ: drop a zoom left over from another weapon (only the FOV; pev->fov stays)
	if (0 != SuperGlockZoom)
	{
		m_pPlayer->m_iFOV = 0;
		SuperGlockZoom = 0;
	}
	if (0 != OzZoom)
	{
		m_pPlayer->m_iFOV = 0;
		OzZoom = 0;
	}

	m_pPlayer->TabulateAmmo();
	m_pPlayer->pev->viewmodel = MAKE_STRING(szViewModel);
	m_pPlayer->pev->weaponmodel = MAKE_STRING(szWeaponModel);
	strcpy(m_pPlayer->m_szAnimExtention, szAnimExt);
	SendWeaponAnim(iAnim, body);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_flLastFireTime = 0.0; // GetNextAttackDelay's bookkeeping (the 2013 SDK's); see the dossier

	return true;
}

// orig 0x12ab20
bool CBasePlayerWeapon::PlayEmptySound()
{
	if (m_iPlayEmptySound)
	{
		// halflife-updated's fix, kept: EMIT_SOUND_PREDICTED (063b8a5; FIXES.md)
		EMIT_SOUND_PREDICTED(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM, 0, PITCH_NORM);
		m_iPlayEmptySound = false;
		return false;
	}
	return false;
}

//=========================================================
//=========================================================
// orig 0x12abb0
int CBasePlayerWeapon::PrimaryAmmoIndex()
{
	return m_iPrimaryAmmoType;
}

//=========================================================
//=========================================================
// orig 0x12abc0
int CBasePlayerWeapon::SecondaryAmmoIndex()
{
	return m_iSecondaryAmmoType;
}

// orig 0x12abd0
void CBasePlayerWeapon::Holster()
{
	// OZ: unzoom
	if (0 != SuperGlockZoom)
	{
		m_pPlayer->m_iFOV = 0;
		m_pPlayer->pev->fov = 0;
		SuperGlockZoom = 0;
	}
	if (0 != OzZoom)
	{
		m_pPlayer->m_iFOV = 0;
		m_pPlayer->pev->fov = 0;
		OzZoom = 0;
	}

	m_fInReload = false; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

// orig 0x12ac70
void CBasePlayerAmmo::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CBasePlayerAmmo::DefaultTouch);
}

// orig 0x12ad00
CBaseEntity* CBasePlayerAmmo::Respawn()
{
	pev->effects |= EF_NODRAW;
	SetTouch(NULL);

	UTIL_SetOrigin(pev, g_pGameRules->VecAmmoRespawnSpot(this)); // move to wherever I'm supposed to repawn.

	SetThink(&CBasePlayerAmmo::Materialize);
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime(this);

	return this;
}

// orig 0x12ad90
void CBasePlayerAmmo::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CBasePlayerAmmo::DefaultTouch);
}

// orig 0x12ae10
void CBasePlayerAmmo::DefaultTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{
		return;
	}

	if (AddAmmo(pOther))
	{
		if (g_pGameRules->AmmoShouldRespawn(this) == GR_AMMO_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			SetTouch(NULL);
			SetThink(&CBasePlayerAmmo::SUB_Remove);
			pev->nextthink = gpGlobals->time + .1;
		}
	}
	else if (gEvilImpulse101)
	{
		// evil impulse 101 hack, kill always
		SetTouch(NULL);
		SetThink(&CBasePlayerAmmo::SUB_Remove);
		pev->nextthink = gpGlobals->time + .1;
	}
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in
// the weapon clip comes along.
//=========================================================
// orig 0x12aef0
bool CBasePlayerWeapon::ExtractAmmo(CBasePlayerWeapon* pWeapon)
{
	bool iReturn = false;

	if (pszAmmo1() != NULL)
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want.
		iReturn = pWeapon->AddPrimaryAmmo(this, m_iDefaultAmmo, (char*)pszAmmo1(), iMaxClip(), iMaxAmmo1());
		m_iDefaultAmmo = 0;
	}

	if (pszAmmo2() != NULL)
	{
		iReturn |= pWeapon->AddSecondaryAmmo(0, (char*)pszAmmo2(), iMaxAmmo2());
	}

	return iReturn;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
// orig 0x12afa0
bool CBasePlayerWeapon::ExtractClipAmmo(CBasePlayerWeapon* pWeapon)
{
	int iAmmo;

	if (m_iClip == WEAPON_NOCLIP)
	{
		iAmmo = 0; // guns with no clips always come empty if they are second-hand
	}
	else
	{
		iAmmo = m_iClip;
	}

	//TODO: should handle -1 return as well (only return true if ammo was taken)
	return pWeapon->m_pPlayer->GiveAmmo(iAmmo, pszAmmo1(), iMaxAmmo1()) != 0; // , &m_iPrimaryAmmoType
}

// orig 0x12c330. Called by AddToPlayer in the original; see CBasePlayerWeapon::AddToPlayer.
bool CBasePlayerWeapon::AddWeapon()
{
	ExtractAmmo(this);
	return true;
}

//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
// orig 0x12b020
// halflife-updated's fix, kept: deferred to DoRetireWeapon (c4a67d4; FIXES.md)
void CBasePlayerWeapon::RetireWeapon()
{
	SetThink(&CBasePlayerWeapon::CallDoRetireWeapon);
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CBasePlayerWeapon::DoRetireWeapon()
{
	if (!m_pPlayer || m_pPlayer->m_pActiveItem != this)
	{
		// Already retired?
		return;
	}

	// first, no viewmodel at all.
	m_pPlayer->pev->viewmodel = iStringNull;
	m_pPlayer->pev->weaponmodel = iStringNull;
	//m_pPlayer->pev->viewmodelindex = NULL;

	g_pGameRules->GetNextBestWeapon(m_pPlayer, this);

	//If we're still equipped and we couldn't switch to another weapon, dequip this one
	if (CanHolster() && m_pPlayer->m_pActiveItem == this)
	{
		m_pPlayer->SwitchWeapon(nullptr);
	}
}

//=========================================================================
// GetNextAttackDelay - An accurate way of calcualting the next attack time.
//=========================================================================
float CBasePlayerWeapon::GetNextAttackDelay(float delay)
{
	if (m_flLastFireTime == 0 || m_flNextPrimaryAttack == -1)
	{
		// At this point, we are assuming that the client has stopped firing
		// and we are going to reset our book keeping variables.
		m_flLastFireTime = gpGlobals->time;
		m_flPrevPrimaryAttack = delay;
	}
	// calculate the time between this shot and the previous
	float flTimeBetweenFires = gpGlobals->time - m_flLastFireTime;
	float flCreep = 0.0f;
	if (flTimeBetweenFires > 0)
		flCreep = flTimeBetweenFires - m_flPrevPrimaryAttack; // postive or negative

	// save the last fire time
	m_flLastFireTime = gpGlobals->time;

	float flNextAttack = UTIL_WeaponTimeBase() + delay - flCreep;
	// we need to remember what the m_flNextPrimaryAttack time is set to for each shot,
	// store it as m_flPrevPrimaryAttack.
	m_flPrevPrimaryAttack = flNextAttack - UTIL_WeaponTimeBase();
	// 	char szMsg[256];
	// 	snprintf( szMsg, sizeof(szMsg), "next attack time: %0.4f\n", gpGlobals->time + flNextAttack );
	// 	OutputDebugString( szMsg );
	return flNextAttack;
}


//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS(weaponbox, CWeaponBox);

TYPEDESCRIPTION CWeaponBox::m_SaveData[] =
	{
		DEFINE_ARRAY(CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
		DEFINE_ARRAY(CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS),
		DEFINE_ARRAY(CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES),
		DEFINE_FIELD(CWeaponBox, m_cAmmoTypes, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CWeaponBox, CBaseEntity);

//=========================================================
//
//=========================================================
// orig 0x12b140
void CWeaponBox::Precache()
{
	PRECACHE_MODEL("models/w_weaponbox.mdl");
}

//=========================================================
//=========================================================
// orig 0x12b170
bool CWeaponBox::KeyValue(KeyValueData* pkvd)
{
	if (m_cAmmoTypes < MAX_AMMO_SLOTS)
	{
		PackAmmo(ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue));
		m_cAmmoTypes++; // count this new ammo type.

		return true;
	}
	else
	{
		ALERT(at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_SLOTS);
	}

	return false;
}

//=========================================================
// CWeaponBox - Spawn
//=========================================================
// orig 0x12b350
void CWeaponBox::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	SET_MODEL(ENT(pev), "models/w_weaponbox.mdl");
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
// orig 0x12b3c0
void CWeaponBox::Kill()
{
	CBasePlayerItem* pWeapon;
	int i;

	// destroy the weapons
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			pWeapon->SetThink(&CBasePlayerItem::SUB_Remove);
			pWeapon->pev->nextthink = gpGlobals->time + 0.1;
			pWeapon = pWeapon->m_pNext;
		}
	}

	// remove the box
	UTIL_Remove(this);
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
// orig 0x12b450
void CWeaponBox::Touch(CBaseEntity* pOther)
{
	if ((pev->flags & FL_ONGROUND) == 0)
	{
		return;
	}

	if (!pOther->IsPlayer())
	{
		// only players may touch a weaponbox.
		return;
	}

	if (!pOther->IsAlive())
	{
		// no dead guys.
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;
	int i;

	// dole out ammo
	for (i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (!FStringNull(m_rgiszAmmo[i]))
		{
			// there's some ammo of this type.
			pPlayer->GiveAmmo(m_rgAmmo[i], STRING(m_rgiszAmmo[i]), MaxAmmoCarry(m_rgiszAmmo[i]));

			//ALERT ( at_console, "Gave %d rounds of %s\n", m_rgAmmo[i], STRING(m_rgiszAmmo[i]) );

			// now empty the ammo from the weaponbox since we just gave it to the player
			m_rgiszAmmo[i] = iStringNull;
			m_rgAmmo[i] = 0;
		}
	}

	// go through my weapons and try to give the usable ones to the player.
	// it's important the the player be given ammo first, so the weapons code doesn't refuse
	// to deploy a better weapon that the player may pick up because he has no ammo for it.
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			CBasePlayerItem* pItem;

			// have at least one weapon in this slot
			while (m_rgpPlayerItems[i])
			{
				//ALERT ( at_console, "trying to give %s\n", STRING( m_rgpPlayerItems[ i ]->pev->classname ) );

				pItem = m_rgpPlayerItems[i];

				// OZ: a box with a crowbar (a thrown one's) is for players without one. sic: the ammo
				// and any weapons in earlier slots are already handed out, and the box stays.
				if (pItem->m_iId == WEAPON_CROWBAR)
				{
					for (int j = 0; j < MAX_ITEM_TYPES; j++)
					{
						for (CBasePlayerItem* pHave = pPlayer->m_rgpPlayerItems[j]; pHave; pHave = pHave->m_pNext)
						{
							if (pHave->m_iId == WEAPON_CROWBAR)
								return;
						}
					}
				}

				m_rgpPlayerItems[i] = m_rgpPlayerItems[i]->m_pNext; // unlink this weapon from the box

				if (pPlayer->AddPlayerItem(pItem))
				{
					pItem->AttachToPlayer(pPlayer);
				}
			}
		}
	}

	EMIT_SOUND(pOther->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	SetTouch(NULL);
	UTIL_Remove(this);
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
// orig 0x12b660
bool CWeaponBox::PackWeapon(CBasePlayerItem* pWeapon)
{
	// is one of these weapons already packed in this box?
	if (HasWeapon(pWeapon))
	{
		return false; // box can only hold one of each weapon type
	}

	if (pWeapon->m_pPlayer)
	{
		if (!pWeapon->m_pPlayer->RemovePlayerItem(pWeapon))
		{
			// failed to unhook the weapon from the player!
			return false;
		}
	}

	int iWeaponSlot = pWeapon->iItemSlot();

	if (m_rgpPlayerItems[iWeaponSlot])
	{
		// there's already one weapon in this slot, so link this into the slot's column
		pWeapon->m_pNext = m_rgpPlayerItems[iWeaponSlot];
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
	}
	else
	{
		// first weapon we have for this slot
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
		pWeapon->m_pNext = NULL;
	}

	pWeapon->pev->spawnflags |= SF_NORESPAWN; // never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink(NULL); // crowbar may be trying to swing again, etc.
	pWeapon->SetTouch(NULL);
	pWeapon->m_pPlayer = NULL;

	// OZ: a box holding a crowbar looks like one
	if (pWeapon->m_iId == WEAPON_CROWBAR)
		SET_MODEL(ENT(pev), "models/w_crowbar.mdl");

	//ALERT ( at_console, "packed %s\n", STRING(pWeapon->pev->classname) );

	return true;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
// orig 0x12b7c0
bool CWeaponBox::PackAmmo(int iszName, int iCount)
{
	int iMaxCarry;

	if (FStringNull(iszName))
	{
		// error here
		ALERT(at_console, "NULL String in PackAmmo!\n");
		return false;
	}

	iMaxCarry = MaxAmmoCarry(iszName);

	if (iMaxCarry != -1 && iCount > 0)
	{
		//ALERT ( at_console, "Packed %d rounds of %s\n", iCount, STRING(iszName) );
		GiveAmmo(iCount, STRING(iszName), iMaxCarry);
		return true;
	}

	return false;
}

//=========================================================
// CWeaponBox - GiveAmmo
//=========================================================
// orig 0x12b850
int CWeaponBox::GiveAmmo(int iCount, const char* szName, int iMax, int* pIndex /* = NULL*/)
{
	int i;

	for (i = 1; i < MAX_AMMO_SLOTS && !FStringNull(m_rgiszAmmo[i]); i++)
	{
		if (stricmp(szName, STRING(m_rgiszAmmo[i])) == 0)
		{
			if (pIndex)
				*pIndex = i;

			int iAdd = V_min(iCount, iMax - m_rgAmmo[i]);
			if (iCount == 0 || iAdd > 0)
			{
				m_rgAmmo[i] += iAdd;

				return i;
			}
			return -1;
		}
	}
	if (i < MAX_AMMO_SLOTS)
	{
		if (pIndex)
			*pIndex = i;

		m_rgiszAmmo[i] = MAKE_STRING(szName);
		m_rgAmmo[i] = iCount;

		return i;
	}
	ALERT(at_console, "out of named ammo slots\n");
	return i;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
// orig 0x12b970
bool CWeaponBox::HasWeapon(CBasePlayerItem* pCheckItem)
{
	CBasePlayerItem* pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs(pItem->pev, STRING(pCheckItem->pev->classname)))
		{
			return true;
		}
		pItem = pItem->m_pNext;
	}

	return false;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
// orig 0x12ba00
bool CWeaponBox::IsEmpty()
{
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			return false;
		}
	}

	for (i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (!FStringNull(m_rgiszAmmo[i]))
		{
			// still have a bit of this type of ammo
			return false;
		}
	}

	return true;
}

//=========================================================
//=========================================================
// orig 0x12ba40
void CWeaponBox::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16);
}


// orig 0x12bb00
void CBasePlayerWeapon::PrintState()
{
	ALERT(at_console, "primary:  %f\n", m_flNextPrimaryAttack);
	ALERT(at_console, "idle   :  %f\n", m_flTimeWeaponIdle);

	//	ALERT( at_console, "nextrl :  %f\n", m_flNextReload );
	//	ALERT( at_console, "nextpum:  %f\n", m_flPumpTime );

	//	ALERT( at_console, "m_frt  :  %f\n", m_fReloadTime );
	ALERT(at_console, "m_finre:  %i\n", static_cast<int>(m_fInReload));
	//	ALERT( at_console, "m_finsr:  %i\n", m_fInSpecialReload );

	ALERT(at_console, "m_iclip:  %i\n", m_iClip);
}


TYPEDESCRIPTION CRpg::m_SaveData[] =
	{
		DEFINE_FIELD(CRpg, m_fSpotActive, FIELD_BOOLEAN),
		DEFINE_FIELD(CRpg, m_cActiveRockets, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CRpg, CBasePlayerWeapon);

TYPEDESCRIPTION CRpgRocket::m_SaveData[] =
	{
		DEFINE_FIELD(CRpgRocket, m_flIgniteTime, FIELD_TIME),
		DEFINE_FIELD(CRpgRocket, m_hLauncher, FIELD_EHANDLE),
};
IMPLEMENT_SAVERESTORE(CRpgRocket, CGrenade);

TYPEDESCRIPTION CShotgun::m_SaveData[] =
	{
		DEFINE_FIELD(CShotgun, m_flNextReload, FIELD_TIME),
		DEFINE_FIELD(CShotgun, m_fInSpecialReload, FIELD_INTEGER),
		DEFINE_FIELD(CShotgun, m_flNextReload, FIELD_TIME),
		// DEFINE_FIELD( CShotgun, m_iShell, FIELD_INTEGER ),
		DEFINE_FIELD(CShotgun, m_flPumpTime, FIELD_TIME),
};
IMPLEMENT_SAVERESTORE(CShotgun, CBasePlayerWeapon);

TYPEDESCRIPTION CGauss::m_SaveData[] =
	{
		DEFINE_FIELD(CGauss, m_fInAttack, FIELD_INTEGER),
		//	DEFINE_FIELD( CGauss, m_flStartCharge, FIELD_TIME ),
		//	DEFINE_FIELD( CGauss, m_flPlayAftershock, FIELD_TIME ),
		//	DEFINE_FIELD( CGauss, m_flNextAmmoBurn, FIELD_TIME ),
		DEFINE_FIELD(CGauss, m_fPrimaryFire, FIELD_BOOLEAN),
};
IMPLEMENT_SAVERESTORE(CGauss, CBasePlayerWeapon);

TYPEDESCRIPTION CEgon::m_SaveData[] =
	{
		//	DEFINE_FIELD( CEgon, m_pBeam, FIELD_CLASSPTR ),
		//	DEFINE_FIELD( CEgon, m_pNoise, FIELD_CLASSPTR ),
		//	DEFINE_FIELD( CEgon, m_pSprite, FIELD_CLASSPTR ),
		DEFINE_FIELD(CEgon, m_shootTime, FIELD_TIME),
		DEFINE_FIELD(CEgon, m_fireState, FIELD_INTEGER),
		DEFINE_FIELD(CEgon, m_fireMode, FIELD_INTEGER),
		DEFINE_FIELD(CEgon, m_shakeTime, FIELD_TIME),
		DEFINE_FIELD(CEgon, m_flAmmoUseTime, FIELD_TIME),
};
IMPLEMENT_SAVERESTORE(CEgon, CBasePlayerWeapon);

// The original's CHgun has no save table.

TYPEDESCRIPTION CSatchel::m_SaveData[] =
	{
		DEFINE_FIELD(CSatchel, m_chargeReady, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CSatchel, CBasePlayerWeapon);
