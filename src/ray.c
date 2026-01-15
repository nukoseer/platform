#include "ray.h"

static ray_result_t ray_hit_unit_sphere(ray_t ray)
{
    ray_result_t result = { .ray = ray };
    vec3 l = v3_sub(v3(0.0f, 0.0f, 0.0f), ray.origin);
    f32 tca = v3_dot(l, ray.direction);

    if (tca >= 0.0f)
    {
        f32 d_squared = v3_dot(l, l) - (tca * tca);

        if (d_squared <= 1.0f)
        {
            f32 thc = sqrtf(1.0f * 1.0f - d_squared);
            f32 t0 = tca - thc;
            f32 t1 = tca + thc;
            f32 t = (t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f);
            
            if (t >= 0.0f)
            {
                result.t = t0;
                result.hit = true;
            }
        }
    }

    return result;
}

static ray_t ray_world(f32 x, f32 y, f32 width, f32 height, f32 fov_y, vec3 camera_position, mat4x4 view_matrix)
{
    f32 normalized_x = 2.0f * (x / width) - 1.0f;
    f32 normalized_y = 1.0f - 2.0f * (y / height);

    f32 tan_y = tanf(fov_y * 0.5f * (f32)DEG2RAD);
    f32 tan_x = tan_y * (width / height);
    
    vec3 direction_view = v3_normalize(v3(normalized_x * tan_x, normalized_y * tan_y, -1.0f));
    vec3 direction_world = v3_normalize(
        v3_add(v3_mulf(view_matrix.columns[0].xyz, direction_view.x),
               v3_add(v3_mulf(view_matrix.columns[1].xyz, direction_view.y),
                      v3_mulf(view_matrix.columns[2].xyz, direction_view.z))));

    return (ray_t){ .origin = camera_position, .direction = direction_world };
}
