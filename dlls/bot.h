// Oz Deathmatch 3.02: RhoBot, the built-in bot (bot_*.cpp), reconstructed from reference/hl_i386.so.
// Class layouts follow the original's offsets (in comments); the names of unsaved members, enum
// types' values and the history struct are ours (VERIFY name), taken from the accessors' names.
// Include after extdll.h, util.h, cbase.h, weapons.h and player.h.

#pragma once

class CBaseBot;

// Steering priorities. A steering call only takes effect when its priority is at least the one
// already set this think (CBaseBot::SteerSafe*). Values from the callers; names ours (VERIFY name).
enum STEER
{
	STEER_WANDER = 0,
	STEER_GOAL = 1,
	STEER_TEAMMATE = 2,
	STEER_COMBAT = 3,
	STEER_FLOCK = 4,
	STEER_FOLLOW = 5,
	STEER_DANGER = 6,
};

// Roaming direction bits over the navigation array (bot_nav.cpp). Names ours (VERIFY name).
enum DIRECTION
{
	DIRECTION_NONE = 0,
	DIRECTION_XPLUS = 1,
	DIRECTION_YMINUS = 2,
	DIRECTION_XMINUS = 4,
	DIRECTION_YPLUS = 8,
	DIRECTION_ALL = 15,
};

// Which way HandleObstacles is turning away from a wall. Names ours (VERIFY name).
enum TURNING_DIRECTION
{
	TURNING_NONE = 0,
	TURNING_LEFT = 1,
	TURNING_RIGHT = 2,
};

// What CBaseBot::AimAtEnemy aims at (CBaseBotFightStyle::m_iAimAt). Names ours (VERIFY name).
#define BOT_AIM_HEAD 0	 // EyePosition()
#define BOT_AIM_BODY 1	 // Center()
#define BOT_AIM_ORIGIN 2 // pev->origin (splash weapons)

//=========================================================
// CBaseBotFightStyle: how the bot uses its current weapon (bot_fightstyle.cpp). CBaseBot +0x718.
//=========================================================
class CBaseBotFightStyle
{
public:
	CBaseBotFightStyle();

	void DispatchWeaponUse(CBasePlayerItem* pActiveItem);
	void OptimizeAttackMode();
	void RandomizeAimAtHead(int iChance);
	void RandomizeSecondaryFire(int iChance);
	void SetNextShootTime(float flDelay, float flMinRandom, float flMaxRandom);

	void UseCrossbow();
	void UseCrowbar();
	void UseEgon();
	void UseGauss();
	void UseGlock();
	void UseHandGrenade();
	void UseHornetGun();
	void UseMP5();
	void UsePython();
	void UseRPG();
	void UseSatchel();
	void UseShotgun();
	void UseSnark();
	void UseTripMine();

	int m_iAimAt;			 // orig +0x00, BOT_AIM_*. VERIFY name
	int m_iAttackMode;		 // orig +0x04, only OptimizeAttackMode writes it, nothing reads it. VERIFY name
	int m_fHoldAttack;		 // orig +0x08, keep the button down 2 s instead of one tap. VERIFY name
	int m_fSecondaryFire;	 // orig +0x0c. VERIFY name
	CBaseBot* m_pBot;		 // orig +0x10, the owner. VERIFY name
	float m_flNextShootTime; // orig +0x14. VERIFY name
	float m_flStopShootTime; // orig +0x18, after this the weapon is chosen again. VERIFY name
};

//=========================================================
// CBaseBotMemory: what the bot saw over the last thinks (bot_memory.cpp). CBaseBot +0x734.
//=========================================================
struct botmemoryframe_t // VERIFY name: the original's layout is three Vectors per frame, frame stride 0x24
{
	Vector vecEnemyVelocity; // orig +0x00
	Vector vecLookAt;		 // orig +0x0c
	Vector vecVelocity;		 // orig +0x18
};

class CBaseBotMemory
{
public:
	CBaseBotMemory();

	void EnemyInSight(CBaseEntity* pEnemy);
	void EnemyOutOfSight();
	void EnemySighted(CBaseEntity* pEnemy);
	void EndFrameUpdate(CBasePlayer* pBot);

	float m_flEnemySightedTime;		  // orig +0x00, when the current enemy was picked. VERIFY name
	int m_fEnemyInSight;			  // orig +0x04. VERIFY name
	Vector m_vecEnemyLastSeen;		  // orig +0x08. VERIFY name
	float m_flEnemyLastSeenTime;	  // orig +0x14. VERIFY name
	botmemoryframe_t m_History[4];	  // orig +0x18, [0] the latest. VERIFY name
	Vector m_vecLastOrigin;			  // orig +0xa8, the bot's origin at the end of its last think. VERIFY name
};

//=========================================================
// CBaseBotStats: name, model, traits and weapon preferences (bot_stats.cpp). CBaseBot +0x7e8.
// The cfg files in bots/ set the bot_* cvars, which ReadBotFile copies here.
//=========================================================
class CBaseBotStats
{
public:
	CBaseBotStats();

	float FDifficultyAdjusted(float flTrait);
	float FindWeaponDesire(CBasePlayerItem* pItem, float flDistance);
	bool ReadBotFile(char* pszFile);

	char m_szName[64];		  // orig +0x00
	char m_szModel[64];		  // orig +0x40
	char m_szTopColor[4];	  // orig +0x80
	char m_szBottomColor[4];  // orig +0x84
	float m_flWpCrossbow;	  // orig +0x88
	float m_flWpCrowbar;	  // orig +0x8c
	float m_flWpEgon;		  // orig +0x90
	float m_flWpGauss;		  // orig +0x94
	float m_flWpGlock;		  // orig +0x98
	float m_flWpHandGrenade;  // orig +0x9c
	float m_flWpHornetGun;	  // orig +0xa0
	float m_flWpMP5;		  // orig +0xa4
	float m_flWpPython;		  // orig +0xa8
	float m_flWpRPG;		  // orig +0xac
	float m_flWpSatchel;	  // orig +0xb0
	float m_flWpShotgun;	  // orig +0xb4
	float m_flWpSnark;		  // orig +0xb8
	float m_flWpTripMine;	  // orig +0xbc
	float m_flTraitAccuracy;  // orig +0xc0
	float m_flTraitAggression; // orig +0xc4
	float m_flTraitChat;	  // orig +0xc8
	float m_flTraitJump;	  // orig +0xcc
	float m_flTraitPerception; // orig +0xd0
	float m_flTraitReflexes;  // orig +0xd4
};

//=========================================================
// CBaseBotGlobals: the one BotGlobals (bot_misc.cpp): skill auto-adjust and the bot cycle.
//=========================================================
class CBaseBotGlobals
{
public:
	CBaseBotGlobals();

	void IncreaseDifficulty();
	void DecreaseDifficulty();
	void HandleAutoAdjDifficulty();

	float m_flNextAutoAdjTime;	// orig +0x00, starts at 12. VERIFY name
	char m_szMapName[33];		// orig +0x04, "boot_camp"; nothing reads it. VERIFY name, size
	char m_szLastBotUsed[123];	// orig +0x25, the botcycle.txt line last used. VERIFY name, size
};

//=========================================================
// CBaseBot: the bot player (bot_ai.cpp, bot_chat.cpp, bot_misc.cpp, bot_move.cpp, bot_nav.cpp).
// Its only virtuals are the two overrides; everything else is non-virtual.
//=========================================================
class CBaseBot : public CBasePlayer
{
public:
	CBaseBot();

	bool IsNetClient() override { return false; } // orig 0x0861a0
	void BotThink() override;

	// bot_ai.cpp
	void ActionOpenFire();
	void ActionChooseGoal();
	void ActionChooseWeapon();
	void ActionLook(int iDistance);
	bool ActionReload();
	void ActionSpeak(const char* pszText);
	bool CheckBotKick();
	float GetBotThinkDelay();
	bool CheckHasDecentWeapon();
	bool CheckNotice(CBaseEntity* pEntity);
	bool CheckVisible(CBaseEntity* pEntity);
	void HandleTime();
	void ThinkDead();
	void ThinkMood();
	void ThinkStart();
	void ThinkSteering();
	void ThinkValidateEnemy();
	void ThinkValidateGoal();
	bool ActionCheckMercy(CBasePlayer* pPlayer);
	bool ActionCheckCloak(CBasePlayer* pPlayer);

	// bot_chat.cpp
	void ActionChat();

	// bot_misc.cpp: not virtual and nothing calls it (dead code in the original too)
	using CBasePlayer::TakeDamage;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType, CBasePlayer* pAttacker);

	// bot_move.cpp
	void AimAtBias();
	void AimAtEnemy();
	void AimAtEntity(CBaseEntity* pEntity);
	void AimWander();
	bool CheckPathObstructedFront(Vector& vecDirection, TraceResult& tr);
	bool CheckPathObstructedSides(Vector& vecDirection, TraceResult& trLeft, TraceResult& trRight);
	bool CheckWallOnLeft();
	bool CheckWallOnRight();
	void HandleMovement();
	Vector HandleObstacles(Vector& vecDesiredVelocity);
	void SteerGroupAlign();
	void SteerGroupCohesion();
	void SteerGroupEscort(CBaseEntity* pLeader);
	void SteerGroupFlock();
	void SteerGroupFollow(CBaseEntity* pLeader);
	void SteerGroupSeparation();
	void SteerPrimitiveEvade(CBaseEntity* pEntity);
	void SteerPrimitiveFlee(CBaseEntity* pEntity);
	void SteerPrimitiveFlee(const Vector& vecFrom);
	void SteerPrimitiveOnLadder();
	void SteerPrimitivePursue(CBaseEntity* pEntity, Vector vecOffset);
	void SteerPrimitiveSeek(CBaseEntity* pEntity);
	void SteerPrimitiveSeek(const Vector& vecTo);
	void SteerPrimitiveWander(float flMaxTurn);
	void SteerPrimitiveWanderImproved();
	void SteerSafeGroupAlign(STEER precedence);
	void SteerSafeGroupCohesion(STEER precedence);
	void SteerSafeGroupEscort(CBaseEntity* pLeader, STEER precedence);
	void SteerSafeGroupFlock(STEER precedence);
	void SteerSafeGroupFollow(CBaseEntity* pLeader, STEER precedence);
	void SteerSafeGroupSeparation(STEER precedence);
	void SteerSafeEvade(CBaseEntity* pEntity, STEER precedence);
	void SteerSafeFlee(CBaseEntity* pEntity, STEER precedence);
	void SteerSafeFlee(const Vector& vecFrom, STEER precedence);
	void SteerSafeOnLadder(STEER precedence);
	void SteerSafePursue(CBaseEntity* pEntity, STEER precedence, Vector vecOffset);
	void SteerSafeSeek(CBaseEntity* pEntity, STEER precedence);
	void SteerSafeSeek(const Vector& vecTo, STEER precedence);
	void SteerSafeWander(float flMaxTurn, STEER precedence);
	void SteerSafeWanderImproved(STEER precedence);

	// bot_nav.cpp
	int ActionChooseDirection();

	// Accessors. The original emits them out of line in bot_ai.cpp, but bot_ai.cpp and
	// bot_misc.cpp inline SetEnemy's body, so they were inline in the class.
	int GetWantToBeInCombat() { return m_fWantToBeInCombat; }					// orig 0x0861b0
	int GetTurningDirection() { return m_iTurningDirection; }					// orig 0x0861c0
	float GetTimeMSecCheck() { return m_flTimeMSecCheck; }						// orig 0x0861d0
	float GetTimeGoalCheckDelay() { return m_flTimeGoalCheckDelay; }			// orig 0x0861e0
	float GetTimeGoalCheck() { return m_flTimeGoalCheck; }						// orig 0x0861f0
	int GetSteerCallPrecedence() { return m_iSteerCallPrecedence; }				// orig 0x086200
	int GetSteerCallCounter() { return m_iSteerCallCounter; }					// orig 0x086210
	float GetServerMaxSpeed() { return bot_speed.value; }						// orig 0x086220
	int GetRoamDirection() { return m_iRoamDirection; }							// orig 0x086240
	float GetMoveVertical() { return m_flMoveVertical; }						// orig 0x086250
	int GetMSecCounter() { return m_iMSecCounter; }								// orig 0x086260
	float GetMSec() { return m_flMSec; }										// orig 0x086270
	float GetMoveStrafe() { return m_flMoveStrafe; }							// orig 0x086280
	float GetMoveForward() { return m_flMoveForward; }							// orig 0x086290
	float GetMovedDistance() { return m_flMovedDistance; }						// orig 0x0862a0
	Vector GetLookAtVec() { return m_vecLookAtVec; }							// orig 0x0862b0
	Vector GetLookAtBiasVec() { return m_vecLookAtBiasVec; }					// orig 0x0862e0
	int GetGoUpOnLadder() { return m_fGoUpOnLadder; }							// orig 0x086310
	CBaseEntity* GetGoal() { return m_hGoal; }									// orig 0x086320
	CBaseEntity* GetEnemy() { return m_hEnemy; }								// orig 0x086350
	Vector GetDesiredVelocity() { return m_vecDesiredVelocity; }				// orig 0x086380
	int GetCalledAimThisFrame() { return m_fCalledAimThisFrame; }				// orig 0x0863b0
	void SetWantToBeInCombat(int fWant) { m_fWantToBeInCombat = fWant; }		// orig 0x0863c0
	void SetTurningDirection(TURNING_DIRECTION dir) { m_iTurningDirection = dir; } // orig 0x0863e0
	void SetTimeMSecCheck(float flTime) { m_flTimeMSecCheck = flTime; }		// orig 0x086400
	void SetTimeGoalCheckDelay(float flDelay) { m_flTimeGoalCheckDelay = flDelay; } // orig 0x086420
	void SetTimeGoalCheck(float flTime) { m_flTimeGoalCheck = flTime; }		// orig 0x086440
	void SetSteerCallPrecedence(STEER precedence) { m_iSteerCallPrecedence = precedence; } // orig 0x086460
	void SetSteerCallCounter(int iCount) { m_iSteerCallCounter = iCount; }		// orig 0x086480
	void SetRoamDirection(DIRECTION dir) { m_iRoamDirection = dir; }			// orig 0x0864a0
	void SetMSecCounter(int iCount) { m_iMSecCounter = iCount; }				// orig 0x0864c0
	void SetMSec(float flMSec) { m_flMSec = flMSec; }							// orig 0x0864e0
	void SetMoveVertical(float flMove) { m_flMoveVertical = flMove; }			// orig 0x086500
	void SetMoveStrafe(float flMove) { m_flMoveStrafe = flMove; }				// orig 0x086520
	void SetMoveForward(float flMove) { m_flMoveForward = flMove; }				// orig 0x086540
	void SetMovedDistance(float flDist) { m_flMovedDistance = flDist; }		// orig 0x086560
	void SetLookAtVec(const Vector& vec) { m_vecLookAtVec = vec; }				// orig 0x086580
	void SetLookAtBiasVec(const Vector& vec) { m_vecLookAtBiasVec = vec; }		// orig 0x0865b0
	void SetGoUpOnLadder(int fUp) { m_fGoUpOnLadder = fUp; }					// orig 0x0865e0
	void SetGoal(CBaseEntity* pGoal) { m_hGoal = pGoal; }						// orig 0x086600
	// orig 0x086630
	void SetEnemy(CBaseEntity* pEnemy)
	{
		m_hEnemy = pEnemy;
		FightStyle.m_fHoldAttack = 0;
		FightStyle.SetNextShootTime(0.0, 0.5, 1.0);
	}
	void SetDesiredVelocity(const Vector& vec) { m_vecDesiredVelocity = vec; }	// orig 0x086690
	void SetCalledAimThisFrame(int fCalled) { m_fCalledAimThisFrame = fCalled; } // orig 0x0866c0

	int m_fCalledAimThisFrame;	 // orig +0x6a0
	Vector m_vecDesiredVelocity; // orig +0x6a4
	int m_fGoUpOnLadder;		 // orig +0x6b0
	Vector m_vecLookAtBiasVec;	 // orig +0x6b4
	Vector m_vecLookAtVec;		 // orig +0x6c0
	float m_flNextBotThink;		 // orig +0x6cc, not set by the constructor (0 from the allocator). VERIFY name
	float m_flMovedDistance;	 // orig +0x6d0
	float m_flMoveForward;		 // orig +0x6d4
	float m_flMoveStrafe;		 // orig +0x6d8
	float m_flMoveVertical;		 // orig +0x6dc
	float m_flMSec;				 // orig +0x6e0
	int m_iMSecCounter;			 // orig +0x6e4
	EHANDLE m_hEnemy;			 // orig +0x6e8
	EHANDLE m_hGoal;			 // orig +0x6f0
	int m_iRoamDirection;		 // orig +0x6f8, DIRECTION
	int m_iSteerCallCounter;	 // orig +0x6fc
	int m_iSteerCallPrecedence;	 // orig +0x700, STEER
	float m_flTimeGoalCheck;	 // orig +0x704
	float m_flTimeGoalCheckDelay; // orig +0x708
	float m_flTimeMSecCheck;	 // orig +0x70c
	int m_iTurningDirection;	 // orig +0x710, TURNING_DIRECTION
	int m_fWantToBeInCombat;	 // orig +0x714
	CBaseBotFightStyle FightStyle; // orig +0x718. VERIFY name
	CBaseBotMemory Memory;		 // orig +0x734. VERIFY name
	CBaseBotStats Stats;		 // orig +0x7e8. VERIFY name
};

// bot_ai.cpp: the ammo ActionChooseGoal looks for, built at load time from the max_* cvars.
struct botammocheck_t // VERIFY name
{
	const char* pszClassname;
	const char* pszAmmoName;
	int iMaxCarry;
};
extern botammocheck_t ammo_check[10];

// bot_misc.cpp
extern CBaseBotGlobals BotGlobals;
extern int skin_used[16];
extern char bot_skins[16][64];
extern int name_used[37];
extern char bot_names[37][64];
char* ChooseRandomName();
char* ChooseRandomSkin();
bool UseBotCycle(int iLine);
void BotClientPutInServer(edict_t* pEntity, int fUseBotCycle);
void BotConnect(int iBot);
