struct VS_INPUT
{
     float2 pos   : POSITION;                               // these names must match D3D11_INPUT_ELEMENT_DESC array
     float3 color : COLOR;
};

struct PS_INPUT
{
  float4 pos   : SV_POSITION;                               // these names do not matter, except SV_... ones
  float4 color : COLOR;
};

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;
    
    output.pos = float4(input.pos, 0, 1);
    output.color = float4(input.color, 1);

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    return input.color;
}
