Texture2D global_glow_mask : register(t0);
SamplerState global_sampler : register(s0);

cbuffer global_blur_buffer : register(b0)
{
    float2 inverse_dst_size;
    float2 direction;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
};

PS_INPUT vs(uint id: SV_VertexID)
{
    static const float2 position[3] = { float2(-1, -1), float2(-1, 3), float2(3, -1)};

    PS_INPUT output;
    output.position = float4(position[id], 0.0, 1.0);

    return output;
}

static const float w[5] = {0.227027, 0.194594, 0.121621, 0.054054, 0.016216};

float3 blur9(float2 uv)
{
    float2 o = direction * inverse_dst_size;
    float3 color = global_glow_mask.Sample(global_sampler, uv).rgb * w[0];

    [unroll] for (int i = 1; i < 5; ++i)
    {
        color += global_glow_mask.Sample(global_sampler, uv + o * i).rgb * w[i];
        color += global_glow_mask.Sample(global_sampler, uv - o * i).rgb * w[i];
    }

    return color;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.position.xy * inverse_dst_size;
    float4 color = float4(blur9(uv), 1);

    return color;
}
