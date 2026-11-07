// Oz Deathmatch 3.02: flyingcrowbar.cpp, reconstructed from reference/hl_i386.so (reference/funcfile.tsv).
//
// flying_crowbar: the crowbar CCrowbar::SecondaryAttack throws. It hurts what it hits, then (without
// the Super Crowbar Rune) drops a weaponbox holding a new weapon_crowbar where it landed.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "game.h"
#include "items.h"

// OZ derives it from CBasePlayerWeapon (its vtable is the weapon's with Spawn and Precache replaced),
// so it has an m_pPlayer, which CCrowbar::SecondaryAttack sets to the thrower.
class CFlyingCrowbar : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT SpinTouch(CBaseEntity* pOther);
	void EXPORT BubbleThink();

	EHANDLE m_hOwner; // orig +0xcc. VERIFY name: pev->owner at spawn, the attacker its damage is credited to
};

LINK_ENTITY_TO_CLASS(flying_crowbar, CFlyingCrowbar); // orig 0xb4eb0

// orig 0xb4ed0
void CFlyingCrowbar::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	if (pev->owner)
		m_hOwner = CBaseEntity::Instance(pev->owner);

	SetThink(&CFlyingCrowbar::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.25;
	SetTouch(&CFlyingCrowbar::SpinTouch);
}

// orig 0xb5000
void CFlyingCrowbar::Precache()
{
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
}

// orig 0xb5060
void CFlyingCrowbar::SpinTouch(CBaseEntity* pOther)
{
	if (pOther->pev->takedamage != DAMAGE_NO)
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		ClearMultiDamage();

		// sic: with the Super Crowbar Rune a throw does oz_damageflyingcrowbar, without it twice that.
		// The inflictor is the crowbar itself.
		if (m_pPlayer->m_iRune == RUNE_CROWBAR)
			pOther->TraceAttack(pev, oz_damageflyingcrowbar.value, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);
		else
			pOther->TraceAttack(pev, oz_damageflyingcrowbar.value + oz_damageflyingcrowbar.value, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);

		if (m_hOwner)
			ApplyMultiDamage(pev, m_hOwner->pev);
		else
			ApplyMultiDamage(pev, pev);
	}

	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;

	if (m_pPlayer->m_iRune == RUNE_CROWBAR)
	{
		if (pOther->IsPlayer())
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM);
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/cbar_hit1.wav", 1, ATTN_NORM);
			if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
			{
				UTIL_Sparks(pev->origin);
				UTIL_Sparks(pev->origin);
				UTIL_Sparks(pev->origin);
			}
		}

		// The original stops here, so every thrown crowbar stays, invisible and not solid, its
		// BubbleThink running every 0.25 s, until the server runs out of edicts. Ours removes it as
		// the path below does (reference/DEVIATIONS.md, Crashes in the original).
		SetThink(&CFlyingCrowbar::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	if (pOther->IsPlayer())
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM);
	}
	else
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/cbar_hit1.wav", 1, ATTN_NORM);
		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
			UTIL_Sparks(pev->origin);
			UTIL_Sparks(pev->origin);
		}
	}

	// leave a crowbar in a weaponbox where it landed, removed after two minutes
	CBaseEntity* pItem = CBaseEntity::Create("weapon_crowbar", pev->origin, pev->angles, edict());
	CWeaponBox* pBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin, pev->angles, edict());
	pBox->pev->angles.x = 0;
	pBox->pev->angles.z = 0;
	pBox->pev->nextthink = gpGlobals->time + 120;
	pBox->SetThink(&CWeaponBox::Kill);
	pBox->PackWeapon((CBasePlayerItem*)pItem);

	// and push the box off the surface that was hit
	Vector vecDir = pev->velocity.Normalize();
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + vecDir * 100, dont_ignore_monsters, edict(), &tr);
	pBox->pev->velocity = tr.vecPlaneNormal * 300;

	SetThink(&CFlyingCrowbar::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

// orig 0xb5600
void CFlyingCrowbar::BubbleThink()
{
	pev->owner = NULL;
	pev->nextthink = gpGlobals->time + 0.25;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}
