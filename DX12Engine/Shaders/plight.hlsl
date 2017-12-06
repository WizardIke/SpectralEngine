Texture2D<float4> Textures[] : register(t0);

SamplerState SampleType : register(s2);

cbuffer PSPerObjectConstantBuffer : register(b1)
{
	float4 ambientColor;
	float4 diffuseColor;
    float4 specularColor;
	float3 lightDirection;
	float specularPower;
    uint diffuseTexture;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDirection : TEXCOORD1;
};

float4 main(PixelInputType input) : SV_TARGET
{
    float4 color = Textures[diffuseTexture].Sample(SampleType, input.tex);
    float4 light = ambientColor;
    float3 normal = normalize(input.normal);
    float lightIntensity = dot(normal, -lightDirection);
	if (lightIntensity > 0.0f)
	{
        light += (diffuseColor * lightIntensity);

        float3 reflection = normalize(2.0f * lightIntensity * normal + lightDirection);
        light += pow(max(dot(reflection, normalize(input.viewDirection)), 0.0f), specularPower); //add specular
    }

    color = color * light;
	color = saturate(color);

	return color;
}