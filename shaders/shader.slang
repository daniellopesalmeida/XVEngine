// shader.slang
// Blinn-Phong with diffuse, specular, gloss, and normal maps
// set 0 binding 0  — FrameData UBO  (per-frame)
// set 1 binding 0  — diffuse   sampler2D
// set 1 binding 1  — specular  sampler2D
// set 1 binding 2  — gloss     sampler2D
// set 1 binding 3  — normal    sampler2D
// push constant    — ObjectData (model + normal matrix columns, 112 bytes)

// ── Uniform data ─────────────────────────────────────────────────────────────

struct FrameData
{
    float4x4 view;
    float4x4 proj;

    float4 lightDir;      // xyz = direction toward light (world space, normalised)
    float4 lightColor;    // xyz = RGB
    float4 cameraPos;     // xyz = world position

    // x = ambientStrength, y = specularStrength, z = shininess, w = unused
    float4 lightParams;
};

struct ObjectData
{
    float4x4 model;

    // Normal matrix (inverse-transpose of model's mat3) stored as 3 columns.
    // Each vec4 is one column of the mat3; shader reads .xyz.
    float4 normalCol0;
    float4 normalCol1;
    float4 normalCol2;
};

[[vk::binding(0, 0)]] ConstantBuffer<FrameData> frameData;
[[vk::push_constant]] ConstantBuffer<ObjectData> objectData;

// Material textures — set 1
[[vk::binding(0, 1)]] Sampler2D texDiffuse;
[[vk::binding(1, 1)]] Sampler2D texSpecular;
[[vk::binding(2, 1)]] Sampler2D texGloss;
[[vk::binding(3, 1)]] Sampler2D texNormal;

// ── Vertex stage ──────────────────────────────────────────────────────────────

struct VSInput
{
    float3 position : POSITION;    // location 0
    float3 color    : COLOR;       // location 1
    float3 normal   : NORMAL;      // location 2
    float2 texCoord : TEXCOORD0;   // location 3
    float3 tangent  : TANGENT;     // location 4
};

struct VSOutput
{
    float4 sv_position : SV_Position;
    float3 worldPos    : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float3 worldTangent: TEXCOORD2;
    float2 uv          : TEXCOORD3;
    float3 color       : TEXCOORD4;
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;

    // Reconstruct normal matrix from pushed columns.
    // float3x3(col0, col1, col2) — Slang/HLSL float3x3 constructor takes rows,
    // so we transpose after construction to get the correct column-major matrix.
    float3x3 normalMatrix = transpose(float3x3(
        objectData.normalCol0.xyz,
        objectData.normalCol1.xyz,
        objectData.normalCol2.xyz
    ));

    float4 worldPos = mul(objectData.model, float4(input.position, 1.0));

    output.sv_position  = mul(frameData.proj, mul(frameData.view, worldPos));
    output.worldPos     = worldPos.xyz;
    output.worldNormal  = normalize(mul(normalMatrix, input.normal));
    output.worldTangent = normalize(mul(normalMatrix, input.tangent));
    output.uv           = input.texCoord;
    output.color        = input.color;

    return output;
}

// ── Fragment stage ────────────────────────────────────────────────────────────

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    // ── Normal mapping ───────────────────────────────────────────────────────
    float3 N = normalize(input.worldNormal);
    float3 T = normalize(input.worldTangent);
    T = normalize(T - dot(T, N) * N);   // re-orthogonalise (Gram-Schmidt)
    float3 B = cross(N, T);
    float3x3 TBN = transpose(float3x3(T, B, N));   // tangent-space → world

    // Sample normal map, remap [0,1] → [-1,1], transform to world space
    float3 sampledNormal = texNormal.Sample(input.uv).xyz * 2.0 - 1.0;
    float3 worldN = normalize(mul(TBN, sampledNormal));

    // ── Sample material textures ─────────────────────────────────────────────
    float3 diffuseColor = texDiffuse.Sample(input.uv).rgb * input.color;
    float  specularMask = texSpecular.Sample(input.uv).r;
    float  glossValue   = texGloss.Sample(input.uv).r;

    // Remap gloss (0–1) to shininess exponent (4–256)
    float shininess = lerp(4.0, 256.0, glossValue);

    // ── Blinn-Phong ──────────────────────────────────────────────────────────
    float3 L = normalize(frameData.lightDir.xyz);
    float3 V = normalize(frameData.cameraPos.xyz - input.worldPos);
    float3 H = normalize(L + V);

    float  ambientStr  = frameData.lightParams.x;
    float  specularStr = frameData.lightParams.y;
    float3 lightRGB    = frameData.lightColor.xyz;

    float3 ambient = ambientStr * diffuseColor * lightRGB;

    float NdotL = max(dot(worldN, L), 0.0);
    float3 diffuse = NdotL * diffuseColor * lightRGB;

    float NdotH = max(dot(worldN, H), 0.0);
    float3 specular = specularStr * specularMask * pow(NdotH, shininess) * lightRGB;

    return float4(ambient + diffuse + specular, 1.0);
}
