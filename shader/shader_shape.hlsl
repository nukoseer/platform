struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

cbuffer global_settings : register(b0)
{
    float4x4 world_matrix;
    float4x4 view_matrix;
    float4x4 projection_matrix;
    float4 camera_world;
};

cbuffer global_globe_param : register(b1)
{
    float shape;
    float yaw;
    float pitch;
    float _pad;
    float2 scale;
    float2 _pad1;
    float4 color;
};

float ease_in_out_back(float t)
{
    const float c1 = 1.70158;
    const float c2 = c1 * 1.525;
    
    return t < 0.5 ? 
        (pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2 :
        (pow(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;
}

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

    float lon = radians(input.position.x);
    float lat = radians(input.position.y);

    float3 globe_position = float3(cos(lat) * sin(lon), sin(lat), cos(lat) * cos(lon)) * 1.005;
    globe_position = rotate_y(globe_position, radians(yaw));
    globe_position = rotate_x(globe_position, radians(pitch));
    float3 flat_position = float3(lon * scale.x, lat * scale.y, 0.0) * 0.9995;

    float4 position = float4(lerp(flat_position, globe_position, ease_in_out_back(saturate(shape))), 1.0);
    float4 world_space = mul(world_matrix, position);
    float4 view_space = mul(view_matrix, world_space);
    
    // NOTE: Before perspective division.
    float4 clip_space = mul(projection_matrix, view_space);

    output.position = clip_space;
    output.color = color;

    return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
    return input.color;
}
