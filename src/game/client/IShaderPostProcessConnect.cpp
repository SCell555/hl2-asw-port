#include "cbase.h"
#include "../../materialsystem/stdshaders/IShaderEnginePostprocessInterface.h"

// -------------------------------------------
// Purpose: Setup globally accessible instance of the Shader API extension interface
// Author: AniCator
// -------------------------------------------
CSysModule *__g_pShaderAPIShaderModule = NULL;
static IShaderEnginePostprocessInterface *__g_shaderExt = NULL;
IShaderEnginePostprocessInterface *GetShaderPostExt()
{
	return __g_shaderExt;
}

bool ConnectShaderPostprocessInterface()
{
	char modulePath[MAX_PATH*4];
	Q_snprintf( modulePath, sizeof( modulePath ), "%s/bin/game_shader_dx9.dll", engine->GetGameDirectory() );
	__g_pShaderAPIShaderModule = Sys_LoadModule( modulePath );

	if ( __g_pShaderAPIShaderModule )
	{
		CreateInterfaceFn shaderShaderAPIDLLFactory = Sys_GetFactory( __g_pShaderAPIShaderModule );
		__g_shaderExt = shaderShaderAPIDLLFactory ? ((IShaderEnginePostprocessInterface *)shaderShaderAPIDLLFactory(POSTPROCESS_INTERFACE_VERSION, NULL)) : NULL;

		if ( !__g_shaderExt )
			Warning( "Unable to pull IShaderEnginePostprocessInterface interface.\n" );
	}
	else
		Warning( "Cannot load game_shader_dx9.dll from %s!\n", modulePath );

	return __g_shaderExt != NULL;
}

void ShutdownShaderPostprocessInterface()
{
	if ( !__g_shaderExt )
		return;

	__g_shaderExt = NULL;

	if (__g_pShaderAPIShaderModule)
		Sys_UnloadModule(__g_pShaderAPIShaderModule);
}