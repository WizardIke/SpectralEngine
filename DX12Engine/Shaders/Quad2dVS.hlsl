//#define USE_PER_VERTEX_COLOR
//#define USE_TEXTURE

#include "Quad2dMaterialVS.h"

struct Output
{
	float4 pos : SV_POSITION;
#ifdef USE_PER_VERTEX_COLOR
	float4 color : COLOR;
#endif
#ifdef USE_TEXTURE
	float2 texCoords : TEXCOORD;
#endif
};

Output main(uint vertexID : SV_VertexID)
{
	Output output;

	// vert id 0 = 0000, uv = (0, 0)
	// vert id 1 = 0001, uv = (1, 0)
	// vert id 2 = 0010, uv = (0, 1)
	// vert id 3 = 0011, uv = (1, 1)
	float2 uv = float2(vertexID & 1, (vertexID >> 1) & 1);

	output.pos = float4(pos.x + (pos.z * uv.x), pos.y - (pos.w * uv.y), 0.0f, 1.0f);
#ifdef USE_PER_VERTEX_COLOR
	output.color = color;
#endif
#ifdef USE_TEXTURE
	output.texCoords = float2(texCoords.x + (texCoords.z * uv.x), texCoords.y + (texCoords.w * uv.y));
#endif

	return output;
}