#ifndef maxAniso
#define maxAniso 16
#endif

#include "VtFeedbackCameraMaterial.h"
#include "VtFeedbackMaterialPS.h"

struct Input 
{
	float4 position : SV_POSITION;
	float2 texcoords : TEXCOORD0;
};

float4 main(Input input) : SV_TARGET
{
	const float maxAnisoLog2 = log2(maxAniso);
	
	float2 texcoords = input.texcoords * float2(usefulTextureWidth, usefulTextureHeight);
	float2 dx = ddx(texcoords);
	float2 dy = ddy(texcoords);
	float px = dot(dx, dx);
	float py = dot(dy, dy);
	float maxLod = 0.5 * log2(max(px, py)); // log2(sqrt()) = 0.5*log2()
	float minLod = 0.5 * log2(min(px, py));
	float anisoLOD = maxLod - min(maxLod - minLod, maxAnisoLog2);
	float desiredLod = max(anisoLOD + feedbackBias, 0.0);

	float4 feedback;
    feedback.xy = input.texcoords * float2(textureWidthInPages, textureHeightInPages);
    feedback.z = virtualTextureID1 + desiredLod;
	feedback.w = virtualTextureID2And3;
	return feedback;
}