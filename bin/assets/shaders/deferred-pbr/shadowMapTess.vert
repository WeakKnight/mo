#version 420 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// worldSpace
out vec2 uv;
// CameraSapce
out vec3 positionCameraSpace;
// CameraSapce
out vec3 normalCameraSpace;

void main() 
{
    mat4 viewModel = view * model; 
    mat3 modelRS = mat3(transpose(inverse(model))) ;

    uv = inUV; 
    // camera space
    positionCameraSpace = vec3(viewModel * vec4(inPosition, 1.0));

    normalCameraSpace = normalize(mat3(viewModel) * inNormal);
}