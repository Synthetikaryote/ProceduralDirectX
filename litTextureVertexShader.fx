// globals
cbuffer MatrixBuffer : register (b0) {
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
cbuffer MaterialBuffer : register (b1) {
	float4 materialAmbient;
	float4 materialDiffuse;
	float4 materialSpecular;
};
cbuffer LightingBuffer : register (b2) {
	float4 viewPosition;
	float4 lightDirection;
	float4 lightAmbient;
	float4 lightDiffuse;
	float4 lightSpecular;
};

// typedefs
struct VertexInputType {
    float4 position : POSITION;
	float4 normal : NORMAL;
    float2 tex : TEXCOORD0;
};
struct PixelInputType {
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float3 dirToLight : TEXCOORD2;
	float3 dirToView : TEXCOORD3;
};

// vertex shader
PixelInputType LitTextureVertexShader(VertexInputType input) {
    PixelInputType output;

    // Change the position vector to be 4 units for proper matrix calculations.
    //input.position.w = 1.0f;

	// Set the w of the normal to be 0 so it doesn't get translated when rotated.
	//input.normal.w = 0.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
	float4 posWorld = mul(input.position, worldMatrix);
    output.position = mul(posWorld, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // store the texture coordinates for the pixel shader
	output.normal = mul(input.normal, worldMatrix).xyz;
	output.dirToLight = -lightDirection.xyz;
	output.dirToView = viewPosition.xyz - posWorld.xyz;
    output.tex = input.tex;
    
    return output;
}