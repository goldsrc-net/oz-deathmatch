// Oz Deathmatch 3.02: bot_move.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).
//
// Aiming and steering. Each think the steering calls fill m_vecDesiredVelocity: a SteerSafe* call
// replaces it when its STEER priority is higher than the one set so far this think, averages
// with it when equal, and is ignored when lower. HandleMovement then turns the desired velocity
// (averaged with the last 4 real ones) and the look direction into RunPlayerMove's moves.
//
// Several of the original's quirks stay: the group steering divides by the number of nearby
// players with no check for 0, SteerGroupSeparation seeks a point near the world origin,
// evade/pursue lead the target by velocity times distance, CheckPathObstructedSides treats its
// angles as a direction, and HandleObstacles clamps a turn to the wrong side. See the dossier.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "bot.h"

//=========================================================
// Aiming
//=========================================================

// Spread the aim by up to 62/62/87 units per 1000 of distance, less with accuracy, and keep a
// shrinking random part of the bias for the next think.
void CBaseBot::AimAtBias() // orig 0x088ef0
{
	const float flLength = m_vecLookAtVec.Length() / 1000.0;
	const float flSpread = (1 - Stats.FDifficultyAdjusted(Stats.m_flTraitAccuracy) / 100.0) * flLength;

	const float x = RANDOM_FLOAT(-62, 62) * flSpread;
	const float y = RANDOM_FLOAT(-62, 62) * flSpread;
	const float z = RANDOM_FLOAT(-87, 87) * flSpread;
	m_vecLookAtBiasVec = m_vecLookAtBiasVec + Vector(x, y, z);

	m_vecLookAtVec = m_vecLookAtVec + m_vecLookAtBiasVec;
	m_vecLookAtVec = m_vecLookAtVec.Normalize();

	const Vector vecBiasDir = m_vecLookAtBiasVec.Normalize();
	m_vecLookAtBiasVec = vecBiasDir * RANDOM_FLOAT(0, 1);
}

void CBaseBot::AimAtEnemy() // orig 0x089230
{
	if (0 != m_fCalledAimThisFrame)
		return;
	m_fCalledAimThisFrame = 1;

	if (FightStyle.m_iAimAt == BOT_AIM_BODY)
		m_vecLookAtVec = m_hEnemy->Center() - GetGunPosition();
	else if (FightStyle.m_iAimAt == BOT_AIM_HEAD)
		m_vecLookAtVec = m_hEnemy->EyePosition() - GetGunPosition();
	else if (FightStyle.m_iAimAt == BOT_AIM_ORIGIN)
		m_vecLookAtVec = m_hEnemy->pev->origin - GetGunPosition();
	else
		ActionSpeak("I don't know how to aim at my enemy!");

	AimAtBias();
}

void CBaseBot::AimAtEntity(CBaseEntity* pEntity) // orig 0x0893d0
{
	if (0 != m_fCalledAimThisFrame)
		return;
	m_fCalledAimThisFrame = 1;

	m_vecLookAtVec = pEntity->pev->origin - GetGunPosition();
	AimAtBias();
	ActionOpenFire();
}

// No target: look along the movement, drifting with the bias.
void CBaseBot::AimWander() // orig 0x089470
{
	m_fCalledAimThisFrame = 1;

	const Vector vecBias = m_vecLookAtBiasVec;
	const float x = RANDOM_FLOAT(-0.1, 0.1);
	const float y = RANDOM_FLOAT(-0.1, 0.1);
	const float z = RANDOM_FLOAT(-0.1, 0.1);

	Vector vecWander = vecBias + Vector(x, y, z);
	vecWander.z = 0;
	vecWander = vecWander.Normalize();

	Vector vecVelocity = pev->velocity.Normalize();
	vecVelocity.z = 0;

	m_vecLookAtVec = vecWander + vecVelocity * 2;
}

//=========================================================
// Obstacles
//=========================================================

// 288 units ahead from the origin, then from the eyes.
bool CBaseBot::CheckPathObstructedFront(Vector& vecDirection, TraceResult& tr) // orig 0x089660
{
	Vector vecStart = pev->origin;
	Vector vecEnd = vecStart + vecDirection.Normalize() * 288.0;
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, edict(), &tr);

	if (tr.flFraction == 1.0)
	{
		vecStart = EyePosition();
		vecEnd = vecStart + vecDirection.Normalize() * 288.0;
		UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, edict(), &tr);
		if (tr.flFraction == 1.0)
			return false;
	}

	return true;
}

// 128 units at 45 degrees to each side. vecDirection is the movement's angles, and the yaw change
// is applied to it and the result normalized as if it were a direction (orig).
bool CBaseBot::CheckPathObstructedSides(Vector& vecDirection, TraceResult& trLeft, TraceResult& trRight) // orig 0x089890
{
	const Vector vecStart = pev->origin;

	vecDirection.y = vecDirection.y - 45.0;
	Vector vecEnd = vecStart + vecDirection.Normalize() * 128.0;
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, edict(), &trLeft);

	vecDirection.y = vecDirection.y + 90.0;
	vecEnd = vecStart + vecDirection.Normalize() * 128.0;
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, edict(), &trRight);

	if (trLeft.flFraction == 1.0 && trRight.flFraction == 1.0)
		return false;
	return true;
}

bool CBaseBot::CheckWallOnLeft() // orig 0x089ab0
{
	TraceResult tr;

	UTIL_MakeVectors(pev->v_angle);
	const Vector vecStart = pev->origin;
	UTIL_TraceLine(vecStart, vecStart + gpGlobals->v_right * -40.0, dont_ignore_monsters, edict(), &tr);
	return tr.flFraction < 1.0;
}

bool CBaseBot::CheckWallOnRight() // orig 0x089b90
{
	TraceResult tr;

	UTIL_MakeVectors(pev->v_angle);
	const Vector vecStart = pev->origin;
	UTIL_TraceLine(vecStart, vecStart + gpGlobals->v_right * 40.0, dont_ignore_monsters, edict(), &tr);
	return tr.flFraction < 1.0;
}

//=========================================================
// HandleMovement: view angles from the look direction (averaged over 5 thinks), move angles from
// the desired velocity (averaged with the last 4 real velocities) bent around obstacles, then
// forward and side moves at bot_speed.
//=========================================================
void CBaseBot::HandleMovement() // orig 0x089c70
{
	const Vector vecLook = m_vecLookAtVec + Memory.m_History[0].vecLookAt + Memory.m_History[1].vecLookAt + Memory.m_History[2].vecLookAt + Memory.m_History[3].vecLookAt;
	pev->v_angle = UTIL_VecToAngles(vecLook);

	if (pev->v_angle.y > 180.0)
		pev->v_angle.y = pev->v_angle.y - 360.0;
	if (pev->v_angle.x > 180.0)
		pev->v_angle.x = pev->v_angle.x - 360.0;

	pev->angles.x = pev->v_angle.x / 3.0;
	pev->angles.y = pev->v_angle.y;
	pev->angles.z = 0;
	pev->v_angle.x = -pev->v_angle.x;

	const float flSpeed = bot_speed.value;

	Vector vecVelocity = (m_vecDesiredVelocity + Memory.m_History[0].vecVelocity + Memory.m_History[1].vecVelocity + Memory.m_History[2].vecVelocity + Memory.m_History[3].vecVelocity) * 0.2;
	const Vector vecMoveAngles = HandleObstacles(vecVelocity);
	const Vector vecViewAngles = pev->v_angle;

	float flMoveYaw = vecMoveAngles.y;
	while (flMoveYaw < 0)
		flMoveYaw += 360.0;

	float flViewYaw = vecViewAngles.y;
	while (flViewYaw < 0)
		flViewYaw += 360.0;

	float flDelta = flViewYaw - flMoveYaw;
	while (flDelta < 0)
		flDelta += 360.0;

	m_flMoveForward = cos(flDelta * 0.017453292) * flSpeed;
	m_flMoveStrafe = -sin(flDelta * 0.017453292) * flSpeed;

	if (IsOnLadder())
		SteerSafeOnLadder(STEER_DANGER);
	else
		m_flMoveVertical = 0;
}

// The angles to move along: the desired velocity's, turned away from a wall in front (by up to
// m_flMSec*360/1000 degrees, on the side picked by m_iTurningDirection) and nudged away from a
// wall at either side.
Vector CBaseBot::HandleObstacles(Vector& vecDesiredVelocity) // orig 0x08a0b0
{
	TraceResult trFront;
	TraceResult trSide;

	Vector vecAngles = UTIL_VecToAngles(vecDesiredVelocity);

	if (!CheckPathObstructedFront(vecDesiredVelocity, trFront))
	{
		m_iTurningDirection = TURNING_NONE;
	}
	else
	{
		const Vector vecWallAngles = UTIL_VecToAngles(trFront.vecPlaneNormal);

		float flYaw = vecAngles.y;
		if (flYaw < 0)
		{
			do
				flYaw += 360.0;
			while (flYaw < 0);
		}
		if (flYaw > 360.0)
		{
			do
				flYaw -= 360.0;
			while (flYaw > 360.0);
		}

		float flWall = vecWallAngles.y - 180.0;
		if (flWall < 0)
		{
			do
				flWall += 360.0;
			while (flWall < 0);
		}
		while (flWall > 360.0)
			flWall -= 360.0;

		float flLeft = flWall - 90.0;
		if (flLeft < 0)
			flLeft += 360.0;
		float flRight = flWall + 90.0;
		if (flRight > 360.0)
			flRight -= 360.0;

		flLeft = flYaw - flLeft;
		if (flLeft < -180.0)
			flLeft += 360.0;
		else if (flLeft > 180.0)
			flLeft -= 360.0;

		flRight = flYaw - flRight;
		if (flRight < -180.0)
			flRight += 360.0;
		else if (flRight > 180.0)
			flRight -= 360.0;

		const float flWallDist = trFront.flFraction * 288.0;

		int iLeft = (int)flLeft;
		if (iLeft < 0)
			iLeft = -iLeft;
		int iRight = (int)flRight;
		if (iRight < 0)
			iRight = -iRight;

		float flTurn;
		float flRandom;
		if ((iRight < iLeft && m_iTurningDirection == TURNING_NONE) || m_iTurningDirection == TURNING_LEFT)
		{
			m_iTurningDirection = TURNING_LEFT;
			flRandom = RANDOM_FLOAT(0.5, 1.5);
			flTurn = flLeft;
		}
		else
		{
			m_iTurningDirection = TURNING_RIGHT;
			flRandom = RANDOM_FLOAT(0.5, 1.5);
			flTurn = flRight;
		}
		vecAngles.y = vecAngles.y - flTurn * ((288.0 - flWallDist) * 0.00390625) * flRandom;

		// Clamp the turn. A turn to one side is clamped to the other side (orig).
		int iTurned = (int)(flYaw - vecAngles.y);
		if (iTurned < 0)
			iTurned = -iTurned;
		const float flMaxTurn = m_flMSec * 360.0 / 1000.0;
		if (flMaxTurn < iTurned)
		{
			if (flYaw - vecAngles.y >= 0)
				vecAngles.y = m_flMSec * 360.0 / 1000.0 + flYaw;
			else
				vecAngles.y = flYaw - flMaxTurn;
		}

		while (vecAngles.y > 180.0)
			vecAngles.y -= 360.0;
		while (vecAngles.y < -180.0)
			vecAngles.y += 360.0;
	}

	// trFront is reused for the left trace (orig).
	Vector vecSideAngles = vecAngles;
	if (CheckPathObstructedSides(vecSideAngles, trFront, trSide))
	{
		if (trFront.flFraction <= trSide.flFraction)
			vecAngles.y = RANDOM_FLOAT(1, m_flMSec * 360.0 / 1000.0) + vecAngles.y;
		else
			vecAngles.y = vecAngles.y - RANDOM_FLOAT(1, m_flMSec * 360.0 / 1000.0);

		while (vecAngles.y > 180.0)
			vecAngles.y -= 360.0;
		while (vecAngles.y < -180.0)
			vecAngles.y += 360.0;
	}

	return vecAngles;
}

//=========================================================
// Group steering, over the live players this bot can see within 128 units
//=========================================================

void CBaseBot::SteerGroupAlign() // orig 0x08a610
{
	int iCount = 0;
	Vector vecSum = g_vecZero;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
		if (!pPlayer || pPlayer == this)
			continue;
		if (!pPlayer->IsAlive() || !CheckVisible(pPlayer))
			continue;
		if ((pev->origin - pPlayer->pev->origin).Length() < 128.0)
		{
			iCount++;
			vecSum = vecSum + pPlayer->pev->velocity;
		}
	}

	m_vecDesiredVelocity = vecSum / (float)iCount; // orig: 0/0 with nobody near
}

void CBaseBot::SteerGroupCohesion() // orig 0x08a7a0
{
	int iCount = 0;
	Vector vecSum = g_vecZero;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
		if (!pPlayer || pPlayer == this)
			continue;
		if (!pPlayer->IsAlive() || !CheckVisible(pPlayer))
			continue;
		if ((pev->origin - pPlayer->pev->origin).Length() < 128.0)
		{
			iCount++;
			vecSum = vecSum + pPlayer->pev->origin;
		}
	}

	const Vector vecCenter = vecSum / (float)iCount; // orig: 0/0 with nobody near
	SteerPrimitiveSeek(vecCenter);
}

void CBaseBot::SteerGroupEscort(CBaseEntity* pLeader) // orig 0x08a920
{
	SteerPrimitivePursue(pLeader, Vector(0, 0, 0));
}

void CBaseBot::SteerGroupFlock() // orig 0x08a980
{
	SteerSafeGroupAlign(STEER_TEAMMATE);
	SteerSafeGroupCohesion(STEER_TEAMMATE);
	SteerSafeGroupSeparation(STEER_TEAMMATE);
}

void CBaseBot::SteerGroupFollow(CBaseEntity* pLeader) // orig 0x08a9c0
{
	SteerPrimitivePursue(pLeader, Vector(0, 0, 0));
}

// Seeks the sum of the away-directions over distance squared, which is a point near the world
// origin, not one near the bot (orig).
void CBaseBot::SteerGroupSeparation() // orig 0x08aa20
{
	Vector vecSum = g_vecZero;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
		if (!pPlayer || pPlayer == this)
			continue;
		if (!pPlayer->IsAlive() || !CheckVisible(pPlayer))
			continue;

		const Vector vecAway = pev->origin - pPlayer->pev->origin;
		const float flDist = vecAway.Length();
		if (flDist < 128.0)
		{
			const Vector vecDir = vecAway.Normalize();
			const float flDistSquared = flDist * flDist;
			vecSum = vecSum + vecDir / flDistSquared;
		}
	}

	SteerPrimitiveSeek(vecSum);
}

//=========================================================
// Primitive steering: each sets m_vecDesiredVelocity
//=========================================================

// Flee the point the entity reaches after its velocity times its distance (orig).
void CBaseBot::SteerPrimitiveEvade(CBaseEntity* pEntity) // orig 0x08ac30
{
	const float flDist = (pEntity->pev->origin - pev->origin).Length();
	const Vector vecFuture = pEntity->pev->velocity * flDist + pEntity->pev->origin;
	SteerPrimitiveFlee(vecFuture);
}

void CBaseBot::SteerPrimitiveFlee(CBaseEntity* pEntity) // orig 0x08ace0
{
	SteerPrimitiveFlee(pEntity->pev->origin);
}

void CBaseBot::SteerPrimitiveFlee(const Vector& vecFrom) // orig 0x08ad10
{
	m_vecDesiredVelocity = (pev->origin - vecFrom).Normalize() * bot_speed.value;
}

// Fighting on a ladder: jump off. Otherwise climb, and turn round when stuck.
void CBaseBot::SteerPrimitiveOnLadder() // orig 0x08adf0
{
	if (m_hEnemy == NULL)
	{
		if (m_flMovedDistance <= 1.0)
		{
			if (0 == m_fGoUpOnLadder)
				m_fGoUpOnLadder = 1;
			else
				m_fGoUpOnLadder = 0;
		}

		if (0 == m_fGoUpOnLadder)
			pev->button |= IN_BACK;
		else
			pev->button |= IN_FORWARD;
	}
	else
	{
		pev->button |= IN_JUMP;
		m_flMoveVertical = bot_speed.value;
	}
}

void CBaseBot::SteerPrimitivePursue(CBaseEntity* pEntity, Vector vecOffset) // orig 0x08aea0
{
	const float flDist = (pEntity->pev->origin - pev->origin).Length();
	const Vector vecFuture = pEntity->pev->velocity * flDist + pEntity->pev->origin + vecOffset;
	SteerPrimitiveSeek(vecFuture);
}

void CBaseBot::SteerPrimitiveSeek(CBaseEntity* pEntity) // orig 0x08af70
{
	SteerPrimitiveSeek(pEntity->pev->origin);
}

void CBaseBot::SteerPrimitiveSeek(const Vector& vecTo) // orig 0x08afa0
{
	m_vecDesiredVelocity = (vecTo - pev->origin).Normalize() * bot_speed.value;
}

// Turn the desired velocity by a random angle of up to flMaxTurn degrees per second.
void CBaseBot::SteerPrimitiveWander(float flMaxTurn) // orig 0x08b080
{
	const float flTurn = flMaxTurn * m_flMSec / 1000.0;

	const Vector vecVelocity = m_vecDesiredVelocity;
	Vector vecAngles = UTIL_VecToAngles(vecVelocity);
	UTIL_MakeVectors(vecAngles); // orig: unused
	vecAngles.y = RANDOM_FLOAT(-flTurn, flTurn) + vecAngles.y;
	UTIL_MakeVectors(vecAngles);

	m_vecDesiredVelocity = gpGlobals->v_forward;
	m_vecDesiredVelocity = m_vecDesiredVelocity.Normalize() * bot_speed.value;
}

// Roam over the navigation array: prefer a direction sharing a bit with the last one (up to 10
// tries), else wander.
void CBaseBot::SteerPrimitiveWanderImproved() // orig 0x08b1f0
{
	int iDirection = ActionChooseDirection();
	for (int i = 0; (m_iRoamDirection & iDirection) == 0 && i < 10; i++)
		iDirection = ActionChooseDirection();

	if (iDirection != DIRECTION_ALL)
	{
		float x = 0;
		if ((iDirection & DIRECTION_XPLUS) != 0)
			x = 1.0;
		else if ((iDirection & DIRECTION_XMINUS) != 0)
			x = -1.0;

		float y = 0;
		if ((iDirection & DIRECTION_YPLUS) != 0)
			y = 1.0;
		else if ((iDirection & DIRECTION_YMINUS) != 0)
			y = -1.0;

		const Vector vecDir = Vector(x, y, 0);
		if (vecDir.Length() > 0)
		{
			m_iRoamDirection = iDirection;
			m_vecDesiredVelocity = vecDir.Normalize() * bot_speed.value;
			return;
		}
	}

	SteerPrimitiveWander(20.0);
}

//=========================================================
// Safe steering: priority-checked wrappers of the above
//=========================================================

void CBaseBot::SteerSafeGroupAlign(STEER precedence) // orig 0x08b470
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerGroupAlign();
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerGroupAlign();
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeGroupCohesion(STEER precedence) // orig 0x08b5b0
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerGroupCohesion();
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerGroupCohesion();
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeGroupEscort(CBaseEntity* pLeader, STEER precedence) // orig 0x08b6f0
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerGroupEscort(pLeader);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerGroupEscort(pLeader);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeGroupFlock(STEER precedence) // orig 0x08b840
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerGroupFlock();
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerGroupFlock();
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

// Calls SteerGroupEscort, not SteerGroupFollow (orig; the two are the same).
void CBaseBot::SteerSafeGroupFollow(CBaseEntity* pLeader, STEER precedence) // orig 0x08b980
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerGroupEscort(pLeader);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerGroupEscort(pLeader);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeGroupSeparation(STEER precedence) // orig 0x08bad0
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerGroupSeparation();
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerGroupSeparation();
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeEvade(CBaseEntity* pEntity, STEER precedence) // orig 0x08bc10
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveEvade(pEntity);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveEvade(pEntity);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeFlee(CBaseEntity* pEntity, STEER precedence) // orig 0x08bd60
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveFlee(pEntity);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveFlee(pEntity);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeFlee(const Vector& vecFrom, STEER precedence) // orig 0x08beb0
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveFlee(vecFrom);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveFlee(vecFrom);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeOnLadder(STEER precedence) // orig 0x08c000
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveOnLadder();
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveOnLadder();
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafePursue(CBaseEntity* pEntity, STEER precedence, Vector vecOffset) // orig 0x08c140
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitivePursue(pEntity, vecOffset);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitivePursue(pEntity, vecOffset);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeSeek(CBaseEntity* pEntity, STEER precedence) // orig 0x08c2b0
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveSeek(pEntity);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveSeek(pEntity);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeSeek(const Vector& vecTo, STEER precedence) // orig 0x08c400
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveSeek(vecTo);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveSeek(vecTo);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeWander(float flMaxTurn, STEER precedence) // orig 0x08c550
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveWander(flMaxTurn);
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveWander(flMaxTurn);
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}

void CBaseBot::SteerSafeWanderImproved(STEER precedence) // orig 0x08c6b0
{
	if (m_iSteerCallPrecedence < precedence)
	{
		SteerPrimitiveWanderImproved();
		m_iSteerCallCounter = 1;
		m_iSteerCallPrecedence = precedence;
	}
	else if (precedence == m_iSteerCallPrecedence)
	{
		const Vector vecOld = m_vecDesiredVelocity;
		const float flCount = m_iSteerCallCounter;
		SteerPrimitiveWanderImproved();
		m_iSteerCallCounter++;
		m_vecDesiredVelocity = (vecOld * flCount + m_vecDesiredVelocity) / (float)m_iSteerCallCounter;
	}
}
