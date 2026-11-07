/*
 *	Glide Offhand Grappling Hook by Per Bernhardsson aka "Bard"
 *
 *	Modifications made for Internet play done by Will Roberson
 *
 *	Copyright (c) 1999, AfterShock Productions. All rights reserved.
 *
 *	Contact bard@ludd.luth.se or tadao@stargate.net if you have questions.
 *
 *	Oz Deathmatch 3.02: grapple.cpp, reconstructed from reference/hl_i386.so
 *	(reference/funcfile.tsv); names and comments from the OZ 1.7 source.
 */

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "grapple.h"
#include "effects.h"
#include "game.h"
#include "items.h"

// create a new entity called "grapple_bolt"
LINK_ENTITY_TO_CLASS(grapple_bolt, CGrappleBolt); // orig 0xc3bc0

// Create the grapple
// orig 0xc3be0
CGrappleBolt* CGrappleBolt::BoltCreate()
{
	// Create a new entity with CGrappleBolt private data
	CGrappleBolt* pBolt = GetClassPtr((CGrappleBolt*)NULL);
	pBolt->pev->classname = MAKE_STRING("grapple");
	pBolt->Spawn();

	return pBolt;
}

/*
 * Since I don't use the primary or secondary fire on a weapon to release
 * the grapplinghook I need some way to initialize the hook with direction
 * and velocity etc.
 */
// orig 0xc3c30
void CGrappleBolt::Init(CBasePlayer* m_pPlayer)
{
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 5;
	Vector vecDir = gpGlobals->v_forward;

	pev->origin = vecSrc;
	pev->angles = anglesAim;
	pev->owner = m_pPlayer->edict();

	pev->takedamage = DAMAGE_NO;
	pev->health = 20;

	// The bolt doesn't move as fast under water... (3.02 uses the air speed out of the water,
	// where 1.7 used the water speed in both)
	if (m_pPlayer->pev->waterlevel == 3)
	{
		if (m_pPlayer->m_iRune == RUNE_HOOK)
		{
			pev->velocity = vecDir * 2000;
			pev->speed = 2000;
		}
		else
		{
			pev->velocity = vecDir * oz_grapplewaterspeed.value;
			pev->speed = oz_grapplewaterspeed.value;
		}
	}
	else
	{
		if (m_pPlayer->m_iRune == RUNE_HOOK)
		{
			pev->velocity = vecDir * 2000;
			pev->speed = 2000;
		}
		else
		{
			pev->velocity = vecDir * oz_grappleairspeed.value;
			pev->speed = oz_grappleairspeed.value;
		}
	}
}

// orig 0xc3e50
void CGrappleBolt::Spawn()
{
	int red, green, blue;
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	// make the model invisible
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;

	// Oz grapple glow
	if (oz_grappleglow.value == 1)
		pev->effects |= EF_LIGHT;

	if (oz_randomgrapplecolor.value == 0)
	{
		red = oz_colorredgrapple.value;
		green = oz_colorgreengrapple.value;
		blue = oz_colorbluegrapple.value;
	}
	else
	{
		red = RANDOM_FLOAT(0, 255);
		green = RANDOM_FLOAT(0, 255);
		blue = RANDOM_FLOAT(0, 255);
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(20);			 // life
	WRITE_BYTE(2);			 // width
	WRITE_BYTE(red);		 // r, g, b
	WRITE_BYTE(green);		 // r, g, b
	WRITE_BYTE(blue);		 // r, g, b
	WRITE_BYTE(230);		 // brightness
	MESSAGE_END();

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/shell.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CGrappleBolt::BoltTouch);	  // Call this function when something is hit.
	SetThink(&CGrappleBolt::BubbleThink); // The think function during flight.
	pev->nextthink = gpGlobals->time + 0.2;
}

/*
 * When the bolt is killed the beam should be killed also.
 * This is a virtual function so there is no need to add code
 * anywhere else for this whatever reason the bolt is killed.
 */
// orig 0xc4180
void CGrappleBolt::Killed(entvars_t* pevAttacker, int iGib)
{
	CBaseEntity::Killed(NULL, 0);
}

// orig 0xc41b0
void CGrappleBolt::Precache()
{
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("common/null.wav");
}

// orig 0xc4250
int CGrappleBolt::Classify()
{
	return CLASS_NONE;
}

/*
 * This function is called when the bolt touches something.
 */
// orig 0xc4260
void CGrappleBolt::BoltTouch(CBaseEntity* pOther)
{
	SetTouch(NULL);
	pev->nextthink = gpGlobals->time + 0.1;

	/*
	 * Play different sounds depending on what is hit.
	 */

	entvars_t* pevOwner = VARS(pev->owner);

	CBasePlayer* plr = GetClassPtr((CBasePlayer*)pevOwner);

	if (pOther->IsPlayer())
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM);
			break;
		}

		TraceResult tr = UTIL_GetGlobalTrace();

		ClearMultiDamage();

		if (plr->m_iRune == RUNE_HOOK)
		{
			pOther->TraceAttack(pevOwner, oz_superhookrunedamage.value, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);
		}
		else
		{
			pOther->TraceAttack(pevOwner, oz_grappledamage.value, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);
		}

		ApplyMultiDamage(pev, pevOwner);

		plr->PlayerReleaseGrapple();
		return;
	}
	else if (pOther->IsMoving())
	{
		plr->PlayerReleaseGrapple();
		return;
	}
	// Oz: the sky holds the hook only when oz_skygrapple is set
	else if (UTIL_PointContents(pev->origin) == CONTENTS_SKY && oz_skygrapple.value == 0)
	{
		plr->PlayerReleaseGrapple();
		return;
	}
	else
	{
		if (oz_grapplesound.value == 0)
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "common/null.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink(&CGrappleBolt::DragPlayer); // New thinkfunction that will move the player.
		/*
		 * If the grapple hit something that is moving we need to store a
		 * reference to that entity.
		 */
		dragger = pOther->edict();
	}

	// Set up the position for the bolt.
	Vector vecDir = pev->velocity.Normalize();
	// Oz: when the owner ducks, the hook moves up 18 units (the standing and ducking hulls'
	// difference) instead of back along its flight
	if ((plr->pev->flags & FL_DUCKING) != 0)
		UTIL_SetOrigin(pev, pev->origin - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN));
	else
		UTIL_SetOrigin(pev, pev->origin - vecDir * 12);

	/*
	 * The distance between the bolt and the dragger so we can calculate if
	 * the dragger has moved between "thinks".
	 */
	draggerDist = pev->origin - pOther->pev->origin;
	pev->angles = UTIL_VecToAngles(vecDir);
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->velocity = Vector(0, 0, 0);
	pev->avelocity.z = 0;
	pev->angles.z = RANDOM_LONG(0, 360);

	// No sparks under water.
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		UTIL_Sparks(pev->origin);
	}
}

// orig 0xc47f0
void CGrappleBolt::BubbleThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	// The original also looks up the owner here (1.7's CTF trail colors used it) and never uses
	// it; with no owner, GetClassPtr(NULL) created an entity every think until the server ran
	// out of edicts. Left out (reference/DEVIATIONS.md, Crashes in the original).

	if (pev->waterlevel == 0)
		return;

	// If under water leave a bubbletrail.
	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

// orig 0xc48d0
void CGrappleBolt::PlayerStoppedThink()
{
	entvars_t* pevOwner = VARS(pev->owner);

	CBasePlayer* plr = GetClassPtr((CBasePlayer*)pevOwner);

	plr->m_afPhysicsFlags |= PFLAG_ONTRAIN; // Turn on this flag so the game thinks you're on a train
											// and it can check the movement keys while hanging.

	plr->IsHanging = true;

	pevOwner->gravity = -1; // Turn off gravity

	pevOwner->movetype = MOVETYPE_NOCLIP; // Only way to make player 100% stable

	pevOwner->origin = endOrigin; // Hold this origin

	pevOwner->velocity = Vector(0, 0, 0); // Drop all velocity
	pevOwner->speed = 0;

	if (oz_grappleglow.value == 1)
		pev->effects &= ~EF_LIGHT; // Turn off flashing light once hanging

	pev->nextthink = gpGlobals->time;
}

// orig 0xc49c0
void CGrappleBolt::DragPlayer()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if (pev->owner)
	{
		entvars_t* pevOwner = VARS(pev->owner);
		CBasePlayer* plr = GetClassPtr((CBasePlayer*)pevOwner);

		pevOwner->gravity = -1;

		Vector prevOrigin = pev->origin;
		(void)prevOrigin;

		/*
		 * If the origin has changed, make sure that the grapple
		 * continues to drag.
		 */

		Vector dist = (pev->origin - pevOwner->origin);

		// Oz: 3.02 pulls at oz_grappledragspeed beyond 150 units and at oz_playerspeed from 150
		// down to 50 (1.7 had one speed beyond 50)
		if (dist.Length() > 150)
		{
			if (plr->m_iRune == RUNE_HOOK)
			{
				pevOwner->velocity = dist.Normalize() * 2000;
				pevOwner->speed = 2000;
			}
			else
			{
				pevOwner->velocity = dist.Normalize() * oz_grappledragspeed.value;
				pevOwner->speed = oz_grappledragspeed.value;
			}
		}
		else if (dist.Length() < 150 && dist.Length() > 50)
		{
			pevOwner->velocity = dist.Normalize() * oz_playerspeed.value;
			pevOwner->speed = oz_playerspeed.value;
		}
		else
		{
			pevOwner->velocity = Vector(0, 0, 0);
			pevOwner->speed = 0;
			endOrigin = pevOwner->origin;
			SetThink(&CGrappleBolt::PlayerStoppedThink);
		}
	}
}
