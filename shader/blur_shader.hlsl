Texture2D global_glow_mask : register(t0);
SamplerState global_linear_sampler : register(s0);

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

//static const uint weight_count = 7;
//static const float w[weight_count] = { 0.155285, 0.144214, 0.115516, 0.079805, 0.047552, 0.024438, 0.010832 };

static const uint weight_count = 3;
static const float w[weight_count] = { 0.251379, 0.221841, 0.152469 };

//static const uint weight_count = 11;
//static const float w[weight_count] = { 0.199471, 0.176033, 0.120985, 0.064759, 0.026995, 0.008764, 0.002216, 0.000436, 0.000067, 0.000008, 0.000001 };

//static const uint weight_count = 5;
//static const float w[weight_count] = { 0.204164, 0.180174, 0.123832, 0.066282, 0.027631 };

float3 gaussian_blur(float2 uv)
{
    float3 color = global_glow_mask.Sample(global_linear_sampler, uv).rgb * w[0];

    [unroll] for (uint i = 0; i < weight_count - 1; i += 2)
    {
        float weight = w[i + 1] + w[i + 2];
        float offset = i + 1.0 + (w[i + 2] / weight);
        float2 dir = direction * inverse_dst_size * offset;
        color += (global_glow_mask.Sample(global_linear_sampler, uv + dir).rgb +
                  global_glow_mask.Sample(global_linear_sampler, uv - dir).rgb) * weight;
    }
    
    return color;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float2 uv = input.position.xy * inverse_dst_size;
    float4 color = float4(gaussian_blur(uv), 1);

    return color;
}
