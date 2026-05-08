struct VertexInput
{
    float3 position : POSITION;  //location 0
    float3 color    : COLOR;     //location 1
    float3 normal   : NORMAL;    //location 2
    float2 texCoord : TEXCOORD;  //location 3
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct ObjectData
{
    float4x4 mvp;
};

[[vk::push_constant]]
ConstantBuffer<ObjectData> objectData;

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(objectData.mvp, float4(input.position, 1.0));
    output.color    = input.color;
    output.normal   = input.normal;
    output.texCoord = input.texCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}