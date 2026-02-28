#include "utils.hlsl"

struct VS_INPUT
{
    float3 prev : TEXCOORD0;
    float3 position : POSITION;
    float3 next : TEXCOORD1;
    float side : TEXCOORD2;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float side : TEXCOORD;
    //float clip : SV_ClipDistance;
};

cbuffer global_param : register(b0)
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 projection_matrix;
    float4 camera_world;
};

cbuffer global_globe_param : register(b1)
{
    float shape;
    float yaw;
    float pitch;
    float line_thickness;
    float2 scale;
    float2 viewport_size;
    float4 color;
};

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    float p_lon = radians(input.prev.x);
    float p_lat = radians(input.prev.y);
    float lon = radians(input.position.x);
    float lat = radians(input.position.y);
    float n_lon = radians(input.next.x);
    float n_lat = radians(input.next.y);

    float3 p_globe_position = float3(cos(p_lat) * sin(p_lon), sin(p_lat), cos(p_lat) * cos(p_lon)) * 1.005;
    float3 globe_position = float3(cos(lat) * sin(lon), sin(lat), cos(lat) * cos(lon)) * 1.005;
    float3 n_globe_position = float3(cos(n_lat) * sin(n_lon), sin(n_lat), cos(n_lat) * cos(n_lon)) * 1.005;

    p_globe_position = rotate_y(p_globe_position, radians(yaw));
    p_globe_position = rotate_x(p_globe_position, radians(pitch));
    globe_position = rotate_y(globe_position, radians(yaw));
    globe_position = rotate_x(globe_position, radians(pitch));
    n_globe_position = rotate_y(n_globe_position, radians(yaw));
    n_globe_position = rotate_x(n_globe_position, radians(pitch));

    float3 p_flat_position = float3(n_lon * scale.x, n_lat * scale.y, 0.0) * 0.9995;
    float3 flat_position = float3(lon * scale.x, lat * scale.y, 0.0) * 0.9995;
    float3 n_flat_position = float3(p_lon * scale.x, p_lat * scale.y, 0.0) * 0.9995;

    float4 p_position = float4(lerp(p_flat_position, p_globe_position, ease_in_out_back(saturate(shape))), 1.0);
    float4 position = float4(lerp(flat_position, globe_position, ease_in_out_back(saturate(shape))), 1.0);
    float4 n_position = float4(lerp(n_flat_position, n_globe_position, ease_in_out_back(saturate(shape))), 1.0);

    float4 p_clip = mul(projection_matrix, mul(view_matrix, mul(world_matrix, p_position)));
    float4 clip = mul(projection_matrix, mul(view_matrix, mul(world_matrix, position)));
    float4 n_clip = mul(projection_matrix, mul(view_matrix, mul(world_matrix, n_position)));

    float2 p_ndc = p_clip.xy / p_clip.w;
    float2 ndc = clip.xy / clip.w;
    float2 n_ndc = n_clip.xy / n_clip.w;

    float2 d0 = ndc - p_ndc;
    float2 d1 = n_ndc - ndc;

    float eps = 1e-6;
    float l0 = dot(d0, d0);
    float l1 = dot(d1, d1);

    if (l0 < eps && l1 < eps)
    {
        // all three points basically the same
        d0 = float2(1.0, 0);
        d1 = float2(1.0, 0);
    }
    else if (l0 < eps)
    {
        d0 = d1;
    }
    else if (l1 < eps)
    {
        d1 = d0;
    }

    float2 dir0 = normalize(d0);
    float2 dir1 = normalize(d1);

    float2 perp0 = float2(-dir0.y, dir0.x);
    float2 perp1 = float2(-dir1.y, dir1.x);

    float2 miter_dir = perp0 + perp1;

    float2 scale = 1.0 / dot(miter_dir, perp1);
    float miter_level = 2.0;
    scale = clamp(scale, -miter_level, miter_level);

    float thickness = line_thickness;
    float half_thickness = thickness * 0.5;
    float2 ndc_per_pixel = 2.0 / viewport_size;

    float2 offset = miter_dir * scale * half_thickness * ndc_per_pixel * input.side;
    float2 new_position = ndc + offset;

    output.position = float4(new_position.xy * clip.w, clip.z, clip.w);
    output.side = input.side;

    //float4 sphere_center = float4(0, 0, 0 ,1);
    //float4 sphere_vs = mul(view_matrix, sphere_center); // sphere center in view space
    //// n_vs: unit vector from center to camera (camera is at origin in view space)
    //float3 n_vs = normalize(-sphere_vs.xyz);
    //// v_vs: center -> point (in view space)
    //float3 v_vs = mul(view_matrix, position).xyz - sphere_vs.xyz;
    //float cosphi = dot(normalize(v_vs), n_vs);
    //
    //output.clip = cosphi - sin(radians(30.0f));
    
    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float distance = abs(input.side);
    float width = fwidth(distance);
    
    float alpha = 1.0 - smoothstep(1.0 - width, 1.0 + width, distance);
    
    return float4(color.rgb, alpha);
}

