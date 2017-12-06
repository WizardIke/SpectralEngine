cbuffer VSPerFrameBuffer : register(b0)
{
    matrix viewProjectionMatrix;
    float3 cameraPosition;
}

cbuffer VSPerObjectBuffer : register(b1)
{
    matrix worldMatrix;
}

struct VertexInputType
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct Output
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

Output main(VertexInputType input)
{
    Output output;

    input.position.w = 1.0f;
    output.position = mul(mul(viewProjectionMatrix, worldMatrix), input.position);

    output.color = input.color;

    return output;
}