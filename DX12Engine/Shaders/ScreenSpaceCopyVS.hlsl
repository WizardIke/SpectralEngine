cbuffer Material : register(b1)
{
	float4 position;
}

struct Input
{
	uint vertexID : SV_VertexID;
};

float4 main(Input input) : SV_POSITION
{
	// vert id 0 = 0000, uv = (0, 0)
	// vert id 1 = 0001, uv = (1, 0)
	// vert id 2 = 0010, uv = (0, 1)
	// vert id 3 = 0011, uv = (1, 1)
	float2 uv = float2(input.vertexID & 1, (input.vertexID >> 1) & 1);

	return float4(position.x + (position.z * uv.x), position.y - (position.w * uv.y), 0.0f, 1.0f);
}