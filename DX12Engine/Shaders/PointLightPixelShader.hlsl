Texture2D<float> depthBuffer;

struct Shader
{
    uint id;
};

struct ShaderMapNode
{
    uint shaderCount;
    Shader shaders[15];
};

RWStructuredBuffer<ShaderMapNode> shaderMap;

struct Output
{
    float4 color : SV_TARGET0;

};

float4 main() : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}