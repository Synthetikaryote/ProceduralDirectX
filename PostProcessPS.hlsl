SamplerState SampleType;
Texture2D tex : register(t0);

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 normal : TEXCOORD1;
};

cbuffer LightingBuffer : register(b0) {
    float4 viewPosition;
    float4 lightDirection;
    float4 lightAmbient;
    float4 lightDiffuse;
    float4 lightSpecular;
};
cbuffer PostProcessBuffer : register(b1) {
    float2 windowSize;
	uint flags;
}

// http://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader
//float random(float2 p){ return frac(cos(dot(p, float2(23.14069263277926f, 2.665144142690225f)))*123456.0f); }

float4 main(VertexShaderOutput input) : SV_TARGET {
    //float2 pixelSize = float2(1.0f, 1.0f) / windowSize;
    //float4 a = tex.Sample(SampleType, float2(input.tex.x, input.tex.y));
    //float4 b = tex.Sample(SampleType, float2(input.tex.x + pixelSize.x, input.tex.y));
    //float4 c = tex.Sample(SampleType, float2(input.tex.x, input.tex.y + pixelSize.y));
    //float4 d = tex.Sample(SampleType, float2(input.tex.x + pixelSize.x, input.tex.y + pixelSize.y));

    //float4 color = (a + b + c + d) * 0.25f;

	float4 color = tex.Sample(SampleType, input.tex);
	//color = color * (0.2f + (sin(frac(input.tex.y * windowSize.y / 8.0f) * 3.1415926535f * 2.0f) * 0.5f + 0.5f) * 1.1f);
	//float rand = random(input.tex);
	//color = color + rand;

	//color = (color.r + color.g + color.b) / 3.0f;
	//float intensity = (color.r + color.g + color.b) / 3.0f;
	//dot(normalize(lightDirection), input.normal);
	//color = round(intensity * 5) / 5 * color;


	// edge detection
	//float2 dx = ddx(input.tex).x;
	//float2 dy = ddy(input.tex).y;

	if (flags) {
		float deltaU = 1.0f / windowSize.x;
		float deltaV = 1.0f / windowSize.y;
		float2 offset[9] = {
			float2(-deltaU, -deltaV),
			float2(0.0f, -deltaV),
			float2(deltaU, -deltaV),
			float2(-deltaU, 0.0f),
			float2(0.0f, 0.0f),
			float2(deltaU, 0.0f),
			float2(-deltaU, deltaV),
			float2(0.0f, deltaV),
			float2(deltaU, deltaV)
		};
		float kernelX[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
		float kernelY[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
		float3 valX = (float3)0;
			float3 valY = (float3)0;
			for (int i = 0; i < 9; ++i) {
				float4 texColor = tex.Sample(SampleType, input.tex + offset[i]);
					valX += kernelX[i] * texColor.xyz;
				valY += kernelY[i] * texColor.xyz;
			}
		color *= (length((valX * valX) + (valY * valY)) > 0.5f) ? 0.0f : 1.0f;
	}

    return color;
}