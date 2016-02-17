// globals
Texture2D heightMap : register(t0);
SamplerState SampleType;

cbuffer MatrixBuffer : register(b0) {
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
cbuffer MaterialBuffer : register(b1) {
    float4 materialAmbient;
    float4 materialDiffuse;
    float4 materialSpecular;
    uint vsSlotsUsed;
    uint psSlotsUsed;
	uint psFlags;
};
cbuffer LightingBuffer : register(b2) {
    float4 viewPosition;
    float4 lightDirection;
    float4 lightAmbient;
    float4 lightDiffuse;
    float4 lightSpecular;
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
    float3 tex : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 dirToLight : TEXCOORD3;
    float3 dirToView : TEXCOORD4;
    float4 worldPos : TEXCOORD5;
};

// vertex shader
VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;

	// Change the position vector to be 4 units for proper matrix calculations.
	//input.position.w = 1.0f;

	// Set the w of the normal to be 0 so it doesn't get translated when rotated.
	//input.normal.w = 0.0f;

	// height displacement
    float4 position = input.position;
	[branch]
    if (vsSlotsUsed & 1 << 0) {
        float heightSample = heightMap.SampleLevel(SampleType, input.tex.xy, 0).x;
        position = float4(position.xyz + input.normal.xyz * heightSample * 0.02f, 1.0f);
    }

	// Calculate the position of the vertex against the world, view, and projection matrices.
    float4 posWorld = mul(position, worldMatrix);
    output.position = mul(posWorld, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

	// store the texture coordinates for the pixel shader
    output.normal = mul(input.normal, worldMatrix).xyz;
	// use the normal since the raycast hits the base sphere anyway
    output.worldPos = float4(output.normal, 1.0f);

    output.tangent = mul(input.tangent, worldMatrix).xyz;
    output.dirToLight = -lightDirection.xyz;
    output.dirToView = viewPosition.xyz - posWorld.xyz;
    output.tex = input.tex;

    return output;
}