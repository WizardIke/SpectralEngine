cbuffer VSPerFrameBuffer : register(b0)
{
    matrix viewProjectionMatrix;
    float3 cameraPosition;
}

cbuffer VSPerObjectBuffer : register(b1)
{
    matrix worldMatrix;
    float fogStart;
    float fogEnd;
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
    float fogFactor : FOG;
};

Output main(Input input)
{
    Output output;

    input.position.w = 1.0f;
    float4 worldPosition = mul(worldMatrix, input.position);
    output.position = mul(viewProjectionMatrix, worldPosition);

    output.tex = input.tex;

    float3 displacementFromCamera = worldPosition.xyz - cameraPosition;

	// Calculate linear fog.    
    output.fogFactor = saturate((fogEnd - displacementFromCamera.z) / (fogEnd - fogStart));

    return output;
}