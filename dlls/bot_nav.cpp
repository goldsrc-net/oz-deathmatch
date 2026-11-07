// Oz Deathmatch 3.02: bot_nav.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).
//
// The bots' navigation array: CGraph::m_NavArray (nodes.h), a short per 32-unit cell of the
// +-4096 world. 1 is the neutral value; StartFrame (client.cpp) adds 1 every frame where each live
// human player stands, ThinkStart subtracts 1 in front of a bot that is stuck, and roaming bots
// pick neighbouring cells weighted by their values. The array lives in WorldGraph, so it goes
// to and from the .nod file with the rest of CGraph (FSaveGraph/FLoadGraph copy sizeof(CGraph)),
// and CHalfLifeMultiplay::ChangeLevel writes it to <runfolder>/nav/<map>.nav as "x.y.z=value".

#include <fstream>

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "nodes.h"
#include "game.h"
#include "bot.h"

//=========================================================
// ActionChooseDirection: pick one of the 8 neighbouring cells at random, weighted by their
// propensity, and return its DIRECTION bits; DIRECTION_ALL when they are all 0.
//=========================================================
int CBaseBot::ActionChooseDirection() // orig 0x08c7f0
{
	const int x = WorldGraph.FConvertGlobalToArray(pev->origin.x);
	const int y = WorldGraph.FConvertGlobalToArray(pev->origin.y);
	const int z = WorldGraph.FConvertGlobalToArray(pev->origin.z);

	int iTotal = WorldGraph.FGetNavPropensity(x + 1, y + 1, z) + WorldGraph.FGetNavPropensity(x + 1, y, z) + WorldGraph.FGetNavPropensity(x + 1, y - 1, z) + WorldGraph.FGetNavPropensity(x, y + 1, z) + WorldGraph.FGetNavPropensity(x, y - 1, z) + WorldGraph.FGetNavPropensity(x - 1, y + 1, z) + WorldGraph.FGetNavPropensity(x - 1, y, z) + WorldGraph.FGetNavPropensity(x - 1, y - 1, z);

	if (iTotal == 0)
		return DIRECTION_ALL;

	int iPick;
	if (iTotal == 1)
		iPick = 1;
	else
		iPick = RANDOM_LONG(1, iTotal);

	iTotal = (iTotal - 1) - WorldGraph.FGetNavPropensity(x + 1, y, z);
	if (iTotal < iPick)
		return DIRECTION_XPLUS;
	iTotal -= WorldGraph.FGetNavPropensity(x + 1, y - 1, z);
	if (iTotal < iPick)
		return DIRECTION_XPLUS | DIRECTION_YMINUS;
	iTotal -= WorldGraph.FGetNavPropensity(x, y - 1, z);
	if (iTotal < iPick)
		return DIRECTION_YMINUS;
	iTotal -= WorldGraph.FGetNavPropensity(x - 1, y - 1, z);
	if (iTotal < iPick)
		return DIRECTION_XMINUS | DIRECTION_YMINUS;
	iTotal -= WorldGraph.FGetNavPropensity(x - 1, y, z);
	if (iTotal < iPick)
		return DIRECTION_XMINUS;
	iTotal -= WorldGraph.FGetNavPropensity(x - 1, y + 1, z);
	if (iTotal < iPick)
		return DIRECTION_XMINUS | DIRECTION_YPLUS;
	iTotal -= WorldGraph.FGetNavPropensity(x, y + 1, z);
	if (iTotal < iPick)
		return DIRECTION_YPLUS;
	if (iTotal - WorldGraph.FGetNavPropensity(x + 1, y + 1, z) < iPick)
		return DIRECTION_XPLUS | DIRECTION_YPLUS;

	return DIRECTION_ALL;
}

float CGraph::FConvertArrayToGlobal(int iIndex) // orig 0x08cb30
{
	return ((float)iIndex + 0.5) * 32.0 - 4096.0;
}

int CGraph::FConvertGlobalToArray(float flCoord) // orig 0x08cb70
{
	return BoundsCheck((int)((flCoord + 4096.0) * 0.03125));
}

int CGraph::BoundsCheck(int iIndex) // orig 0x08cbc0
{
	if (iIndex < 0)
		iIndex = 0;
	else if (iIndex > 255)
		iIndex = 255;
	return iIndex;
}

// Out-of-range and negative cells count as 0.
int CGraph::FGetNavPropensity(int x, int y, int z) // orig 0x08cbe0
{
	if (x == BoundsCheck(x) && y == BoundsCheck(y) && z == BoundsCheck(z))
	{
		if (m_NavArray[x][y][z] >= 0)
			return m_NavArray[x][y][z];
	}
	return 0;
}

// Nothing in 3.02 calls this. The format string ends in a space, so on Linux the file never opens
// and nothing is read (orig).
bool CGraph::FLoadBotNav(char* pszMapName) // orig 0x08cc60
{
	char szFile[50] = "default";

	sprintf(szFile, "%s/nav/%s.nav ", CVAR_GET_STRING("runfolder"), pszMapName);
	ALERT(at_console, szFile);
	slurp_array(WorldGraph.m_NavArray, szFile);
	ALERT(at_console, "NavigationArray Loaded.\n");
	return true;
}

// Lines longer than 24 characters fail the read and end the loop (orig).
void CGraph::slurp_array(short (*pArray)[256][256], char* pszFile) // orig 0x08cd20
{
	std::ifstream file(pszFile);
	char szLine[25];

	while (file.getline(szLine, 25, '\n'))
		parse_nav_line(pArray, szLine);

	file.close();
}

// Copy *ppszSrc into pszDest up to cDelim or the end of the string, or up to cStop, then step
// past the delimiter (not past cStop).
void nextItem(char* pszDest, char** ppszSrc, char cDelim, char cStop) // orig 0x08d4e0
{
	while (**ppszSrc != cDelim && **ppszSrc != '\0')
	{
		if (**ppszSrc == cStop)
		{
			*pszDest = '\0';
			return;
		}
		*pszDest++ = **ppszSrc;
		(*ppszSrc)++;
	}

	if (**ppszSrc != cStop && strlen(*ppszSrc) != 0)
		(*ppszSrc)++;

	*pszDest = '\0';
}

// "x.y.z=value". The buffers hold 3, 3, 3 and 7 characters and the indices are not range-checked
// (orig); the shipped nav files stay within both. A trailing '\r' (the shipped files are CRLF)
// stays in the value, which atoi ignores.
void CGraph::parse_nav_line(short (*pArray)[256][256], char* pszLine) // orig 0x08cea0
{
	char szX[4];
	char szY[4];
	char szZ[4];
	char szValue[8];

	nextItem(szX, &pszLine, '.', '\0');
	nextItem(szY, &pszLine, '.', '\0');
	nextItem(szZ, &pszLine, '=', '\0');
	nextItem(szValue, &pszLine, '\n', '\0');

	const int x = atoi(szX);
	const int y = atoi(szY);
	const int z = atoi(szZ);
	pArray[x][y][z] = atoi(szValue);
}

// Halve every cell that isn't 1; called when a cell would overflow the short.
void CGraph::HalfNavArray() // orig 0x08d550
{
	for (int x = 0; x < 256; x++)
	{
		for (int y = 0; y < 256; y++)
		{
			for (int z = 0; z < 256; z++)
			{
				if (m_NavArray[x][y][z] != 1)
					m_NavArray[x][y][z] = m_NavArray[x][y][z] / 2;
			}
		}
	}
}

void CGraph::InitNavArray() // orig 0x08d5e0
{
	for (int x = 0; x < 256; x++)
	{
		for (int y = 0; y < 256; y++)
		{
			for (int z = 0; z < 256; z++)
				m_NavArray[x][y][z] = 1;
		}
	}
}

bool CGraph::IsInWorld(Vector& vecOrigin) // orig 0x08d650
{
	if (vecOrigin.x <= 4096.0 && vecOrigin.y <= 4096.0 && vecOrigin.z <= 4096.0 && vecOrigin.x >= -4096.0 && vecOrigin.y >= -4096.0 && vecOrigin.z >= -4096.0)
		return true;
	return false;
}

void CGraph::MarkLocationFavorable(Vector& vecOrigin) // orig 0x08d6f0
{
	if (!IsInWorld(vecOrigin))
		return;

	const int x = FConvertGlobalToArray(vecOrigin.x);
	const int y = FConvertGlobalToArray(vecOrigin.y);
	const int z = FConvertGlobalToArray(vecOrigin.z);

	if (m_NavArray[x][y][z] == 32767)
		HalfNavArray();
	else
		m_NavArray[x][y][z]++;
}

void CGraph::MarkLocationUnfavorable(Vector& vecOrigin) // orig 0x08d7a0
{
	if (!IsInWorld(vecOrigin))
		return;

	const int x = FConvertGlobalToArray(vecOrigin.x);
	const int y = FConvertGlobalToArray(vecOrigin.y);
	const int z = FConvertGlobalToArray(vecOrigin.z);

	if (m_NavArray[x][y][z] < -32766)
		HalfNavArray();
	else
		m_NavArray[x][y][z]--;
}

// Writes every cell that isn't 1. fopen isn't checked (orig): without a <runfolder>/nav directory
// fprintf gets NULL and the server crashes at the map change.
void CGraph::SaveNavToFile() // orig 0x08d850
{
	char szFile[50] = "default";

	sprintf(szFile, "%s/nav/%s.nav", CVAR_GET_STRING("runfolder"), STRING(gpGlobals->mapname));
	FILE* pFile = fopen(szFile, "w");

	char szLine[25] = "error";
	for (int x = 0; x < 256; x++)
	{
		for (int y = 0; y < 256; y++)
		{
			for (int z = 0; z < 256; z++)
			{
				if (WorldGraph.m_NavArray[x][y][z] != 1)
				{
					sprintf(szLine, "%d.%d.%d=%d\n", x, y, z, (int)WorldGraph.m_NavArray[x][y][z]);
					fprintf(pFile, "%s", szLine); // orig fprintf(pFile, szLine); the line has no '%'
				}
			}
		}
	}

	fclose(pFile);
}
