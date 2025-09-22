struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position   : SV_POSITION;
};

cbuffer global_settings : register(b0)
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 projection_matrix;
};

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 position = float4(input.position, 1.0);
    position = mul(world_matrix, position);
    position = mul(view_matrix, position);
    position = mul(projection_matrix, position);

    output.position = position;

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    return float4(1.0, 1.0, 1.0, 1.0);;
}
