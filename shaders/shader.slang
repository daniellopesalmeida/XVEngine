// shader.slang — Blinn-Phong directional light
// Compiled to shader.vert.spv (vertMain) and shader.frag.spv (fragMain)

// ── Uniform buffer — set 0, binding 0 ────────────────────────────────────────
struct FrameData
{
    float4x4 view;
    float4x4 proj;

    float4 lightDir;    // xyz = world-space direction toward light, w = unused
    float4 lightColor;  // xyz = RGB, w = unused
    float4 cameraPos;   // xyz = world-space camera position, w = unused

    // x = ambientStrength, y = specularStrength, z = shininess, w = unused
    float4 lightParams;
};

[[vk::binding(0, 0)]]
ConstantBuffer<FrameData> frameData;

// ── Push constant — per draw call ─────────────────────────────────────────────
struct ObjectData
{
    float4x4 model;

    // Normal matrix (inverse-transpose of model upper-left 3x3)
    // Packed as 3x float4 — we only use .xyz of each
    float4 normalRow0;
    float4 normalRow1;
    float4 normalRow2;
};

[[vk::push_constant]]
ConstantBuffer<ObjectData> objectData;

// ── Vertex input ──────────────────────────────────────────────────────────────
struct VertexInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 color    : COLOR;
    [[vk::location(2)]] float3 normal   : NORMAL;
    [[vk::location(3)]] float2 texCoord : TEXCOORD;
};

// ── Vertex -  Fragment ─────────────────────────────────────────────────────────
struct VertexOutput
{
    float4 position  : SV_Position;
    float3 color     : COLOR;
    float3 worldPos  : TEXCOORD0;   // world-space position for specular
    float3 worldNorm : TEXCOORD1;   // world-space normal
};

// ── Vertex shader ─────────────────────────────────────────────────────────────
[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    // Reconstruct the normal matrix from packed rows
    float3x3 normalMatrix = float3x3(
        objectData.normalRow0.xyz,
        objectData.normalRow1.xyz,
        objectData.normalRow2.xyz
    );

    float4 worldPos4 = mul(objectData.model, float4(input.position, 1.0));

    VertexOutput output;
    output.position  = mul(frameData.proj, mul(frameData.view, worldPos4));
    output.color     = input.color;
    output.worldPos  = worldPos4.xyz;
    //transform normal to world space — normalMatrix handles non-uniform scale
    output.worldNorm = normalize(mul(normalMatrix, input.normal));

    return output;
}

// ── Fragment shader — Blinn-Phong ─────────────────────────────────────────────
[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    float3 N = normalize(input.worldNorm);
    float3 L = normalize(frameData.lightDir.xyz);   // toward light
    float3 V = normalize(frameData.cameraPos.xyz - input.worldPos);
    float3 H = normalize(L + V);                    // half-vector

    float ambientStr  = frameData.lightParams.x;
    float specularStr = frameData.lightParams.y;
    float shininess   = frameData.lightParams.z;

    float3 lightCol = frameData.lightColor.xyz;
    float3 baseCol  = input.color;

    // Ambient
    float3 ambient = ambientStr * lightCol * baseCol;

    // Diffuse — Lambert
    float diff = max(dot(N, L), 0.0);
    float3 diffuse = diff * lightCol * baseCol;

    // Specular — Blinn-Phong
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float3 specular = specularStr * spec * lightCol;  // specular highlight uses light color only

    float3 result = ambient + diffuse + specular;
    return float4(result, 1.0);
}
