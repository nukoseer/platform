cbuffer global_glow_mask_buffer : register(b0)
{
    float3 glow_color;
    float _pad;
};

float4 ps(void) : SV_TARGET
{
    return float4(glow_color, 1.0);
}
