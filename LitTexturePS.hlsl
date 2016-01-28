// globals
Texture2D tex : register(t0);
TextureCube cubeTexture : register(t1);
Texture2D spec : register(t2);
Texture2D normalMap : register(t3);
SamplerState SampleType;

cbuffer MaterialBuffer : register(b0) {
    float4 materialAmbient;
    float4 materialDiffuse;
    float4 materialSpecular;
	uint vsSlotsUsed;
	uint psSlotsUsed;
};
cbuffer LightingBuffer : register(b1) {
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
	float3 tangent : TEXCOORD2;
	float3 dirToLight : TEXCOORD3;
	float3 dirToView : TEXCOORD4;
};

// pixel shader
float4 PixelShaderFunction(VertexShaderOutput input) : SV_TARGET {
	float4 ambient = saturate(lightAmbient * materialAmbient);

	// diffuse
	float4 diffuseColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	[branch] if (psSlotsUsed & 1 << 0)
		diffuseColor = tex.Sample(SampleType, input.tex.xy);
	[branch] if (psSlotsUsed & 1 << 1)
		diffuseColor = cubeTexture.Sample(SampleType, input.tex);
	diffuseColor.w = 1.0f;

	// specular
	float specularColor = 0.f;
	[branch] if (psSlotsUsed & 1 << 2)
		specularColor = spec.Sample(SampleType, input.tex.xy).x;

	float3 dirToLight = normalize(input.dirToLight);
	float3 dirToView = normalize(input.dirToView);
	float3 normal = normalize(input.normal);

	// normal map
	float bumpIntensity = 1.0f;
	float3 bumpNormal = normal;
	[branch] if (psSlotsUsed & 1 << 3) {
		float3 tangent = normalize(input.tangent);
		float3 binormal = cross(tangent, normal);
		float3 normalSample = normalMap.Sample(SampleType, input.tex.xy).xyz;
		normalSample = (normalSample * 2.0f) - 1.0f;
		bumpNormal = normalize((normalSample.x * tangent) + (normalSample.y * binormal) + (normalSample.z * normal));
	}

	float d = saturate(dot(dirToLight, bumpNormal));
    float intensity = 1.0f;
	float4 diffuse = intensity * d * lightDiffuse * materialDiffuse;

    float3 reflected = reflect(-dirToLight, normal);
    float shininess = 3.0f;
    float rdv = dot(reflected, dirToView);
    float s = pow(saturate(rdv), shininess);
    float4 specular = saturate(intensity * lightSpecular * materialSpecular * s);

	return (diffuse + ambient) * diffuseColor + (specular * specularColor);
	//return specularColor;
}