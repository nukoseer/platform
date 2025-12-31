
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
