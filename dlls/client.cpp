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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to
//				   have one without a hardcoded player.mdl in tf_client.cpp

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include <algorithm>
#include <string>
#include <vector>

#include "extdll.h"
#include "util.h"
#include "filesystem_utils.h"
#include "cbase.h"
#include "com_model.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "UserMessages.h"
#include "nodes.h"
#include "items.h"
#include "grapple.h"

DLL_GLOBAL unsigned int g_ulFrameCount;

// Oz: the bots' functions and globals (bot.h, bot_misc.cpp)
#include "bot.h"

// Oz: the bot cycle state StartFrame keeps; orig 0x19c518 and 0x19c514 (bss beside the bot globals)
int bInitBotCycle;
float TimeNextPossibleBotConnect;

// Oz: the info_player_deathmatch count ServerActivate takes; the runes read it (items.cpp).
// orig 0x19c51c (bss beside the bot globals; its file in the original is not known, ServerActivate counts it)
int NumPlayerStarts;

extern void CopyToBodyQue(entvars_t* pev);

void LinkUserMessages();

/*
 * used by kill command and disconnect command
 * ROBIN: Moved here from player.cpp, to allow multiple player models
 */
// orig 0x95020
void set_suicide_frame(entvars_t* pev)
{
	if (!FStrEq(STRING(pev->model), "models/player.mdl"))
		return; // allready gibbed

	//	pev->frame		= $deatha11;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	pev->deadflag = DEAD_DEAD;
	pev->nextthink = -1;
}


/*
===========
ClientConnect

called when a player connects to a server
============
*/
// orig 0x95090
qboolean ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	return static_cast<qboolean>(g_pGameRules->ClientConnected(pEntity, pszName, pszAddress, szRejectReason));

	// a client connecting during an intermission can cause problems
	//	if (intermission_running)
	//		ExitIntermission ();
}


/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
// orig 0x950d0. OZ: frees a bot's private data first (so the GetClassPtr below makes a blank player),
// lets go of the grappling hook and drops the rune, and clears FL_CLIENT/FL_FAKECLIENT.
void ClientDisconnect(edict_t* pEntity)
{
	// Oz: RhoBot
	if ((pEntity->v.flags & FL_FAKECLIENT) != 0)
		FREE_PRIVATE(pEntity);

	if (g_fGameOver)
		return;

	entvars_t* pev = &pEntity->v;
	CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);

	// Oz grapple
	if (pPlayer->pGrappleBolt)
	{
		if (pPlayer->IsHanging)
		{
			pPlayer->IsHanging = false;
			pPlayer->m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
		}
		pev->movetype = MOVETYPE_WALK;
		pev->gravity = 1;
		pPlayer->pGrappleBolt->Killed(NULL, 0);
		pPlayer->pGrappleBolt = NULL;
	}

	// Oz runes - disconnect spawn
	if (pPlayer->m_iRune != 0)
		pPlayer->RemoveRune();

	// halflife-updated's fix, kept: no "- %s has left" text for a client that never got a name
	char text[256] = "";
	if (!FStringNull(pEntity->v.netname))
		snprintf(text, sizeof(text), "- %s has left the game\n", STRING(pEntity->v.netname));
	text[sizeof(text) - 1] = 0;
	MESSAGE_BEGIN(MSG_ALL, gmsgSayText, NULL);
	WRITE_BYTE(ENTINDEX(pEntity));
	WRITE_STRING(text);
	MESSAGE_END();

	CSound* pSound;
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(pEntity));
	{
		// since this client isn't around to think anymore, reset their sound.
		if (pSound)
		{
			pSound->Reset();
		}
	}

	// Oz
	GetClassPtr((CBasePlayer*)pev)->pev->flags &= ~(FL_FAKECLIENT | FL_CLIENT);

	// since the edict doesn't get deleted, fix it so it doesn't interfere.
	pEntity->v.takedamage = DAMAGE_NO; // don't attract autoaim
	pEntity->v.solid = SOLID_NOT;	   // nonsolid
	UTIL_SetOrigin(&pEntity->v, pEntity->v.origin);

	// halflife-updated's fix, kept: stop controlling a func_tank (de2751c, halflife#2594)
	if (pPlayer->m_pTank != NULL)
	{
		pPlayer->m_pTank->Use(pPlayer, pPlayer, USE_OFF, 0);
		pPlayer->m_pTank = NULL;
	}

	g_pGameRules->ClientDisconnected(pEntity);
}


// called by ClientKill and DeadThink
// orig 0x952e0
void respawn(entvars_t* pev, bool fCopyCorpse)
{
	if (0 != gpGlobals->coop || 0 != gpGlobals->deathmatch)
	{
		// Oz observer mode
		if (fCopyCorpse && pev->movetype != MOVETYPE_NOCLIP)
		{
			// make a copy of the dead body for appearances sake
			CopyToBodyQue(pev);
		}

		// respawn player
		GetClassPtr((CBasePlayer*)pev)->Spawn();
	}
	else
	{ // restart the entire server
		SERVER_COMMAND("reload\n");
	}
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
// orig 0x95370. OZ: no kill at 0 health or in observer mode; the booby trap rune remembers a suicide.
void ClientKill(edict_t* pEntity)
{
	entvars_t* pev = &pEntity->v;

	CBasePlayer* pl = (CBasePlayer*)CBasePlayer::Instance(pev);

	if (pl->pev->health == 0)
		return;

	// Oz observer mode
	if (pl->pev->flags == FL_SPECTATOR)
		return;

	if (pl->m_fNextSuicideTime > gpGlobals->time)
		return; // prevent suiciding too ofter

	pl->m_fNextSuicideTime = gpGlobals->time + 1; // don't let them suicide for 5 seconds after suiciding

	//jam : Oz runes booby trap rune
	if (pl->m_iRune == RUNE_TRAP) // Booby Trap Rune
		pl->wassuicide = 1;

	// have the player kill themself
	pev->health = 0;
	pl->Killed(pev, GIB_NEVER);

	//	pev->modelindex = g_ulModelIndexPlayer;
	//	pev->frags -= 2;		// extra penalty
	//	respawn( pev );
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
// orig 0x95440. No iuser1/iuser2 reset: that is Valve's 2013 spectator code (same call as mod-rockcrowbar).
void ClientPutInServer(edict_t* pEntity)
{
	CBasePlayer* pPlayer;

	entvars_t* pev = &pEntity->v;

	pPlayer = GetClassPtr((CBasePlayer*)pev);
	pPlayer->SetCustomDecalFrames(-1); // Assume none;

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn();

	// Reset interpolation during first frame
	pPlayer->pev->effects |= EF_NOINTERP;
}

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;



#if defined(_MSC_VER) || defined(WIN32)
typedef wchar_t uchar16;
typedef unsigned int uchar32;
#else
typedef unsigned short uchar16;
typedef wchar_t uchar32;
#endif

//-----------------------------------------------------------------------------
// Purpose: determine if a uchar32 represents a valid Unicode code point
//-----------------------------------------------------------------------------
bool Q_IsValidUChar32(uchar32 uVal)
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return (uVal < 0x110000u) && ((uVal - 0x00D800u) > 0x7FFu) && ((uVal & 0xFFFFu) < 0xFFFEu) && ((uVal - 0x00FDD0u) > 0x1Fu);
}


// Decode one character from a UTF-8 encoded string. Treats 6-byte CESU-8 sequences
// as a single character, as if they were a correctly-encoded 4-byte UTF-8 sequence.
int Q_UTF8ToUChar32(const char* pUTF8_, uchar32& uValueOut, bool& bErrorOut)
{
	const uint8* pUTF8 = (const uint8*)pUTF8_;

	int nBytes = 1;
	uint32 uValue = pUTF8[0];
	uint32 uMinValue = 0;

	// 0....... single byte
	if (uValue < 0x80)
		goto decodeFinishedNoCheck;

	// Expecting at least a two-byte sequence with 0xC0 <= first <= 0xF7 (110...... and 11110...)
	if ((uValue - 0xC0u) > 0x37u || (pUTF8[1] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0xC0 << 6) + pUTF8[1] - 0x80;
	nBytes = 2;
	uMinValue = 0x80;

	// 110..... two-byte lead byte
	if ((uValue & (0x20 << 6)) == 0)
		goto decodeFinished;

	// Expecting at least a three-byte sequence
	if ((pUTF8[2] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x20 << 12) + pUTF8[2] - 0x80;
	nBytes = 3;
	uMinValue = 0x800;

	// 1110.... three-byte lead byte
	if ((uValue & (0x10 << 12)) == 0)
		goto decodeFinishedMaybeCESU8;

	// Expecting a four-byte sequence, longest permissible in UTF-8
	if ((pUTF8[3] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x10 << 18) + pUTF8[3] - 0x80;
	nBytes = 4;
	uMinValue = 0x10000;

	// 11110... four-byte lead byte. fall through to finished.

decodeFinished:
	if (uValue >= uMinValue && Q_IsValidUChar32(uValue))
	{
	decodeFinishedNoCheck:
		uValueOut = uValue;
		bErrorOut = false;
		return nBytes;
	}
decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;

decodeFinishedMaybeCESU8:
	// Do we have a full UTF-16 surrogate pair that's been UTF-8 encoded afterwards?
	// That is, do we have 0xD800-0xDBFF followed by 0xDC00-0xDFFF? If so, decode it all.
	if ((uValue - 0xD800u) < 0x400u && pUTF8[3] == 0xED && (uint8)(pUTF8[4] - 0xB0) < 0x10 && (pUTF8[5] & 0xC0) == 0x80)
	{
		uValue = 0x10000 + ((uValue - 0xD800u) << 10) + ((uint8)(pUTF8[4] - 0xB0) << 6) + pUTF8[5] - 0x80;
		nBytes = 6;
		uMinValue = 0x10000;
	}
	goto decodeFinished;
}



//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-8 string contains invalid sequences.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate(const char* pUTF8)
{
	bool bError = false;
	while ('\0' != *pUTF8)
	{
		uchar32 uVal;
		// Our UTF-8 decoder silently fixes up 6-byte CESU-8 (improperly re-encoded UTF-16) sequences.
		// However, these are technically not valid UTF-8. So if we eat 6 bytes at once, it's an error.
		int nCharSize = Q_UTF8ToUChar32(pUTF8, uVal, bError);
		if (bError || nCharSize == 6)
			return false;
		pUTF8 += nCharSize;
	}
	return true;
}

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
// orig 0x95490. SDK 2.3's, with OZ's logchat: the line goes to the log only when logchat is 0, else to the
// server console only. Valve's 2013 additions are left out as features (the chat flood timer, "(SPEC)" chat,
// the voice-mute check, as mod-rockcrowbar); its UTF-8 check is kept as a fix (the isprint test it replaced
// dropped any text that was all non-ASCII).
void Host_Say(edict_t* pEntity, bool teamonly)
{
	CBasePlayer* client;
	int j;
	char* p;
	char text[128];
	char szTemp[256];
	const char* cpSay = "say";
	const char* cpSayTeam = "say_team";
	const char* pcmd = CMD_ARGV(0);

	// We can get a raw string now, without the "say " prepended
	if (CMD_ARGC() == 0)
		return;

	if (!stricmp(pcmd, cpSay) || !stricmp(pcmd, cpSayTeam))
	{
		if (CMD_ARGC() >= 2)
		{
			p = (char*)CMD_ARGS();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else // Raw text, need to prepend argv[0]
	{
		if (CMD_ARGC() >= 2)
		{
			sprintf(szTemp, "%s %s", (char*)pcmd, (char*)CMD_ARGS());
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf(szTemp, "%s", (char*)pcmd);
		}
		p = szTemp;
	}

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p) - 1] = 0;
	}

	// make sure the text has content
	// halflife-updated's fix, kept: Q_UnicodeValidate for the isprint/isspace loop
	if (!p || '\0' == p[0] || !Q_UnicodeValidate(p))
		return; // no character found, so say nothing

	// turn on color set 2  (color on,  no sound)
	if (teamonly)
		sprintf(text, "%c(TEAM) %s: ", 2, STRING(pEntity->v.netname));
	else
		sprintf(text, "%c%s: ", 2, STRING(pEntity->v.netname));

	j = sizeof(text) - 2 - strlen(text); // -2 for /n and null terminator
	if ((int)strlen(p) > j)
		p[j] = 0;

	strcat(text, p);
	strcat(text, "\n");

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	while (((client = (CBasePlayer*)UTIL_FindEntityByClassname(client, "player")) != NULL) && (!FNullEnt(client->edict())))
	{
		if (!client->pev)
			continue;

		if (client->edict() == pEntity)
			continue;

		if (!(client->IsNetClient())) // Not a client ? (should never be true)
			continue;

		if (teamonly && g_pGameRules->PlayerRelationship(client, CBaseEntity::Instance(pEntity)) != GR_TEAMMATE)
			continue;

		MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, client->pev);
		WRITE_BYTE(ENTINDEX(pEntity));
		WRITE_STRING(text);
		MESSAGE_END();
	}

	// print to the sending client
	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, &pEntity->v);
	WRITE_BYTE(ENTINDEX(pEntity));
	WRITE_STRING(text);
	MESSAGE_END();

	// Oz: logchat
	if (oz_logchat.value == 0)
	{
		const char* temp;
		if (teamonly)
			temp = "say_team";
		else
			temp = "say";

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" %s \"%s\"\n",
				STRING(pEntity->v.netname),
				GETPLAYERUSERID(pEntity),
				GETPLAYERAUTHID(pEntity),
				g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "model"),
				temp,
				p);
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" %s \"%s\"\n",
				STRING(pEntity->v.netname),
				GETPLAYERUSERID(pEntity),
				GETPLAYERAUTHID(pEntity),
				GETPLAYERUSERID(pEntity),
				temp,
				p);
		}
	}
	else
	{
		// echo to server console
		g_engfuncs.pfnServerPrint(text);
	}
}


/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
// Use CMD_ARGV,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
// orig 0x959e0. OZ's command set (the bot commands, runes, grapple, help screens, server messages).
// Only net clients run commands. Not in the original, and left out: Valve's 2013 "spectate", "specmode"
// and "follownext" (OZ has its own observer mode). Kept from halflife-updated: "fullupdate" and
// "closemenus" (sent by today's client) and the % filter on "Unknown command", as mod-rockcrowbar.
void ClientCommand(edict_t* pEntity)
{
	const char* pcmd = CMD_ARGV(0);
	const char* pstr;

	// Is the client spawned yet?
	if (!pEntity->pvPrivateData)
		return;

	entvars_t* pev = &pEntity->v;

	auto player = GetClassPtr<CBasePlayer>(reinterpret_cast<CBasePlayer*>(&pEntity->v));

	// Oz: bots don't run client commands
	if (!player->IsNetClient())
		return;

	if (FStrEq(pcmd, "say"))
	{
		Host_Say(pEntity, false);
	}
	else if (FStrEq(pcmd, "say_team"))
	{
		Host_Say(pEntity, true);
	}
	else if (FStrEq(pcmd, "fullupdate"))
	{
		player->ForceClientDllUpdate();
	}
	else if (FStrEq(pcmd, "give"))
	{
		// halflife-updated's fix, kept: sv_cheats read on demand (04f8b91, halflife#2511), not g_flWeaponCheat
		if (0 != g_psv_cheats->value)
		{
			int iszItem = ALLOC_STRING(CMD_ARGV(1)); // Make a copy of the classname
			player->GiveNamedItem(STRING(iszItem));
		}
	}

	else if (FStrEq(pcmd, "drop"))
	{
		// player is dropping an item.
		player->DropPlayerItem((char*)CMD_ARGV(1));
	}
	else if (FStrEq(pcmd, "fov"))
	{
		if (0 != g_psv_cheats->value && CMD_ARGC() > 1)
		{
			player->m_iFOV = atoi(CMD_ARGV(1));
		}
		else
		{
			CLIENT_PRINTF(pEntity, print_console, UTIL_VarArgs("\"fov\" is \"%d\"\n", (int)player->m_iFOV));
		}
	}
	else if (FStrEq(pcmd, "use"))
	{
		player->SelectItem((char*)CMD_ARGV(1));
	}
	else if (((pstr = strstr(pcmd, "weapon_")) != NULL) && (pstr == pcmd))
	{
		player->SelectItem(pcmd);
	}
	else if (FStrEq(pcmd, "lastinv"))
	{
		player->SelectLastItem();
	}
	// Oz cheat detection: a client without the cheat bounces these back here (CBasePlayer::DetectCheats)
	else if (FStrEq(pcmd, "bot_fov"))
	{
		player->m_iBotFovCheck = 0;
	}
	else if (FStrEq(pcmd, "enable_wallhack"))
	{
		player->m_iWallhackCheck = 0;
	}
	// Oz: RhoBot, listen server only
	else if (FStrEq(pcmd, "addbot"))
	{
		if (IS_DEDICATED_SERVER())
		{
			CLIENT_PRINTF(pEntity, print_console, "addbot not allowed from client!\n");
			return;
		}

		int iPlayers = 0;
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);
			if (pPlayer && pPlayer->IsPlayer())
				iPlayers++;
		}

		if (iPlayers > 31)
		{
			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "Rho-Bot supports a maximum of 32 players.\n");
			return;
		}

		char cmd[128] = "bot_number ###";
		sprintf(cmd, "bot_number %d\n", (int)bot_number.value + 1);
		SERVER_COMMAND(cmd);
		if (IS_DEDICATED_SERVER())
			printf("%s", cmd);
	}
	else if (FStrEq(pcmd, "removebots"))
	{
		if (IS_DEDICATED_SERVER())
		{
			CLIENT_PRINTF(pEntity, print_console, "removebots not allowed from client!\n");
			return;
		}

		char cmd[128] = "bot_number ###";
		sprintf(cmd, "bot_number %d\n", 0);
		SERVER_COMMAND(cmd);
		if (IS_DEDICATED_SERVER())
			printf("%s", cmd);
	}
	else if (FStrEq(pcmd, "removebot"))
	{
		if (IS_DEDICATED_SERVER())
		{
			CLIENT_PRINTF(pEntity, print_console, "removebot not allowed from client!\n");
			return;
		}

		// Any entity with that netname, bot or not (the original's lookup)
		CBaseEntity* pBot = UTIL_FindEntityByString(NULL, "netname", CMD_ARGV(1));
		if (!pBot)
		{
			char msg[128] = "kick Some Bot\n";
			sprintf(msg, "could not kick \"%s\"\n", CMD_ARGV(1));
			CLIENT_PRINTF(pEntity, print_console, msg);
			return;
		}

		pBot->pev->flags &= ~(FL_FAKECLIENT | FL_CLIENT);
		pBot->pev->health = 0;

		char kick[128] = "kick Some Bot\n";
		sprintf(kick, "kick \"%s\"\n", STRING(pBot->pev->netname));
		CLIENT_COMMAND(pEntity, kick); // the listen server's host runs the kick

		char cmd[128] = "bot_number ###";
		sprintf(cmd, "bot_number %d\n", (int)bot_number.value - 1);
		SERVER_COMMAND(cmd);
		if (IS_DEDICATED_SERVER())
			printf("%s", cmd);
	}
	// Oz runes
	else if (FStrEq(pcmd, "userune"))
	{
		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);

		if (pPlayer->m_iRune == RUNE_TRAP) // Booby Trap Rune: self destruct
		{
			CBasePlayer* pSelf = GetClassPtr((CBasePlayer*)pev);
			pSelf->TakeDamage(VARS(INDEXENT(0)), VARS(INDEXENT(0)), 1000, DMG_GENERIC);
		}

		if (pPlayer->m_iRune == RUNE_TELEPORT) // Teleport Rune
			GetClassPtr((CBasePlayer*)pev)->Teleport();
	}
	// Oz info
	else if (FStrEq(pcmd, "showtime") || FStrEq(pcmd, "showmapname") || FStrEq(pcmd, "info") || FStrEq(pcmd, "timeleft"))
	{
		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
		char szText[201];
		hudtextparms_t hText;

		const int iTimeLimit = (int)CVAR_GET_FLOAT("mp_timelimit");
		const int iFragLimit = (int)CVAR_GET_FLOAT("mp_fraglimit");
		const int iTimeLeft = (int)((float)((int)CVAR_GET_FLOAT("mp_timelimit") * 60) - gpGlobals->time);

		sprintf(szText, "Current Map: %s\nTime Limit: %02d\nFrag Limit: %02d\nEstimated Time Remaining: %02d:%02d\n",
			STRING(gpGlobals->mapname), iTimeLimit, iFragLimit, iTimeLeft / 60, iTimeLeft % 60);

		memset(&hText, 0, sizeof(hText));
		hText.channel = 4;
		hText.x = 0.01;
		hText.y = 0.6;
		hText.effect = 0;
		hText.r1 = hText.g1 = hText.b1 = 255;
		hText.a1 = 255;
		hText.r2 = hText.g2 = hText.b2 = 255;
		hText.a2 = 255;
		hText.fadeinTime = 0;
		hText.fadeoutTime = 0;
		hText.holdTime = 5;
		hText.fxTime = 0.5;
		UTIL_HudMessage(pPlayer, hText, szText);
	}
	else if (FStrEq(pcmd, "help"))
	{
		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
		char szText[201];
		hudtextparms_t hText;

		ClientPrint(pev, HUD_PRINTCONSOLE, "OZ CONSOLE COMMANDS:\n\nhelp - display this message\ninfo - display current map name and time\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, "+hook - aim and hold for grappling hook\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, "droprune - drops your current rune so you may get another\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, "userune - teleport or self destruct\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, "+voicerecord - hold key down to enable microphone\n \nEXAMPLE:\nbind x +hook\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, " \n  -= type: more and hit enter for more options =-\n");

		sprintf(szText, "CONSOLE COMMANDS:\n\ninfo\n+hook\ndroprune\nuserune\n\nEXAMPLE:\n bind x +hook\n");

		memset(&hText, 0, sizeof(hText));
		hText.channel = 4;
		hText.x = 0.01;
		hText.y = 0.5;
		hText.effect = 0;
		hText.r1 = 0;
		hText.g1 = 255;
		hText.b1 = 0;
		hText.a1 = 255;
		hText.r2 = 0;
		hText.g2 = 255;
		hText.b2 = 0;
		hText.a2 = 255;
		hText.fadeinTime = 1;
		hText.fadeoutTime = 3;
		hText.holdTime = 60;
		hText.fxTime = 0.5;
		UTIL_HudMessage(pPlayer, hText, szText);
	}
	else if (FStrEq(pcmd, "more"))
	{
		GetClassPtr((CBasePlayer*)pev);
		ClientPrint(pev, HUD_PRINTCONSOLE, "     -=  ADVANCED OPTIONS  =-\n \nThese are NOT bound to keys\n \ntype directly in console followed by enter\n \n");
		ClientPrint(pev, HUD_PRINTCONSOLE, "voice_enable 1 - enables voicemode\nvoice_forcemicrecord 1.000 - sets microphone enabled\nvoice_scale 1.5 - sets voice volume\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, "voice_overdrive 0 - mutes game audio when others talk\nfps_modem 90 - sets max fps to 90\nnet_graph 3 - compact network monitor\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, " \n \n type: cam and hit enter for Oz spectator options\n");
	}
	else if (FStrEq(pcmd, "cam"))
	{
		GetClassPtr((CBasePlayer*)pev);
		ClientPrint(pev, HUD_PRINTCONSOLE, "     -=  OZ SPECTATOR OPTIONS  =- \n");
		ClientPrint(pev, HUD_PRINTCONSOLE, " \nThen use your jump button to toggle camera views\nYour secondary fire button to watch other players\n");
		ClientPrint(pev, HUD_PRINTCONSOLE, "And your normal fire button to escape back to the game\n\n");
	}
	// Oz grapple
	else if (FStrEq(pcmd, "+hook"))
	{
		if (pev->deadflag != DEAD_NO)
			return;

		if (pev->movetype == MOVETYPE_NOCLIP)
			return;

		if (oz_grappledisable.value > 0)
			return;

		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
		if (pPlayer->pGrappleBolt)
			pPlayer->pGrappleBolt->Killed(NULL, 0);

		pPlayer->pGrappleBolt = CGrappleBolt::BoltCreate();
		pPlayer->pGrappleBolt->Init(pPlayer);
	}
	else if (FStrEq(pcmd, "-hook"))
	{
		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
		if (!pPlayer->pGrappleBolt)
			return;

		if (pPlayer->IsHanging)
		{
			pPlayer->IsHanging = false;
			pPlayer->m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
		}

		// Oz runes - make sure we don't reset the gravity of the low gravity rune
		if (pPlayer->m_iRune == RUNE_LOWGRAV) // Low Gravity Rune
		{
			pev->gravity = 1;
			pev->gravity = oz_lowgravruneamount.value;
		}
		else
		{
			pev->gravity = 1;
		}

		pev->movetype = MOVETYPE_WALK;
		pPlayer->pGrappleBolt->Killed(NULL, 0);
		pPlayer->pGrappleBolt = NULL;
	}
	// Oz personal messages
	else if (FStrEq(pcmd, "intN=1") || FStrEq(pcmd, "intN=2") || FStrEq(pcmd, "intN=3") || FStrEq(pcmd, "intN=4"))
	{
		GetClassPtr((CBasePlayer*)pev);
		char szText[201];
		hudtextparms_t hText;

		memset(&hText, 0, sizeof(hText));
		if (FStrEq(pcmd, "intN=1"))
		{
			sprintf(szText, "The Originator and Programmer of\n\nOz Deathmatch\n\nhas entered the server\n\nSay hello to Willi!\n");
			hText.x = 0.04;
			hText.y = 0.8;
		}
		else
		{
			if (FStrEq(pcmd, "intN=2"))
				sprintf(szText, "Attention!\n\n              Player's \n\n         attention\n\n             is required.\n");
			else if (FStrEq(pcmd, "intN=3"))
				sprintf(szText, "    NOTICE!\n\n              This server is monitored\n\n         For\n\n            Inappropriate Conduct.\n");
			else
				sprintf(szText, "Behold...For a \n\n              Paradigm Lozt Regular\n\n         walks among you!\n");
			hText.x = 0.3;
			hText.y = 0.25;
		}
		hText.channel = 4;
		hText.effect = 0;
		hText.r1 = hText.g1 = hText.b1 = 255;
		hText.a1 = 255;
		hText.r2 = hText.g2 = hText.b2 = 255;
		hText.a2 = 255;
		hText.fadeinTime = 1;
		hText.fadeoutTime = 5;
		hText.holdTime = 5;
		hText.fxTime = 0.5;
		UTIL_HudMessageAll(hText, szText);
	}
	// Oz runes - drop command
	else if (FStrEq(pcmd, "droprune"))
	{
		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
		if (pPlayer->m_iRune == 0)
			return;

		char szText[201];
		hudtextparms_t hText;

		sprintf(szText, "%s Discarded!\n", GetRuneName(pPlayer->m_iRune));

		if (oz_logrune.value == 1)
		{
			if (g_teamplay)
			{
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" Discarded the  \"%s\".\n",
					STRING(pEntity->v.netname),
					GETPLAYERUSERID(pEntity),
					GETPLAYERAUTHID(pEntity),
					g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "model"),
					GetRuneName(pPlayer->m_iRune));
			}
			else
			{
				UTIL_LogPrintf("\"%s<%i><%s><%i>\" Discarded the \"%s\".\n",
					STRING(pEntity->v.netname),
					GETPLAYERUSERID(pEntity),
					GETPLAYERAUTHID(pEntity),
					GETPLAYERUSERID(pEntity),
					GetRuneName(pPlayer->m_iRune));
			}
		}

		pPlayer->RemoveRune();

		memset(&hText, 0, sizeof(hText));
		hText.channel = 1;
		hText.x = 0.04;
		hText.y = 0.85;
		hText.effect = 0;
		hText.r1 = 255;
		hText.g1 = 0;
		hText.b1 = 0;
		hText.a1 = 255;
		hText.r2 = 255;
		hText.g2 = 0;
		hText.b2 = 0;
		hText.a2 = 255;
		hText.fadeinTime = 0;
		hText.fadeoutTime = 1;
		hText.holdTime = 5;
		hText.fxTime = 0.5;
		UTIL_HudMessage(pPlayer, hText, szText);
	}
	// Oz runes - current rune
	else if (FStrEq(pcmd, "runestatus"))
	{
		CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);
		hudtextparms_t hText;

		memset(&hText, 0, sizeof(hText));
		hText.channel = 4;
		hText.x = 0.04;
		hText.y = 0.75;
		hText.effect = 0;
		hText.r1 = 255;
		hText.g1 = 255;
		hText.b1 = 0;
		hText.a1 = 255;
		hText.r2 = 255;
		hText.g2 = 255;
		hText.b2 = 0;
		hText.a2 = 255;
		hText.fadeinTime = 0;
		hText.fadeoutTime = 1;
		hText.holdTime = 5;
		hText.fxTime = 0.5;
		UTIL_HudMessage(pPlayer, hText, GetRuneDescription(pPlayer->m_iRune));
	}
	else if (FStrEq(pcmd, "closemenus"))
	{
		// just ignore it
	}
	else if (g_pGameRules->ClientCommand(player, pcmd))
	{
		// MenuSelect returns true only if the command is properly handled,  so don't print a warning
	}
	else
	{
		// tell the user they entered an unknown command
		char command[128];

		// check the length of the command (prevents crash)
		// max total length is 192 ...and we're adding a string below ("Unknown command: %s\n")
		strncpy(command, pcmd, 127);
		command[127] = '\0';
		// First parse the name and remove any %'s
		for (char* pApersand = command; pApersand != NULL && *pApersand != 0; pApersand++)
		{
			// Replace it with a space
			if (*pApersand == '%')
				*pApersand = ' ';
		}

		// tell the user they entered an unknown command
		ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs("Unknown command: %s\n", command));
	}
}


/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
// orig 0x97140. Same as the original (it has the % filter too); halflife-updated's maxClients > 1 test
// around the name-change message changes nothing in multiplayer.
void ClientUserInfoChanged(edict_t* pEntity, char* infobuffer)
{
	// Is the client spawned yet?
	if (!pEntity->pvPrivateData)
		return;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if (!FStringNull(pEntity->v.netname) && STRING(pEntity->v.netname)[0] != 0 && !FStrEq(STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue(infobuffer, "name")))
	{
		char sName[256];
		char* pName = g_engfuncs.pfnInfoKeyValue(infobuffer, "name");
		strncpy(sName, pName, sizeof(sName) - 1);
		sName[sizeof(sName) - 1] = '\0';

		// First parse the name and remove any %'s
		for (char* pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++)
		{
			// Replace it with a space
			if (*pApersand == '%')
				*pApersand = ' ';
		}

		// Set the name
		g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pEntity), infobuffer, "name", sName);

		if (gpGlobals->maxClients > 1)
		{
			char text[256];
			sprintf(text, "* %s changed name to %s\n", STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
			MESSAGE_BEGIN(MSG_ALL, gmsgSayText, NULL);
			WRITE_BYTE(ENTINDEX(pEntity));
			WRITE_STRING(text);
			MESSAGE_END();
		}

		// team match?
		if (g_teamplay)
		{
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" changed name to \"%s\"\n",
				STRING(pEntity->v.netname),
				GETPLAYERUSERID(pEntity),
				GETPLAYERAUTHID(pEntity),
				g_engfuncs.pfnInfoKeyValue(infobuffer, "model"),
				g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
		}
		else
		{
			UTIL_LogPrintf("\"%s<%i><%s><%i>\" changed name to \"%s\"\n",
				STRING(pEntity->v.netname),
				GETPLAYERUSERID(pEntity),
				GETPLAYERAUTHID(pEntity),
				GETPLAYERUSERID(pEntity),
				g_engfuncs.pfnInfoKeyValue(infobuffer, "name"));
		}
	}

	g_pGameRules->ClientUserInfoChanged(GetClassPtr((CBasePlayer*)&pEntity->v), infobuffer);
}

static int g_serveractive = 0;

// orig 0x97450
void ServerDeactivate()
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate
	if (g_serveractive != 1)
	{
		return;
	}

	g_serveractive = 0;

	// Peform any shutdown operations here...
	//
}

// orig 0x97480. OZ counts the deathmatch starts first (NumPlayerStarts, read by the runes).
// halflife-updated's loop bounds kept: the original's SDK 2.x test (i < clientMax) skips the world and
// tries the last client slot; neither has anything to activate.
void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax)
{
	int i;
	CBaseEntity* pClass;

	// Oz
	NumPlayerStarts = 0;
	CBaseEntity* pSpot = NULL;
	while ((pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch")) != NULL)
		NumPlayerStarts++;

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

	// Clients have not been initialized yet
	for (i = 0; i < edictCount; i++)
	{
		if (0 != pEdictList[i].free)
			continue;

		// Clients aren't necessarily initialized until ClientPutInServer()
		if ((i > 0 && i <= clientMax) || !pEdictList[i].pvPrivateData)
			continue;

		pClass = CBaseEntity::Instance(&pEdictList[i]);
		// Activate this entity if it's got a class & isn't dormant
		if (pClass && (pClass->pev->flags & FL_DORMANT) == 0)
		{
			pClass->Activate();
		}
		else
		{
			ALERT(at_console, "Can't instance %s\n", STRING(pEdictList[i].v.classname));
		}
	}

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();
}


/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
// orig 0x975b0
void PlayerPreThink(edict_t* pEntity)
{
	entvars_t* pev = &pEntity->v;
	CBasePlayer* pPlayer = (CBasePlayer*)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PreThink();
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
// orig 0x976b0. OZ: the join-time HUD lines (version, help, oz_line1/oz_line2, MOTD, observer hint),
// the rune HUD refresh, the default-name message and spawn protection (glow shell and god mode for
// 120 frames or until the first attack).
void PlayerPostThink(edict_t* pEntity)
{
	entvars_t* pev = &pEntity->v;
	CBasePlayer* pPlayer = (CBasePlayer*)GET_PRIVATE(pEntity);

	// Oz messaging (the original calls through pPlayer unchecked here)
	if (pPlayer->IsNetClient())
	{
		if (pPlayer->m_iRuneHudTime < 900)
			pPlayer->m_iRuneHudTime++;
		else
			pPlayer->UpdateRuneInfo();

		if (pPlayer->m_iStart < 26)
			pPlayer->m_iStart++;
		if (pPlayer->m_iCommands < 26)
			pPlayer->m_iCommands++;
		if (pPlayer->m_iOz < 26)
			pPlayer->m_iOz++;
		if (pPlayer->m_iMOTDTimer < 26)
			pPlayer->m_iMOTDTimer++;

		if (pPlayer->m_iNameCheckTimer < 26 && ++pPlayer->m_iNameCheckTimer == 25)
		{
			// a default name gets PlayerMessage
			const char* pszName = STRING(pPlayer->pev->netname);
			const size_t iLen = strlen(pszName);
			if (iLen == 6)
			{
				if (0 == stricmp(pszName, "Player"))
					pPlayer->PlayerMessage(pPlayer->edict());
			}
			else if (iLen == 9 && 0 != isdigit(pszName[1])) // VERIFY: the original tests isdigit on name[1] (0x97847); an odd test, read literally
			{
				pPlayer->PlayerMessage(pPlayer->edict());
			}

			pPlayer->m_iOz = 0;
			pPlayer->m_iStart = 0;
			pPlayer->m_iCommands = 0;
		}

		if (pPlayer->m_iMOTDTimer == 25)
		{
			if (pPlayer->IsNetClient())
				CLIENT_COMMAND(pEntity, "console 1.0\n");

			if (oz_picture.value == 1)
			{
				pPlayer->m_iOz = 26;
				pPlayer->m_iStart = 26;
				pPlayer->m_iCommands = 26;
			}
			else
			{
				pPlayer->m_iOz = 0;
				pPlayer->m_iStart = 0;
				pPlayer->m_iCommands = 0;
				pPlayer->MOTD(pPlayer->edict());
			}
		}

		if (pPlayer->m_iCommands == 25)
		{
			char szText[201];
			hudtextparms_t hText;

			sprintf(szText, "Type help in the console for command help\n");

			memset(&hText, 0, sizeof(hText));
			hText.channel = 4;
			hText.x = 0.01;
			hText.y = 0.9;
			hText.effect = 0;
			hText.r1 = 0;
			hText.g1 = 255;
			hText.b1 = 0;
			hText.a1 = 255;
			hText.r2 = 0;
			hText.g2 = 255;
			hText.b2 = 0;
			hText.a2 = 255;
			hText.fadeinTime = 1;
			hText.fadeoutTime = 5;
			hText.holdTime = 45;
			hText.fxTime = 0.5;
			UTIL_HudMessage(pPlayer, hText, szText);
			pPlayer->m_iCommands = 26;
		}

		if (pPlayer->m_iOz == 25)
		{
			if (pPlayer->IsNetClient())
			{
				CLIENT_COMMAND(pEntity, UTIL_VarArgs("cl_forwardspeed %f\n", CVAR_GET_FLOAT("sv_maxspeed")));
				CLIENT_COMMAND(pEntity, UTIL_VarArgs("cl_backspeed %f\n", CVAR_GET_FLOAT("sv_maxspeed")));
			}

			char szText[201];
			hudtextparms_t hText;

			if (CVAR_GET_FLOAT("mp_teamplay") != 0)
				sprintf(szText, "Oz Teamplay v3.02\n%s\n", CVAR_GET_STRING("oz_link"));
			else
				sprintf(szText, "Oz Deathmatch v3.02\n%s\n", CVAR_GET_STRING("oz_link"));

			memset(&hText, 0, sizeof(hText));
			hText.channel = 2;
			hText.x = 0.5;
			hText.y = 0.01;
			hText.effect = 0;
			hText.r1 = 0;
			hText.g1 = 255;
			hText.b1 = 0;
			hText.a1 = 255;
			hText.r2 = 0;
			hText.g2 = 255;
			hText.b2 = 0;
			hText.a2 = 255;
			hText.fadeinTime = 1;
			hText.fadeoutTime = 5;
			hText.holdTime = 45;
			hText.fxTime = 0; // the original leaves it at memset's 0
			UTIL_HudMessage(pPlayer, hText, szText);
			pPlayer->m_iOz = 26;
		}

		// the server's own lines, the frame after the help hint
		if (pPlayer->m_iCommands == 26)
		{
			char szText[201];
			hudtextparms_t hText;

			sprintf(szText, "%s\n%s\n", CVAR_GET_STRING("oz_line1"), CVAR_GET_STRING("oz_line2"));

			memset(&hText, 0, sizeof(hText));
			hText.channel = 0;
			hText.x = 0.01;
			hText.y = 0.6;
			hText.effect = 0;
			hText.r1 = 215;
			hText.g1 = 150;
			hText.b1 = 253;
			hText.a1 = 255;
			hText.r2 = 215;
			hText.g2 = 150;
			hText.b2 = 253;
			hText.a2 = 255;
			hText.fadeinTime = 1;
			hText.fadeoutTime = 5;
			hText.holdTime = 7;
			hText.fxTime = 0.5;
			UTIL_HudMessage(pPlayer, hText, szText);
			pPlayer->m_iCommands = 27;
		}

		if (pPlayer->m_iStart == 25 && pPlayer->m_iObserver == 1)
		{
			char szText[201];
			hudtextparms_t hText;

			sprintf(szText, "                                    - Observer Mode -\nFire or Use to escape - Jump to change view - Alt Fire changes Players\n");

			memset(&hText, 0, sizeof(hText));
			hText.channel = 1;
			hText.x = 0.3;
			hText.y = 0.85;
			hText.effect = 0;
			hText.r1 = 255;
			hText.g1 = 0;
			hText.b1 = 0;
			hText.a1 = 255;
			hText.r2 = 255;
			hText.g2 = 0;
			hText.b2 = 0;
			hText.a2 = 255;
			hText.fadeinTime = 0;
			hText.fadeoutTime = 0;
			hText.holdTime = 40;
			hText.fxTime = 0.5;
			UTIL_HudMessage(pPlayer, hText, szText);
			pPlayer->m_iStart = 26;
		}
	}

	// Oz spawn protection
	if (pPlayer->m_iSpawnProtect < 120)
	{
		if ((pPlayer->m_afButtonPressed & IN_ATTACK) != 0)
			pPlayer->m_iSpawnProtect = 120;
		else
			pPlayer->m_iSpawnProtect++;
	}

	if (pPlayer->m_iSpawnProtect == 120)
	{
		pPlayer->pev->rendermode = kRenderNormal;
		pev->renderfx &= ~kRenderFxGlowShell;
		pPlayer->pev->flags &= ~FL_GODMODE;
		pPlayer->m_iSpawnProtect = 121;
	}

	if (pPlayer)
		pPlayer->PostThink();
}



// orig 0x97e60
void ParmsNewLevel()
{
}


// orig 0x97e70
void ParmsChangeLevel()
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA* pSaveData = (SAVERESTOREDATA*)gpGlobals->pSaveData;

	if (pSaveData)
		pSaveData->connectionCount = BuildChangeList(pSaveData->levelList, MAX_LEVEL_CONNECTIONS);
}

static std::vector<std::string> g_MapsToLoad;

static void LoadNextMap()
{
	const std::string mapName = std::move(g_MapsToLoad.back());
	g_MapsToLoad.pop_back();

	pmove->Con_Printf("Loading map \"%s\" automatically (%d left)\n",
		mapName.c_str(), static_cast<int>(g_MapsToLoad.size() + 1));

	if (g_MapsToLoad.empty())
	{
		pmove->Con_Printf("Loading last map\n");
		g_MapsToLoad.shrink_to_fit();
	}

	SERVER_COMMAND(UTIL_VarArgs("map \"%s\"\n", mapName.c_str()));
}

static void LoadAllMaps()
{
	if (!g_MapsToLoad.empty())
	{
		pmove->Con_Printf("Already loading all maps (%d remaining)\nUse sv_stop_loading_all_maps to stop\n",
			static_cast<int>(g_MapsToLoad.size()));
		return;
	}

	FileFindHandle_t handle = FILESYSTEM_INVALID_FIND_HANDLE;

	const char* fileName = g_pFileSystem->FindFirst("maps/*.bsp", &handle);

	if (fileName != nullptr)
	{
		do
		{
			std::string mapName = fileName;
			mapName.resize(mapName.size() - 4);

			if (std::find_if(g_MapsToLoad.begin(), g_MapsToLoad.end(), [=](const auto& candidate)
					{ return 0 == stricmp(candidate.c_str(), mapName.c_str()); }) == g_MapsToLoad.end())
			{
				g_MapsToLoad.push_back(std::move(mapName));
			}
		} while ((fileName = g_pFileSystem->FindNext(handle)) != nullptr);

		g_pFileSystem->FindClose(handle);

		// Sort in reverse order so the first map in alphabetic order is loaded first.
		std::sort(g_MapsToLoad.begin(), g_MapsToLoad.end(), [](const auto& lhs, const auto& rhs)
			{ return rhs < lhs; });
	}

	if (!g_MapsToLoad.empty())
	{
		if (CMD_ARGC() == 2)
		{
			const char* firstMapToLoad = CMD_ARGV(1);

			// Clear out all maps that would have been loaded before this one.
			if (auto it = std::find(g_MapsToLoad.begin(), g_MapsToLoad.end(), firstMapToLoad);
				it != g_MapsToLoad.end())
			{
				const std::size_t numberOfMapsToSkip = g_MapsToLoad.size() - (it - g_MapsToLoad.begin());

				g_MapsToLoad.erase(it + 1, g_MapsToLoad.end());

				pmove->Con_Printf("Skipping %d maps to start with \"%s\"\n",
					static_cast<int>(numberOfMapsToSkip), g_MapsToLoad.back().c_str());
			}
			else
			{
				pmove->Con_Printf("Unknown map \"%s\", starting from beginning\n", firstMapToLoad);
			}
		}

		pmove->Con_Printf("Loading %d maps one at a time to generate files\n", static_cast<int>(g_MapsToLoad.size()));

		// Load the first map right now.
		LoadNextMap();
	}
	else
	{
		pmove->Con_Printf("No maps to load\n");
	}
}

void InitMapLoadingUtils()
{
	g_engfuncs.pfnAddServerCommand("sv_load_all_maps", &LoadAllMaps);
	// Escape hatch in case the command is executed in error.
	g_engfuncs.pfnAddServerCommand("sv_stop_loading_all_maps", []()
		{ g_MapsToLoad.clear(); });
}

static bool g_LastAllowBunnyHoppingState = false;

// orig 0x97eb0. Oz: a connected client's player, or NULL. GetClassPtr makes one for a client edict
// without private data, as the original does.
CBasePlayer* CBasePlayerByIndex(int playerIndex)
{
	CBasePlayer* pPlayer = NULL;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t* pPlayerEdict = INDEXENT(playerIndex);
		if (pPlayerEdict && !pPlayerEdict->free && (pPlayerEdict->v.flags & (FL_CLIENT | FL_FAKECLIENT)) != 0)
			pPlayer = GetClassPtr((CBasePlayer*)&pPlayerEdict->v);
	}

	return pPlayer;
}

//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
// orig 0x97f20. OZ: RhoBot, run before the game rules: bot thinking, nav marking under live humans,
// min_clients/bot_number upkeep, then the "bot" cvar's commands (once a second). Kept from
// halflife-updated: the "bj" physics key (sv_allowbunnyhopping, reference/DEVIATIONS.md) and the
// sv_load_all_maps helper, both only while the game isn't over.
void StartFrame()
{
	static float check_server_cmd = 0.0;
	static char cmd_line[80];
	static char server_cmd[80];
	static int index;

	const int iBotNumber = (int)bot_number.value;
	int iPlayers = 0;
	int iBots = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = CBasePlayerByIndex(i);
		if (pPlayer && pPlayer->IsPlayer())
		{
			iPlayers++;
			if (pPlayer->IsBot())
			{
				iBots++;
				pPlayer->BotThink();
			}
			else if (!pPlayer->IsBot() && pPlayer->IsNetClient() && pPlayer->IsAlive())
			{
				WorldGraph.MarkLocationFavorable(pPlayer->pev->origin);
			}
		}
	}

	if ((oz_minclients.value == 0 && iBotNumber == 0) || gpGlobals->time < 10.0)
	{
		bInitBotCycle = 0;
	}
	else if (iBots == 0 && bInitBotCycle == 0)
	{
		TimeNextPossibleBotConnect = 10.0;
		bInitBotCycle = UseBotCycle(0);
		bInitBotCycle = 1;
	}
	else if (((iBots < oz_minclients.value - (iPlayers - iBots) && iPlayers < gpGlobals->maxClients - 1 && iBots < 32) || iBots < iBotNumber) && TimeNextPossibleBotConnect <= gpGlobals->time)
	{
		TimeNextPossibleBotConnect = gpGlobals->time + 10.0;
		BotConnect(iBots);
	}

	if ((iBots > iBotNumber && iPlayers == gpGlobals->maxClients) || (iPlayers > oz_minclients.value && iBots > iBotNumber))
	{
		for (int i = gpGlobals->maxClients; i > 0; i--)
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
			if (pPlayer && pPlayer->IsBot())
			{
				pPlayer->pev->health = 0;
				sprintf(server_cmd, "kick \"%s\"\n", STRING(pPlayer->pev->netname));
				SERVER_COMMAND(server_cmd);
				bInitBotCycle = UseBotCycle(--iBots);
				break;
			}
		}
	}

	BotGlobals.HandleAutoAdjDifficulty();

	if (g_pGameRules)
	{
		g_pGameRules->Think();

		// the "bot" cvar: addbot, removebot <name>, removebots
		if (fabs(check_server_cmd - gpGlobals->time) > 1.0)
		{
			check_server_cmd = gpGlobals->time + 1.0;

			const char* cvar_bot = CVAR_GET_STRING("bot");
			if (cvar_bot && cvar_bot[0])
			{
				char* cmd;
				char* arg1 = NULL;

				strcpy(cmd_line, cvar_bot); // unchecked, as the original (see the dossier)
				index = 0;
				cmd = cmd_line;

				// skip to blank or end of string...
				while (cmd_line[index] != ' ' && cmd_line[index] != 0)
					index++;

				if (cmd_line[index] == ' ')
				{
					cmd_line[index++] = 0;
					arg1 = &cmd_line[index];

					// removebot takes the rest of the line (a name may have spaces); the rest split two more words
					if (0 != strcmp(cmd, "removebot"))
					{
						while (cmd_line[index] != ' ' && cmd_line[index] != 0)
							index++;

						if (cmd_line[index] == ' ')
						{
							cmd_line[index++] = 0;

							while (cmd_line[index] != ' ' && cmd_line[index] != 0)
								index++;

							if (cmd_line[index] == ' ')
								cmd_line[index++] = 0;
						}
					}
				}

				if (0 == strcmp(cmd, "addbot"))
				{
					int iCount = 0;
					for (int i = 1; i <= gpGlobals->maxClients; i++)
					{
						CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);
						if (pPlayer && pPlayer->IsPlayer())
							iCount++;
					}

					if (iCount > 31)
					{
						UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "Rho-Bot supports a maximum of 32 players.\n");
						if (IS_DEDICATED_SERVER())
							printf("Rho-Bot supports a maximum of 32 players.\n");
						return; // the original returns here, before clearing "bot" (see the dossier)
					}

					char msg[128] = "bot_number ###";
					sprintf(msg, "bot_number %d\n", (int)bot_number.value + 1);
					SERVER_COMMAND(msg);
					if (IS_DEDICATED_SERVER())
						printf("%s", msg);
				}
				else if (0 == strcmp(cmd, "removebot"))
				{
					if (arg1)
					{
						CBaseEntity* pBot = UTIL_FindEntityByString(NULL, "netname", arg1);
						char msg[128] = "kick Some Bot\n";
						if (!pBot)
						{
							sprintf(msg, "could not kick \"%s\"\n", arg1);
							UTIL_ClientPrintAll(HUD_PRINTNOTIFY, msg);
						}
						else
						{
							pBot->pev->health = 0;
							sprintf(server_cmd, "kick \"%s\"\n", STRING(pBot->pev->netname));
							SERVER_COMMAND(server_cmd);

							strcpy(msg, "bot_number ###");
							sprintf(msg, "bot_number %d\n", (int)bot_number.value - 1);
							SERVER_COMMAND(msg);
						}
						if (IS_DEDICATED_SERVER())
							printf("%s", msg);
					}
				}
				else if (0 == strcmp(cmd, "removebots"))
				{
					char msg[128] = "bot_number ###";
					sprintf(msg, "bot_number %d\n", 0);
					SERVER_COMMAND(msg);
					if (IS_DEDICATED_SERVER())
						printf("%s", msg);
				}
			}

			CVAR_SET_STRING("bot", "");
		}
	}

	if (g_fGameOver)
	{
		check_server_cmd = 0;
		return;
	}

	gpGlobals->teamplay = teamplay.value;
	g_ulFrameCount++;

	const bool allowBunnyHopping = sv_allowbunnyhopping.value != 0;

	if (allowBunnyHopping != g_LastAllowBunnyHoppingState)
	{
		g_LastAllowBunnyHoppingState = allowBunnyHopping;

		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto player = UTIL_PlayerByIndex(i);

			if (!player)
			{
				continue;
			}

			g_engfuncs.pfnSetPhysicsKeyValue(player->edict(), "bj", UTIL_dtos1(allowBunnyHopping ? 1 : 0));
		}
	}

	// If we're loading all maps then change maps after 3 seconds (time starts at 1)
	// to give the game time to generate files.
	if (!g_MapsToLoad.empty() && gpGlobals->time > 4)
	{
		LoadNextMap();
	}
}


// orig 0x98870. OZ adds ambience/particle_suck1.wav. plats/vehicle_ignition.wav stays for
// halflife-updated's func_vehicle, which this build still has (the original has neither).
void ClientPrecache()
{
	// setup precaches always needed
	PRECACHE_SOUND("player/sprayer.wav"); // spray paint sound for PreAlpha

	// PRECACHE_SOUND("player/pl_jumpland2.wav");		// UNDONE: play 2x step sound

	PRECACHE_SOUND("player/pl_fallpain2.wav");
	PRECACHE_SOUND("player/pl_fallpain3.wav");

	PRECACHE_SOUND("player/pl_step1.wav"); // walk on concrete
	PRECACHE_SOUND("player/pl_step2.wav");
	PRECACHE_SOUND("player/pl_step3.wav");
	PRECACHE_SOUND("player/pl_step4.wav");

	PRECACHE_SOUND("common/npc_step1.wav"); // NPC walk on concrete
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");

	PRECACHE_SOUND("player/pl_metal1.wav"); // walk on metal
	PRECACHE_SOUND("player/pl_metal2.wav");
	PRECACHE_SOUND("player/pl_metal3.wav");
	PRECACHE_SOUND("player/pl_metal4.wav");

	PRECACHE_SOUND("player/pl_dirt1.wav"); // walk on dirt
	PRECACHE_SOUND("player/pl_dirt2.wav");
	PRECACHE_SOUND("player/pl_dirt3.wav");
	PRECACHE_SOUND("player/pl_dirt4.wav");

	PRECACHE_SOUND("player/pl_duct1.wav"); // walk in duct
	PRECACHE_SOUND("player/pl_duct2.wav");
	PRECACHE_SOUND("player/pl_duct3.wav");
	PRECACHE_SOUND("player/pl_duct4.wav");

	PRECACHE_SOUND("player/pl_grate1.wav"); // walk on grate
	PRECACHE_SOUND("player/pl_grate2.wav");
	PRECACHE_SOUND("player/pl_grate3.wav");
	PRECACHE_SOUND("player/pl_grate4.wav");

	PRECACHE_SOUND("player/pl_slosh1.wav"); // walk in shallow water
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");

	PRECACHE_SOUND("player/pl_tile1.wav"); // walk on tile
	PRECACHE_SOUND("player/pl_tile2.wav");
	PRECACHE_SOUND("player/pl_tile3.wav");
	PRECACHE_SOUND("player/pl_tile4.wav");
	PRECACHE_SOUND("player/pl_tile5.wav");

	PRECACHE_SOUND("player/pl_swim1.wav"); // breathe bubbles
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");

	PRECACHE_SOUND("player/pl_ladder1.wav"); // climb ladder rung
	PRECACHE_SOUND("player/pl_ladder2.wav");
	PRECACHE_SOUND("player/pl_ladder3.wav");
	PRECACHE_SOUND("player/pl_ladder4.wav");

	PRECACHE_SOUND("player/pl_wade1.wav"); // wade in water
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");

	PRECACHE_SOUND("debris/wood1.wav"); // hit wood texture
	PRECACHE_SOUND("debris/wood2.wav");
	PRECACHE_SOUND("debris/wood3.wav");

	PRECACHE_SOUND("plats/train_use1.wav"); // use a train
	PRECACHE_SOUND("plats/vehicle_ignition.wav");

	PRECACHE_SOUND("buttons/spark5.wav"); // hit computer texture
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/glass1.wav");
	PRECACHE_SOUND("debris/glass2.wav");
	PRECACHE_SOUND("debris/glass3.wav");

	PRECACHE_SOUND(SOUND_FLASHLIGHT_ON);
	PRECACHE_SOUND(SOUND_FLASHLIGHT_OFF);

	// player gib sounds
	PRECACHE_SOUND("common/bodysplat.wav");

	// player pain sounds
	PRECACHE_SOUND("player/pl_pain2.wav");
	PRECACHE_SOUND("player/pl_pain4.wav");
	PRECACHE_SOUND("player/pl_pain5.wav");
	PRECACHE_SOUND("player/pl_pain6.wav");
	PRECACHE_SOUND("player/pl_pain7.wav");

	PRECACHE_MODEL("models/player.mdl");

	// hud sounds

	PRECACHE_SOUND("common/wpn_hudoff.wav");
	PRECACHE_SOUND("common/wpn_hudon.wav");
	PRECACHE_SOUND("common/wpn_moveselect.wav");
	PRECACHE_SOUND("common/wpn_select.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");


	// geiger sounds

	PRECACHE_SOUND("player/geiger6.wav");
	PRECACHE_SOUND("player/geiger5.wav");
	PRECACHE_SOUND("player/geiger4.wav");
	PRECACHE_SOUND("player/geiger3.wav");
	PRECACHE_SOUND("player/geiger2.wav");
	PRECACHE_SOUND("player/geiger1.wav");

	if (giPrecacheGrunt)
		UTIL_PrecacheOther("monster_human_grunt");

	PRECACHE_SOUND("ambience/particle_suck1.wav"); // Oz
}

/*
===============
GetGameDescription

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
// orig 0x98e40
const char* GetGameDescription()
{
	if (g_pGameRules) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Oz_DM 3.0";
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
// orig 0x98e90
void Sys_Error(const char* error_string)
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
// orig 0x98ea0
void PlayerCustomization(edict_t* pEntity, customization_t* pCust)
{
	entvars_t* pev = &pEntity->v;
	CBasePlayer* pPlayer = (CBasePlayer*)GET_PRIVATE(pEntity);

	if (!pPlayer)
	{
		ALERT(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		ALERT(at_console, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2); // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		ALERT(at_console, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
// orig 0x98f40
void SpectatorConnect(edict_t* pEntity)
{
	entvars_t* pev = &pEntity->v;
	CBaseSpectator* pPlayer = (CBaseSpectator*)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorConnect();
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
// orig 0x98f80
void SpectatorDisconnect(edict_t* pEntity)
{
	entvars_t* pev = &pEntity->v;
	CBaseSpectator* pPlayer = (CBaseSpectator*)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorDisconnect();
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
// orig 0x98fc0
void SpectatorThink(edict_t* pEntity)
{
	entvars_t* pev = &pEntity->v;
	CBaseSpectator* pPlayer = (CBaseSpectator*)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorThink();
}

////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
// orig 0x99000. The original's SDK 2.3 code sees from the observed player (Valve's 2013 seed dropped it).
void SetupVisibility(edict_t* pViewEntity, edict_t* pClient, unsigned char** pvs, unsigned char** pas)
{
	Vector org;
	edict_t* pView = pClient;

	// Find the client's PVS
	if (pViewEntity)
	{
		pView = pViewEntity;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pClient);
	if (pPlayer && pPlayer->pev->iuser2 && pPlayer->m_hObserverTarget)
	{
		pView = pPlayer->m_hObserverTarget->edict();
	}

	if ((pClient->v.flags & FL_PROXY) != 0)
	{
		*pvs = NULL; // the spectator proxy sees
		*pas = NULL; // and hears everything
		return;
	}

	org = pView->v.origin + pView->v.view_ofs;
	if ((pView->v.flags & FL_DUCKING) != 0)
	{
		org = org + (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
	}

	*pvs = ENGINE_SET_PVS((float*)&org);
	*pas = ENGINE_SET_PAS((float*)&org);
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
// orig 0x991a0. halflife-updated's kept: the MAX_EDICTS test (fix), the FL_FLY/m_EFlags SLERP rule
// (halflife#3228; decided with mod-rc2's AddToFullPack, reference FIXES.md). Left out: the HL25
// EFLAG_FLESH_SOUND flag (a feature, as mod-rockcrowbar).
int AddToFullPack(struct entity_state_s* state, int e, edict_t* ent, edict_t* host, int hostflags, int player, unsigned char* pSet)
{
	// Entities with an index greater than this will corrupt the client's heap because 
	// the index is sent with only 11 bits of precision (2^11 == 2048).
	// So we don't send them, just like having too many entities would result
	// in the entity not being sent.
	if (e >= MAX_EDICTS)
	{
		return 0;
	}

	int i;

	auto entity = reinterpret_cast<CBaseEntity*>(GET_PRIVATE(ent));

	// don't send if flagged for NODRAW and it's not the host getting the message
	if ((ent->v.effects & EF_NODRAW) != 0 &&
		(ent != host))
		return 0;

	// Ignore ents without valid / visible models
	if (0 == ent->v.modelindex || !STRING(ent->v.model))
		return 0;

	// Don't send spectators to other players
	if ((ent->v.flags & FL_SPECTATOR) != 0 && (ent != host))
	{
		return 0;
	}

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if (ent != host)
	{
		if (!ENGINE_CHECK_VISIBILITY((const struct edict_s*)ent, pSet))
		{
			return 0;
		}
	}


	// Don't send entity to local client if the client says it's predicting the entity itself.
	if ((ent->v.flags & FL_SKIPLOCALHOST) != 0)
	{
		if ((hostflags & 1) != 0 && (ent->v.owner == host))
			return 0;
	}

	if (0 != host->v.groupinfo)
	{
		UTIL_SetGroupTrace(host->v.groupinfo, GROUP_OP_AND);

		// Should always be set, of course
		if (0 != ent->v.groupinfo)
		{
			if (g_groupop == GROUP_OP_AND)
			{
				if ((ent->v.groupinfo & host->v.groupinfo) == 0)
					return 0;
			}
			else if (g_groupop == GROUP_OP_NAND)
			{
				if ((ent->v.groupinfo & host->v.groupinfo) != 0)
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	memset(state, 0, sizeof(*state));

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number = e;
	state->entityType = ENTITY_NORMAL;

	// Flag custom entities.
	if ((ent->v.flags & FL_CUSTOMENTITY) != 0)
	{
		state->entityType = ENTITY_BEAM;
	}

	//
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime = (int)(1000.0 * ent->v.animtime) / 1000.0;

	memcpy(state->origin, ent->v.origin, 3 * sizeof(float));
	memcpy(state->angles, ent->v.angles, 3 * sizeof(float));
	memcpy(state->mins, ent->v.mins, 3 * sizeof(float));
	memcpy(state->maxs, ent->v.maxs, 3 * sizeof(float));

	memcpy(state->startpos, ent->v.startpos, 3 * sizeof(float));
	memcpy(state->endpos, ent->v.endpos, 3 * sizeof(float));

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;

	state->frame = ent->v.frame;

	state->skin = ent->v.skin;
	state->effects = ent->v.effects;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	/*
	if (0 == player &&
		0 != ent->v.animtime &&
		ent->v.velocity[0] == 0 &&
		ent->v.velocity[1] == 0 &&
		ent->v.velocity[2] == 0)
	{
		state->eflags |= EFLAG_SLERP;
	}
	*/

	if ((ent->v.flags & FL_FLY) != 0)
	{
		state->eflags |= EFLAG_SLERP;
	}
	else
	{
		state->eflags &= ~EFLAG_SLERP;
	}

	state->eflags |= entity->m_EFlags;

	state->scale = ent->v.scale;
	state->solid = ent->v.solid;
	state->colormap = ent->v.colormap;

	state->movetype = ent->v.movetype;
	state->sequence = ent->v.sequence;
	state->framerate = ent->v.framerate;
	state->body = ent->v.body;

	for (i = 0; i < 4; i++)
	{
		state->controller[i] = ent->v.controller[i];
	}

	for (i = 0; i < 2; i++)
	{
		state->blending[i] = ent->v.blending[i];
	}

	state->rendermode = ent->v.rendermode;
	state->renderamt = ent->v.renderamt;
	state->renderfx = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor.x;
	state->rendercolor.g = ent->v.rendercolor.y;
	state->rendercolor.b = ent->v.rendercolor.z;

	state->aiment = 0;
	if (ent->v.aiment)
	{
		state->aiment = ENTINDEX(ent->v.aiment);
	}

	state->owner = 0;
	if (ent->v.owner)
	{
		int owner = ENTINDEX(ent->v.owner);

		// Only care if owned by a player
		if (owner >= 1 && owner <= gpGlobals->maxClients)
		{
			state->owner = owner;
		}
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	if (0 == player)
	{
		state->playerclass = ent->v.playerclass;
	}

	// Special stuff for players only
	if (0 != player)
	{
		memcpy(state->basevelocity, ent->v.basevelocity, 3 * sizeof(float));

		state->weaponmodel = MODEL_INDEX(STRING(ent->v.weaponmodel));
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction = ent->v.friction;

		state->gravity = ent->v.gravity;
		//		state->team			= ent->v.team;
		//
		state->usehull = (ent->v.flags & FL_DUCKING) != 0 ? 1 : 0;
		state->health = ent->v.health;
	}

	return 1;
}

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
// orig 0x99710
void CreateBaseline(int player, int eindex, struct entity_state_s* baseline, struct edict_s* entity, int playermodelindex, Vector* player_mins, Vector* player_maxs)
{
	baseline->origin = entity->v.origin;
	baseline->angles = entity->v.angles;
	baseline->frame = entity->v.frame;
	baseline->skin = (short)entity->v.skin;

	// render information
	baseline->rendermode = (byte)entity->v.rendermode;
	baseline->renderamt = (byte)entity->v.renderamt;
	baseline->rendercolor.r = (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g = (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b = (byte)entity->v.rendercolor.z;
	baseline->renderfx = (byte)entity->v.renderfx;

	if (0 != player)
	{
		baseline->mins = *player_mins;
		baseline->maxs = *player_maxs;

		baseline->colormap = eindex;
		baseline->modelindex = playermodelindex;
		baseline->friction = 1.0;
		baseline->movetype = MOVETYPE_WALK;

		baseline->scale = entity->v.scale;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1.0;
		baseline->gravity = 1.0;
	}
	else
	{
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;

		baseline->colormap = 0;
		baseline->modelindex = entity->v.modelindex; //SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype = entity->v.movetype;

		baseline->scale = entity->v.scale;
		baseline->solid = entity->v.solid;
		baseline->framerate = entity->v.framerate;
		baseline->gravity = entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int field;
} entity_field_alias_t;

#define FIELD_ORIGIN0 0
#define FIELD_ORIGIN1 1
#define FIELD_ORIGIN2 2
#define FIELD_ANGLES0 3
#define FIELD_ANGLES1 4
#define FIELD_ANGLES2 5

static entity_field_alias_t entity_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
		{"angles[0]", 0},
		{"angles[1]", 0},
		{"angles[2]", 0},
};

// orig 0x99920
void Entity_FieldInit(struct delta_s* pFields)
{
	entity_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN0].name);
	entity_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN1].name);
	entity_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN2].name);
	entity_field_alias[FIELD_ANGLES0].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES0].name);
	entity_field_alias[FIELD_ANGLES1].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES1].name);
	entity_field_alias[FIELD_ANGLES2].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES2].name);
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network. 
FIXME:  Move to script
==================
*/
// orig 0x999e0
void Entity_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	entity_state_t *f, *t;
	static bool initialized = false;

	if (!initialized)
	{
		Entity_FieldInit(pFields);
		initialized = true;
	}

	f = (entity_state_t*)from;
	t = (entity_state_t*)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	const bool localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();
	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->impacttime != 0) && (t->starttime != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

static entity_field_alias_t player_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
};

// orig 0x99bf0
void Player_FieldInit(struct delta_s* pFields)
{
	player_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN0].name);
	player_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN1].name);
	player_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN2].name);
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network. 
==================
*/
// orig 0x99c60
void Player_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	entity_state_t *f, *t;
	static bool initialized = false;

	if (!initialized)
	{
		Player_FieldInit(pFields);
		initialized = true;
	}

	f = (entity_state_t*)from;
	t = (entity_state_t*)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	const bool localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();
	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

#define CUSTOMFIELD_ORIGIN0 0
#define CUSTOMFIELD_ORIGIN1 1
#define CUSTOMFIELD_ORIGIN2 2
#define CUSTOMFIELD_ANGLES0 3
#define CUSTOMFIELD_ANGLES1 4
#define CUSTOMFIELD_ANGLES2 5
#define CUSTOMFIELD_SKIN 6
#define CUSTOMFIELD_SEQUENCE 7
#define CUSTOMFIELD_ANIMTIME 8

entity_field_alias_t custom_entity_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
		{"angles[0]", 0},
		{"angles[1]", 0},
		{"angles[2]", 0},
		{"skin", 0},
		{"sequence", 0},
		{"animtime", 0},
};

// orig 0x99dd0
void Custom_Entity_FieldInit(struct delta_s* pFields)
{
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].name);
	custom_entity_field_alias[CUSTOMFIELD_SKIN].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].name);
	custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].name);
	custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].name);
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
// orig 0x99f20
void Custom_Encode(struct delta_s* pFields, const unsigned char* from, const unsigned char* to)
{
	entity_state_t *f, *t;
	int beamType;
	static bool initialized = false;

	if (!initialized)
	{
		Custom_Entity_FieldInit(pFields);
		initialized = true;
	}

	f = (entity_state_t*)from;
	t = (entity_state_t*)to;

	beamType = t->rendermode & 0x0f;

	if (beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field);
	}

	if (beamType != BEAM_POINTS)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field);
	}

	if (beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field);
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ((int)f->animtime == (int)t->animtime)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field);
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
// orig 0x9a0c0
void RegisterEncoders()
{
	DELTA_ADDENCODER("Entity_Encode", Entity_Encode);
	DELTA_ADDENCODER("Custom_Encode", Custom_Encode);
	DELTA_ADDENCODER("Player_Encode", Player_Encode);
}

// orig 0x9a130
int GetWeaponData(struct edict_s* player, struct weapon_data_s* info)
{
	memset(info, 0, MAX_WEAPONS * sizeof(weapon_data_t));

#if defined(CLIENT_WEAPONS)
	int i;
	weapon_data_t* item;
	entvars_t* pev = &player->v;
	CBasePlayer* pl = dynamic_cast<CBasePlayer*>(CBasePlayer::Instance(pev));
	CBasePlayerWeapon* gun;

	ItemInfo II;

	if (!pl)
		return 1;

	// go through all of the weapons and make a list of the ones to pack
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (pl->m_rgpPlayerItems[i])
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem* pPlayerItem = pl->m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				gun = pPlayerItem->GetWeaponPtr();
				if (gun && gun->UseDecrement())
				{
					// Get The ID.
					memset(&II, 0, sizeof(II));
					gun->GetItemInfo(&II);

					if (II.iId >= 0 && II.iId < MAX_WEAPONS)
					{
						item = &info[II.iId];

						item->m_iId = II.iId;
						item->m_iClip = gun->m_iClip;

						item->m_flTimeWeaponIdle = V_max(gun->m_flTimeWeaponIdle, -0.001f);
						item->m_flNextPrimaryAttack = V_max(gun->m_flNextPrimaryAttack, -0.001f);
						item->m_flNextSecondaryAttack = V_max(gun->m_flNextSecondaryAttack, -0.001f);
						item->m_fInReload = static_cast<int>(gun->m_fInReload);
						item->m_fInSpecialReload = gun->m_fInSpecialReload;
						item->fuser1 = V_max(gun->pev->fuser1, -0.001f);
						item->fuser2 = gun->m_flStartThrow;
						item->fuser3 = gun->m_flReleaseThrow;
						item->iuser1 = gun->m_chargeReady;
						item->iuser2 = gun->m_fInAttack;
						item->iuser3 = gun->m_fireState;

						gun->GetWeaponData(*item);

						//						item->m_flPumpTime				= V_max( gun->m_flPumpTime, -0.001f );
					}
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}
#endif
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
// orig 0x9a370. OZ sends the spectator iuser1/iuser2 only with oz_advspec 1. Valve's 2013 first-person
// spectating (the chased player's data while OBS_IN_EYE) is left out as a feature. Kept: the fov from
// m_iFOV (halflife-updated's 6f8aac9 removed pev->fov; same value) and this repo's m_WeaponBits sync.
void UpdateClientData(const edict_t* ent, int sendweapons, struct clientdata_s* cd)
{
	if (!ent || !ent->pvPrivateData)
		return;
	entvars_t* pev = (entvars_t*)&ent->v;
	CBasePlayer* pl = dynamic_cast<CBasePlayer*>(CBasePlayer::Instance(pev));

	cd->flags = pev->flags;
	cd->health = pev->health;

	cd->viewmodel = MODEL_INDEX(STRING(pev->viewmodel));

	cd->waterlevel = pev->waterlevel;
	cd->watertype = pev->watertype;
	// halflife-updated stores the player's weapon bitmask in
	// CBasePlayer::m_WeaponBits (std::uint64_t) and ships the bits to its
	// own cl_dll via the custom gmsgWeapons message rather than the
	// engine's per-frame client_data.iWeaponBits delta. Vanilla HL clients
	// (i.e., a stock anniversary client connecting to this server) don't
	// have the matching cl_dll, so they only see cd->weapons / iWeaponBits.
	// Without this sync, pev->weapons stays 0, the WEAPON_SUIT bit (31)
	// never reaches the vanilla HUD, and the HEV display never appears.
	// Replicate the lower 32 bits of m_WeaponBits into cd->weapons so the
	// legacy channel carries WEAPON_SUIT and weapons 0..30 to vanilla
	// clients. The halflife-updated cl_dll ignores cdata->iWeaponBits
	// (see cl_dll/hud_update.cpp:38), so updated clients are unaffected.
	if (pl)
		cd->weapons = static_cast<int>(pl->m_WeaponBits & 0xFFFFFFFF);
	else
		cd->weapons = pev->weapons;

	// Vectors
	cd->origin = pev->origin;
	cd->velocity = pev->velocity;
	cd->view_ofs = pev->view_ofs;
	cd->punchangle = pev->punchangle;

	cd->bInDuck = pev->bInDuck;
	cd->flTimeStepSound = pev->flTimeStepSound;
	cd->flDuckTime = pev->flDuckTime;
	cd->flSwimTime = pev->flSwimTime;
	cd->waterjumptime = pev->teleport_time;

	strcpy(cd->physinfo, ENGINE_GETPHYSINFO(ent));

	cd->fov = pev->fov; // the original's (orig 0x9a370): OZ's zooms set pev->fov; halflife-updated's m_iFOV (6f8aac9) is a singleplayer save fix, design here
	cd->fov = pl->m_iFOV;
	cd->weaponanim = pev->weaponanim;

	cd->pushmsec = pev->pushmsec;

	//Spectator mode
	if (oz_advspec.value == 1) // Oz
	{
		cd->iuser1 = pev->iuser1;
		cd->iuser2 = pev->iuser2;
	}



#if defined(CLIENT_WEAPONS)
	if (0 != sendweapons)
	{
		if (pl)
		{
			cd->m_flNextAttack = pl->m_flNextAttack;
			cd->fuser2 = pl->m_flNextAmmoBurn;
			cd->fuser3 = pl->m_flAmmoStartCharge;
			cd->vuser1.x = pl->ammo_9mm;
			cd->vuser1.y = pl->ammo_357;
			cd->vuser1.z = pl->ammo_argrens;
			cd->ammo_nails = pl->ammo_bolts;
			cd->ammo_shells = pl->ammo_buckshot;
			cd->ammo_rockets = pl->ammo_rockets;
			cd->ammo_cells = pl->ammo_uranium;
			cd->vuser2.x = pl->ammo_hornets;


			if (pl->m_pActiveItem)
			{
				CBasePlayerWeapon* gun = pl->m_pActiveItem->GetWeaponPtr();
				if (gun && gun->UseDecrement())
				{
					ItemInfo II;
					memset(&II, 0, sizeof(II));
					gun->GetItemInfo(&II);

					cd->m_iId = II.iId;

					cd->vuser3.z = gun->m_iSecondaryAmmoType;
					cd->vuser4.x = gun->m_iPrimaryAmmoType;
					cd->vuser4.y = pl->m_rgAmmo[gun->m_iPrimaryAmmoType];
					cd->vuser4.z = pl->m_rgAmmo[gun->m_iSecondaryAmmoType];

					if (pl->m_pActiveItem->m_iId == WEAPON_RPG)
					{
						cd->vuser2.y = static_cast<vec_t>(((CRpg*)pl->m_pActiveItem)->m_fSpotActive);
						cd->vuser2.z = ((CRpg*)pl->m_pActiveItem)->m_cActiveRockets;
					}
				}
			}
		}
	}
#endif
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
// orig 0x9a6b0
void CmdStart(const edict_t* player, const struct usercmd_s* cmd, unsigned int random_seed)
{
	entvars_t* pev = (entvars_t*)&player->v;
	CBasePlayer* pl = dynamic_cast<CBasePlayer*>(CBasePlayer::Instance(pev));

	if (!pl)
		return;

	if (pl->pev->groupinfo != 0)
	{
		UTIL_SetGroupTrace(pl->pev->groupinfo, GROUP_OP_AND);
	}

	pl->random_seed = random_seed;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
// orig 0x9a730
void CmdEnd(const edict_t* player)
{
	entvars_t* pev = (entvars_t*)&player->v;
	CBasePlayer* pl = dynamic_cast<CBasePlayer*>(CBasePlayer::Instance(pev));

	if (!pl)
		return;
	if (pl->pev->groupinfo != 0)
	{
		UTIL_UnsetGroupTrace();
	}
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
// orig 0x9a790
int ConnectionlessPacket(const struct netadr_s* net_from, const char* args, char* response_buffer, int* response_buffer_size)
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
// orig 0x9a7b0. halflife-updated's fix, kept (c3c5576, halflife#3229): SDK 2.3's assigns the vectors
// to its own pointer copies and never fills mins/maxs.
int GetHullBounds(int hullnumber, float* mins, float* maxs)
{
	return static_cast<int>(PM_GetHullBounds(hullnumber, mins, maxs));
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
// orig 0x9a890
void CreateInstancedBaselines()
{
	int iret = 0;
	entity_state_t state;

	memset(&state, 0, sizeof(state));

	// Create any additional baselines here for things like grendates, etc.
	// iret = ENGINE_INSTANCE_BASELINE( pc->pev->classname, &state );

	// Destroy objects.
	//UTIL_Remove( pc );
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
// orig 0x9a8d0
int InconsistentFile(const edict_t* player, const char* filename, char* disconnect_message)
{
	// Server doesn't care?
	if (CVAR_GET_FLOAT("mp_consistency") != 1)
		return 0;

	// Default behavior is to kick the player
	sprintf(disconnect_message, "Server is enforcing file consistency for %s\n", filename);

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too, 
  if you want.
================================
*/
// orig 0x9a940. OZ returns 0 (no lag compensation); halflife-updated's seed returns 1.
int AllowLagCompensation()
{
	return 0;
}

// orig 0x975f0. Oz: the held rune's name at the bottom of the screen (not in observer mode).
void CBasePlayer::UpdateRuneInfo()
{
	CBasePlayer* pPlayer = GetClassPtr((CBasePlayer*)pev);

	if (pPlayer->m_iObserver != 1)
	{
		hudtextparms_t hText;

		memset(&hText, 0, sizeof(hText));
		hText.channel = 1;
		hText.x = 0.46;
		hText.y = 0.98;
		hText.effect = 0;
		hText.r1 = 210;
		hText.g1 = 120;
		hText.b1 = 25;
		hText.a1 = 10;
		hText.r2 = 210;
		hText.g2 = 120;
		hText.b2 = 25;
		hText.a2 = 10;
		hText.fadeinTime = 0;
		hText.fadeoutTime = 0.5;
		hText.holdTime = 999;
		hText.fxTime = 0;
		UTIL_HudMessage(pPlayer, hText, GetRuneName(pPlayer->m_iRune));
	}

	pPlayer->m_iRuneHudTime = 0;
}
