struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct PS_INPUT
{
    float4 position    : SV_POSITION;
    float3 normal_view : TEXCOORD0;
};

cbuffer global_settings : register(b0)
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 projection_matrix;
};

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    float4 position = float4(input.position, 1.0);
    float4 world_space = mul(world_matrix, position);
    float4 view_space = mul(view_matrix, world_space);
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    float4 normal_world = mul(world_matrix, float4(input.normal, 1.0));
    float4 normal_view = mul(view_matrix, normal_world);

    output.position = clip_space;
    output.normal_view = normalize(normal_view.xyz);

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    // float3 light_dir = normalize(float3(0.5, 0.7, -1.0));
    // float  ndl  = saturate(dot(normalize(input.normal_view), -light_dir));
    // float3 base = float3(0.1, 0.4, 0.9);
    // float3 col  = base*(0.2 + 0.8*ndl);

    return float4(0.0, 0.0, 0.0, 1.0);
}
