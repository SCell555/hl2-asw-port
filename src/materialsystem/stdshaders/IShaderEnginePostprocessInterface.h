#ifndef I_SHADERAPI_EXT_H
#define I_SHADERAPI_EXT_H

#ifdef CLIENT_DLL
#include "interface.h"
#else
#include "../public/tier1/interface.h"
#ifndef NULL
#define NULL 0
#endif
#include "string_t.h"
#endif

class CBaseShader;
class IMaterialVar;

#pragma once


#define DECLARE_THING(var, type) \
	virtual void Set##var##Enabled(type) = 0;\
	virtual type Is##var##Enabled() = 0;\

class IShaderEnginePostprocessInterface : public IBaseInterface
{
public:
	virtual void Connect() = 0;
	DECLARE_THING(Dof, bool);
	DECLARE_THING(Vignette, bool);
	DECLARE_THING(LocalContrast, bool);
	DECLARE_THING(BlurredVignette, bool);
	DECLARE_THING(Noise, bool);
	DECLARE_THING(Desaturate, bool);
	DECLARE_THING(ScreenEffect, bool);

	virtual void SetFloatArrayValue(float*, int, const char*) = 0;

	virtual void	SetScreenTexture(const char*) = 0;
	virtual const char* GetScreenTexture() = 0;
	virtual void	SetNoiseTexture(const char*) = 0;
	virtual const char* GetNoiseTexture() = 0;
	//virtual void	SetTextureParamIfNeeded(CBaseShader*, IMaterialVar **, int, const char*) = 0;
};

#define POSTPROCESS_INTERFACE_VERSION "PostProcessInterfaceVer001"


#ifdef CLIENT_DLL
extern IShaderEnginePostprocessInterface *GetShaderPostExt();
#else
#undef DECLARE_THING
#define DECLARE_THING(var,type) \
	public:\
	virtual void Set##var##Enabled(type b){ m_##type####var##Enabled = b; }\
	virtual type Is##var##Enabled(){ return m_##type####var##Enabled; }\
	private:\
	type m_##type####var##Enabled;\


class CShaderEnginePostprocessInterface : public IShaderEnginePostprocessInterface
{
public:
	CShaderEnginePostprocessInterface();
	~CShaderEnginePostprocessInterface();
	
	virtual void Connect();

	DECLARE_THING(Dof, bool);
	DECLARE_THING(Vignette, bool);
	DECLARE_THING(LocalContrast, bool);
	DECLARE_THING(BlurredVignette, bool);
	DECLARE_THING(Noise, bool);
	DECLARE_THING(Desaturate, bool);
	DECLARE_THING(ScreenEffect, bool);

public:
	virtual void	SetScreenTexture(const char*);
	virtual const char* GetScreenTexture();
	virtual void	SetNoiseTexture(const char*);
	virtual const char* GetNoiseTexture();
	virtual void SetFloatArrayValue(float*, int, const char*);

	virtual void	SetTextureParamIfNeeded(CBaseShader*, IMaterialVar **, int, const char*);
	virtual void	UpdateFloatArrayIfNeeded(float*, int, const char*);
private:
	string_t m_sOldScreenTexture;
	string_t m_sCurScreenTexture;
	string_t m_sOldNoiseTexture;
	string_t m_sCurNoiseTexture;
	float	m_fBloomTable[4];
	float	m_fBlurTable[3];
	float	m_fContrastTable[3];
};


extern CShaderEnginePostprocessInterface __g_shaderExt;
FORCEINLINE CShaderEnginePostprocessInterface *GetShaderPostExt()
{
	return &__g_shaderExt;
}
#endif

#endif