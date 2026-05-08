struct VertexInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR;
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

    output.color = input.color;

    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
    //return float4(1,0,0,1);
}