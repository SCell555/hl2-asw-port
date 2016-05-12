//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_shared.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "characterset.h"
#include "isaverestore.h"
#include "gamerules.h"

IScriptVM * g_pScriptVM;
extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_VScript, "VScript", LCF_CONSOLE_ONLY, LS_WARNING );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START float debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE



HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return NULL;
	}

	static const char *pszExtensions[] =
	{
		"",		// SL_NONE
		".gm",	// SL_GAMEMONKEY
		".nut",	// SL_SQUIRREL
		".lua", // SL_LUA
		".py",  // SL_PYTHON
	};

	const char *pszVMExtension = pszExtensions[g_pScriptVM->GetLanguage()];
	const char *pszIncomingExtension = V_strrchr( pszScriptName , '.' );
	if ( pszIncomingExtension && V_strcmp( pszIncomingExtension, pszVMExtension ) != 0 )
	{
		Log_Warning( LOG_VScript, "Script file type does not match VM type\n" );
		return NULL;
	}

	CFmtStr scriptPath;
	if ( pszIncomingExtension )
	{
		scriptPath.sprintf( "scripts/vscripts/%s", pszScriptName );
	}
	else
	{	
		scriptPath.sprintf( "scripts/vscripts/%s%s", pszScriptName,  pszVMExtension );
	}

	const char *pBase;
	CUtlBuffer bufferScript;

	if ( g_pScriptVM->GetLanguage() == SL_PYTHON )
	{
		// python auto-loads raw or precompiled modules - don't load data here
		pBase = NULL;
	}
	else
	{
		bool bResult = filesystem->ReadFile( scriptPath, "GAME", bufferScript );

		if( !bResult )
		{
			Log_Warning( LOG_VScript, "Script not found (%s) \n", scriptPath.operator const char *() );
			Assert( "Error running script" );
		}

		pBase = (const char *) bufferScript.Base();

		if ( !pBase || !*pBase )
		{
			return NULL;
		}
	}


	const char *pszFilename = V_strrchr( scriptPath, '/' );
	pszFilename++;
	HSCRIPT hScript = g_pScriptVM->CompileScript( pBase, pszFilename );
	if ( !hScript )
	{
		Log_Warning( LOG_VScript, "FAILED to compile and execute script file named %s\n", scriptPath.operator const char *() );
		Assert( "Error running script" );
	}
	return hScript;
}

static int g_ScriptServerRunScriptDepth;

bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	if ( !pszScriptName || !*pszScriptName )
	{
		Log_Warning( LOG_VScript, "Cannot run script: NULL script name\n" );
		return false;
	}

	// Prevent infinite recursion in VM
	if ( g_ScriptServerRunScriptDepth > 16 )
	{
		Log_Warning( LOG_VScript, "IncludeScript stack overflow\n" );
		return false;
	}

	g_ScriptServerRunScriptDepth++;
	HSCRIPT	hScript = VScriptCompileScript( pszScriptName, bWarnMissing );
	bool bSuccess = false;
	if ( hScript )
	{
#ifdef GAME_DLL
		if ( gpGlobals->maxClients == 1 )
		{
			CBaseEntity *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				g_pScriptVM->SetValue( "player", pPlayer->GetScriptInstance() );
			}
		}
#endif
		bSuccess = ( g_pScriptVM->Run( hScript, hScope ) != SCRIPT_ERROR );
		if ( !bSuccess )
		{
			Log_Warning( LOG_VScript, "Error running script named %s\n", pszScriptName );
			Assert( "Error running script" );
		}
	}
	g_ScriptServerRunScriptDepth--;
	return bSuccess;
}

#ifdef CLIENT_DLL
CON_COMMAND( script_client, "Run the text as a script (client)" )
#else
CON_COMMAND( script, "Run the text as a script (server)" )
#endif
{
	if ( !*args[1] )
	{
		Log_Warning( LOG_VScript, "No function name specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args.GetCommandString();

#ifdef CLIENT_DLL
	pszScript += 13;
#else
	pszScript += 6;
#endif
	
	while ( *pszScript == ' ' )
	{
		pszScript++;
	}

	if ( !*pszScript )
	{
		return;
	}

	if ( *pszScript != '\"' )
	{
		g_pScriptVM->Run( pszScript );
	}
	else
	{
		pszScript++;
		const char *pszEndQuote = pszScript;
		while ( *pszEndQuote !=  '\"' )
		{
			pszEndQuote++;
		}
		if ( !*pszEndQuote )
		{
			return;
		}
		*((char *)pszEndQuote) = 0;
		g_pScriptVM->Run( pszScript );
		*((char *)pszEndQuote) = '\"';
	}
}


CON_COMMAND_SHARED( script_execute, "Run a vscript file" )
{
	if ( !*args[1] )
	{
		Log_Warning( LOG_VScript, "No script specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}

	VScriptRunScript( args[1], true );
}

CON_COMMAND_SHARED( script_debug, "Connect the vscript VM to the script debugger" )
{
	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->ConnectDebugger();
}

CON_COMMAND_SHARED( script_help, "Output help for script functions, optionally with a search string" )
{
	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}
	const char *pszArg1 = "*";
	if ( *args[1] )
	{
		pszArg1 = args[1];
	}

	g_pScriptVM->Run( CFmtStr( "PrintHelp( \"%s\" );", pszArg1 ) );
}

CON_COMMAND_SHARED( script_dump_all, "Dump the state of the VM to the console" )
{
	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->DumpState();
}

#include "weapon_parse.h"

static FileWeaponInfo_t *workingInfo = NULL;
static bool isWorking = false;

void BeginCreatingNewWeapon()
{
	if (isWorking)
	{
		AssertMsg(false, "Already creating new weapon?");
		return;
	}
	workingInfo = new FileWeaponInfo_t;
	workingInfo->m_bIsCustom = true;

	isWorking = true;
}
static unsigned int modCount = 0;

struct modification_type_t{
	const char* modName;
	int number;
	modification_type_t(const char* name):number(modCount++){}
};

static const modification_type_t modificationNames[] = {
	"reloadSingly",
	"dualWeapons",
	"fireRate",
	"ironsightFireRate",
	"zoomFireRate",
	"infiniteAmmoEnabled",
	"minRange",
	"maxRange",
	"canFireUnderwater",
	"fireBoth",
	"bulletEnabled",
	"damage",
	"shotCount",
	"spread",
	"ironsightSpread",
	"zoomSpread",
	"missleEnabled",
	"hasRecoilRPGMissle",
	"SMGGrenadeEnabled",
	"SMGGrenadeDamage",
	"hasRecoilSMGGrenade",
	"AR2EnergyBallEnabled",
	"combineBallRadius",
	"combineBallMass",
	"combineBallDuration",
	"recoilEasyDampen",
	"recoilDegrees",
	"recoilSeconds",
	"usePrimaryAmmo",
	"primary_ammo",
	"secondary_ammo",
	"className",
	"printName",
	"viewModel",
	"worldModel",
	"anim_prefix",
	"bucket",
	"bucket_position",
	"clip_size",
	"clip2_size",
	"default_clip",
	"default_clip2",
	"weight",
	"rumble",
	"item_flags",
	"showusagehint",
	"autoswitchto",
	"autoswitchfrom",
	"builtRightHanded",
	"allowFlipping",
	"meleeWeapon",
	"useMuzzleSmoke",
	"muzzleflash",
};

void AddWeaponProperty(const char* prop, const char* value, const char* part)
{
	Assert(workingInfo);
	if (!workingInfo)
		return;

	FileWeaponInfo_t::WepDef *workingPart = NULL;

	unsigned int workingNum = modCount + 1;
	for (int i = 0; i < ARRAYSIZE(modificationNames); i++)
		if (Q_strcmp(modificationNames[i].modName, prop) == 0)
		{
			workingNum = modificationNames[i].number;
			break;
		}

	if (*part || part[0])
	{
		if (Q_strcmp(part, "primary") == 0)
		{
			workingPart = &workingInfo->primary;
		}
		else if (Q_strcmp(part, "secondary") == 0)
		{
			workingPart = &workingInfo->secondary;
		}
	}

	if (!workingPart && workingNum > 1 && workingNum < 28)
	{
		AssertMsg(0, "Modifying part value without specifying one.");
		return;
	}

	Assert(workingNum < modCount);

	float spr[3];
	switch (workingNum)
	{
	case 0:
		workingInfo->m_sCanReloadSingly = Q_atoi(value) != 0 ? true : false;
		break;
	case 1:
		workingInfo->m_sDualWeapons = Q_atoi(value) != 0 ? true : false;
		break;
	case 2:
		workingPart->fireRate = Q_atof(value);
		break;
	case 3:
		workingPart->ironsightFireRate = Q_atof(value);
		break;
	case 4:
		workingPart->zoomFireRate = Q_atof(value);
		break;
	case 5:
		workingPart->infiniteAmmoEnabled = Q_atoi(value) != 0 ? true : false;
		break;
	case 6:
		workingPart->minRange = Q_atoi(value);
		break;
	case 7:
		workingPart->maxRange = Q_atoi(value);
		break;
	case 8:
		workingPart->canFireUnderwater = Q_atoi(value) != 0 ? true : false;
		break;
	case 9:
		workingPart->fireBoth = Q_atoi(value) != 0 ? true : false;
		break;
	case 10:
		workingPart->bulletEnabled = Q_atoi(value) != 0 ? true : false;
		break;
	case 11:
		workingPart->damage = Q_atof(value);
		break;
	case 12:
		workingPart->shotCount = Q_atoi(value);
		break;
	case 13:
		UTIL_StringToFloatArray(spr, 3, value);
		workingPart->spread.x = sin(spr[0] / 2.0f);
		workingPart->spread.y = sin(spr[1] / 2.0f);
		workingPart->spread.z = sin(spr[2] / 2.0f);
		break;
	case 14:
		UTIL_StringToFloatArray(spr, 3, value);
		workingPart->ironsightSpread.x = sin(spr[0] / 2.0f);
		workingPart->ironsightSpread.y = sin(spr[1] / 2.0f);
		workingPart->ironsightSpread.z = sin(spr[2] / 2.0f);
		break;
	case 15:
		UTIL_StringToFloatArray(spr, 3, value);
		workingPart->zoomSpread.x = sin(spr[0] / 2.0f);
		workingPart->zoomSpread.y = sin(spr[1] / 2.0f);
		workingPart->zoomSpread.z = sin(spr[2] / 2.0f);
		break;
	case 16:
		workingPart->missleEnabled = Q_atoi(value) != 0 ? true : false;
		break;
	case 17:
		workingPart->hasRecoilRPGMissle = Q_atoi(value) != 0 ? true : false;
		break;
	case 18:
		workingPart->SMGGrenadeEnabled = Q_atoi(value) != 0 ? true : false;
		break;
	case 19:
		workingPart->SMGGrenadeDamage = Q_atof(value);
		break;
	case 20:
		workingPart->hasRecoilSMGGrenade = Q_atoi(value) != 0 ? true : false;
		break;
	case 21:
		workingPart->AR2EnergyBallEnabled = Q_atoi(value) != 0 ? true : false;
		break;
	case 22:
		workingPart->combineBallRadius = Q_atof(value);
		break;
	case 23:
		workingPart->combineBallMass = Q_atof(value);
		break;
	case 24:
		workingPart->combineBallDuration = Q_atof(value);
		break;
	case 25:
		workingPart->recoilEasyDampen = Q_atof(value);
		break;
	case 26:
		workingPart->recoilDegrees = Q_atof(value);
		break;
	case 27:
		workingPart->recoilSeconds = Q_atof(value);
		break;
	case 28:
		workingInfo->m_sUsePrimaryAmmo = Q_atoi(value) != 0 ? true : false;
		break;
	case 29:
		Q_strcpy(workingInfo->szAmmo1, value);
		break;
	case 30:
		Q_strcpy(workingInfo->szAmmo2, value);
		break;
	case 31:
		Q_strcpy(workingInfo->szClassName, value);
		break;
	case 32:
		Q_strcpy(workingInfo->szPrintName, value);
		break;
	case 33:
		Q_strcpy(workingInfo->szViewModel, value);
		break;
	case 34:
		Q_strcpy(workingInfo->szWorldModel, value);
		break;
	case 35:
		Q_strcpy(workingInfo->szAnimationPrefix, value);
		break;
	case 36:
		workingInfo->iSlot = Q_atoi(value);
		break;
	case 37:
		workingInfo->iPosition = Q_atoi(value);
		break;
	case 38:
		workingInfo->iMaxClip1 = Q_atoi(value);
		break;
	case 39:
		workingInfo->iMaxClip2 = Q_atoi(value);
		break;
	case 40:
		workingInfo->iDefaultClip1 = Q_atoi(value);
		break;
	case 41:
		workingInfo->iDefaultClip2 = Q_atoi(value);
		break;
	case 42:
		workingInfo->iWeight = Q_atoi(value);
		break;
	case 43:
		workingInfo->iRumbleEffect = Q_atoi(value);
		break;
	case 44:
		workingInfo->iFlags = Q_atoi(value);
		break;
	case 45:
		workingInfo->bShowUsageHint = Q_atoi(value) != 0 ? true : false;
		break;
	case 46:
		workingInfo->bAutoSwitchTo = Q_atoi(value) != 0 ? true : false;
		break;
	case 47:
		workingInfo->bAutoSwitchFrom = Q_atoi(value) != 0 ? true : false;
		break;
	case 48:
		workingInfo->m_bBuiltRightHanded = Q_atoi(value) != 0 ? true : false;
		break;
	case 49:
		workingInfo->m_bAllowFlipping = Q_atoi(value) != 0 ? true : false;
		break;
	case 50:
		workingInfo->m_bMeleeWeapon = Q_atoi(value) != 0 ? true : false;
		break;
	case 51:
		workingInfo->m_bUseMuzzleSmoke = Q_atoi(value) != 0 ? true : false;
		break;
	case 52:
		int col[4];
		UTIL_StringToIntArray(col, 4, value);
		workingInfo->muzzle = { col[0], col[1], col[2], col[3] };
		break;
	default:
		break;
	}
}

#ifdef CLIENT_DLL
#include "c_weapon_custom.h"
#include "weapons_resource.h"
static C_BaseEntity *C_ScriptedWeaponFactory(void)
{
	return static_cast< C_BaseEntity * >(new C_WeaponCustom);
};
#else
#include "weapon_custom.h"
#endif

void EndCreatingNewWeapon()
{
	if (!workingInfo)
	{
		AssertMsg(false, "Calling end message without starting new one?");
		return;
	}
	if (!workingInfo->szClassName)
		return;

#ifdef HL2_DLL
	do
		static CEntityFactory<CWeaponCustom> newWeapon(workingInfo->szClassName);
	while(0);
#else
	if (!GetClassMap().Lookup(workingInfo->szClassName))
	{
		GetClassMap().Add(workingInfo->szClassName, "C_WeaponCustom", sizeof(C_WeaponCustom), &C_ScriptedWeaponFactory);
	}
//	gWR.LoadWeaponSprites(workingInfo); //FIXME
#endif
	workingInfo->bParsedScript = true;

	GetWeaponInfoDatabase()->Insert(workingInfo->szClassName, workingInfo);

	isWorking = false;
}