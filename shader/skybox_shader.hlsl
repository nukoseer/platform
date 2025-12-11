struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD;
};

cbuffer global_params : register(b0)
{
    float4x4 view_no_translation_matrix;
    float4x4 projection_matrix;
    float yaw;
    float pitch;
    float shape;
    float pad;
};

float3 rotate_y(float3 position, float yaw)
{
    float cos_yaw = cos(yaw);
    float sin_yaw = sin(yaw);

    return float3(cos_yaw * position.x + sin_yaw * position.z,
                  position.y,
                  -sin_yaw * position.x + cos_yaw * position.z);
}

float3 rotate_x(float3 position, float pitch)
{
    float cos_pitch = cos(pitch);
    float sin_pitch = sin(pitch);

    return float3(position.x,
                  cos_pitch * position.y + sin_pitch * position.z,
                  -sin_pitch * position.y + cos_pitch * position.z);
}

PS_INPUT vs(VS_INPUT input)
{
    PS_INPUT output;

    float4 position = float4(input.position, 1.0);
    float4 view_space = mul(view_no_translation_matrix, position);
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    output.position = clip_space;
    output.direction = input.position;
    
    return output;
}

TextureCube  global_bg_texture : register(t0);
SamplerState global_bg_sampler : register(s0);

float4 ps(PS_INPUT input) : SV_TARGET
{
    float3 direction = normalize(input.direction) * -1.0;
    float3 yaw_direction = rotate_y(direction, radians(-yaw));
    float3 yaw_pitch_direction = rotate_x(yaw_direction, radians(-pitch));

    float3 color = global_bg_texture.Sample(global_bg_sampler, yaw_pitch_direction).rgb;

    return float4(color, max(0.1, shape));
}
