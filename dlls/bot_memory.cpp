// Oz Deathmatch 3.02: bot_memory.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "bot.h"

CBaseBotMemory::CBaseBotMemory() // orig 0x087880
{
	m_flEnemySightedTime = gpGlobals->time;
	m_fEnemyInSight = 0;
	m_vecEnemyLastSeen = g_vecZero;
	m_flEnemyLastSeenTime = gpGlobals->time;
	for (int i = 0; i < 4; i++)
	{
		m_History[i].vecEnemyVelocity = g_vecZero;
		m_History[i].vecLookAt = g_vecZero;
		m_History[i].vecVelocity = g_vecZero;
	}
	m_vecLastOrigin = g_vecZero;
}

void CBaseBotMemory::EnemyInSight(CBaseEntity* pEnemy) // orig 0x087a60
{
	m_fEnemyInSight = 1;
	m_flEnemyLastSeenTime = gpGlobals->time;
	m_vecEnemyLastSeen = pEnemy->pev->origin;
}

void CBaseBotMemory::EnemyOutOfSight() // orig 0x087ab0
{
	m_fEnemyInSight = 0;
}

// A new enemy: its velocity fills the whole history.
void CBaseBotMemory::EnemySighted(CBaseEntity* pEnemy) // orig 0x087ad0
{
	EnemyInSight(pEnemy);
	m_flEnemySightedTime = gpGlobals->time;
	m_History[0].vecEnemyVelocity = pEnemy->pev->velocity;
	m_History[1].vecEnemyVelocity = m_History[0].vecEnemyVelocity;
	m_History[2].vecEnemyVelocity = m_History[1].vecEnemyVelocity;
	m_History[3].vecEnemyVelocity = m_History[2].vecEnemyVelocity;
}

// End of a think: shift the history one frame and record this one.
void CBaseBotMemory::EndFrameUpdate(CBasePlayer* pPlayer) // orig 0x087ba0
{
	CBaseBot* pBot = (CBaseBot*)pPlayer;

	m_vecLastOrigin = pBot->pev->origin;

	m_History[3].vecLookAt = m_History[2].vecLookAt;
	m_History[2].vecLookAt = m_History[1].vecLookAt;
	m_History[1].vecLookAt = m_History[0].vecLookAt;
	m_History[0].vecLookAt = pBot->m_vecLookAtVec;

	m_History[3].vecVelocity = m_History[2].vecVelocity;
	m_History[2].vecVelocity = m_History[1].vecVelocity;
	m_History[1].vecVelocity = m_History[0].vecVelocity;
	m_History[0].vecVelocity = pBot->pev->velocity;

	if (pBot->m_hEnemy != NULL)
	{
		m_History[3].vecEnemyVelocity = m_History[2].vecEnemyVelocity;
		m_History[2].vecEnemyVelocity = m_History[1].vecEnemyVelocity;
		m_History[1].vecEnemyVelocity = m_History[0].vecEnemyVelocity;
		m_History[0].vecEnemyVelocity = ((CBaseEntity*)pBot->m_hEnemy)->pev->velocity;
	}
}
