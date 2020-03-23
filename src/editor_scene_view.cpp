#include "editor_scene_view.h"

#include "render_target.h"
#include "texture.h"
#include "camera.h"
#include "game.h"
#include "scene.h"
#include "time_manager.h"
#include "ray_cast.h"
#include "editor_window_system.h"
#include "ImGuizmo.h"
#include "input.h"
#include "component_manager.h"
#include "actor.h"
#include "mesh_component.h"
#include "event.h"
#include "debug.h"
#include "deferred_pipeline.h"

constexpr float CAMERA_SPEED = 12.0f;
constexpr float CAMERA_SENSITIVITY = 0.2f;

static ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
static Camera* cameraCom = nullptr;

static float yaw = -90.0f;
static float pitch = 0.0f;

static bool cameraMoved = false;

EditorSceneView::EditorSceneView(unsigned int initialWidth, unsigned int initialHeight, bool initialOpen, std::string title) : EditorWindow(initialWidth, initialHeight, initialOpen, title)
{    
    sceneCamera = new Actor();

    cameraCom = ComponentManager::GetInstance()->CreateCameraComponent();
    cameraCom->SetRenderTarget(Game::MainRenderTargetGetPointer());

    sceneCamera->AddComponent(cameraCom);

    sceneCamera->SetPositionLocal(glm::vec3(0.0f, 0.0f, 12.0f));
    
    sceneCamera->SetLayerFlag(LAYER_MASK::ONLY_FOR_EDITOR_OBJECTS);
    sceneCamera->SetPropertyFlag(PROPERTY_MASK::NON_SERIALIZED);

    Game::ActiveSceneGetPointer()->rootNode->AddChild(sceneCamera);
}

glm::vec2 WorldToScreen(glm::mat4& projection, glm::mat4& view, glm::vec2 screenSize, glm::vec3 position)
{
    auto clipSpacePos = projection * (view * glm::vec4(position, 1.0f));
    auto ndcSpacePos = glm::vec3(clipSpacePos.x, clipSpacePos.y, clipSpacePos.z) / clipSpacePos.w;

    glm::vec2 windowSpacePos = ((glm::vec2(ndcSpacePos.x, -ndcSpacePos.y) + 1.0f) / 2.0f) * glm::vec2(screenSize.x, screenSize.y);
    return windowSpacePos;
}

void EditorSceneView::OnIMGUI() 
{
    OnSceneCameraControl();

    // Object Picking
    auto io = ImGui::GetIO();
    auto drawList = ImGui::GetWindowDrawList();
    const glm::vec2 p = ImGui::GetCursorScreenPos();

    if (!initialized)
    {
        initialized = true;
        Game::MainRenderTargetGetPointer()->Resize(contentSize.x, contentSize.y);
    }
    else
    {
        auto textureId = Game::MainRenderTargetGetPointer()->GetAttachmentTexture(0)->AsID();
        ImGui::Image((ImTextureID)textureId, Game::MainRenderTargetGetPointer()->GetSize(), ImVec2(0, 1), ImVec2(1, 0));
    }

    ImGui::SetCursorPos(glm::vec2(20.0f, 30.0f));
    ImGui::SetNextItemWidth(90.0f);
    const char* items[] = { "Shading", "Albedo", "Position", "Normal", "Roughness", "Metalness", "Depth", "LightPass", "SSR", "SSR Combine", "Back Z", "Shadow Map"};
    static int item_current = 0;
    ImGui::Combo("View", &item_current, items, IM_ARRAYSIZE(items));
    ImGui::SetCursorPos(glm::vec2(0.0f, 0.0f));

    DeferredPipeline* pipleLine = cameraCom->GetPipeline();
    pipleLine->debugOption = item_current;

    ImGui::SameLine();

    static bool guizmoOpen = false;
    ImGui::Checkbox("Guizmo", &guizmoOpen);

    ImGui::SameLine();
    ImGui::Checkbox("Wired Frame", &pipleLine->showWiredFrame);

    auto projection = cameraCom->GetProjection();
    auto view = cameraCom->GetViewMatrix();

    auto renderables = ComponentManager::GetInstance()->GetMeshComponents();
    
    // Draw Guizmo
    if(guizmoOpen)
    {
        for (auto renderable : renderables)
        {
            auto aabb = renderable->GetOwner()->GetAABB();
            // Face 1
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[0].y, aabb[0].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[0].y, aabb[0].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[0].y, aabb[0].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[0].y, aabb[1].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[0].y, aabb[1].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[0].y, aabb[1].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[0].y, aabb[1].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[0].y, aabb[0].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            // Face 2
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[1].y, aabb[0].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[1].y, aabb[0].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[1].y, aabb[0].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[1].y, aabb[1].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[1].y, aabb[1].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[1].y, aabb[1].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[1].y, aabb[1].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[1].y, aabb[0].z)),
                ImColor(1.0f, 0.0f, 0.0f));

            drawList->AddLine(
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[0].y, aabb[0].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[1].y, aabb[0].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[1].y, aabb[1].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[0].x, aabb[0].y, aabb[1].z)),
                ImColor(1.0f, 0.0f, 0.0f));

            drawList->AddLine(
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[0].y, aabb[0].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[1].y, aabb[0].z)),
                ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[1].y, aabb[1].z)),
                p + WorldToScreen(projection, view, contentSize, glm::vec3(aabb[1].x, aabb[0].y, aabb[1].z)),
                ImColor(1.0f, 0.0f, 0.0f));

        }
    }

    auto windowPos = glm::vec2(ImGui::GetMousePos()) - contentMin;
   
    if (ImGui::IsWindowHovered())
    {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver())
        {
            // another
            auto cameraRay = cameraCom->ScreenRay(windowPos.x, windowPos.y);

            RayCastInteraction interaction;
            Physics::RayCast(cameraRay, LAYER_MASK::ONLY_FOR_EDITOR_OBJECTS, interaction);

            if (interaction.target != nullptr)
            {
                EditorWindowSystem::GetInstance()->SetActorSelection(interaction.target);
                Debug::Log("Actor %s Casted By Ray", interaction.target->GetName().c_str());
            }
            else
            {
                EditorWindowSystem::GetInstance()->ClearActorSelection();
            }
        }
    }
    

    auto selection = EditorWindowSystem::GetInstance()->GetActorSelection();
    if (selection.size() == 1)
    {
        auto actor = selection[0];
            
        if (Input::CheckKey(KEYBOARD_KEY::W))
        {
            operation = ImGuizmo::OPERATION::TRANSLATE;
        }
        else if (Input::CheckKey(KEYBOARD_KEY::E))
        {
            operation = ImGuizmo::OPERATION::ROTATE;
        }
        else if (Input::CheckKey(KEYBOARD_KEY::R))
        {
            operation = ImGuizmo::OPERATION::SCALE;
        }
            
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(contentMin.x, contentMin.y, contentSize.x, contentSize.y);
        glm::mat4 matrix = actor->GetLocalToWorldMatrix();
        glm::mat4 delta = glm::mat4(1.0f);
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation, ImGuizmo::MODE::LOCAL, glm::value_ptr(matrix), glm::value_ptr(delta));
            
        glm::vec3 deltaPosition;
        glm::vec3 deltaRotation;
        glm::vec3 deltaScale;

        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(delta), glm::value_ptr(deltaPosition), glm::value_ptr(deltaRotation), glm::value_ptr(deltaScale));

        if ((Math::AbsSum(deltaPosition) + Math::AbsSum(deltaRotation)) > 0.0f || Math::AbsSum(deltaScale) != 3.0f)
        {
            glm::vec3 worldPosition;
            glm::vec3 worldRotation;
            glm::vec3 worldScale;

            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), glm::value_ptr(worldPosition), glm::value_ptr(worldRotation), glm::value_ptr(worldScale));

            actor->SetScale(worldScale);
            actor->SetRotation(worldRotation);
            actor->SetPosition(worldPosition);

            SelectionPropertyChangeEvent e;
            Dispatch(e);
        }
    }  
}

void EditorSceneView::OnResize() 
{
    spdlog::info("Scene Render Target Is {}", Game::MainRenderTargetGetPointer()->GetGPUHandle());
    Game::MainRenderTargetGetPointer()->Resize(contentSize.x, contentSize.y);
}

bool EditorSceneView::CameraMoved()
{
    return cameraMoved;
}

void EditorSceneView::ClearCameraMoved()
{
    cameraMoved = false;
}

void EditorSceneView::OnSceneCameraControl()
{
    auto io = ImGui::GetIO();
    float deltaTime = Time::GetDeltaTime();

    if (ImGui::IsWindowHovered())
    {   
      /*  auto selection = EditorWindowSystem::GetInstance()->GetActorSelection();
        if (selection.size() <= 0)
        {
            return;
        }
        
        auto actor = selection[0];
        auto target = actor;*/
        auto target = sceneCamera;
        float wheelValue = io.MouseWheel;
        if (wheelValue != 0.0f)
        {
            target->Translate(wheelValue * CAMERA_SPEED * deltaTime * cameraCom->cameraFront);
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            glm::vec2 delta = io.MouseDelta;
            target->Translate(deltaTime * CAMERA_SPEED * 0.5f * (-glm::normalize(glm::cross(cameraCom->cameraFront, cameraCom->cameraUp)) * delta.x + cameraCom->cameraUp * delta.y));
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            glm::vec2 offset = io.MouseDelta;
            float xoffset = offset.x;
            float yoffset = offset.y;

            xoffset *= CAMERA_SENSITIVITY;
            yoffset *= CAMERA_SENSITIVITY;

            yaw += xoffset;
            pitch -= yoffset;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;

            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraCom->cameraFront = glm::normalize(front);

            cameraMoved = true;
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            float cameraSpeed = CAMERA_SPEED * Time::GetDeltaTime();
           
            if (Input::GetKeyState(KEYBOARD_KEY::W) == KEY_STATE::PRESS)
            {
                sceneCamera->Translate(cameraCom->cameraFront * cameraSpeed);
                cameraMoved = true;
            }
            if (Input::GetKeyState(KEYBOARD_KEY::S) == KEY_STATE::PRESS)
            {
                sceneCamera->Translate(-cameraCom->cameraFront * cameraSpeed);
                cameraMoved = true;
            }
            if (Input::GetKeyState(KEYBOARD_KEY::D) == KEY_STATE::PRESS)
            {
                sceneCamera->Translate(glm::normalize(glm::cross(cameraCom->cameraFront, cameraCom->cameraUp)) * cameraSpeed);
                cameraMoved = true;
            }
            if (Input::GetKeyState(KEYBOARD_KEY::A) == KEY_STATE::PRESS)
            {
                sceneCamera->Translate(-glm::normalize(glm::cross(cameraCom->cameraFront, cameraCom->cameraUp)) * cameraSpeed);
                cameraMoved = true;
            }
        }
    }
}