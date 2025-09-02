struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
};

Texture2D    global_scene    : register(t0);   // Offscreen single-sample texture
SamplerState global_sampler  : register(s0);

PS_INPUT vs(uint id : SV_VertexID)
{
    static const float2 pos[3] = { float2(-1, -1), float2(-1, 3), float2(3, -1) };
    // Map NDC to UV: [-1, 1] -> [0, 1]
    static const float2 uv[3] = { float2(0, 1), float2(0, -1), float2(2, 1) };

    PS_INPUT output;
    output.pos = float4(pos[id], 0, 1);
    output.uv = uv[id];

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float3 color = global_scene.Sample(global_sampler, input.uv).rgb;

    return float4(color, 1.0);
}
