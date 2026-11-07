/*
 *	Glide Offhand Grappling Hook by Per Bernhardsson aka "Bard"
 *
 *	Modifications made for Internet play done by Will Roberson
 *
 *	Copyright (c) 1999, AfterShock Productions. All rights reserved.
 *
 *	Oz Deathmatch 3.02: reconstructed from reference/hl_i386.so; names from the OZ 1.7 source.
 */

#pragma once

class CBasePlayer;

// Oz grapple: the hook a player fires (client.cpp's grapple commands create it,
// CBasePlayer::PlayerReleaseGrapple kills it). Base CBaseEntity (hierarchy.tsv).
class CGrappleBolt : public CBaseEntity
{
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	void EXPORT BubbleThink();
	void EXPORT BoltTouch(CBaseEntity* pOther);
	void EXPORT DragPlayer();
	void EXPORT PlayerStoppedThink();

	int m_iTrail;		 // orig +0x64: sprites/smoke.spr, the beam follow trail
	Vector draggerDist;	 // orig +0x68: bolt origin - the hit entity's origin at impact (written, never read)
	Vector endOrigin;	 // orig +0x74: where the owner hangs once the drag ends
	bool dragFinished;	 // orig +0x80: 1.7's; no 3.02 code uses it
	bool dragWasFinished; // orig +0x81: same
	bool draggerMoving;	 // orig +0x82: same
	edict_t* dragger;	 // orig +0x84: the entity the bolt hit (written, never read)

public:
	static CGrappleBolt* BoltCreate();
	void Init(CBasePlayer* plr);
	void Killed(entvars_t* pevAttacker, int iGib) override;
	bool dragging; // orig +0x88: 1.7's; no 3.02 code uses it
};
