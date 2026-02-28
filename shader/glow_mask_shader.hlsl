struct PS_INPUT
{
    float4 position : SV_POSITION;
    float side : TEXCOORD;
};

cbuffer global_glow_mask_buffer : register(b0)
{
    float3 glow_color;
    float glow;
};

float4 ps(PS_INPUT input) : SV_TARGET
{
    float distance = abs(input.side);
    float width = fwidth(distance);
    float coverage = 1.0 - smoothstep(1.0 - width, 1.0 + width, distance);

    if (glow == 1.0f)
    {
        float intensity = 1.0; // start high (because blur will dim it)
        return float4(glow_color * intensity * coverage, 1.0);
    }
    else
    {
        // Make the mask wider than the crisp line
        float halo = exp2(-distance * distance * 6.0) * coverage; // 0..1-ish
        return float4(halo, halo, halo, 1.0);
    }
}