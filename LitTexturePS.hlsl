// globals
TextureCube cubeTexture : register(t0);
SamplerState SampleType;

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
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float3 tex : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 dirToLight : TEXCOORD2;
    float3 dirToView : TEXCOORD3;
};

// pixel shader
float4 PixelShaderFunction(VertexShaderOutput input) : SV_TARGET {
    float4 ambient = saturate(lightAmbient * materialAmbient);

    float4 diffuseColor = cubeTexture.Sample(SampleType, input.tex);
    diffuseColor.w = 1.0f;

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