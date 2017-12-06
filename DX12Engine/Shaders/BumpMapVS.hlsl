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
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
};

struct Output
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BINORMAL;
};

Output main(Input input)
{
    Output output;

    input.position.w = 1.0f;
    output.position = mul(mul(viewProjectionMatrix, worldMatrix), input.position);

    output.tex = input.tex;

    output.normal = mul((float3x3) worldMatrix, input.normal);
    output.normal = normalize(output.normal);

    output.tangent = mul((float3x3) worldMatrix, input.tangent);
    output.tangent = normalize(output.tangent);

    output.bitangent = mul((float3x3) worldMatrix, input.bitangent);
    output.bitangent = normalize(output.bitangent);

    return output;
}