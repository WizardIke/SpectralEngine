cbuffer VSPerFrameBuffer : register(b0)
{
    float screenWidth;
    float screenHeight;
}


float2 main(uint vertexID : SV_VertexID) : TEXCOORD0
{
    float2 pos = float2((vertexID & 1) * screenWidth, ((vertexID >> 1) & 1) * screenHeight);
	return pos;
}