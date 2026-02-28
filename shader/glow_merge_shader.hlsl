struct PS_INPUT
{
    float4 position : SV_POSITION;
};

cbuffer global_glow_merge_param : register(b0)
{
    float2 viewport_size;
    float intensity;
    float glow;
};

Texture2D global_scene : register(t0);  // Scene texture
Texture2D global_glow : register(t1);   // Glow texture
SamplerState global_point_sampler : register(s0);
SamplerState global_linear_sampler : register(s1);

float4 ps(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.position.xy / viewport_size;
    float3 scene = global_scene.Sample(global_point_sampler, uv).rgb;
    float3 blur = global_glow.Sample(global_linear_sampler, uv).rgb;
    
    if (glow == 1.0f)
    {
         float3 color = blur * intensity;
         return float4(scene + color, 1.0);
    }
    else
    {
         float3 factor = saturate(1.0 - intensity * blur);
         return float4(scene * factor, 1.0);
    }
}
