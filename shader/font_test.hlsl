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
    float2 _pad;
};

Texture2D global_texture : register(t0);
SamplerState global_point_sampler : register(s0);

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    //float2 normal_position = 2.0 / viewport_size * input.position;
    float2 normal_position = input.position / viewport_size * 2.0f - 1.0f;
    normal_position.y = -normal_position.y;

    output.position = float4(normal_position, 0.0f, 1.0f);
    output.uv = input.uv;

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float3 glyph = global_texture.Sample(global_point_sampler, input.uv).rgb;
    // NOTE: For testing.
    float4 color = float4(glyph, 1.0f);
    
    return color;
}
