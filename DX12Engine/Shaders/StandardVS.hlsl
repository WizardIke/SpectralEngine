//#define USE_PER_VERTEX_BASE_COLOR
//#define USE_TEXTURE
//#define USE_NORMAL
//#define USE_TANGENT_FRAME
//#define USE_WORLD_POSITION

cbuffer Camera : register(b0)
{
	matrix viewProjectionMatrix;
	float3 cameraPosition;
};

cbuffer Material : register(b1)
{
	matrix worldMatrix;
};

struct Input
{
	float4 position : POSITION;
#ifdef USE_PER_VERTEX_BASE_COLOR
	float4 color : COLOR;
#endif
#ifdef USE_TEXTURE
	float2 tex : TEXCOORD0;
#endif
#ifdef USE_NORMAL
	float3 normal : NORMAL0;
#endif
#ifdef USE_TANGENT_FRAME
	float3 tangent : TANGENT;
	float3 bitangent : BINORMAL;
#endif
};

struct Output
{
	float4 position : SV_POSITION;
#ifdef USE_PER_VERTEX_BASE_COLOR
	float4 color : COLOR;
#endif
#ifdef USE_TEXTURE
	float2 texCoords : TEXCOORD0;
#endif
#if defined(USE_NORMAL)
	float3 normal : NORMAL;
#ifdef USE_TANGENT_FRAME
	float3 tangent : TANGENT;
	float3 bitangent : BINORMAL;
#endif
#endif
#if defined(USE_WORLD_POSITION)
	float3 worldPosition : POSITION;
#endif
};

Output main(Input input)
{
	Output output;

	input.position.w = 1.0f;
	float4 worldPosition = mul(worldMatrix, input.position);
	output.position = mul(viewProjectionMatrix, worldPosition);
#ifdef USE_TEXTURE
	output.texCoords = input.tex;
#endif
#ifdef USE_PER_VERTEX_BASE_COLOR
	output.color = input.color;
#endif
#ifdef USE_NORMAL
	output.normal = mul((float3x3)worldMatrix, input.normal);
#endif
#ifdef USE_TANGENT_FRAME
	output.tangent = mul((float3x3)worldMatrix, input.tangent);
	output.bitangent = mul((float3x3)worldMatrix, input.bitangent);
#endif
#ifdef USE_WORLD_POSITION
	output.worldPosition = worldPosition.xyz;
#endif

	return output;
}