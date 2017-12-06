Texture2D<float4> Textures[] : register(t0);
SamplerState SampleType : register(s2);

cbuffer PSperObjectConstantBuffer : register(b1)
{
	float4 diffuseColor[4];
    uint DiffuseTexture;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 lightPos1 : TEXCOORD1;
	float3 lightPos2 : TEXCOORD2;
	float3 lightPos3 : TEXCOORD3;
	float3 lightPos4 : TEXCOORD4;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float4 textureColor;
	float lightIntensity1, lightIntensity2, lightIntensity3, lightIntensity4;
	float4 color, color1, color2, color3, color4;
	
	lightIntensity1 = saturate(dot(input.normal, input.lightPos1));
	lightIntensity2 = saturate(dot(input.normal, input.lightPos2));
	lightIntensity3 = saturate(dot(input.normal, input.lightPos3));
	lightIntensity4 = saturate(dot(input.normal, input.lightPos4));

	color1 = diffuseColor[0] * lightIntensity1;
	color2 = diffuseColor[1] * lightIntensity2;
	color3 = diffuseColor[2] * lightIntensity3;
	color4 = diffuseColor[3] * lightIntensity4;

    textureColor = Textures[DiffuseTexture].Sample(SampleType, input.tex);

	color = saturate(color1 + color2 + color3 + color4) * textureColor;

	return color;
}