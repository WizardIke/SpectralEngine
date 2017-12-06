cbuffer VSPerFrameBuffer : register(b0)
{
    matrix viewProjectionMatrix;
    float3 cameraPosition;
}

cbuffer VSPerObjectBuffer : register(b1)
{
    matrix worldMatrix;
}

struct Input
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct Output
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

Output main(Input input)
{
    Output output;

    input.position.w = 1.0f;
    output.position = mul(mul(viewProjectionMatrix, worldMatrix), input.position);
    output.tex = input.tex;

    return output;
}