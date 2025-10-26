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
    float _pad0;
    
    float2 center;
    uint center_enable;
    float depth_nudge;

    float4 color;
};

float ease_in_out_back(float t)
{
    const float c1 = 1.70158;
    const float c2 = c1 * 1.525;
    
    return t < 0.5 ? 
        (pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2 :
        (pow(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;
}

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    const float PI = 3.14159265358979323846;
    
    float lon = radians(input.position.x);
    float lat = radians(input.position.y);

    float3 globe_position = float3(cos(lat) * cos(-lon + PI * 0.5), sin(lat), cos(lat) * sin(-lon + PI * 0.5)) * 1.005;

    if (center_enable)
    {
        lon += center.x;
        lat -= center.y;
    }

    float3 flat_position = float3(lon * scale.x, lat * scale.y, 0.0) * 0.9995;

    if (center_enable)
    {
        flat_position *= 2.0;
    }

    float4 position = float4(lerp(flat_position, globe_position, ease_in_out_back(saturate(shape))), 1.0);
    float4 world_space = mul(world_matrix, position);
    float4 view_space = mul(view_matrix, world_space);
    
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    output.position = clip_space;
    output.color = float4(0.1, 0.1, 0.1, 1.0);
    output.color = color;

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    return input.color;
}
