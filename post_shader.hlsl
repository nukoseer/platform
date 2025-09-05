struct PS_INPUT
{
    float4 position : SV_POSITION;
};

Texture2D    global_scene    : register(t0);   // Offscreen single-sample texture
SamplerState global_sampler  : register(s0);

cbuffer global_post_setting : register(b0)
{
    float2 inverse_dst_size;
    float2 inverse_src_size;

    float invert;
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

    // Invert if greater than 0.0
    color = lerp(color, 1.0 - color, invert);

    return float4(color, 1.0);
}
