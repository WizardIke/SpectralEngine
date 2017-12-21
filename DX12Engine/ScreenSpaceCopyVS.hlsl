cbuffer Material : register(b1)
{
	float4 position;
}

float4 main() : SV_POSITION
{
	// vert id 0 = 0000, uv = (0, 0)
	// vert id 1 = 0001, uv = (1, 0)
	// vert id 2 = 0010, uv = (0, 1)
	// vert id 3 = 0011, uv = (1, 1)
	float2 uv = float2(input.vertexID & 1, (input.vertexID >> 1) & 1);

	return float4(input.pos.x + (input.pos.z * uv.x), input.pos.y - (input.pos.w * uv.y), 0.0f, 1.0f);
}