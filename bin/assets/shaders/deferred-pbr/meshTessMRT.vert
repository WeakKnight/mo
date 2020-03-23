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

out vec2 uv;

// CameraSapce
out vec3 positionCameraSpace;
// CameraSapce
out vec3 normalCameraSpace;
// CameraSapce
out vec3 tangentCameraSpace;
// CameraSapce
out vec3 bitangentCameraSpace;

void main() 
{
    mat4 viewModel = view * model; 
    mat3 modelRS = mat3(transpose(inverse(model))) ;
    // world space
    vec3 position = vec3(model * vec4(inPosition, 1.0));
    vec3 normal = normalize(modelRS * inNormal); 
    uv = inUV;
    vec3 tangent = normalize(modelRS * inTangent);
    vec3 bitangent = normalize(modelRS * inBitangent);
    
    // camera space
    positionCameraSpace = vec3(viewModel * vec4(inPosition, 1.0));

    mat3 viewModelInv = mat3(transpose(inverse(viewModel)));
    normalCameraSpace = normalize(viewModelInv * inNormal);
    tangentCameraSpace = normalize(viewModelInv * inTangent);
    bitangentCameraSpace = normalize(viewModelInv * inBitangent);

    // screen space
    // gl_Position = projection * viewModel * vec4(inPosition, 1.0);
}