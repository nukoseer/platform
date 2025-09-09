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

// static const float w[5] = {0.227027, 0.194594, 0.121621, 0.054054, 0.016216};
static const float w[5] = { 0.204164, 0.180174, 0.123832, 0.066282, 0.027631 };

float3 gaussian_blur_9tap_1d(float2 uv)
{
    float3 color = global_glow_mask.Sample(global_sampler, uv).rgb * w[0];

    float weight1 = w[1] + w[2];
    float offset1 = 1.0 + (w[2] / weight1);
    float2 direction1 = direction * inverse_dst_size * offset1;

    color += (global_glow_mask.Sample(global_sampler, uv + direction1).rgb + global_glow_mask.Sample(global_sampler, uv - direction1).rgb) * weight1;

    float weight2 = w[3] + w[4];
    float offset2 = 3.0 + (w[4] / weight2);
    float2 direction2 = direction * inverse_dst_size * offset2;

    color += (global_glow_mask.Sample(global_sampler, uv + direction2).rgb + global_glow_mask.Sample(global_sampler, uv - direction2).rgb) * weight2;

    return color;
}

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
    float4 color = float4(gaussian_blur_9tap_1d(uv), 1);

    return color;
}
