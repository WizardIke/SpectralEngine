#ifndef maxAniso
#define maxAniso 16
#endif

cbuffer Material : register(b1)
{
	float virtualTextureID;
	float feedbackBias; // log2( feedbackWidth / windowWidth ) + dynamicLODbias
	float textureWidthInPages;
	float usefulTextureWidth; //width of virtual texture not counting padding
};

struct Input {
	//float4 position : VS_POSITION;
	float4 texcoord0 : TEXCOORD0;
};

float4 main(Input input) : VS_TARGET
{
	const float maxAnisoLog2 = log2(maxAniso);
	
	float2 texcoords = input.texcoord0.xy * usefulTextureWidth;
	float2 dx = ddx(texcoords);
	float2 dy = ddy(texcoords);
	float px = dot(dx, dx);
	float py = dot(dy, dy);
	float maxLod = 0.5 * log2(max(px, py)); // log2(sqrt()) = 0.5*log2()
	float minLod = 0.5 * log2(min(px, py));
	float anisoLOD = maxLod - min(maxLod - minLod, maxAnisoLog2);
	float desiredLod = max(anisoLOD + feedbackBias, 0.0);

	float4 feedback;
	feedback.xy = fragment.texcoord0.xy * textureWidthInPages;
	feedback.z = desiredLod;
	feedback.w = virtualTextureID;
	return feedback;
}