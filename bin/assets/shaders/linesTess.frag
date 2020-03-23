#version 420 core

out vec4 outColor;

in vec3 positionFS;
in vec3 normalFS;
in vec2 UVFS;

void main() 
{
    outColor = vec4(0.0, 1.0, 0.0, 1.0);
}