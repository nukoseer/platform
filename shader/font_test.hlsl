struct VS_INPUT
{
    float2 position : POSITION;
    float2 uv : TEXCOORD;
    float2 size : SIZE;
    float4 color : COLOR;
    float4 clip : CLIP;
    float border_thickness : BORDERTHICKNESS;
    float has_texture : HASTEXTURE;
    uint vertex_id : SV_VertexID; 
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    nointerpolation float2 half_size : SIZE0;
    float2 sdf_sample_position : SDF;
    float4 color : COLOR;
    float4 clip : CLIP;
    nointerpolation float border_thickness : BORDERTHICKNESS;
    nointerpolation float has_texture : HASTEXTURE;
};

cbuffer global_parameters : register(b0)
{
    float2 viewport_size;
    float2 _pad;
};

Texture2D global_texture : register(t0);
SamplerState global_point_sampler : register(s0);

static const uint corner_of_vert[6] = { 0, 1, 3, 3, 2, 0 };

float linear_from_srgb_f32(float x)
{
    return x < 0.0404482362771082 ? x / 12.92 : pow(abs((x + 0.055) / 1.055), 2.4);
}

float4 linear_from_srgba(float4 v)
{
    float4 result = float4(linear_from_srgb_f32(v.x),
                           linear_from_srgb_f32(v.y),
                           linear_from_srgb_f32(v.z),
                           v.w);
  return result;
}


float rect_sdf(float2 position, float2 half_extent, float radius)
{
    float2 d = abs(position) - half_extent + radius;
    return min(max(d.x, d.y), 0.0f) + length(max(d, 0.0f)) - radius;
}


PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    float2 normal_position = input.position / viewport_size * 2.0f - 1.0f;
    normal_position.y = -normal_position.y;

    uint corner = corner_of_vert[input.vertex_id % 6];
    float2 dst_verts_pct = float2((corner >> 1) ? 1.0f : 0.0f,
                                  (corner & 1) ? 0.0f : 1.0f);
    
    output.position = float4(normal_position, 0.0f, 1.0f);
    output.uv = input.uv;
    output.half_size = input.size * 0.5f;
    output.sdf_sample_position = (2.0f * dst_verts_pct - 1.0f) * output.half_size;
    output.color = input.color;
    output.clip = input.clip;
    output.border_thickness = input.border_thickness;
    output.has_texture = input.has_texture;

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    clip(float4(input.position.xy - input.clip.xy, input.clip.zw - input.position.xy));
    
    float4 texture_sample = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (input.has_texture > 0.0f)
    {
        texture_sample = linear_from_srgba(global_texture.Sample(global_point_sampler, input.uv).rgba);
    }

    float border_sdf_t = 1.0f;
    
    if (input.border_thickness > 0.0f)
    {
        float distance = rect_sdf(input.sdf_sample_position, input.half_size - input.border_thickness, 0.0f);
        border_sdf_t = smoothstep(0.0f, 0.001f, distance);
    }

    if (border_sdf_t < 0.001f)
    {
        discard;
    }

    float4 color = texture_sample * input.color;
    color.a *= border_sdf_t;
    
    return color;
}
