//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//
// Primary attack: fires a beam that hooks on a surface.
// Secondary attack: switches between pull and rapple modes
//
//
//=============================================================================//
#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_grapple.h"
#ifdef CLIENT_DLL
// #include "c_hl2mp_player.h"
//#include 
// #include "c_te_effect_dispatch.h"
#include "c_te_effect_dispatch.h"
#else
#include "game.h"
// #include "hl2mp_player.h"
#include "player.h"
#include "te_effect_dispatch.h"
#include "IEffects.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "beam_shared.h"
#include "explode.h"
#include "ammodef.h"	/* This is needed for the tracing done later */
#include "gamestats.h" //
#include "soundent.h" //
#include "vphysics/constraints.h"
#include "physics_saverestore.h"
#endif
//#include "effect_dispatch_data.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HOOK_MODEL	"models/props_junk/rock001a.mdl"
#define BOLT_MODEL			"models/crossbow_bolt.mdl"
#define BOLT_AIR_VELOCITY	3500
#define BOLT_WATER_VELOCITY	1500
#define	BOLT_SKIN_NORMAL	0
#define BOLT_SKIN_GLOW		1

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS(grapple_hook, CGrappleHook);

BEGIN_DATADESC(CGrappleHook)
	// Function Pointers
	DEFINE_THINKFUNC(FlyThink),
	DEFINE_THINKFUNC(HookedThink),
	DEFINE_FUNCTION(HookTouch),
	DEFINE_FIELD(m_hPlayer, FIELD_EHANDLE),
	DEFINE_FIELD(m_hOwner, FIELD_EHANDLE),
	DEFINE_FIELD(m_hBolt, FIELD_EHANDLE),
	DEFINE_FIELD(m_bPlayerWasStanding, FIELD_BOOLEAN),
END_DATADESC()

CGrappleHook *CGrappleHook::HookCreate(const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner)
{
	// Create a new entity with CGrappleHook private data
	CGrappleHook *pHook = (CGrappleHook *)CreateEntityByName("grapple_hook");
	UTIL_SetOrigin(pHook, vecOrigin);
	pHook->SetAbsAngles(angAngles);
	pHook->Spawn();
	CWeaponGrapple *pOwner = (CWeaponGrapple *)pentOwner;
	pHook->m_hOwner = pOwner;
	pHook->SetOwnerEntity(pOwner->GetOwner());
	pHook->m_hPlayer = (CBasePlayer *)pOwner->GetOwner();
	return pHook;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CGrappleHook::~CGrappleHook(void)
{
	if (m_hBolt)
	{
		UTIL_Remove(m_hBolt);
		m_hBolt = NULL;
	}
	// Revert Jay's gai flag
	if (m_hPlayer)
		m_hPlayer->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, false);
}
//-----------------------------------------------------------------------------
// Purpose:
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGrappleHook::CreateVPhysics(void)
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_STANDABLE, false);
	return true;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CGrappleHook::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}
//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CGrappleHook::Spawn(void)
{
	Precache();
	SetModel(HOOK_MODEL);
	SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM);
	UTIL_SetSize(this, -Vector(1, 1, 1), Vector(1, 1, 1));
	SetSolid(SOLID_BBOX);
	SetGravity(0.05f);
	// The rock is invisible, the crossbow bolt is the visual representation
	AddEffects(EF_NODRAW);
	// Make sure we're updated if we're underwater
	UpdateWaterState();
	SetTouch(&CGrappleHook::HookTouch);
	SetThink(&CGrappleHook::FlyThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
	//m_pSpring = NULL;
	m_fSpringLength = 0.0f;
	m_bPlayerWasStanding = false;

	// Create bolt model and parent it
	CBaseEntity *pBolt = CBaseEntity::CreateNoSpawn("prop_dynamic", GetAbsOrigin(), GetAbsAngles(), this);
	pBolt->SetModelName(MAKE_STRING(BOLT_MODEL));
	pBolt->SetModel(BOLT_MODEL);
	DispatchSpawn(pBolt);
	pBolt->SetParent(this);
}
void CGrappleHook::Precache(void)
{
	PrecacheModel(HOOK_MODEL);
	PrecacheModel(BOLT_MODEL);
}
//-----------------------------------------------------------------------------
// Purpose:
// Input : *pOther -
//-----------------------------------------------------------------------------
void CGrappleHook::HookTouch(CBaseEntity *pOther)
{
	if (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS))
		return;
	if ((pOther != m_hOwner) && (pOther->m_takedamage != DAMAGE_NO))
	{
		m_hOwner->NotifyHookDied();
		SetTouch(NULL);
		SetThink(NULL);
		UTIL_Remove(this);
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();
		// See if we struck the world
		if (pOther->GetMoveType() == MOVETYPE_NONE && !(tr.surface.flags & SURF_SKY))
		{
			EmitSound("Weapon_AR2.Reload_Push");
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			//FIXME: We actually want to stick (with hierarchy) to what we've hit
			SetMoveType(MOVETYPE_NONE);
			Vector vForward;
			AngleVectors(GetAbsAngles(), &vForward);
			VectorNormalize(vForward);
			CEffectData data;
			data.m_vOrigin = tr.endpos;
			data.m_vNormal = vForward;
			data.m_nEntIndex = 0;
			// DispatchEffect( "Impact", data );
			// AddEffects( EF_NODRAW );
			SetTouch(NULL);
			VPhysicsDestroyObject();
			VPhysicsInitNormal(SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false);
			AddSolidFlags(FSOLID_NOT_SOLID);
			// SetMoveType( MOVETYPE_NONE );
			if (!m_hPlayer)
			{
				Assert(0);
				return;
			}
			// Set Jay's gai flag
			m_hPlayer->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, true);
			//IPhysicsObject *pPhysObject = m_hPlayer->VPhysicsGetObject();
			IPhysicsObject *pRootPhysObject = VPhysicsGetObject();
			Assert(pRootPhysObject);
			//Assert(pPhysObject);
			pRootPhysObject->EnableMotion(false);
			// Root has huge mass, tip has little
			pRootPhysObject->SetMass(VPHYSICS_MAX_MASS);
			// pPhysObject->SetMass( 100 );
			// float damping = 3;
			// pPhysObject->SetDamping( &damping, &damping );
			Vector origin = m_hPlayer->GetAbsOrigin();
			Vector rootOrigin = GetAbsOrigin();
			m_fSpringLength = (origin - rootOrigin).Length();
			m_bPlayerWasStanding = ((m_hPlayer->GetFlags() & FL_DUCKING) == 0);
			SetThink(&CGrappleHook::HookedThink);
			SetNextThink(gpGlobals->curtime + 0.1f);
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ((tr.surface.flags & SURF_SKY) == false)
			{
				UTIL_ImpactTrace(&tr, DMG_BULLET);
			}
			SetTouch(NULL);
			SetThink(NULL);
			m_hOwner->NotifyHookDied();
			UTIL_Remove(this);
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGrappleHook::HookedThink(void)
{
	//set next globalthink
	SetNextThink(gpGlobals->curtime + 0.05f); //0.1f
	//All of this push the player far from the hook
	Vector tempVec1 = m_hPlayer->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize(tempVec1);
	int temp_multiplier = -1;
	m_hPlayer->SetGravity(0.0f);
	m_hPlayer->SetGroundEntity(NULL);
	if (m_hOwner->m_bHook){
		temp_multiplier = 1;
		m_hPlayer->SetAbsVelocity(tempVec1*temp_multiplier * 50);//400
	}
	else
	{
		m_hPlayer->SetAbsVelocity(tempVec1*temp_multiplier * 400);//400
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGrappleHook::FlyThink(void)
{
	QAngle angNewAngles;
	VectorAngles(GetAbsVelocity(), angNewAngles);
	SetAbsAngles(angNewAngles);
	SetNextThink(gpGlobals->curtime + 0.1f);
}
#endif
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGrapple, DT_WeaponGrapple)
#ifdef CLIENT_DLL
void RecvProxy_HookDied(const CRecvProxyData *pData, void *pStruct, void *pOut)
{
	CWeaponGrapple *pGrapple = ((CWeaponGrapple*)pStruct);
	RecvProxy_IntToEHandle(pData, pStruct, pOut);
	CBaseEntity *pNewHook = pGrapple->GetHook();
	if (pNewHook == NULL)
	{
		if (pGrapple->GetOwner() && pGrapple->GetOwner()->GetActiveWeapon() == pGrapple)
		{
			pGrapple->NotifyHookDied();
		}
	}
}
#endif
BEGIN_NETWORK_TABLE(CWeaponGrapple, DT_WeaponGrapple)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bInZoom)),
	RecvPropBool(RECVINFO(m_bMustReload)),
	RecvPropEHandle(RECVINFO(m_hHook), RecvProxy_HookDied),
	RecvPropInt(RECVINFO(m_nBulletType)),
#else
	SendPropBool(SENDINFO(m_bInZoom)),
	SendPropBool(SENDINFO(m_bMustReload)),
	SendPropEHandle(SENDINFO(m_hHook)),
	SendPropInt(SENDINFO(m_nBulletType)),
#endif
END_NETWORK_TABLE()
#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGrapple)
	DEFINE_PRED_FIELD(m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bMustReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif
LINK_ENTITY_TO_CLASS(weapon_grapple, CWeaponGrapple);
PRECACHE_WEAPON_REGISTER(weapon_grapple);
#ifndef CLIENT_DLL
acttable_t	CWeaponGrapple::m_acttable[] =
{
	// { ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_CROSSBOW, false },
	// { ACT_HL2MP_RUN, ACT_HL2MP_RUN_CROSSBOW, false },
	// { ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_CROSSBOW, false },
	// { ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_CROSSBOW, false },
	// { ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW, false },
	// // Valve never made a crossbow reload anim for playermodels, I guess use the ar2 one for now
	// //{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_CROSSBOW, false },
	// { ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_AR2, false },
	// { ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_CROSSBOW, false },
	{ ACT_IDLE, ACT_IDLE, false },
	{ ACT_RUN, ACT_RUN, false },
	{ ACT_WALK_CROUCH, ACT_WALK_CROUCH, false },
	{ ACT_GESTURE_RELOAD, ACT_GESTURE_RELOAD, false },
	{ ACT_JUMP, ACT_JUMP, false },
};
IMPLEMENT_ACTTABLE(CWeaponGrapple);
#endif
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGrapple::CWeaponGrapple(void)
{
	m_bReloadsSingly = true;
	m_bFiresUnderwater = true;
	m_bInZoom = false;
	m_bMustReload = false;
	m_nBulletType = -1;
#ifndef CLIENT_DLL
	m_hChargerSprite = NULL;
	m_hRope = NULL;
#endif
}

#define	CROSSBOW_GLOW_SPRITE	"sprites/light_glow02_noz.vmt"
#define	CROSSBOW_GLOW_SPRITE2	"sprites/blueflare1.vmt"

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponGrapple::Precache(void)
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther("grapple_hook");
#endif
	PrecacheScriptSound("Weapon_Crossbow.BoltHitWorld");

	PrecacheModel(CROSSBOW_GLOW_SPRITE);
	PrecacheModel(CROSSBOW_GLOW_SPRITE2);

	BaseClass::Precache();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::PrimaryAttack(void)
{
	// Can't have an active hook out
	if (m_hHook != NULL)
		return;

	FireHook();
	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration(ACT_VM_PRIMARYATTACK));
}
//-----------------------------------------------------------------------------
// Purpose: Toggles the grapple hook modes
//-----------------------------------------------------------------------------
void CWeaponGrapple::SecondaryAttack(void)
{
	ToggleHook();
	WeaponSound(EMPTY);
}
//-----------------------------------------------------------------------------
// Purpose:
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Reload(void)
{
	if ((m_bMustReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		//Redraw the weapon
		SendWeaponAnim(ACT_VM_RELOAD); //ACT_VM_RELOAD
		//Update our times
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
		//Mark this as done
		m_bMustReload = false;
	}
	return true;
}
//-----------------------------------------------------------------------------
// Purpose: Toggles between pull and rappel mode
//-----------------------------------------------------------------------------
bool CWeaponGrapple::ToggleHook(void)
{
#ifndef CLIENT_DLL
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
	if (m_bHook)
	{
		m_bHook = false;
		ClientPrint(pPlayer, HUD_PRINTCENTER, "Pull mode");
		return m_bHook;
	}
	else
	{
		m_bHook = true;
		ClientPrint(pPlayer, HUD_PRINTCENTER, "Rappel mode");
		return m_bHook;
	}
#endif
	return m_bHook;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::ItemBusyFrame(void)
{
	// Allow zoom toggling even when we're reloading
	//CheckZoomToggle();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::ItemPostFrame(void)
{
	//Enforces being able to use PrimaryAttack and Secondary Attack
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if ((pOwner->m_nButtons & IN_ATTACK))
	{
		if (m_flNextPrimaryAttack < gpGlobals->curtime)
		{
			PrimaryAttack();
		}
	}
	else if (m_bMustReload) //&& HasWeaponIdleTimeElapsed() )
	{
		Reload();
	}
	if ((pOwner->m_afButtonPressed & IN_ATTACK2))
	{
		if (m_flNextPrimaryAttack < gpGlobals->curtime)
		{
			SecondaryAttack();
		}
	}
	else if (m_bMustReload) //&& HasWeaponIdleTimeElapsed() )
	{
		Reload();
	}
	//Allow a refire as fast as the player can click
	if (((pOwner->m_nButtons & IN_ATTACK) == false))
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
#ifndef CLIENT_DLL
	if (m_hHook)
	{
		if (!(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2))
		{
			m_hHook->SetTouch(NULL);
			m_hHook->SetThink(NULL);
			UTIL_Remove(m_hHook);
			m_hHook = NULL;
			NotifyHookDied();
			m_bMustReload = true;
		}
	}
#endif
	// BaseClass::ItemPostFrame();
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::CreateRope(void)
{
	if (!m_hHook)
	{
		Assert(0);
		return false;
	}

	m_hRope = CRopeKeyframe::Create(this, m_hHook, 1, 0);

	if (m_hRope)
	{
		m_hRope->m_Width = 2;
		m_hRope->m_nSegments = ROPE_MAX_SEGMENTS;
		m_hRope->EnableWind(false);
		m_hRope->EnableCollision(); // Collision looks worse than no collision
		m_hRope->SetupHangDistance(0);
	}

	return true;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Fires the hook
//-----------------------------------------------------------------------------
void CWeaponGrapple::FireHook(void)
{
	if (m_bMustReload)
		return;
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (pOwner == NULL)
		return;
#ifndef CLIENT_DLL
	Vector vecAiming = pOwner->GetAutoaimVector(0);
	Vector vecSrc = pOwner->Weapon_ShootPosition();
	QAngle angAiming;
	VectorAngles(vecAiming, angAiming);
	CGrappleHook *pHook = CGrappleHook::HookCreate(vecSrc, angAiming, this);
	if (pOwner->GetWaterLevel() == 3)
	{
		pHook->SetAbsVelocity(vecAiming * BOLT_WATER_VELOCITY);
	}
	else
	{
		pHook->SetAbsVelocity(vecAiming * BOLT_AIR_VELOCITY);
	}
	m_hHook = pHook;

	CreateRope();

#endif
	pOwner->ViewPunch(QAngle(-2, 0, 0));
	WeaponSound( SINGLE );
	//WeaponSound( SPECIAL2 );
	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;


	DoLoadEffect();
	SetChargerState(CHARGER_STATE_DISCHARGE);
}
//-----------------------------------------------------------------------------
// Purpose:
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Deploy(void)
{
	if (m_bMustReload)
	{
		return DefaultDeploy((char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix());
	}

	SetSkin(BOLT_SKIN_GLOW);

	return BaseClass::Deploy();
}
//-----------------------------------------------------------------------------
// Purpose:
// Input : *pSwitchingTo -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Holster(CBaseCombatWeapon *pSwitchingTo)
{
#ifndef CLIENT_DLL
	if (m_hHook)
	{
		m_hHook->SetTouch(NULL);
		m_hHook->SetThink(NULL);
		UTIL_Remove(m_hHook);
		m_hHook = NULL;
		NotifyHookDied();
		m_bMustReload = true;
	}
#endif
	return BaseClass::Holster(pSwitchingTo);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::Drop(const Vector &vecVelocity)
{
#ifndef CLIENT_DLL
	if (m_hHook)
	{
		m_hHook->SetTouch(NULL);
		m_hHook->SetThink(NULL);
		UTIL_Remove(m_hHook);
		m_hHook = NULL;
		NotifyHookDied();
		m_bMustReload = true;
	}
#endif
	BaseClass::Drop(vecVelocity);
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponGrapple::HasAnyAmmo(void)
{
	if (m_hHook != NULL)
		return true;
	return BaseClass::HasAnyAmmo();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CWeaponGrapple::CanHolster(void)
{
	//Can't have an active hook out
	if (m_hHook != NULL)
		return false;
	return BaseClass::CanHolster();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::NotifyHookDied(void)
{
	m_hHook = NULL;
#ifndef CLIENT_DLL
	if (m_hRope)
	{
		UTIL_Remove(m_hRope); //Kill beam
		m_hRope = NULL;
	}
#endif
}
//-----------------------------------------------------------------------------
// Purpose:
// Input : &tr - used to figure out where to do the effect
// nDamageType - ???
//-----------------------------------------------------------------------------
void CWeaponGrapple::DoImpactEffect(trace_t &tr, int nDamageType)
{
#ifndef CLIENT_DLL
	if ((tr.surface.flags & SURF_SKY) == false)
	{
		CPVSFilter filter(tr.endpos);
		te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);
		m_nBulletType = GetAmmoDef()->Index("GaussEnergy");
		UTIL_ImpactTrace(&tr, m_nBulletType);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::DoLoadEffect(void)
{
	SetSkin(BOLT_SKIN_GLOW);

	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if (pViewModel == NULL)
		return;

	CEffectData	data;

#ifdef CLIENT_DLL
	data.m_hEntity = pViewModel->GetRefEHandle();
#else
	data.m_nEntIndex = pViewModel->entindex();
#endif
	data.m_nAttachmentIndex = 1;

	DispatchEffect("CrossbowLoad", data);

#ifndef CLIENT_DLL

	CSprite *pBlast = CSprite::SpriteCreate(CROSSBOW_GLOW_SPRITE2, GetAbsOrigin(), false);

	if (pBlast)
	{
		pBlast->SetAttachment(pOwner->GetViewModel(), 1);
		pBlast->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
		pBlast->SetBrightness(128);
		pBlast->SetScale(0.2f);
		pBlast->FadeOutFromSpawn();
	}
#endif

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponGrapple::SetChargerState(ChargerState_t state)
{
	// Make sure we're setup
	CreateChargerEffects();

	// Don't do this twice
	if (state == m_nChargeState)
		return;

	m_nChargeState = state;

	switch (m_nChargeState)
	{
	case CHARGER_STATE_START_LOAD:

		WeaponSound(SPECIAL1);

		// Shoot some sparks and draw a beam between the two outer points
		DoLoadEffect();

		break;
#ifndef CLIENT_DLL
	case CHARGER_STATE_START_CHARGE:
	{
		if (m_hChargerSprite == NULL)
			break;

		m_hChargerSprite->SetBrightness(32, 0.5f);
		m_hChargerSprite->SetScale(0.025f, 0.5f);
		m_hChargerSprite->TurnOn();
	}

	break;

	case CHARGER_STATE_READY:
	{
		// Get fully charged
		if (m_hChargerSprite == NULL)
			break;

		m_hChargerSprite->SetBrightness(80, 1.0f);
		m_hChargerSprite->SetScale(0.1f, 0.5f);
		m_hChargerSprite->TurnOn();
	}

	break;

	case CHARGER_STATE_DISCHARGE:
	{
		SetSkin(BOLT_SKIN_NORMAL);

		if (m_hChargerSprite == NULL)
			break;

		m_hChargerSprite->SetBrightness(0);
		m_hChargerSprite->TurnOff();
	}

	break;
#endif
	case CHARGER_STATE_OFF:
	{
		SetSkin(BOLT_SKIN_NORMAL);

#ifndef CLIENT_DLL
		if (m_hChargerSprite == NULL)
			break;

		m_hChargerSprite->SetBrightness(0);
		m_hChargerSprite->TurnOff();
#endif
	}
	break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : skinNum - 
//-----------------------------------------------------------------------------
void CWeaponGrapple::SetSkin(int skinNum)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if (pViewModel == NULL)
		return;
#ifndef CLIENT_DLL
	pViewModel->m_nSkin = skinNum;
#else
	pViewModel->SetSkin(skinNum);
#endif
}

#define	BOLT_TIP_ATTACHMENT	2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::CreateChargerEffects(void)
{
#ifndef CLIENT_DLL
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (m_hChargerSprite != NULL)
		return;

	m_hChargerSprite = CSprite::SpriteCreate(CROSSBOW_GLOW_SPRITE, GetAbsOrigin(), false);

	if (m_hChargerSprite)
	{
		m_hChargerSprite->SetAttachment(pOwner->GetViewModel(), BOLT_TIP_ATTACHMENT);
		m_hChargerSprite->SetTransparency(kRenderTransAdd, 255, 128, 0, 255, kRenderFxNoDissipation);
		m_hChargerSprite->SetBrightness(0);
		m_hChargerSprite->SetScale(0.1f);
		m_hChargerSprite->TurnOff();
	}
#endif
}