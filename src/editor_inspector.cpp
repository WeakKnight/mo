#include "editor_inspector.h"
#include "input.h"
#include "actor.h"
#include "editor_window_system.h"

EditorInspector::EditorInspector(unsigned int initialWidth, unsigned int initialHeight, bool initialOpen, std::string title) : EditorWindow(initialWidth, initialHeight, initialOpen, title)
{
}

void EditorInspector::OnIMGUI()
{
	auto selection = EditorWindowSystem::GetInstance()->GetActorSelection();
	if (selection.size() == 1)
	{
		auto actor = selection[0];
		if (actor != previousActor)
		{
			previousActor = actor;
			return;
		}

		glm::vec3 localPos = actor->GetPositionLocal();
		glm::vec3 rotation = actor->GetRotationEulerLocal();
		glm::vec3 scale = actor->GetScaleLocal();

		ImGui::InputFloat3("Position", glm::value_ptr(localPos), 3);
		ImGui::InputFloat3("Rotation", glm::value_ptr(rotation), 3);
		ImGui::InputFloat3("Scale", glm::value_ptr(scale), 3);

		actor->SetPositionLocal(localPos);
		actor->SetRotationEulerLocal(rotation);
		actor->SetScaleLocal(scale);
	}
}

void EditorInspector::OnResize()
{
}