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
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

out vec2 fragUV;

void main() 
{
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
    fragUV = inUV;
}