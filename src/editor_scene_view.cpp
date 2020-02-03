#include "editor_scene_view.h"

#include "render_target.h"
#include "texture.h"
#include "camera.h"
#include "game.h"
#include "scene.h"
#include "time_manager.h"

EditorSceneView::EditorSceneView(unsigned int initialWidth, unsigned int initialHeight, bool initialOpen, std::string title) : EditorWindow(initialWidth, initialHeight, initialOpen, title)
{
    sceneViewRenderTarget = new RenderTarget(initialWidth, initialHeight);
    
    sceneCamera = new Camera();
    sceneCamera->renderTarget = sceneViewRenderTarget;
    sceneCamera->SetLocalPosition(glm::vec3(0.0f, 2.0f, 6.0f));

    Game::ActiveSceneGetPointer()->rootNode->AddChild(sceneCamera);
}

//void EditorSceneView::SetSceneViewRenderTarget(RenderTarget* renderTarget)
//{
//    sceneViewRenderTarget = renderTarget;
//}

void EditorSceneView::OnIMGUI() 
{
    OnSceneCameraControl();

    if (!initialized)
    {
        initialized = true;
        sceneViewRenderTarget->Resize(contentSize.x, contentSize.y);
    }
    else
    {
        auto textureId = sceneViewRenderTarget->GetAttachmentTexture(0)->AsID();
        ImGui::Image((ImTextureID)textureId, sceneViewRenderTarget->GetSize(), ImVec2(0, 1), ImVec2(1, 0));
    }
}

void EditorSceneView::OnResize() 
{
    sceneViewRenderTarget->Resize(contentSize.x, contentSize.y);
}

void EditorSceneView::OnSceneCameraControl()
{
    auto io = ImGui::GetIO();
    float deltaTime = Time::GetDeltaTime();

    if (ImGui::IsWindowHovered())
    {   
        float wheelValue = io.MouseWheel;
        if (wheelValue != 0.0f)
        {
            sceneCamera->Translate(wheelValue * 2.0f * deltaTime * glm::vec3(0.0f, 0.0f, -1.0f));
        }
    }
}