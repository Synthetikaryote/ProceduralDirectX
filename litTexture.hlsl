// globals
Texture2D shaderTexture;
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
    float2 tex : TEXCOORD0;
};
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 dirToLight : TEXCOORD2;
    float3 dirToView : TEXCOORD3;
};

// vertex shader
VertexShaderOutput VertexShaderFunction(VertexShaderInput input) {
    VertexShaderOutput output;

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

// pixel shader
float4 PixelShaderFunction(VertexShaderOutput input) : SV_TARGET {
    float4 ambient = saturate(lightAmbient * materialAmbient);

    float4 diffuseColor = shaderTexture.Sample(SampleType, input.tex);

    float3 dirToLight = normalize(input.dirToLight);
    float3 dirToView = normalize(input.dirToView);
    float3 normal = normalize(input.normal);
    float d = saturate(dot(dirToLight, normal));
    float intensity = 1.0f;
    float4 diffuse = intensity * d * lightDiffuse * materialDiffuse;

    float3 reflected = reflect(-dirToLight, normal);
    float shininess = 3.0f;
    float rdv = dot(reflected, dirToView);
    float s = pow(saturate(rdv), shininess);
    float4 specular = saturate(intensity * lightSpecular * materialSpecular * s);

    return (diffuse + ambient) * diffuseColor + specular;
}