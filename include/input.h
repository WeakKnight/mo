#pragma once

#include <GLFW/glfw3.h>
#include "common.h"

enum class KEYBOARD_KEY
{
	ESC = GLFW_KEY_ESCAPE,
	F6 = GLFW_KEY_F6,
	W = GLFW_KEY_W,
	A = GLFW_KEY_A,
	S = GLFW_KEY_S,
	D = GLFW_KEY_D,
	Q = GLFW_KEY_Q,
	E = GLFW_KEY_E,
	R = GLFW_KEY_R,
};

enum class MOUSE_KEY
{
	RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
	LEFT = GLFW_MOUSE_BUTTON_LEFT,
	MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE
};

enum class KEY_STATE
{
	PRESS = GLFW_PRESS,
	RELEASE = GLFW_RELEASE,
};

namespace Input
{
	void Init(GLFWwindow* window);
	void Update();
	KEY_STATE GetKeyState(KEYBOARD_KEY key);
	KEY_STATE GetMouseButtonState(MOUSE_KEY key);
	glm::vec2 GetMouseOffset();
	bool CheckKey(KEYBOARD_KEY key);
}
