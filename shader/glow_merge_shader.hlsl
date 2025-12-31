struct PS_INPUT
{
    float4 position : SV_POSITION;
};

cbuffer global_glow_merge_param : register(b0)
{
    float2 viewport_size;
    float intensity;
    float _pad;
};

Texture2D global_glow : register(t0);   // Glow texture
SamplerState global_linear_sampler : register(s0);

float4 ps(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.position.xy / viewport_size;
    float3 blur = global_glow.Sample(global_linear_sampler, uv).rgb;
    float3 color = blur * intensity;

    return float4(color, 1.0);
}
