Texture2D<float4> Textures[] : register(t0);
SamplerState wrapSampler : register(s2);
SamplerState clampSample : register(s1); 

cbuffer PSPerObjectBuffer : register(b1)
{
	float2 distortion1;
	float2 distortion2;
	float2 distortion3;
	float distortionScale;
	float distortionBias;
    uint noiseTexture;
    uint fireTexture;
    uint alphaTexture;
}

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float2 texCoords1 : TEXCOORD1;
	float2 texCoords2 : TEXCOORD2;
	float2 texCoords3 : TEXCOORD3;
};

float4 main(PixelInputType input) : SV_TARGET
{
	float4 noise1;
	float4 noise2;
	float4 noise3;
	float2 finalNoise;
	float perturb;
	float2 noiseCoords;
	float4 fireColor;

	// Sample the same noise texture using the three different texture coordinates to get three different noise scales.
    noise1 = Textures[noiseTexture].Sample(wrapSampler, input.texCoords1);
    noise2 = Textures[noiseTexture].Sample(wrapSampler, input.texCoords2);
    noise3 = Textures[noiseTexture].Sample(wrapSampler, input.texCoords3);

	// Move the noise from the (0, 1) range to the (-0.5, +0.5) range.
	noise1 = noise1 - 0.5f;
	noise2 = noise2 - 0.5f;
	noise3 = noise3 - 0.5f;

	// Distort the three noise x and y coordinates by the three different distortion x and y values.
	noise1.xy = noise1.xy * distortion1.xy;
	noise2.xy = noise2.xy * distortion2.xy;
	noise3.xy = noise3.xy * distortion3.xy;

	// Combine all three distorted noise results into a single noise result.
	finalNoise = noise1.xy + noise2.xy + noise3.xy;

	// Perturb the input texture Y coordinates by the distortion scale and bias values.  
	// The perturbation gets stronger as you move up the texture which creates the flame flickering at the top effect.
	perturb = ((1.0f - input.tex.y) * distortionScale) + distortionBias;

	// Now create the perturbed and distorted texture sampling coordinates that will be used to sample the fire color texture.
	noiseCoords.xy = (finalNoise.xy * perturb) + input.tex.xy;

	// Sample the color from the fire texture using the perturbed and distorted texture sampling coordinates.
	// Use the clamping sample state instead of the wrap sample state to prevent flames wrapping around.
    fireColor = Textures[fireTexture].Sample(clampSample, noiseCoords.xy);
    fireColor.a = (Textures[alphaTexture].Sample(clampSample, noiseCoords.xy)).r;

	return fireColor;
}