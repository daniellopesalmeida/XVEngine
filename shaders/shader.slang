struct VertexInput
{
    float3 position : POSITION;  // location 0
    float3 color    : COLOR;     // location 1
    float3 normal   : NORMAL;    // location 2
    float2 texCoord : TEXCOORD;  // location 3
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

// ========================================
// FRAME DATA (descriptor set 0 binding 0)
// ========================================

struct FrameData
{
    float4x4 view;
    float4x4 proj;
};

[[vk::binding(0, 0)]]
ConstantBuffer<FrameData> frameData;


// ========================================
// OBJECT DATA (push constants)
// ========================================

struct ObjectData
{
    float4x4 model;
};

[[vk::push_constant]]
ConstantBuffer<ObjectData> objectData;


// ========================================
// VERTEX SHADER
// ========================================

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;

    float4 localPos = float4(input.position, 1.0);

    float4 worldPos = mul(objectData.model, localPos);
    float4 viewPos  = mul(frameData.view, worldPos);

    output.position = mul(frameData.proj, viewPos);

    output.color    = input.color;
    output.normal   = input.normal;
    output.texCoord = input.texCoord;

    return output;
}


// ========================================
// FRAGMENT SHADER
// ========================================

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}