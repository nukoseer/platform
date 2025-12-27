struct VS_INPUT
{
    float3 position : POSITION;
};

struct GS_INPUT
{
    float4 position : SV_POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float side : TEXCOORD0;
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
    float yaw;
    float pitch;
    float _pad;
    float2 scale;
    float2 viewport_size;
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

float3 rotate_y(float3 position, float yaw)
{
    float cos_yaw = cos(yaw);
    float sin_yaw = sin(yaw);

    return float3(cos_yaw * position.x + sin_yaw * position.z,
                  position.y,
                  -sin_yaw * position.x + cos_yaw * position.z);
}

float3 rotate_x(float3 position, float pitch)
{
    float cos_pitch = cos(pitch);
    float sin_pitch = sin(pitch);

    return float3(position.x,
                  cos_pitch * position.y + sin_pitch * position.z,
                  -sin_pitch * position.y + cos_pitch * position.z);
}

GS_INPUT vs(VS_INPUT input)
{
    GS_INPUT output;

    float lon = radians(input.position.x);
    float lat = radians(input.position.y);

    float3 globe_position = float3(cos(lat) * sin(lon), sin(lat), cos(lat) * cos(lon)) * 1.005;
    globe_position = rotate_y(globe_position, radians(yaw));
    globe_position = rotate_x(globe_position, radians(pitch));
    float3 flat_position = float3(lon * scale.x, lat * scale.y, 0.0) * 0.9995;

    float4 position = float4(lerp(flat_position, globe_position, ease_in_out_back(saturate(shape))), 1.0);
    float4 world_space = mul(world_matrix, position);
    float4 view_space = mul(view_matrix, world_space);
    
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    output.position = clip_space;

    return output;
}

[maxvertexcount(128)]
void gs(line GS_INPUT lines[2], inout TriangleStream<PS_INPUT> tri)
{
    float4 a = lines[0].position;
    float4 b = lines[1].position;

    // NOTE: Clip space to ndc.
    float2 a_ndc = a.xy / a.w;
    float2 b_ndc = b.xy / b.w;

    float2 d = b_ndc - a_ndc;
    float len_squared = dot(d, d);
    if (len_squared < 1e-12) return;

    float2 direction = d * rsqrt(len_squared);
    float2 perpendicular = float2(-direction.y, direction.x);

    float thickness = 4.0;
    float2 ndc_per_pixel = 2.0 / viewport_size;
    float2 offset_ndc = perpendicular * thickness * 0.5 * ndc_per_pixel;

    float2 a0 = a_ndc - offset_ndc;
    float2 a1 = a_ndc + offset_ndc;
    float2 b0 = b_ndc - offset_ndc;
    float2 b1 = b_ndc + offset_ndc;

    PS_INPUT output;

    output.position = float4(a0 * a.w, a.z, a.w);
    output.side = -1;
    tri.Append(output);
    
    output.position = float4(a1 * a.w, a.z, a.w);
    output.side = 1;
    tri.Append(output);
    
    output.position = float4(b0 * b.w, b.z, b.w);
    output.side = -1;
    tri.Append(output);
    
    output.position = float4(b1 * b.w, b.z, b.w);
    output.side = 1;
    tri.Append(output);

    tri.RestartStrip();

    const int segments = 32;

    for (int i = 0; i <= segments; ++i)
    {
        float angle = 3.14159 * float(i) / float(segments);
        float2 offset = cos(angle) * perpendicular * thickness * 0.5 + sin(angle) * direction * thickness * 0.5;
        float2 ndc_offset = offset * ndc_per_pixel;
        output.position = float4((a_ndc + ndc_offset) * a.w, a.z, a.w);
        output.side = sin(angle);
        tri.Append(output);
    }

    tri.RestartStrip();

    // float half_thickness = thickness * 0.5;
    // int i = 0;
    // // First, emit center as degenerate start if needed, but for fan in strip: alternate center and arc
    // for (i = 0; i < segments; ++i)
    // {
    //     // Vertex i
    //     float angle_i = 3.14159 * float(i) / float(segments) + 3.14159;  // Start from pi for "back" half-circle
    //     float2 offset_i = cos(angle_i) * perpendicular * half_thickness + sin(angle_i) * direction * half_thickness;
    //     float2 ndc_offset_i = offset_i * ndc_per_pixel;
    //     output.position = float4((a_ndc + ndc_offset_i) * a.w, a.z, a.w);
    //     output.side = sin(angle_i);  // -1 to 1 for dist
    //     tri.Append(output);

    //     // Center (A)
    //     output.position = a;
    //     output.side = 0.0;  // Center
    //     tri.Append(output);

    //     // Vertex i+1
    //     float angle_ip1 = 3.14159 * float(i + 1) / float(segments) + 3.14159;
    //     float2 offset_ip1 = cos(angle_ip1) * perpendicular * half_thickness + sin(angle_ip1) * direction * half_thickness;
    //     float2 ndc_offset_ip1 = offset_ip1 * ndc_per_pixel;
    //     output.position = float4((a_ndc + ndc_offset_ip1) * a.w, a.z, a.w);
    //     output.side = sin(angle_ip1);
    //     tri.Append(output);
    // }
    // tri.RestartStrip();

    // // Filled semi-circle cap at B (end): Similar, but forward direction
    // for (i = 0; i < segments; ++i)
    // {
    //     float angle_i = 3.14159 * float(i) / float(segments);
    //     float2 offset_i = cos(angle_i) * perpendicular * half_thickness + sin(angle_i) * direction * half_thickness;
    //     float2 ndc_offset_i = offset_i * ndc_per_pixel;
    //     output.position = float4((b_ndc + ndc_offset_i) * b.w, b.z, b.w);
    //     output.side = sin(angle_i);
    //     tri.Append(output);

    //     // Center (B)
    //     output.position = b;
    //     output.side = 0.0;
    //     tri.Append(output);

    //     float angle_ip1 = 3.14159 * float(i + 1) / float(segments);
    //     float2 offset_ip1 = cos(angle_ip1) * perpendicular * half_thickness + sin(angle_ip1) * direction * half_thickness;
    //     float2 ndc_offset_ip1 = offset_ip1 * ndc_per_pixel;
    //     output.position = float4((b_ndc + ndc_offset_ip1) * b.w, b.z, b.w);
    //     output.side = sin(angle_ip1);
    //     tri.Append(output);
    // }
    // tri.RestartStrip();
}

// [maxvertexcount(128)]
// void gs(lineadj GS_INPUT lines[4], inout TriangleStream<PS_INPUT> tri)
// {
//     float4 a = lines[1].position;
//     float4 b = lines[2].position;

//     // NOTE: Clip space to ndc.
//     float2 a_ndc = a.xy / a.w;
//     float2 b_ndc = b.xy / b.w;

//     float2 d = b_ndc - a_ndc;
//     float len_squared = dot(d, d);
//     if (len_squared < 1e-12) return;

//     float2 direction = d * rsqrt(len_squared);
//     float2 perpendicular = float2(-direction.y, direction.x);

//     float thickness = 2.0;
//     float2 ndc_per_pixel = 2.0 / viewport_size;
//     float2 offset_ndc = perpendicular * thickness * 0.5 * ndc_per_pixel;

//     float2 a0 = a_ndc - offset_ndc;
//     float2 a1 = a_ndc + offset_ndc;
//     float2 b0 = b_ndc - offset_ndc;
//     float2 b1 = b_ndc + offset_ndc;

//     PS_INPUT output;

//     output.position = float4(a0 * a.w, a.z, a.w);
//     output.side = -1;
//     tri.Append(output);
    
//     output.position = float4(a1 * a.w, a.z, a.w);
//     output.side = 1;
//     tri.Append(output);
    
//     output.position = float4(b0 * b.w, b.z, b.w);
//     output.side = -1;
//     tri.Append(output);
    
//     output.position = float4(b1 * b.w, b.z, b.w);
//     output.side = 1;
//     tri.Append(output);

//     tri.RestartStrip();

//     const int segments = 32;

//     for (int i = 0; i <= segments; ++i)
//     {
//         float angle = 3.14159 * float(i) / float(segments);
//         float2 offset = cos(angle) * perpendicular * thickness * 0.5 + sin(angle) * direction * thickness * 0.5;
//         float2 ndc_offset = offset * ndc_per_pixel;
//         output.position = float4((a_ndc + ndc_offset) * a.w, a.z, a.w);
//         output.side = sin(angle);
//         tri.Append(output);
//     }

//     tri.RestartStrip();
// }


float4 ps(PS_INPUT input) : SV_Target {
    float dist = abs(input.side);  // 0 = center, 1 = edge

    // Anti-aliasing: Smooth fade at edges
    float edgeFade = smoothstep(0.9, 1.0, dist);  // Adjust for sharpness (tighter = sharper)

    // Neon effect: Bright core + soft glow
    float core = 1.0 - smoothstep(0.7, 1.0, dist);  // Solid bright center
    float glow = pow(1.0 - dist, 3.0) * 0.8;  // Outer halo; tweak power for shape

    // float3 neonColor = float3(0.0, 1.0, 1.5);  // Cyan, over-saturated
    float3 neonColor = color.rgb;
    float3 color = neonColor * (core + glow);
    float alpha = (core + glow * 0.5) * (1.0 - edgeFade);  // Fade alpha at edges for AA

    return float4(color, alpha);
}

// float4 ps(PS_INPUT input) : SV_TARGET
// {
//     // return color;

//     // NOTE: 0 at center, 1 at edge
//     float d = abs(input.side);

//     // NOTE: analytic AA on the edge using derivatives
//     float w = fwidth(d);
//     float alpha = 1.0 - smoothstep(1.0 - w, 1.0, d);

//     // NOTE: neon profile: core + halo (no blur yet)
//     float core = exp2(-d*d * 40.0);
//     float halo = exp2(-d*d *  6.0);

//     float intensity = core * 1.5 + halo * 0.8;
    
//     // float3 col = float3(0.9964, 0.8431, 0.4941) * intensity;
//     float3 col = color.rgb * intensity;

//     // alpha blend (or additive if you want pure emissive)
//     return float4(col, alpha);
// }

