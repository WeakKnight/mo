#version 420 core

// Light Space
layout (location = 0) out float gPosition;

in vec2 uvFS;
// CameraSapce
in vec3 positionCameraSpaceFS;
// CameraSapce
in vec3 normalCameraSpaceFS;

void main()
{
    gPosition = positionCameraSpaceFS.z;
}