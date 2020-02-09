#pragma once
#include "common.h"
#include "component.h"

class RenderTarget;
struct Ray;

enum CAMERA_RENDER_FLAG
{
	RENDER_NOTHING = 0,
	RENDER_ID_BUFFER = 1 << 0,
	RENDER_GIZMOS = 1 << 1,  
	RENDER_WIRE_FRAMES = 1 <<2
};

class Camera: public Component
{
	MO_OBJECT("Camera")
public:
	Camera();

	virtual void Clear();
	void Render();
	glm::mat4 GetProjection();
	glm::mat4 GetViewMatrix();

	Ray ScreenRay(float screenX, float screenY);

	float fov = 45.0f;
	float ratio = 1024.0f / 768.0f;
	float nearPlane = 0.3f;
	float farPlane = 3000.0f;

	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	unsigned int renderFlag = CAMERA_RENDER_FLAG::RENDER_NOTHING;

    glm::vec4 clearColor = glm::vec4(49.0f/255.0f, 77.0f/255.0f, 121.0f/255.0f, 1.0f);
	RenderTarget* renderTarget = nullptr;
};