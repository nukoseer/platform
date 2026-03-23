#include "utils.hlsl"

struct VS_INPUT
{
    float2 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
};

cbuffer global_shape_param : register(b0)
{
    float4 color;
    float2 viewport_size;
    float2 center;
    float2 main_center;
    float2 part_center;
    float max_radius;
    float zoom;
    float _pad[2];
};

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    float lon = radians(input.position.x);
    float lat = radians(input.position.y);
    
    float2 main_center_lonlat = radians(float2(main_center.x, main_center.y));
    float2 part_center_lonlat = radians(float2(part_center.x, part_center.y));
    
    float2 offset = float2(lon - main_center_lonlat.x, lat - main_center_lonlat.y);

    float aspect = viewport_size.x / viewport_size.y;
    float2 position = float2(offset.x / aspect * zoom, offset.y * zoom);
    
    //float2 clip_offset = float2(
    //    (1200 / viewport_size.x) * 2.0 - 1.0,
    //    1.0 - (560 / viewport_size.y) * 2.0  // flip Y
    //);
    //
    //position += clip_offset;
      
    // Project directly to clip space — no world/view/proj needed
    float4 clip = float4(position, 0.5, 1.0);
 
    output.position = clip;
    
    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    return color;
}

