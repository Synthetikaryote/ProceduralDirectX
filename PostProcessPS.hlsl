SamplerState SampleType;
Texture2D tex : register(t0);

struct VertexShaderOutput {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(VertexShaderOutput input) : SV_TARGET
{
	float4 color = tex.Sample(SampleType, input.tex);
	//color = (color.r + color.g + color.b) / 3.0f;
	return color;
}