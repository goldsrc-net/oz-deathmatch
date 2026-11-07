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

===== player.cpp ========================================================

  functions dealing with the player

*/

#include <limits>
#include <algorithm>

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "pm_shared.h"
#include "hltv.h"
#include "UserMessages.h"
#include "client.h"

// #define DUCKFIX

extern void CopyToBodyQue(entvars_t* pev);
extern edict_t* EntSelectSpawnPoint(CBaseEntity* pPlayer);

extern bool IsBustingGame();

// Oz: from OZ's other files
extern const char* GetRuneName(int iRune);					// items.cpp, orig 0xcb200
extern void SpawnRunes(edict_t* pEdict, int iRune, int iCount); // items.cpp, orig 0xcbad0
extern DLL_GLOBAL bool g_runes_exist;						// globals.cpp in the original

CBaseEntity* FindEntityForward(CBaseEntity* pMe);

#define TRAIN_ACTIVE 0x80
#define TRAIN_NEW 0xc0
#define TRAIN_OFF 0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW 0x02
#define TRAIN_MEDIUM 0x03
#define TRAIN_FAST 0x04
#define TRAIN_BACK 0x05

#define FLASH_DRAIN_TIME 1.2  //100 units/3 minutes
#define FLASH_CHARGE_TIME 0.2 // 100 units/20 seconds  (seconds per unit)

// Global Savedata for player
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
	{
		DEFINE_FIELD(CBasePlayer, m_flFlashLightTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_iFlashBattery, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),

		DEFINE_ARRAY(CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS),
		DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_flTimeStepSound, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flSwimTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flDuckTime, FIELD_TIME),
		DEFINE_FIELD(CBasePlayer, m_flWallJumpTime, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_flSuitUpdate, FIELD_TIME),
		DEFINE_ARRAY(CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
		DEFINE_FIELD(CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER),
		DEFINE_ARRAY(CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
		DEFINE_ARRAY(CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
		DEFINE_FIELD(CBasePlayer, m_lastDamageAmount, FIELD_INTEGER),

		DEFINE_ARRAY(CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES),
		DEFINE_FIELD(CBasePlayer, m_pActiveItem, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayer, m_pLastItem, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayer, m_WeaponBits, FIELD_INT64),

		DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
		DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_tSneaking, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayer, m_iTargetVolume, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iWeaponVolume, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iWeaponFlash, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_fLongJump, FIELD_BOOLEAN),
		DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
		DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),

		DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
		DEFINE_FIELD(CBasePlayer, m_hViewEntity, FIELD_EHANDLE),
		DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),

		DEFINE_FIELD(CBasePlayer, m_SndRoomtype, FIELD_INTEGER),
		// Don't save these. Let the game recalculate the closest env_sound, and continue to use the last room type like it always has.
		//DEFINE_FIELD(CBasePlayer, m_SndLast, FIELD_EHANDLE),
		//DEFINE_FIELD(CBasePlayer, m_flSndRange, FIELD_FLOAT),

		DEFINE_FIELD(CBasePlayer, m_flStartCharge, FIELD_TIME),

		//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
		//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
		//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
		//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_BOOLEAN ), // reset to zero on load
		//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
		//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
		//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
		//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
		//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
		//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
		//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
		//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
		//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
		//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
		//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore

};

LINK_ENTITY_TO_CLASS(player, CBasePlayer);



// orig 0xf07f0
void CBasePlayer::Pain()
{
	float flRndSound; //sound randomizer

	flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.33)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if (flRndSound <= 0.66)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
}

/* 
 *
 */
// orig 0xf0890
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

#if 0 /*                                                                 \
static void ThrowGib(entvars_t *pev, char *szGibModel, float flDamage)   \
{                                                                        \
	edict_t *pentNew = CREATE_ENTITY();                                  \
	entvars_t *pevNew = VARS(pentNew);                                   \
																	   \ \
	pevNew->origin = pev->origin;                                        \
	SET_MODEL(ENT(pevNew), szGibModel);                                  \
	UTIL_SetSize(pevNew, g_vecZero, g_vecZero);                          \
																	   \ \
	pevNew->velocity		= VecVelocityForDamage(flDamage);                  \
	pevNew->movetype		= MOVETYPE_BOUNCE;                                 \
	pevNew->solid			= SOLID_NOT;                                         \
	pevNew->avelocity.x		= RANDOM_FLOAT(0,600);                          \
	pevNew->avelocity.y		= RANDOM_FLOAT(0,600);                          \
	pevNew->avelocity.z		= RANDOM_FLOAT(0,600);                          \
	CHANGE_METHOD(ENT(pevNew), em_think, SUB_Remove);                    \
	pevNew->ltime		= gpGlobals->time;                                    \
	pevNew->nextthink	= gpGlobals->time + RANDOM_FLOAT(10,20);           \
	pevNew->frame		= 0;                                                  \
	pevNew->flags		= 0;                                                  \
}                                                                        \
																	   \ \
																	   \ \
static void ThrowHead(entvars_t *pev, char *szGibModel, floatflDamage)   \
{                                                                        \
	SET_MODEL(ENT(pev), szGibModel);                                     \
	pev->frame			= 0;                                                    \
	pev->nextthink		= -1;                                                \
	pev->movetype		= MOVETYPE_BOUNCE;                                    \
	pev->takedamage		= DAMAGE_NO;                                        \
	pev->solid			= SOLID_NOT;                                            \
	pev->view_ofs		= Vector(0,0,8);                                      \
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,56));              \
	pev->velocity		= VecVelocityForDamage(flDamage);                     \
	pev->avelocity		= RANDOM_FLOAT(-1,1) * Vector(0,600,0);              \
	pev->origin.z -= 24;                                                 \
	ClearBits(pev->flags, FL_ONGROUND);                                  \
}                                                                        \
																	   \ \
																	   \ \
*/
#endif

// orig 0xf09c0
int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

// orig 0xf0a50
void CBasePlayer::DeathSound()
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1, 5))
	{
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT(ENT(pev), "HEV_DEAD");
}

// override takehealth
// bitsDamageType indicates type of damage healed.

// orig 0xf0b10
bool CBasePlayer::TakeHealth(float flHealth, int bitsDamageType)
{
	return CBaseMonster::TakeHealth(flHealth, bitsDamageType);
}

// orig 0xf0b40
Vector CBasePlayer::GetGunPosition()
{
	//	UTIL_MakeVectors(pev->v_angle);
	//	m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;

	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
// orig 0xf0bb0. Oz: the hitgroup multipliers are the oz_*damage cvars, and the attacker, if a
// net client, gets a center print naming the hitgroup.
void CBasePlayer::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (0 != pev->takedamage)
	{
		// VERIFY: the original leaves this uninitialized for HITGROUP_GENERIC (and any hitgroup
		// above 7) and prints whatever the stack held; NULL prints an empty center message
		const char* pszHit = nullptr;

		m_LastHitGroup = ptr->iHitgroup;

		switch (ptr->iHitgroup)
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			pszHit = "\n\nHead Shot!\n";
			flDamage *= oz_headdamage.value;
			break;
		case HITGROUP_CHEST:
			pszHit = "\n\nChest Shot!\n";
			flDamage *= oz_chestdamage.value;
			break;
		case HITGROUP_STOMACH:
			pszHit = "\n\nStomach Shot!\n";
			flDamage *= oz_stomachdamage.value;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			pszHit = "\n\nArm Shot!\n";
			flDamage *= oz_armdamage.value;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			pszHit = "\n\nLeg Shot!\n";
			flDamage *= oz_legdamage.value;
			break;
		default:
			break;
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage); // a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);

		if (CBaseEntity::Instance(pevAttacker)->IsNetClient())
			ClientPrint(pevAttacker, HUD_PRINTCENTER, pszHit);
	}
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO 0.2 // Armor Takes 80% of the damage
#define ARMOR_BONUS 0.5 // Each Point of Armor is work 1/x points of health

// orig 0xf0db0. Oz: a multiplayer blast shakes the screen (oz_shake_*); the protection rune
// halves the damage after armor; the vampire rune gives its attacker half the damage as health,
// to oz_maxhealthwithvamprune; a player killed by himself is marked wassuicide (no booby trap).
bool CBasePlayer::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	bool ffound = true;
	bool fmajor;
	bool fcritical;
	bool fTookDamage;
	bool ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ((bitsDamageType & DMG_BLAST) != 0 && g_pGameRules->IsMultiplayer())
	{
		if (oz_shake_enable.value == 1)
			UTIL_ScreenShake(pev->origin, oz_shake_amp.value, oz_shake_freq.value, oz_shake_dur.value, oz_shake_radius.value);

		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if (!IsAlive())
		return false;
	// go take the damage first


	CBaseEntity* pAttacker = CBaseEntity::Instance(pevAttacker);

	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		// Refuse the damage
		return false;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Oz: both through GetClassPtr, which would create a player for a NULL pevAttacker (the
	// original reads pevAttacker above already, so it never is)
	CBasePlayer* plr = GetClassPtr((CBasePlayer*)pev);
	CBasePlayer* plrAttacker = GetClassPtr((CBasePlayer*)pevAttacker);

	// Armor.
	if (0 != pev->armorvalue && (bitsDamageType & (DMG_FALL | DMG_DROWN)) == 0) // armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor;

		flArmor = (flDamage - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > pev->armorvalue)
		{
			flArmor = pev->armorvalue;
			flArmor *= (1 / flBonus);
			flNew = flDamage - flArmor;
			pev->armorvalue = 0;
		}
		else
			pev->armorvalue -= flArmor;

		flDamage = flNew;
	}

	// Oz runes - protection halving
	if (plr->m_iRune == 7) // Protection Rune
		flDamage *= 0.5;

	// Oz runes - vampire: the attacker drinks half the damage
	if (plrAttacker->IsPlayer() && plrAttacker->m_iRune == 9 && plr != plrAttacker) // Vampire Rune
	{
		if (plrAttacker->pev->health < oz_maxhealthwithvamprune.value)
			plrAttacker->pev->health += flDamage * 0.5;

		if (plrAttacker->pev->health > oz_maxhealthwithvamprune.value)
			plrAttacker->pev->health = oz_maxhealthwithvamprune.value;
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// Oz: killed by himself
	if (pev->health <= 0 && plrAttacker->IsPlayer() && pevAttacker == plr->pev)
		wassuicide = 1;

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if ((bitsDamageType & (DMG_PARALYZE << i)) != 0)
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);							  // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);				  // take damage event
	WRITE_SHORT(ENTINDEX(this->edict()));	  // index number of primary entity
	WRITE_SHORT(ENTINDEX(ENT(pevInflictor))); // index number of secondary entity
	WRITE_LONG(5);							  // eventflags (priority and flags)
	MESSAGE_END();


	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

	// DMG_BURN
	// DMG_FREEZE
	// DMG_BLAST
	// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;			// make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED) != 0) && ffound && 0 != bitsDamage)
	{
		ffound = false;

		if ((bitsDamage & DMG_CLUB) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = true;
		}
		if ((bitsDamage & (DMG_FALL | DMG_CRUSH)) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", false, SUIT_NEXT_IN_30SEC); // major fracture
			else
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // minor fracture

			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = true;
		}

		if ((bitsDamage & DMG_BULLET) != 0)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", false, SUIT_NEXT_IN_30SEC); // blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_BULLET;
			ffound = true;
		}

		if ((bitsDamage & DMG_SLASH) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", false, SUIT_NEXT_IN_30SEC); // major laceration
			else
				SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC); // minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = true;
		}

		if ((bitsDamage & DMG_SONIC) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_1MIN); // internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = true;
		}

		if ((bitsDamage & (DMG_POISON | DMG_PARALYZE)) != 0)
		{
			SetSuitUpdate("!HEV_DMG3", false, SUIT_NEXT_IN_1MIN); // blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = true;
		}

		if ((bitsDamage & DMG_ACID) != 0)
		{
			SetSuitUpdate("!HEV_DET1", false, SUIT_NEXT_IN_1MIN); // hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = true;
		}

		if ((bitsDamage & DMG_NERVEGAS) != 0)
		{
			SetSuitUpdate("!HEV_DET0", false, SUIT_NEXT_IN_1MIN); // biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = true;
		}

		if ((bitsDamage & DMG_RADIATION) != 0)
		{
			SetSuitUpdate("!HEV_DET2", false, SUIT_NEXT_IN_1MIN); // radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = true;
		}
		if ((bitsDamage & DMG_SHOCK) != 0)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = true;
		}
	}

	pev->punchangle.x = -2;

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75)
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", false, SUIT_NEXT_IN_30MIN); // automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN); // morphine shot
	}

	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN); // near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN); // health critical

		// give critical health warnings
		if (!RANDOM_LONG(0, 3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) != 0 && flHealthPrev < 75)
	{
		if (flHealthPrev < 50)
		{
			if (!RANDOM_LONG(0, 3))
				SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); //seek medical attention
		}
		else
			SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN); // health dropping
	}

	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
// orig 0xf15e0. SDK 2.3's. halflife-updated's reload of the active weapon before packing is
// out (design, as mod-rc2/mod-rockcrowbar); its empty-satchel check stays (fix), and its busters
// branch is inert unless sv_busters is set.
void CBasePlayer::PackDeadPlayerItems()
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon* rgpPackWeapons[MAX_WEAPONS];
	int iPackAmmo[MAX_AMMO_SLOTS + 1];
	int iPW = 0; // index into packweapons array
	int iPA = 0; // index into packammo array

	memset(rgpPackWeapons, 0, sizeof(rgpPackWeapons));
	memset(iPackAmmo, -1, sizeof(iPackAmmo));

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons(this);
	iAmmoRules = g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO)
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems(true);
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem* pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				switch (iWeaponRules)
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if (m_pActiveItem && pPlayerItem == m_pActiveItem)
					{
						// this is the active item. Pack it.
						rgpPackWeapons[iPW++] = (CBasePlayerWeapon*)pPlayerItem;
					}
					break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[iPW++] = (CBasePlayerWeapon*)pPlayerItem;
					break;

				default:
					break;
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	// now go through ammo and make a list of which types to pack.
	if (iAmmoRules != GR_PLR_DROP_AMMO_NO)
	{
		for (i = 0; i < MAX_AMMO_SLOTS; i++)
		{
			if (m_rgAmmo[i] > 0)
			{
				// player has some ammo of this type.
				switch (iAmmoRules)
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[iPA++] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if (m_pActiveItem && i == m_pActiveItem->PrimaryAmmoIndex())
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if (m_pActiveItem && i == m_pActiveItem->SecondaryAmmoIndex())
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

	// create a box to pack the stuff into.
	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin, pev->angles, edict());

	pWeaponBox->pev->angles.x = 0; // don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink(&CWeaponBox::Kill);
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

	if (IsBustingGame())
	{
		if (HasNamedPlayerItem("weapon_egon"))
		{
			for (i = 0; i < MAX_ITEM_TYPES; i++)
			{
				CBasePlayerItem* pItem = m_rgpPlayerItems[i];

				if (pItem)
				{
					if (!strcmp("weapon_egon", STRING(pItem->pev->classname)))
					{
						pWeaponBox->PackWeapon(pItem);

						SET_MODEL(ENT(pWeaponBox->pev), "models/w_egon.mdl");

						pWeaponBox->pev->velocity = Vector(0, 0, 0);
						pWeaponBox->pev->renderfx = kRenderFxGlowShell;
						pWeaponBox->pev->renderamt = 25;
						pWeaponBox->pev->rendercolor = Vector(0, 75, 250);

						break;
					}
				}
			}
		}
	}
	else
	{
		bool bPackItems = true;

		if (iAmmoRules == GR_PLR_DROP_AMMO_ACTIVE && iWeaponRules == GR_PLR_DROP_GUN_ACTIVE)
		{
			if (rgpPackWeapons[0] && FClassnameIs(rgpPackWeapons[0]->pev, "weapon_satchel") && (iPackAmmo[0] == -1 || (m_rgAmmo[iPackAmmo[0]] == 0)))
			{
				bPackItems = false;
			}
		}

		if (bPackItems)
		{
			// pack the ammo
			while (iPackAmmo[iPA] != -1)
			{
				pWeaponBox->PackAmmo(MAKE_STRING(CBasePlayerItem::AmmoInfoArray[iPackAmmo[iPA]].pszName), m_rgAmmo[iPackAmmo[iPA]]);
				iPA++;
			}

			// now pack all of the items in the lists
			while (rgpPackWeapons[iPW])
			{
				// weapon unhooked from the player. Pack it into der box.
				pWeaponBox->PackWeapon(rgpPackWeapons[iPW]);

				iPW++;
			}
		}

		pWeaponBox->pev->velocity = pev->velocity * 1.2; // weaponbox has player's velocity, then some.
	}

	RemoveAllItems(true); // now strip off everything that wasn't handled by the code above.
}

// orig 0xf1940. SDK 2.3's, ending with an empty CurWeapon (halflife-updated moved that into
// UpdateClientData: design, as mod-rockcrowbar). halflife-updated's func_tank release stays (fix).
void CBasePlayer::RemoveAllItems(bool removeSuit)
{
	if (m_pActiveItem)
	{
		ResetAutoaim();
		m_pActiveItem->Holster();
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	int i;
	CBasePlayerItem* pPendingItem;
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while (m_pActiveItem)
		{
			pPendingItem = m_pActiveItem->m_pNext;
			m_pActiveItem->Drop();
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;

	pev->viewmodel = 0;
	pev->weaponmodel = 0;

	m_WeaponBits = 0ULL;
	// Mirror m_WeaponBits = 0 onto the documented pev->weapons mask so
	// server-side consumers (Metamod plugins, the engine's
	// cdata->iWeaponBits delivery to vanilla clients) see the wipe too.
	// SetHasSuit() below goes through SetWeaponBit() / ClearWeaponBit(),
	// which are paired to maintain pev->weapons in lockstep with
	// m_WeaponBits.
	pev->weapons = 0;

	//Re-add suit bit if needed.
	SetHasSuit(!removeSuit);

	for (i = 0; i < MAX_AMMO_SLOTS; i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();

	// send Selected Weapon Message to our client
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */

// orig 0xf1af0. Oz: the flashlight goes off first; pev->fov is zeroed, as in the original.
// The Health message stays halflife-updated's short (fix; LinkUserMessages registers 2 bytes). halflife-updated's multiplayer non-solid corpse (HL25 backport
// df5605d) is out (design); its respawn count reset stays (fix, as mod-rc2).
void CBasePlayer::Killed(entvars_t* pevAttacker, int iGib)
{
	CSound* pSound;

	FlashlightTurnOff();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	g_pGameRules->PlayerKilled(this, pevAttacker, g_pevLastInflictor);

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	{
		if (pSound)
		{
			pSound->Reset();
		}
	}

	SetAnimation(PLAYER_DIE);

	m_flRespawnTimer = 0.0f;

	pev->modelindex = g_ulModelIndexPlayer; // don't use eyes

	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_TOSS;
	ClearBits(pev->flags, FL_ONGROUND);
	if (pev->velocity.z < 10)
		pev->velocity.z += RANDOM_FLOAT(0, 300);

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, false, 0);

	// send "health" update message to zero
	m_iClientHealth = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
	WRITE_SHORT(m_iClientHealth);
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0XFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	if ((pev->health < -40 && iGib != GIB_NEVER) || iGib == GIB_ALWAYS)
	{
		pev->solid = SOLID_NOT;
		GibMonster(); // This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}

	DeathSound();

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


// Set the activity based on an event or current state
// orig 0xf1e10
void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if ((pev->flags & FL_FROZEN) != 0)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch (playerAnim)
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;

	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;

	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity();
		break;

	case PLAYER_ATTACK1:
		switch (m_Activity)
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if (!FBitSet(pev->flags, FL_ONGROUND) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP)) // Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if (pev->waterlevel > 1)
		{
			if (speed == 0)
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if (m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity(m_Activity);
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		//ALERT(at_console, "Set die animation to %d\n", animDesired);

		pev->gaitsequence = 0;
		pev->sequence = animDesired;
		pev->frame = 0;
		ResetSequenceInfo();
		return;

	case ACT_RANGE_ATTACK1:
		if (FBitSet(pev->flags, FL_DUCKING)) // crouching
			strcpy(szAnim, "crouch_shoot_");
		else
			strcpy(szAnim, "ref_shoot_");
		strcat(szAnim, m_szAnimExtention);
		animDesired = LookupSequence(szAnim);
		if (animDesired == -1)
			animDesired = 0;

		if (pev->sequence != animDesired || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence = animDesired;
		ResetSequenceInfo();
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if (FBitSet(pev->flags, FL_DUCKING)) // crouching
				strcpy(szAnim, "crouch_aim_");
			else
				strcpy(szAnim, "ref_aim_");
			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		if (speed == 0)
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCHIDLE);
			// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCH);
		}
	}
	else if (speed > 220)
	{
		pev->gaitsequence = LookupActivity(ACT_RUN);
	}
	else if (speed > 0)
	{
		pev->gaitsequence = LookupActivity(ACT_WALK);
	}
	else
	{
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
		pev->gaitsequence = LookupSequence("deep_idle");
	}


	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}

/*
===========
TabulateAmmo
This function is used to find and store 
all the ammo we have into the ammo vars.
============
*/
// orig 0xf2310
void CBasePlayer::TabulateAmmo()
{
	ammo_9mm = AmmoInventory(GetAmmoIndex("9mm"));
	ammo_357 = AmmoInventory(GetAmmoIndex("357"));
	ammo_argrens = AmmoInventory(GetAmmoIndex("ARgrenades"));
	ammo_bolts = AmmoInventory(GetAmmoIndex("bolts"));
	ammo_buckshot = AmmoInventory(GetAmmoIndex("buckshot"));
	ammo_rockets = AmmoInventory(GetAmmoIndex("rockets"));
	ammo_uranium = AmmoInventory(GetAmmoIndex("uranium"));
	ammo_hornets = AmmoInventory(GetAmmoIndex("Hornets"));
}


/*
===========
WaterMove
============
*/
#define AIRTIME 12 // lung full of air lasts this many seconds

// orig 0xf23f0. Oz: a lungful lasts oz_playerair seconds, not AIRTIME. halflife-updated's
// count of the drowning damage really taken and its head-under check for the swim sounds stay
// (fixes, as mod-rockcrowbar).
void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3)
	{
		// not underwater

		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + oz_playerair.value;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{ // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time) // drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;

				const float oldHealth = pev->health;

				TakeDamage(CWorld::World->pev, CWorld::World->pev, pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				// Account for god mode, the unkillable flag, potential damage mitigation
				// and gaining health from drowning to avoid counting damage not actually taken.
				const float drownDamageTaken = std::max(0.f, std::floor(oldHealth - pev->health));

				m_idrowndmg += drownDamageTaken;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (0 == pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{
			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}

	// make bubbles

	if (pev->waterlevel == 3)
	{
		air = (int)(pev->air_finished - gpGlobals->time);
		if (!RANDOM_LONG(0, 0x1f) && RANDOM_LONG(0, (int)(oz_playerair.value - 1)) >= air)
		{
			switch (RANDOM_LONG(0, 3))
			{
			case 0:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM);
				break;
			case 1:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM);
				break;
			case 2:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM);
				break;
			case 3:
				EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM);
				break;
			}
		}
	}

	if (pev->watertype == CONTENT_LAVA) // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(CWorld::World->pev, CWorld::World->pev, 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME) // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(CWorld::World->pev, CWorld::World->pev, 4 * pev->waterlevel, DMG_ACID);
	}

	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}


// true if the player is attached to a ladder
// orig 0xf2900
bool CBasePlayer::IsOnLadder()
{
	return (pev->movetype == MOVETYPE_FLY);
}

// orig 0xf2920. Oz: after the death animation the grapple lets go, a booby trap carrier who did
// not kill himself blows up (the player becomes a 999-damage grenade's owner and its classname
// "Booby Trap Rune", for the death notice), the rune is logged and dropped (RemoveRune); after
// six seconds dead a multiplayer player respawns into OZ's observer mode (deathmatch: spectating
// from a spawn point; teamplay: through PlayerSpawn's first-join observer) instead of the death
// cam. halflife-updated's time-based wait (the original counts 120 frames) stays, as mod-rc2;
// its HL25 body copy when the animation ends (ac6b664) and its spectator check are out
// (design: OZ's observer is SDK 2.2's).
void CBasePlayer::PlayerDeathThink()
{
	float flForward;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if (HasWeapons())
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}


	if (0 != pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		StudioFrameAdvance();

		m_flRespawnTimer += gpGlobals->frametime;
		if (m_flRespawnTimer < 4.0f) // 120 frames at 30 fps -- animations should be no longer than this
			return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if (pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND))
		pev->movetype = MOVETYPE_NONE;

	if (pev->deadflag == DEAD_DYING)
		pev->deadflag = DEAD_DEAD;

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	// Oz Grapple
	CBasePlayer* plr = GetClassPtr((CBasePlayer*)pev);
	if (plr->pGrappleBolt)
	{
		if (IsHanging)
		{
			IsHanging = false;
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
		}
		pev->movetype = MOVETYPE_WALK;
		pev->gravity = 1;
		// MERGE: CGrappleBolt is grapple.h's (a CBaseEntity); call Killed on it directly once included
		reinterpret_cast<CBaseEntity*>(plr->pGrappleBolt)->Killed(NULL, 0);
		plr->pGrappleBolt = NULL;
	}

	// Oz Runes - booby trap
	if (m_iRune == 10 && 0 == wassuicide) // Booby Trap Rune
	{
		// the new grenade entity is never used (ShootTimed is static and makes its own): an
		// empty edict is left behind every time (original bug)
		CGrenade* pGrenade = GetClassPtr((CGrenade*)NULL);
		pev->classname = MAKE_STRING("Booby Trap Rune");
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		pev->dmg = 999;
		pev->nextthink = 0;
		pGrenade->ShootTimed(plr->pev, Center(), plr->pev->angles, 0);
	}

	if (m_iRune != 0)
	{
		if (!g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" Died while wearing the \"%s\".\n",
				STRING(pev->netname),
				GETPLAYERUSERID(edict()),
				GETPLAYERAUTHID(edict()),
				GETPLAYERUSERID(edict()),
				GetRuneName(m_iRune));
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" Died while wearing the  \"%s\".\n",
				STRING(pev->netname),
				GETPLAYERUSERID(edict()),
				GETPLAYERAUTHID(edict()),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(edict()), "model"),
				GetRuneName(m_iRune));
		}
	}

	RemoveRune();

	bool fAnyButtonDown = (pev->button & ~IN_SCORE) != 0;

	// wait for all buttons released
	if (pev->deadflag == DEAD_DEAD)
	{
		if (fAnyButtonDown)
			return;

		if (g_pGameRules->FPlayerCanRespawn(this))
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}

		return;
	}

	// Oz observer mode, in place of SDK's death cam
	if (g_pGameRules->IsMultiplayer() && (gpGlobals->time > (m_fDeadTime + 6)) && (m_afPhysicsFlags & PFLAG_OBSERVER) == 0)
	{
		if (CVAR_GET_FLOAT("mp_teamplay") == 0)
		{
			m_iFirst = 1;
			pev->flags = FL_SPECTATOR;
			plr->m_fNextSuicideTime = gpGlobals->time + 1;
			respawn(pev, (m_afPhysicsFlags & PFLAG_OBSERVER) == 0);

			CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
			edict_t* pentSpawnSpot = EntSelectSpawnPoint(pPlayer);
			pPlayer->StartObserver(VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles);
		}
		else
		{
			m_iFirst = 0;
			pev->flags = FL_SPECTATOR;
			plr->m_fNextSuicideTime = gpGlobals->time + 1;
			respawn(pev, (m_afPhysicsFlags & PFLAG_OBSERVER) == 0);
		}
	}

	// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	if (!fAnyButtonDown && !(g_pGameRules->IsMultiplayer() && forcerespawn.value > 0 && (gpGlobals->time > (m_fDeadTime + 5))))
		return;

	pev->button = 0;
	m_flRespawnTimer = 0.0f;

	//ALERT(at_console, "Respawn\n");

	respawn(pev, (m_afPhysicsFlags & PFLAG_OBSERVER) == 0); // don't copy a corpse if we're in deathcam.
	pev->nextthink = -1;
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
// orig 0xf2fe0. Oz: no intermission spot search: the camera goes up from the corpse, into
// StartObserver, with a "Start Deathcam" center print. Nothing in 3.02 calls it.
void CBasePlayer::StartDeathCam()
{
	if (pev->view_ofs == g_vecZero)
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	// no intermission spot. Push them up in the air, looking down at their corpse
	TraceResult tr;
	CopyToBodyQue(pev);
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, 128), ignore_monsters, edict(), &tr);

	StartObserver(tr.vecEndPos, UTIL_VecToAngles(tr.vecEndPos - pev->origin));
	ClientPrint(pev, HUD_PRINTCENTER, "Start Deathcam");
}

// orig 0xf3120. Oz: SDK 2.2's spectator, deathmatch only (mp_teamplay 0): chase the next player
// (iuser1 2), health 0, FL_SPECTATOR, m_iObserver set. In teamplay only SDK's parked view.
void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle)
{
	if (CVAR_GET_FLOAT("mp_teamplay") == 0)
	{
		// Holster weapon immediately, to allow it to cleanup
		if (m_pActiveItem)
			m_pActiveItem->Holster();

		if (m_pTank != NULL)
		{
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}

		// clear out the suit message cache so we don't keep chattering
		SetSuitUpdate(NULL, false, 0);

		// Tell Ammo Hud that the player is dead
		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
		MESSAGE_END();

		// reset FOV
		m_iFOV = m_iClientFOV = 0;
		pev->fov = 0;
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		// Setup flags
		m_iHideHUD |= (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
		pev->flags = FL_SPECTATOR;
		m_afPhysicsFlags |= PFLAG_OBSERVER;
		pev->effects = EF_NODRAW;
		pev->view_ofs = g_vecZero;
		pev->angles = pev->v_angle = vecViewAngle;
		pev->fixangle = 1;
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
		pev->movetype = MOVETYPE_NOCLIP;
		ClearBits(m_afPhysicsFlags, PFLAG_DUCKING);
		ClearBits(pev->flags, FL_DUCKING);
		ClearBits(pev->flags, FL_MONSTER); // VERIFY: the original clears 0x20 and 0x01 here, both
		ClearBits(pev->flags, FL_FLY);	   // already clear (pev->flags was just set)
		pev->deadflag = DEAD_RESPAWNABLE;
		pev->health = 0;

		// Clear out the status bar
		m_fInitHUD = true;

		pev->team = 0;
		MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_STRING("");
		MESSAGE_END();

		m_iObserver = 1;

		// Remove all the player's stuff
		RemoveAllItems(false);

		// Move them to the new position
		UTIL_SetOrigin(pev, vecPosition);

		// Find a player to watch
		pev->iuser1 = OBS_CHASE_FREE;
		Observer_FindNextPlayer(true);
		m_flNextObserverInput = 0;
	}
	else
	{
		m_afPhysicsFlags |= PFLAG_OBSERVER;
		pev->view_ofs = g_vecZero;
		pev->angles = pev->v_angle = vecViewAngle;
		pev->fixangle = 1;
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
		pev->movetype = MOVETYPE_NOCLIP;
		pev->modelindex = 0;
		UTIL_SetOrigin(pev, vecPosition);
	}
}

// orig 0xf3500. Oz: leaves the spectator mode; PostThink respawns the player on attack or use
void CBasePlayer::StopObserver()
{
	if (0 != pev->iuser1 || 0 != pev->iuser2)
	{
		m_iObserver = 0;
		pev->iuser1 = pev->iuser2 = 0;
		m_hObserverTarget = NULL;
		m_iHideHUD &= ~HIDEHUD_HEALTH;
		m_iHideHUD &= ~HIDEHUD_WEAPONS;
		m_fInitHUD = true;
		pev->button &= ~IN_ATTACK2;
		pev->nextthink = gpGlobals->time + 2;
	}
}

// orig 0xf35a0. SDK 2.2's: the next player who is not spectating and is drawn
void CBasePlayer::Observer_FindNextPlayer(bool bReverse)
{
	int iStart;
	if (m_hObserverTarget)
		iStart = ENTINDEX(m_hObserverTarget->edict());
	else
		iStart = ENTINDEX(edict());
	int iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = bReverse ? -1 : 1;

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		CBaseEntity* pEnt = UTIL_PlayerByIndex(iCurrent);
		if (!pEnt)
			continue;
		if (pEnt == this)
			continue;
		// Don't spec observers or invisible players
		if (((CBasePlayer*)pEnt)->IsObserver() || (pEnt->pev->effects & EF_NODRAW) != 0)
			continue;

		// MOD AUTHORS: Add checks on target here.

		m_hObserverTarget = pEnt;
		break;

	} while (iCurrent != iStart);

	// Did we find a target?
	if (m_hObserverTarget)
	{
		// Store the target in pev so the physics DLL can get to it
		pev->iuser2 = ENTINDEX(m_hObserverTarget->edict());
		// Move to the target
		UTIL_SetOrigin(pev, m_hObserverTarget->pev->origin);

		ALERT(at_console, "Now Tracking %s\n", STRING(m_hObserverTarget->pev->netname));
	}
	else
	{
		ALERT(at_console, "No observer targets.\n");
	}
}

// orig 0xf3780. Oz: jump toggles free roaming and, with oz_spectators 1, chasing; attack or use
// leaves the observer mode; attack2 steps to the next player
void CBasePlayer::Observer_HandleButtons()
{
	// Slow down mouse clicks
	if (m_flNextObserverInput > gpGlobals->time)
		return;

	// Jump changes from modes: Chase to Roaming
	if ((m_afButtonPressed & IN_JUMP) != 0)
	{
		if (pev->iuser1 == OBS_ROAMING && oz_spectators.value == 1)
			Observer_SetMode(OBS_CHASE_FREE);
		else
			Observer_SetMode(OBS_ROAMING);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack or use leaves the observer mode
	if ((pev->button & IN_ATTACK) != 0 || (m_afButtonPressed & IN_USE) != 0)
	{
		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
		pPlayer->StopObserver();
	}

	// Attack2 moves to the next player
	if ((m_afButtonPressed & IN_ATTACK2) != 0 && pev->iuser1 != OBS_ROAMING)
	{
		Observer_FindNextPlayer(true);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

// orig 0xf3890. SDK 2.2's: free roaming or free-look chase
void CBasePlayer::Observer_SetMode(int iMode)
{
	// Just abort if we're changing to the mode we're already in
	if (iMode == pev->iuser1)
		return;

	// Changing to Roaming?
	if (iMode == OBS_ROAMING)
	{
		// MOD AUTHORS: If you don't want to allow roaming observers at all in your mod, just abort here.
		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;

		ClientPrint(pev, HUD_PRINTCENTER, "Switched to Free Roaming mode");
		return;
	}

	// Changing to Chase Freelook?
	if (iMode == OBS_CHASE_FREE)
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if (m_hObserverTarget == NULL)
			Observer_FindNextPlayer(false);

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_FREE;
			pev->iuser2 = ENTINDEX(m_hObserverTarget->edict());
			ClientPrint(pev, HUD_PRINTCENTER, "Switched to FREELOOK chase mode");
			pev->maxspeed = 0;
		}
		else
		{
			ClientPrint(pev, HUD_PRINTCENTER, "No valid targets\nCan not switch to CHASE MODE");
			Observer_SetMode(OBS_ROAMING);
		}

		return;
	}
}

//
// PlayerUse - handles USE keypress
//
#define PLAYER_SEARCH_RADIUS (float)64

// orig 0xf39c0. SDK 2.3's. halflife-updated's observer early-out is out (OZ's observer never
// reaches here alive); its func_vehicle use stays.
void CBasePlayer::PlayerUse()
{
	// Was use pressed or released?
	if (((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE) == 0)
		return;

	// Hit Use on a train?
	if ((m_afButtonPressed & IN_USE) != 0)
	{
		if (m_pTank != NULL)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
			return;
		}
		else
		{
			if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);
				if (pTrain && (pTrain->Classify() == CLASS_VEHICLE))
					((CFuncVehicle*)pTrain)->m_pDriver = NULL;
				return;
			}
			else
			{ // Start controlling the train!
				CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);

				if (pTrain && (pev->button & IN_JUMP) == 0 && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) != 0 && pTrain->OnControls(pev))
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					if (pTrain->Classify() == CLASS_VEHICLE)
					{
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/vehicle_ignition.wav", 0.8, ATTN_NORM);
						((CFuncVehicle*)pTrain)->m_pDriver = this;
					}
					else
						EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
					return;
				}
			}
		}
	}

	CBaseEntity* pObject = NULL;
	CBaseEntity* pClosest = NULL;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors(pev->v_angle); // so we know which way we are facing

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != NULL)
	{

		if ((pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE)) != 0)
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin(pObject->pev) - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

			flDot = DotProduct(vecLOS, gpGlobals->v_forward);
			if (flDot > flMaxDot)
			{ // only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject)
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls
		int caps = pObject->ObjectCaps();

		if ((m_afButtonPressed & IN_USE) != 0)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if (((pev->button & IN_USE) != 0 && (caps & FCAP_CONTINUOUS_USE) != 0) ||
			((m_afButtonPressed & IN_USE) != 0 && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE)) != 0))
		{
			if ((caps & FCAP_CONTINUOUS_USE) != 0)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ((m_afButtonReleased & IN_USE) != 0 && (pObject->ObjectCaps() & FCAP_ONOFF_USE) != 0) // BUGBUG This is an "off" use
		{
			pObject->Use(this, this, USE_SET, 0);
		}
	}
	else
	{
		if ((m_afButtonPressed & IN_USE) != 0)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}



// orig 0xf3da0. SDK 2.3's; halflife-updated's func_vehicle momentum stays.
void CBasePlayer::Jump()
{
	Vector vecWallCheckDir; // direction we're tracing a line to find a wall when walljumping
	Vector vecAdjustedVelocity;
	Vector vecSpot;
	TraceResult tr;

	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;

	if (pev->waterlevel >= 2)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if (!FBitSet(m_afButtonPressed, IN_JUMP))
		return; // don't pogo stick

	if ((pev->flags & FL_ONGROUND) == 0 || !pev->groundentity)
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors(pev->angles);

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk

	SetAnimation(PLAYER_JUMP);

	if (m_fLongJump &&
		(pev->button & IN_DUCK) != 0 &&
		(pev->flDuckTime > 0) &&
		pev->velocity.Length() > 50)
	{
		SetAnimation(PLAYER_SUPERJUMP);
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t* pevGround = VARS(pev->groundentity);
	if (pevGround && (pevGround->flags & FL_CONVEYOR) != 0)
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}

	// JoshA: CS behaviour does this for tracktrain + train as well,
	// but let's just do this for func_vehicle to avoid breaking existing content.
	//
	// If you're standing on a moving train... then add the velocity of the train to yours.
	if (pevGround && (/*(!strcmp( "func_tracktrain", STRING(pevGround->classname))) ||
			(!strcmp( "func_train", STRING(pevGround->classname))) ) ||*/
			(!strcmp("func_vehicle", STRING(pevGround->classname)))))
		pev->velocity = pev->velocity + pevGround->velocity;
}



// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
// orig 0xf3ec0
void FixPlayerCrouchStuck(edict_t* pPlayer)
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for (int i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace);
		if (0 != trace.fStartSolid)
			pPlayer->v.origin.z++;
		else
			break;
	}
}

// orig 0xf3f20
void CBasePlayer::Duck()
{
	if ((pev->button & IN_DUCK) != 0)
	{
		if (m_IdealActivity != ACT_LEAP)
		{
			SetAnimation(PLAYER_WALK);
		}
	}
}

//
// ID's player as such.
//
// orig 0xf3f60
int CBasePlayer::Classify()
{
	return CLASS_PLAYER;
}


// orig 0xf3f70
void CBasePlayer::AddPoints(int score, bool bAllowNegativeScore)
{
	// Positive score always adds
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (pev->frags < 0) // Can't go more negative
				return;

			if (-score > pev->frags) // Will this go negative?
			{
				score = -pev->frags; // Sum will be 0
			}
		}
	}

	pev->frags += score;

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_SHORT(pev->frags);
	WRITE_SHORT(m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(g_pGameRules->GetTeamIndex(m_szTeamName) + 1);
	MESSAGE_END();
}


// orig 0xf40e0
void CBasePlayer::AddPointsToTeam(int score, bool bAllowNegativeScore)
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			{
				pPlayer->AddPoints(score, bAllowNegativeScore);
			}
		}
	}
}


#define CLIMB_SHAKE_FREQUENCY 22 // how many frames in between screen shakes when climbing
#define MAX_CLIMB_SPEED 200		 // fastest vertical climbing speed possible
#define CLIMB_SPEED_DEC 15		 // climbing deceleration rate
#define CLIMB_PUNCH_X -7		 // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z 7			 // how far to 'punch' client Z axis when climbing

// orig 0xf4190. Oz: a net client is checked for cheats (DetectCheats) every frame; in observer
// mode only the observer buttons run, before the suit; the train controls ignore a hanging
// player (1.7's IsHanging: no fallback trace for the train, no forward on it). The node graph
// pointers are set in Precache, as in the original. halflife-updated's view entity reset and
// func_vehicle driving stay; its spectator calls (Observer_CheckTarget/CheckProperties) are out.
void CBasePlayer::PreThink()
{
	int buttonsChanged = (m_afButtonLast ^ pev->button); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed = buttonsChanged & pev->button;	  // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button); // The ones not down are "released"

	g_pGameRules->PlayerThink(this);

	if (g_fGameOver)
		return; // intermission or finale

	// Oz cheat detection
	if (IsNetClient())
		DetectCheats();

	UTIL_MakeVectors(pev->v_angle); // is this still used?

	ItemPreFrame();
	WaterMove();

	if (g_pGameRules && g_pGameRules->FAllowFlashlight())
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	if (m_bResetViewEntity)
	{
		m_bResetViewEntity = false;

		CBaseEntity* viewEntity = m_hViewEntity;

		if (viewEntity)
		{
			SET_VIEW(edict(), viewEntity->edict());
		}
	}

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	// Oz observer mode
	if (0 != pev->iuser1)
	{
		Observer_HandleButtons();
		return;
	}

	CheckSuitUpdate();

	if (pev->deadflag >= DEAD_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
	{
		CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);
		float vel;

		// Oz Grapple -> !IsHanging
		if (!pTrain && !IsHanging)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, ENT(pev), &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = CBaseEntity::Instance(trainTrace.pHit);


			if (!pTrain || (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) == 0 || !pTrain->OnControls(pev))
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				// halflife-updated's func_vehicle release; its test lacked the NULL check
				if (pTrain && pTrain->Classify() == CLASS_VEHICLE)
					((CFuncVehicle*)pTrain)->m_pDriver = NULL;
				return;
			}
		}
		// VERIFY: hanging on the ground with no train under him, the original reads pTrain->pev
		// through NULL here (1.7's code too); kept
		else if (!FBitSet(pev->flags, FL_ONGROUND) || FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL) || ((pev->button & (IN_MOVELEFT | IN_MOVERIGHT)) != 0 && pTrain->Classify() != CLASS_VEHICLE))
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			// and it isn't a func_vehicle
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if (pTrain->Classify() == CLASS_VEHICLE)
		{
			if (pev->button & IN_FORWARD)
			{
				vel = 1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			if (pev->button & IN_BACK)
			{
				vel = -1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			if (pev->button & IN_MOVELEFT)
			{
				vel = 20;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			if (pev->button & IN_MOVERIGHT)
			{
				vel = 30;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
		}
		else
		{
			// Oz Grapple -> !IsHanging (3.02 tests it for forward only, 1.7 for both)
			if ((m_afButtonPressed & IN_FORWARD) != 0 && !IsHanging)
			{
				vel = 1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
			else if ((m_afButtonPressed & IN_BACK) != 0)
			{
				vel = -1;
				pTrain->Use(this, this, USE_SET, (float)vel);
			}
		}

		if (0 != vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if ((m_iTrain & TRAIN_ACTIVE) != 0)
		m_iTrain = TRAIN_NEW; // turn off train

	if ((pev->button & IN_JUMP) != 0)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}


	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) != 0 || FBitSet(pev->flags, FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) != 0)
		Duck();

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if ((m_afPhysicsFlags & PFLAG_ONBARNACLE) != 0)
	{
		pev->velocity = g_vecZero;
	}
}
/* Time based Damage works as follows: 
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time 
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.  
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
		
	
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */


// orig 0xf4600. SDK 2.3's; halflife-updated's fabs stays (fix, e19fe47)
void CBasePlayer::CheckTimeBasedDamage()
{
	int i;
	byte bDuration = 0;

	static float gtbdPrev = 0.0;

	if ((m_bitsDamageType & DMG_TIMEBASED) == 0)
		return;

	// only check for time based damage approx. every 2 seconds
	if (fabs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;

	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if ((m_bitsDamageType & (DMG_PARALYZE << i)) != 0)
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = V_min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4; // get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (0 != m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison) && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (0 != m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", false, SUIT_REPEAT_OK);
					}
				}


				// decrement damage duration, detect when done.
				if (0 == m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation. 
Some functions are automatic, some require power. 
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit 
		will come on and the battery will drain while the player stays in the area. 
		After the battery is dead, the player starts to take damage. 
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge. 

Notification (via the HUD)

x	Health
x	Ammo  
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged. 
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD 
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor. 

Augmentation 

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds. 
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA	
		Used automatically after picked up and after player enters the water. 
		Works for N seconds. Single use.	
	
Things powered by the battery

	Armor		
		Uses N watts for every M units of damage.
	Heat/Cool	
		Uses N watts for every second in hot/cold area.
	Long Jump	
		Uses N watts for every jump.
	Alien Cloak	
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield	
		Augments armor. Reduces Armor drain by one half
 
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

// orig 0xf4820
void CBasePlayer::UpdateGeigerCounter()
{
	byte range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client

	range = (byte)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, NULL, pev);
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME 3.5
#define SUITFIRSTUPDATETIME 0.1

// orig 0xf4920
void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (g_pGameRules->IsMultiplayer())
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
		{
			isentence = m_rgSuitPlayList[isearch];
			if (0 != isentence)
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (0 != isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[CBSENTENCENAME_MAX + 1];
				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check
			m_flSuitUpdate = 0;
	}
}

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

// orig 0xf4a70
void CBasePlayer::SetSuitUpdate(const char* name, bool fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;


	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (0 == m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (0 != iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT - 1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
	}
}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
// orig 0xf4c90
void CheckPowerups(entvars_t* pev)
{
	if (pev->health <= 0)
		return;

	pev->modelindex = g_ulModelIndexPlayer; // don't use eyes
}


//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
// orig 0xf4cd0. SDK 2.3's (no UTIL_MakeVectors, as decided for mod-rockcrowbar)
void CBasePlayer::UpdatePlayerSound()
{
	int iBodyVolume;
	int iVolume;
	CSound* pSound;

	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (!pSound)
	{
		ALERT(at_console, "Client lost reserved sound!\n");
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		iBodyVolume = pev->velocity.Length();

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast.
		if (iBodyVolume > 512)
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ((pev->button & IN_JUMP) != 0)
	{
		iBodyVolume += 100;
	}

	// convert player move speed and actions into sound audible by monsters.
	if (m_iWeaponVolume > iBodyVolume)
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player.
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if (m_iWeaponVolume < 0)
	{
		iVolume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if (m_iTargetVolume > iVolume)
	{
		iVolume = m_iTargetVolume;
	}
	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
		{
			iVolume = 0;
		}
	}

	if (m_fNoPlayerSound)
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if (gpGlobals->time > m_flStopExtraSoundTime)
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= (bits_SOUND_PLAYER | m_iExtraSoundTypes);
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors ( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the
	// player is making.
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}


// orig 0xf4f00. Oz: with oz_fallingdamage below 1 a hard landing deals 0 (TakeDamage still
// runs); a player hidden by PlayerSpawn's first-join observer (EF_NODRAW) spawns for real on
// attack or use; Identify every other frame of even seconds while oz_playerid is on (one static
// toggle shared by all players); the regeneration rune's tick every 30 frames; the cloaking
// rune's shimmer. halflife-updated's fix of setting m_afButtonLast on every path stays, with the
// original's spot for it on the normal path (the respawn above reads the button first).
void CBasePlayer::PostThink()
{
	static bool donethis = false; // orig .data 0x166234 (donethis.1288)

	if (g_fGameOver)
	{
		m_afButtonLast = pev->button; // halflife-updated's fix, kept: the early exits set it too
		goto pt_end; // intermission or finale
	}

	if (!IsAlive())
	{
		m_afButtonLast = pev->button; // halflife-updated's fix, kept: the early exits set it too
		goto pt_end;
	}

	// Handle Tank controlling
	if (m_pTank != NULL)
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if (m_pTank->OnControls(pev) && 0 == pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{ // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
	}

	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > pev->health)
			{ //splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if (flFallDamage > 0)
			{
				// Oz: falling damage only with oz_fallingdamage
				if (oz_fallingdamage.value >= 1)
					TakeDamage(CWorld::World->pev, CWorld::World->pev, flFallDamage, DMG_FALL);
				else
					TakeDamage(CWorld::World->pev, CWorld::World->pev, 0, DMG_FALL);
				pev->punchangle.x = 0;
			}
		}

		if (IsAlive())
		{
			SetAnimation(PLAYER_WALK);
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
		{
			CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2);
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character
	if (IsAlive())
	{
		if (0 == pev->velocity.x && 0 == pev->velocity.y)
			SetAnimation(PLAYER_IDLE);
		else if ((0 != pev->velocity.x || 0 != pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation(PLAYER_WALK);
		else if (pev->waterlevel > 1)
			SetAnimation(PLAYER_WALK);
	}

	StudioFrameAdvance();
	CheckPowerups(pev);

	UpdatePlayerSound();

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

	// Oz: out of the first-join observer
	if (((pev->button & IN_ATTACK) != 0 || (pev->button & IN_USE) != 0) && pev->effects == EF_NODRAW)
	{
		pev->button = 0;
		m_flRespawnTimer = 0.0f;
		pev->effects &= ~EF_NODRAW;
		pev->takedamage = DAMAGE_YES;
		pev->flags &= ~FL_SPECTATOR;
		respawn(pev, (m_afPhysicsFlags & PFLAG_OBSERVER) == 0);
		pev->movetype = MOVETYPE_WALK;
		pev->flags = FL_CLIENT;
		m_fNextSuicideTime = gpGlobals->time + 1;
	}

	// Oz player id
	if (0 == ((int)gpGlobals->time & 1) && !donethis && oz_playerid.value > 0)
	{
		donethis = true;
		Identify();
	}
	else
		donethis = false;

	// Oz runes - regeneration
	if (m_iRune == 2) // Regeneration Rune
	{
		if (pev->health < oz_maxhealth.value || pev->armorvalue < oz_maxcharge.value)
		{
			regencounter++;

			if (regencounter == 30)
			{
				regencounter = 0;

				pev->health += oz_regenrunehealth.value;
				if (pev->health > oz_maxhealth.value)
					pev->health = oz_maxhealth.value;

				pev->armorvalue += oz_regenrunearmor.value;
				if (pev->armorvalue > oz_maxcharge.value)
					pev->armorvalue = oz_maxcharge.value;
			}
		}
	}

	// Oz runes - cloaking: firing shows the player for 10 seconds, then the shimmer follows speed
	if (m_iRune == 5) // Cloaking Rune
	{
		if ((pev->button & (IN_ATTACK | IN_ATTACK2)) != 0)
		{
			pev->rendermode = kRenderTransTexture;
			pev->renderamt = 6;
			pev->renderfx |= kRenderFxGlowShell; // VERIFY: OR'd in, as the original
			pev->rendercolor = Vector(50, 50, 50);
			pev->renderamt = 128;
			m_flCloakTime = gpGlobals->time + 10;
		}

		if (m_flCloakTime <= gpGlobals->time)
		{
			float flAmt = pev->velocity.Length() * 0.5;
			if (flAmt > 50)
				flAmt = 50;
			if (flAmt < 0)
				flAmt = 0;

			pev->rendermode = kRenderTransTexture;
			pev->renderamt = 6;
			pev->renderfx |= kRenderFxGlowShell;
			pev->rendercolor = Vector(50, 50, 50);
			pev->renderamt = flAmt;
		}
	}

pt_end:
#if defined(CLIENT_WEAPONS)
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			CBasePlayerItem* pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				CBasePlayerWeapon* gun = pPlayerItem->GetWeaponPtr();

				if (gun && gun->UseDecrement())
				{
					gun->m_flNextPrimaryAttack = V_max(gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0f);
					gun->m_flNextSecondaryAttack = V_max(gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001f);

					if (gun->m_flTimeWeaponIdle != 1000)
					{
						gun->m_flTimeWeaponIdle = V_max(gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001f);
					}

					if (gun->pev->fuser1 != 1000)
					{
						gun->pev->fuser1 = V_max(gun->pev->fuser1 - gpGlobals->frametime, -0.001f);
					}

					gun->DecrementTimers();

					// Only decrement if not flagged as NO_DECREMENT
					//					if ( gun->m_flPumpTime != 1000 )
					//	{
					//		gun->m_flPumpTime	= V_max( gun->m_flPumpTime - gpGlobals->frametime, -0.001 );
					//	}
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if (m_flNextAttack < -0.001)
		m_flNextAttack = -0.001;

	if (m_flNextAmmoBurn != 1000)
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;

		if (m_flNextAmmoBurn < -0.001)
			m_flNextAmmoBurn = -0.001;
	}

	if (m_flAmmoStartCharge != 1000)
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;

		if (m_flAmmoStartCharge < -0.001)
			m_flAmmoStartCharge = -0.001;
	}
#endif
}


// checks if the spot is clear of players
// orig 0xf5950
bool IsSpawnPointValid(CBaseEntity* pPlayer, CBaseEntity* pSpot)
{
	CBaseEntity* ent = NULL;

	if (!pSpot->IsTriggered(pPlayer))
	{
		return false;
	}

	while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
	{
		// if ent is a client, don't spawn on 'em
		if (ent->IsPlayer() && ent != pPlayer)
			return false;
	}

	return true;
}

/*
============
EntSelectSpawnPoint

Returns the entity to spawn at

USES AND SETS GLOBAL g_pLastSpawn
============
*/
// orig 0xf59d0. SDK 2.3's (1-5 random steps through the deathmatch spots).
edict_t* EntSelectSpawnPoint(CBaseEntity* pPlayer)
{
	CBaseEntity* pSpot;
	edict_t* player;

	player = pPlayer->edict();

	// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_coop");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else if (g_pGameRules->IsDeathmatch())
	{
		// The original's 1-5 steps; halflife-updated's count over the spawn spots (HL25 spawn
		// selection backport b61c0be) is a design change, left out
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for (int i = RANDOM_LONG(1, 5); i > 0; i--)
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		if (FNullEnt(pSpot)) // skip over the null point
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");

		CBaseEntity* pFirstSpot = pSpot;

		do
		{
			if (pSpot)
			{
				// check if pSpot is valid
				if (IsSpawnPointValid(pPlayer, pSpot))
				{
					if (pSpot->pev->origin == Vector(0, 0, 0))
					{
						pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
						continue;
					}

					// if so, go to pSpot
					goto ReturnSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		} while (pSpot != pFirstSpot); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if (!FNullEnt(pSpot))
		{
			CBaseEntity* ent = NULL;
			while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
			{
				// if ent is a client, kill em (unless they are ourselves)
				if (ent->IsPlayer() && !(ent->edict() == player))
					ent->TakeDamage(CWorld::World->pev, CWorld::World->pev, 300, DMG_GENERIC);
			}
			goto ReturnSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if (FStringNull(gpGlobals->startspot) || 0 == strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(NULL, "info_player_start");
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname(NULL, STRING(gpGlobals->startspot));
		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}

ReturnSpot:
	if (FNullEnt(pSpot))
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return CWorld::World->edict();
	}

	g_pLastSpawn = pSpot;
	return pSpot->edict();
}

// orig 0xf5e70. Oz: health, armor, maximum health, air and speed from the oz_* cvars (bots:
// bot_speed, and bot_blood picks their blood color); the HUD message counters; after PlayerSpawn
// the rune is cleared, spawn protection starts (godmode and a red glow shell, cleared by
// client.cpp's PlayerPostThink), and the first player in places every rune (g_runes_exist).
// halflife-updated's gauss charge start, "bj" key, room type and chat time stay.
void CBasePlayer::Spawn()
{
	wassuicide = 0;

	m_flStartCharge = gpGlobals->time; // halflife-updated's fix, kept: no stale gauss charge
	m_bIsSpawning = true;

	//Make sure this gets reset even if somebody adds an early return or throws an exception.
	const CallOnDestroy resetIsSpawning{[this]()
		{
			//Done spawning; reset.
			m_bIsSpawning = false;
		}};

	pev->classname = MAKE_STRING("player");
	pev->health = oz_starthealth.value;
	pev->armorvalue = oz_startcharge.value;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = oz_maxhealth.value;
	pev->air_finished = gpGlobals->time + oz_playerair.value;
	pev->flags &= FL_PROXY | FL_FAKECLIENT; // keep proxy and fakeclient flags set by engine
	pev->flags |= FL_CLIENT;
	pev->dmg = 2; // initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 1.0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	m_fLongJump = false; // no longjump module.

	// Oz messaging
	if (oz_picture.value == 1)
	{
		m_iOz = 26;
		m_iStart = 26;
		m_iCommands = 26;
	}
	else
	{
		m_iOz = 0;
		m_iStart = 26;
		m_iCommands = 26;
		m_iNameCheckTimer = 0;
	}

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "bj", UTIL_dtos1(sv_allowbunnyhopping.value != 0 ? 1 : 0));

	m_iFOV = 0;		   // init field of view.
	pev->fov = 0;
	m_iClientFOV = -1; // make sure fov reset is sent
	m_ClientSndRoomtype = -1;

	m_flNextDecalTime = 0; // let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0; // wait a few seconds until user-defined message registrations
											 // are recieved by all clients

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5; // some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;

	// Oz: bot blood colors (bot_blood 0: one of 244/209/195, 1: 244, 2: 209, 3: red, else 195)
	const int iBotBlood = (int)bot_blood.value;
	if (IsBot())
	{
		const int iRandom = RANDOM_LONG(1, 3);
		switch (iBotBlood)
		{
		case 0:
			if (iRandom == 1)
				m_bloodColor = 244;
			else if (iRandom == 2)
				m_bloodColor = 209;
			else
				m_bloodColor = 195;
			break;
		case 1:
			m_bloodColor = 244;
			break;
		case 2:
			m_bloodColor = 209;
			break;
		case 3:
			m_bloodColor = BLOOD_COLOR_RED;
			break;
		default:
			m_bloodColor = 195;
			break;
		}
	}
	else
		m_bloodColor = BLOOD_COLOR_RED;

	m_flNextAttack = UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam(this);
	g_pGameRules->GetPlayerSpawnSpot(this);

	SET_MODEL(ENT(pev), "models/player.mdl");
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence = LookupActivity(ACT_IDLE);

	if (FBitSet(pev->flags, FL_DUCKING))
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector(0, 32, 0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
	{
		ALERT(at_console, "Couldn't alloc player sound slot!\n");
	}

	m_fNoPlayerSound = false; // normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = true;
	m_iClientHideHUD = -1; // force this to be recalculated
	m_fWeapon = false;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;

	// reset all ammo values to 0
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0; // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;

	m_flNextChatTime = gpGlobals->time;

	g_pGameRules->PlayerSpawn(this);

	// Oz runes, spawn protection
	m_iRune = 0;
	pev->rendermode = kRenderNormal;
	m_iSpawnProtect = 0;
	m_iTeleportTime = 0;
	pev->flags |= FL_GODMODE;
	pev->renderfx |= kRenderFxGlowShell;
	pev->rendercolor = Vector(150, 0, 0);
	pev->gravity = 1;

	if (IsNetClient())
		pev->maxspeed = oz_playerspeed.value;
	else
		pev->maxspeed = bot_speed.value;

	didhaveexploderune = 0;

	// Oz runes - spawning when first player enters
	if (!g_runes_exist)
	{
		g_runes_exist = true;

		for (int i = 0; i < oz_fragrunenumber.value; i++)
			SpawnRunes(edict(), 1, 1);
		for (int i = 0; i < oz_regenrunenumber.value; i++)
			SpawnRunes(edict(), 2, 1);
		for (int i = 0; i < oz_crowbarrunenumber.value; i++)
			SpawnRunes(edict(), 3, 1);
		for (int i = 0; i < oz_hasterunenumber.value; i++)
			SpawnRunes(edict(), 4, 1);
		for (int i = 0; i < oz_cloakrunenumber.value; i++)
			SpawnRunes(edict(), 5, 1);
		for (int i = 0; i < oz_hookrunenumber.value; i++)
			SpawnRunes(edict(), 6, 1);
		for (int i = 0; i < oz_protectrunenumber.value; i++)
			SpawnRunes(edict(), 7, 1);
		for (int i = 0; i < oz_lowgravrunenumber.value; i++)
			SpawnRunes(edict(), 8, 1);
		for (int i = 0; i < oz_vamprunenumber.value; i++)
			SpawnRunes(edict(), 9, 1);
		for (int i = 0; i < oz_traprunenumber.value; i++)
			SpawnRunes(edict(), 10, 1);
		for (int i = 0; i < oz_superglockrunenumber.value; i++)
			SpawnRunes(edict(), 11, 1);
		for (int i = 0; i < oz_randomrunenumber.value; i++)
			SpawnRunes(edict(), 15, 1);
		for (int i = 0; i < oz_superspeedrunenumber.value; i++)
			SpawnRunes(edict(), 12, 1);
		for (int i = 0; i < oz_identifyrunenumber.value; i++)
			SpawnRunes(edict(), 13, 1);
		for (int i = 0; i < oz_teleportrunenumber.value; i++)
			SpawnRunes(edict(), 14, 1);
	}
}


// orig 0xf6880. SDK 2.3's, with the node graph pointers set here (not in PreThink)
void CBasePlayer::Precache()
{
	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.

	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if (0 != WorldGraph.m_fGraphPresent && 0 == WorldGraph.m_fGraphPointersSet)
	{
		if (!WorldGraph.FSetGraphPointers())
		{
			ALERT(at_console, "**Graph pointers were not set!\n");
		}
		else
		{
			ALERT(at_console, "**Graph Pointers Set!\n");
		}
	}

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_iTrain |= TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5; // won't update for 1/2 a second

	if (gInitHUD)
		m_fInitHUD = true;
}


// orig 0xf6950
bool CBasePlayer::Save(CSave& save)
{
	if (!CBaseMonster::Save(save))
		return false;

	return save.WriteFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));
}


//
// Marks everything as new so the player will resend this to the hud.
//
// orig 0xf69a0
void CBasePlayer::RenewItems()
{
}


// orig 0xf69b0. SDK 2.3's; halflife-updated's FOV/room type resets, TabulateAmmo and view entity reset stay (single player only)
bool CBasePlayer::Restore(CRestore& restore)
{
	if (!CBaseMonster::Restore(restore))
		return false;

	bool status = restore.ReadFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));

	SAVERESTOREDATA* pSaveData = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	// landmark isn't present.
	if (0 == pSaveData->fUseLandmark)
	{
		ALERT(at_console, "No Landmark:%s\n", pSaveData->szLandmarkName);

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint(this);
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
	pev->v_angle.z = 0; // Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = 1; // turn this way immediately

	m_iClientFOV = -1; // Make sure the client gets the right FOV value.
	m_ClientSndRoomtype = -1;

	// Reset room type on level change.
	if (!FStrEq(restore.GetData().szCurrentMapName, STRING(gpGlobals->mapname)))
	{
		m_SndRoomtype = 0;
	}

	// Copied from spawn() for now
	m_bloodColor = BLOOD_COLOR_RED;

	g_ulModelIndexPlayer = pev->modelindex;

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	if (m_fLongJump)
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "1");
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	}

	RenewItems();

	TabulateAmmo();

#if defined(CLIENT_WEAPONS)
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	m_bResetViewEntity = true;

	m_bRestored = true;

	return status;
}



// orig 0xf6c20
void CBasePlayer::SelectNextItem(int iItem)
{
	CBasePlayerItem* pItem;

	pItem = m_rgpPlayerItems[iItem];

	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext;
		if (!pItem)
		{
			return;
		}

		CBasePlayerItem* pLast;
		pLast = pItem;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[iItem] = pItem;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();
	}
}

// orig 0xf6d00. SDK 2.3's; halflife-updated's m_ForceSendAnimations stays (fix, #154)
void CBasePlayer::SelectItem(const char* pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem* pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];

			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
					break;
				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;


	if (pItem == m_pActiveItem)
		return;

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->m_ForceSendAnimations = true;
		m_pActiveItem->Deploy();
		m_pActiveItem->m_ForceSendAnimations = false;
		m_pActiveItem->UpdateItemInfo();
	}
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
// orig 0xf6eb0
bool CBasePlayer::HasWeapons()
{
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			return true;
		}
	}

	return false;
}

// orig 0xf6ee0
void CBasePlayer::SelectPrevItem(int iItem)
{
}


// orig 0xf6ef0
const char* CBasePlayer::TeamID()
{
	if (pev == NULL) // Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn(entvars_t* pevOwner);
	void Think() override;

	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

// orig 0xf6f20
void CSprayCan::Spawn(entvars_t* pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

// orig 0xf7000
void CSprayCan::Think()
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer* pPlayer;

	pPlayer = (CBasePlayer*)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);

	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, true);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn(entvars_t* pevOwner);
	void Spray();
};

// orig 0xf7190
void CBloodSplat::Spawn(entvars_t* pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink(&CBloodSplat::Spray);
	pev->nextthink = gpGlobals->time + 0.1;
}

// orig 0xf7260
void CBloodSplat::Spray()
{
	TraceResult tr;

	if (g_Language != LANGUAGE_GERMAN)
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}
	SetThink(&CBloodSplat::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================

static edict_t* GiveNamedItem_Common(entvars_t* pev, const char* pszName)
{
	int istr = MAKE_STRING(pszName);

	edict_t* pent = CREATE_NAMED_ENTITY(istr);
	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in GiveNamedItem!\n");
		return nullptr;
	}
	VARS(pent)->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn(pent);

	return pent;
}

// orig 0xf7340
void CBasePlayer::GiveNamedItem(const char* szName)
{
	auto pent = GiveNamedItem_Common(pev, szName);

	if (!pent)
	{
		return;
	}

	DispatchTouch(pent, ENT(pev));
}

void CBasePlayer::GiveNamedItem(const char* szName, int defaultAmmo)
{
	auto pent = GiveNamedItem_Common(pev, szName);

	if (!pent)
	{
		return;
	}

	auto entity = CBaseEntity::Instance(pent);

	if (!entity)
	{
		return;
	}

	if (auto weapon = dynamic_cast<CBasePlayerWeapon*>(entity); weapon)
	{
		weapon->m_iDefaultAmmo = defaultAmmo;
	}

	DispatchTouch(pent, ENT(pev));
}

// orig 0xf7430. SDK 2.3's (halflife-updated's copy is UTIL_FindEntityForward in util.cpp)
CBaseEntity* FindEntityForward(CBaseEntity* pMe)
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs, pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, pMe->edict(), &tr);
	if (tr.flFraction != 1.0 && !FNullEnt(tr.pHit))
	{
		CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
		return pHit;
	}
	return NULL;
}

// orig 0xf7590
bool CBasePlayer::FlashlightIsOn()
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}


void CBasePlayer::FlashlightTurnOn()
{
	if (!g_pGameRules->FAllowFlashlight())
	{
		return;
	}

	if (HasSuit())
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM);
		SetBits(pev->effects, EF_DIMLIGHT);
		MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, NULL, pev);
		WRITE_BYTE(1);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
	}
}


// orig 0xf76a0
void CBasePlayer::FlashlightTurnOff()
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM);
	ClearBits(pev->effects, EF_DIMLIGHT);
	MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
// orig 0xf7770. SDK 2.3's; halflife-updated's extra resends (hide HUD, FOV, weapon bits, room type, ammo) stay (fix: a demo or fullupdate gets the whole HUD state)
void CBasePlayer::ForceClientDllUpdate()
{
	m_iClientHealth = -1;
	m_iClientBattery = -1;
	m_iClientHideHUD = -1;
	m_iClientFOV = -1;
	m_ClientWeaponBits = 0;
	m_ClientSndRoomtype = -1;

	for (int i = 0; i < MAX_AMMO_SLOTS; ++i)
	{
		m_rgAmmoLast[i] = 0;
	}

	m_iTrain |= TRAIN_NEW; // Force new train message.
	m_fWeapon = false;	   // Force weapon send
	m_fKnownItem = false;  // Force weaponinit messages.
	m_fInitHUD = true;	   // Force HUD gmsgResetHUD message

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
// orig 0xf77d0. SDK 2.1's set: impulse 204 (ForceClientDllUpdate) is back (halflife-updated
// dropped it: design, as mod-rockcrowbar).
void CBasePlayer::ImpulseCommands()
{
	TraceResult tr; // UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();

	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
	case 99:
	{

		bool iOn;

		if (0 == gmsgLogo)
		{
			iOn = true;
			gmsgLogo = REG_USER_MSG("Logo", 1);
		}
		else
		{
			iOn = false;
		}

		ASSERT(gmsgLogo > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgLogo, NULL, pev);
		WRITE_BYTE(static_cast<int>(iOn));
		MESSAGE_END();

		if (!iOn)
			gmsgLogo = 0;
		break;
	}
	case 100:
		// temporary flashlight for level designers
		if (FlashlightIsOn())
		{
			FlashlightTurnOff();
		}
		else
		{
			FlashlightTurnOn();
		}
		break;

	case 204: // Demo recording, update client dll specific data again.
		ForceClientDllUpdate();
		break;

	case 201: // paint decal

		if (gpGlobals->time < m_flNextDecalTime)
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan* pCan = GetClassPtr((CSprayCan*)NULL);
			pCan->Spawn(pev);
		}

		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands(iImpulse);
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
// orig 0xf7a90. SDK 2.3's (FindEntityForward, the plain texture line: halflife-updated's
// texture type print is out). halflife-updated's live sv_cheats test stays, as mod-rc2.
void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
	if (0 == g_psv_cheats->value)
	{
		return;
	}

	CBaseEntity* pEntity;
	TraceResult tr;

	switch (iImpulse)
	{
	case 76:
	{
		if (!giPrecacheGrunt)
		{
			giPrecacheGrunt = true;
			ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
		}
		else
		{
			UTIL_MakeVectors(Vector(0, pev->v_angle.y, 0));
			Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
		}
		break;
	}


	case 101:
		gEvilImpulse101 = true;
		GiveNamedItem("item_suit");
		GiveNamedItem("item_battery");
		GiveNamedItem("weapon_crowbar");
		GiveNamedItem("weapon_9mmhandgun");
		GiveNamedItem("ammo_9mmclip");
		GiveNamedItem("weapon_shotgun");
		GiveNamedItem("ammo_buckshot");
		GiveNamedItem("weapon_9mmAR");
		GiveNamedItem("ammo_9mmAR");
		GiveNamedItem("ammo_ARgrenades");
		GiveNamedItem("weapon_handgrenade");
		GiveNamedItem("weapon_tripmine");
		GiveNamedItem("weapon_357");
		GiveNamedItem("ammo_357");
		GiveNamedItem("weapon_crossbow");
		GiveNamedItem("ammo_crossbow");
		GiveNamedItem("weapon_egon");
		GiveNamedItem("weapon_gauss");
		GiveNamedItem("ammo_gaussclip");
		GiveNamedItem("weapon_rpg");
		GiveNamedItem("ammo_rpgclip");
		GiveNamedItem("weapon_satchel");
		GiveNamedItem("weapon_snark");
		GiveNamedItem("weapon_hornetgun");

		gEvilImpulse101 = false;
		break;

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs(pev, 1, true);
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 105: // player makes no sound for monsters to hear.
	{
		if (m_fNoPlayerSound)
		{
			ALERT(at_console, "Player is audible\n");
			m_fNoPlayerSound = false;
		}
		else
		{
			ALERT(at_console, "Player is silent\n");
			m_fNoPlayerSound = true;
		}
		break;
	}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			ALERT(at_console, "Classname: %s", STRING(pEntity->pev->classname));

			if (!FStringNull(pEntity->pev->targetname))
			{
				ALERT(at_console, " - Targetname: %s\n", STRING(pEntity->pev->targetname));
			}
			else
			{
				ALERT(at_console, " - TargetName: No Targetname\n");
			}

			ALERT(at_console, "Model: %s\n", STRING(pEntity->pev->model));
			if (!FStringNull(pEntity->pev->globalname))
				ALERT(at_console, "Globalname: %s\n", STRING(pEntity->pev->globalname));
		}
		break;

	case 107:
	{
		TraceResult tr;

		edict_t* pWorld = CWorld::World->edict();

		Vector start = pev->origin + pev->view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
		if (tr.pHit)
			pWorld = tr.pHit;
		const char* pTextureName = TRACE_TEXTURE(pWorld, start, end);
		if (pTextureName)
			ALERT(at_console, "Texture: %s\n", pTextureName);
	}
	break;
	case 195: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_fly", pev->origin, pev->angles);
	}
	break;
	case 196: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_large", pev->origin, pev->angles);
	}
	break;
	case 197: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_human", pev->origin, pev->angles);
	}
	break;
	case 199: // show nearest node and all connections
	{
		ALERT(at_console, "%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
		WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
	}
	break;
	case 202: // Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			CBloodSplat* pBlood = GetClassPtr((CBloodSplat*)NULL);
			pBlood->Spawn(pev);
		}
		break;
	case 203: // remove creature.
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			if (0 != pEntity->pev->takedamage)
				pEntity->SetThink(&CBaseEntity::SUB_Remove);
		}
		break;
	}
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
// orig 0xf84a0. SDK 2.3's. halflife-updated's fix, kept (as mod-rc2): the weapon goes in the
// list before its ammo is extracted, and the counts go out at once (#153, #157). WeapPickup stays
// the original's: the weapons' own AddToPlayer sends it (halflife-updated's send here is out).
bool CBasePlayer::AddPlayerItem(CBasePlayerItem* pItem)
{
	CBasePlayerItem* pInsert;

	pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while (pInsert)
	{
		if (FClassnameIs(pInsert->pev, STRING(pItem->pev->classname)))
		{
			if (pItem->AddDuplicate(pInsert))
			{
				g_pGameRules->PlayerGotWeapon(this, pItem);
				pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo();
				if (m_pActiveItem)
					m_pActiveItem->UpdateItemInfo();

				pItem->Kill();
			}
			else if (gEvilImpulse101)
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill();
			}
			return false;
		}
		pInsert = pInsert->m_pNext;
	}


	if (pItem->CanAddToPlayer(this))
	{
		pItem->AddToPlayer(this);

		g_pGameRules->PlayerGotWeapon(this, pItem);
		pItem->CheckRespawn();

		// Add to the list before extracting ammo so weapon ownership checks work properly.
		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		if (auto weapon = pItem->GetWeaponPtr(); weapon)
		{
			weapon->ExtractAmmo(weapon);

			//Immediately update the ammo HUD so weapon pickup isn't sometimes red because the HUD doesn't know about regenerating/free ammo yet.
			if (-1 != weapon->m_iPrimaryAmmoType)
			{
				SendSingleAmmoUpdate(CBasePlayer::GetAmmoIndex(weapon->pszAmmo1()));
			}

			if (-1 != weapon->m_iSecondaryAmmoType)
			{
				SendSingleAmmoUpdate(CBasePlayer::GetAmmoIndex(weapon->pszAmmo2()));
			}
		}

		// should we switch to this item?
		if (g_pGameRules->FShouldSwitchWeapon(this, pItem))
		{
			SwitchWeapon(pItem);
		}

		return true;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill();
	}
	return false;
}



// orig 0xf8670
bool CBasePlayer::RemovePlayerItem(CBasePlayerItem* pItem)
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim();
		pItem->Holster();
		pItem->pev->nextthink = 0; // crowbar may be trying to swing again, etc.
		pItem->SetThink(NULL);
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	else if (m_pLastItem == pItem) // VERIFY: the original's else (an item both active and last stays last)
		m_pLastItem = NULL;

	CBasePlayerItem* pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return true;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != pItem)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = pItem->m_pNext;
			return true;
		}
	}
	return false;
}


//
// Returns the unique ID for the ammo, or -1 if error
//
// orig 0xf8780. SDK 2.3's; halflife-updated's exhaustible-weapon give stays (fix, 6951bf1)
int CBasePlayer::GiveAmmo(int iCount, const char* szName, int iMax)
{
	if (!szName)
	{
		// no ammo.
		return -1;
	}

	if (!g_pGameRules->CanHaveAmmo(this, szName, iMax))
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = 0;

	i = GetAmmoIndex(szName);

	if (i < 0 || i >= MAX_AMMO_SLOTS)
		return -1;

	int iAdd = V_min(iCount, iMax - m_rgAmmo[i]);
	if (iAdd < 1)
		return i;

	// If this is an exhaustible weapon make sure the player has it.
	if (const auto& ammoType = CBasePlayerItem::AmmoInfoArray[i]; ammoType.WeaponName != nullptr)
	{
		if (!HasNamedPlayerItem(ammoType.WeaponName))
		{
			GiveNamedItem(ammoType.WeaponName, 0);
		}
	}

	m_rgAmmo[i] += iAdd;


	if (0 != gmsgAmmoPickup) // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, NULL, pev);
		WRITE_BYTE(GetAmmoIndex(szName)); // ammo ID
		WRITE_BYTE(iAdd);				  // amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}


/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
// orig 0xf88a0
void CBasePlayer::ItemPreFrame()
{
#if defined(CLIENT_WEAPONS)
	if (m_flNextAttack > 0)
#else
	if (gpGlobals->time < m_flNextAttack)
#endif
	{
		return;
	}

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPreFrame();
}


/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
// orig 0xf88f0. SDK 2.3's; halflife-updated's return when PlayerUse just took a tank stays (fix, 4fe7e6e)
void CBasePlayer::ItemPostFrame()
{
	// check if the player is using a tank
	if (m_pTank != NULL)
		return;

#if defined(CLIENT_WEAPONS)
	if (m_flNextAttack > 0)
#else
	if (gpGlobals->time < m_flNextAttack)
#endif
	{
		return;
	}

	ImpulseCommands();

	// check again if the player is using a tank if they started using it in PlayerUse
	if (m_pTank != NULL)
		return;

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPostFrame();
}

// orig 0xf8960
int CBasePlayer::AmmoInventory(int iAmmoIndex)
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

// orig 0xf8990
int CBasePlayer::GetAmmoIndex(const char* psz)
{
	int i;

	if (!psz)
		return -1;

	for (i = 1; i < MAX_AMMO_SLOTS; i++)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if (stricmp(psz, CBasePlayerItem::AmmoInfoArray[i].pszName) == 0)
			return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
// orig 0xf8a10
void CBasePlayer::SendAmmoUpdate()
{
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		InternalSendSingleAmmoUpdate(i);
	}
}

void CBasePlayer::SendSingleAmmoUpdate(int ammoIndex)
{
	if (ammoIndex < 0 || ammoIndex >= MAX_AMMO_SLOTS)
	{
		return;
	}

	InternalSendSingleAmmoUpdate(ammoIndex);
}

void CBasePlayer::InternalSendSingleAmmoUpdate(int ammoIndex)
{
	if (m_rgAmmo[ammoIndex] != m_rgAmmoLast[ammoIndex])
	{
		m_rgAmmoLast[ammoIndex] = m_rgAmmo[ammoIndex];

		ASSERT(m_rgAmmo[ammoIndex] >= 0);
		ASSERT(m_rgAmmo[ammoIndex] < 255);

		// send "Ammo" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoX, NULL, pev);
		WRITE_BYTE(ammoIndex);
		WRITE_BYTE(V_max(V_min(m_rgAmmo[ammoIndex], 254), 0)); // clamp the value to one byte
		MESSAGE_END();
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
// orig 0xf8ae0. Oz: the flashlight does not drain with oz_nightops. As the original: no Weapons
// message (the weapon bits reach Valve's client in the client data), no status bar, no empty
// CurWeapon here (RemoveAllItems sends it). halflife-updated's two-byte Health with its one-health
// rounding, flashlight resend after a restore and room type send (bc3add9) stay (fixes).
void CBasePlayer::UpdateClientData()
{
	if (m_fInitHUD)
	{
		m_fInitHUD = false;
		gInitHUD = false;

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		if (!m_fGameHUDInitialized)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, NULL, pev);
			MESSAGE_END();

			g_pGameRules->InitHUD(this);
			m_fGameHUDInitialized = true;

			if (g_pGameRules->IsMultiplayer())
			{
				FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
			}
		}

		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pev);
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if (m_iFOV != m_iClientFOV)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
		WRITE_BYTE(m_iFOV);
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	//TODO: will not work properly in multiplayer
	if (gDisplayTitle)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgShowGameTitle, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();
		gDisplayTitle = false;
	}

	if (pev->health != m_iClientHealth)
	{
		int iHealth = std::clamp<float>(pev->health, 0.f, (float)(std::numeric_limits<short>::max())); // make sure that no negative health values are sent
		if (pev->health > 0.0f && pev->health <= 1.0f)
			iHealth = 1; // halflife-updated's fix, kept: a living player never shows 0

		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
		WRITE_SHORT(iHealth);
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}


	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ASSERT(gmsgBattery > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgBattery, NULL, pev);
		WRITE_SHORT((int)pev->armorvalue);
		MESSAGE_END();
	}

	if (0 != pev->dmg_take || 0 != pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t* other = pev->dmg_inflictor;
		if (other)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(other);
			if (pEntity)
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN(MSG_ONE, gmsgDamage, NULL, pev);
		WRITE_BYTE(pev->dmg_save);
		WRITE_BYTE(pev->dmg_take);
		WRITE_LONG(visibleDamageBits);
		WRITE_COORD(damageOrigin.x);
		WRITE_COORD(damageOrigin.y);
		WRITE_COORD(damageOrigin.z);
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	if (m_bRestored)
	{
		//Always tell client about battery state
		MESSAGE_BEGIN(MSG_ONE, gmsgFlashBattery, NULL, pev);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		//Tell client the flashlight is on
		if (FlashlightIsOn())
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, NULL, pev);
			WRITE_BYTE(1);
			WRITE_BYTE(m_iFlashBattery);
			MESSAGE_END();
		}
	}

	// Update Flashlight
	if ((0 != m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->time))
	{
		if (FlashlightIsOn())
		{
			// Oz: no drain with oz_nightops. Then m_flFlashLightTime stays due and FlashBat goes
			// out every frame (original bug)
			if (0 != m_iFlashBattery && oz_nightops.value < 1)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				m_iFlashBattery--;

				if (0 == m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgFlashBattery, NULL, pev);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}


	if ((m_iTrain & TRAIN_NEW) != 0)
	{
		ASSERT(gmsgTrain > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgTrain, NULL, pev);
		WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	if (!m_fKnownItem)
	{
		m_fKnownItem = true;

		// WeaponInit Message
		// byte  = # of weapons
		//
		// for each weapon:
		// byte		name str length (not including null)
		// bytes... name
		// byte		Ammo Type
		// byte		Ammo2 Type
		// byte		bucket
		// byte		bucket pos
		// byte		flags
		// ????		Icons

		// Send ALL the weapon info now
		int i;

		for (i = 0; i < MAX_WEAPONS; i++)
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

			if (WEAPON_NONE == II.iId)
				continue;

			const char* pszName;
			if (!II.pszName)
				pszName = "Empty";
			else
				pszName = II.pszName;

			MESSAGE_BEGIN(MSG_ONE, gmsgWeaponList, NULL, pev);
			WRITE_STRING(pszName);				   // string	weapon name
			WRITE_BYTE(GetAmmoIndex(II.pszAmmo1)); // byte		Ammo Type
			WRITE_BYTE(II.iMaxAmmo1);			   // byte     Max Ammo 1
			WRITE_BYTE(GetAmmoIndex(II.pszAmmo2)); // byte		Ammo2 Type
			WRITE_BYTE(II.iMaxAmmo2);			   // byte     Max Ammo 2
			WRITE_BYTE(II.iSlot);				   // byte		bucket
			WRITE_BYTE(II.iPosition);			   // byte		bucket pos
			WRITE_BYTE(II.iId);					   // byte		id (bit index into m_WeaponBits)
			WRITE_BYTE(II.iFlags);				   // byte		Flags
			MESSAGE_END();
		}
	}


	SendAmmoUpdate();

	// Update all the items
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i]) // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData(this);
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Send new room type to client.
	if (m_ClientSndRoomtype != m_SndRoomtype)
	{
		m_ClientSndRoomtype = m_SndRoomtype;

		MESSAGE_BEGIN(MSG_ONE, SVC_ROOMTYPE, nullptr, edict());
		WRITE_SHORT((short)m_SndRoomtype); // sequence number
		MESSAGE_END();
	}

	//Handled anything that needs resetting
	m_bRestored = false;
}


//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
// orig 0xf9440
bool CBasePlayer::FBecomeProne()
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return true;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
// orig 0xf9460
void CBasePlayer::BarnacleVictimBitten(entvars_t* pevBarnacle)
{
	TakeDamage(pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB);
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns.
//=========================================================
// orig 0xf94b0
void CBasePlayer::BarnacleVictimReleased()
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}


//=========================================================
// Illumination
// return player light level plus virtual muzzle flash
//=========================================================
// orig 0xf94d0
int CBasePlayer::Illumination()
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}


// orig 0xf9520
void CBasePlayer::EnableControl(bool fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}


#define DOT_1DEGREE 0.9998476951564
#define DOT_2DEGREE 0.9993908270191
#define DOT_3DEGREE 0.9986295347546
#define DOT_4DEGREE 0.9975640502598
#define DOT_5DEGREE 0.9961946980917
#define DOT_6DEGREE 0.9945218953683
#define DOT_7DEGREE 0.9925461516413
#define DOT_8DEGREE 0.9902680687416
#define DOT_9DEGREE 0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
// orig 0xf9550. SDK 2.3's. halflife-updated's ResetAutoaim with sv_aim 0 stays (fix); its
// sv_allow_autoaim test is out (design), as mod-rockcrowbar.
Vector CBasePlayer::GetAutoaimVector(float flDelta)
{
	if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors(pev->v_angle + pev->punchangle);
		return gpGlobals->v_forward;
	}

	Vector vecSrc = GetGunPosition();
	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (true || g_iSkillLevel == SKILL_MEDIUM)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		// flDelta *= 0.5;
	}

	bool m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta);

	// update ontarget if changed
	if (!g_pGameRules->AllowAutoTargetCrosshair())
		m_fOnTarget = false;
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveItem->UpdateItemInfo();
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (false || g_iSkillLevel == SKILL_EASY)
	{
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if (g_psv_aim->value != 0)
	{
		if (m_vecAutoAim.x != m_lastx ||
			m_vecAutoAim.y != m_lasty)
		{
			SET_CROSSHAIRANGLE(edict(), -m_vecAutoAim.x, m_vecAutoAim.y);

			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}
	else
	{
		ResetAutoaim();
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);
	return gpGlobals->v_forward;
}


// orig 0xf9970. SDK 2.3's; no sv_allow_autoaim (as GetAutoaimVector).
Vector CBasePlayer::AutoaimDeflection(Vector& vecSrc, float flDist, float flDelta)
{
	edict_t* pEdict = UTIL_GetEntityList() + 1;
	CBaseEntity* pEntity;
	float bestdot;
	Vector bestdir;
	edict_t* bestent;
	TraceResult tr;

	if (g_psv_aim->value == 0)
	{
		m_fOnTarget = false;
		return g_vecZero;
	}

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = false;

	UTIL_TraceLine(vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr);


	if (tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
	{
		// don't look through water
		if (!((pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3) || (pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0)))
		{
			if (tr.pHit->v.takedamage == DAMAGE_AIM)
				m_fOnTarget = true;

			return m_vecAutoAim;
		}
	}

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		Vector center;
		Vector dir;
		float dot;

		if (0 != pEdict->free) // Not in use
			continue;

		if (pEdict->v.takedamage != DAMAGE_AIM)
			continue;
		if (pEdict == edict())
			continue;
		//		if (pev->team > 0 && pEdict->v.team == pev->team)
		//			continue;	// don't aim at teammate
		if (!g_pGameRules->ShouldAutoAim(this, pEdict))
			continue;

		pEntity = Instance(pEdict);
		if (pEntity == NULL)
			continue;

		if (!pEntity->IsAlive())
			continue;

		// don't look through water
		if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) || (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
			continue;

		center = pEntity->BodyTarget(vecSrc);

		dir = (center - vecSrc).Normalize();

		// make sure it's in front of the player
		if (DotProduct(dir, gpGlobals->v_forward) < 0)
			continue;

		dot = fabs(DotProduct(dir, gpGlobals->v_right)) + fabs(DotProduct(dir, gpGlobals->v_up)) * 0.5;

		// tweek for distance
		dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

		if (dot > bestdot)
			continue; // to far to turn

		UTIL_TraceLine(vecSrc, center, dont_ignore_monsters, edict(), &tr);
		if (tr.flFraction != 1.0 && tr.pHit != pEdict)
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if (IRelationship(pEntity) < 0)
		{
			if (!pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
				// ALERT( at_console, "friend\n");
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if (bestent)
	{
		bestdir = UTIL_VecToAngles(bestdir);
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if (bestent->v.takedamage == DAMAGE_AIM)
			m_fOnTarget = true;

		return bestdir;
	}

	return Vector(0, 0, 0);
}


// orig 0xf9ff0
void CBasePlayer::ResetAutoaim()
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		SET_CROSSHAIRANGLE(edict(), 0, 0);
	}
	m_fOnTarget = false;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
// orig 0xfa0a0
void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
// orig 0xfa0d0
int CBasePlayer::GetCustomDecalFrames()
{
	return m_nCustomSprayFrames;
}


//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item.
//=========================================================
// orig 0xfa0e0. SDK 2.3's: halflife-updated's refusal when there is no next weapon is out
// (design, as mod-rc2/mod-rockcrowbar).
void CBasePlayer::DropPlayerItem(char* pszItemName)
{
	if (!g_pGameRules->IsMultiplayer() || (weaponstay.value > 0))
	{
		// no dropping in single player.
		return;
	}

	if (0 == strlen(pszItemName))
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	}

	CBasePlayerItem* pWeapon;
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			if (pszItemName)
			{
				// try to match by name.
				if (0 == strcmp(pszItemName, STRING(pWeapon->pev->classname)))
				{
					// match!
					break;
				}
			}
			else
			{
				// trying to drop active item
				if (pWeapon == m_pActiveItem)
				{
					// active item!
					break;
				}
			}

			pWeapon = pWeapon->m_pNext;
		}


		// if we land here with a valid pWeapon pointer, that's because we found the
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if (pWeapon)
		{
			g_pGameRules->GetNextBestWeapon(this, pWeapon);

			UTIL_MakeVectors(pev->angles);

			ClearWeaponBit(pWeapon->m_iId); // take item off hud

			CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->PackWeapon(pWeapon);
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

			// drop half of the ammo for this weapon.
			int iAmmoIndex;

			iAmmoIndex = GetAmmoIndex(pWeapon->pszAmmo1()); // ???

			if (iAmmoIndex != -1)
			{
				// this weapon weapon uses ammo, so pack an appropriate amount.
				if ((pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) != 0)
				{
					// pack up all the ammo, this weapon is its own ammo type
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex]);
					m_rgAmmo[iAmmoIndex] = 0;
				}
				else
				{
					// pack half of the ammo
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex] / 2);
					m_rgAmmo[iAmmoIndex] /= 2;
				}
			}

			return; // we're done, so stop searching with the FOR loop.
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
// orig 0xfa440
bool CBasePlayer::HasPlayerItem(CBasePlayerItem* pCheckItem)
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
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
// orig 0xfa4d0
bool CBasePlayer::HasNamedPlayerItem(const char* pszItemName)
{
	CBasePlayerItem* pItem;
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pItem = m_rgpPlayerItems[i];

		while (pItem)
		{
			if (0 == strcmp(pszItemName, STRING(pItem->pev->classname)))
			{
				return true;
			}
			pItem = pItem->m_pNext;
		}
	}

	return false;
}

//=========================================================
// HasPlayerItemFromID
// Just compare IDs, rather than classnames
//=========================================================
bool CBasePlayer::HasPlayerItemFromID(int nID)
{
	CBasePlayerItem* pItem;
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pItem = m_rgpPlayerItems[i];

		while (pItem)
		{
			if (pItem->m_iId == nID)
			{
				return true;
			}

			pItem = pItem->m_pNext;
		}
	}

	return false;
}

//=========================================================
//
//=========================================================
// orig 0xfa540. SDK 2.3's; halflife-updated's NULL check and m_ForceSendAnimations stay (fixes, as mod-rc2)
bool CBasePlayer::SwitchWeapon(CBasePlayerItem* pWeapon)
{
	if (pWeapon && !pWeapon->CanDeploy())
	{
		return false;
	}

	ResetAutoaim();

	if (m_pActiveItem)
	{
		m_pActiveItem->Holster();
	}

	m_pActiveItem = pWeapon;

	if (pWeapon)
	{
		pWeapon->m_ForceSendAnimations = true;
		pWeapon->Deploy();
		pWeapon->m_ForceSendAnimations = false;
	}

	return true;
}

void CBasePlayer::EquipWeapon()
{
	if (m_pActiveItem)
	{
		if ((!FStringNull(pev->viewmodel) || !FStringNull(pev->weaponmodel)))
		{
			//Already have a weapon equipped and deployed.
			return;
		}

		//Have a weapon equipped, but not deployed.
		if (m_pActiveItem->CanDeploy() && m_pActiveItem->Deploy())
		{
			return;
		}
	}

	//No weapon equipped or couldn't deploy it, find a suitable alternative.
	g_pGameRules->GetNextBestWeapon(this, m_pActiveItem, true);
}

void CBasePlayer::SetPrefsFromUserinfo(char* infobuffer)
{
	const char* value = g_engfuncs.pfnInfoKeyValue(infobuffer, "cl_autowepswitch");

	if ('\0' != *value)
	{
		m_iAutoWepSwitch = atoi(value);
	}
	else
	{
		m_iAutoWepSwitch = 1;
	}
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static const char* m_szPoses[4];
};

const char* CDeadHEV::m_szPoses[] = {"deadback", "deadsitting", "deadstomach", "deadtable"};

// orig 0xfa5c0
bool CDeadHEV::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_hevsuit_dead, CDeadHEV);

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
// orig 0xfa7a0
void CDeadHEV::Spawn()
{
	PRECACHE_MODEL("models/player.mdl");
	SET_MODEL(ENT(pev), "models/player.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hevsuit with bad pose\n");
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}


class CStripWeapons : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

// orig 0xfa8c0. SDK 2.3's; halflife-updated's UTIL_GetLocalPlayer stays (the original took slot 1's private data, the world's when empty)
void CStripWeapons::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = NULL;

	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer*)pActivator;
	}
	else if (!g_pGameRules->IsDeathmatch())
	{
		pPlayer = (CBasePlayer*)UTIL_GetLocalPlayer();
	}

	if (pPlayer)
		pPlayer->RemoveAllItems(false);
}


class CRevertSaved : public CPointEntity
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT MessageThink();
	void EXPORT LoadThink();
	bool KeyValue(KeyValueData* pkvd) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration() { return pev->dmg_take; }
	inline float HoldTime() { return pev->dmg_save; }
	inline float MessageTime() { return m_messageTime; }
	inline float LoadTime() { return m_loadTime; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
	{
		DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT), // These are not actual times, but durations, so save as floats
		DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

// orig 0xfaa30
bool CRevertSaved::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

// orig 0xfb0b0
void CRevertSaved::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}


// orig 0xfb140
void CRevertSaved::MessageThink()
{
	UTIL_ShowMessageAll(STRING(pev->message));
	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}


// orig 0xfb1d0
void CRevertSaved::LoadThink()
{
	if (0 == gpGlobals->deathmatch)
	{
		SERVER_COMMAND("reload\n");
	}
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
	void Spawn() override;
	void Think() override;
};

// orig 0xfb220
void CInfoIntermission::Spawn()
{
	UTIL_SetOrigin(pev, pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2; // let targets spawn!
}

// orig 0xfb2a0
void CInfoIntermission::Think()
{
	edict_t* pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);

// Oz Grapple
// orig 0xfb3f0. Lets go of the grapple: out of the hang, normal gravity and walking, the bolt killed
void CBasePlayer::PlayerReleaseGrapple()
{
	if (pGrappleBolt)
	{
		if (IsHanging)
		{
			IsHanging = false;
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
		}
		pev->gravity = 1;
		pev->movetype = MOVETYPE_WALK;
		// MERGE: CGrappleBolt is grapple.h's (a CBaseEntity); call Killed on it directly once included
		reinterpret_cast<CBaseEntity*>(pGrappleBolt)->Killed(NULL, 0);
		pGrappleBolt = NULL;
	}
}

// Oz motd fix
#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536 // (MAX_MOTD_CHUNK * 4)

// orig 0xfb470. The MOTD for one client: maps/<map>.motd if there is one (loaded twice, both
// freed), else the motdfile cvar's file, after the ServerName message (hostname)
void CBasePlayer::MOTD(edict_t* client)
{
	int length, char_count = 0;
	char szMapMOTD[256];

	sprintf(szMapMOTD, "maps/%s.motd", STRING(gpGlobals->mapname));
	char* pMapFile = (char*)LOAD_FILE_FOR_ME(szMapMOTD, &length);

	char* aFileList;
	if (!pMapFile)
		aFileList = (char*)LOAD_FILE_FOR_ME((char*)CVAR_GET_STRING("motdfile"), &length);
	else
		aFileList = (char*)LOAD_FILE_FOR_ME(szMapMOTD, &length);

	char* pFileList = aFileList;

	MESSAGE_BEGIN(MSG_ONE, gmsgServerName, NULL, client);
	WRITE_STRING(CVAR_GET_STRING("hostname"));
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while (pFileList && '\0' != *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0; // strncpy doesn't always append the null terminator
		}

		char_count += strlen(chunk);
		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, client);
		WRITE_BYTE(*pFileList ? 0 : 1); // 0 means there is still more message to come
		WRITE_STRING(chunk);
		MESSAGE_END();
	}

	FREE_FILE(aFileList);
	FREE_FILE(pMapFile);
}

// Oz player id
// orig 0xfb8c0. The player in the crosshair, center printed: his name and rune (oz_playerid on,
// alive, not holding the identify rune), or with the identify rune his name, health, armor,
// distance and rune. Nothing while firing.
void CBasePlayer::Identify()
{
	TraceResult tr;

	Vector anglesAim = pev->v_angle;
	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;
	Vector vecSrc = GetGunPosition() - gpGlobals->v_up * 1;
	Vector vecDir = gpGlobals->v_forward;

	UTIL_TraceLine(vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, edict(), &tr);

	// a trace that hits nothing gives the world (Instance of NULL), which is not a player
	CBasePlayer* pOther = (CBasePlayer*)CBaseEntity::Instance(tr.pHit);

	if (!pOther->IsPlayer())
		return;

	if ((pev->button & (IN_ATTACK | IN_ATTACK2)) != 0)
		return;

	// CBasePlayer::BodyTarget's point, written out (the original inlines it)
	Vector vecTarget = pOther->Center() + pOther->pev->view_ofs * RANDOM_FLOAT(0.5, 1.1);
	Vector vecDelta = vecTarget - GetGunPosition();
	vecDelta.z = 0;
	const float flDist = vecDelta.Length();

	if (oz_playerid.value > 0 && m_iRune != 13 && IsAlive()) // not the Identify Rune
	{
		ClientPrint(pev, HUD_PRINTCENTER, UTIL_VarArgs("-%s-\n-%s-\n", STRING(pOther->pev->netname), GetRuneName(pOther->m_iRune)));
	}
	else if (m_iRune == 13) // Identify Rune
	{
		ClientPrint(pev, HUD_PRINTCENTER, UTIL_VarArgs("%s\nHealth: %d Armor: %d\nDistance: %d Rune: %s", STRING(pOther->pev->netname), (int)pOther->pev->health, (int)pOther->pev->armorvalue, (int)flDist, GetRuneName(pOther->m_iRune)));
	}
}

// Oz runes
// orig 0xfbc80. Drops the rune: the speed and whatever the rune changed come back, and the
// rune goes back into the map (SpawnRunes); a rune picked up as a random rune goes back as one
void CBasePlayer::RemoveRune()
{
	if (0 == m_iRune)
	{
		pev->gravity = 1;
		return;
	}

	if (0 != m_iRandomRune)
	{
		m_iRune = 15; // Random Rune
		m_iRandomRune = 0;
	}

	switch (m_iRune)
	{
	case 0:
		pev->gravity = 1;
		break;

	case 1:	 // Frag Rune
	case 2:	 // Regeneration Rune
	case 3:	 // Super Crowbar Rune
	case 4:	 // Haste Rune
	case 5:	 // Cloaking Rune
	case 6:	 // Super Hook Rune
	case 7:	 // Protection Rune
	case 8:	 // Low Gravity Rune
	case 9:	 // Vampire Rune
	case 10: // Booby Trap Rune
	case 11: // Super Glock Rune
	case 12: // Super Speed Rune
	case 13: // Identify Rune
	case 14: // Teleport Rune
	case 15: // Random Rune
		if (IsNetClient())
			pev->maxspeed = oz_playerspeed.value;
		else
			pev->maxspeed = bot_speed.value;

		if (m_iRune == 5) // Cloaking Rune
		{
			pev->rendermode = kRenderNormal;
			pev->renderfx &= ~kRenderFxGlowShell;
		}
		else if (m_iRune == 8) // Low Gravity Rune
		{
			pev->gravity = 1;
		}

		SpawnRunes(edict(), m_iRune, 1);
		break;
	}

	m_iRune = 0;
}

// One of player_names (ChooseRandomPlayerName, orig 0xfc150), none twice until all have been used
static int player_name_used[85] = {0}; // orig .data 0x1677c0
static char player_names[85][64] =	   // orig .data 0x166280
	{
		"Dead_Guy", "Kill_Me", "Noobee", "Crazy_Fool", "Kick_Me", "How_Do_You_Hook?", "Whats_A_Rune?",
		"Immoral_Skull", "Lifeless_Mutton", "Soul_Sculptor", "Hydra_Lore", "Innoect_Victim", "Trojan_Horse",
		"Stu_Padaso", "Concealed_Crisis", "Seraph_Of_Demise", "Unlawful_Death", "Scooby-Doo", "Scooby-Dum",
		"Scrappy-Doo", "Shaggy", "Fred", "Velma", "Daphne", "Sleepy", "Dopey", "Happy", "Sneezy", "Grumpy",
		"Doc", "Bashful", "Tommy", "Chucky", "Phil", "Lil", "Dill", "Reptar", "Robo-snail", "Eat_me", "BSOD",
		"I'm_With_Stupid", "Is_my_Flashlight_on?", "Cannon_Fodder", "Can_I_be_on_your_Team?",
		"How_do_I_do_that?", "Loser", "Lost_1", "Lost_2", "Lost_3", "1000101", "Ben", "BoBoKiLLeR", "botman",
		"Carmack", "Chandler_Bing", "Coffee", "crimrOw", "Darkblade", "DarkFig", "DeaDPooL", "Deltha_O'Neal",
		"Eraserman", "Eric", "Grym_Reaper", "gumby", "Oberon", "incognito", "Jackal", "J!mmyPop", "kastral",
		"kevlar", "Mace", "NARLOTEP", "Vacindak", "Oski", "phewl", "Scooter", "Professor_Harvey", "Shpen",
		"Slippy", "Cutter", "Dr_Deb", "Capt._John", "Commando_Dad", "Lazarus_Long"};

// orig 0xfc150
char* ChooseRandomPlayerName()
{
	int index = RANDOM_LONG(0, 84);

	int i;
	for (i = 0; i < 85; i++)
	{
		if (0 == player_name_used[i])
			break;
	}

	// all used: start over
	if (i == 85)
	{
		for (i = 0; i < 85; i++)
			player_name_used[i] = 0;
	}

	while (player_name_used[index] == 1)
	{
		index++;
		if (index == 85)
			index = 0;
	}

	player_name_used[index] = 1;
	return player_names[index];
}

// orig 0xfc1f0. A player who kept the default name: renamed "[PNP]<random name>" (a client
// command) and sent player.txt in the MOTD window
void CBasePlayer::PlayerMessage(edict_t* client)
{
	char szRandom[64] = "123456789012345678901234567890123456789012345678901234567890123";
	char szName[64] = "123456789012345678901234567890123456789012345678901234567890123";

	strncpy(szRandom, ChooseRandomPlayerName(), 63);
	sprintf(szName, "[PNP]%s", szRandom);
	CLIENT_COMMAND(client, UTIL_VarArgs("name %s\n", szName));

	int length, char_count = 0;
	char* aFileList = (char*)LOAD_FILE_FOR_ME("player.txt", &length);
	char* pFileList = aFileList;

	while (pFileList && '\0' != *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0;
		}

		char_count += strlen(chunk);
		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, client);
		WRITE_BYTE(*pFileList ? 0 : 1);
		WRITE_STRING(chunk);
		MESSAGE_END();
	}

	FREE_FILE(aFileList);

	CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
	pPlayer->m_iOz = 0;
	pPlayer->m_iStart = 26;
	pPlayer->m_iCommands = 26;
}

// orig .data 0x167914: when the next probe goes out, in whole seconds, one for all players
int timer = 0;

// orig 0xfc420. With oz_cheatdetect, every 5 s (one global timer, so one player per round)
// sends console commands that only HLHack and the XQZ2 wallhack answer; three unanswered rounds
// in a row (client.cpp's ClientCommand clears the counts on an answer) name the player to all.
void CBasePlayer::DetectCheats()
{
	if (oz_cheatdetect.value == 0)
		return;

	if (timer < gpGlobals->time)
	{
		timer = (int)(gpGlobals->time + 5);

		if (m_iBotFovCheck > 2)
		{
			CLIENT_COMMAND(edict(), "bot 0; zoom_fov 1; +zoom; bot_delay 30\n");
			UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("<SERVER> *** Aim bot detected on %s! ***\n", STRING(pev->netname)));
			// the original prints the auth ID string's address with %i (original bug)
			UTIL_LogPrintf("// --CHEATER-- \"HLHack 1.3\" detected on \"%s<%i>\"\n", STRING(pev->netname), (int)(intptr_t)GETPLAYERAUTHID(edict()));
		}
		m_iBotFovCheck++;
		CLIENT_COMMAND(edict(), "bot_fov 0\n");

		if (m_iWallhackCheck > 2)
		{
			CLIENT_COMMAND(edict(), "enable_wallhack 0; enable_aimhack 0\n");
			UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("<SERVER> *** XQZ2 Wall hack/Aim bot detected on %s! ***\n", STRING(pev->netname)));
			UTIL_LogPrintf("// --CHEATER-- \"XQZ2 Wall Hack/Aim bot\" detected on \"%s<%i>\"\n", STRING(pev->netname), (int)(intptr_t)GETPLAYERAUTHID(edict()));
		}
		m_iWallhackCheck++;
		CLIENT_COMMAND(edict(), "enable_wallhack 0\n");
	}
}

// orig 0xfc630. The teleport rune: to a spawn point, at most every 5 s
void CBasePlayer::Teleport()
{
	if (m_iTeleportTime < gpGlobals->time)
	{
		edict_t* pentSpawnSpot = EntSelectSpawnPoint(this);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "ambience/particle_suck1.wav", 1, ATTN_NORM);
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
		pev->angles = VARS(pentSpawnSpot)->angles;
		m_iTeleportTime = (int)(gpGlobals->time + 5);
	}
}

// TEMP STUB (player): real one belongs in globals.cpp (OZ's; not in this tree: world.cpp?)
DLL_GLOBAL bool g_runes_exist = false;

// TEMP STUB (player): real one belongs in items.cpp, orig 0xcb200
const char* GetRuneName(int iRune)
{
	return " ";
}

// TEMP STUB (player): real one belongs in items.cpp, orig 0xcbad0
void SpawnRunes(edict_t* pEdict, int iRune, int iCount)
{
}
