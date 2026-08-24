struct VS_INPUT
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer global_parameters : register(b0)
{
    float2 viewport_size;
};

Texture2D global_texture : register(t0);
SamplerState global_point_sampler : register(s0);

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    float2 normal_position = 2.0f / viewport_size * input.position;

    output.position = float4(normal_position, 0.0f, 1.0f);
    output.uv = uv;    
}

void ps(PS_INPUT input) : SV_TARGET
{
    vec3 glyph = global_texture.Sample(global_point_sampler, input.uv).rgb;
}