#include "game.h"
#include "scene.h"

#include "editor_window_system.h"
#include "render_target.h"

#include "editor_scene_view.h"
#include "camera.h"
#include "serialization.h"
#include "string_utils.h"
#include "configs.h"
#include "component_manager.h"
#include "command_buffer.h"
#include "resources.h"

namespace Game
{
    Scene* activeScene = nullptr;
    RenderTarget* mainRenderTarget = nullptr;
    glm::vec4 clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);
    CommandBuffer* commandBuffer = nullptr;
    std::string environmentPath;

    void InitEntryScene()
    {
        auto scenePath = "scenes/" + Configuration::GetEntryScene();
        auto sceneContent = StringUtils::ReadFile(scenePath);
        activeScene = Serialization::DeserializeScene(sceneContent);
    }

    void Init()
    {
        commandBuffer = new CommandBuffer();

        std::vector<RenderTargetDescriptor> renderTargetDescriptors;
        RenderTargetDescriptor descriptor = RenderTargetDescriptor();
        descriptor.format = RENDER_TARGET_FORMAT::RGBA8888;
        renderTargetDescriptors.push_back(descriptor);
        
        mainRenderTarget = new RenderTarget(300, 300, renderTargetDescriptors, false, false);
        
        ComponentManager::Init();
        Serialization::LoadProject();
        InitEntryScene();

        EditorWindowSystem::Init();
    }
   
    void Update()
    {
        activeScene->Tick();
        EditorWindowSystem::GetInstance()->Update();
    }

    void PreRender()
    {
        auto cameras = ComponentManager::GetInstance()->GetCameraComponents();
        for (auto camera : cameras)
        {
            camera->PreRender();
        }
    }

    void Render()
    {
        auto cameras = ComponentManager::GetInstance()->GetCameraComponents();
        for (auto camera:cameras)
        {
            camera->Render();
        }
    }

    void End()
    {
        EditorWindowSystem::Destroy();

        if (activeScene != nullptr)
        {
            delete activeScene;
        }

        delete mainRenderTarget;
    }

    void SetEnvironmentPath(const std::string& path)
    {
        environmentPath = path;
    }
    /*
   API
   */
    CommandBuffer* GetCommandBuffer()
    {
        return commandBuffer;
    }
    RenderTarget* MainRenderTargetGetPointer()
    {
        return mainRenderTarget;
    }

    glm::vec2 MainRenderTargetGetSize()
    {
        return mainRenderTarget->GetSize();
    }

    Scene* ActiveSceneGetPointer()
    {
        return activeScene;
    }


    std::string GetEnviromentPath()
    {
        return environmentPath;
    }
} // namespace Game