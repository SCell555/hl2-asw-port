#ifndef ENV_PROJECTEDTEXTURE_H
#define ENV_PROJECTEDTEXTURE_H
#ifdef _WIN32
#pragma once
#endif

#define ENV_PROJECTEDTEXTURE_STARTON			(1<<0)
#define ENV_PROJECTEDTEXTURE_ALWAYSUPDATE		(1<<1)
#define ENV_PROJECTEDTEXTURE_VOLUMETRIC			(1<<2)
#define ENV_PROJECTEDTEXTURE_CAMPFIRE_MODE		(1<<3)
#define ENV_PROJECTEDTEXTURE_UBERLIGHT			(1<<4)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvProjectedTexture : public CPointEntity
{
	DECLARE_CLASS( CEnvProjectedTexture, CPointEntity );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvProjectedTexture();
	bool KeyValue( const char *szKeyName, const char *szValue );
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );

	// Always transmit to clients
	virtual int UpdateTransmitState();
	virtual void Activate( void );
	virtual void Spawn( void );

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputAlwaysUpdateOn( inputdata_t &inputdata );
	void InputAlwaysUpdateOff( inputdata_t &inputdata );
	void InputSetFOV( inputdata_t &inputdata );
	void InputSetTarget( inputdata_t &inputdata );
	void InputSetCameraSpace( inputdata_t &inputdata );
	void InputSetLightOnlyTarget( inputdata_t &inputdata );
	void InputSetLightWorld( inputdata_t &inputdata );
	void InputSetEnableShadows( inputdata_t &inputdata );
	void InputSetLightColor( inputdata_t &inputdata );
	void InputSetSpotlightTexture( inputdata_t &inputdata );
	void InputSetAmbient( inputdata_t &inputdata );
	void InputEnableVolumetrics( inputdata_t &inputdata );
	void InputDisableVolumetrics( inputdata_t &inputdata );
	void InputSetVolumetricIntence( inputdata_t &inputdata );
	void InputEnableUberLight( inputdata_t &inputdata );
	void InputDisableUberLight( inputdata_t &inputdata );

	void InitialThink( void );

	CNetworkHandle( CBaseEntity, m_hTargetEntity );

private:

	CNetworkVar( bool, m_bState );
	CNetworkVar( bool, m_bAlwaysUpdate );
	CNetworkVar( bool, m_bUberLightEnabled );
	CNetworkVar( float, m_flLightFOV );
	CNetworkVar( bool, m_bEnableShadows );
	CNetworkVar( bool, m_bSimpleProjection );
	CNetworkVar( bool, m_bLightOnlyTarget );
	CNetworkVar( bool, m_bLightWorld );
	CNetworkVar( bool, m_bCameraSpace );
	CNetworkVar( float, m_flBrightnessScale );
	CNetworkColor32( m_LightColor );
	CNetworkVar( float, m_flColorTransitionTime );
	CNetworkVar( float, m_flAmbient );
	CNetworkString( m_SpotlightTextureName, MAX_PATH );
	CNetworkVar( int, m_nSpotlightTextureFrame );
	CNetworkVar( float, m_flNearZ );
	CNetworkVar( float, m_flFarZ );
	CNetworkVar( int, m_nShadowQuality );
	CNetworkVar( int, m_nCampfireColorChangeMode );
	CNetworkVar( float, m_flProjectionSize );
	CNetworkVar( float, m_flRotation );
	CNetworkVar( float, m_flVolIntence );
	CNetworkVar( float, m_flCampfireSwayAmplitude );
	CNetworkVar( float, m_flCampfireBrightnessAmp );
	CNetworkVar( float, m_flCampfireSwaySpeed );
	CNetworkVar( float, m_flCampfireColorChangeAmp );
	CNetworkVar( bool, m_bEnableVolumetrics );
	CNetworkVar( bool, m_bEnableCampfireMode );
	CNetworkVar( float, m_flUberLightRoundness );
	CNetworkVar( float, m_flUberLightFalloffdist );
	CNetworkVar( float, m_flUberLightWedge );
	CNetworkVar( float, m_flUberLightHedge );
	CNetworkVar( float, m_flUberLightShearx );
	CNetworkVar( float, m_flUberLightSheary );
};


#endif	// ENV_PROJECTEDTEXTURE_H