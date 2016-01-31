//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Sunlight shadow control entity.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "c_baseplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar cl_sunlight_ortho_size;
extern ConVar cl_sunlight_depthbias;

ConVar cl_globallight_enabled( "cl_globallight_enabled", "0", FCVAR_ARCHIVE);
ConVar cl_globallight_freeze( "cl_globallight_freeze", "0" ); //debug
ConVar cl_globallight_xoffset( "cl_globallight_xoffset", "0" );
ConVar cl_globallight_yoffset( "cl_globallight_yoffset", "0" );
ConVar cl_globallight_drawfrustum( "cl_globallight_drawfrustum", "0" );
ConVar cl_globallight_orthosize( "cl_globallight_orthosize", "1000" );
ConVar cl_globallight_showpos( "cl_globallight_showpos", "0" );
ConVar cl_globallight_xpos( "cl_globallight_xpos", "0" );
ConVar cl_globallight_ypos( "cl_globallight_ypos", "0" );

//------------------------------------------------------------------------------
// Purpose : Sunlights shadow control entity
//------------------------------------------------------------------------------
class C_GlobalLight : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_GlobalLight, C_BaseEntity );
	DECLARE_DATADESC();
	DECLARE_CLIENTCLASS();

	C_GlobalLight();
	virtual ~C_GlobalLight();

	void OnDataChanged( DataUpdateType_t updateType );
	void Spawn();
	bool ShouldDraw();

	void ClientThink();

private:
	Vector m_shadowDirection;
	Vector m_vMoveDirection;
	bool m_bEnabled;
	bool m_bVolumetricsEnabled;
	bool m_bMovingEnabled;
	bool m_bInitMoveVars;
	char m_TextureName[ MAX_PATH ];
	CTextureReference m_SpotlightTexture;
	color32	m_LightColor;
	Vector m_CurrentLinearFloatLightColor;
	float m_flCurrentLinearFloatLightAlpha;
	float m_flColorTransitionTime;
	float m_flSunDistance;
	float m_flFOV;
	float m_flNearZ;
	float m_flNorthOffset;
	float m_flVolIntence;
	float m_flVolNumPlanes;
	float m_flMoveSpeed;
	bool m_bEnableShadows;
	bool m_bOldEnableShadows;

	QAngle movangles;

	static ClientShadowHandle_t m_LocalFlashlightHandle;
};


ClientShadowHandle_t C_GlobalLight::m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;


IMPLEMENT_CLIENTCLASS_DT(C_GlobalLight, DT_GlobalLight, CGlobalLight)
	RecvPropVector(RECVINFO(m_shadowDirection)),
	RecvPropBool(RECVINFO(m_bEnabled)),
	RecvPropBool(RECVINFO(m_bVolumetricsEnabled)),
	RecvPropBool(RECVINFO(m_bMovingEnabled)),
	RecvPropString(RECVINFO(m_TextureName)),
	RecvPropInt(RECVINFO(m_LightColor), 0, RecvProxy_Int32ToColor32),
	RecvPropFloat(RECVINFO(m_flColorTransitionTime)),
	RecvPropFloat(RECVINFO(m_flSunDistance)),
	RecvPropFloat(RECVINFO(m_flFOV)),
	RecvPropFloat(RECVINFO(m_flNearZ)),
	RecvPropFloat(RECVINFO(m_flNorthOffset)),
	RecvPropFloat(RECVINFO(m_flVolIntence)),
	RecvPropFloat(RECVINFO(m_flVolNumPlanes)),
	RecvPropFloat(RECVINFO(m_flMoveSpeed)),
	RecvPropBool(RECVINFO(m_bEnableShadows)),
END_RECV_TABLE()

BEGIN_DATADESC( C_GlobalLight )
	DEFINE_FIELD( m_vMoveDirection, FIELD_VECTOR ),
	DEFINE_FIELD( m_bInitMoveVars, FIELD_BOOLEAN ),
	DEFINE_FIELD( movangles, FIELD_VECTOR ),
END_DATADESC()

C_GlobalLight::C_GlobalLight()
{
	m_bInitMoveVars = true;

	movangles = QAngle( 0, 0, 0 );
}

C_GlobalLight::~C_GlobalLight()
{
	if ( m_LocalFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LocalFlashlightHandle );
		m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void C_GlobalLight::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_SpotlightTexture.Init( m_TextureName, TEXTURE_GROUP_OTHER, true );
	}

	BaseClass::OnDataChanged( updateType );
}

void C_GlobalLight::Spawn()
{
	BaseClass::Spawn();

	m_bOldEnableShadows = m_bEnableShadows;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_GlobalLight::ShouldDraw()
{
	return false;
}

void C_GlobalLight::ClientThink()
{
	VPROF("C_GlobalLight::ClientThink");

	if ( m_bEnabled && cl_globallight_enabled.GetBool())
		{
			Vector vLinearFloatLightColor( m_LightColor.r, m_LightColor.g, m_LightColor.b );
			float flLinearFloatLightAlpha = m_LightColor.a;

		if ( m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha )
		{
			float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

			m_CurrentLinearFloatLightColor.x = Approach( vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.y = Approach( vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed );
			m_CurrentLinearFloatLightColor.z = Approach( vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed );
			m_flCurrentLinearFloatLightAlpha = Approach( flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed );
		}

		FlashlightState_t state;

		// moving light (like sun or moon)
		// convert shadow direction vector to angles
		// then add a special value to pitch
		// then convert angles back to shadow direction vector
		if( m_bMovingEnabled )
		{
			if( m_bInitMoveVars )
			{
				VectorAngles( m_shadowDirection, movangles );

				m_bInitMoveVars = false;
			}

			movangles.x = Approach( movangles.x + 1, movangles.x, gpGlobals->frametime * m_flMoveSpeed );

			if( movangles.x == 180 )
			{
				movangles.x = -180;
			}

			AngleVectors( movangles, &m_vMoveDirection );
		}

		Vector vDirection = m_bMovingEnabled ? m_vMoveDirection : m_shadowDirection;
		VectorNormalize( vDirection );

		//Vector vViewUp = Vector( 0.0f, 1.0f, 0.0f );
		Vector vSunDirection2D = vDirection;
		vSunDirection2D.z = 0.0f;

		HACK_GETLOCALPLAYER_GUARD( "C_GlobalLight::ClientThink" );

		if ( !C_BasePlayer::GetLocalPlayer() )
			return;

		Vector vPos;
		QAngle EyeAngles;
		float flZNear, flZFar, flFov;

		C_BasePlayer::GetLocalPlayer()->CalcView( vPos, EyeAngles, flZNear, flZFar, flFov );
//		vPos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
		
//		vPos = Vector( 0.0f, 0.0f, 500.0f );
		vPos = ( vPos + vSunDirection2D * m_flNorthOffset ) - vDirection * m_flSunDistance;
		vPos += Vector( cl_globallight_xoffset.GetFloat(), cl_globallight_yoffset.GetFloat(), 0.0f );
		
		if (cl_globallight_showpos.GetBool() == true)
		{
			if (cl_globallight_xpos.GetFloat() !=0 &&  cl_globallight_ypos.GetFloat() !=0) 
			{
				DevMsg("X = %3.0f\n Y = %3.0f\n", cl_globallight_xpos.GetFloat(), cl_globallight_ypos.GetFloat());
			}
			else 
				DevMsg("X = %3.0f\n Y = %3.0f\n", vPos.x, vPos.y);
		}
		if (cl_globallight_xpos.GetFloat() !=0 &&  cl_globallight_ypos.GetFloat() !=0) 
		{
			vPos.x = cl_globallight_xpos.GetFloat();
			vPos.y = cl_globallight_ypos.GetFloat();
		}
		if (cl_globallight_freeze.GetBool())
			return;

		QAngle angAngles;
		VectorAngles( vDirection, angAngles );

		Vector vForward, vRight, vUp;
		AngleVectors( angAngles, &vForward, &vRight, &vUp );

		state.m_fHorizontalFOVDegrees = m_flFOV;
		state.m_fVerticalFOVDegrees = m_flFOV;

		state.m_vecLightOrigin = vPos;
		BasisToQuaternion( vForward, vRight, vUp, state.m_quatOrientation );

		state.m_fQuadraticAtten = 1.0f;
		state.m_fLinearAtten = m_flSunDistance * 2.0f;
		state.m_fConstantAtten = 1.0f;
		state.m_FarZAtten = m_flSunDistance * 2.0f;
		state.m_Color[0] = m_CurrentLinearFloatLightColor.x * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
		state.m_Color[1] = m_CurrentLinearFloatLightColor.y * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
		state.m_Color[2] = m_CurrentLinearFloatLightColor.z * ( 1.0f / 255.0f ) * m_flCurrentLinearFloatLightAlpha;
		state.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
		state.m_NearZ = 4.0f;
		state.m_FarZ = m_flSunDistance * 2.0f;
		state.m_fBrightnessScale = 1.0f;
		state.m_bGlobalLight = true;

		float flOrthoSize = cl_globallight_orthosize.GetFloat();

		if ( flOrthoSize > 0 )
		{
			state.m_bOrtho = true;
			state.m_fOrthoLeft = -flOrthoSize;
			state.m_fOrthoTop = -flOrthoSize;
			state.m_fOrthoRight = flOrthoSize;
			state.m_fOrthoBottom = flOrthoSize;
		}
		else
		{
			state.m_bOrtho = false;
		}

		state.m_bDrawShadowFrustum = cl_globallight_drawfrustum.GetBool();
		state.m_flShadowSlopeScaleDepthBias = 2;// g_pMaterialSystemHardwareConfig->GetShadzowSlopeScaleDepthBias();
		state.m_flShadowDepthBias = 0.0005f;	// g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
		state.m_bEnableShadows = m_bEnableShadows;
		state.m_pSpotlightTexture = m_SpotlightTexture;
		state.m_pProjectedMaterial = NULL; // don't complain cause we aren't using simple projection in this class
		state.m_nSpotlightTextureFrame = 0;
		state.m_flShadowFilterSize = 0.2f;
		state.m_nShadowQuality = 1; // Allow entity to affect shadow quality
		state.m_bShadowHighRes = true;
		state.m_flShadowAtten = 0.8f;

		//volumetric light tweaks
		state.m_bVolumetric = m_bVolumetricsEnabled;
		state.m_nNumPlanes = m_flVolNumPlanes;
		state.m_flVolumetricIntensity = m_flVolIntence;

		if ( m_bOldEnableShadows != m_bEnableShadows )
		{
			// If they change the shadow enable/disable, we need to make a new handle
			if ( m_LocalFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				g_pClientShadowMgr->DestroyFlashlight( m_LocalFlashlightHandle );
				m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
			}

			m_bOldEnableShadows = m_bEnableShadows;
		}

		if( m_LocalFlashlightHandle == CLIENTSHADOW_INVALID_HANDLE )
		{
			m_LocalFlashlightHandle = g_pClientShadowMgr->CreateFlashlight( state );
		}
		else
		{
			g_pClientShadowMgr->UpdateFlashlightState( m_LocalFlashlightHandle, state );
			g_pClientShadowMgr->UpdateProjectedTexture( m_LocalFlashlightHandle, true );
		}
	}
	else if ( m_LocalFlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LocalFlashlightHandle );
		m_LocalFlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
	BaseClass::ClientThink();
}
