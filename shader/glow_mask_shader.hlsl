struct PS_INPUT
{
    float4 position : SV_POSITION;
    float side : TEXCOORD;
};

cbuffer global_glow_mask_buffer : register(b0)
{
    float3 glow_color;
    float _pad;
};

float4 ps(PS_INPUT input) : SV_TARGET
{
    float distance = abs(input.side);
    float width = fwidth(distance);
    float alpha = 1.0 - smoothstep(1.0 - width, 1.0 + width, distance);

    float intensity = 1.0; // start high (because blur will dim it)

    return float4(glow_color * intensity * alpha, 1.0);
    // return float4(glow_color, 1.0);
}
