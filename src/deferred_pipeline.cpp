#include "deferred_pipeline.h"
#include "camera.h"
#include "render_target.h"
#include "game.h"
#include "scene.h"
#include "component_manager.h"
#include "mesh_component.h"
#include "command_buffer.h"
#include "actor.h"
#include "mesh.h"
#include "material.h"
#include "resources.h"
#include "light.h"
#include "texture.h"
#include "editor_scene_view.h"

void DeferredPipeline::Init(Camera* camera)
{
	this->camera = camera;
	assert(this->camera->GetRenderTarget() != nullptr);

	lightPassMaterial = Resources::GetMaterial("lightPass.json");
	gbufferDebugMaterial = Resources::GetMaterial("gbufferDebug.json");
	renderTargetBlitMaterial = Resources::GetMaterial("renderTargetBlit.json");
	ssrMaterial = Resources::GetMaterial("ssr.json");
	ssrCombineMaterial = Resources::GetMaterial("ssrCombine.json");
	backFaceMaterial = Resources::GetMaterial("meshDepth.json");
	shadowMapMaterial = Resources::GetMaterial("shadowMap.json");
	linesMaterial = Resources::GetMaterial("lines.json");

	// CreateGBuffer
	std::vector<RenderTargetDescriptor> gBufferDescriptors;
	
	// Position
	RenderTargetDescriptor gBufferAttachment0Descriptor = RenderTargetDescriptor();
	gBufferAttachment0Descriptor.format = RENDER_TARGET_FORMAT::RGB32F;
	gBufferAttachment0Descriptor.mipmap = true;

	// Normal Metalness
	RenderTargetDescriptor gBufferAttachment1Descriptor = RenderTargetDescriptor();
	gBufferAttachment1Descriptor.format = RENDER_TARGET_FORMAT::RGBA16F;

	// Albedo Specular
	RenderTargetDescriptor gBufferAttachment2Descriptor = RenderTargetDescriptor();
	gBufferAttachment2Descriptor.format = RENDER_TARGET_FORMAT::RGBA16F;

	gBufferDescriptors.push_back(gBufferAttachment0Descriptor);
	gBufferDescriptors.push_back(gBufferAttachment1Descriptor);
	gBufferDescriptors.push_back(gBufferAttachment2Descriptor);

	gBuffer = new RenderTarget(this->camera->GetRenderTarget()->GetSize().x, this->camera->GetRenderTarget()->GetSize().y, gBufferDescriptors);

	// Create Light Pass
	std::vector<RenderTargetDescriptor> lightPassDescriptors;

	// Color
	RenderTargetDescriptor lightPassColorAttachment0Descriptor = RenderTargetDescriptor();
	lightPassColorAttachment0Descriptor.format = RENDER_TARGET_FORMAT::RGBA16F;

	lightPassDescriptors.push_back(lightPassColorAttachment0Descriptor);

	lightPass = new RenderTarget(this->camera->GetRenderTarget()->GetSize().x, this->camera->GetRenderTarget()->GetSize().y, lightPassDescriptors, false);

	// Create SSR Pass
	std::vector<RenderTargetDescriptor> ssrPassDescriptors;

	// Color
	RenderTargetDescriptor ssrPassColorAttachment0Descriptor = RenderTargetDescriptor();
	ssrPassColorAttachment0Descriptor.format = RENDER_TARGET_FORMAT::RGBA16F;
	ssrPassColorAttachment0Descriptor.mipmap = true;

	ssrPassDescriptors.push_back(ssrPassColorAttachment0Descriptor);

	ssrPass = new RenderTarget(this->camera->GetRenderTarget()->GetSize().x, this->camera->GetRenderTarget()->GetSize().y, ssrPassDescriptors, false);

	// Create SSR Combine Pass
	std::vector<RenderTargetDescriptor> ssrCombinePassDescriptors;

	// Color
	RenderTargetDescriptor ssrCombinePassColorAttachment0Descriptor = RenderTargetDescriptor();
	ssrCombinePassColorAttachment0Descriptor.format = RENDER_TARGET_FORMAT::RGBA16F;
	ssrCombinePassColorAttachment0Descriptor.mipmap = true;

	ssrCombinePassDescriptors.push_back(ssrCombinePassColorAttachment0Descriptor);

	ssrCombinePass = new RenderTarget(this->camera->GetRenderTarget()->GetSize().x, this->camera->GetRenderTarget()->GetSize().y, ssrCombinePassDescriptors, false);

	// Back Face Pass
	std::vector<RenderTargetDescriptor> backFacePassDescriptors;
	RenderTargetDescriptor backFaceAttachment0ColorAttachment0Descriptor = RenderTargetDescriptor();
	backFaceAttachment0ColorAttachment0Descriptor.format = RENDER_TARGET_FORMAT::R32F;
	backFaceAttachment0ColorAttachment0Descriptor.mipmap = true;

	backFacePassDescriptors.push_back(backFaceAttachment0ColorAttachment0Descriptor);

	backFacePass = new RenderTarget(this->camera->GetRenderTarget()->GetSize().x, this->camera->GetRenderTarget()->GetSize().y, backFacePassDescriptors);

	// TODO should use render target to generate radiance map
	radianceMap = Resources::GetTexture("skybox/probe/diffuse/diffuse.json");
	iradianceMap = Resources::GetTexture("skybox/probe/specular/specular0.json");
}

void DeferredPipeline::Resize(const glm::vec2& size)
{
	if (gBuffer != nullptr && lightPass != nullptr)
	{
		gBuffer->Resize(size.x, size.y);
		lightPass->Resize(size.x, size.y);
		ssrPass->Resize(size.x, size.y);
		ssrCombinePass->Resize(size.x, size.y);
		backFacePass->Resize(size.x, size.y);
	}
}

void DeferredPipeline::Bake()
{
	BakeShadowMap();
}

void DeferredPipeline::BakeShadowMap()
{
	Scene* currentScene = Game::ActiveSceneGetPointer();

	std::list<MeshComponent*> meshComponents = ComponentManager::GetInstance()->GetMeshComponents();
	std::list<Light*> lights = ComponentManager::GetInstance()->GetLightComponents();

	glm::mat4 renderTargetProjection = camera->GetRenderTargetProjection();
	glm::mat4 view = camera->GetViewMatrix();
	glm::mat4 invView = glm::inverse(view);
	glm::mat4 projection = camera->GetProjection();
	glm::mat4 invProjection = glm::inverse(projection);

	auto cb = Game::GetCommandBuffer();

	for (auto light : lights)
	{
		if (light->GetCastShadow())
		{
			if (shadowMaps.find(light) == shadowMaps.end())
			{
				// Position
				RenderTargetDescriptor gBufferAttachment0Descriptor = RenderTargetDescriptor();
				gBufferAttachment0Descriptor.format = RENDER_TARGET_FORMAT::R32F;
				gBufferAttachment0Descriptor.mipmap = true;

				std::vector<RenderTargetDescriptor> gBufferDescriptors;
				gBufferDescriptors.push_back(gBufferAttachment0Descriptor);

				// Create Shadow Map
				RenderTarget* shadowMapTarget = new RenderTarget(1024, 1024, gBufferDescriptors);
				shadowMaps[light] = shadowMapTarget;
			}

			cb->SetRenderTarget(shadowMaps[light]);
			cb->SetViewport(glm::vec2(0.0f, 0.0f), glm::vec2(1024.0f, 1024.0f));
			
			// cb->SetClearDepth(1.0f);
			cb->SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

			cb->Clear(CLEAR_BIT::COLOR | CLEAR_BIT::DEPTH);

			if (light->GetLightType() == LightType::Spot)
			{
				glm::mat4 lightView = glm::inverse(light->GetOwner()->GetLocalToWorldMatrix());
				glm::mat4 lightProjection = light->GetLightProjection();

				for (auto meshComponent : meshComponents)
				{
					auto materials = meshComponent->materials;

					Mesh* mesh = meshComponent->mesh;
					assert(mesh != nullptr);

					glm::mat4 transformation = meshComponent->GetOwner()->GetLocalToWorldMatrix();

					for (int i = 0; i < mesh->children.size(); i++)
					{
						Material* material = nullptr;
						if (i >= materials.size())
						{
							material = materials[0];
						}
						else
						{
							material = materials[i];
						}

						if (material->GetPass() != MATERIAL_PASS::DEFERRED)
						{
							continue;
						}

						cb->RenderMeshMVP( shadowMapMaterial, mesh->children[i], transformation, lightView, lightProjection);
					}
				}
			}

			cb->Submit();
		}
	}
}

void DeferredPipeline::Render()
{
	//if (!baked)
	//{
	//	Bake();
	//	baked = true;
	//	return;
	//}

	BakeShadowMap();

	// Render Deferred Pass
	RenderDeferredPass();
	
	if (debugOption == 0)
	{
		// Render Forward Pass
		RenderForwardPass();
	}
	else
	{
		RenderDebugPass();
	}
}

void DeferredPipeline::RenderDeferredPass()
{
	static int frameIndex = 0;

	Scene* currentScene = Game::ActiveSceneGetPointer();

	std::list<MeshComponent*> meshComponents = ComponentManager::GetInstance()->GetMeshComponents();
	std::list<Light*> lights = ComponentManager::GetInstance()->GetLightComponents();

	glm::mat4 renderTargetProjection = camera->GetRenderTargetProjection();
	glm::mat4 view = camera->GetViewMatrix();
	glm::mat4 invView = glm::inverse(view);
	glm::mat4 projection = camera->GetProjection();
	glm::mat4 invProjection = glm::inverse(projection);

	auto cb = Game::GetCommandBuffer();

	// G Buffer Rendering

	cb->SetRenderTarget(gBuffer);

	glm::vec2 renderTargetSize = gBuffer->GetSize();

	cb->SetViewport(glm::vec2(0.0f, 0.0f), glm::vec2(renderTargetSize.x, renderTargetSize.y));
	
	cb->SetClearDepth(1.0f);
	cb->SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

	cb->Clear(CLEAR_BIT::COLOR | CLEAR_BIT::DEPTH);

	for (auto meshComponent : meshComponents)
	{
		if (camera->cullingMask != EVERY_THING)
		{
			if ((meshComponent->GetOwner()->GetLayerFlag() & camera->cullingMask) == 0)
			{
				continue;
			}
		}

		auto materials = meshComponent->materials;

		Mesh* mesh = meshComponent->mesh;
		assert(mesh != nullptr);

		glm::mat4 transformation = meshComponent->GetOwner()->GetLocalToWorldMatrix();

		for (int i = 0; i < mesh->children.size(); i++)
		{
			Material* material = nullptr;
			if (i >= materials.size())
			{
				material = materials[0];
			}
			else
			{
				material = materials[i];
			}

			if (material->GetPass() != MATERIAL_PASS::DEFERRED)
			{
				continue;
			}

			cb->RenderMesh(camera, material, mesh->children[i], transformation);
		}
	}

	// Back Face Rendering
	cb->SetRenderTarget(backFacePass);

	cb->SetViewport(glm::vec2(0.0f, 0.0f), glm::vec2(renderTargetSize.x, renderTargetSize.y));

	cb->SetClearDepth(1.0f);
	cb->SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

	cb->Clear(CLEAR_BIT::COLOR | CLEAR_BIT::DEPTH);
	cb->CullFace(FACE_ORIENTATION::FORWARD);

	for (auto meshComponent : meshComponents)
	{
		if (camera->cullingMask != EVERY_THING)
		{
			if ((meshComponent->GetOwner()->GetLayerFlag() & camera->cullingMask) == 0)
			{
				continue;
			}
		}

		auto materials = meshComponent->materials;

		Mesh* mesh = meshComponent->mesh;
		assert(mesh != nullptr);

		glm::mat4 transformation = meshComponent->GetOwner()->GetLocalToWorldMatrix();

		for (int i = 0; i < mesh->children.size(); i++)
		{
			Material* material = nullptr;
			if (i >= materials.size())
			{
				material = materials[0];
			}
			else
			{
				material = materials[i];
			}

			if (material->GetPass() != MATERIAL_PASS::DEFERRED)
			{
				continue;
			}

			cb->RenderMesh(camera, backFaceMaterial, mesh->children[i], transformation);
		}
	}
	cb->CullFace(FACE_ORIENTATION::BACKWARD);

	// Light Pass Rendering

	// Draw On Light Pass
	cb->SetRenderTarget(lightPass);
	cb->SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
	cb->Clear(CLEAR_BIT::COLOR);

	// G Buffer Pass
	lightPassMaterial->SetTextureProperty("gBufferPosition", GetPositionTexture());
	lightPassMaterial->SetTextureProperty("gBufferNormalMetalness", GetNormalMetalnessTexture());
	lightPassMaterial->SetTextureProperty("gBufferAlbedoRoughness", GetAlbedoRoughnessTexture());

	// Light Uniforms
	int index = 0;
	for (auto light : lights)
	{
		if (index >= 16)
		{
			break;
		}

		// Camera Space
		glm::vec3 lightPos = view * glm::vec4(light->GetOwner()->GetPosition(), 1.0f);

		switch (light->GetLightType())
		{
			case LightType::Point:
			{
				lightPassMaterial->SetInt("lights[" + std::to_string(index) + "].Type", 0);
				lightPassMaterial->SetFloat("lights[" + std::to_string(index) + "].SpotAngle", 0);
				lightPassMaterial->SetVector3("lights[" + std::to_string(index) + "].SpotDir", glm::vec3(0.0f));
			}
			break;
			case LightType::Spot:
			{
				lightPassMaterial->SetInt("lights[" + std::to_string(index) + "].Type", 1);
				lightPassMaterial->SetFloat("lights[" + std::to_string(index) + "].SpotAngle", light->GetSpotAngle());
				lightPassMaterial->SetFloat("lights[" + std::to_string(index) + "].SpotEdgeAngle", light->GetSpotEdgeAngle());
				auto rotationMat = glm::mat3x3(view * light->GetOwner()->GetLocalToWorldMatrix());
				lightPassMaterial->SetVector3("lights[" + std::to_string(index) + "].SpotDir", rotationMat * light->GetSpotDir());
			}
			break;
		}

		bool castShadow = light->GetCastShadow();
		lightPassMaterial->SetInt("lights[" + std::to_string(index) + "].CastShadow", castShadow);
		if (castShadow)
		{
			// auto rotationMat = glm::mat3x3(light->GetOwner()->GetLocalToWorldMatrix());
			// auto spotDir = glm::normalize(rotationMat * light->GetSpotDir());
			glm::mat4 lightView = glm::inverse(light->GetOwner()->GetLocalToWorldMatrix());
			// glm::mat4 lightView = glm::lookAt(light->GetOwner()->GetPosition(), light->GetOwner()->GetPosition() + spotDir, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightProjection = light->GetLightProjection();

			lightPassMaterial->SetTextureProperty("lights[" + std::to_string(index) + "].ShadowMap", shadowMaps[light]->GetAttachmentTexture(0));
			lightPassMaterial->SetMatrix4("lights[" + std::to_string(index) + "].LightProjection", lightProjection);
			lightPassMaterial->SetMatrix4("lights[" + std::to_string(index) + "].LightView", lightView);
		}

		lightPassMaterial->SetVector3("lights[" + std::to_string(index) + "].Color", light->GetLightIntensityColor());
		lightPassMaterial->SetVector3("lights[" + std::to_string(index) + "].Position", lightPos);

		index += 1;
	}

	for (int i = index; i <= 15; i++)
	{
		lightPassMaterial->SetInt("lights[" + std::to_string(index) + "].CastShadow", 0);
		lightPassMaterial->SetVector3("lights[" + std::to_string(index) + "].Color", glm::vec3(0.0f));
		lightPassMaterial->SetVector3("lights[" + std::to_string(index) + "].Position", glm::vec3(0.0f));
		lightPassMaterial->SetInt("lights[" + std::to_string(index) + "].Type", 0);
		lightPassMaterial->SetFloat("lights[" + std::to_string(index) + "].SpotAngle", 0);
		lightPassMaterial->SetVector3("lights[" + std::to_string(index) + "].SpotDir", glm::vec3(0.0f));
	}

	lightPassMaterial->SetTextureProperty("radianceMap", radianceMap);
	lightPassMaterial->SetMatrix4("invView", invView);

	cb->DisableCullFace();
	cb->RenderQuad(glm::vec2(0.0f, 0.0f), renderTargetSize, renderTargetProjection, lightPassMaterial);

	// Render SSR Pass
	cb->SetRenderTarget(ssrPass);
	cb->SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

	// If Camera Moved, Clear, Frame Index Set To 0
	if (EditorSceneView::CameraMoved())
	{
		cb->Clear(CLEAR_BIT::COLOR);
		frameIndex = 0;
		EditorSceneView::ClearCameraMoved();
	}
	// Else, Not Clear, Keep Adding Sample

	// G Buffer Uniforms
	ssrMaterial->SetTextureProperty("gBufferPosition", GetPositionTexture());
	ssrMaterial->SetTextureProperty("gBufferNormalMetalness", GetNormalMetalnessTexture());
	ssrMaterial->SetTextureProperty("gBufferAlbedoRoughness", GetAlbedoRoughnessTexture());
	// SSR Combine Pass Uniform
	ssrMaterial->SetTextureProperty("ssrCombine", ssrCombinePass->GetAttachmentTexture(0));
	ssrMaterial->SetTextureProperty("ssrPass", ssrPass->GetAttachmentTexture(0));
	ssrMaterial->SetTextureProperty("iradianceMap", iradianceMap);
	// Camera Uniforms
	// ssrMaterial->SetMatrix4("view", view);
	ssrMaterial->SetMatrix4("invView", invView);
	ssrMaterial->SetMatrix4("perspectiveProjection", projection);
	ssrMaterial->SetTextureProperty("backZPass", backFacePass->GetAttachmentTexture(0));
	ssrMaterial->SetInt("frameIndex", frameIndex);

	frameIndex++;
	// ssrMaterial->SetMatrix4("invProjection", invProjection);

	cb->RenderQuad(glm::vec2(0.0f, 0.0f), renderTargetSize, renderTargetProjection, ssrMaterial);

	// Render SSR Combine Pass

	cb->SetRenderTarget(ssrCombinePass);
	cb->SetClearColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
	cb->Clear(CLEAR_BIT::COLOR);

	// Combine Uniforms
	ssrCombineMaterial->SetTextureProperty("ssrPass", ssrPass->GetAttachmentTexture(0));
	ssrCombineMaterial->SetTextureProperty("lightPass", lightPass->GetAttachmentTexture(0));

	cb->RenderQuad(glm::vec2(0.0f, 0.0f), renderTargetSize, renderTargetProjection, ssrCombineMaterial);
	cb->EnableCullFace();

	cb->Submit();
}

void DeferredPipeline::RenderForwardPass()
{
	RenderTarget* renderTarget = camera->GetRenderTarget();
	RenderTarget* hdrTarget = camera->GetHdrTarget();

	glm::vec2 renderTargetSize = renderTarget->GetSize();
	Scene* currentScene = Game::ActiveSceneGetPointer();

	std::list<MeshComponent*> meshComponents = ComponentManager::GetInstance()->GetMeshComponents();

	std::list<Light*> lights = ComponentManager::GetInstance()->GetLightComponents();

	auto cb = Game::GetCommandBuffer();

	if (camera->hasPostProcessing)
	{
		cb->SetRenderTarget(hdrTarget);
	}
	else
	{
		cb->SetRenderTarget(renderTarget);
	}

	cb->SetViewport(glm::vec2(0.0f, 0.0f), glm::vec2(renderTargetSize.x, renderTargetSize.y));
	
	cb->SetClearColor(camera->clearColor);
	cb->Clear(CLEAR_BIT::COLOR);

	cb->CopyDepthBuffer(gBuffer, (camera->hasPostProcessing?hdrTarget:renderTarget));

	// Draw SSR Combine Pass On Current Render Target
	cb->DisableDepth();
	cb->DisableCullFace();
	renderTargetBlitMaterial->SetTextureProperty("renderTarget", ssrCombinePass->GetAttachmentTexture(0));
	cb->RenderQuad(glm::vec2(0.0f, 0.0f), renderTargetSize, camera->GetRenderTargetProjection(), renderTargetBlitMaterial);
	cb->EnableCullFace();
	cb->EnableDepth();

	for (auto meshComponent : meshComponents)
	{
		if (camera->cullingMask != EVERY_THING)
		{
			if ((meshComponent->GetOwner()->GetLayerFlag() & camera->cullingMask) == 0)
			{
				continue;
			}
		}

		auto materials = meshComponent->materials;

		Mesh* mesh = meshComponent->mesh;
		assert(mesh != nullptr);

		glm::mat4 transformation = meshComponent->GetOwner()->GetLocalToWorldMatrix();

		for (int i = 0; i < mesh->children.size(); i++)
		{
			Material* material = nullptr;
			if (i >= materials.size())
			{
				material = materials[0];
			}
			else
			{
				material = materials[i];
			}

			if (material->GetPass() != MATERIAL_PASS::FORWARD)
			{
				continue;
			}

			cb->RenderMesh(camera, material, mesh->children[i], transformation);
		}
	}

	for (auto meshComponent : meshComponents)
	{
		auto materials = meshComponent->materials;

		Mesh* mesh = meshComponent->mesh;
		assert(mesh != nullptr);

		glm::mat4 transformation = meshComponent->GetOwner()->GetLocalToWorldMatrix();

		for (int i = 0; i < mesh->children.size(); i++)
		{
			Material* material = linesMaterial;

			if (material->GetPass() != MATERIAL_PASS::FORWARD)
			{
				continue;
			}

			cb->RenderMesh(camera, material, mesh->children[i], transformation);
		}
	}

	// Render skybox
	cb->RenderSkyBox(glm::mat4(glm::mat3(camera->GetViewMatrix())), camera->GetProjection());

	// Post Processing
	if (camera->hasPostProcessing)
	{
		cb->SetRenderTarget(renderTarget);
		cb->DisableDepth();

		camera->GetPostProcessingMaterial()->SetTextureProperty("hdrTarget", hdrTarget->GetAttachmentTexture(0));
		camera->GetPostProcessingMaterial()->SetFloat("exposure", 1.5f);

		cb->DisableCullFace();
		cb->RenderQuad(glm::vec2(0.0f, 0.0f), renderTargetSize, camera->GetRenderTargetProjection(), camera->GetPostProcessingMaterial());
		cb->EnableCullFace();
		cb->EnableDepth();
	}

	cb->Submit();
}

void DeferredPipeline::RenderDebugPass()
{
	if (gBuffer == nullptr)
	{
		return;
	}

	RenderTarget* renderTarget = camera->GetRenderTarget();
	RenderTarget* hdrTarget = camera->GetHdrTarget();

	glm::vec2 renderTargetSize = renderTarget->GetSize();
	Scene* currentScene = Game::ActiveSceneGetPointer();

	std::list<MeshComponent*> meshComponents = ComponentManager::GetInstance()->GetMeshComponents();

	std::list<Light*> lights = ComponentManager::GetInstance()->GetLightComponents();

	auto cb = Game::GetCommandBuffer();
	cb->SetRenderTarget(renderTarget);

	if (renderTarget->HasDepth())
	{
		cb->EnableDepth();
	}

	cb->SetViewport(glm::vec2(0.0f, 0.0f), glm::vec2(renderTargetSize.x, renderTargetSize.y));

	cb->DisableDepth();

	gbufferDebugMaterial->SetInt("debugOption", debugOption);
	gbufferDebugMaterial->SetTextureProperty("gBufferPosition", GetPositionTexture());
	gbufferDebugMaterial->SetTextureProperty("gBufferNormalMetalness", GetNormalMetalnessTexture());
	gbufferDebugMaterial->SetTextureProperty("gBufferAlbedoRoughness", GetAlbedoRoughnessTexture());
	gbufferDebugMaterial->SetTextureProperty("gBufferDepth", GetDepthTexture());

	gbufferDebugMaterial->SetTextureProperty("lightPass", lightPass->GetAttachmentTexture(0));
	gbufferDebugMaterial->SetTextureProperty("ssrPass", ssrPass->GetAttachmentTexture(0));
	gbufferDebugMaterial->SetTextureProperty("ssrCombinePass", ssrCombinePass->GetAttachmentTexture(0));
	gbufferDebugMaterial->SetTextureProperty("backFacePass", backFacePass->GetAttachmentTexture(0));

	if (shadowMaps.size() >= 1)
	{
		gbufferDebugMaterial->SetTextureProperty("shadowMap", shadowMaps.begin()->second->GetAttachmentTexture(0));
	}

	cb->DisableCullFace();
	cb->RenderQuad(glm::vec2(0.0f, 0.0f), renderTargetSize, camera->GetRenderTargetProjection(), gbufferDebugMaterial);
	cb->EnableCullFace();

	cb->EnableDepth();

	cb->Submit();
}

Texture* DeferredPipeline::GetPositionTexture() const
{
	if (gBuffer != nullptr)
	{
		return gBuffer->GetAttachmentTexture(0);
	}

	return nullptr;
}

Texture* DeferredPipeline::GetNormalMetalnessTexture() const
{
	if (gBuffer != nullptr)
	{
		return gBuffer->GetAttachmentTexture(1);
	}

	return nullptr;
}

Texture* DeferredPipeline::GetAlbedoRoughnessTexture() const
{
	if (gBuffer != nullptr)
	{
		return gBuffer->GetAttachmentTexture(2);
	}

	return nullptr;
}

Texture* DeferredPipeline::GetDepthTexture() const
{
	if (gBuffer != nullptr)
	{
		return gBuffer->GetDepthTexture();
	}

	return nullptr;
}