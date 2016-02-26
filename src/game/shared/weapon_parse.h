//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//

#ifndef WEAPON_PARSE_H
#define WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class IFileSystem;

typedef unsigned short WEAPON_FILE_INFO_HANDLE;

// -----------------------------------------------------------
// Weapon sound types
// Used to play sounds defined in the weapon's classname.txt file
// This needs to match pWeaponSoundCategories in weapon_parse.cpp
// ------------------------------------------------------------
typedef enum {
	EMPTY,
	SINGLE,
	SINGLE_NPC,
	WPN_DOUBLE, // Can't be "DOUBLE" because windows.h uses it.
	DOUBLE_NPC,
	BURST,
	RELOAD,
	RELOAD_NPC,
	MELEE_MISS,
	MELEE_HIT,
	MELEE_HIT_WORLD,
	SPECIAL1,
	SPECIAL2,
	SPECIAL3,
	TAUNT,
	FAST_RELOAD,

	// Add new shoot sound types here

	NUM_SHOOT_SOUND_TYPES,
} WeaponSound_t;

int GetWeaponSoundFromString( const char *pszString );

#define MAX_SHOOT_SOUNDS	16			// Maximum number of shoot sounds per shoot type

#define MAX_WEAPON_STRING	80
#define MAX_WEAPON_PREFIX	16
#define MAX_WEAPON_AMMO_NAME		32

#define WEAPON_PRINTNAME_MISSING "!!! Missing printname on weapon"

class CHudTexture;
class KeyValues;

//-----------------------------------------------------------------------------
// Purpose: Contains the data read from the weapon's script file. 
// It's cached so we only read each weapon's script file once.
// Each game provides a CreateWeaponInfo function so it can have game-specific
// data (like CS move speeds) in the weapon script.
//-----------------------------------------------------------------------------
class FileWeaponInfo_t
{
public:

	FileWeaponInfo_t();
	
	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse( KeyValues *pKeyValuesData, const char *szWeaponName );

	
public:	
	bool					bParsedScript;
	bool					bLoadedHudElements;

// SHARED
	char					szClassName[MAX_WEAPON_STRING];
	char					szPrintName[MAX_WEAPON_STRING];			// Name for showing in HUD, etc.

	char					szViewModel[MAX_WEAPON_STRING];			// View model of this weapon
	char					szWorldModel[MAX_WEAPON_STRING];		// Model of this weapon seen carried by the player
	char					szAnimationPrefix[MAX_WEAPON_PREFIX];	// Prefix of the animations that should be used by the player carrying this weapon
	int						iSlot;									// inventory slot.
	int						iPosition;								// position in the inventory slot.
	int						iMaxClip1;								// max primary clip size (-1 if no clip)
	int						iMaxClip2;								// max secondary clip size (-1 if no clip)
	int						iDefaultClip1;							// amount of primary ammo in the gun when it's created
	int						iDefaultClip2;							// amount of secondary ammo in the gun when it's created
	int						iWeight;								// this value used to determine this weapon's importance in autoselection.
	int						iRumbleEffect;							// Which rumble effect to use when fired? (xbox)
	bool					bAutoSwitchTo;							// whether this weapon should be considered for autoswitching to
	bool					bAutoSwitchFrom;						// whether this weapon can be autoswitched away from when picking up another weapon or ammo
	int						iFlags;									// miscellaneous weapon flags
	char					szAmmo1[MAX_WEAPON_AMMO_NAME];			// "primary" ammo type
	char					szAmmo2[MAX_WEAPON_AMMO_NAME];			// "secondary" ammo type
	char					szAIAddOn[MAX_WEAPON_STRING];			// addon that this weapon can become

	// Sound blocks
	char					aShootSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];	

	int						iAmmoType;
	int						iAmmo2Type;
	bool					m_bMeleeWeapon;		// Melee weapons can always "fire" regardless of ammo.

	// This tells if the weapon was built right-handed (defaults to true).
	// This helps cl_righthand make the decision about whether to flip the model or not.
	bool					m_bBuiltRightHanded;	// it is built left or right handed.
	bool					m_bAllowFlipping;	// False to disallow flipping the model, regardless of whether

	Vector					vecIronsightPosOffset;
	QAngle					angIronsightAngOffset;
	float					flIronsightFOVOffset;
	bool					hasIronsight;						

	ColorRGBExp32 muzzle;

// CLIENT DLL
	// Sprite data, read from the data file
	int						iSpriteCount;
	CHudTexture						*iconActive;
	CHudTexture	 					*iconInactive;
	CHudTexture 					*iconAmmo;
	CHudTexture 					*iconAmmo2;
	CHudTexture 					*iconCrosshair;
	CHudTexture 					*iconAutoaim;
	CHudTexture 					*iconZoomedCrosshair;
	CHudTexture 					*iconZoomedAutoaim;
	CHudTexture						*iconSmall;

// TF2 specific
	bool					bShowUsageHint;							// if true, then when you receive the weapon, show a hint about it

// SERVER DLL

	bool	m_sCanReloadSingly;
	bool	m_sDualWeapons;
	bool	m_sWeaponOptions;
	bool	m_bUseMuzzleSmoke;
	bool	m_bIsCustom;

	struct WepDef
	{
		bool	bulletEnabled;
		bool	missleEnabled;
		char	ammoType;
		float	damage;
		int		shotCount;
		float	fireRate;
		Vector	spread;
		bool	infiniteAmmoEnabled;
		bool	SMGGrenadeEnabled;
		float	SMGGrenadeDamage;
		bool	AR2EnergyBallEnabled;
		float	combineBallRadius;
		float	combineBallMass;
		float	combineBallDuration;
		float	recoilEasyDampen;
		float	recoilDegrees;
		float	recoilSeconds;
		Vector	ironsightSpread;
		Vector	zoomSpread;
		float	ironsightFireRate;
		float	zoomFireRate;
		bool	hasFire;
		bool	hasRecoilSMGGrenade;
		bool	hasRecoilRPGMissle;
		int		minRange;
		int		maxRange;
		bool	canFireUnderwater;
		bool	fireBoth;
	}primary, secondary;

	bool	 m_sUsePrimaryAmmo;

	bool	m_sUsesZoom;
	bool	m_sUsesZoomSound;
	bool	m_sUsesZoomColor;
	bool	m_sUseZoomOnPrimaryFire;
	int		m_sZoomColorRed;
	int		m_sZoomColorGreen;
	int		m_sZoomColorBlue;
	int		m_sZoomColorAlpha;

	bool	m_sUsesCustomization;
	bool	m_sUsesBodygroupSection1;
	bool	m_sUsesBodygroupSection2;
	bool	m_sUsesBodygroupSection3;
	bool	m_sUsesBodygroupSection4;
	bool	m_sUsesBodygroupSection5;
	bool	m_sUsesBodygroupSection6;
	int		m_sBodygroup1;
	int		m_sBodygroup2;
	int		m_sBodygroup3;
	int		m_sBodygroup4;
	int		m_sBodygroup5;
	int		m_sBodygroup6;
	int		m_sSubgroup1;
	int		m_sSubgroup2;
	int		m_sSubgroup3;
	int		m_sSubgroup4;
	int		m_sSubgroup5;
	int		m_sSubgroup6;
	int		m_sWeaponSkin;

};

// The weapon parse function
bool ReadWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, 
	WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey = NULL );

// If weapon info has been loaded for the specified class name, this returns it.
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot( const char *name );

FileWeaponInfo_t *GetFileWeaponInfoFromHandle( WEAPON_FILE_INFO_HANDLE handle );
WEAPON_FILE_INFO_HANDLE GetInvalidWeaponInfoHandle( void );
void PrecacheFileWeaponInfoDatabase( IFileSystem *filesystem, const unsigned char *pICEKey );


// 
// Read a possibly-encrypted KeyValues file in. 
// If pICEKey is NULL, then it appends .txt to the filename and loads it as an unencrypted file.
// If pICEKey is non-NULL, then it appends .ctx to the filename and loads it as an encrypted file.
//
// (This should be moved into a more appropriate place).
//
KeyValues* ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey );


// Each game implements this. It can return a derived class and override Parse() if it wants.
extern FileWeaponInfo_t* CreateWeaponInfo();

#include "utldict.h"

CUtlDict< FileWeaponInfo_t*, unsigned short > *GetWeaponInfoDatabase();

#endif // WEAPON_PARSE_H
