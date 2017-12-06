cbuffer PerFrameBuffer
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

cbuffer FogBuffer
{
	float fogStart;
	float fogEnd;
};

struct VertexInputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float fogFactor : FOG;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	float4 displacementFromCamera;


	// Change the position vector to be 4 units for proper matrix calculations.
	input.position.w = 1.0f;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(input.position, worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	// Calculate the camera position.
	displacementFromCamera = mul(input.position, worldMatrix);
	displacementFromCamera = mul(displacementFromCamera, viewMatrix);

	// Calculate linear fog.    
	output.fogFactor = saturate((fogEnd - displacementFromCamera.z) / (fogEnd - fogStart));

	return output;
}