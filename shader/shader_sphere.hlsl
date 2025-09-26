struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct PS_INPUT
{
    float4 position    : SV_POSITION;
    float3 position_world : TEXCOORD0;
    float3 normal_world :   TEXCOORD1;
};

cbuffer global_settings : register(b0)
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 projection_matrix;
    float4 camera_world;
};

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    float4 position = float4(input.position, 1.0);
    float4 world_space = mul(world_matrix, position);
    float4 view_space = mul(view_matrix, world_space);
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    float3 normal_world = mul((float3x3)world_matrix, input.normal);

    output.position = clip_space;
    output.position_world = world_space.xyz;
    output.normal_world = normal_world;

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float3 n = normalize(input.normal_world);
    float3 v = normalize(camera_world.xyz - input.position_world);
    
    return float4(saturate(dot(n, v) * 0.5 + 0.5).xxx, 1.0);
    // return float4(0.5 * (normalize(input.normal_world) + 1.0), 1.0);
}
