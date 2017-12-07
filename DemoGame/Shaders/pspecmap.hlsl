Texture2D diffuseTexture;
Texture2D NormalTexture;
Texture2D SpecularTexture;
SamplerState SampleType;

cbuffer LightBuffer
{
	float4 ambientColor;
	float4 diffuseColor;
	float4 specularColor;
	float specularPower;
	float3 lightDirection;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float3 viewDirection : TEXCOORD1;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float4 textureColor;
	float4 bumpMap;
	float3 bumpNormal;
	float3 lightDir;
	float lightIntensity;
	float4 color;
	float4 specularIntensity;
	float3 reflection;
	float4 specular;

	// Sample the texture pixel at this location.
	textureColor = diffuseTexture.Sample(SampleType, input.tex);

	// Sample the pixel in the bump map.
	bumpMap = NormalTexture.Sample(SampleType, input.tex);

	// Expand the range of the normal value from (0, +1) to (-1, +1).
	bumpMap = (bumpMap * 2.0f) - 1.0f;

	// Calculate the normal from the data in the bump map.
	bumpNormal = input.normal + bumpMap.x * input.tangent + bumpMap.y * input.binormal;

	// Normalize the resulting bump normal.
	bumpNormal = normalize(bumpNormal);

	// Invert the light direction for calculations.
	lightDir = -lightDirection;

	// Calculate the amount of light on this pixel based on the bump map normal value.
	lightIntensity = saturate(dot(bumpNormal, lightDir));

	color = ambientColor;

	if (lightIntensity > 0.0f)
	{
		color += (diffuseColor * lightIntensity);

		color *= textureColor;
		// Sample the pixel from the specular map texture.
		specularIntensity = SpecularTexture.Sample(SampleType, input.tex);
		// Calculate the reflection vector based on the light intensity, normal vector, and light direction.
		reflection = normalize(2 * lightIntensity * bumpNormal - lightDir);

		// Determine the amount of specular light based on the reflection vector, viewing direction, and specular power.
		specular = pow(saturate(dot(reflection, input.viewDirection)), specularPower);
		// Use the specular map to determine the intensity of specular light at this pixel.
		specular = specular * specularIntensity;

		// Add the specular component last to the output color.
		color = saturate(color + specular);
	}
	else color *= textureColor;

	return color;
}