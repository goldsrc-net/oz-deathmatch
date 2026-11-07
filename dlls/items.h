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

#pragma once

class CItem : public CBaseEntity
{
public:
	void Spawn() override;
	CBaseEntity* Respawn() override;
	void EXPORT ItemTouch(CBaseEntity* pOther);
	void EXPORT Materialize();
	virtual bool MyTouch(CBasePlayer* pPlayer) { return false; } // orig 0xccf00
	void RuneHud(CBasePlayer* pPlayer, const char* pszText); // Oz: orig 0xccd90
};

// Oz runes: the values of CBasePlayer::m_iRune and CRune::m_iRuneType. The original uses bare
// numbers; these names are ours, after GetRuneName's.
enum
{
	RUNE_NONE = 0,
	RUNE_FRAG,		  // 1
	RUNE_REGEN,		  // 2
	RUNE_CROWBAR,	  // 3
	RUNE_HASTE,		  // 4
	RUNE_CLOAK,		  // 5
	RUNE_HOOK,		  // 6
	RUNE_PROTECT,	  // 7
	RUNE_LOWGRAV,	  // 8
	RUNE_VAMPIRE,	  // 9
	RUNE_TRAP,		  // 10
	RUNE_SUPERGLOCK,  // 11
	RUNE_SUPERSPEED,  // 12
	RUNE_IDENTIFY,	  // 13
	RUNE_TELEPORT,	  // 14
	RUNE_RANDOM,	  // 15: SpawnRunes picks one CanHaveRune allows, and the rune keeps changing
};

const char* GetRuneName(int iRune);		   // Oz: orig 0xcb200
const char* GetRuneDescription(int iRune); // Oz: orig 0xcb370
Vector GetRuneColor(int iRune);			   // Oz: orig 0xcb510
bool CanHaveRune(int iRune);			   // Oz: orig 0xcb940
// VERIFY name: iUnused, every caller passes 1 and the body never reads it
void SpawnRunes(edict_t* pOwner, int iRune, int iUnused); // Oz: orig 0xcbad0
