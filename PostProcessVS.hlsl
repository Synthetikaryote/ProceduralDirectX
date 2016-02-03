struct VertexShaderInput {
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float3 tex : TEXCOORD0;
};
struct VertexShaderOutput {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input) {
	VertexShaderOutput output;
	output.position = input.position;
	output.tex = input.tex.xy;
	return output;
}