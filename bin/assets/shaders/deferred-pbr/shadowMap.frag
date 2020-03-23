#version 420 core

// Light Space
layout (location = 0) out float gPosition;

// CameraSapce
in vec3 positionCameraSpace;

void main()
{
    gPosition = positionCameraSpace.z;
}