struct ModelViewProjection
{
	matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0); // Template Constructs require ShaderModel 5.1

struct VertexPosColor
{
	float3 Position : POSITION; // Note: the value semantics here need to reflect what was specified for the imput assembler during pipeline creation
	float3 CubemapCoords : TEX_COORDS;
};

struct VertexShaderOutput
{
	float4 CubemapCoords : TEX_COORDS;
	float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
	OUT.CubemapCoords = float4(IN.CubemapCoords, 1.0f);

	return OUT;
}