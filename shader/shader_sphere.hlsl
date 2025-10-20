struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position       : SV_POSITION;
    float3 position_world : TEXCOORD0;
    float3 normal_world   : TEXCOORD1;
};

cbuffer global_params : register(b0)
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 projection_matrix;
    float4 camera_world;
};

cbuffer global_water_params : register(b1)
{
    float time;
    float earth_angle;
    float tiling_a;
    float speed_a;
    float tiling_b;
    float speed_b;
    float ripple_amp;
    float _pad;
};

// TODO: We do not use any water shader. Maybe delete later?
Texture2D global_water_normal_a : register(t0);
Texture2D global_water_normal_b : register(t1);
SamplerState global_linear_wrap_sampler : register(s0);

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    const float PI = 3.14159265358979323846;
    
    float lon = -radians(input.position.x);
    float lat = radians(input.position.y);

    float4 position = float4(cos(lat) * cos(lon), sin(lat), cos(lat) * sin(lon), 1.0);
    
    float4 world_space = mul(world_matrix, position);
    float4 view_space = mul(view_matrix, world_space);
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    float3 normal_world = mul((float3x3)world_matrix, normalize(position.xyz));

    output.position = clip_space;
    output.position_world = world_space.xyz;
    output.normal_world = normal_world;

    return output;
}

// NOTE: 8x8 Bayer ordered dither table
static const float dither8x8_table[64] =
{
    0,  48, 12, 60, 3,  51, 15, 63,  32, 16, 44, 28, 35, 19, 47, 31, 
    8,  56, 4,  52, 11, 59, 7,  55,  40, 24, 36, 20, 43, 27, 39, 23, 
    2,  50, 14, 62, 1,  49, 13, 61,  34, 18, 46, 30, 33, 17, 45, 29, 
    10, 58, 6,  54, 9,  57, 5,  53,  42, 26, 38, 22, 41, 25, 37, 21
};

float3 dither8x8(float2 pixel)
{
    uint x = ((uint)pixel.x) & 7;
    uint y = ((uint)pixel.y) & 7;
    float t = (dither8x8_table[y * 8 + x] + 0.5) / 64.0; // 0..1
    // ~1/255 in linear space
    return (1.0 / 255.0) * (t - 0.5).xxx;
};

float3 reflect(float3 incoming, float3 normal)
{
    return incoming - 2.0 * dot(incoming, normal) * normal;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float3 n = normalize(input.normal_world);
    float3 v = normalize(camera_world.xyz - input.position_world);
    float3 l = normalize(float3(0.0, 4.0, 2.0));
    // float3 l = normalize(float3(2.0, 2.0, 1.5));
    float3 albedo = float3(0.18, 0.18, 0.18);
    float3 diffuse_color = float3(0.02, 0.02, 0.02);
    float diffuse_intensity = 1.0;
    float3 light_color = float3(1.0, 1.0, 1.0);
    float light_intensity = 1.0;
    float specular_power = 32;
    float ks = 0.08;

    // float3 hit_color = albedo / 3.14159265358979323846 * light_intensity * light_color * max(0.0, dot(n, l));

    float n_dot_l = saturate(dot(n, l));
    float visibility = step(0.0, n_dot_l);

    float3 diffuse = visibility * albedo * n_dot_l * diffuse_color * diffuse_intensity;
    
    float3 r = reflect(-l, n);
    float3 specular = visibility * pow(max(0.0, dot(r, v)), specular_power) * light_color * light_intensity;

    float3 color = diffuse + ks * specular;

    // NOTE: Apply dithering to avoid banding.
    float3 dithered_color = saturate(color) + dither8x8(input.position.xy);
    
    return float4(saturate(dithered_color), 1);

    // return float4(saturate(dot(n, v) * 0.5 + 0.5).xxx, 1.0);
    // return float4(0.5 * (normalize(input.normal_world) + 1.0), 1.0);
}
    
// float3 unpack_rgb(float3 rgb)
// {
//     float3 n = rgb * 2.0 - 1.0;
//     n.xy *= 2.0;
//     n = normalize(n);
//     // n.g = -n.g;

//     return n;
// }

// float4 ps(PS_INPUT input) : SV_TARGET
// {
//     float3 n = normalize(input.normal_world);
//     float lon = atan2(n.z, n.x) + earth_angle;
//     float lat = asin(n.y);
//     float2 uv = float2((lon + 3.14159265) / (2 * 3.14159265),
//                        (lat + 1.57079633) / 3.14159265);

//     float2 uv_a = uv * tiling_a + float2(speed_a * time, 0);
//     float2 uv_b = uv * tiling_b + float2(0, speed_b * time);

//     // float3 n_a = global_water_normal_a.Sample(global_linear_wrap_sampler, uv_a).rgb * 2.0 - 1.0;
//     // float3 n_b = global_water_normal_b.Sample(global_linear_wrap_sampler, uv_b).rgb * 2.0 - 1.0;
//     // n_a.xy *= 4.0;
//     // n_b.xy *= 4.0;
//     // float2 n_t = 0.6 * n_a.xy - 0.4 * n_b.xy;
//     // float ripple = saturate(dot(n_t, n_t));

//     float3 n_a = unpack_rgb(global_water_normal_a.Sample(global_linear_wrap_sampler, uv_a).rgb);
//     float3 n_b = unpack_rgb(global_water_normal_b.Sample(global_linear_wrap_sampler, uv_b).rgb);
//     float3 n_t = normalize(0.6 * n_a + 0.6 * n_b);
//     // float soft_amount = 0.55;                    // 0 = glass-flat, 1 = full detail
//     // n_t = normalize(lerp(float3(0,0,1), n_t, soft_amount));
//     // float ripple = saturate(n_t.z);
//     float ripple = saturate(dot(n_t.xy, n_t.xy));
    
//     float3 base = float3(0.06, 0.18, 0.32);
//     float k = 1.0 + ripple_amp * (ripple - 0.5) * 2.0;
//     float3 color = base * k;

//     float3 t = normalize(float3(-sin(lon), 0, cos(lon)));
//     float3 b = normalize(cross(n, t));
//     float3x3 tbn = float3x3(t, b, n);
//     float3 n_w = normalize(mul(tbn, n_t));

//     float3 l = normalize(float3(-0.3, 0.6, 0.7));
//     float3 v = normalize(camera_world.xyz - input.position_world);
//     float  n_dot_l = saturate(dot(n_w, l));
//     float3 h = normalize(l + v);
//     float spec = pow(saturate(dot(n_w, h)), 32.0);
    
//     color *= 0.55 + 0.45 * n_dot_l;

//     // Fresnel-ish rim (optional but nice)
//     float n_dot_v = saturate(dot(n_w, v));
//     float f0 = 0.02;
//     float fresnel = f0 + (1.0 - f0) * pow(1.0 - n_dot_v, 5.0);
//     color += fresnel * 0.12 + spec * 0.11;
   
//     return float4(color, 1.0);
// }
