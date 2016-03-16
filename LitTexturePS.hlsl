// globals
Texture2D tex : register(t0);
TextureCube cubeTexture : register(t1);
Texture2D spec : register(t2);
Texture2D normalMap : register(t3);
Texture2D shadowMap : register(t4);
Texture2D heightMap : register(t5);
SamplerState SampleType : register(s0);
SamplerState SamplerShadow : register(s1);

cbuffer MaterialBuffer : register(b0) {
    float4 materialAmbient;
    float4 materialDiffuse;
    float4 materialSpecular;
    uint vsSlotsUsed;
    uint psSlotsUsed;
	uint psFlags;
	float time;
};
cbuffer LightingBuffer : register(b1) {
    float4 viewPosition;
    float4 lightDirection;
    float4 lightAmbient;
    float4 lightDiffuse;
    float4 lightSpecular;
	matrix shadowMatrix;
	float2 shadowMapTexelSize;
};
cbuffer BrushBuffer : register(b2) {
    float4 cursorPosition;
    uint cursorFlags;
    float cursorRadiusSq;
    float cursorLineThickness;
}

// typedefs
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float3 tex : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 tangent : TEXCOORD2;
    float3 dirToLight : TEXCOORD3;
    float3 dirToView : TEXCOORD4;
    float4 worldPos : TEXCOORD5;
	float4 shadowPosition : TEXCOORD6;
};

// pixel shader
float4 main(VertexShaderOutput input) : SV_TARGET {
    float4 ambient = saturate(lightAmbient * materialAmbient);

	// diffuse
    float4 diffuseColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (psSlotsUsed & 1 << 0)
        diffuseColor = tex.Sample(SampleType, input.tex.xy);
    if (psSlotsUsed & 1 << 1)
        diffuseColor = cubeTexture.Sample(SampleType, input.tex);
    diffuseColor.w = 1.0f;

	// specular
    float specularColor = 0.0f;
	if (psSlotsUsed & 1 << 2) {
		specularColor = spec.Sample(SampleType, input.tex.xy).x;
	}

    float3 dirToLight = normalize(input.dirToLight);
    float3 dirToView = normalize(input.dirToView);
    float3 normal = normalize(input.normal);

	// normal map
    float bumpIntensity = 1.0f;
    float3 bumpNormal = normal;
    if (psSlotsUsed & 1 << 3) {
        float3 tangent = normalize(input.tangent);
        float3 binormal = cross(tangent, normal);
        float3 normalSample = normalMap.Sample(SampleType, input.tex.xy).xyz;
        normalSample = (normalSample * 2.0f) - 1.0f;
        bumpNormal = normalize((normalSample.x * tangent) + (normalSample.y * binormal) + (normalSample.z * normal));
    }

	// height map effect
	if (psSlotsUsed & 1 << 5) {
		float height = 0.38f - heightMap.Sample(SampleType, input.tex.xy).x;
		if (height > sin(time * 0.01f * 3.14159f * 2.0f + 0.4f * 3.14159f) * 0.385f || specularColor > 0.3f) {
			float s = 2.0f;
			float2 noise = (frac(sin(dot(input.tex.xy, float2(12.9898, 78.233)*2.0)) * 43758.5453));
				float rand = abs(noise.x + noise.y) * 0.5;
			float randR = sin(input.tex.x) * 0.5f + 0.5f;
			float randG = cos(input.tex.y) * 0.5f + 0.5f;
			float randB = saturate(randR + randG);
			float lit = saturate(((cos(input.tex.x) * 0.5f + 0.5f) * (sin(input.tex.y) * 0.5f + 0.5f)) * rand);
			diffuseColor = float4(0.3f, 0.6f, 0.8f, 1.0f);
			bumpNormal = normal;
			specularColor = 1.0f;
			if (rand > 0.995f) {
				float i = abs(sin((time / s + 0.5f + input.tex.x * 6.28 * 100 + input.tex.y * 6.28 * 50) * 3.14));
				diffuseColor = float4(0.3f + 0.7f * i, 0.6f + 0.4f * i, 0.8f + 0.2f * i, 1.0f);
			}
		}
	}

    float d = saturate(dot(dirToLight, bumpNormal));
	if (psFlags & 1) {
		d = round(d * 3.0f) / 3.0f;
	}
    float intensity = 1.0f;
    float4 diffuse = intensity * d * lightDiffuse * materialDiffuse;

    float3 reflected = reflect(-dirToLight, normal);
    float shininess = 3.0f;
    float s = pow(saturate(dot(reflected, dirToView)), shininess);
	if (psFlags & 1) {
		s = round(s * 3.0f) / 3.0f;
	}
	float4 specular = saturate(intensity * lightSpecular * materialSpecular * s);

    // cursor
    if (cursorFlags) {
        float3 dist = input.worldPos.xyz - cursorPosition.xyz;
        float lenSq = dot(dist, dist);
        if (lenSq < cursorRadiusSq) {
            diffuse *= 1.5f;
            ambient = 0.5f;
        }
    }

	float4 litColor = (diffuse + ambient) * diffuseColor + (specular * specularColor);

	// shadow
	if (psSlotsUsed & 1 << 4) {
		float3 shadowPosition = input.shadowPosition.xyz / input.shadowPosition.w;

		float shadowCoeff = 1.0f;
		bool use4Sample = true;
		if (use4Sample) {
			// 4-sample pcf from nvidia
			// http://http.developer.nvidia.com/GPUGems/gpugems_ch11.html
			float2 offset = (float)(frac(shadowPosition.xy * 0.5) > 0.25);  // mod
				offset.y += offset.x;  // y ^= x in floating point
			if (offset.y > 1.1)
				offset.y = 0;
			shadowCoeff = 1.0 - (
				(shadowPosition.z > shadowMap.Sample(SamplerShadow, shadowPosition.xy + (offset + float2(-1.5, 0.5)) * shadowMapTexelSize).r + 0.0001 ? 1.0 : 0.0) +
				(shadowPosition.z > shadowMap.Sample(SamplerShadow, shadowPosition.xy + (offset + float2(0.5, 0.5)) * shadowMapTexelSize).r + 0.0001 ? 1.0 : 0.0) +
				(shadowPosition.z > shadowMap.Sample(SamplerShadow, shadowPosition.xy + (offset + float2(-1.5, -1.5)) * shadowMapTexelSize).r + 0.0001 ? 1.0 : 0.0) +
				(shadowPosition.z > shadowMap.Sample(SamplerShadow, shadowPosition.xy + (offset + float2(0.5, -1.5)) * shadowMapTexelSize).r + 0.0001 ? 1.0 : 0.0)
			) * 0.25;
		}
		else {
			// brute force 16-hit pcf
			float sum = 0;
			float x, y;
			for (y = -1.5; y <= 1.5; y += 1.0)
				for (x = -1.5; x <= 1.5; x += 1.0)
					sum += shadowPosition.z > shadowMap.Sample(SamplerShadow, shadowPosition.xy + float2(x, y) * shadowMapTexelSize).r + 0.0001 ? 1.0 : 0.0;
			shadowCoeff = 1.0 - sum / 16.0;
		}
		litColor = (diffuse * shadowCoeff + ambient) * diffuseColor + (specular * shadowCoeff * specularColor);
	}

	return litColor;

}
