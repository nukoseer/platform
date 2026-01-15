#pragma once

typedef struct ray_t
{
    vec3 origin;
    vec3 direction;
} ray_t;

typedef struct ray_result_t
{
    ray_t ray;
    bool hit;
    f32 t;
} ray_result_t;

