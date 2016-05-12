#include "cbase.h"

#include "../../materialsystem/stdshaders/IShaderEnginePostprocessInterface.h"

class C_ShaderAPIGameSystem : public CAutoGameSystemPerFrame
{
	typedef CAutoGameSystemPerFrame BaseClass;
public:
	C_ShaderAPIGameSystem();
	~C_ShaderAPIGameSystem();

	virtual bool Init();
	virtual void Shutdown();

private:
	virtual void Update( float frametime );
};

static C_ShaderAPIGameSystem __g_shaderapigamesystem;
C_ShaderAPIGameSystem *GetShaderAPIGameSystem()
{
	return &__g_shaderapigamesystem;
}

C_ShaderAPIGameSystem::C_ShaderAPIGameSystem() : BaseClass( "CShaderAPIGameSystem" )
{

}

C_ShaderAPIGameSystem::~C_ShaderAPIGameSystem()
{

}

bool C_ShaderAPIGameSystem::Init()
{
	extern bool ConnectShaderPostprocessInterface();
	bool bGotDefShaderDll = ConnectShaderPostprocessInterface();
	if(bGotDefShaderDll)
	{
		GetShaderPostExt()->Connect();
	}
	return true;
}

void C_ShaderAPIGameSystem::Shutdown()
{
	extern void ShutdownShaderPostprocessInterface();
	ShutdownShaderPostprocessInterface();
}

void C_ShaderAPIGameSystem::Update( float frametime )
{
	
}