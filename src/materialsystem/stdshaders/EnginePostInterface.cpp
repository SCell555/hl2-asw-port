#include "IShaderEnginePostprocessInterface.h"

#include "basevsshader.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

#pragma once

CShaderEnginePostprocessInterface __g_shaderExt;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CShaderEnginePostprocessInterface, IShaderEnginePostprocessInterface, POSTPROCESS_INTERFACE_VERSION, __g_shaderExt);

CShaderEnginePostprocessInterface::CShaderEnginePostprocessInterface()
{
	m_boolDofEnabled = false;
	m_boolVignetteEnabled = false;
	m_boolLocalContrastEnabled = false;
	m_boolBlurredVignetteEnabled = false;
	m_boolNoiseEnabled = false;
	m_fBloomTable[0] = 1.0f;
	m_fBloomTable[1] = 0.0f;
	m_fBloomTable[2] = 0.0f;
	m_fBloomTable[3] = 0.0f;
	m_fBlurTable[0] = 1.0f;
	m_fBlurTable[1] = 1000.0f;
	m_fBlurTable[2] = 0.0f;
	m_fBlurTable[0] = 0.7f;
	m_fBlurTable[1] = 1.0f;
	m_fBlurTable[2] = 0.0f;
}

CShaderEnginePostprocessInterface::~CShaderEnginePostprocessInterface()
{
}

void CShaderEnginePostprocessInterface::Connect()
{
	DevMsg("CShaderEnginePostprocessInterface connected!\n");
}

void CShaderEnginePostprocessInterface::SetScreenTexture(const char* text)
{
	m_sCurScreenTexture = castable_string_t(text);
}

const char* CShaderEnginePostprocessInterface::GetScreenTexture()
{
	return m_sCurScreenTexture.ToCStr();
}

void CShaderEnginePostprocessInterface::SetNoiseTexture(const char* text)
{
	m_sCurNoiseTexture = castable_string_t(text);
}

const char* CShaderEnginePostprocessInterface::GetNoiseTexture()
{
	return m_sCurNoiseTexture.ToCStr();
}

void CShaderEnginePostprocessInterface::SetTextureParamIfNeeded(CBaseShader* inst, IMaterialVar **params, int num, const char* val)
{
	const char* o;
	if (!Q_strcmp(val,"screen"))
	{
		if (!Q_strcmp(m_sCurScreenTexture.ToCStr(), m_sOldScreenTexture.ToCStr()))
			return;
		m_sCurScreenTexture = m_sOldScreenTexture;
		o = m_sCurScreenTexture.ToCStr();
	}
	else if (!Q_strcmp(val, "noise"))
	{
		if (!Q_strcmp(m_sCurNoiseTexture.ToCStr(), m_sOldNoiseTexture.ToCStr()))
			return;
		m_sCurNoiseTexture = m_sOldNoiseTexture;
		o = m_sCurNoiseTexture.ToCStr();
	}
	else
		return;

	params[num]->SetStringValue(o);
	inst->LoadTexture(num);
}

void CShaderEnginePostprocessInterface::UpdateFloatArrayIfNeeded(float* arr, int siz, const char* nam)
{
	if (!Q_strcmp(nam, "bloom"))
	{
		for (int i = 0; i < siz; i++)
			if (m_fBloomTable[i] != arr[i])
				arr[i] = m_fBloomTable[i];
	}
	else if (!Q_strcmp(nam, "blur"))
	{
		for (int i = 0; i < siz; i++)
			if (m_fBlurTable[i] != arr[i])
				arr[i] = m_fBlurTable[i];
	}
	else if (!Q_strcmp(nam, "contrast"))
	{
		for (int i = 0; i < siz; i++)
			if (m_fContrastTable[i] != arr[i])
				arr[i] = m_fContrastTable[i];
	}
}

void CShaderEnginePostprocessInterface::SetFloatArrayValue(float* arr, int siz, const char* nam)
{
	if (!Q_strcmp(nam, "bloom"))
	{
		Assert(siz != 4);
		Q_memcpy(m_fBloomTable, arr, sizeof(float) * siz);
	}
	else if (!Q_strcmp(nam, "blur"))
	{
		Assert(siz != 3);
		Q_memcpy(m_fBlurTable, arr, sizeof(float) * siz);
	}
	else if (!Q_strcmp(nam, "contrast"))
	{
		Assert(siz != 3);
		Q_memcpy(m_fContrastTable, arr, sizeof(float) * siz);
	}
}