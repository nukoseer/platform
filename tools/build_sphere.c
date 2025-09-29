#pragma once

#include <stdio.h>

#include "../src/utils.h"
#include "../src/maths.h"

static void build_uv_sphere(f32 sphere_radius, u32 stacks, u32 slices)
{
    // NOTE: stacks: latitude bands (pole-to-pole), slices: longitude
    u32 lat_count = stacks + 1;
    u32 lon_count = slices + 1;

    // NOTE: Vertices
    printf("static f32 global_sphere_vertices[] = \n{");
    for (u32 i = 0; i <= stacks; ++i)
    {
        f32 phi = (f32)i / (f32)stacks * (f32)PI;          // 0..PI
        f32 y = cosf(phi); // NOTE: Up coordinate.
        f32 radius = sinf(phi); // NOTE: Radius of circle at this latitude.
        for (u32 j = 0; j <= slices; ++j)
        {
            f32 theta = (f32)j / (f32)slices * 2.0f * (f32)PI; // 0..2PI
            f32 x = radius * cosf(theta);
            f32 z = -radius * sinf(theta);
            vec3 normal = v3_normalize(v3(x, y, z)); // NOTE: Unit outward normal.
            vec3 position = v3_mulf(normal, sphere_radius);

            printf("%+3.12ff, %+3.12ff, %+3.12ff, %+3.12ff, %+3.12ff, %+3.12ff, ",
                   position.x, position.y, position.z, normal.x, normal.y, normal.z);

            // vec2 uv  = v2((f32)j/slices, 1.0f - (f32)i/stacks);
        }
    }
    printf("\n};\n\n");

    // NOTE: Indices (two tris per quad). This winding is CCW when looking from outside.
    printf("static u16 global_sphere_indices[] = \n{");
    for (u32 i = 0; i < stacks; ++i)
    {
        for (u32 j = 0; j < slices; ++j)
        {
            u32 i0 =  i      * lon_count + j;
            u32 i1 = (i + 1) * lon_count + j;
            u32 i2 =  i      * lon_count + (j + 1);
            u32 i3 = (i + 1) * lon_count + (j + 1);

            printf("%u, %u, %u, %u, %u, %u, ", i0, i1, i2, i2, i1, i3);
        }
    }
    printf("\n};\n");
}

int main(void)
{
    build_uv_sphere(1.0, 32, 64);
    
    return 0;
}
