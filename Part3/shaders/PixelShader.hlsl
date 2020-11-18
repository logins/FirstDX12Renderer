struct PixelShaderInput
{
	float4 Color : COLOR;
};

struct ColorMod
{
	float Modifier;
};

ConstantBuffer<ColorMod> ColorModifierCB : register(b0, space1); // Note: Template constructs need SM 5.1

float4 main(PixelShaderInput IN) : SV_Target
{
	IN.Color.xyz *= ColorModifierCB.Modifier;

	return IN.Color;
}