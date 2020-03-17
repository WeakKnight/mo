#version 420 core

out vec4 outColor;
in vec2 fragUV;

// camera space
uniform sampler2D gBufferPosition;
// camera space
uniform sampler2D gBufferNormalMetalness;
uniform sampler2D gBufferAlbedoRoughness;

uniform samplerCube radianceMap;

uniform mat4 invView;

struct Light
{
    vec3 Position;
    vec3 Color;
    int Type;
    float SpotAngle;
    float SpotEdgeAngle;
    vec3 SpotDir;
    int CastShadow;
    sampler2D ShadowMap;
    mat4 LightProjection;
    mat4 LightView;
};

uniform Light lights[16];

const float PI = 3.14159265;
const float shadowTexelSize = 1.0/1024.0;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec2 ViewSpaceToLightScreenSpace(vec3 worldPos, mat4 lightView, mat4 lightProjection)
{    
    // Light Space
    vec3 lightSpacePos = vec3(lightView * vec4(worldPos, 1.0));
    vec4 clipSpacePos = lightProjection * vec4(lightSpacePos, 1.0);
    vec3 ndcSpacePos = clipSpacePos.xyz / clipSpacePos.w;
    
    if(min(ndcSpacePos.x, min(ndcSpacePos.y, ndcSpacePos.z)) < -1.0 || max(ndcSpacePos.x, max(ndcSpacePos.y, ndcSpacePos.z)) > 1.0)
    {
        return vec2(-1.0);
    }

    vec2 screenSpacePos = (ndcSpacePos.xy + vec2(1.0)) / 2.0;
    return screenSpacePos.xy;
}

bool DetectShadow(vec3 worldPos, vec2 shadowMapUV, int i)
{
    if(shadowMapUV.x > 0.0 && shadowMapUV.x < 1.0 && shadowMapUV.y > 0.0 && shadowMapUV.y < 1.0)
    {
        float lightSpaceZ = texture(lights[i].ShadowMap, shadowMapUV).r;
        vec3 fragLightSpacePos = vec3(lights[i].LightView * vec4(worldPos, 1.0));
        // 
        if(lightSpaceZ > (fragLightSpacePos.z + 0.15))
        {
            return true;
        }
    }
    
    return false;
}

void main() 
{
    vec4 albedoRoughness = texture(gBufferAlbedoRoughness, fragUV);
    vec4 normalMetalness = texture(gBufferNormalMetalness, fragUV);

    vec3 albedo = albedoRoughness.rgb;
    float roughness = albedoRoughness.a;
    vec3 N = normalMetalness.xyz;
    float metalness = normalMetalness.w; 
    vec3 position = texture(gBufferPosition, fragUV).xyz;
    vec3 worldPos = vec3(invView * vec4(position, 1.0));
    vec3 V = normalize(-position);

    float colorFactor = length(N);

    vec3 lambert = vec3(0.0);
    vec3 specular = vec3(0.0);
    vec2 shadowMapUVRight = vec2(shadowTexelSize, 0.0);
    vec2 shadowMapUVUp = vec2(0.0, shadowTexelSize);

    for(int i = 0; i < 4; i++)
    {
        vec3 lightPos = lights[i].Position;
        vec3 lightColor = lights[i].Color;
        int lightType = lights[i].Type;

        if(lightColor.x + lightColor.y + lightColor.z > 0.0)
        {
            vec3 L = normalize(lightPos - position);
            vec3 R = reflect(-L, N);
            vec3 H = (V + R) * 0.5;

            float HDotN = clamp(dot(H, N), 0.0, 1.0);
            float NDotL = clamp(dot(N, L), 0.0, 1.0);    

            vec3 lightIntensity = NDotL * lightColor;

            if(lightType == 1)
            {
                // Camera Space
                vec3 spotDir = normalize(lights[i].SpotDir);
                vec3 lightDir = -L;
                float spotDotLight = dot(spotDir, lightDir);

                float cutOffOuter = cos(lights[i].SpotAngle + lights[i].SpotEdgeAngle);
                float epsilon   = cos(lights[i].SpotAngle) - cutOffOuter;
                float intensity = clamp(
                    (spotDotLight - cutOffOuter) / epsilon, 0.0, 1.0
                );    

                if(spotDotLight < lights[i].SpotAngle)
                {
                    lightIntensity = intensity * lightIntensity;
                }

                if(lights[i].CastShadow == 1)
                {
                    vec2 shadowMapUV = ViewSpaceToLightScreenSpace(worldPos, lights[i].LightView, lights[i].LightProjection);
                    
                    float lightFactor = 0.0;
                   
                    if(!DetectShadow(worldPos, shadowMapUV, i))
                    {
                        lightFactor += 1.00;
                    }

                    lightIntensity = lightFactor * lightIntensity;
                }
            }

            float lightDistance = distance(lightPos, position);
            float falloff = 1.0 / (lightDistance * lightDistance);

            lambert += (falloff * lightIntensity * albedo / PI);
        }
    }

    vec3 environmentLight = albedo * texture(radianceMap ,mat3(invView) * N).rgb;
    vec3 result = environmentLight + colorFactor * (lambert + specular);

    outColor = vec4(result, 1.0);
}