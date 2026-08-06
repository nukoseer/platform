struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D    global_scene          : register(t0);   // Offscreen single-sample texture
SamplerState global_point_sampler  : register(s0);

PS_INPUT vs(uint id : SV_VertexID)
{
    static const float2 position[3] = { float2(-1, -1), float2(-1, 3), float2(3, -1) };
    static const float2 uv[3] = { float2(0, 1), float2(0, -1), float2(2, 1) };

    PS_INPUT output;
    output.position = float4(position[id], 0, 1);
    output.uv = uv[id];

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    float4 color = global_scene.Sample(global_point_sampler, input.uv);

    return color;
}
