
struct PixelShaderInput
{
	float4 CubemapCoords : TEX_COORDS;
};

TextureCube Cubemap : register(t0, space1); // Note: Template constructs need SM 5.1

SamplerState CubemapSampler : register(s0); // Note: Since we are using a static sampler, we do not specify a register space

float4 main(PixelShaderInput IN) : SV_Target
{
	return Cubemap.Sample(CubemapSampler, IN.CubemapCoords.xyz); // Note: It is returning SV_Target (used as color by the pipeline)
}