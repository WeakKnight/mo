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
 
// This Pass Should Output Non-Analytical Specular Term Based On Last Frame Before Post Processing
void main() 
{
    vec4 albedoRoughness = texture(gBufferAlbedoRoughness, fragUV);
    vec4 normalMetalness = texture(gBufferNormalMetalness, fragUV);

    float roughness = albedoRoughness.a;
    float metalness = normalMetalness.w; 

    // View Space
    vec3 position = texture(gBufferPosition, fragUV).xyz;
    vec3 N = normalMetalness.xyz;
    vec3 V = normalize(-position);
    float backZ = texture(backZPass, fragUV).r;
    float thickness = position.z - backZ;

    float colorFactor = length(N);
    // perfect reflection vector 
    vec3 R = normalize(reflect(-V, N));
    vec3 worldR = mat3(invView) * R;

    vec3 result = vec3(0.0, 0.0, 0.0);
    // reflection raytracing
    {
        // Do Ray Cast
        vec3 origin = position;
        // Perfect Reflection
        vec3 dir = R;
        vec3 hitPos;

        if (RayCast(origin, dir, hitPos, 0.4, 48, 8))
        {
            vec2 screenSpaceHitPos = ViewSpaceToScreenSpace(hitPos);
            if(screenSpaceHitPos.x > 0.0 && screenSpaceHitPos.x < 1.0 && screenSpaceHitPos.y > 0.0 && screenSpaceHitPos.y < 1.0)
            {
                result += 0.2 * texture(ssrCombine, screenSpaceHitPos).rgb;
            }
            else
            {
                result += 1.0 * texture(iradianceMap, worldR).rgb;
            }
        }
        // Fallback To Radiance Map
        else
        {
            result += 1.0 * texture(iradianceMap, worldR).rgb;
        }
    }
    // result = vec3(fragUV, 0.0);
    // result = vec3(thickness, 0, 0);
    // result = vec3(ViewSpaceToScreenSpace(position), 0.0);

    outColor = vec4(colorFactor * result, 1.0);
}