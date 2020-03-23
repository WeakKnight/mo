#include "command_buffer.h"
#include <glad/glad.h>
#include "mesh.h"
#include "camera.h"
#include "render_target.h"
#include "material.h"
#include "gpu_buffer.h"
#include "time_manager.h"
#include "game.h"
#include "scene.h"
#include "component_manager.h"
#include "light.h"
#include "actor.h"
#include "vertex_array.h"
#include "resources.h"

float quadVertices[] =
{
	0.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,

	0.0f, 1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f, 1.0f,
	1.0f, 0.0f, 1.0f, 0.0f
};

float skyboxVertices[] = {       
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};

void GLStateCache::Init()
{

}

CommandBuffer::CommandBuffer()
{
	const GLubyte* glVersion = glGetString(GL_VERSION);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glPatchParameteri(GL_PATCH_VERTICES, 3);

	spdlog::info("GL Version Is {}", glVersion);

	quadVertexBuffer = new GPUBuffer();
	quadVertexBuffer->SetData(BUFFER_USAGE::ARRAY, quadVertices, sizeof(quadVertices), BUFFER_DRAW_TYPE::STATIC_DRAW);
	quadVertexArray = new VertexArray(quadVertexBuffer, nullptr);
	quadVertexArray->LayoutAddFloat2();
	quadVertexArray->LayoutAddFloat2();
	quadVertexArray->UpdateLayoutToGPU();

	skyBoxVertexBuffer = new GPUBuffer();
	skyBoxVertexBuffer->SetData(BUFFER_USAGE::ARRAY, skyboxVertices, sizeof(skyboxVertices), BUFFER_DRAW_TYPE::STATIC_DRAW);

	skyBoxVertexArray = new VertexArray(skyBoxVertexBuffer, nullptr);
	skyBoxVertexArray->LayoutAddFloat3();
	skyBoxVertexArray->UpdateLayoutToGPU();

	skyBoxMaterial = Resources::GetMaterial("skybox.json");
}

void CommandBuffer::Clear(unsigned int mask)
{
	std::shared_ptr<CommandClear> cmd = std::make_shared<CommandClear>();
	cmd->mask = mask;

	AddCommand(cmd);
}

void CommandBuffer::SetClearDepth(float depth)
{
	std::shared_ptr<CommandSetClearDepth> cmd = std::make_shared<CommandSetClearDepth>();
	cmd->depth = depth;

	// AddCommand(cmd);
}

void CommandBuffer::SetClearColor(const glm::vec4& color)
{
	std::shared_ptr<CommandSetClearColor> cmd = std::make_shared<CommandSetClearColor>();
	cmd->color = color;

	AddCommand(cmd);
}

void CommandBuffer::SetViewport(const glm::vec2& origin, const glm::vec2& size)
{
	std::shared_ptr<CommandSetViewPort> cmd = std::make_shared<CommandSetViewPort>();
	cmd->origin = origin;
	cmd->contentSize = size;

	AddCommand(cmd);
}

void CommandBuffer::SetRenderTarget(RenderTarget* renderTarget)
{
	std::shared_ptr<CommandSetRenderTarget> cmd = std::make_shared<CommandSetRenderTarget>();
	cmd->renderTarget = renderTarget;

	AddCommand(cmd);
}

void CommandBuffer::RenderMesh(Camera* camera, Material* material, SubMesh* mesh, glm::mat4& transformation)
{
	std::shared_ptr<CommandRenderMesh> cmd = std::make_shared<CommandRenderMesh>();
	cmd->camera = camera;
	cmd->material = material;
	cmd->mesh = mesh;
	cmd->transformation = transformation;

	AddCommand(cmd);
}

void CommandBuffer::RenderMeshMVP(Material* material, SubMesh* mesh, glm::mat4& model, glm::mat4& view, glm::mat4& projection)
{
	std::shared_ptr<CommandRenderMeshMVP> cmd = std::make_shared<CommandRenderMeshMVP>();
	cmd->view = view;
	cmd->material = material;
	cmd->mesh = mesh;
	cmd->model = model;
	cmd->projection = projection;

	AddCommand(cmd);
}

void CommandBuffer::RenderQuad(const glm::vec2& position, const glm::vec2& size, const glm::mat4& projection, Material* material)
{
	std::shared_ptr<CommandRenderQuad> cmd = std::make_shared<CommandRenderQuad>();
	cmd->material = material;

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(position, 0.0f));
	model = glm::scale(model, glm::vec3(size, 1.0f));

	cmd->model = model;
	cmd->projection = projection;

	AddCommand(cmd);
}

void CommandBuffer::RenderSkyBox(const glm::mat4& view, const glm::mat4& projection)
{
	std::shared_ptr<CommandRenderSkybox> cmd = std::make_shared<CommandRenderSkybox>();
	cmd->view = view;
	cmd->projection = projection;

	AddCommand(cmd);
}

void CommandBuffer::EnableDepth()
{
	std::shared_ptr<CommandEnableDepth> cmd = std::make_shared<CommandEnableDepth>();
	AddCommand(cmd);
}

void CommandBuffer::DisableDepth()
{
	std::shared_ptr<CommandDisableDepth> cmd = std::make_shared<CommandDisableDepth>();
	AddCommand(cmd);
}

void CommandBuffer::CopyDepthBuffer(RenderTarget* src, RenderTarget* dst)
{
	std::shared_ptr<CommandCopyDepthBuffer> cmd = std::make_shared<CommandCopyDepthBuffer>();
	cmd->src = src;
	cmd->dst = dst;

	AddCommand(cmd);
}

void CommandBuffer::EnableCullFace()
{
	std::shared_ptr<CommandEnableCullFace> cmd = std::make_shared<CommandEnableCullFace>();
	AddCommand(cmd);
}

void CommandBuffer::DisableCullFace()
{
	std::shared_ptr<CommandDisableCullFace> cmd = std::make_shared<CommandDisableCullFace>();
	AddCommand(cmd);
}

void CommandBuffer::CullFace(FACE_ORIENTATION faceOrientation)
{
	std::shared_ptr<CommandCullFace> cmd = std::make_shared<CommandCullFace>();
	cmd->faceOrientation = faceOrientation;

	AddCommand(cmd);
}

void CommandBuffer::Submit()
{
	// TODO Sort And Merge

	// prepare common variables
	std::list<Light*> lights = ComponentManager::GetInstance()->GetLightComponents();
	Scene* scene = Game::ActiveSceneGetPointer();
	stateCahce.currentRenderTarget = nullptr;

	for (auto cmd : commandList)
	{
		switch (cmd->commandType)
		{
		case RENDER_COMMAND_TYPE::CLEAR:
		{
			auto clearCmd = std::static_pointer_cast<CommandClear>(cmd);
			unsigned int bufferBit = 0;
			if ((clearCmd->mask >> 0) && 1)
			{
				bufferBit = bufferBit | GL_COLOR_BUFFER_BIT;
			}

			if ((clearCmd->mask >> 1) && 1)
			{
				bufferBit = bufferBit | GL_DEPTH_BUFFER_BIT;
			}

			if ((clearCmd->mask >> 2) && 1)
			{
				bufferBit = bufferBit | GL_STENCIL_BUFFER_BIT;
			}

			glClear(bufferBit);
		}
			break;
		case RENDER_COMMAND_TYPE::RENDER_SKYBOX:
		{
			auto skyBoxCmd = std::static_pointer_cast<CommandRenderSkybox>(cmd);
			glDepthFunc(GL_LEQUAL);

			skyBoxMaterial->SetMatrix4("view", skyBoxCmd->view);
			skyBoxMaterial->SetMatrix4("projection", skyBoxCmd->projection);

			skyBoxMaterial->Use();

			skyBoxVertexArray->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 36);
			skyBoxVertexArray->UnBind();

			glDepthFunc(GL_LESS);
		}
			break;
		case RENDER_COMMAND_TYPE::RENDER_QUAD:
		{
			auto quadCmd = std::static_pointer_cast<CommandRenderQuad>(cmd);
			auto material = quadCmd->material;
			
			std::vector<MATERIAL_EXTENSION> extensions = material->GetExtensions();
			for (auto extension : extensions)
			{
				if (extension == MATERIAL_EXTENSION::TIME)
				{
					material->SetFloat("time", Time::GetTime());
				}
			}

			material->SetMatrix4("model", quadCmd->model);
			material->SetMatrix4("projection", quadCmd->projection);

			material->Use();
			
			quadVertexArray->Bind();
			glDrawArrays(GL_TRIANGLES, 0, 6);
			quadVertexArray->UnBind();
		}
			break;
		case RENDER_COMMAND_TYPE::RENDER_MESH:
		{
			auto meshCmd = std::static_pointer_cast<CommandRenderMesh>(cmd);
			auto material = meshCmd->material;
			auto camera = meshCmd->camera;
			auto transformation = meshCmd->transformation;
			auto mesh = meshCmd->mesh;
			auto light = lights.front();

			std::vector<MATERIAL_EXTENSION> extensions = material->GetExtensions();
			for (auto extension : extensions)
			{
				if (extension == MATERIAL_EXTENSION::MODEL_VIEW_PROJECTION)
				{
					material->SetUniformBlock("CameraBlock", 0);
					camera->GetUniformBlock()->BindBufferBase(BUFFER_USAGE::UNIFORM, 0);

					material->SetMatrix4("model", transformation);
				}
				else if (extension == MATERIAL_EXTENSION::TIME)
				{
					material->SetFloat("time", Time::GetTime());
				}
				else if (extension == MATERIAL_EXTENSION::AMBIENT)
				{
					material->SetVector3("ambient", scene->GetAmbient());
				}
				else if (extension == MATERIAL_EXTENSION::NEAREST_LIGHT)
				{
					int index = 0;
					for (auto light: lights)
					{
						if (index >= 16)
						{
							break;
						}

						material->SetVector3("lights[" + std::to_string(index) + "].Color", light->GetLightIntensityColor());
						material->SetVector3("lights[" + std::to_string(index) + "].Position", light->GetOwner()->GetPosition());

						index += 1;
					}

					for (int i = index; i <= 15; i++)
					{
						material->SetVector3("lights[" + std::to_string(index) + "].Color", glm::vec3(0.0f));
						material->SetVector3("lights[" + std::to_string(index) + "].Position", glm::vec3(0.0f));	
					}
				}
				else if (extension == MATERIAL_EXTENSION::CAMERA)
				{
					material->SetVector3("cameraPos", camera->GetOwner()->GetPosition());
				}
			}

			material->Use();

			mesh->vertexArray->Bind();
			if (material->HasTess())
			{
				glDrawElements(GL_PATCHES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
			}
			else
			{
				glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
			}
			mesh->vertexArray->UnBind();
		}
			break;
		case RENDER_COMMAND_TYPE::RENDER_MESH_MVP:
		{
			auto meshCmd = std::static_pointer_cast<CommandRenderMeshMVP>(cmd);
			auto material = meshCmd->material;
			auto mesh = meshCmd->mesh;

			material->SetMatrix4("model", meshCmd->model);
			material->SetMatrix4("view", meshCmd->view);
			material->SetMatrix4("projection", meshCmd->projection);
			material->Use();

			mesh->vertexArray->Bind();
			if (material->HasTess())
			{
				glDrawElements(GL_PATCHES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
			}
			else
			{
				glDrawElements(GL_TRIANGLES, mesh->indices.size(), GL_UNSIGNED_INT, 0);
			}
			mesh->vertexArray->UnBind();
		}
		break;
		case RENDER_COMMAND_TYPE::SET_CLEAR_COLOR:
		{
			auto setCmd = std::static_pointer_cast<CommandSetClearColor>(cmd);
			glClearColor(setCmd->color.x, setCmd->color.y, setCmd->color.z, setCmd->color.w);
		}
			break;
		case RENDER_COMMAND_TYPE::SET_CLEAR_DEPTH:
		{
			auto setCmd = std::static_pointer_cast<CommandSetClearDepth>(cmd);
			glClearDepth(setCmd->depth);
		}
			break;
		case RENDER_COMMAND_TYPE::SET_RENDER_TARGET:
		{
			auto setCmd = std::static_pointer_cast<CommandSetRenderTarget>(cmd);
			stateCahce.currentRenderTarget = setCmd->renderTarget;
			setCmd->renderTarget->Bind();
		}
			break;
		case RENDER_COMMAND_TYPE::SET_VIEW_PORT:
		{
			auto setCmd = std::static_pointer_cast<CommandSetViewPort>(cmd);
			glViewport(setCmd->origin.x, setCmd->origin.y, setCmd->contentSize.x, setCmd->contentSize.y);
		}
			break;
		case RENDER_COMMAND_TYPE::COPY_DEPTH_BUFFER:
		{
			auto copyCmd = std::static_pointer_cast<CommandCopyDepthBuffer>(cmd);
			copyCmd->dst->CopyDepth(copyCmd->src);
		}
		break;
		case RENDER_COMMAND_TYPE::ENABLE_DEPTH:
		{
			glEnable(GL_DEPTH_TEST);
		}
		break;
		case RENDER_COMMAND_TYPE::DISABLE_DEPTH:
		{
			glDisable(GL_DEPTH_TEST);
		}
		break;
		case RENDER_COMMAND_TYPE::ENABLE_CULL_FACE:
		{
			glEnable(GL_CULL_FACE);
		}
		break;
		case RENDER_COMMAND_TYPE::DISABLE_CULL_FACE:
		{
			glDisable(GL_CULL_FACE);
		}
		break;
		case RENDER_COMMAND_TYPE::CULL_FACE:
		{
			auto cullFaceCmd = std::static_pointer_cast<CommandCullFace>(cmd);
			if (cullFaceCmd->faceOrientation == FACE_ORIENTATION::FORWARD)
			{
				glCullFace(GL_FRONT);
			}
			else if (cullFaceCmd->faceOrientation == FACE_ORIENTATION::BACKWARD)
			{
				glCullFace(GL_BACK);
			}
			else
			{
				glCullFace(GL_FRONT_AND_BACK);
			}
		}
		break;
		default :
			spdlog::info("Unsupported Cmd");
			break;
		}
	}

	if (stateCahce.currentRenderTarget != nullptr)
	{
		stateCahce.currentRenderTarget->Unbind();
	}

	commandList.clear();
}