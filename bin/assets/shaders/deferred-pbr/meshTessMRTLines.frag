#version 420 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormalMetalness;
layout (location = 2) out vec4 gAlbedoRoughness;

in vec2 uvFS;

// CameraSapce
in vec3 positionCameraSpaceFS;
// CameraSapce
in vec3 normalCameraSpaceFS;
// CameraSapce
in vec3 tangentCameraSpaceFS;
// CameraSapce
in vec3 bitangentCameraSpaceFS;

uniform sampler2D normalMap;
uniform float tillingX;
uniform float tillingY;

void main()
{
    float actualULength = 1.0 / tillingX;
    float actualVLength = 1.0 / tillingY;

    float xIndex = floor(uvFS.x / actualULength);
    float yIndex = floor(uvFS.y / actualVLength);

    vec2 actualUV = vec2((uvFS.x - xIndex * actualULength)/actualULength, (uvFS.y - yIndex * actualVLength)/actualVLength);

    float metalness = 0.0;
    float roughness = 1.0;

    // Attachment 0 CameraSpace
    gPosition = positionCameraSpaceFS;
    
    // Attachment 1
    vec3 mappedNormal = vec3(texture(normalMap, actualUV));
    mappedNormal = normalize(mappedNormal * 2.0 - 1.0);

    vec3 N = normalize(normalCameraSpaceFS * mappedNormal.z + tangentCameraSpaceFS * mappedNormal.x + bitangentCameraSpaceFS * mappedNormal.y);
    gNormalMetalness = vec4(N, metalness);

    // Attachment 2
    vec3 diffuseColor = vec3(0.0, 1.0, 0.0);
    gAlbedoRoughness = vec4(diffuseColor, roughness);
}