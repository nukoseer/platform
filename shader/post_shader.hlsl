struct PS_INPUT
{
    float4 position : SV_POSITION;
};

Texture2D    global_scene    : register(t0);   // Offscreen single-sample texture
Texture2D    global_glow     : register(t1);   // Glow texture
Texture2D    global_glow_mask: register(t2);   // Glow mask texture
SamplerState global_sampler  : register(s0);

cbuffer global_post_setting : register(b0)
{
    float2 inverse_dst_size;
    float2 inverse_src_size;
    float aspect_ratio;
    float invert;
    
    float vignette;
    float vignette_soft;
    
    float glow_intensity;
    float3 _pad;
};

PS_INPUT vs(uint id : SV_VertexID)
{
    static const float2 position[3] = { float2(-1, -1), float2(-1, 3), float2(3, -1) };

    PS_INPUT output;
    output.position = float4(position[id], 0, 1);

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.position.xy * inverse_dst_size;
    float3 color = global_scene.Sample(global_sampler, uv).rgb;
    float4 blur = global_glow.Sample(global_sampler, uv);
    float3 core_a = global_glow_mask.Sample(global_sampler, uv).a;

    float3 halo_only = blur.rgb * (1.0 - core_a);
    color = color + halo_only * glow_intensity;

    if (vignette > 0.0)
    {
        float2 difference = (uv - 0.5);
        float ratio = length(difference);
        // NOTE: Radius where darkening begins.
        // Higher the vignette begins closer to the center.
        float inner = 1.0 - vignette;
        // NOTE: Radius where it reaches the full strength.
        float outer = inner + (vignette * vignette_soft);
        float vig = smoothstep(inner, outer, ratio);
        color *= (1.0 - vig);
    }
    
    // Invert if greater than 0.0
    color = lerp(color, 1.0 - color, invert);

    return float4(color, 1.0);
}
