// Oz Deathmatch 3.02: bot_misc.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).
//
// Creating bots, the bot cycle and skill auto-adjust. StartFrame (client.cpp) drives all of it:
// it calls UseBotCycle(0) once at the start of a map, BotConnect when bots are short, kicks a bot
// when there are too many, and calls BotGlobals.HandleAutoAdjDifficulty every frame.
//
// The bot cycle: botcycle.txt (opened relative to the server's working directory, so it sits in
// the half-life folder, not the mod's) lists one bot cfg name per line. UseBotCycle(n) execs
// bots/<line n>.cfg, which sets the bot_* cvars, and keeps the line in BotGlobals; BotConnect
// names the next bot after it and reads the cvars back through CBaseBotStats::ReadBotFile.

#include <fstream>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "bot.h"

extern CBasePlayer* CBasePlayerByIndex(int playerIndex); // client.cpp

int skin_used[16];
char bot_skins[16][64] = {
	"barney",
	"barney",
	"gina",
	"gordon",
	"gman",
	"gman",
	"helmet",
	"hgrunt",
	"nmgrunt",
	"recon",
	"recon",
	"robo",
	"scientist",
	"scientist",
	"zombie",
	"zombie",
};

int name_used[37];
char bot_names[37][64] = {
	"1000101",
	"Ben",
	"BoBoKiLLeR",
	"botman",
	"Carmack",
	"Chandler Bing",
	"Coffee",
	"crimrOw",
	"Darkblade",
	"DarkFig",
	"DeaDPooL",
	"Deltha O'Neal",
	"Eraserman",
	"Eric",
	"Grym Reaper",
	"gumby",
	"Oberon",
	"incognito",
	"Jackal",
	"J!mmyPop",
	"kastral",
	"kevlar",
	"Mace",
	"NARLOTEP",
	"Starbreaker",
	"Vacindak",
	"Oski",
	"phewl",
	"Scooter",
	"Professor Harvey",
	"Shpen",
	"Slippy",
	"Cutter",
	"Dr Deb",
	"Capt. John",
	"Commando Dad",
	"Lazarus Long",
};

CBaseBotGlobals BotGlobals;

CBaseBotGlobals::CBaseBotGlobals() // orig 0x087de0
{
	m_flNextAutoAdjTime = 12.0;
	strcpy(m_szMapName, "boot_camp");
	strcpy(m_szLastBotUsed, "NoBotLastUsed");
}

// Up 5 while below bot_aa_max.
void CBaseBotGlobals::IncreaseDifficulty() // orig 0x087e80
{
	char szCommand[128] = "bot_skill ###";

	if ((float)(int)bot_skill.value < bot_aa_max.value)
	{
		sprintf(szCommand, "bot_skill %d\n", (int)bot_skill.value + 5);
		SERVER_COMMAND(szCommand);
		if (IS_DEDICATED_SERVER())
			printf("%s", szCommand); // orig printf(szCommand)
	}
}

// Down 1, to 0 from below 2.
void CBaseBotGlobals::DecreaseDifficulty() // orig 0x087f70
{
	char szCommand[128] = "bot_skill ###";

	int iSkill;
	if ((int)bot_skill.value < 2)
		iSkill = 0;
	else
		iSkill = (int)bot_skill.value - 1;

	sprintf(szCommand, "bot_skill %d\n", iSkill);
	SERVER_COMMAND(szCommand);
	if (IS_DEDICATED_SERVER())
		printf("%s", szCommand); // orig printf(szCommand)
}

// Every 12 s with bot_skill_aa on: compare the bots' frags per death with the human players'
// (both start at 1/1) and nudge bot_skill towards the humans. The 12 s step is added to the last
// time, not to now, so after a long time with it off it fires every frame until it catches up.
void CBaseBotGlobals::HandleAutoAdjDifficulty() // orig 0x088050
{
	if ((int)bot_skill_aa.value == 0)
		return;
	if (!(m_flNextAutoAdjTime < gpGlobals->time))
		return;

	float flBotFrags = 1.0;
	float flBotDeaths = 1.0;
	float flHumanFrags = 1.0;
	float flHumanDeaths = 1.0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = CBasePlayerByIndex(i);
		if (!pPlayer)
			continue;

		if (pPlayer->IsBot())
		{
			flBotFrags += pPlayer->pev->frags;
			flBotDeaths += pPlayer->m_iDeaths;
		}
		else if (!pPlayer->IsBot() && pPlayer->IsNetClient())
		{
			flHumanFrags += pPlayer->pev->frags;
			flHumanDeaths += pPlayer->m_iDeaths;
		}
	}

	if (flHumanFrags / flHumanDeaths <= flBotFrags / flBotDeaths)
	{
		const float flBotRatio = flBotFrags / flBotDeaths;
		const float flHumanRatio = flHumanFrags / flHumanDeaths;
		if (flHumanRatio < flBotRatio)
			DecreaseDifficulty();
	}
	else
	{
		IncreaseDifficulty();
	}

	m_flNextAutoAdjTime = m_flNextAutoAdjTime + 12.0;
}

// A name from bot_names no other bot has, starting at a random one; all free again once all 37
// are used.
char* ChooseRandomName() // orig 0x0881d0
{
	int iName = RANDOM_LONG(0, 36);

	int i;
	for (i = 0; i < 37; i++)
	{
		if (name_used[i] == 0)
			break;
	}
	if (i == 37)
	{
		for (i = 0; i < 37; i++)
			name_used[i] = 0;
	}

	while (name_used[iName] == 1)
	{
		iName++;
		if (iName == 37)
			iName = 0;
	}

	name_used[iName] = 1;
	return bot_names[iName];
}

char* ChooseRandomSkin() // orig 0x088270
{
	int iSkin = RANDOM_LONG(0, 15);

	int i;
	for (i = 0; i < 16; i++)
	{
		if (skin_used[i] == 0)
			break;
	}
	if (i == 16)
	{
		for (i = 0; i < 16; i++)
			skin_used[i] = 0;
	}

	while (skin_used[iSkin] == 1)
	{
		iSkin++;
		if (iSkin == 16)
			iSkin = 0;
	}

	skin_used[iSkin] = 1;
	return bot_skins[iSkin];
}

// Exec bots/<line iLine of botcycle.txt>.cfg and remember the line; false when the file or the
// line is missing or the line is blank. The whole line is kept, trailing whitespace too: the
// shipped botcycle.txt is CRLF, so the name ends in '\r' (orig).
bool UseBotCycle(int iLine) // orig 0x088310
{
	std::ifstream file("botcycle.txt");
	char szLastBotUsed[100];
	char szLine[100];
	// The original's command buffer is 100 bytes, so a line over 82 characters overruns it in the
	// sprintf below. Ours fits the longest line getline gives (reference/DEVIATIONS.md, Crashes in
	// the original).
	char szCommand[128];

	strcpy(szLastBotUsed, BotGlobals.m_szLastBotUsed); // orig: copied and never used

	if (!file.is_open())
		return false;

	file.seekg(0, std::ios::beg);
	if (iLine != 0)
	{
		for (int i = 0; i < iLine; i++)
			file.getline(szLine, 100, '\n');
	}

	if (file.eof())
	{
		file.close();
		return false;
	}

	file.getline(szLine, 100, '\n');
	file.close();

	// Only checks that the line isn't blank; nothing is trimmed. (orig: reads szLine[-1] once i
	// reaches 0, and fails there whatever it finds)
	int i = strlen(szLine);
	while (i > 0 && 0 != isspace((unsigned char)szLine[i - 1]))
		i--;
	if (i <= 0)
		return false;

	strcpy(BotGlobals.m_szLastBotUsed, szLine);
	sprintf(szCommand, "exec \"bots/%s.cfg\"\n", BotGlobals.m_szLastBotUsed);
	SERVER_COMMAND(szCommand);
	return true;
}

// Spawn the new bot and set its name, model and colours: from the bot cycle when its cfg file
// exists, else its own name with a random skin and colours.
void BotClientPutInServer(edict_t* pEntity, int fUseBotCycle) // orig 0x0885e0
{
	char szName[64] = "gordon";
	char szModel[64] = "gordon";
	char szTopColor[4] = "255";
	char szBottomColor[4] = "255";

	CBaseBot* pBot = GetClassPtr((CBaseBot*)VARS(pEntity));
	pBot->Spawn();
	pEntity->v.flags |= FL_FAKECLIENT;

	CBaseBot* pPlayer = (CBaseBot*)CBaseEntity::Instance(pEntity);

	bool fFromFile = false;
	if (0 != fUseBotCycle)
	{
		// The original formats into 120 bytes, which a long runfolder with a long bot cycle line
		// overruns. Ours has room for the path (reference/DEVIATIONS.md, Crashes in the original).
		char szFile[256] = "default.cfg";
		snprintf(szFile, sizeof(szFile), "%s/bots/%s.cfg", CVAR_GET_STRING("runfolder"), BotGlobals.m_szLastBotUsed);
		if (pPlayer->Stats.ReadBotFile(szFile))
		{
			strcpy(szName, pPlayer->Stats.m_szName);
			strcpy(szModel, pPlayer->Stats.m_szModel);
			strcpy(szBottomColor, pPlayer->Stats.m_szBottomColor);
			strcpy(szTopColor, pPlayer->Stats.m_szTopColor);
			fFromFile = true;
		}
	}

	if (!fFromFile)
	{
		strcpy(szName, STRING(pEntity->v.netname));
		strcpy(szModel, ChooseRandomSkin());
		sprintf(szBottomColor, "%d", RANDOM_LONG(1, 255));
		sprintf(szTopColor, "%d", RANDOM_LONG(1, 255));
	}

	g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()), g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "name", szName);
	g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()), g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "model", szModel);
	g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()), g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "topcolor", szTopColor);
	g_engfuncs.pfnSetClientKeyValue(ENTINDEX(pPlayer->edict()), g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "bottomcolor", szBottomColor);

	g_pGameRules->PlayerThink(pPlayer); // orig: vtable slot 19 of CGameRules. VERIFY: PlayerThink, not PlayerSpawn
}

// Create bot number iBot (0 for the first): named after its bot cycle line, else a random name.
void BotConnect(int iBot) // orig 0x088990
{
	if (iBot == 0)
	{
		for (int i = 0; i < 16; i++)
			skin_used[i] = 0;
		for (int i = 0; i < 37; i++)
			name_used[i] = 0;
		strcpy(BotGlobals.m_szLastBotUsed, "NoBotLastUsed");
	}

	const bool fUseBotCycle = UseBotCycle(iBot);

	char szName[64] = "123456789012345678901234567890123456789012345678901234567890123";
	if (!fUseBotCycle)
		strncpy(szName, ChooseRandomName(), 63);
	else
		strncpy(szName, BotGlobals.m_szLastBotUsed, 63);

	// Drop control characters, non-ASCII and quotes. After a removal the next character is
	// skipped, so of two in a row only the first goes (orig).
	int iLength = strlen(szName);
	for (int i = 0; i < iLength; i++)
	{
		if ((unsigned char)(szName[i] - 0x20) > 0x5e || szName[i] == '"')
		{
			for (int j = i; j < iLength; j++)
				szName[j] = szName[j + 1];
			iLength--;
		}
	}

	edict_t* pEntity = (*g_engfuncs.pfnCreateFakeClient)(szName);
	if (FNullEnt(pEntity))
	{
		printf("FAILED TO CREATE BOT'%s'\n", szName);
		return;
	}

	if (pEntity->pvPrivateData != NULL)
		FREE_PRIVATE(pEntity);

	if (pEntity == NULL) // never true here (orig)
	{
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("There are no free client slots. Set maxplayers to a higher value.\n"));
		if (IS_DEDICATED_SERVER())
			printf("There are no free client slots. Set maxplayers to a higher value.\n");
		CVAR_SET_FLOAT("bot_number", bot_number.value - 1.0);
		return;
	}

	BotClientPutInServer(pEntity, fUseBotCycle);
	UseBotCycle(iBot + 1);
}

// Not virtual (the extra argument), and nothing calls it in 3.02: bots take damage through
// CBasePlayer::TakeDamage. Kept as the original has it.
bool CBaseBot::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType, CBasePlayer* pAttacker) // orig 0x088bc0
{
	CBaseEntity* pAttackerEntity = CBaseEntity::Instance(pevAttacker->pContainingEntity);

	const bool fTookDamage = CBasePlayer::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	if (pAttackerEntity != this && pAttackerEntity->IsPlayer() && ActionCheckMercy(pAttacker))
	{
		if (m_hEnemy == NULL || gpGlobals->time - Memory.m_flEnemyLastSeenTime > 1.0)
		{
			SetEnemy(NULL);
			m_vecLookAtVec = pAttackerEntity->pev->origin - pev->origin;

			if (pAttackerEntity->IsAlive() && g_pGameRules->PlayerRelationship(this, pAttackerEntity) == GR_NOTTEAMMATE)
			{
				SetWantToBeInCombat(1);
				ALERT(at_console, "SetWantToBeInCombat == TRUE9.\n");
				SetEnemy(pAttackerEntity);
			}

			if (m_hEnemy != NULL)
			{
				ActionChooseWeapon();
				const float x = RANDOM_FLOAT(-1, 1);
				const float y = RANDOM_FLOAT(-1, 1);
				const float z = RANDOM_FLOAT(-1, 1);
				m_vecLookAtBiasVec = Vector(x, y, z);
				Memory.EnemySighted(m_hEnemy);
			}
		}
	}

	return fTookDamage;
}
