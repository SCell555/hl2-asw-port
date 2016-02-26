//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The sound categories found in the weapon classname.txt files
// This needs to match the WeaponSound_t enum in weapon_parse.h
#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
const char *pWeaponSoundCategories[ NUM_SHOOT_SOUND_TYPES ] = 
{
	"empty",
	"single_shot",
	"single_shot_npc",
	"double_shot",
	"double_shot_npc",
	"burst",
	"reload",
	"reload_npc",
	"melee_miss",
	"melee_hit",
	"melee_hit_world",
	"special1",
	"special2",
	"special3",
	"taunt",
	"fastreload"
};
#else
extern const char *pWeaponSoundCategories[ NUM_SHOOT_SOUND_TYPES ];
#endif

int GetWeaponSoundFromString( const char *pszString )
{
	for ( int i = EMPTY; i < NUM_SHOOT_SOUND_TYPES; i++ )
	{
		if ( !Q_stricmp(pszString,pWeaponSoundCategories[i]) )
			return (WeaponSound_t)i;
	}
	return -1;
}


// Item flags that we parse out of the file.
typedef struct
{
	const char *m_pFlagName;
	int m_iFlagValue;
} itemFlags_t;
#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
itemFlags_t g_ItemFlags[8] =
{
	{ "ITEM_FLAG_SELECTONEMPTY",	ITEM_FLAG_SELECTONEMPTY },
	{ "ITEM_FLAG_NOAUTORELOAD",		ITEM_FLAG_NOAUTORELOAD },
	{ "ITEM_FLAG_NOAUTOSWITCHEMPTY", ITEM_FLAG_NOAUTOSWITCHEMPTY },
	{ "ITEM_FLAG_LIMITINWORLD",		ITEM_FLAG_LIMITINWORLD },
	{ "ITEM_FLAG_EXHAUSTIBLE",		ITEM_FLAG_EXHAUSTIBLE },
	{ "ITEM_FLAG_DOHITLOCATIONDMG", ITEM_FLAG_DOHITLOCATIONDMG },
	{ "ITEM_FLAG_NOAMMOPICKUPS",	ITEM_FLAG_NOAMMOPICKUPS },
	{ "ITEM_FLAG_NOITEMPICKUP",		ITEM_FLAG_NOITEMPICKUP }
};
#else
extern itemFlags_t g_ItemFlags[7];
#endif


static CUtlDict< FileWeaponInfo_t*, unsigned short > m_WeaponInfoDatabase;

CUtlDict< FileWeaponInfo_t*, unsigned short > *GetWeaponInfoDatabase()
{
	return &m_WeaponInfoDatabase;
}

#ifdef _DEBUG
// used to track whether or not two weapons have been mistakenly assigned the wrong slot
bool g_bUsedWeaponSlots[MAX_WEAPON_SLOTS][MAX_WEAPON_POSITIONS] = { 0 };

#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : FileWeaponInfo_t
//-----------------------------------------------------------------------------
static WEAPON_FILE_INFO_HANDLE FindWeaponInfoSlot( const char *name )
{
	// Complain about duplicately defined metaclass names...
	unsigned short lookup = m_WeaponInfoDatabase.Find( name );
	if ( lookup != m_WeaponInfoDatabase.InvalidIndex() )
	{
		return lookup;
	}

	FileWeaponInfo_t *insert = CreateWeaponInfo();

	lookup = m_WeaponInfoDatabase.Insert( name, insert );
	Assert( lookup != m_WeaponInfoDatabase.InvalidIndex() );
	return lookup;
}

// Find a weapon slot, assuming the weapon's data has already been loaded.
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot( const char *name )
{
	return m_WeaponInfoDatabase.Find( name );
}



// FIXME, handle differently?
static FileWeaponInfo_t gNullWeaponInfo;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
// Output : FileWeaponInfo_t
//-----------------------------------------------------------------------------
FileWeaponInfo_t *GetFileWeaponInfoFromHandle( WEAPON_FILE_INFO_HANDLE handle )
{
	if ( handle >= m_WeaponInfoDatabase.Count() )
	{
		return &gNullWeaponInfo;
	}

	if ( handle == m_WeaponInfoDatabase.InvalidIndex() )
	{
		return &gNullWeaponInfo;
	}

	return m_WeaponInfoDatabase[ handle ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : WEAPON_FILE_INFO_HANDLE
//-----------------------------------------------------------------------------
WEAPON_FILE_INFO_HANDLE GetInvalidWeaponInfoHandle( void )
{
	return (WEAPON_FILE_INFO_HANDLE)m_WeaponInfoDatabase.InvalidIndex();
}

#if 0
void ResetFileWeaponInfoDatabase( void )
{
	int c = m_WeaponInfoDatabase.Count(); 
	for ( int i = 0; i < c; ++i )
	{
		delete m_WeaponInfoDatabase[ i ];
	}
	m_WeaponInfoDatabase.RemoveAll();

#ifdef _DEBUG
	memset(g_bUsedWeaponSlots, 0, sizeof(g_bUsedWeaponSlots));
#endif
}
#endif

void PrecacheFileWeaponInfoDatabase( IFileSystem *filesystem, const unsigned char *pICEKey )
{
	if ( m_WeaponInfoDatabase.Count() )
		return;

	KeyValues *manifest = new KeyValues( "weaponscripts" );
	manifest->UsesEscapeSequences( true );
	if ( manifest->LoadFromFile( filesystem, "scripts/weapon_manifest.txt", "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL ; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				char fileBase[512];
				Q_FileBase( sub->GetString(), fileBase, sizeof(fileBase) );
				WEAPON_FILE_INFO_HANDLE tmp;
#ifdef CLIENT_DLL
				if ( ReadWeaponDataFromFileForSlot( filesystem, fileBase, &tmp, pICEKey ) )
				{
					gWR.LoadWeaponSprites( tmp );
				}
#else
				ReadWeaponDataFromFileForSlot( filesystem, fileBase, &tmp, pICEKey );
#endif
			}
			else
			{
				Warning( "Expecting 'file', got %s\n", sub->GetName() );
			}
		}
	}
	manifest->deleteThis();
}

KeyValues* ReadEncryptedKVFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey )
{
	Assert( strchr( szFilenameWithoutExtension, '.' ) == NULL );
	char szFullName[512];

	const char *pSearchPath = "GAME";

	// Open the weapon data file, and abort if we can't
	KeyValues *pKV = new KeyValues( "WeaponDatafile" );
	pKV->UsesEscapeSequences( true );

	Q_snprintf(szFullName,sizeof(szFullName), "%s.txt", szFilenameWithoutExtension);

	if ( !pKV->LoadFromFile( filesystem, szFullName, pSearchPath ) ) // try to load the normal .txt file first
	{
#ifndef _XBOX
		if ( pICEKey )
		{
			Q_snprintf(szFullName,sizeof(szFullName), "%s.ctx", szFilenameWithoutExtension); // fall back to the .ctx file

			FileHandle_t f = filesystem->Open( szFullName, "rb", pSearchPath );

			if (!f)
			{
				pKV->deleteThis();
				return NULL;
			}
			// load file into a null-terminated buffer
			int fileSize = filesystem->Size(f);
			char *buffer = (char*)MemAllocScratch(fileSize + 1);
		
			Assert(buffer);
		
			filesystem->Read(buffer, fileSize, f); // read into local buffer
			buffer[fileSize] = 0; // null terminate file as EOF
			filesystem->Close( f );	// close file after reading

			UTIL_DecodeICE( (unsigned char*)buffer, fileSize, pICEKey );

			bool retOK = pKV->LoadFromBuffer( szFullName, buffer, filesystem );

			MemFreeScratch();

			if ( !retOK )
			{
				pKV->deleteThis();
				return NULL;
			}
		}
		else
		{
			pKV->deleteThis();
			return NULL;
		}
#else
		pKV->deleteThis();
		return NULL;
#endif
	}

	return pKV;
}


//-----------------------------------------------------------------------------
// Purpose: Read data on weapon from script file
// Output:  true  - if data2 successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------

bool ReadWeaponDataFromFileForSlot( IFileSystem* filesystem, const char *szWeaponName, WEAPON_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey )
{
	if ( !phandle )
	{
		Assert( 0 );
		return false;
	}
	
	*phandle = FindWeaponInfoSlot( szWeaponName );
	FileWeaponInfo_t *pFileInfo = GetFileWeaponInfoFromHandle( *phandle );
	Assert( pFileInfo );

	if ( pFileInfo->bParsedScript )
		return true;

	char sz[128];
	Q_snprintf( sz, sizeof( sz ), "scripts/%s", szWeaponName );
	KeyValues *pKV = ReadEncryptedKVFile( filesystem, sz, pICEKey );
	if ( !pKV )
		return false;

	pFileInfo->Parse( pKV, szWeaponName );

	pKV->deleteThis();

	return true;
}


//-----------------------------------------------------------------------------
// FileWeaponInfo_t implementation.
//-----------------------------------------------------------------------------

FileWeaponInfo_t::FileWeaponInfo_t()
{
	bParsedScript = false;
	bLoadedHudElements = false;
	szClassName[0] = 0;
	szPrintName[0] = 0;

	szViewModel[0] = 0;
	szWorldModel[0] = 0;
	szAnimationPrefix[0] = 0;
	iSlot = 0;
	iPosition = 0;
	iMaxClip1 = 0;
	iMaxClip2 = 0;
	iDefaultClip1 = 0;
	iDefaultClip2 = 0;
	iWeight = 0;
	iRumbleEffect = -1;
	bAutoSwitchTo = false;
	bAutoSwitchFrom = false;
	iFlags = 0;
	szAmmo1[0] = 0;
	szAmmo2[0] = 0;
	szAIAddOn[0] = 0;
	memset( aShootSounds, 0, sizeof( aShootSounds ) );
	iAmmoType = 0;
	iAmmo2Type = 0;
	m_bMeleeWeapon = false;
	iSpriteCount = 0;
	iconActive = 0;
	iconInactive = 0;
	iconAmmo = 0;
	iconAmmo2 = 0;
	iconCrosshair = 0;
	iconAutoaim = 0;
	iconZoomedCrosshair = 0;
	iconZoomedAutoaim = 0;
	bShowUsageHint = false;
	m_bAllowFlipping = true;
	m_bBuiltRightHanded = true;

	m_bAllowFlipping = true;
	m_bIsCustom = false;
	m_bBuiltRightHanded = true;
	m_bUseMuzzleSmoke = false;
	m_sCanReloadSingly = false;
	m_sDualWeapons = false;
	m_sWeaponOptions = false;
	muzzle = { 0, 0, 0, 0 };
	secondary = primary = {
		false,
		false,
		0,
		0.f,
		0,
		0.f,
		Vector(0, 0, 0),
		false,
		false,
		0.f,
		false,
		0.f,
		0.f,
		0.f,
		0.f,
		0.f,
		0.f,
		Vector(0, 0, 0),
		Vector(0, 0, 0),
		0.f,
		0.f,
		false,
		false,
		false,
		0,
		0,
		false,
		false
	};
}

#ifdef CLIENT_DLL
extern ConVar hud_fastswitch;
#endif

void FileWeaponInfo_t::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	// Okay, we tried at least once to look this up...
	bParsedScript = true;

	// Classname
	Q_strncpy( szClassName, szWeaponName, MAX_WEAPON_STRING );
	// Printable name
	Q_strncpy( szPrintName, pKeyValuesData->GetString( "printname", WEAPON_PRINTNAME_MISSING ), MAX_WEAPON_STRING );
	// View model & world model
	Q_strncpy( szViewModel, pKeyValuesData->GetString( "viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szWorldModel, pKeyValuesData->GetString( "playermodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szAnimationPrefix, pKeyValuesData->GetString( "anim_prefix" ), MAX_WEAPON_PREFIX );
	iSlot = pKeyValuesData->GetInt( "bucket", 0 );
	iPosition = pKeyValuesData->GetInt( "bucket_position", 0 );
	
	// Use the console (X360) buckets if hud_fastswitch is set to 2.
#ifdef CLIENT_DLL
	if ( hud_fastswitch.GetInt() == 2 )
#else
	if ( IsX360() )
#endif
	{
		iSlot = pKeyValuesData->GetInt( "bucket_360", iSlot );
		iPosition = pKeyValuesData->GetInt( "bucket_position_360", iPosition );
	}
	iMaxClip1 = pKeyValuesData->GetInt( "clip_size", WEAPON_NOCLIP );					// Max primary clips gun can hold (assume they don't use clips by default)
	iMaxClip2 = pKeyValuesData->GetInt( "clip2_size", WEAPON_NOCLIP );					// Max secondary clips gun can hold (assume they don't use clips by default)
	iDefaultClip1 = pKeyValuesData->GetInt( "default_clip", iMaxClip1 );		// amount of primary ammo placed in the primary clip when it's picked up
	iDefaultClip2 = pKeyValuesData->GetInt( "default_clip2", iMaxClip2 );		// amount of secondary ammo placed in the secondary clip when it's picked up
	iWeight = pKeyValuesData->GetInt( "weight", 0 );

	iRumbleEffect = pKeyValuesData->GetInt( "rumble", -1 );
	
	// LAME old way to specify item flags.
	// Weapon scripts should use the flag names.
	iFlags = pKeyValuesData->GetInt( "item_flags", ITEM_FLAG_LIMITINWORLD );

	for ( int i=0; i < ARRAYSIZE( g_ItemFlags ); i++ )
	{
		int iVal = pKeyValuesData->GetInt( g_ItemFlags[i].m_pFlagName, -1 );
		if ( iVal == 0 )
		{
			iFlags &= ~g_ItemFlags[i].m_iFlagValue;
		}
		else if ( iVal == 1 )
		{
			iFlags |= g_ItemFlags[i].m_iFlagValue;
		}
	}


	bShowUsageHint = pKeyValuesData->GetBool( "showusagehint", false );
	bAutoSwitchTo = pKeyValuesData->GetBool( "autoswitchto", true );
	bAutoSwitchFrom = pKeyValuesData->GetBool( "autoswitchfrom", true );
	m_bBuiltRightHanded = pKeyValuesData->GetBool( "BuiltRightHanded", true );
	m_bAllowFlipping = pKeyValuesData->GetBool( "AllowFlipping", true );
	m_bMeleeWeapon = pKeyValuesData->GetBool( "MeleeWeapon", false );
	m_bUseMuzzleSmoke = pKeyValuesData->GetBool( "UseMuzzleSmoke", false );

#if defined(_DEBUG) && defined(HL2_CLIENT_DLL)
	// make sure two weapons aren't in the same slot & position
	if ( iSlot >= MAX_WEAPON_SLOTS ||
		iPosition >= MAX_WEAPON_POSITIONS )
	{
		Warning( "Invalid weapon slot or position [slot %d/%d max], pos[%d/%d max]\n",
			iSlot, MAX_WEAPON_SLOTS - 1, iPosition, MAX_WEAPON_POSITIONS - 1 );
	}
	else
	{
		if (g_bUsedWeaponSlots[iSlot][iPosition])
		{
			Warning( "Duplicately assigned weapon slots in selection hud:  %s (%d, %d)\n", szPrintName, iSlot, iPosition );
		}
		g_bUsedWeaponSlots[iSlot][iPosition] = true;
	}
#endif

	// Primary ammo used
	const char *pAmmo = pKeyValuesData->GetString( "primary_ammo", "None" );
	if ( strcmp("None", pAmmo) == 0 )
		Q_strncpy( szAmmo1, "", sizeof( szAmmo1 ) );
	else
		Q_strncpy( szAmmo1, pAmmo, sizeof( szAmmo1 )  );
	iAmmoType = GetAmmoDef()->Index( szAmmo1 );
	
	// Secondary ammo used
	pAmmo = pKeyValuesData->GetString( "secondary_ammo", "None" );
	if ( strcmp("None", pAmmo) == 0)
		Q_strncpy( szAmmo2, "", sizeof( szAmmo2 ) );
	else
		Q_strncpy( szAmmo2, pAmmo, sizeof( szAmmo2 )  );
	iAmmo2Type = GetAmmoDef()->Index( szAmmo2 );

	// AI AddOn
	const char *pAIAddOn = pKeyValuesData->GetString( "ai_addon", "ai_addon_basecombatweapon" );
	if ( strcmp("None", pAIAddOn) == 0)
		Q_strncpy( szAIAddOn, "", sizeof( szAIAddOn ) );
	else
		Q_strncpy( szAIAddOn, pAIAddOn, sizeof( szAIAddOn )  );

	KeyValues *pSights = pKeyValuesData->FindKey("IronSight");
	if (pSights)
	{
		vecIronsightPosOffset.x = pSights->GetFloat("forward", 0.0f);
		vecIronsightPosOffset.y = pSights->GetFloat("right", 0.0f);
		vecIronsightPosOffset.z = pSights->GetFloat("up", 0.0f);

		angIronsightAngOffset[PITCH] = pSights->GetFloat("pitch", 0.0f);
		angIronsightAngOffset[YAW] = pSights->GetFloat("yaw", 0.0f);
		angIronsightAngOffset[ROLL] = pSights->GetFloat("roll", 0.0f);

		flIronsightFOVOffset = pSights->GetFloat("fov", 0.0f);
		hasIronsight = true;
	}
	else
	{
		hasIronsight = false;
		vecIronsightPosOffset = vec3_origin;
		angIronsightAngOffset.Init();
		flIronsightFOVOffset = 0.0f;
	}

#if 0	//no need for it now
	if (m_bIsCustom)
	{
		KeyValues *pWeaponSpec = pKeyValuesData->FindKey("WeaponSpec");
		if (pWeaponSpec)
		{
			KeyValues *pWeaponOptions = pWeaponSpec->FindKey("WeaponOptions");
			if (pWeaponOptions)
			{
				m_sWeaponOptions = true;
				m_sCanReloadSingly = (pWeaponOptions->GetInt("CanReloadSingly", 1) != 0) ? true : false;
				m_sDualWeapons = (pWeaponOptions->GetInt("DualWeapons", 0) != 0) ? true : false;
			}
			else
			{
				m_sWeaponOptions = false;
			}

			KeyValues *pPrimaryFire = pWeaponSpec->FindKey("PrimaryFire");
			if (pPrimaryFire)
			{
				primary.hasFire = true;
				primary.fireRate = pPrimaryFire->GetFloat("FireRate", 1.0f);
				primary.ironsightFireRate = pPrimaryFire->GetFloat("IronsightFireRate", primary.fireRate);
				primary.zoomFireRate = pPrimaryFire->GetFloat("ZoomFireRate", primary.fireRate);
				primary.infiniteAmmoEnabled = (pPrimaryFire->GetInt("InfiniteAmmo", 0) != 0) ? true : false;
				primary.minRange = pPrimaryFire->GetInt("MinRange", 0);
				primary.maxRange = pPrimaryFire->GetInt("MaxRange", 0);
				primary.canFireUnderwater = (pPrimaryFire->GetInt("CanFireUnderwater", 1) != 0) ? true : false;
				primary.fireBoth = (pWeaponOptions->GetInt("FireBothGuns", 1) != 0) ? true : false;
				KeyValues *pBullet1 = pPrimaryFire->FindKey("Bullet");
				if (pBullet1)
				{
					primary.bulletEnabled = true;
					primary.damage = pBullet1->GetFloat("Damage", 0);
					primary.shotCount = pBullet1->GetInt("ShotCount", 0);

					KeyValues *pSpread1 = pBullet1->FindKey("Spread");
					if (pSpread1)
					{
						primary.spread.x = sin((pSpread1->GetFloat("x", 0.0f) / 2.0f));
						primary.spread.y = sin((pSpread1->GetFloat("y", 0.0f) / 2.0f));
						primary.spread.z = sin((pSpread1->GetFloat("z", 0.0f) / 2.0f));
					}
					else
					{
						primary.spread.x = 0.0f;
						primary.spread.y = 0.0f;
						primary.spread.z = 0.0f;
					}

					KeyValues *pIronsightSpread1 = pBullet1->FindKey("IronsightSpread");
					if (pIronsightSpread1)
					{
						primary.ironsightSpread.x = sin((pIronsightSpread1->GetFloat("x", 0.0f) / 2.0f));
						primary.ironsightSpread.y = sin((pIronsightSpread1->GetFloat("y", 0.0f) / 2.0f));
						primary.ironsightSpread.z = sin((pIronsightSpread1->GetFloat("z", 0.0f) / 2.0f));
					}
					else
					{
						primary.ironsightSpread.x = primary.spread.x;
						primary.ironsightSpread.y = primary.spread.y;
						primary.ironsightSpread.z = primary.spread.z;
					}

					KeyValues *pZoomSpread1 = pBullet1->FindKey("ZoomSpread");
					if (pZoomSpread1)
					{
						primary.zoomSpread.x = sin((pZoomSpread1->GetFloat("x", 0.0f) / 2.0f));
						primary.zoomSpread.y = sin((pZoomSpread1->GetFloat("y", 0.0f) / 2.0f));
						primary.zoomSpread.z = sin((pZoomSpread1->GetFloat("z", 0.0f) / 2.0f));
					}
					else
					{
						primary.zoomSpread.x = primary.spread.x;
						primary.zoomSpread.y = primary.spread.y;
						primary.zoomSpread.z = primary.spread.z;
					}
				}
				else
				{
					primary.damage = 0.0f;
					primary.shotCount = 0;
					primary.bulletEnabled = false;
				}

				KeyValues *pMissle1 = pPrimaryFire->FindKey("Missle");
				if (pMissle1) //No params yet, but setting this will enable missles
				{
					primary.missleEnabled = true;
					primary.hasRecoilRPGMissle = (pMissle1->GetInt("UseRecoil", 1) != 0) ? true : false;
				}
				else
				{
					primary.missleEnabled = false;
				}

				KeyValues *pSMGGrenade1 = pPrimaryFire->FindKey("SMGGrenade");
				if (pSMGGrenade1) //No params yet, but setting this will enable missles
				{
					primary.SMGGrenadeEnabled = true;
					primary.SMGGrenadeDamage = pSMGGrenade1->GetFloat("Damage", 0);
					primary.hasRecoilSMGGrenade = (pSMGGrenade1->GetInt("UseRecoil", 1) != 0) ? true : false;
				}
				else
				{
					primary.SMGGrenadeEnabled = false;
					primary.SMGGrenadeDamage = 0.0;
				}

				KeyValues *pAR2EnergyBall1 = pPrimaryFire->FindKey("AR2EnergyBall");
				if (pAR2EnergyBall1) //No params yet, but setting this will enable missles
				{
					primary.AR2EnergyBallEnabled = true;
					primary.combineBallRadius = pAR2EnergyBall1->GetFloat("Radius", 0);
					primary.combineBallMass = pAR2EnergyBall1->GetFloat("Mass", 0);
					primary.combineBallDuration = pAR2EnergyBall1->GetFloat("Duration", 0);
				}
				else
				{
					primary.AR2EnergyBallEnabled = false;
					primary.combineBallRadius = 0.0;
					primary.combineBallMass = 0.0;
					primary.combineBallDuration = 0.0;
				}

				KeyValues *pRecoil1 = pPrimaryFire->FindKey("Recoil");
				if (pRecoil1) //No params yet, but setting this will enable missles
				{
					primary.recoilEasyDampen = pRecoil1->GetFloat("EasyDampen", 0);
					primary.recoilDegrees = pRecoil1->GetFloat("Degrees", 0);
					primary.recoilSeconds = pRecoil1->GetFloat("Seconds", 0);
				}
				else
				{
					primary.recoilEasyDampen = 0.0;
					primary.recoilDegrees = 0.0;
					primary.recoilSeconds = 0.0;
				}
			}
			else
			{
				primary.hasFire = false;
			}

			KeyValues *pSecondaryFire = pWeaponSpec->FindKey("SecondaryFire");
			if (pSecondaryFire)
			{
				secondary.hasFire = true;
				secondary.fireRate = pSecondaryFire->GetFloat("FireRate", 1.0f);
				secondary.ironsightFireRate = pSecondaryFire->GetFloat("IronsightFireRate", secondary.fireRate);
				secondary.zoomFireRate = pSecondaryFire->GetFloat("ZoomFireRate", secondary.fireRate);
				m_sUsePrimaryAmmo = (pSecondaryFire->GetInt("UsePrimaryAmmo", 0) != 0) ? true : false;
				secondary.infiniteAmmoEnabled = (pSecondaryFire->GetInt("InfiniteAmmo", 0) != 0) ? true : false;
				secondary.minRange = pSecondaryFire->GetInt("MinRange", 0);
				secondary.maxRange = pSecondaryFire->GetInt("MaxRange", 0);
				secondary.canFireUnderwater = (pSecondaryFire->GetInt("CanFireUnderwater", 1) != 0) ? true : false;
				secondary.fireBoth = (pWeaponOptions->GetInt("FireBothGuns", 0) != 0) ? true : false;
				KeyValues *pBullet1 = pSecondaryFire->FindKey("Bullet");
				if (pBullet1)
				{
					secondary.bulletEnabled = true;
					secondary.damage = pBullet1->GetFloat("Damage", 0);
					secondary.shotCount = pBullet1->GetInt("ShotCount", 0);

					KeyValues *pSpread1 = pBullet1->FindKey("Spread");
					if (pSpread1)
					{
						secondary.spread.x = sin((pSpread1->GetFloat("x", 0.0f) / 2.0f));
						secondary.spread.y = sin((pSpread1->GetFloat("y", 0.0f) / 2.0f));
						secondary.spread.z = sin((pSpread1->GetFloat("z", 0.0f) / 2.0f));
					}
					else
					{
						secondary.spread.x = 0.0f;
						secondary.spread.y = 0.0f;
						secondary.spread.z = 0.0f;
					}

					KeyValues *pIronsightSpread1 = pBullet1->FindKey("IronsightSpread");
					if (pIronsightSpread1)
					{
						secondary.ironsightSpread.x = sin((pIronsightSpread1->GetFloat("x", 0.0f) / 2.0f));
						secondary.ironsightSpread.y = sin((pIronsightSpread1->GetFloat("y", 0.0f) / 2.0f));
						secondary.ironsightSpread.z = sin((pIronsightSpread1->GetFloat("z", 0.0f) / 2.0f));
					}
					else
					{
						secondary.ironsightSpread.x = secondary.spread.x;
						secondary.ironsightSpread.y = secondary.spread.y;
						secondary.ironsightSpread.z = secondary.spread.z;
					}

					KeyValues *pZoomSpread1 = pBullet1->FindKey("ZoomSpread");
					if (pZoomSpread1)
					{
						secondary.zoomSpread.x = sin((pZoomSpread1->GetFloat("x", 0.0f) / 2.0f));
						secondary.zoomSpread.y = sin((pZoomSpread1->GetFloat("y", 0.0f) / 2.0f));
						secondary.zoomSpread.z = sin((pZoomSpread1->GetFloat("z", 0.0f) / 2.0f));
					}
					else
					{
						secondary.zoomSpread.x = secondary.spread.x;
						secondary.zoomSpread.y = secondary.spread.y;
						secondary.zoomSpread.z = secondary.spread.z;
					}
				}
				else
				{
					secondary.damage = 0.0f;
					secondary.shotCount = 0;
					secondary.bulletEnabled = false;
				}

				KeyValues *pMissle1 = pSecondaryFire->FindKey("Missle");
				if (pMissle1) //No params yet, but setting this will enable missles
				{
					secondary.missleEnabled = true;
					secondary.hasRecoilRPGMissle = (pMissle1->GetInt("UseRecoil", 1) != 0) ? true : false;
				}
				else
				{
					secondary.missleEnabled = false;
				}

				KeyValues *pSMGGrenade1 = pSecondaryFire->FindKey("SMGGrenade");
				if (pSMGGrenade1) //No params yet, but setting this will enable missles
				{
					secondary.SMGGrenadeEnabled = true;
					secondary.SMGGrenadeDamage = pSMGGrenade1->GetFloat("Damage", 0);
					secondary.hasRecoilSMGGrenade = (pSMGGrenade1->GetInt("UseRecoil", 1) != 0) ? true : false;
				}
				else
				{
					secondary.SMGGrenadeEnabled = false;
					secondary.SMGGrenadeDamage = 0.0;
				}

				KeyValues *pAR2EnergyBall1 = pSecondaryFire->FindKey("AR2EnergyBall");
				if (pAR2EnergyBall1) //No params yet, but setting this will enable missles
				{
					secondary.AR2EnergyBallEnabled = true;
					secondary.combineBallRadius = pAR2EnergyBall1->GetFloat("Radius", 0);
					secondary.combineBallMass = pAR2EnergyBall1->GetFloat("Mass", 0);
					secondary.combineBallDuration = pAR2EnergyBall1->GetFloat("Duration", 0);
				}
				else
				{
					secondary.AR2EnergyBallEnabled = false;
					secondary.combineBallRadius = 0.0;
					secondary.combineBallMass = 0.0;
					secondary.combineBallDuration = 0.0;
				}

				KeyValues *pRecoil1 = pSecondaryFire->FindKey("Recoil");
				if (pRecoil1) //No params yet, but setting this will enable missles
				{
					secondary.recoilEasyDampen = pRecoil1->GetFloat("EasyDampen", 0);
					secondary.recoilDegrees = pRecoil1->GetFloat("Degrees", 0);
					secondary.recoilSeconds = pRecoil1->GetFloat("Seconds", 0);
				}
				else
				{
					secondary.recoilEasyDampen = 0.0;
					secondary.recoilDegrees = 0.0;
					secondary.recoilSeconds = 0.0;
				}
			}
			else
			{
				secondary.hasFire = false;
			}



			KeyValues *pZoom = pWeaponSpec->FindKey("Zoom");
			if (pZoom)
			{
				m_sUsesZoom = true;
				m_sUseZoomOnPrimaryFire = (pZoom->GetInt("UseOnPrimaryFire", 0) != 0) ? true : false;
				m_sUsesZoomSound = (pZoom->GetInt("UsesSound", 1) != 0) ? true : false;
				m_sUsesZoomColor = (pZoom->GetInt("UsesColor", 1) != 0) ? true : false;
				KeyValues *pZoomColor = pZoom->FindKey("ZoomColor");
				{
					if (pZoomColor && m_sUsesZoomColor)
					{
						m_sZoomColorRed = pZoomColor->GetInt("Red", 0);
						m_sZoomColorGreen = pZoomColor->GetInt("Green", 0);
						m_sZoomColorBlue = pZoomColor->GetInt("Blue", 0);
						m_sZoomColorAlpha = pZoomColor->GetInt("Alpha", 0);
					}
					else
						m_sZoomColorAlpha = m_sZoomColorBlue = m_sZoomColorGreen = m_sZoomColorRed = 0;

				}
			}
			else
			{
				m_sUsesZoom = false;
			}

			KeyValues *pCustomization = pWeaponSpec->FindKey("Customization");
			if (pCustomization)
			{
				m_sUsesCustomization = true;

				m_sWeaponSkin = pCustomization->GetInt("Skin", 0);
				KeyValues *pBodygroup1 = pWeaponSpec->FindKey("Bodygroup1");
				if (pBodygroup1)
				{
					m_sBodygroup1 = pBodygroup1->GetInt("Bodygroup", 0);
					m_sSubgroup1 = pBodygroup1->GetInt("Subgroup", 0);
				}


				KeyValues *pBodygroup2 = pWeaponSpec->FindKey("Bodygroup2");
				if (pBodygroup2)
				{
					m_sBodygroup2 = pBodygroup2->GetInt("Bodygroup", 0);
					m_sSubgroup2 = pBodygroup2->GetInt("Subgroup", 0);
				}


				KeyValues *pBodygroup3 = pWeaponSpec->FindKey("Bodygroup3");
				{
					if (pBodygroup3)
					{
						m_sBodygroup3 = pBodygroup3->GetInt("Bodygroup", 0);
						m_sSubgroup3 = pBodygroup3->GetInt("Subgroup", 0);
					}
				}

				KeyValues *pBodygroup4 = pWeaponSpec->FindKey("Bodygroup4");
				if (pBodygroup4)
				{
					m_sBodygroup4 = pBodygroup4->GetInt("Bodygroup", 0);
					m_sSubgroup4 = pBodygroup4->GetInt("Subgroup", 0);
				}


				KeyValues *pBodygroup5 = pWeaponSpec->FindKey("Bodygroup5");
				if (pBodygroup5)
				{
					m_sBodygroup5 = pBodygroup5->GetInt("Bodygroup", 0);
					m_sSubgroup5 = pBodygroup5->GetInt("Subgroup", 0);
				}


				KeyValues *pBodygroup6 = pWeaponSpec->FindKey("Bodygroup6");
				if (pBodygroup6)
				{
					m_sBodygroup6 = pBodygroup6->GetInt("Bodygroup", 0);
					m_sSubgroup6 = pBodygroup6->GetInt("Subgroup", 0);
				}

			}
			else
			{
				m_sUsesCustomization = false;
			}
		}
	}
#endif

	KeyValues *flash = pKeyValuesData->FindKey("muzzleflash");
	if (flash)
	{
		muzzle.r = flash->GetInt("red", 255);
		muzzle.g = flash->GetInt("green", 192);
		muzzle.b = flash->GetInt("blue", 64);
		muzzle.exponent = flash->GetInt("exponent", 1);
	}
	else
	{
		muzzle.r = 255;
		muzzle.g = 192;
		muzzle.b = 64;
		muzzle.exponent = 2;
	}

	// Now read the weapon sounds
	memset( aShootSounds, 0, sizeof( aShootSounds ) );
	KeyValues *pSoundData = pKeyValuesData->FindKey( "SoundData" );
	if ( pSoundData )
	{
		for ( int i = EMPTY; i < NUM_SHOOT_SOUND_TYPES; i++ )
		{
			const char *soundname = pSoundData->GetString( pWeaponSoundCategories[i] );
			if ( soundname && soundname[0] )
			{
				Q_strncpy( aShootSounds[i], soundname, MAX_WEAPON_STRING );
			}
		}
	}
}

