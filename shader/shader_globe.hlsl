struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float4 clip     : SV_ClipDistance0;
    float debug     : TEXCOORD0;
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

    // NOTE: World center in view space.
    float4 center_view = mul(view_matrix, float4(0.0, 0.0, 0.0, 1.0));
    // NOTE: Center to camera vector. Camera is at the origin in view space.
    float3 center_to_camera = float3(0.0, 0.0, 0.0) - center_view.xyz;
    float3 center_to_point = view_space.xyz - center_view.xyz;
    float cos_phi = dot(normalize(center_to_point), normalize(center_to_camera));

    output.position = clip_space;
    output.color = float4(0.1, 0.1, 0.1, 1.0);
    // output.color = float4(input.color, 1.0);

    float degree = 20.0;
    float rad = radians(degree);
    float threshold = sin(rad);
    
    output.clip = cos_phi - threshold;
    // output.clip = 1;
    output.debug = cos_phi - threshold;

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    // return float4(step(0.0, input.debug), 0, step(0, -input.debug), 1.0);
    // return float4(0.9964f, 0.8431f, 0.4941f, 1.0f);
    return float4(0.04, 0.04, 0.04, 1.0f);
    // return input.color;
}
