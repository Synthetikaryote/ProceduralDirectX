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
}

float4 main(VertexShaderOutput input) : SV_TARGET {
    //float2 pixelSize = float2(1.0f, 1.0f) / windowSize;
    //float4 a = tex.Sample(SampleType, float2(input.tex.x, input.tex.y));
    //float4 b = tex.Sample(SampleType, float2(input.tex.x + pixelSize.x, input.tex.y));
    //float4 c = tex.Sample(SampleType, float2(input.tex.x, input.tex.y + pixelSize.y));
    //float4 d = tex.Sample(SampleType, float2(input.tex.x + pixelSize.x, input.tex.y + pixelSize.y));

    //float4 color = (a + b + c + d) * 0.25f;

    float4 color = tex.Sample(SampleType, input.tex);
    color = color * (0.2f + (sin(frac(input.tex.y * windowSize.y / 4.0f) * 3.1415926535f * 2.0f) * 0.5f + 0.5f) * 1.1f);
	//color = (color.r + color.g + color.b) / 3.0f;
    //float intensity = (color.r + color.g + color.b) / 3.0f;
    //dot(normalize(lightDirection), input.normal);
    //color = round(intensity * 5) / 5 * color;
    return color;
}