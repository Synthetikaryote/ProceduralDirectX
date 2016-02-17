// globals
Texture2D heightMap : register(t0);
SamplerState SampleType;

cbuffer DepthMapBuffer : register(b0) {
	matrix worldViewProj;
	uint vsSlotsUsed;
};

// typedefs
struct VertexShaderInput {
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float3 tex : TEXCOORD0;
};
struct VertexShaderOutput {
	float4 position : SV_POSITION;
};

// vertex shader
VertexShaderOutput main(VertexShaderInput input) {
	VertexShaderOutput output;

	// height displacement
	float4 position = input.position;
	if (vsSlotsUsed & 1 << 0) {
		float heightSample = heightMap.SampleLevel(SampleType, input.tex.xy, 0).x;
		position = float4(position.xyz + input.normal.xyz * heightSample * 0.02f, 1.0f);
	}

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(position, worldViewProj);

	return output;
}