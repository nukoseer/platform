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
        f32 v = (f32)i / (f32)stacks;
        // f32 lat = -0.5f * (f32)PI + v * (f32)PI;
        f32 lat = -90.0f + v * 180.0f;

        for (u32 j = 0; j <= slices; ++j)
        {
            f32 u = (f32)j / (f32)slices;
            // f32 lon = -(f32)PI + u * (2.0f * (f32)PI);
            f32 lon = -180.0f + u * 360.0f;
            
            printf("%+3.12ff, %+3.12ff, %+3.12ff, ",
                   -lon, lat, 0.0f);

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
