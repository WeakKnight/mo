#version 420 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

layout(location = 0) in vec3 inPosition;

// CameraSapce
out vec3 positionCameraSpace;

void main() 
{
    mat4 viewModel = view * model; 
    mat3 modelRS = mat3(transpose(inverse(model))) ;
    
    // camera space
    positionCameraSpace = vec3(viewModel * vec4(inPosition, 1.0));

    // screen space
    gl_Position = projection * viewModel * vec4(inPosition, 1.0);
}