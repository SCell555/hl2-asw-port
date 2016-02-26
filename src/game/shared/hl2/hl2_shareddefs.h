//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2_SHAREDDEFS_H
#define HL2_SHAREDDEFS_H

#ifdef _WIN32
#pragma once
#endif

#include "const.h"

//#include "asw_shareddefs.h"
//--------------------------------------------------------------------------
// Collision groups
//--------------------------------------------------------------------------

enum
{
	HL2COLLISION_GROUP_PLASMANODE = LAST_SHARED_COLLISION_GROUP,
	HL2COLLISION_GROUP_SPIT,
	HL2COLLISION_GROUP_HOMING_MISSILE,
	HL2COLLISION_GROUP_COMBINE_BALL,

	HL2COLLISION_GROUP_FIRST_NPC,
	HL2COLLISION_GROUP_HOUNDEYE,
	HL2COLLISION_GROUP_CROW,
	HL2COLLISION_GROUP_HEADCRAB,
	HL2COLLISION_GROUP_STRIDER,
	HL2COLLISION_GROUP_GUNSHIP,
	HL2COLLISION_GROUP_ANTLION,
	HL2COLLISION_GROUP_LAST_NPC,
	HL2COLLISION_GROUP_COMBINE_BALL_NPC,
	 

};


//--------------
// HL2 SPECIFIC
//--------------
#define DMG_SNIPER			(DMG_LASTGENERICFLAG<<1)	// This is sniper damage
#define DMG_MISSILEDEFENSE	(DMG_LASTGENERICFLAG<<2)	// The only kind of damage missiles take. (special missile defense)


#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_ColorGradingData );
#else
EXTERN_SEND_TABLE( DT_ColorGradingData );
#endif

struct ColorGradingData_t
{
	ColorGradingData_t();

	DECLARE_CLASS_NOBASE( ColorGradingData_t );
	DECLARE_SIMPLE_DATADESC();
	DECLARE_EMBEDDED_NETWORKVAR();

	CNetworkVar( Vector, contrast );
	CNetworkVar( Vector, brightness );

	CNetworkVar( float, levels_r_min_input);
	CNetworkVar( float, levels_r_max_input);
	CNetworkVar( float, levels_r_min_output);
	CNetworkVar( float, levels_r_max_output);

	CNetworkVar( float, levels_g_min_input);
	CNetworkVar( float, levels_g_max_input);
	CNetworkVar( float, levels_g_min_output);
	CNetworkVar( float, levels_g_max_output);

	CNetworkVar( float, levels_b_min_input);
	CNetworkVar( float, levels_b_max_input);
	CNetworkVar( float, levels_b_min_output);
	CNetworkVar( float, levels_b_max_output);

	CNetworkVar( float, saturation );
	CNetworkVar( float, gamma );
};
#endif // HL2_SHAREDDEFS_H
