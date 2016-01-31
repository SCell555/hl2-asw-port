//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_env_projectedtexture.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

float C_EnvProjectedTexture::m_flVisibleBBoxMinHeight = -FLT_MAX;

IMPLEMENT_CLIENTCLASS_DT( C_EnvProjectedTexture, DT_EnvProjectedTexture, CEnvProjectedTexture )
	RecvPropEHandle( RECVINFO( m_hTargetEntity )	),
	RecvPropBool(	 RECVINFO( m_bState )			),
	RecvPropBool(	 RECVINFO( m_bAlwaysUpdate )	),
	RecvPropBool(	 RECVINFO( m_bUberLightEnabled ) ),
	RecvPropFloat(	 RECVINFO( m_flLightFOV )		),
	RecvPropBool(	 RECVINFO( m_bEnableShadows )	),
	RecvPropBool(	 RECVINFO( m_bSimpleProjection )	),
	RecvPropBool(	 RECVINFO( m_bLightOnlyTarget ) ),
	RecvPropBool(	 RECVINFO( m_bLightWorld )		),
	RecvPropBool(	 RECVINFO( m_bCameraSpace )		),
	RecvPropBool(	 RECVINFO( m_bEnableCampfireMode )	),
	RecvPropFloat(	 RECVINFO( m_flBrightnessScale )	),
	RecvPropInt(	 RECVINFO( m_LightColor ), 0, RecvProxy_Int32ToColor32 ),
	RecvPropFloat(	 RECVINFO( m_flColorTransitionTime )		),
	RecvPropFloat(	 RECVINFO( m_flAmbient )		),
	RecvPropString(  RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt(	 RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat(	 RECVINFO( m_flNearZ )	),
	RecvPropFloat(	 RECVINFO( m_flFarZ )	),
	RecvPropInt(	 RECVINFO( m_nShadowQuality )	),
	RecvPropFloat(	 RECVINFO( m_flProjectionSize )	),
	RecvPropFloat(	 RECVINFO( m_flRotation )	),
	RecvPropFloat(	 RECVINFO( m_flVolIntence )	),
	RecvPropBool(	 RECVINFO( m_bEnableVolumetrics )	),
	RecvPropInt( RECVINFO( m_nCampfireColorChangeMode ) ),
	RecvPropFloat( RECVINFO( m_flCampfireSwayAmplitude ) ),
	RecvPropFloat( RECVINFO( m_flCampfireBrightnessAmp ) ),
	RecvPropFloat( RECVINFO( m_flCampfireSwaySpeed ) ),
	RecvPropFloat( RECVINFO( m_flCampfireColorChangeAmp ) ),
	RecvPropFloat( RECVINFO( m_flUberLightRoundness ) ),
	RecvPropFloat( RECVINFO( m_flUberLightFalloffdist ) ),
	RecvPropFloat( RECVINFO( m_flUberLightWedge ) ),
	RecvPropFloat( RECVINFO( m_flUberLightHedge ) ),
	RecvPropFloat( RECVINFO( m_flUberLightShearx ) ),
	RecvPropFloat( RECVINFO( m_flUberLightSheary ) ),
END_RECV_TABLE()

C_EnvProjectedTexture *C_EnvProjectedTexture::Create( )
{
	C_EnvProjectedTexture *pEnt = new C_EnvProjectedTexture();

	pEnt->m_flNearZ = 4.0f;
	pEnt->m_flFarZ = 2000.0f;
//	strcpy( pEnt->m_SpotlightTextureName, "particle/rj" );
	pEnt->m_bLightWorld = true;
	pEnt->m_bLightOnlyTarget = false;
	pEnt->m_bSimpleProjection = false;
	pEnt->m_nShadowQuality = 1;
	pEnt->m_flLightFOV = 10.0f;
	pEnt->m_LightColor.r = 255;
	pEnt->m_LightColor.g = 255;
	pEnt->m_LightColor.b = 255;
	pEnt->m_LightColor.a = 255;
	pEnt->m_bEnableShadows = false;
	pEnt->m_bEnableVolumetrics = false;
	pEnt->m_flColorTransitionTime = 1.0f;
	pEnt->m_bCameraSpace = false;
	pEnt->SetAbsAngles( QAngle( 90, 0, 0 ) );
	pEnt->m_bAlwaysUpdate = true;
	pEnt->m_bState = true;
	pEnt->m_bUberLightEnabled = false;
	pEnt->m_flProjectionSize = 500.0f;
	pEnt->m_flRotation = 0.0f;
	pEnt->m_flVolIntence = 1.0f;
	pEnt->m_bEnableCampfireMode = false;
	pEnt->m_nCampfireColorChangeMode = 0;
	pEnt->m_flCampfireBrightnessAmp = 0.0f;
	pEnt->m_flCampfireColorChangeAmp = 0.0f;
	pEnt->m_flCampfireSwayAmplitude = 0.0f;
	pEnt->m_flCampfireSwaySpeed = 0.0f;
	pEnt->m_flUberLightRoundness = 1.0f;
	pEnt->m_flUberLightFalloffdist = 128.0f;
	pEnt->m_flUberLightWedge = 0.05f;
	pEnt->m_flUberLightHedge = 0.0f;
	pEnt->m_flUberLightShearx = 0.0f;
	pEnt->m_flUberLightSheary = 0.0f;

	return pEnt;
}

C_EnvProjectedTexture::C_EnvProjectedTexture( void )
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_bForceUpdate = true;
	bFlickFirstTime = true;
	m_pMaterial = NULL;

	flTargetXFlickerPos = 0.0f;
	flTargetZFlickerPos = 0.0f;
	flTargetBrightness = 0.0f;
	flOriginalBrightness = 0.0f;
	flColorY = 0.0f;

	iTargetColorY = 0;

	vecOriginalPos = GetAbsOrigin();	// Basically, I can redo that and make an ability to parent flickering projectedtexture, but we don't need that 
										// feature for now, so I just don't want to waste my time;

	AddToEntityList( ENTITY_LIST_SIMULATE );
}

C_EnvProjectedTexture::~C_EnvProjectedTexture( void )
{
	ShutDownLightHandle();
}

void C_EnvProjectedTexture::ShutDownLightHandle( void )
{
	// Clear out the light
	if( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		if ( m_bSimpleProjection == true )
		{
			g_pClientShadowMgr->DestroyProjection( m_LightHandle );
		}
		else
		{
			g_pClientShadowMgr->DestroyFlashlight( m_LightHandle );
		}
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}


void C_EnvProjectedTexture::SetMaterial( IMaterial *pMaterial )
{
	m_pMaterial = pMaterial;
}


void C_EnvProjectedTexture::SetLightColor( byte r, byte g, byte b, byte a )
{
	m_LightColor.r = r;
	m_LightColor.g = g;
	m_LightColor.b = b;
	m_LightColor.a = a;
}


void C_EnvProjectedTexture::SetSize( float flSize )
{
	m_flProjectionSize = flSize;
}

// add to rotation
void C_EnvProjectedTexture::SetRotation( float flRotation )
{
	if ( m_flRotation != flRotation )
	{
		m_flRotation = flRotation;
//		m_bForceUpdate = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_EnvProjectedTexture::OnDataChanged( DataUpdateType_t updateType )
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_SpotlightTexture.Init( m_SpotlightTextureName, TEXTURE_GROUP_OTHER, true );
	}

	m_bForceUpdate = true;
	UpdateLight();
	BaseClass::OnDataChanged( updateType );
}

void C_EnvProjectedTexture::UpdateLight( void )
{
	VPROF("C_EnvProjectedTexture::UpdateLight");
	bool bVisible = true;

	Vector vLinearFloatLightColor( m_LightColor.r, m_LightColor.g, m_LightColor.b );
	float flLinearFloatLightAlpha = m_LightColor.a;

	if ( m_bAlwaysUpdate || m_bEnableCampfireMode )
	{
		m_bForceUpdate = true;
	}

	if ( m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha )
	{
		float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

		m_CurrentLinearFloatLightColor.x = Approach( vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed );
		m_CurrentLinearFloatLightColor.y = Approach( vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed );
		m_CurrentLinearFloatLightColor.z = Approach( vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed );
		m_flCurrentLinearFloatLightAlpha = Approach( flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed );

		m_bForceUpdate = true;
	}
	
	if ( !m_bForceUpdate )
	{
		bVisible = IsBBoxVisible();		
	}

	if ( m_bState == false || !bVisible )
	{
		// Spotlight's extents aren't in view
		ShutDownLightHandle();

		return;
	}

	if ( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE || m_hTargetEntity != NULL || m_bForceUpdate )
	{
		bool doflameflicker = m_bEnableCampfireMode;

		Vector vForward, vRight, vUp, vPos = GetAbsOrigin();

		if( doflameflicker )
		{
			if( bFlickFirstTime )
			{
				vecOriginalPos = GetAbsOrigin();

				// properly init our random values
				flTargetZFlickerPos = vPos.z + random->RandomFloat( -m_flCampfireSwayAmplitude, m_flCampfireSwayAmplitude );
				flTargetXFlickerPos = vPos.x + random->RandomFloat( -m_flCampfireSwayAmplitude, m_flCampfireSwayAmplitude );

				flOriginalBrightness = m_flBrightnessScale;
				flTargetBrightness = m_flBrightnessScale + random->RandomFloat( -m_flCampfireBrightnessAmp, m_flCampfireBrightnessAmp );

				flColorY = m_CurrentLinearFloatLightColor.y;
				iTargetColorY = m_CurrentLinearFloatLightColor.y + random->RandomFloat( -m_flCampfireColorChangeAmp, m_flCampfireColorChangeAmp );

				bFlickFirstTime = false;
			}

			// catch up our origin, don't let us go too far away from spawn point
			if( ( vPos - vecOriginalPos ).Length() > m_flCampfireSwayAmplitude )
			{
				flTargetZFlickerPos = vecOriginalPos.z;
				flTargetXFlickerPos = vecOriginalPos.x;
			}

			// smoothly and randomly modify our origin
			if( vPos.z != flTargetZFlickerPos )
			{
				vPos.z = Approach( flTargetZFlickerPos, vPos.z, gpGlobals->frametime * m_flCampfireSwaySpeed );
			}
			else
			{
				flTargetZFlickerPos = vPos.z + random->RandomFloat( -m_flCampfireSwayAmplitude, m_flCampfireSwayAmplitude );
			}
			
			if( vPos.x != flTargetXFlickerPos )
			{
				vPos.x = Approach( flTargetXFlickerPos, vPos.x, gpGlobals->frametime * m_flCampfireSwaySpeed );
			}
			else
			{
				flTargetXFlickerPos = vPos.x + random->RandomFloat( -m_flCampfireSwayAmplitude, m_flCampfireSwayAmplitude );
			}
			
			SetAbsOrigin( vPos );

			// color flicker
			if( m_nCampfireColorChangeMode > 0 )
			{
				if( flColorY != iTargetColorY )
				{
					if( ( iTargetColorY - flColorY ) > m_flCampfireColorChangeAmp || ( flColorY - iTargetColorY ) > m_flCampfireColorChangeAmp )
					{
						iTargetColorY = m_CurrentLinearFloatLightColor.y;
					}
				
					flColorY = Approach( (float) iTargetColorY, flColorY, gpGlobals->frametime * m_flCampfireSwaySpeed * 2 );
				}
				else
				{
					float randomfloat = 0;

					if( m_nCampfireColorChangeMode < 3 )
					{
						randomfloat = m_nCampfireColorChangeMode > 1 ? random->RandomFloat( -m_flCampfireColorChangeAmp, 0 ) : random->RandomFloat( 0, m_flCampfireColorChangeAmp );
					}
					else
					{
						randomfloat = random->RandomFloat( -m_flCampfireColorChangeAmp, m_flCampfireColorChangeAmp );
					}

					iTargetColorY = m_CurrentLinearFloatLightColor.y + randomfloat;
				}
			}

			// brightness flicker
			if( m_flCampfireBrightnessAmp > 0 )
			{
				if( flTargetBrightness != m_flBrightnessScale )
				{
					if( ( flTargetBrightness - flOriginalBrightness ) > m_flCampfireBrightnessAmp || ( flOriginalBrightness - flTargetBrightness ) > m_flCampfireBrightnessAmp )
					{
						flTargetBrightness = flOriginalBrightness;
					}

					m_flBrightnessScale = Approach( flTargetBrightness, m_flBrightnessScale, gpGlobals->frametime * m_flCampfireSwaySpeed / 10 );
				}
				else
				{
					flTargetBrightness = m_flBrightnessScale + random->RandomFloat( -m_flCampfireBrightnessAmp, m_flCampfireBrightnessAmp );
				}
			}
		}

		FlashlightState_t state;

		if ( m_hTargetEntity != NULL )
		{
			if ( m_bCameraSpace )
			{
				const QAngle &angles = GetLocalAngles();

				C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

				if( pPlayer )
				{
					const QAngle playerAngles = pPlayer->GetAbsAngles();

					Vector vPlayerForward, vPlayerRight, vPlayerUp;
					AngleVectors( playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp );

					matrix3x4_t	mRotMatrix;
					AngleMatrix( angles, mRotMatrix );

					VectorITransform( vPlayerForward, mRotMatrix, vForward );
					VectorITransform( vPlayerRight, mRotMatrix, vRight );
					VectorITransform( vPlayerUp, mRotMatrix, vUp );

					float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
					vPos = m_hTargetEntity->GetAbsOrigin() - vForward*dist;

					VectorNormalize( vForward );
					VectorNormalize( vRight );
					VectorNormalize( vUp );
				}
			}
			else
			{
				vForward = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
				VectorNormalize( vForward );

				// JasonM - unimplemented
				Assert (0);

				//Quaternion q = DirectionToOrientation( dir );


				//
				// JasonM - set up vRight, vUp
				//

				//			VectorNormalize( vRight );
				//			VectorNormalize( vUp );
			}
		}
		else
		{
			AngleVectors( GetAbsAngles(), &vForward, &vRight, &vUp );
		}

		state.m_fHorizontalFOVDegrees = m_flLightFOV;
		state.m_fVerticalFOVDegrees = m_flLightFOV;

		state.m_vecLightOrigin = vPos;
		BasisToQuaternion( vForward, vRight, vUp, state.m_quatOrientation );
		state.m_NearZ = m_flNearZ;
		state.m_FarZ = m_flFarZ;

		// quickly check the proposed light's bbox against the view frustum to determine whether we
		// should bother to create it, if it doesn't exist, or cull it, if it does.
		if ( m_bSimpleProjection == false )
		{
#pragma message("OPTIMIZATION: this should be made SIMD")
			// get the half-widths of the near and far planes, 
			// based on the FOV which is in degrees. Remember that
			// on planet Valve, x is forward, y left, and z up. 
			const float tanHalfAngle = tan( m_flLightFOV * ( M_PI/180.0f ) * 0.5f );
			const float halfWidthNear = tanHalfAngle * m_flNearZ;
			const float halfWidthFar = tanHalfAngle * m_flFarZ;
			// now we can build coordinates in local space: the near rectangle is eg 
			// (0, -halfWidthNear, -halfWidthNear), (0,  halfWidthNear, -halfWidthNear), 
			// (0,  halfWidthNear,  halfWidthNear), (0, -halfWidthNear,  halfWidthNear)

			VectorAligned vNearRect[4] = { 
				VectorAligned( m_flNearZ, -halfWidthNear, -halfWidthNear), VectorAligned( m_flNearZ,  halfWidthNear, -halfWidthNear),
				VectorAligned( m_flNearZ,  halfWidthNear,  halfWidthNear), VectorAligned( m_flNearZ, -halfWidthNear,  halfWidthNear) 
			};

			VectorAligned vFarRect[4] = { 
				VectorAligned( m_flFarZ, -halfWidthFar, -halfWidthFar), VectorAligned( m_flFarZ,  halfWidthFar, -halfWidthFar),
				VectorAligned( m_flFarZ,  halfWidthFar,  halfWidthFar), VectorAligned( m_flFarZ, -halfWidthFar,  halfWidthFar) 
			};

			matrix3x4_t matOrientation( vForward, -vRight, vUp, vPos );

			enum
			{
				kNEAR = 0,
				kFAR = 1,
			};
			VectorAligned vOutRects[2][4];

			for ( int i = 0 ; i < 4 ; ++i )
			{
				VectorTransform( vNearRect[i].Base(), matOrientation, vOutRects[0][i].Base() );
			}
			for ( int i = 0 ; i < 4 ; ++i )
			{
				VectorTransform( vFarRect[i].Base(), matOrientation, vOutRects[1][i].Base() );
			}

			// now take the min and max extents for the bbox, and see if it is visible.
			Vector mins = **vOutRects; 
			Vector maxs = **vOutRects; 
			for ( int i = 1; i < 8 ; ++i )
			{
				VectorMin( mins, *(*vOutRects+i), mins );
				VectorMax( maxs, *(*vOutRects+i), maxs );
			}

#if 0 //for debugging the visibility frustum we just calculated
			NDebugOverlay::Triangle( vOutRects[0][0], vOutRects[0][1], vOutRects[0][2], 255, 0, 0, 100, true, 0.0f ); //first tri
			NDebugOverlay::Triangle( vOutRects[0][2], vOutRects[0][1], vOutRects[0][0], 255, 0, 0, 100, true, 0.0f ); //make it double sided
			NDebugOverlay::Triangle( vOutRects[0][2], vOutRects[0][3], vOutRects[0][0], 255, 0, 0, 100, true, 0.0f ); //second tri
			NDebugOverlay::Triangle( vOutRects[0][0], vOutRects[0][3], vOutRects[0][2], 255, 0, 0, 100, true, 0.0f ); //make it double sided

			NDebugOverlay::Triangle( vOutRects[1][0], vOutRects[1][1], vOutRects[1][2], 0, 0, 255, 100, true, 0.0f ); //first tri
			NDebugOverlay::Triangle( vOutRects[1][2], vOutRects[1][1], vOutRects[1][0], 0, 0, 255, 100, true, 0.0f ); //make it double sided
			NDebugOverlay::Triangle( vOutRects[1][2], vOutRects[1][3], vOutRects[1][0], 0, 0, 255, 100, true, 0.0f ); //second tri
			NDebugOverlay::Triangle( vOutRects[1][0], vOutRects[1][3], vOutRects[1][2], 0, 0, 255, 100, true, 0.0f ); //make it double sided

			NDebugOverlay::Box( vec3_origin, mins, maxs, 0, 255, 0, 100, 0.0f );
#endif
			
			bool bVisible = IsBBoxVisible( mins, maxs );
			if (!bVisible)
			{
				// Spotlight's extents aren't in view
				if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
				{
					ShutDownLightHandle();
				}

				return;
			}
		}

		float flAlpha = m_flCurrentLinearFloatLightAlpha * ( 1.0f / 255.0f );

		state.m_fQuadraticAtten = 1.0;
		state.m_fLinearAtten = 100;
		state.m_fConstantAtten = 1.0f;
		state.m_FarZAtten = m_flFarZ;
		state.m_fBrightnessScale = m_flBrightnessScale;
		state.m_Color[0] = m_CurrentLinearFloatLightColor.x * ( 1.0f / 255.0f ) * flAlpha;
		state.m_Color[1] = ( doflameflicker ? flColorY : m_CurrentLinearFloatLightColor.y ) * ( 1.0f / 255.0f ) * flAlpha;
		state.m_Color[2] = m_CurrentLinearFloatLightColor.z * ( 1.0f / 255.0f ) * flAlpha;
		state.m_Color[3] = m_flAmbient;
		state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
		state.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();
		state.m_bEnableShadows = m_bEnableShadows;
		state.m_pSpotlightTexture = m_SpotlightTexture;
		state.m_pProjectedMaterial = NULL; // only complain if we're using material projection
		state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;
		state.m_flProjectionSize = m_flProjectionSize;
		state.m_flProjectionRotation = m_flRotation;

		//volumetric light tweaks
		state.m_bVolumetric = m_bEnableVolumetrics;
		state.m_flVolumetricIntensity = m_flVolIntence;
		
		state.m_bUberlight = m_bUberLightEnabled;

		if( m_bUberLightEnabled )
		{
			state.m_uberlightState.m_fRoundness = m_flUberLightRoundness;
			state.m_uberlightState.m_fNearEdge = m_flNearZ;
			state.m_uberlightState.m_fFarEdge = m_flFarZ;
			state.m_uberlightState.m_fWidth = m_flLightFOV / 90;
			state.m_uberlightState.m_fHeight = m_flLightFOV / 90;
			state.m_uberlightState.m_fCutOff = m_flFarZ + m_flUberLightFalloffdist;
			state.m_uberlightState.m_fCutOn = m_flNearZ;
			state.m_uberlightState.m_fWedge = m_flUberLightWedge;
			state.m_uberlightState.m_fHedge = m_flUberLightHedge;
			state.m_uberlightState.m_fShearx = m_flUberLightShearx;
			state.m_uberlightState.m_fSheary = m_flUberLightSheary;
		}

		state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality

		if ( m_bSimpleProjection == true )
		{
			state.m_bSimpleProjection = true;
			state.m_bOrtho = true;
			state.m_fOrthoLeft = -m_flProjectionSize;
			state.m_fOrthoTop = -m_flProjectionSize;
			state.m_fOrthoRight = m_flProjectionSize;
			state.m_fOrthoBottom = m_flProjectionSize;
		}

		if( m_LightHandle == CLIENTSHADOW_INVALID_HANDLE )
		{
			// Hack: env projected textures don't work like normal flashlights; they're not assigned to a given splitscreen slot,
			// but the flashlight code requires this
			HACK_GETLOCALPLAYER_GUARD( "Env projected texture" );
			if ( m_bSimpleProjection == true )
			{
				m_LightHandle = g_pClientShadowMgr->CreateProjection( state );
			}
			else
			{
				m_LightHandle = g_pClientShadowMgr->CreateFlashlight( state );
			}

			if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
			{
				m_bForceUpdate = false;
			}
		}
		else
		{
			if ( m_bSimpleProjection == true )
			{
				g_pClientShadowMgr->UpdateProjectionState( m_LightHandle, state );
			}
			else
			{
				g_pClientShadowMgr->UpdateFlashlightState( m_LightHandle, state );
			}
			m_bForceUpdate = false;
		}

		g_pClientShadowMgr->GetFrustumExtents( m_LightHandle, m_vecExtentsMin, m_vecExtentsMax );

		m_vecExtentsMin = m_vecExtentsMin - GetAbsOrigin();
		m_vecExtentsMax = m_vecExtentsMax - GetAbsOrigin();
	}

	if( m_bLightOnlyTarget )
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, m_hTargetEntity );
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, NULL );
	}

	g_pClientShadowMgr->SetFlashlightLightWorld( m_LightHandle, m_bLightWorld );

	if ( !m_bForceUpdate )
	{
		g_pClientShadowMgr->UpdateProjectedTexture( m_LightHandle, true );
	}
}

bool C_EnvProjectedTexture::Simulate( void )
{
	UpdateLight();

	BaseClass::Simulate();
	return true;
}

bool C_EnvProjectedTexture::IsBBoxVisible( Vector vecExtentsMin, Vector vecExtentsMax )
{
	// Z position clamped to the min height (but must be less than the max)
	float flVisibleBBoxMinHeight = MIN( vecExtentsMax.z - 1.0f, m_flVisibleBBoxMinHeight );
	vecExtentsMin.z = MAX( vecExtentsMin.z, flVisibleBBoxMinHeight );

	// Check if the bbox is in the view
	return !engine->CullBox( vecExtentsMin, vecExtentsMax );
}

