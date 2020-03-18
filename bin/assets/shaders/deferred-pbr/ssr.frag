#version 420 core

out vec4 outColor;
noperspective in vec2 fragUV;

// camera space
uniform sampler2D gBufferPosition;
// camera space
uniform sampler2D gBufferNormalMetalness;
uniform sampler2D gBufferAlbedoRoughness;
// SSR Combine Pass
uniform sampler2D ssrCombine;
uniform samplerCube iradianceMap;

// uniform mat4 view;
uniform mat4 invView;
uniform mat4 perspectiveProjection;
// uniform mat4 invProjection;

// camera space Z
uniform sampler2D backZPass;

const float PI = 3.14159265;
#define Scale vec3(.8, .8, .8)
#define K 19.19

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}  

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}  

vec2 ViewSpaceToScreenSpace(vec3 viewPos)
{
    vec4 clipSpacePos = perspectiveProjection * vec4(viewPos, 1.0);
    vec3 ndcSpacePos = clipSpacePos.xyz / clipSpacePos.w;
    
    if(min(ndcSpacePos.x, min(ndcSpacePos.y, ndcSpacePos.z)) < -1.0 || max(ndcSpacePos.x, max(ndcSpacePos.y, ndcSpacePos.z)) > 1.0)
    {
        return vec2(-1.0);
    }

    vec2 screenSpacePos = (ndcSpacePos.xy + vec2(1.0)) / 2.0;
    return screenSpacePos.xy;
}

// Screem Pos: [0 to 1], [0 to 1]
float ComputeZDepth(vec2 screenPos)
{
    return texture(gBufferPosition, screenPos).z;
}

float distanceSquared(vec2 a, vec2 b) 
{ 
    a -= b; 
    return dot(a, a); 
}

bool RayCast(vec3 origin, vec3 dir, out vec3 hitPos, float stepDistance, int stepNum, int binarySteps)
{
    // initial hitpos
    hitPos = origin;
    vec3 deltaVector = stepDistance * dir;
   
    for(int i = 0; i < stepNum; i++)
    {
        hitPos = hitPos + deltaVector;
        vec2 hitPosScreenSpace = ViewSpaceToScreenSpace(hitPos);
        
        if(min(hitPosScreenSpace.x, hitPosScreenSpace.y) < 0.0)
        {
            continue;
        }

        float zDepth = ComputeZDepth(hitPosScreenSpace);

        if(zDepth <= -1000)
        {
            continue;
        }

        float deltaDepth = hitPos.z - zDepth;
        
        // zDepth In Front Of Current Hit Pos
        if((deltaVector.z - deltaDepth) < 1.2)
        {
            if(deltaDepth < 0.0)
            {
                // Do Binary Roll Back To More Accurate Result
                vec3 startPoint = hitPos - deltaVector;
                vec3 endPoint = hitPos;
                vec3 midPoint = vec3(0.0);

                for(int j = 0; j < binarySteps; j++)
                {
                    // mid point
                    midPoint = (startPoint + endPoint) * 0.5;
                    float currentZDepth = ComputeZDepth(ViewSpaceToScreenSpace(midPoint));
                    float currentDeltaDepth = midPoint.z - currentZDepth;
                    
                    // Go Left, Change End Point
                    if(currentDeltaDepth < 0.0)
                    {
                        endPoint = (midPoint + endPoint) * 0.5;
                    }
                    // Go Right, Change Start Point
                    else
                    {
                        startPoint = (startPoint + midPoint) * 0.5;
                    }
                }

                hitPos = midPoint;

                return true;
            }
        }
    }

    return false;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}  

vec3 hash(vec3 a)
{
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}

// This Pass Should Output Non-Analytical Specular Term Based On Last Frame Before Post Processing
void main() 
{
    vec4 albedoRoughness = texture(gBufferAlbedoRoughness, fragUV);
    vec4 normalMetalness = texture(gBufferNormalMetalness, fragUV);

    vec3 albedo = albedoRoughness.rgb;
    float roughness = albedoRoughness.a;
    float metalness = normalMetalness.w; 

    // View Space
    vec3 position = texture(gBufferPosition, fragUV).xyz;
    vec3 worldPos = vec3(invView * vec4(position, 1.0));

    vec3 N = normalMetalness.xyz;
    vec3 V = normalize(-position);
    float backZ = texture(backZPass, fragUV).r;
    float thickness = position.z - backZ;

    float colorFactor = length(N);
    vec3 R = normalize(reflect(-V, N));
    vec3 worldR = normalize(mat3(invView) * R);
    
    vec3 result = vec3(0.0, 0.0, 0.0);
    // reflection raytracing
    for(int i = 0; i < 8; i++)
    {
        // Do Ray Cast
        vec3 origin = position;
        // Perfect Reflection
        vec3 hitPos;
        
        vec3 H = normalize(V + R);
        
        float NoV = clamp(dot(N, V), 0.0, 1.0);
        float NoH = clamp(dot(N, H), 0.0, 1.0);
        float VoH = clamp(dot(N, V), 0.0, 1.0);

        vec3 SampleColor = vec3(0.0);

        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metalness);
        vec3 Fresnel = fresnelSchlick(NoV, F0);

        vec3 jitt = mix(vec3(0.0), vec3(hash(worldPos + vec3(i))), roughness);
        vec3 L = normalize(R + jitt);
        vec3 worldL = normalize(mat3(invView) * L);

        float NoL = max(dot(N, L), 0.0);

        if(NoL > 0)
        {
            if (RayCast(origin, L, hitPos, 0.4, 48, 8))
            {
                vec2 screenSpaceHitPos = ViewSpaceToScreenSpace(hitPos);
                if(screenSpaceHitPos.x > 0.0 && screenSpaceHitPos.x < 1.0 && screenSpaceHitPos.y > 0.0 && screenSpaceHitPos.y < 1.0)
                {
                    // vec3 hitPosNormal = texture(gBufferNormalMetalness, screenSpaceHitPos).rgb;                        
                    SampleColor = texture(ssrCombine, screenSpaceHitPos).rgb;
                }
                else
                {
                    SampleColor = texture(iradianceMap, worldL).rgb;
                }
            }
            // Fallback To Radiance Map
            else
            {
                SampleColor = texture(iradianceMap, worldL).rgb;
            }

            result += ( SampleColor * Fresnel);
        }
    }

    // result = vec3(fragUV, 0.0);
    // result = vec3(thickness, 0, 0);
    // result = vec3(ViewSpaceToScreenSpace(position), 0.0);
    result = result / 8.0;
    outColor = vec4(colorFactor * result, 1.0);
}