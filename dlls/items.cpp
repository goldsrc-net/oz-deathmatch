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

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "UserMessages.h"
#include "game.h"

// Oz: the rune and suit model paths are built here (bss, 256 bytes in the original)
char szCommand[256];

extern int NumPlayerStarts; // Oz: the info_player_deathmatch count ServerActivate takes (TEMP STUB at the bottom)

// Oz: a rune lying in the world. Base CItem (hierarchy.tsv); Spawn, Precache and Touch are its
// only virtuals, so CItem::Spawn's DropToFloor and ItemTouch never run for it.
class CRune : public CItem
{
public:
	void Spawn() override;
	void Precache() override;
	void Touch(CBaseEntity* pOther) override;
	void EXPORT RuneThink();
	void Move();
	void SetColor(Vector vecColor);
	void ActivateRune(CBasePlayer* pPlayer);

	int m_iRuneType;		 // orig +0x64, VERIFY name: RUNE_*
	float m_flMoveTime;		 // orig +0x68, VERIFY name: when the rune jumps to another start
	bool m_fRandom;			 // orig +0x6c (int), VERIFY name: a random rune, its type keeps changing
	float m_flNextColorTime; // orig +0x70, VERIFY name: when a random rune changes type next
							 // (orig +0x74: 4 more bytes no function touches)
};

// Oz runes

// orig 0xcb200
const char* GetRuneName(int iRune)
{
	switch (iRune)
	{
	case 0:
		return " ";
	case 1:
		return "Frag Rune";
	case 2:
		return "Regeneration Rune";
	case 3:
		return "Super Crowbar Rune";
	case 4:
		return "Haste Rune";
	case 5:
		return "Cloaking Rune";
	case 6:
		return "Super Hook Rune";
	case 7:
		return "Protection Rune";
	case 8:
		return "Low Gravity Rune";
	case 9:
		return "Vampire Rune";
	case 10:
		return "Booby Trap Rune";
	case 11:
		return "Super Glock Rune";
	case 12:
		return "Super Speed Rune";
	case 13:
		return "Identify Rune";
	case 14:
		return "Teleport Rune";
	default:
		return "Unknown Rune";
	}
}

// orig 0xcb370
const char* GetRuneDescription(int iRune)
{
	switch (iRune)
	{
	case 0:
		return "You don't have a Rune!";
	case 1:
		return UTIL_VarArgs("Frag Rune - you get %i extra point(s) for each kill!\n", (int)oz_fragruneamount.value);
	case 2:
		return "Regeneration Rune - your health and armor will continually rise!\n";
	case 3:
		return "Crowbar Rune - your crowbar has infinite damage, USE IT!\n";
	case 4:
		return "Haste Rune - your weapons cycle twice as fast!\n";
	case 5:
		return "Cloaking Rune - you are partially invisible!\n";
	case 6:
		return "Super Hook Rune - your grapple flys faster, pulls faster, and does damage!\n";
	case 7:
		return "Protection Rune - you recieve half damage!\n";
	case 8:
		return "Low Gravity Rune - you jump higher and recieve less falling damage!\n";
	case 9:
		return "Vampire Rune - you gain 1 health for every 2 damage you deal!\n";
	case 10:
		return "Booby Trap Rune - you will explode if you are killed, taking others with you!\n";
	case 11:
		return "Super Glock Rune - unlimited ammo and a deadly sniper mode!\n";
	case 12:
		return "Super Speed Rune - you can now run faster!\n";
	case 13:
		return "Identify Rune - you can now see other players health and armor values!\n";
	case 14:
		return "Teleport Rune - Bind a key to userune and you can teleport every 10 seconds!\n";
	default:
		return "Unknown Rune";
	}
}

// The glow shell color of each rune; rune_scheme above 0 picks the second set.
// orig 0xcb510
Vector GetRuneColor(int iRune)
{
	switch (iRune)
	{
	case 1:
		return Vector(255, 255, 255);
	case 2:
		if (oz_runescheme.value > 0)
			return Vector(0, 255, 0);
		return Vector(0, 250, 0);
	case 3:
		if (oz_runescheme.value > 0)
			return Vector(255, 0, 191);
		return Vector(250, 0, 0);
	case 4:
		if (oz_runescheme.value > 0)
			return Vector(255, 0, 0);
		return Vector(0, 0, 250);
	case 5:
		if (oz_runescheme.value > 0)
			return Vector(0, 127, 0);
		return Vector(0, 250, 250);
	case 6:
		if (oz_runescheme.value > 0)
			return Vector(0, 0, 127);
		return Vector(250, 150, 0);
	case 7:
		if (oz_runescheme.value > 0)
			return Vector(191, 255, 191);
		return Vector(150, 250, 150);
	case 8:
		if (oz_runescheme.value > 0)
			return Vector(0, 255, 255);
		return Vector(250, 250, 0);
	case 9:
		if (oz_runescheme.value > 0)
			return Vector(127, 255, 0);
		return Vector(250, 0, 250);
	case 10:
		if (oz_runescheme.value > 0)
			return Vector(255, 127, 0);
		return Vector(250, 150, 150);
	case 11:
		if (oz_runescheme.value > 0)
			return Vector(127, 0, 0);
		return Vector(50, 50, 50);
	case 12:
		if (oz_runescheme.value > 0)
			return Vector(0, 0, 255);
		return Vector(0, 150, 250);
	case 13:
		if (oz_runescheme.value > 0)
			return Vector(127, 127, 127);
		return Vector(150, 250, 0);
	case 14:
		if (oz_runescheme.value > 0)
			return Vector(0, 127, 255);
		return Vector(150, 0, 250);
	default:
		return Vector(0, 0, 0);
	}
}

// Is this rune turned on (its *_rune cvar above 0)?
// orig 0xcb940
bool CanHaveRune(int iRune)
{
	switch (iRune)
	{
	case 1:
		return oz_fragrunenumber.value > 0;
	case 2:
		return oz_regenrunenumber.value > 0;
	case 3:
		return oz_crowbarrunenumber.value > 0;
	case 4:
		return oz_hasterunenumber.value > 0;
	case 5:
		return oz_cloakrunenumber.value > 0;
	case 6:
		return oz_hookrunenumber.value > 0;
	case 7:
		return oz_protectrunenumber.value > 0;
	case 8:
		return oz_lowgravrunenumber.value > 0;
	case 9:
		return oz_vamprunenumber.value > 0;
	case 10:
		if (oz_traprunenumber.value > 0)
			return true;
		// Oz: no break in the original, so a disabled booby trap rune is allowed when the
		// super glock rune is on
		[[fallthrough]];
	case 11:
		return oz_superglockrunenumber.value > 0;
	case 12:
		return oz_superspeedrunenumber.value > 0;
	case 13:
		return oz_identifyrunenumber.value > 0;
	case 14:
		return oz_teleportrunenumber.value > 0;
	default:
		return false;
	}
}

// Drop a rune at a random deathmatch start. RUNE_RANDOM picks one CanHaveRune allows and makes a
// rune that keeps changing (CRune::RuneThink).
// orig 0xcbad0
void SpawnRunes(edict_t* pOwner, int iRune, int iUnused)
{
	CBaseEntity* pSpot = NULL;
	int iSpot = RANDOM_LONG(1, NumPlayerStarts);

	for (int i = 0; i < iSpot; i++)
		pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");

	// The original has no check here: a map without deathmatch starts crashed it
	// (reference/DEVIATIONS.md, Crashes in the original).
	if (!pSpot)
		return;

	CBaseEntity* pEntity = CBaseEntity::Create("oz_rune", pSpot->pev->origin, pSpot->pev->angles, pOwner);
	CRune* pRune = GetClassPtr((CRune*)pEntity->pev);

	if (iRune == RUNE_RANDOM)
	{
		do
		{
			iRune = RANDOM_LONG(1, 14);
		} while (!CanHaveRune(iRune));

		pRune->m_fRandom = true;
		pRune->m_flNextColorTime = gpGlobals->time + 5;
	}

	pRune->m_iRuneType = iRune;
	pRune->SetColor(GetRuneColor(iRune));

	pRune->pev->angles.x = 0;
	pRune->pev->angles.z = 0;
	pRune->pev->velocity.x = RANDOM_FLOAT(-300, 300);
	pRune->pev->velocity.y = RANDOM_FLOAT(-300, 300);
	pRune->pev->velocity.z = RANDOM_FLOAT(0, 300);
}

class CWorldItem : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	int m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem); // orig 0xcbc20

// orig 0xcbc40
bool CWorldItem::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

// orig 0xcbe00
void CWorldItem::Spawn()
{
	CBaseEntity* pEntity = NULL;

	switch (m_iType)
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create("item_battery", pev->origin, pev->angles);
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create("item_antidote", pev->origin, pev->angles);
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create("item_security", pev->origin, pev->angles);
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create("item_suit", pev->origin, pev->angles);
		break;
	}

	if (!pEntity)
	{
		ALERT(at_console, "unable to create world_item %d\n", m_iType);
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}


// orig 0xcbf50
void CItem::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove(this);
		return;
	}
}

// orig 0xcc050
void CItem::ItemTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
	{
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// ok, a player is touching this item, but can he have it?
	if (!g_pGameRules->CanHaveItem(pPlayer, this))
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch(pPlayer))
	{
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		SetTouch(NULL);

		// player grabbed the item.
		g_pGameRules->PlayerGotItem(pPlayer, this);
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove(this);
	}
}

// orig 0xcc140
CBaseEntity* CItem::Respawn()
{
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin(pev, g_pGameRules->VecItemRespawnSpot(this)); // blip to whereever you should respawn.

	SetThink(&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime(this);
	return this;
}

// orig 0xcc1d0
void CItem::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CItem::ItemTouch);
}

#define SF_SUIT_SHORTLOGON 0x0001

class CItemSuit : public CItem
{
	// Oz: the suit takes the rune model (rune_model, as CRune::Spawn), but no glow shell
	// orig 0x13e210
	void Spawn() override
	{
		Precache();
		const char* pszCustom = CVAR_GET_STRING("customrunemodel");
		switch ((int)oz_runemodel.value)
		{
		default:
			SET_MODEL(ENT(pev), "models/w_suit.mdl");
			break;
		case 2:
			SET_MODEL(ENT(pev), "models/chumtoad.mdl");
			break;
		case 3:
			SET_MODEL(ENT(pev), "models/crystal.mdl");
			break;
		case 4:
			SET_MODEL(ENT(pev), "models/w_longjump.mdl");
			break;
		case 5:
			SET_MODEL(ENT(pev), "models/w_oxygen.mdl");
			break;
		case 6:
			SET_MODEL(ENT(pev), "models/headcrab.mdl");
			break;
		case 7:
			ALERT(at_console, "Using custom rune model\n");
			sprintf(szCommand, "models/%s", pszCustom);
			SET_MODEL(ENT(pev), szCommand);
			break;
		}
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 128;
		CItem::Spawn();
	}
	// orig 0x13e140
	void Precache() override
	{
		PRECACHE_SOUND("items/gunpickup2.wav");
		PRECACHE_MODEL("models/w_suit.mdl");
		PRECACHE_MODEL("models/chumtoad.mdl");
		PRECACHE_MODEL("models/crystal.mdl");
		PRECACHE_MODEL("models/w_longjump.mdl");
		PRECACHE_MODEL("models/w_oxygen.mdl");
		PRECACHE_MODEL("models/headcrab.mdl");
		sprintf(szCommand, "models/%s", CVAR_GET_STRING("customrunemodel"));
		PRECACHE_MODEL(szCommand);
	}
	// orig 0x13e0c0
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->HasSuit())
			return false;

		if ((pev->spawnflags & SF_SUIT_SHORTLOGON) != 0)
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A0"); // short version of suit logon,
		else
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_AAx"); // long version of suit logon

		pPlayer->SetHasSuit(true);
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit); // orig 0xcc250



class CItemBattery : public CItem
{
	// orig 0x13e020
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_battery.mdl");
		CItem::Spawn();
	}
	// orig 0x13dfe0
	void Precache() override
	{
		PRECACHE_MODEL("models/w_battery.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	// orig 0x13de20
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->pev->deadflag != DEAD_NO)
		{
			return false;
		}

		// Oz: max_charge and battery replace MAX_NORMAL_BATTERY and sk_battery
		int iMaxCharge = (int)oz_maxcharge.value;

		if ((pPlayer->pev->armorvalue < iMaxCharge) &&
			pPlayer->HasSuit())
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += oz_battery.value;
			pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, (float)iMaxCharge);

			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(pPlayer->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			//EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery); // orig 0xcc270


class CItemAntidote : public CItem
{
	// orig 0x13dd80
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CItem::Spawn();
	}
	// orig 0x13dd50
	void Precache() override
	{
		PRECACHE_MODEL("models/w_antidote.mdl");
	}
	// orig 0x13dd10
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->SetSuitUpdate("!HEV_DET4", false, SUIT_NEXT_IN_1MIN);

		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote); // orig 0xcc290


class CItemSecurity : public CItem
{
	// orig 0x13dc70
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_security.mdl");
		CItem::Spawn();
	}
	// orig 0x13dc40
	void Precache() override
	{
		PRECACHE_MODEL("models/w_security.mdl");
	}
	// orig 0x13dc20
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity); // orig 0xcc2b0

class CItemLongJump : public CItem
{
	// orig 0x13db80
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_longjump.mdl");
		CItem::Spawn();
	}
	// orig 0x13db50
	void Precache() override
	{
		PRECACHE_MODEL("models/w_longjump.mdl");
	}
	// orig 0x13da50
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->m_fLongJump)
		{
			return false;
		}

		if (pPlayer->HasSuit())
		{
			pPlayer->m_fLongJump = true; // player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "slj", "1");

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A1"); // Play the longjump sound UNDONE: Kelly? correct sound?
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump); // orig 0xcc2d0

LINK_ENTITY_TO_CLASS(oz_rune, CRune); // orig 0xcc2f0

// orig 0xcc310
void CRune::Spawn()
{
	Precache();
	int iModel = (int)oz_runemodel.value;
	const char* pszCustom = CVAR_GET_STRING("customrunemodel");
	switch (iModel)
	{
	default:
		SET_MODEL(ENT(pev), "models/w_suit.mdl");
		break;
	case 2:
		SET_MODEL(ENT(pev), "models/chumtoad.mdl");
		break;
	case 3:
		SET_MODEL(ENT(pev), "models/crystal.mdl");
		break;
	case 4:
		SET_MODEL(ENT(pev), "models/w_longjump.mdl");
		break;
	case 5:
		SET_MODEL(ENT(pev), "models/w_oxygen.mdl");
		break;
	case 6:
		SET_MODEL(ENT(pev), "models/headcrab.mdl");
		break;
	case 7:
		sprintf(szCommand, "models/%s", pszCustom);
		SET_MODEL(ENT(pev), szCommand);
		break;
	}
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 128;

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	SetThink(&CRune::RuneThink);
	pev->nextthink = gpGlobals->time + 0.1;
	m_flMoveTime = gpGlobals->time + oz_respawntimerune.value;
}

// Oz: tests rune_scheme, not rune_model. The other models are precached anyway: W_Precache
// precaches item_suit, whose Precache takes every rune model.
// orig 0xcc560
void CRune::Precache()
{
	PRECACHE_SOUND("items/gunpickup2.wav");
	if (oz_runescheme.value > 0)
	{
		PRECACHE_MODEL("models/w_suit.mdl");
		PRECACHE_MODEL("models/chumtoad.mdl");
		PRECACHE_MODEL("models/crystal.mdl");
		PRECACHE_MODEL("models/w_longjump.mdl");
		PRECACHE_MODEL("models/w_oxygen.mdl");
		PRECACHE_MODEL("models/headcrab.mdl");
		sprintf(szCommand, "models/%s", CVAR_GET_STRING("customrunemodel"));
		PRECACHE_MODEL(szCommand);
	}
	else
	{
		PRECACHE_MODEL("models/w_suit.mdl");
	}
}

// orig 0xcc670
void CRune::Touch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	if (pPlayer->m_iRune > 0 || pPlayer->pev->deadflag != DEAD_NO)
		return;

	pPlayer->m_iRune = m_iRuneType;
	if (m_fRandom)
		pPlayer->m_iRandomRune = 1;

	// Oz: sets the rune's own maxspeed, not the player's; ActivateRune sets the player's
	if (pPlayer->IsNetClient())
		pev->maxspeed = oz_playerspeed.value;
	else
		pev->maxspeed = bot_speed.value;

	ActivateRune(pPlayer);
	EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	RuneHud(pPlayer, GetRuneDescription(m_iRuneType));

	if (g_teamplay)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" Picked up the  \"%s\".\n",
			STRING(pPlayer->pev->netname),
			GETPLAYERUSERID(pPlayer->edict()),
			GETPLAYERAUTHID(pPlayer->edict()),
			g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model"),
			GetRuneName(pPlayer->m_iRune));
	}
	else
	{
		// Oz: the user id fills the team field too
		UTIL_LogPrintf("\"%s<%i><%s><%i>\" Picked up the \"%s\".\n",
			STRING(pPlayer->pev->netname),
			GETPLAYERUSERID(pPlayer->edict()),
			GETPLAYERAUTHID(pPlayer->edict()),
			GETPLAYERUSERID(pPlayer->edict()),
			GetRuneName(pPlayer->m_iRune));
	}

	UTIL_Remove(this);
}

// Spin; a random rune changes type every 5 seconds; every respawntime_rune seconds the rune
// jumps to another deathmatch start.
// orig 0xcc8b0
void CRune::RuneThink()
{
	float flTurn = fmod(gpGlobals->time, 2);
	pev->angles.x = 0;
	pev->angles.y = flTurn * 180 - 90;
	pev->angles.z = 0;

	if (m_fRandom && m_flNextColorTime <= gpGlobals->time)
	{
		do
		{
			m_iRuneType = RANDOM_LONG(1, 14);
		} while (!CanHaveRune(m_iRuneType));

		SetColor(GetRuneColor(m_iRuneType));
		m_flNextColorTime = gpGlobals->time + 5;
	}

	if (m_flMoveTime <= gpGlobals->time)
		Move();

	pev->nextthink = gpGlobals->time + 0.1;
}

// orig 0xcc9e0
void CRune::Move()
{
	CBaseEntity* pSpot = NULL;
	int iSpot = RANDOM_LONG(1, NumPlayerStarts);

	for (int i = 0; i < iSpot; i++)
		pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");

	// The original moves to pSpot unchecked, a crash on a map without deathmatch starts; ours
	// leaves the rune where it is until the next move.
	if (!pSpot)
	{
		m_flMoveTime = gpGlobals->time + oz_respawntimerune.value;
		return;
	}

	pev->origin = pSpot->pev->origin;
	pev->angles.x = 0;
	pev->angles.z = 0;
	pev->velocity.x = RANDOM_FLOAT(-300, 300);
	pev->velocity.y = RANDOM_FLOAT(-300, 300);
	pev->velocity.z = RANDOM_FLOAT(0, 300);
	m_flMoveTime = gpGlobals->time + oz_respawntimerune.value;
}

// orig 0xccae0
void CRune::SetColor(Vector vecColor)
{
	pev->renderfx |= kRenderFxGlowShell;
	pev->rendercolor = vecColor;
	pev->renderamt = 30;
}

// The pickup side of each rune; the rest lives with the player and weapon code that reads
// m_iRune.
// orig 0xccb30
void CRune::ActivateRune(CBasePlayer* pPlayer)
{
	switch (m_iRuneType)
	{
	case RUNE_FRAG:
	case RUNE_REGEN:
	case RUNE_CLOAK:
	case RUNE_HOOK:
	case RUNE_PROTECT:
	case RUNE_VAMPIRE:
	case RUNE_TRAP:
	case RUNE_IDENTIFY:
	case RUNE_TELEPORT:
		if (pPlayer->IsNetClient())
			pPlayer->pev->maxspeed = oz_playerspeed.value;
		else
			pPlayer->pev->maxspeed = bot_speed.value;
		break;

	case RUNE_CROWBAR:
		if (pPlayer->IsNetClient())
			pPlayer->pev->maxspeed = oz_playerspeed.value;
		else
			pPlayer->pev->maxspeed = bot_speed.value;
		if (!pPlayer->HasNamedPlayerItem("weapon_crowbar"))
			pPlayer->GiveNamedItem("weapon_crowbar");
		break;

	case RUNE_HASTE:
		if (pPlayer->IsNetClient())
			pPlayer->pev->maxspeed = oz_playerspeed.value * 0.8;
		else
			pPlayer->pev->maxspeed = bot_speed.value;
		break;

	case RUNE_LOWGRAV:
		pPlayer->pev->gravity = 1;
		pPlayer->pev->gravity *= oz_lowgravruneamount.value;
		if (pPlayer->IsNetClient())
			pPlayer->pev->maxspeed = oz_playerspeed.value * 1.25;
		else
			pPlayer->pev->maxspeed = bot_speed.value;
		break;

	case RUNE_SUPERGLOCK:
		if (!pPlayer->HasNamedPlayerItem("weapon_9mmhandgun"))
			pPlayer->GiveNamedItem("weapon_9mmhandgun");
		pPlayer->GiveAmmo((int)oz_magazineglock.value, "9mm", (int)oz_max9mm.value);
		if (pPlayer->IsNetClient())
			pPlayer->pev->maxspeed = oz_playerspeed.value;
		else
			pPlayer->pev->maxspeed = bot_speed.value;
		break;

	case RUNE_SUPERSPEED:
		if (pPlayer->IsNetClient())
			pPlayer->pev->maxspeed = CVAR_GET_FLOAT("sv_maxspeed");
		else
			pPlayer->pev->maxspeed = bot_speed.value;
		break;
	}
}

// orig 0xccd90
void CItem::RuneHud(CBasePlayer* pPlayer, const char* pszText)
{
	EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

	hudtextparms_t textparms;
	memset(&textparms, 0, sizeof(textparms));
	textparms.channel = 1;
	textparms.x = 0.04;
	textparms.y = 0.75;
	textparms.effect = 0;
	textparms.r1 = 0;
	textparms.g1 = 255;
	textparms.b1 = 0;
	textparms.a1 = 255;
	textparms.r2 = 0;
	textparms.g2 = 255;
	textparms.b2 = 0;
	textparms.a2 = 255;
	textparms.fadeinTime = 0;
	textparms.fadeoutTime = 3;
	textparms.holdTime = 5;
	textparms.fxTime = 0.5;
	UTIL_HudMessage(pPlayer, textparms, pszText);

	pPlayer->m_iRuneHudTime = 700;
}

// TEMP STUB (items): real one belongs in client.cpp (VERIFY: the defining file; ServerActivate,
// client.cpp, counts the starts into it)
int NumPlayerStarts = 0;
