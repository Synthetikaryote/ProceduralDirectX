struct VertexShaderInput {
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
    float3 tex : TEXCOORD0;
};
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 normal : TEXCOORD1;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    output.position = input.position;
    output.normal = input.normal;
    output.tex = input.tex.xy;
    return output;
}