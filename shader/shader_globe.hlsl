struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
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

    // NOTE: A bit bigger radius.
    input.position = input.position * 1.0005f;
    
    float4 position = float4(input.position, 1.0);
    float4 world_space = mul(world_matrix, position);
    float4 view_space = mul(view_matrix, world_space);
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    output.position = clip_space;
    output.color = float4(0.1, 0.1, 0.1, 1.0);
    // output.color = float4(input.color, 1.0);

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
     return float4(0.04, 0.04, 0.04, 1.0f);
}
