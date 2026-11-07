// Oz Deathmatch 3.02: bot_chat.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "bot.h"

// One of 28 lines at random, said to everyone. ThinkSteering calls it now and then in combat
// when bot_dontchat is 0.
void CBaseBot::ActionChat() // orig 0x086730
{
	char szText[256];

	switch (RANDOM_LONG(0, 27))
	{
	case 0:
		strcpy(szText, "Bring It !");
		break;
	case 1:
		strcpy(szText, "Hey, Look up there!");
		break;
	case 2:
		strcpy(szText, "Mwahahaha");
		break;
	case 3:
		strcpy(szText, "This server ROX !");
		break;
	case 4:
		strcpy(szText, "Bots Rule!");
		break;
	case 5:
		strcpy(szText, "Who wants some??");
		break;
	case 6:
		strcpy(szText, "You know you are supposed to aim at me, right?");
		break;
	case 7:
		strcpy(szText, "I'm so gonna own you!");
		break;
	case 8:
		strcpy(szText, "Mess with the best, Die like the rest.");
		break;
	case 9:
		strcpy(szText, "Sompin smells funny...");
		break;
	case 10:
		strcpy(szText, "Wazzuppp??");
		break;
	case 11:
		strcpy(szText, "Sup G?");
		break;
	case 12:
		strcpy(szText, "Yes!");
		break;
	case 13:
		strcpy(szText, "Resistance is FUTILE!");
		break;
	case 14:
		strcpy(szText, "Who's your Daddy?");
		break;
	case 15:
		strcpy(szText, "I'm a BOT, You're NOT!");
		break;
	case 16:
		strcpy(szText, "Is my flashlite on?");
		break;
	case 17:
		strcpy(szText, "Where's the Beef");
		break;
	case 18:
		strcpy(szText, "Yo, check my Mad Bot Skillz!");
		break;
	case 19:
		strcpy(szText, "Come Get Some!");
		break;
	case 20:
		strcpy(szText, "Shhh... This won't hurt a bit...");
		break;
	case 21:
		strcpy(szText, "LOL");
		break;
	case 22:
		strcpy(szText, "Where's Style?");
		break;
	case 23:
		strcpy(szText, "Oz Deathmatch Rules!");
		break;
	case 24:
		strcpy(szText, "I love this game.");
		break;
	case 25:
		strcpy(szText, "Burn Baby!");
		break;
	case 26:
		strcpy(szText, "Ya Feel Lucky?");
		break;
	case 27:
		strcpy(szText, "Oz Rules!");
		break;
	}

	ActionSpeak(szText);
}
