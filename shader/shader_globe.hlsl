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

cbuffer global_globe_param : register(b1)
{
    float shape;
    float2 scale;
    float _pad;
};

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    const float PI = 3.14159265358979323846;
    
    float lon = radians(input.position.x);
    float lat = radians(input.position.y);

    float3 globe_position = float3(cos(lat) * cos(-lon + PI * 0.5), sin(lat), cos(lat) * sin(-lon + PI * 0.5)) * 1.005;
    float3 flat_position = float3(lon * scale.x, lat * scale.y, 0.0) * 0.9995;
    float4 position = float4(lerp(flat_position, globe_position, saturate(shape)), 1.0);

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
