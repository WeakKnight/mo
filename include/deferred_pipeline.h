#pragma once
#include "common.h"

class Camera;
class RenderTarget;
class Texture;
class Material;
class Light;

class DeferredPipeline
{
public:
	void Init(Camera* camera);

	void Bake();
	void BakeShadowMap();

	void Render();

	void RenderDeferredPass();
	void RenderForwardPass();
	void RenderDebugPass();

	void Resize(const glm::vec2& size);

	Texture* GetPositionTexture() const;
	Texture* GetNormalMetalnessTexture() const;
	Texture* GetAlbedoRoughnessTexture() const;
	Texture* GetDepthTexture() const;

	// 0 ---- without debug
	int debugOption = 0;

private:
	bool baked = false;

	Camera* camera = nullptr;

	// Deferred Material
	Material* lightPassMaterial = nullptr;
	Material* gbufferDebugMaterial = nullptr;
	Material* renderTargetBlitMaterial = nullptr;
	// SSR Material
	Material* ssrMaterial = nullptr;
	Material* ssrCombineMaterial = nullptr;

	// Back Face Material
	Material* backFaceMaterial = nullptr;

	// Shadow Map Material
	Material* shadowMapMaterial = nullptr;
	
	// Deferred Pass
	RenderTarget* gBuffer = nullptr;
	RenderTarget* lightPass = nullptr;

	// SSR PASS
	RenderTarget* ssrPass = nullptr;
	RenderTarget* ssrCombinePass = nullptr;

	// For Thickness
	RenderTarget* backFacePass = nullptr;

	std::map<Light*, RenderTarget*> shadowMaps;

	// Texture
	// TODO Use RenderTarget To Generate Radiance Map
	Texture* radianceMap = nullptr;
	Texture* iradianceMap = nullptr;
};