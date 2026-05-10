// ── Shared structures ─────────────────────────────────────────────────────────

static const uint MAX_LIGHTS = 16;

struct GpuLight
{
    float4 colorAndIntensity;  // xyz=color,     w=intensity
    float4 positionAndRange;   // xyz=position,  w=range
    float4 directionAndType;   // xyz=direction, w=type (0=Dir, 1=Point, 2=Spot)
    float4 coneAngles;         // x=cos(inner),  y=cos(outer), zw=unused
};

struct FrameData
{
    float4x4 view;
    float4x4 proj;
    float4   cameraPos;               // xyz=world-space camera pos, w=unused
    float4   ambientColorAndStrength; // xyz=color, w=strength
    uint     lightCount;
    uint     _pad0;
    uint     _pad1;
    uint     _pad2;
    GpuLight lights[MAX_LIGHTS];
};

// Push constants — one per draw call
struct PushConstants
{
    float4x4 model;
    // Normal matrix columns packed as vec4s (w unused).
    // Reconstructed as float3x3 in the vertex shader.
    float4 normalRow0;
    float4 normalRow1;
    float4 normalRow2;
};

// set 0, binding 0 — per-frame data
[[vk::binding(0, 0)]] ConstantBuffer<FrameData> frameData;

// set 1 — per-material textures
// Sampler2D = combined image sampler — matches eCombinedImageSampler in Material.cpp
[[vk::binding(0, 1)]] Sampler2D diffuseTex;
[[vk::binding(1, 1)]] Sampler2D specularTex;
[[vk::binding(2, 1)]] Sampler2D glossTex;
[[vk::binding(3, 1)]] Sampler2D normalTex;

[[vk::push_constant]] PushConstants pc;

// ── Vertex stage ──────────────────────────────────────────────────────────────

struct VSInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 tangent  : TANGENT;  // xyz=tangent dir, w=handedness
};

struct VSOutput
{
    float4 sv_position : SV_Position;
    float3 worldPos    : WORLD_POS;
    float3 normal      : NORMAL;
    float2 texCoord    : TEXCOORD;
    float3 tangent     : TANGENT;
    float3 bitangent   : BITANGENT;
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;

    float4 worldPos4  = mul(pc.model, float4(input.position, 1.0));
    output.worldPos   = worldPos4.xyz;
    output.sv_position = mul(frameData.proj, mul(frameData.view, worldPos4));
    output.texCoord   = input.texCoord;

    // Reconstruct the 3x3 normal matrix from the packed rows
    float3x3 normalMatrix = float3x3(
        pc.normalRow0.xyz,
        pc.normalRow1.xyz,
        pc.normalRow2.xyz
    );

    float3 N = normalize(mul(normalMatrix, input.normal));
    float3 T = normalize(mul(normalMatrix, input.tangent.xyz));
    // Re-orthogonalise T against N (Gram-Schmidt), then compute B with handedness
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * input.tangent.w;

    output.normal    = N;
    output.tangent   = T;
    output.bitangent = B;

    return output;
}

// ── Fragment stage ────────────────────────────────────────────────────────────

// Inverse-square attenuation clamped to [0,1] with a smooth falloff at 'range'.
float Attenuation(float dist, float range)
{
    if (range <= 0.0) return 1.0;
    float x = saturate(1.0 - (dist / range));
    return x * x;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    // ── Sample textures ───────────────────────────────────────────────────────
    float4 diffuseSample  = diffuseTex .Sample(input.texCoord);
    float3 specularSample = specularTex.Sample(input.texCoord).rgb;
    float  glossSample    = glossTex   .Sample(input.texCoord).r;
    float3 normalSample   = normalTex  .Sample(input.texCoord).rgb;

    // ── Build TBN normal ──────────────────────────────────────────────────────
    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    float3x3 TBN = float3x3(T, B, N);

    // Decode [0,1] normal map to [-1,1] tangent-space, then to world-space
    float3 tangentNormal = normalSample * 2.0 - 1.0;
    N = normalize(mul(tangentNormal, TBN));

    // ── Material parameters ───────────────────────────────────────────────────
    float3 albedo    = diffuseSample.rgb;
    float  shininess = max(glossSample * 256.0, 1.0);  // gloss → shininess exponent

    float3 V = normalize(frameData.cameraPos.xyz - input.worldPos);

    // ── Ambient ───────────────────────────────────────────────────────────────
    float3 ambColor    = frameData.ambientColorAndStrength.xyz;
    float  ambStrength = frameData.ambientColorAndStrength.w;
    float3 ambient     = ambColor * ambStrength * albedo;

    // ── Per-light accumulation ────────────────────────────────────────────────
    float3 diffuseAcc  = float3(0.0, 0.0, 0.0);
    float3 specularAcc = float3(0.0, 0.0, 0.0);

    uint count = min(frameData.lightCount, MAX_LIGHTS);
    for (uint i = 0; i < count; i++)
    {
        GpuLight light = frameData.lights[i];

        float3 lightColor = light.colorAndIntensity.xyz;
        float  intensity  = light.colorAndIntensity.w;
        int    lightType  = int(light.directionAndType.w);

        float3 L;          // direction from surface toward light
        float  atten = 1.0;

        if (lightType == 0)
        {
            // Directional — direction stored as "toward light" convention
            L = normalize(light.directionAndType.xyz);
        }
        else if (lightType == 1)
        {
            // Point
            float3 toLight = light.positionAndRange.xyz - input.worldPos;
            float  dist    = length(toLight);
            L     = toLight / max(dist, 0.0001);
            atten = Attenuation(dist, light.positionAndRange.w);
        }
        else
        {
            // Spot
            float3 toLight  = light.positionAndRange.xyz - input.worldPos;
            float  dist     = length(toLight);
            L = toLight / max(dist, 0.0001);

            // Cone attenuation: compare cos of angle to stored thresholds
            float3 spotDir  = normalize(light.directionAndType.xyz);
            float  cosAngle = dot(-L, spotDir);  // -L because spotDir points away from light
            float  cosInner = light.coneAngles.x;
            float  cosOuter = light.coneAngles.y;
            float  spotAtten = saturate((cosAngle - cosOuter) / max(cosInner - cosOuter, 0.0001));

            atten = Attenuation(dist, light.positionAndRange.w) * spotAtten;
        }

        float  NdotL     = max(dot(N, L), 0.0);
        float3 H         = normalize(L + V);
        float  NdotH     = max(dot(N, H), 0.0);
        float  spec      = pow(NdotH, shininess);

        float3 radiance  = lightColor * intensity * atten;
        diffuseAcc  += albedo          * NdotL * radiance;
        specularAcc += specularSample  * spec  * radiance;
    }

    float3 finalColor = ambient + diffuseAcc + specularAcc;

    // Alpha from diffuse texture (needed for transparency later)
    return float4(finalColor, diffuseSample.a);
}
