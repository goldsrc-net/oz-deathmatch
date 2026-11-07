// Oz Deathmatch 3.02: bot_ai.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).
//
// RhoBot's think. StartFrame (client.cpp) calls BotThink on every bot each frame; a full think
// runs every GetBotThinkDelay seconds (0.1 to 0.4, shorter with reflexes) and RunPlayerMove runs
// every frame with the last moves.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "effects.h"
#include "nodes.h"
#include "game.h"
#include "bot.h"

// Built when the library loads, before GameDLLInit registers the max_* cvars, so every iMaxCarry
// is 0 and CanHaveAmmo never lets a bot want ammo (orig).
botammocheck_t ammo_check[10] = { // orig 0x085df0 (_GLOBAL_.I.)
	{"ammo_9mmclip", "9mm", (int)oz_max9mm.value},
	{"ammo_9mmAR", "9mm", (int)oz_max9mm.value},
	{"ammo_ARgrenades", "ARgrenades", (int)oz_maxm203.value},
	{"ammo_buckshot", "buckshot", (int)oz_maxshells.value},
	{"ammo_crossbow", "bolts", (int)oz_maxxbowbolts.value},
	{"ammo_357", "357", (int)oz_max357rounds.value},
	{"ammo_rpgclip", "rockets", (int)oz_maxrockets.value},
	{"ammo_egonclip", "uranium", (int)oz_maxuranium.value},
	{"ammo_gaussclip", "uranium", (int)oz_maxuranium.value},
	{"", NULL, 0},
};

// m_rgAmmo[iIndex] as the original reads it. A weapon without that ammo has index -1, and the
// original then reads the word before m_rgAmmo in its CBasePlayer, m_pLastItem (+0x4d0).
// VERIFY: emulates the original's out-of-bounds read; our layout differs.
static int BotAmmoCount(CBasePlayer* pPlayer, int iIndex)
{
	if (iIndex == -1)
		return pPlayer->m_pLastItem != NULL ? 1 : 0;
	return pPlayer->m_rgAmmo[iIndex];
}

CBaseBot::CBaseBot() // orig 0x0832b0
{
	m_fCalledAimThisFrame = 0;
	m_vecDesiredVelocity = g_vecZero;
	m_fGoUpOnLadder = 1;
	m_vecLookAtBiasVec = g_vecZero;
	m_vecLookAtVec = g_vecZero;
	m_flMovedDistance = 0;
	m_flMoveForward = 0;
	m_flMoveStrafe = 0;
	m_flMoveVertical = 0;
	m_flMSec = 0;
	m_iMSecCounter = 0;
	m_iRoamDirection = DIRECTION_ALL;
	m_flTimeGoalCheck = gpGlobals->time;
	m_flTimeGoalCheckDelay = 0.3;
	m_flTimeMSecCheck = gpGlobals->time;
	m_iTurningDirection = TURNING_NONE;
	m_fWantToBeInCombat = 0;
	m_hEnemy = NULL;
	m_hGoal = NULL;
	FightStyle.m_pBot = this;
	// m_flNextBotThink, m_iSteerCallCounter and m_iSteerCallPrecedence are not set (orig).
}

// Fire when the fight style's window is open; reload an empty clip; choose a weapon again when
// the window has closed.
void CBaseBot::ActionOpenFire() // orig 0x083560
{
	if (!m_pActiveItem || !m_pActiveItem->CanDeploy() || FightStyle.m_flStopShootTime <= gpGlobals->time)
	{
		ActionChooseWeapon();
		return;
	}

	CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)m_pActiveItem;
	const ItemInfo& info = CBasePlayerItem::ItemInfoArray[pWeapon->m_iId];

	if (info.pszAmmo1 && pWeapon->m_iClip <= 0 && info.iMaxClip != -1)
	{
		ActionReload();
		return;
	}

	if (FightStyle.m_flNextShootTime > gpGlobals->time)
		return;
	if (pWeapon->m_fInReload)
		return;

	if (0 != FightStyle.m_fSecondaryFire)
	{
		if (!info.pszAmmo1 || info.iMaxClip == -1 || BotAmmoCount(this, pWeapon->m_iSecondaryAmmoType) != 0)
		{
			pev->button |= IN_ATTACK2;
			return;
		}
	}
	else
	{
		if (info.pszAmmo2 && info.iMaxClip != -1 && BotAmmoCount(this, pWeapon->m_iPrimaryAmmoType) == 0)
		{
			pev->button |= IN_ATTACK2;
			return;
		}
	}

	pev->button |= IN_ATTACK;
}

// Walk the m_pLink list ActionLook built and make the best item the goal: desire times
// (256 - distance) cubed. Without a decent weapon only weapons count, else ammo, weapons,
// health, armour, the long jump and weaponboxes.
void CBaseBot::ActionChooseGoal() // orig 0x083700
{
	float flBest = 0;

	CBaseEntity* pEntity = m_pLink;
	while (pEntity)
	{
		CBasePlayerItem* pItem = (CBasePlayerItem*)pEntity;
		float flDesire = 0;
		const char* pszClassname = STRING(pEntity->pev->classname);

		if (!CheckHasDecentWeapon())
		{
			// "weapon" also matches "weaponbox", whose m_pPlayer is read from another member
			// (orig). VERIFY: our CWeaponBox layout differs.
			if ((pEntity->pev->effects & EF_NODRAW) == 0 && CheckVisible(pEntity) && 0 == strncmp("weapon", pszClassname, 6))
			{
				if (pItem->m_pPlayer)
					continue; // orig: without stepping to the next one, so this never ends
				if (g_pGameRules->CanHavePlayerItem(this, pItem))
					flDesire = 100.0;
			}
		}
		else if ((pEntity->pev->effects & EF_NODRAW) == 0 && CheckVisible(pEntity))
		{
			if (0 == strncmp("ammo_", pszClassname, 5))
			{
				for (int i = 0; ammo_check[i].pszClassname[0] != '\0'; i++)
				{
					if (0 == strcmp(ammo_check[i].pszClassname, pszClassname) && g_pGameRules->CanHaveAmmo(this, ammo_check[i].pszAmmoName, ammo_check[i].iMaxCarry))
					{
						flDesire = 1.0;
						break;
					}
				}
			}
			else if (0 == strncmp("weapon_", pszClassname, 7))
			{
				if (pItem->m_pPlayer)
					continue; // orig: never ends, as above
				if (g_pGameRules->CanHavePlayerItem(this, pItem))
					flDesire = Stats.FindWeaponDesire(pItem, 8192.0);
			}
			else if (0 == strcmp(pszClassname, "item_healthkit"))
				flDesire = 100.0 - pev->health;
			else if (0 == strcmp(pszClassname, "weaponbox"))
				flDesire = 80.0;
			else if (0 == strcmp(pszClassname, "item_battery"))
				flDesire = 100.0 - pev->armorvalue;
			else if (0 == strcmp(pszClassname, "item_longjump"))
				flDesire = 1.0;
		}

		if (flDesire > 0)
		{
			const float flNear = 256.0 - (pEntity->pev->origin - pev->origin).Length();
			flDesire = flNear * flNear * flNear * flDesire;
			if (flDesire > flBest)
			{
				m_hGoal = pEntity;
				flBest = flDesire;
			}
		}

		pEntity = pEntity->m_pLink;
	}
}

// Switch to the carried weapon with the best random share of its desire at the enemy's
// distance, then set up its fight style.
void CBaseBot::ActionChooseWeapon() // orig 0x083b60
{
	if (!m_pActiveItem || !m_pActiveItem->CanHolster())
		return;

	CBasePlayerItem* pBest = NULL;
	int iBest = 0;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		for (CBasePlayerItem* pItem = m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext)
		{
			float flDistance = 8192.0;
			if (m_hEnemy != NULL)
				flDistance = (m_hEnemy->pev->origin - pev->origin).Length();

			const float flDesire = Stats.FindWeaponDesire(pItem, flDistance);
			const float flScore = RANDOM_FLOAT(0, 1) * flDesire;
			if (flScore > iBest && pItem->CanDeploy())
			{
				iBest = (int)flScore;
				pBest = pItem;
			}
		}
	}

	if (pBest && pev)
		SwitchWeapon(pBest);

	FightStyle.DispatchWeaponUse(m_pActiveItem);
}

// Pick the nearest noticed enemy (unless the current one was seen recently), steer with
// teammates, then go over everything within iDistance: dodge projectiles, lasers and tripmines,
// press buttons, and list the rest in m_pLink for ActionChooseGoal.
void CBaseBot::ActionLook(int iDistance) // orig 0x083d20
{
	float flNearest = 8192.0;

	if (m_hEnemy == NULL || (Stats.m_flTraitAggression * 3.0) / 100.0 < gpGlobals->time - Memory.m_flEnemyLastSeenTime)
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
			if (!pPlayer || pPlayer == this)
				continue;
			if (!pPlayer->IsAlive() || !CheckVisible(pPlayer) || !FInViewCone(pPlayer))
				continue;
			if (pPlayer->pev->flags == FL_SPECTATOR) // orig: the whole flags word, not the bit
				continue;
			if (!ActionCheckMercy(pPlayer) || !ActionCheckCloak(pPlayer))
				continue;
			if (pPlayer->IsNetClient() && bot_observer.value != 0)
				continue;

			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_NOTTEAMMATE && CheckNotice(pPlayer))
			{
				const float flDist = (pPlayer->pev->origin - pev->origin).Length();
				if (flDist < flNearest)
				{
					SetEnemy(pPlayer);
					flNearest = flDist;
				}
			}
			else if (!pPlayer->IsBot() && g_pGameRules->PlayerRelationship(this, pPlayer) != GR_NOTTEAMMATE)
			{
				// A human teammate: follow close, else catch up.
				if ((pPlayer->pev->origin - pev->origin).Length() < 128.0)
					SteerSafeGroupFollow(pPlayer, STEER_FOLLOW);
				else
					SteerSafePursue(pPlayer, STEER_TEAMMATE, Vector(0, 0, 0));
			}
			else if (pPlayer->IsBot() && g_pGameRules->PlayerRelationship(this, pPlayer) != GR_NOTTEAMMATE)
			{
				// A bot teammate: flock close, else catch up.
				if ((pPlayer->pev->origin - pev->origin).Length() < 128.0)
					SteerSafeGroupFlock(STEER_FLOCK);
				else
					SteerSafePursue(pPlayer, STEER_TEAMMATE, Vector(0, 0, 0));
			}
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

	m_pLink = NULL;

	const Vector vecRange = Vector(iDistance, iDistance, iDistance);
	const bool fNoGoal = m_hGoal == NULL;
	bool fCheckGoal = false;
	if (gpGlobals->time > m_flTimeGoalCheck)
	{
		fCheckGoal = true;
		m_flTimeGoalCheck = m_flTimeGoalCheckDelay + gpGlobals->time;
	}

	CBaseEntity* pList[100];
	const Vector vecMaxs = pev->origin + vecRange;
	const Vector vecMins = pev->origin - vecRange;
	const int iCount = UTIL_EntitiesInBox(pList, 100, vecMins, vecMaxs, 0);

	for (int i = 0; i < iCount; i++)
	{
		CBaseEntity* pEntity = pList[i];
		if (!pEntity || pEntity == this)
			continue;
		if (!CheckVisible(pEntity) || !CheckNotice(pEntity))
			continue;

		const float flDist = (pEntity->pev->origin - pev->origin).Length();
		const char* pszClassname = STRING(pEntity->pev->classname);

		if (0 == strcmp(pszClassname, "monster_satchel") || 0 == strcmp(pszClassname, "monster_snark") || 0 == strcmp(pszClassname, "hornet") || 0 == strcmp(pszClassname, "rpg_rocket") || 0 == strcmp(pszClassname, "grenade"))
		{
			SteerSafeEvade(pEntity, STEER_DANGER);
		}
		else if (0 == strcmp(pszClassname, "laser_spot"))
		{
			SteerSafeFlee(pEntity, STEER_DANGER);
		}
		else if (0 == strcmp(pszClassname, "beam"))
		{
			const Vector vecStart = ((CBeam*)pEntity)->GetStartPos();
			if ((vecStart - pev->origin).Length() < 375.0)
				SteerSafeFlee(vecStart, STEER_DANGER);
		}
		else if (0 == strcmp(pszClassname, "monster_tripmine"))
		{
			if (flDist <= 375.0)
			{
				SteerSafeFlee(pEntity, STEER_DANGER);
			}
			else if (m_hEnemy == NULL)
			{
				// Shoot it from afar.
				AimAtEntity(pEntity);
				pev->button = IN_ATTACK;
			}
			else if ((pEntity->pev->origin - m_hEnemy->pev->origin).Length() < 375.0)
			{
				// Shoot it when the enemy is near it.
				AimAtEntity(pEntity);
			}
		}
		else if (0 == strcmp(pszClassname, "func_button"))
		{
			if (flDist < 64.0)
				pev->button |= IN_USE;
		}
		else if (fNoGoal && fCheckGoal)
		{
			pEntity->m_pLink = m_pLink;
			m_pLink = pEntity;
		}
	}
}

// Reload when the clip isn't full.
bool CBaseBot::ActionReload() // orig 0x084850
{
	CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)m_pActiveItem;
	if (pWeapon)
	{
		const ItemInfo& info = CBasePlayerItem::ItemInfoArray[pWeapon->m_iId];
		if (info.pszAmmo1 && info.iMaxClip != -1 && pWeapon->m_iClip != info.iMaxClip)
		{
			pev->button |= IN_RELOAD;
			return true;
		}
	}
	return false;
}

void CBaseBot::ActionSpeak(const char* pszText) // orig 0x0848c0
{
	char szText[256];

	sprintf(szText, "%s: %s\n", STRING(pev->netname), pszText);
	UTIL_SayTextAll(szText, this);
}

// True when the bot is dead or the game is over, after doing what it does then: while dead,
// press fire half the time to respawn.
bool CBaseBot::CheckBotKick() // orig 0x084920
{
	if (pev->takedamage == DAMAGE_NO)
	{
		pev->health = 0;
		pev->deadflag = DEAD_DEAD;
	}

	if (g_fGameOver)
	{
		CSound* pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
		if (pSound)
			pSound->Reset();
		ThinkDead();
		return true;
	}

	pev->button = 0;
	if (pev->health < 1.0 || pev->deadflag != DEAD_NO || !IsAlive())
	{
		HandleTime();
		ThinkDead();
		pev->button = 0;
		if (RANDOM_LONG(1, 100) > 50)
			pev->button = IN_ATTACK;

		(*g_engfuncs.pfnRunPlayerMove)(edict(), pev->v_angle, m_flMoveForward, m_flMoveStrafe, m_flMoveVertical, pev->button, 0, (byte)(long long)m_flMSec);
		return true;
	}

	return false;
}

float CBaseBot::GetBotThinkDelay() // orig 0x084ac0
{
	return 0.1 + 0.003 * (100.0 - Stats.FDifficultyAdjusted(Stats.m_flTraitReflexes));
}

void CBaseBot::BotThink() // orig 0x084b20
{
	if (CheckBotKick())
		return;

	if (m_flNextBotThink < gpGlobals->time)
	{
		ThinkStart();
		if (m_hEnemy != NULL)
			ThinkValidateEnemy();
		if (m_hGoal != NULL)
			ThinkValidateGoal();
		ActionLook(256);
		ActionChooseGoal();
		ThinkMood();
		ThinkSteering();
		if (0 == m_fCalledAimThisFrame)
			AimWander();
		HandleMovement();
		Memory.EndFrameUpdate(this);
		m_flNextBotThink = GetBotThinkDelay() + gpGlobals->time;
	}
	else
	{
		HandleTime();
	}

	(*g_engfuncs.pfnRunPlayerMove)(edict(), pev->v_angle, m_flMoveForward, m_flMoveStrafe, m_flMoveVertical, pev->button, 0, (byte)(long long)m_flMSec);
}

// Anything in weapon slots 1 to 3 (pistols, rifles, heavy).
bool CBaseBot::CheckHasDecentWeapon() // orig 0x084ca0
{
	for (int i = 1; i < 4; i++)
	{
		if (m_rgpPlayerItems[i])
			return true;
	}
	return false;
}

// Whether the bot notices pEntity: how much of it is visible, how near, how fast it moves and
// how close to the view direction, against the (difficulty-adjusted) perception.
bool CBaseBot::CheckNotice(CBaseEntity* pEntity) // orig 0x084ce0
{
	if (!FInViewCone(pEntity))
		return false;

	float flVisible = 0;
	if (FVisible(pEntity->pev->origin))
		flVisible = 0.3;
	if (FVisible(pEntity->Center()))
		flVisible = flVisible + 0.4;
	if (FVisible(pEntity->EyePosition()))
		flVisible = flVisible + 0.3;

	if (flVisible == 0)
		return false;

	const float flMiss = 1 - Stats.FDifficultyAdjusted(Stats.m_flTraitPerception) / 100.0;
	if (flMiss == 0)
		return true;

	const Vector vecTo = pEntity->Center() - EyePosition();
	float flDist = vecTo.Length();
	if (flDist > 8192.0)
		flDist = 8192.0;
	if (flDist < 32.0)
		flDist = 32.0;
	const float flNear = 8192.0 / (flDist * 16.0);

	const float flSpeed = pEntity->pev->velocity.Length() / CVAR_GET_FLOAT("sv_maxspeed");

	float flDot = 1.0;
	if (vecTo.Length() > 128.0)
	{
		UTIL_MakeVectors(pev->v_angle);
		flDot = DotProduct(gpGlobals->v_forward.Normalize(), vecTo.Normalize());
	}

	if (flVisible * flNear * flNear * (float)(flSpeed * (flSpeed + flSpeed + 1) + 1) * flDot <= flMiss)
		return false;
	return true;
}

bool CBaseBot::CheckVisible(CBaseEntity* pEntity) // orig 0x085060
{
	if (!pEntity || pEntity == this || !pEntity->pev)
		return false;
	return FVisible(pEntity);
}

// m_flMSec for RunPlayerMove: 450 over the frames counted in the last half second, 1 to 100.
void CBaseBot::HandleTime() // orig 0x0850b0
{
	if (m_flTimeMSecCheck <= gpGlobals->time)
	{
		m_flTimeMSecCheck = gpGlobals->time + 0.5;
		if (m_iMSecCounter > 0)
			m_flMSec = (int)(450 / (long long)m_iMSecCounter);
		m_iMSecCounter = 0;
	}
	else
	{
		m_iMSecCounter++;
	}

	if (m_flMSec < 1.0)
		m_flMSec = 1.0;
	if (m_flMSec > 100.0)
		m_flMSec = 100.0;
}

void CBaseBot::ThinkDead() // orig 0x085180
{
	m_vecDesiredVelocity = g_vecZero;
	m_fGoUpOnLadder = 1;
	m_vecLookAtVec = g_vecZero;
	m_flMovedDistance = 0;
	m_flMoveForward = 0;
	m_flMoveStrafe = 0;
	m_flMoveVertical = 0;
	SetEnemy(NULL);
	m_hGoal = NULL;
	m_iRoamDirection = DIRECTION_ALL;
	m_flTimeGoalCheck = gpGlobals->time;
	m_iTurningDirection = TURNING_NONE;
	m_fWantToBeInCombat = 1;
}

// Fight or flee. Reads m_pActiveItem without checking it (orig).
void CBaseBot::ThinkMood() // orig 0x0852b0
{
	CBasePlayerWeapon* pWeapon = (CBasePlayerWeapon*)m_pActiveItem;

	if (pWeapon->m_fInReload)
	{
		m_fWantToBeInCombat = 0;
		return;
	}

	if (0 == strcmp(STRING(m_pActiveItem->pev->classname), "weapon_crowbar"))
	{
		m_fWantToBeInCombat = 1;
		return;
	}

	if (m_hEnemy == NULL)
	{
		if (120.0 - Stats.m_flTraitAggression < pev->health + pev->armorvalue && CheckHasDecentWeapon())
		{
			m_fWantToBeInCombat = 1;
			return;
		}
	}
	else
	{
		CBasePlayer* pEnemy = (CBasePlayer*)(CBaseEntity*)m_hEnemy;
		if ((!pEnemy->m_pActiveItem || 0 != strcmp(STRING(pEnemy->m_pActiveItem->pev->classname), "weapon_crowbar")) && (Stats.m_flTraitAggression - 30.0) + pev->health + pev->armorvalue > 30.0)
		{
			m_fWantToBeInCombat = 1;
			return;
		}
	}

	m_fWantToBeInCombat = 0;
}

// Reset the think's state. A bot that hasn't moved since the last think marks the cell 32 units
// ahead of its move as unfavourable.
void CBaseBot::ThinkStart() // orig 0x0853e0
{
	HandleTime();
	m_fCalledAimThisFrame = 0;
	m_iSteerCallCounter = 0;
	m_iSteerCallPrecedence = STEER_WANDER;
	pev->button = 0;

	m_flMovedDistance = (Memory.m_vecLastOrigin - pev->origin).Length();

	// Never true, the precedence was just reset (orig).
	if (IsAlive() && m_iSteerCallPrecedence != STEER_WANDER)
		WorldGraph.MarkLocationFavorable(pev->origin);

	if (m_flMovedDistance <= 1.0 && m_vecDesiredVelocity.Length2D() > 0)
	{
		const Vector vecOrigin = pev->origin;
		UTIL_MakeVectors(pev->v_angle);
		const Vector vecMove = (gpGlobals->v_forward * m_flMoveForward + gpGlobals->v_right * m_flMoveStrafe).Normalize();

		float flMax = fabs(vecMove.x);
		if (!(fabs(vecMove.y) < flMax))
			flMax = fabs(vecMove.y);

		Vector vecAhead = vecOrigin + vecMove * (32.0 / flMax);
		WorldGraph.MarkLocationUnfavorable(vecAhead);
	}
}

// Fight (aim, fire, jump, duck, chat, chase) or run (evade, flee), or go for the goal or roam.
void CBaseBot::ThinkSteering() // orig 0x085690
{
	if (m_hEnemy != NULL && m_hEnemy->IsAlive())
	{
		if (!CheckVisible(m_hEnemy))
		{
			if (Memory.m_fEnemyInSight != 0 && !ActionReload())
				ActionChooseWeapon();
			Memory.EnemyOutOfSight();
		}
		else
		{
			AimAtEnemy();
			ActionOpenFire();
			Memory.EnemyInSight(m_hEnemy);
		}

		if (!CheckHasDecentWeapon())
		{
			if (RANDOM_FLOAT(1, 1000) < (bot_jump.value / 100.0) * Stats.m_flTraitJump)
				pev->button |= IN_JUMP;
		}
		else if (0 == m_fWantToBeInCombat)
		{
			if (RANDOM_FLOAT(1, 2000) < (bot_jump.value / 100.0) * Stats.m_flTraitJump)
			{
				switch (RANDOM_LONG(0, 3))
				{
				case 0:
				case 3:
					pev->button |= IN_JUMP;
					break;
				case 1:
				case 2:
					pev->button |= IN_DUCK;
					break;
				}
			}

			if (Memory.m_fEnemyInSight != 0)
				SteerSafeEvade(m_hEnemy, STEER_DANGER);
			else
				SteerSafeFlee(Memory.m_vecEnemyLastSeen, STEER_DANGER);
			return;
		}
		else
		{
			if (RANDOM_FLOAT(1, 4000) < (bot_jump.value / 100.0) * Stats.m_flTraitJump)
			{
				if (bot_dontchat.value == 0)
				{
					switch (RANDOM_LONG(0, 5))
					{
					case 0:
					case 2:
					case 4:
						pev->button |= IN_JUMP;
						break;
					case 1:
					case 5:
						pev->button |= IN_DUCK;
						break;
					case 3:
						ActionChat();
						break;
					}
				}
				else
				{
					switch (RANDOM_LONG(0, 3))
					{
					case 0:
					case 2:
					case 3:
						pev->button |= IN_JUMP;
						break;
					case 1:
						pev->button |= IN_DUCK;
						break;
					}
				}
			}

			if (Memory.m_fEnemyInSight != 0)
				SteerSafePursue(m_hEnemy, STEER_COMBAT, Vector(0, 0, 0));
			else
				SteerSafeSeek(Memory.m_vecEnemyLastSeen, STEER_COMBAT);
			return;
		}
	}
	else
	{
		ActionReload();
	}

	if (m_hGoal == NULL)
		SteerSafeWanderImproved(STEER_WANDER);
	else
		SteerSafeSeek(m_hGoal, STEER_GOAL);
}

// Keep the enemy while alive and not lost for longer than 10 s, more or less with aggression.
void CBaseBot::ThinkValidateEnemy() // orig 0x085ab0
{
	if (m_hEnemy != NULL && CheckVisible(m_hEnemy) && FInViewCone(m_hEnemy))
		Memory.EnemyInSight(m_hEnemy);

	if (m_hEnemy != NULL && m_hEnemy->pev->deadflag == DEAD_NO)
	{
		if (!((Stats.m_flTraitAggression - 50.0) / 10.0 + 10.0 < gpGlobals->time - Memory.m_flEnemyLastSeenTime))
			return;
	}

	SetEnemy(NULL);
}

void CBaseBot::ThinkValidateGoal() // orig 0x085bf0
{
	if (m_hGoal != NULL && CheckVisible(m_hGoal) && (m_hGoal->pev->effects & EF_NODRAW) == 0)
		return;

	m_hGoal = NULL;
}

// bot_mercy 1: a human on half the bot's frags or fewer is spared, and up to three quarters
// sometimes (30 in 100). Counted from frags + 1.
bool CBaseBot::ActionCheckMercy(CBasePlayer* pPlayer) // orig 0x085c60
{
	if (bot_mercy.value == 1.0 && pPlayer->IsNetClient())
	{
		const float flTheirs = pPlayer->pev->frags + 1.0;
		const float flMine = pev->frags + 1.0;
		if (flTheirs <= flMine * 0.5)
			return false;
		if (flMine * 0.5 < flTheirs && flTheirs <= flMine * 0.75 && RANDOM_FLOAT(1, 100) <= 30.0)
			return false;
	}
	return true;
}

// True when the player can be seen: not cloaked (renderamt 100 or less) or moving fast.
bool CBaseBot::ActionCheckCloak(CBasePlayer* pPlayer) // orig 0x085d50
{
	if (pPlayer->pev->renderamt > 100.0 || pPlayer->pev->velocity.Length() > 150.0)
		return true;
	return false;
}
