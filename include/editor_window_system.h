#pragma once
#include "common.h"
#include "editor_window.h"
#include "editor_scene_view.h"

class EditorWindowSystem
{
public:
    static void Init()
    {
        instance = new EditorWindowSystem();
        instance->InitWindows();
    }

    static EditorWindowSystem *GetInstance()
    {
        return instance;
    }

    void Update()
    {
        for (size_t i = 0; i < editorWindows.size(); i++)
        {
            std::shared_ptr<EditorWindow> editor = editorWindows[i];
            editor->OnFrame();
        }
    }

    template <typename T>
    std::shared_ptr<T> GetEditor()
    {
        size_t target_hash_code = typeid(T).hash_code();
        for (size_t i = 0; i < editorWindows.size(); i++)
        {
            std::shared_ptr<EditorWindow> editor = editorWindows[i];
            spdlog::info("Editor Name Is {}", typeid(*editor).name());
            if (target_hash_code == typeid(*editor).hash_code())
            {
                return std::dynamic_pointer_cast<T>(editor);
            }
        }

        return nullptr;
    }

    std::vector<std::shared_ptr<EditorWindow>> editorWindows;

private:
    EditorWindowSystem()
    {
    }

    void InitWindows()
    {
        editorWindows.push_back(std::make_shared<EditorWindow>(800, 600, true, "Base Editor Window"));
        editorWindows.push_back(std::make_shared<EditorWindow>(200, 200, true, "Other Base Editor Window"));
        editorWindows.push_back(std::make_shared<EditorSceneView>(200, 200, true, "Scene View"));
    }

    static EditorWindowSystem *instance;
};

EditorWindowSystem *EditorWindowSystem::instance = nullptr;