#ifndef ESTRANGEDCLIENTRENDERTARGETS_H
#define ESTRANGEDCLIENTRENDERTARGETS_H
#ifdef _WIN32
#pragma once
#endif

#include "baseclientrendertargets.h"
#include "materialsystem/MaterialSystemUtil.h"

class IMaterialSystem;
class IMaterialSystemHardwareConfig;

class CHL2ClientRenderTargets : public CBaseClientRenderTargets, public CAutoGameSystemPerFrame
{
	DECLARE_CLASS_GAMEROOT(CHL2ClientRenderTargets, CBaseClientRenderTargets);
public:
	virtual void InitClientRenderTargets(IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig);
	virtual void ShutdownClientRenderTargets();

	void InitSSAORenderTargets();
	void ShutdownSSAORenderTargets();

	ITexture* CreateSSAODepthTexture();
	ITexture* CreateSSAOResultTexture();
	ITexture* CreateSSAOBlurYTexture();

	virtual void PreRender();

private:
	CTextureReference m_SSAODepthTexture;
	CTextureReference m_SSAOResultTexture;
	CTextureReference m_SSAOBlurYTexture;

	int m_iBackBufferWidth;
	int m_iBackBufferHeight;

	IMaterialSystem *m_pMaterialSystem;
};

extern CHL2ClientRenderTargets* TNERenderTargets;

#endif // ESTRANGEDCLIENTRENDERTARGETS_H