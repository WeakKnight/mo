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

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metalnessMap;
uniform sampler2D roughnessMap;

uniform float tillingX;
uniform float tillingY;

void main()
{
    float actualULength = 1.0 / tillingX;
    float actualVLength = 1.0 / tillingY;

    float xIndex = floor(uvFS.x / actualULength);
    float yIndex = floor(uvFS.y / actualVLength);

    vec2 actualUV = vec2((uvFS.x - xIndex * actualULength)/actualULength, (uvFS.y - yIndex * actualVLength)/actualVLength);

    float metalness = texture(metalnessMap, actualUV).r;
    float roughness = texture(roughnessMap, actualUV).r;

    // Attachment 0 CameraSpace
    gPosition = positionCameraSpaceFS;
    
    // Attachment 1
    vec3 mappedNormal = vec3(texture(normalMap, actualUV));
    mappedNormal = normalize(mappedNormal * 2.0 - 1.0);

    vec3 N = normalize(normalCameraSpaceFS * mappedNormal.z + tangentCameraSpaceFS * mappedNormal.x + bitangentCameraSpaceFS * mappedNormal.y);
    gNormalMetalness = vec4(N, metalness);

    // Attachment 2
    vec3 diffuseColor = texture(albedoMap, actualUV).rgb;
    gAlbedoRoughness = vec4(diffuseColor, roughness);
}