#version 420 core

layout (std140, binding = 0) uniform CameraBlock
{
    mat4 projection;
    mat4 view;
};

uniform mat4 model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

out vec3 positionVert;
out vec3 normalVert;
out vec2 UVVert;

void main() 
{
    positionVert = (model * vec4(inPosition, 1.0)).xyz;
    normalVert = (model * vec4(inNormal, 0.0)).xyz;
    UVVert = inUV;
}