#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <intrin.h>
#include "utils.h"
#include "platform.h"
#include "maths.h"

#include "../shader/glow_mask_pixel_shader.h"
#include "../shader/blur_vertex_shader.h"
#include "../shader/blur_pixel_shader.h"
#include "../shader/post_vertex_shader.h"
#include "../shader/post_pixel_shader.h"

#include "../shader/vertex_shader_shape.h"
#include "../shader/pixel_shader_shape.h"
#include "../shader/geometry_shader_shape.h"

#include "../shader/vertex_shader_sphere.h"
#include "../shader/pixel_shader_sphere.h"

#include "../shader/skybox_vertex_shader.h"
#include "../shader/skybox_pixel_shader.h"

#include "shape_meta_data.inl"
#include "shape_data.inl"
#include "sphere_data.inl"

#define CELL_X_COUNT    360
#define CELL_Y_COUNT    180
#define CELL_SLOT_COUNT 10

typedef struct transform_param_t
{
    mat4x4 world;
    mat4x4 view;
    mat4x4 projection;
    vec4 camera_world;
} transform_param_t;

typedef struct skybox_param_t
{
    mat4x4 view_no_translation;
    mat4x4 projection;
    f32 yaw;
    f32 pitch;
    f32 shape;
    f32 _pad;
} skybox_param_t;

typedef struct sphere_param_t
{
    vec3 _pad0;
    f32 alpha;
} sphere_param_t;

typedef struct sphere_info_t
{
    graphics_buffer_t param_buffer;
    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;

    sphere_param_t param;
} sphere_info_t;

typedef struct shape_param_t
{
    f32 shape;
    f32 yaw;
    f32 pitch;
    f32 _pad0;
    vec2 scale;
    vec2 viewport_size;
    vec4 color;
} shape_param_t;

typedef struct shape_info_t
{
    graphics_buffer_t param_buffer;
    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    graphics_buffer_t vertex_buffer_sphere;
    graphics_buffer_t index_buffer_sphere;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_shader_t geometry_shader;
    graphics_program_t program;

    shape_param_t param;
} shape_info_t;

typedef struct post_setting_t
{
    f32 inverse_dst_size[2];
    f32 inverse_src_size[2];
    f32 aspect_ratio;

    f32 invert;

    f32 vignette;
    f32 vignette_soft;

    f32 glow_intensity;
    f32 _pad[3];
} post_setting_t;

typedef struct glow_mask_setting_t
{
    f32 glow_color[4];
} glow_mask_setting_t;

typedef struct glow_blur_setting_t
{
    f32 inverse_dst_size[2];
    f32 direction[2];
} glow_blur_setting_t;

typedef struct camera_t
{
    vec3 position;
    vec3 target;
    vec3 up;

    f32 fov_y; // NOTE: Radians.
    f32 aspect_ratio;

    mat4x4 view;
    mat4x4 projection;

    mat4x4 view_no_translation;
} camera_t;

typedef struct skybox_vertex_t
{
    vec3 position;
} skybox_vertex_t;

typedef struct game_t
{
    graphics_texture_t offscreen_scene;
    graphics_texture_t offscreen_depth;
    graphics_target_t offscreen_target;
    
    graphics_pipeline_t default_pipeline;
    graphics_pipeline_t d_test_write_pipeline;
    graphics_pipeline_t d_test_pipeline;

    graphics_sampler_t point_sampler;
    graphics_sampler_t linear_sampler;

    camera_t camera;
    transform_param_t transform_param;
    graphics_buffer_t transform_buffer;
    graphics_shader_t vertex_shader_3d;

    sphere_info_t sphere_info;
    shape_info_t shape_info;

    glow_mask_setting_t glow_mask_setting;
    graphics_buffer_t glow_buffer;
    graphics_texture_t glow_mask;
    graphics_texture_t glow_a;
    graphics_texture_t glow_b;
    graphics_target_t glow_mask_target;
    graphics_target_t glow_a_target;
    graphics_target_t glow_b_target;
    graphics_shader_t glow_pixel_shader;
    graphics_program_t glow_program;

    glow_blur_setting_t glow_blur_setting;
    graphics_buffer_t blur_buffer;
    graphics_shader_t blur_vertex_shader;
    graphics_shader_t blur_pixel_shader;
    graphics_program_t blur_program;

    graphics_buffer_t post_buffer;
    graphics_shader_t post_vertex_shader;
    graphics_shader_t post_pixel_shader;
    graphics_program_t post_program;
    post_setting_t post_setting;

    graphics_texture_t skybox_texture;
    graphics_buffer_t skybox_param_buffer;
    graphics_buffer_t skybox_vertex_buffer;
    graphics_buffer_t skybox_index_buffer;
    graphics_shader_t skybox_vertex_shader;
    graphics_shader_t skybox_pixel_shader;
    graphics_program_t skybox_program;

#if FONT_ENABLE
    graphics_2d_font_t font;
    graphics_2d_font_color_t font_color;
#endif

    u8* cells;
    u8 country_index;

    // NOTE: 1.0f is globe map, 0.0f flat map.
    f32 shape_value;
    f32 shape_speed;
    f32 shape_direction;
} game_t;

 #pragma pack(push, 1)

typedef struct bmp_header_t
{
    u16 file_type;
    u32 file_size;
    u16 reserved_1;
    u16 reserved_2;
    u32 bitmap_offset;
    u32 size;
    u32 width;
    u32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 size_of_bitmap;
    i32 horizontal_resolution;
    i32 vertical_resolution;
    u32 colors_used;
    u32 colors_important;

    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
} bmp_header_t;

#pragma pack(pop)

typedef struct bmp_image_t
{
    u8* data;
    u32 width;
    u32 height;
    u32 pitch;
} bmp_image_t;

static bmp_image_t load_bmp_image(const io_t* io, const char* file_name)
{
    bmp_image_t result = { 0 };
    io_file_read_result_t read_result = io->read_file(file_name);

    if (read_result.size != 0)
    {
        bmp_header_t* header = (bmp_header_t*)read_result.data;
        u32* pixels = (u32*)((u8*)read_result.data + header->bitmap_offset);
        u32* memory = malloc(header->width * header->height * header->bits_per_pixel);

        memset(memory, 0, header->width * header->height * header->bits_per_pixel);

        result.data = (u8*)memory;
        result.width = header->width;
        result.height = header->height;

        assert(header->compression == 3 && "[BMP] Unsupported compression format");
        // NOTE: If you are using this generically for some reason,
        // please remember that BMP files can go in either direction and
        // the height will be negative for top-down.
        // Also, there can be compression etc., etc...
        // Don't think this is complete BMP loading code because it is not.

        // NOTE: Byter order in memory is determined by the header itself.
        // So we have to read out the masks and convert the pixels ourselves.

        u32 red_mask = header->red_mask;
        u32 green_mask = header->green_mask;
        u32 blue_mask = header->blue_mask;
        u32 alpha_mask = ~(red_mask | green_mask | blue_mask);
        
        u32 red_index = 0;
        u32 green_index = 0;
        u32 blue_index = 0;
        u32 alpha_index = 0;
    
        bool red_found = _BitScanForward((unsigned long*)&red_index, red_mask);
        bool green_found = _BitScanForward((unsigned long*)&green_index, green_mask);
        bool blue_found = _BitScanForward((unsigned long*)&blue_index, blue_mask);
        bool alpha_found = _BitScanForward((unsigned long*)&alpha_index, alpha_mask);
    
        assert(red_found   && "[BMP] Invalid red channel mask.");
        assert(green_found && "[BMP] Invalid green channel mask.");
        assert(blue_found  && "[BMP] Invalid blue channel mask.");
        assert(alpha_found && "[BMP] Invalud alpha channel mask.");
    
        for (u32 y = 0; y < header->height; ++y)
        {
            u32 flipped_y = (header->height - 1) - y;
            
            for (u32 x = 0; x < header->width; ++x)
            {
    	        u32 color = pixels[y * header->width + x];

    	        f32 r = (f32)((color & red_mask) >> red_index);
    	        f32 g = (f32)((color & green_mask) >> green_index);
    	        f32 b = (f32)((color & blue_mask) >> blue_index);
    	        f32 a = (f32)((color & alpha_mask) >> alpha_index);

    	        memory[flipped_y * header->width + x] = (((u32)(a + 0.5f) << 24) |
                                                         ((u32)(b + 0.5f) << 16) |
                                                         ((u32)(g + 0.5f) << 8)  |
                                                         ((u32)(r + 0.5f) << 0));
            }
        }
    }
    else
    {
        assert(!"[BMP] File could not read.");
    }

    result.pitch = result.width * 4;
    
#if 0
    result.memory = (u8*)result.memory + result.pitch * (result.height - 1);
    result.pitch = -result.pitch;
#endif

    io->release_file_memory(read_result.data);

    return result;
}

static void resize_offscreen_buffer(platform_t* platform, game_t* game)
{
    graphics_t* graphics = platform->graphics;
    bool resized = platform->resized;
    bool is_valid = graphics->is_valid_target(game->offscreen_target);

    if (is_valid && resized)
    {
        graphics->delete_target(game->offscreen_target);
        graphics->delete_target(game->glow_mask_target);
        graphics->delete_target(game->glow_a_target);
        graphics->delete_target(game->glow_b_target);
        graphics->delete_texture_2d(game->offscreen_scene);
        graphics->delete_texture_2d(game->offscreen_depth);
        graphics->delete_texture_2d(game->glow_mask);
        graphics->delete_texture_2d(game->glow_a);
        graphics->delete_texture_2d(game->glow_b);
    }

    if (!is_valid || resized)
    {
        game->offscreen_scene = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = platform->width,
            .height = platform->height,
        }, 0, 0);

        game->offscreen_depth = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_D24_UNORM_S8_UINT,
            .bind = BIND_DEPTH_STENCIL,
            .width = platform->width,
            .height = platform->height,
        }, 0, 0);

        game->glow_mask = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(platform->width * 0.5f),
            .height = (u32)(platform->height * 0.5f),
        }, 0, 0);

        game->glow_a = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(platform->width * 0.5f),
            .height = (u32)(platform->height * 0.5f),
        }, 0, 0);

        game->glow_b = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(platform->width * 0.5f),
            .height = (u32)(platform->height * 0.5f),
        }, 0, 0);

        // NOTE: Create RenderTargetView for offscreen_scene to render.
        game->offscreen_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->offscreen_scene, .depth = game->offscreen_depth });
        game->glow_mask_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->glow_mask });
        game->glow_a_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->glow_a });
        game->glow_b_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->glow_b });
    }
}

static inline mat4x4 rotate_x(f32 angle)
{
    f32 rad = angle * (f32)DEG2RAD;

    mat4x4 rotation_matrix =
    {
        .columns =
        {
            [0] = v4(1.0f, 0.0f,       0.0f,      0.0f),
            [1] = v4(0.0f, cosf(rad), sinf(rad), 0.0f),
            [2] = v4(0.0f, -sinf(rad),  cosf(rad), 0.0f),
            [3] = v4(0.0f, 0.0f,       0.0f,      1.0f)
        },
    };

    return rotation_matrix;
}

static inline mat4x4 rotate_y(f32 angle)
{
    f32 rad = angle * (f32)DEG2RAD;

    mat4x4 rotation_matrix =
    {
        .columns =
        {
            [0] = v4(cosf(rad), 0.0f, sinf(rad), 0.0f),
            [1] = v4(0.0f,      1.0f,  0.0f,      0.0f),
            [2] = v4(-sinf(rad), 0.0f,  cosf(rad), 0.0f),
            [3] = v4(0.0f,      0.0f,  0.0f,      1.0f)
        },
    };

    return rotation_matrix;
}

static inline mat4x4 rotate_z(f32 angle)
{
    f32 rad = angle * (f32)DEG2RAD;

    mat4x4 rotation_matrix =
    {
        .columns =
        {
            [0] = v4(cosf(rad),  sinf(rad),  0.0f, 0.0f),
            [1] = v4(-sinf(rad),   cosf(rad),  0.0f, 0.0f),
            [2] = v4(0.0f,        0.0f,       1.0f, 0.0f),
            [3] = v4(0.0f,        0.0f,       0.0f, 1.0f)
        },
    };


    return rotation_matrix;
}

// NOTE: View matrix, right-handed, -z
static inline mat4x4 view_matrix(vec3 reference_up, vec3 from, vec3 to)
{
    vec3 forward = v3_normalize(v3_sub(to, from));
    f32 cos_up = fabsf(v3_dot(reference_up, forward));
    vec3 up_hint = (cos_up > 0.999f) ? v3(0.0f, 0.0f, 1.0f) : reference_up;
    vec3 right = v3_normalize(v3_cross(forward, up_hint));
    vec3 up = v3_cross(right, forward);

    mat4x4 result =
    {
        .columns =
        {
            [0] = v4v(right, 0.0f),
            [1] = v4v(up, 0.0f),
            [2] = v4v(v3_neg(forward), 0.0f),
            [3] = v4(0.0f, 0.0f, 0.0f, 1.0f),
        },
    };

    result.columns[3].x = -v3_dot(right, from);
    result.columns[3].y = -v3_dot(up, from);
    result.columns[3].z = v3_dot(forward, from);
    result.columns[3].w = 1.0f;

    return result;
}

// NOTE: Camera space to clip space (no perspective division)
// xc: -wc <= xc <= wc
// yc: -wc <= yc <= wc
// zc:   0 <= zc <= wc -> D3D/Vulkan
// wc:  wc
static inline mat4x4 perspective_projection(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    mat4x4 result =
    {
        .columns =
        {
            [0] = v4(2.0f * near / (right - left), 0.0f, 0.0f, 0.0f),
            [1] = v4(0.0f, 2.0f * near / (top - bottom), 0.0f, 0.0f),
            [2] = v4((right + left) / (right - left), (top + bottom) / (top - bottom), -far / (far - near), -1.0f),
            [3] = v4(0.0f, 0.0f, -(far * near) / (far - near), 0.0f),
        },
    };
    
    return result;
}

static inline mat4x4 perspective_projection_fov_y(f32 fov_y, f32 aspect_ratio, f32 near, f32 far)
{
    f32 tangent = tanf(fov_y * 0.5f * (f32)DEG2RAD);
    f32 top = near * tangent;
    f32 right = top * aspect_ratio;

    mat4x4 result =
    {
        .columns =
        {
            [0] = v4(near / right, 0.0f, 0.0f, 0.0f),
            [1] = v4(0.0f, near / top, 0.0f, 0.0f),
            [2] = v4(0.0f, 0.0f, -far / (far - near), -1.0f),
            [3] = v4(0.0f, 0.0f, -(far * near) / (far - near), 0.0f),
        },
    };
    
    return result;
}

static inline mat4x4 orthographic_projection(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far)
{
    mat4x4 result =
    {
        .columns =
        {
            [0] = v4(2.0f / (right - left), 0.0f, 0.0f, 0.0f),
            [1] = v4(0.0f, 2.0f / (top - bottom), 0.0f, 0.0f),
            [2] = v4(0.0f, 0.0f, 1.0f / (far - near), 0.0f),
            [3] = v4(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -near / (far - near), 1.0f),
        },
    };

    return result;
}

static void init_sphere(const graphics_t* graphics, sphere_info_t* sphere_info)
{
    sphere_info->param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(sphere_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    sphere_info->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_sphere_vertices,
        .size = sizeof(global_sphere_vertices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    sphere_info->index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_sphere_indices,
        .size = array_count(global_sphere_indices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_INDEX_BUFFER,
        .index_format = array_count(global_sphere_indices) > 0xFFFF ? FORMAT_R32_UINT : FORMAT_R16_UINT,
    });

    sphere_info->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_sphere,
        .bytecode_size = sizeof(vshader_sphere),
        .stage = STAGE_VERTEX_SHADER,
    });

    sphere_info->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_sphere,
        .bytecode_size = sizeof(pshader_sphere),
        .stage = STAGE_PIXEL_SHADER,
    });

    sphere_info->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = sphere_info->vertex_shader,
        .pixel_shader = sphere_info->pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });
}

static void init_shape(const graphics_t* graphics, shape_info_t* shape_info)
{
    shape_info->param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(shape_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    shape_info->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_shape_points,
        .size = sizeof(global_shape_points),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    shape_info->index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_shape_indices,
        .size = array_count(global_shape_indices),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_INDEX_BUFFER,
        .index_format = array_count(global_shape_indices) > 0xFFFF ? FORMAT_R32_UINT : FORMAT_R16_UINT,
    });

    shape_info->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_shape,
        .bytecode_size = sizeof(vshader_shape),
        .stage = STAGE_VERTEX_SHADER,
    });

    shape_info->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_shape,
        .bytecode_size = sizeof(pshader_shape),
        .stage = STAGE_PIXEL_SHADER,
    });

    shape_info->geometry_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = gshader_shape,
        .bytecode_size = sizeof(gshader_shape),
        .stage = STAGE_GEOMETRY_SHADER,
    });

    shape_info->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = shape_info->vertex_shader,
        .pixel_shader = shape_info->pixel_shader,
        .geometry_shader = shape_info->geometry_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });
}

// NOTE: [-180, 180] -> [0, 360)
static inline u32 x_index_from_lon(f32 lon)
{
    f32 u = (lon + 180.0f) / 360.0f;
    u32 x = (u32)floorf(u * 360.0f);
    x = clamp_u32(0, x, CELL_X_COUNT - 1);

    return x;
}

// NOTE: [-90, 90] -> [0, 180)
static inline u32 y_index_from_lat(f32 lat)
{
    f32 v = (lat + 90.0f) / 180.0f;
    u32 y = (u32)floor(v * 180.0f);
    y = clamp_u32(0, y, CELL_Y_COUNT - 1);

    return y;
}

static inline u32 cell_index(u32 x, u32 y)
{
    u32 result = (y * CELL_X_COUNT + x) * CELL_SLOT_COUNT;

    return result;
}

static inline void cell_insert(u8* cells, u8 country_index, u32 x, u32 y)
{
    u32 index = cell_index(x, y);

    for (u32 slot_index = 0; slot_index < CELL_SLOT_COUNT; ++slot_index)
    {
        u8* country_id = cells + index + slot_index;

        if (*country_id == country_index + 1)
        {
            break;
        }
        else if (*country_id != 0)
        {
            continue;
        }
        else
        {
            *country_id = country_index + 1;
            break;
        }
    }
}

static void cell_insert_country(u8* cells, u8 country_index, f32 lon_min, f32 lon_max, f32 lat_min, f32 lat_max)
{
    u32 x_min = x_index_from_lon(lon_min);
    u32 x_max = x_index_from_lon(lon_max);
    u32 y_min = y_index_from_lat(lat_min);
    u32 y_max = y_index_from_lat(lat_max);

    for (u32 y = y_min; y < y_max; ++y)
    {
        for (u32 x = x_min; x < x_max; ++x)
        {
            cell_insert(cells, country_index, x, y);
        }
    }
}

static f32 global_earth_yaw;
static f32 global_earth_pitch;
static bool global_earth_reset;

static bool inside_country(const vec3* points, u32 point_count, f32 lon, f32 lat)
{
    bool inside = false;

    for (u32 i = 0, j = point_count - 1; i < point_count; j = i++)
    {
        f32 xi = points[i].x;
        f32 yi = points[i].y;
        f32 xj = points[j].x;
        f32 yj = points[j].y;

        bool hit = ((yi > lat) != (yj > lat)) && (lon < (xj - xi) * (lat - yi) / (yj - yi) + xi);

        inside ^= hit;
    }

    return inside;
}

static u8 cell_get_country_id(const u8* cells, f32 lon, f32 lat)
{
    u32 result = 0;
    u32 x = x_index_from_lon(lon);
    u32 y = y_index_from_lat(lat);
    u32 index = cell_index(x, y);

    for (u32 slot_index = 0; slot_index < CELL_SLOT_COUNT; ++slot_index)
    {
        u32 country_id = cells[index + slot_index];
        
        if (country_id != 0)
        {
            u32 country_index = country_id - 1;
            u16 part_offset = global_shape_part_offset_counts[country_index][0];
            u16 part_count = global_shape_part_offset_counts[country_index][1];

            for (u32 part_index = 0; part_index < part_count; ++part_index)
            {
                u32 start = global_shape_parts[part_offset + part_index][0];
                u32 end = global_shape_parts[part_offset + part_index][1];

                if (inside_country(global_shape_points + start, end - start, lon, lat))
                {
                    result = country_id;
                    break;
                }       
            }
        }
        else
        {
            break;
        }
    }

    return result;
}

static u8 cell_get_country(const u8* cells, f32 lon, f32 lat)
{
    u8 country_id = cell_get_country_id(cells, lon, lat);
    u8 country_index = 0xFF;
    
    if (country_id)
    {
        country_index = country_id - 1;
    }

    return country_index;
}

typedef struct ray_t
{
    vec3 origin;
    vec3 direction;
} ray_t;

static ray_t make_world_ray(f32 x, f32 y, f32 width, f32 height, f32 fov_y, vec3 camera_position, mat4x4 view_matrix)
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

static bool ray_unit_sphere(ray_t ray, f32* t_out)
{
    bool hit = false;
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
                *t_out = t0;
                hit = true;
            }
        }
    }

    return hit;
}

static inline void init_camera(camera_t* camera, vec3 position, vec3 target, f32 fov_y, f32 aspect_ratio)
{
    *camera = (camera_t)
    {
        .position = position,
        .target = target,
        .up = v3(0.0f, 1.0f, 0.0f), // NOTE: Reference up.
            
        .fov_y = fov_y,
        .aspect_ratio = aspect_ratio,
    };
}

static inline void update_camera(camera_t* camera)
{
    camera->view = view_matrix(camera->up, camera->position, camera->target);
    camera->projection = perspective_projection_fov_y(camera->fov_y, camera->aspect_ratio, 0.0001f, 100.0f);
    camera->view_no_translation = camera->view;
    camera->view_no_translation.columns[3].x = 0.0f;
    camera->view_no_translation.columns[3].y = 0.0f;
    camera->view_no_translation.columns[3].z = 0.0f;
}

init_function(init)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    io_t* io = platform->io;
    game_t* game = (game_t*)memory->permanent;

    init_camera(&game->camera, v3(0.0f, 0.0f, 2.5f), v3(0.0f, 0.0f, 0.0f),
                60.0f, (f32)platform->width / (f32)platform->height);
    init_sphere(graphics, &game->sphere_info);
    init_shape(graphics, &game->shape_info);

    game->shape_info.vertex_buffer_sphere = game->sphere_info.vertex_buffer;
    game->shape_info.index_buffer_sphere = game->sphere_info.index_buffer;

    game->shape_value = 1.0f;
    game->shape_direction = 1.0f;

    game->default_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t){ .alpha_blend_enable = true, });

    game->d_test_write_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = true,
        .depth_test = true,
        .depth_write = true,
        .alpha_blend_enable = true,
    });

    game->d_test_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = false,
        .depth_test = true,
        .depth_write = false,
        .alpha_blend_enable = true,
    });

    game->linear_sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_LINEAR,
        .address_u = TEXTURE_ADDRESS_CLAMP,
        .address_v = TEXTURE_ADDRESS_CLAMP,
        .address_w = TEXTURE_ADDRESS_CLAMP,
    });

    game->point_sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_POINT,
        .address_u = TEXTURE_ADDRESS_WRAP,
        .address_v = TEXTURE_ADDRESS_WRAP,
        .address_w = TEXTURE_ADDRESS_WRAP,
    });

    game->transform_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(transform_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->glow_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = 16,
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->glow_pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = glow_mask_pshader,
        .bytecode_size = sizeof(glow_mask_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->glow_program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->shape_info.vertex_shader,
        .pixel_shader = game->glow_pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });

    game->blur_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = 16,
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->blur_vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = blur_vshader,
        .bytecode_size = sizeof(blur_vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    game->blur_pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = blur_pshader,
        .bytecode_size = sizeof(blur_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->blur_program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->blur_vertex_shader,
        .pixel_shader = game->blur_pixel_shader,
        // NOTE: No input layout.
        .attributes = 0,
        .attribute_count = 0,
    });

    // NOTE: Post shaders and pipeline.
    game->post_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(post_setting_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->post_vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = post_vshader,
        .bytecode_size = sizeof(post_vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    game->post_pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = post_pshader,
        .bytecode_size = sizeof(post_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->post_program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->post_vertex_shader,
        .pixel_shader = game->post_pixel_shader,
        // NOTE: No input layout.
        .attributes = 0,
        .attribute_count = 0,
    });

#if FONT_ENABLE
    game->font = graphics->create_font("Consolas", 12);
    // game->font_color = graphics->create_font_color(0.8313f, 0.0f, 0.4705f, 1.0f);
    // game->font_color = graphics->create_font_color(0.38f, 0.38f, 0.38f, 1.0f);
    game->font_color = graphics->create_font_color(0.6862f, 0.6862f, 0.6862f, 1.0f);
#endif

    io->release_file_memory(io->read_file("..\\src\\game.c").data);

    u32 total_part_count = global_shape_part_offset_counts[array_count(global_shape_part_offset_counts) - 1][0] + 1;
    f32* part_outlines = calloc(1, total_part_count * sizeof(f32) * 4);
    u32 part_outline_count = 0;

    for (u8 country_index = 0; country_index < array_count(global_shape_part_offset_counts); ++country_index)
    {
        u32 part_offset = global_shape_part_offset_counts[country_index][0];
        u32 part_count = global_shape_part_offset_counts[country_index][1];

        for (u32 part_index = 0; part_index < part_count; ++part_index)
        {
            u32 start = global_shape_parts[part_offset + part_index][0];
            u32 end = global_shape_parts[part_offset + part_index][1];

            f32 lon_min = 1000.0f;
            f32 lon_max = -1000.0f;
            f32 lat_min = 1000.0f;
            f32 lat_max = -1000.0f;

            for (u32 k = start; k < end; ++k)
            {
                vec3 point = global_shape_points[k];

                if (point.x < lon_min)
                {
                    lon_min = point.x;
                }

                if (point.y < lat_min)
                {
                    lat_min = point.y;
                }

                if (point.x > lon_max)
                {
                    lon_max = point.x;
                }

                if (point.y > lat_max)
                {
                    lat_max = point.y;
                }
            }

            part_outlines[part_outline_count + 0] = lon_min;
            part_outlines[part_outline_count + 1] = lon_max;
            part_outlines[part_outline_count + 2] = lat_min;
            part_outlines[part_outline_count + 3] = lat_max;
            part_outline_count += 4;
        }
    }

    assert(total_part_count * 4 == part_outline_count);

    game->cells = calloc(1, CELL_X_COUNT * CELL_Y_COUNT * CELL_SLOT_COUNT);
    f32* outlines = part_outlines;

    for (u8 country_index = 0; country_index < array_count(global_shape_part_offset_counts); ++country_index)
    {
        for (u32 part_index = 0; part_index < global_shape_part_offset_counts[country_index][1]; ++part_index)
        {
            f32 lon_min = outlines[0];
            f32 lon_max = outlines[1];
            f32 lat_min = outlines[2];
            f32 lat_max = outlines[3];
            outlines += 4;
            
            cell_insert_country(game->cells, country_index, lon_min, lon_max, lat_min, lat_max);
        }
    }

    free(part_outlines);

    // TODO: Probably we can be more clever.
    // This affects start up time drastically of course...
    // We also do not support any other image type. BMPs are pretty large files.
    bmp_image_t px_space = load_bmp_image(io, "..\\resources\\px_bmp.bmp");
    bmp_image_t nx_space = load_bmp_image(io, "..\\resources\\nx_bmp.bmp");
    bmp_image_t py_space = load_bmp_image(io, "..\\resources\\py_bmp.bmp");
    bmp_image_t ny_space = load_bmp_image(io, "..\\resources\\ny_bmp.bmp");
    bmp_image_t pz_space = load_bmp_image(io, "..\\resources\\pz_bmp.bmp");
    bmp_image_t nz_space = load_bmp_image(io, "..\\resources\\nz_bmp.bmp");

    void* skybox_data[6]  = { px_space.data,  nx_space.data,  py_space.data,  ny_space.data,  pz_space.data,  nz_space.data };
    u32 skybox_pitches[6] = { px_space.pitch, nx_space.pitch, py_space.pitch, ny_space.pitch, pz_space.pitch, nz_space.pitch };

    game->skybox_texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
    {
        .format = FORMAT_R8G8B8A8_UNORM_SRGB,
        .bind = BIND_SHADER_RESOURCE,
        .width = (u32)(px_space.width),
        .height = (u32)(px_space.height),
        .array_size = array_count(skybox_data),
        .misc = MISC_TEXTURE_CUBE,
    }, skybox_data, skybox_pitches);

    for (u32 skybox_index = 0; skybox_index < array_count(skybox_data); ++skybox_index)
    {
        // TODO: malloc / free was never the right way... 
        free(skybox_data[skybox_index]);
    }
    
    // NOTE: Unit cube centered at origin.
    static const skybox_vertex_t skybox_vertices[] =
    {
        // +X
        { +1.0f, -1.0f, -1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, +1.0f, +1.0f }, { +1.0f, +1.0f, -1.0f },
        // -X
        { -1.0f, -1.0f, +1.0f }, { -1.0f, -1.0f, -1.0f }, { -1.0f, +1.0f, -1.0f }, { -1.0f, +1.0f, +1.0f },
        // +Y
        { -1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { +1.0f, +1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f },
        // -Y
        { -1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f }, { +1.0f, -1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f },
        // +Z
        { -1.0f, -1.0f, +1.0f }, { -1.0f, +1.0f, +1.0f }, { +1.0f, +1.0f, +1.0f }, { +1.0f, -1.0f, +1.0f },
        // -Z
        { +1.0f, -1.0f, -1.0f }, { +1.0f, +1.0f, -1.0f }, { -1.0f, +1.0f, -1.0f }, { -1.0f, -1.0f, -1.0f },
    };
    
    static const u16 skybox_indices[] =
    {
         0,  1, 2,    0,  2,  3, // +X
         4,  5, 6,    4,  6,  7, // -X
         8,  9, 10,   8, 10, 11, // +Y
        12, 13, 14,  12, 14, 15, // -Y
        16, 17, 18,  16, 18, 19, // +Z
        20, 21, 22,  20, 22, 23  // -Z
    };

    game->skybox_param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(skybox_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->skybox_vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = skybox_vertices,
        .size = sizeof(skybox_vertices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    game->skybox_index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = skybox_indices,
        .size = array_count(skybox_indices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_INDEX_BUFFER,
        .index_format = FORMAT_R16_UINT,
    });

    game->skybox_vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = skybox_vshader,
        .bytecode_size = sizeof(skybox_vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    game->skybox_pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = skybox_pshader,
        .bytecode_size = sizeof(skybox_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });
    
    game->skybox_program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->skybox_vertex_shader,
        .pixel_shader = game->skybox_pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });
}

static vec2 xy_to_lon_lat(f32 x, f32 y, f32 width, f32 height)
{
    vec2 result = { 0 };
    vec2 normalized_position = v2(x / width, y / height);
    // NOTE: Map to lon(-180, 180), lat(-90, 90).
    result.x = (2.0f * normalized_position.x - 1.0f) * 180.0f;
    result.y = (1.0f - 2.0f * normalized_position.y) * 90.0f;

    return result;
}

static vec2 ray_to_lon_lat(ray_t ray)
{
    vec2 result = { 0 };
    f32 t = 0.0f;

    if (ray_unit_sphere(ray, &t))
    {
        vec3 hit_position = v3_normalize(v3_add(ray.origin, v3_mulf(ray.direction, t)));

        // NOTE: Undo pitch angle.
        f32 pitch_rad = -global_earth_pitch * (f32)DEG2RAD;
        vec3 position = (vec3)
        {
            .x = hit_position.x,
            .y = cosf(pitch_rad) * hit_position.y + sinf(pitch_rad) * hit_position.z,
            .z = -sinf(pitch_rad) * hit_position.y + cosf(pitch_rad) * hit_position.z,
        };
        
        f32 lon = atan2f(position.x, position.z);
        f32 lat = asinf(position.y);

        // NOTE: Undo yaw angle.
        lon -= global_earth_yaw * (f32)DEG2RAD;
        
        while (lon > (f32)PI)  lon -= 2.0f * (f32)PI;
        while (lon < (f32)-PI) lon += 2.0f * (f32)PI;

        result.x = lon / (f32)DEG2RAD;
        result.y = lat / (f32)DEG2RAD;
    }

    return result;
}

update_function(update)
{
    memory_t* memory = platform->memory;
    input_t* input = platform->input;
    game_t* game = (game_t*)memory->permanent;
    camera_t* camera = &game->camera;

    if (input->mouse_position.z != 0.0f)
    {
        camera->position.z += 3.0f * platform->delta_time * -input->mouse_position.z;
    }

    update_camera(camera);

    if (input->keys[KEY_MOUSE_LEFT].action == KEY_ACTION_PRESS && game->shape_value == 1.0f)
    {
        global_earth_yaw += 3.0f * input->mouse_delta.x * platform->delta_time;
        global_earth_pitch += 3.0f * -input->mouse_delta.y * platform->delta_time;
    }

    global_earth_pitch = clamp(-90.0f, global_earth_pitch, 90.0f);

    vec2 xy_lon_lat = xy_to_lon_lat(input->mouse_position.x, input->mouse_position.y,
                                    (f32)platform->width, (f32)platform->height);
    
    ray_t mouse_ray = make_world_ray(input->mouse_position.x, input->mouse_position.y,
                                     (f32)platform->width, (f32)platform->height, camera->fov_y,
                                     camera->position, camera->view);
    
    vec2 ray_lon_lat = ray_to_lon_lat(mouse_ray);

    if (input->keys[KEY_R].action == KEY_ACTION_RELEASE)
    {
        global_earth_reset = !global_earth_reset;
    }

    f32 speed = 200.0f * platform->delta_time;
    
    if (global_earth_reset)
    {
        if (global_earth_yaw != 0.0f)
        {
            if (global_earth_yaw > 0.0f && global_earth_yaw < speed)
            {
                global_earth_yaw = 0.0f;
            }
        
            if (global_earth_yaw > 0.0f)
            {
                global_earth_yaw -= speed;
            }

            if (global_earth_yaw < 0.0f && global_earth_yaw > -speed)
            {
                global_earth_yaw = 0.0f;
            }
        
            if (global_earth_yaw < 0.0f)
            {
                global_earth_yaw += speed;
            }
        }
        if (global_earth_pitch != 0.0f)
        {
            if (global_earth_pitch > 0.0f && global_earth_pitch < speed)
            {
                global_earth_pitch = 0.0f;
            }
        
            if (global_earth_pitch > 0.0f)
            {
                global_earth_pitch -= speed;
            }

            if (global_earth_pitch < 0.0f && global_earth_pitch > -speed)
            {
                global_earth_pitch = 0.0f;
            }
        
            if (global_earth_pitch < 0.0f)
            {
                global_earth_pitch += speed;
            }
        }

        if (global_earth_yaw == 0.0f && global_earth_pitch == 0.0f)
        {
            global_earth_reset = false;
        }
    }

    game->shape_direction = (input->keys[KEY_T].action == KEY_ACTION_RELEASE ?
                             -game->shape_direction : game->shape_direction);
    game->shape_speed = 1.0f * platform->delta_time * game->shape_direction;
    game->shape_value = clamp(0.0f, game->shape_value + game->shape_speed, 1.0f);

    if (game->shape_value == 0.0f || (game->shape_value == 1.0f && (ray_lon_lat.x != 0.0f || ray_lon_lat.y != 0.0f)))
    {
        f32 lon = lerp(xy_lon_lat.x, game->shape_value, ray_lon_lat.x);
        f32 lat = lerp(xy_lon_lat.y, game->shape_value, ray_lon_lat.y);

        game->country_index = cell_get_country(game->cells, lon, lat);

        if (game->country_index != 0xFF && game->country_index < array_count(global_shape_country_names))
        {
            fprintf(stderr, "\rx: %f, y: %f, lon: %f, lat: %f, yaw: %f, pitch: %f, country: %s, id: %u",
                    input->mouse_position.x, input->mouse_position.y,
                    lon, lat, global_earth_yaw, global_earth_pitch,
                    global_shape_country_names[game->country_index], game->country_index);
        }
    }
}

render_function(render)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    game_t* game = (game_t*)memory->permanent;
    camera_t* camera = &game->camera;

    resize_offscreen_buffer(platform, game);

    game->transform_param.world = m4x4d(1.0f);
    game->transform_param.view = camera->view;
    game->transform_param.projection = camera->projection;
    game->transform_param.camera_world = v4v(camera->position, 0.0f);

    graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t)
    {
        .clear_color = true, .clear_rgba = { 0.005f, 0.005f, 0.005f, 1.0f },
        .clear_depth = true, .clear_depth_value = 1.0f
    });
    {
        skybox_param_t skybox_param =
        {
            .view_no_translation = camera->view_no_translation,
            .projection = camera->projection,
            .yaw = global_earth_yaw,
            .pitch = global_earth_pitch,
            .shape = game->shape_value,
        };

        graphics->update_buffer(game->skybox_param_buffer, &skybox_param, 0, sizeof(skybox_param_t));
        graphics->set_buffer(game->skybox_param_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(game->skybox_param_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_vertex_buffer(game->skybox_vertex_buffer, 0, sizeof(skybox_vertex_t), 0);
        graphics->set_index_buffer(game->skybox_index_buffer, 0);
        graphics->set_program(game->skybox_program);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->skybox_texture, 1, 0);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
        graphics->set_pipeline(game->default_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, game->skybox_index_buffer.size, 0, 0);
    }
    graphics->end_pass();
    
    graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t)
    {
        .clear_color = false, .clear_depth = false,
    });
    {
        sphere_info_t* sphere_info = &game->sphere_info;
        sphere_info->param.alpha = game->shape_value;
        graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
        graphics->update_buffer(sphere_info->param_buffer, &sphere_info->param, 0, sizeof(sphere_info->param));
        graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(game->transform_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_buffer(sphere_info->param_buffer, STAGE_VERTEX_SHADER, 1, 0, 0);
        graphics->set_buffer(sphere_info->param_buffer, STAGE_PIXEL_SHADER, 1, 0, 0);
        graphics->set_vertex_buffer(sphere_info->vertex_buffer, 0, sizeof(vec3), 0);
        graphics->set_index_buffer(sphere_info->index_buffer, 0);
        graphics->set_program(sphere_info->program);
        graphics->set_pipeline(game->d_test_write_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, sphere_info->index_buffer.size, 0, 0);
    }
    graphics->end_pass();

    shape_info_t* shape_info = &game->shape_info;
    shape_param_t* shape_param = &shape_info->param;

    f32 half_height = camera->position.z * tanf(camera->fov_y * (f32)DEG2RAD * 0.5f);
    f32 half_width = half_height * camera->aspect_ratio;
    f32 scale_x = half_width / (f32)PI;
    f32 scale_y = half_height / (0.5f * (f32)PI);
    
    shape_param->shape = game->shape_value;
    shape_param->yaw = global_earth_yaw;
    shape_param->pitch = global_earth_pitch;
    shape_param->scale = v2(scale_x, scale_y);
    shape_param->viewport_size = v2((f32)game->offscreen_scene.width, (f32)game->offscreen_scene.height);

    // graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t)
    // {
    //     .clear_color = false,
    //     .clear_depth = false
    // });
    // {
    //     shape_param->color = v4(0.006f, 0.006f, 0.006f, 1.0f);

    //     graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
    //     graphics->update_buffer(shape_info->param_buffer, &shape_info->param, 0, sizeof(shape_info->param));
    //     graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 0, 0, 0);
    //     graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_GEOMETRY_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
    //     graphics->set_vertex_buffer(shape_info->vertex_buffer_sphere, 0, sizeof(vec3), 0);
    //     graphics->set_index_buffer(shape_info->index_buffer_sphere, 0);
    //     graphics->set_program(shape_info->program);
    //     graphics->set_pipeline(game->d_test_pipeline);
    //     graphics->draw_indexed(TOPOLOGY_LINE_LIST, shape_info->index_buffer_sphere.size, 0, 0);
    // }
    // graphics->end_pass();

    graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = false });
    {
        graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
        graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_vertex_buffer(shape_info->vertex_buffer, 0, sizeof(vec3), 0);
        graphics->set_index_buffer(shape_info->index_buffer, 0);
        graphics->set_program(shape_info->program);
        graphics->set_pipeline(game->d_test_pipeline);

        shape_param->color = v4(0.04f, 0.04f, 0.04f, 1.0f);

        graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
        graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_GEOMETRY_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);

        graphics->draw_indexed(TOPOLOGY_LINE_LIST, shape_info->index_buffer.size, 0, 0);
 
        if (game->country_index != 0xFF)
        {
            shape_param->color = v4(0.2f, 0.2f, 0.2f, 0.7f);
            graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
            graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_GEOMETRY_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
            graphics->draw_indexed(TOPOLOGY_LINE_LIST,
                                   global_shape_offset_index_counts[game->country_index][1],
                                   global_shape_offset_index_counts[game->country_index][0], 0);   
        }
    }
    graphics->end_pass();

    // graphics->begin_pass(game->glow_mask_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.0f, 0.0f, 0.0f, 0.0f } });
    // {
    //     graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
    //     graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
    //     graphics->set_vertex_buffer(shape_info->vertex_buffer, 0, sizeof(vec3), 0);
    //     graphics->set_index_buffer(shape_info->index_buffer, 0);
    //     graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
    //     graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER, 1, 0, 0);
        
    //     game->glow_mask_setting = (glow_mask_setting_t){ .glow_color = { 0.9964f, 0.8431f, 0.4941f, 0.0f } };
    //     graphics->update_buffer(game->glow_buffer, &game->glow_mask_setting, 0, sizeof(game->glow_mask_setting));
    //     graphics->set_buffer(game->glow_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
    //     graphics->set_program(game->glow_program);
    //     graphics->set_pipeline(game->default_pipeline);
    //     graphics->draw_indexed(TOPOLOGY_LINE_LIST, shape_info->index_buffer.size, 0, 0);
    // }
    // graphics->end_pass();
    
    // NOTE: Horizontal blur. glow_mask -> glow_a.
    graphics->begin_pass(game->glow_a_target, &(graphics_pass_desc_t){ .clear_color = false });
    {
        game->glow_blur_setting = (glow_blur_setting_t)
        {
            .inverse_dst_size = { 1.0f / game->glow_a.width, 1.0f / game->glow_a.height },
            .direction = { 1.0f, 0.0f }
        };
        graphics->update_buffer(game->blur_buffer, &game->glow_blur_setting, 0, sizeof(game->glow_blur_setting));
        graphics->set_buffer(game->blur_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->blur_program);
        graphics->set_pipeline(game->default_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->glow_mask, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    // NOTE: Vertical blur. glow_a -> glow_b.
    graphics->begin_pass(game->glow_b_target, &(graphics_pass_desc_t){ .clear_color = false });
    {
        game->glow_blur_setting = (glow_blur_setting_t)
        {
            .inverse_dst_size = { 1.0f / game->glow_b.width, 1.0f / game->glow_b.height },
            .direction = { 0.0f, 1.0f }
        };
        graphics->update_buffer(game->blur_buffer, &game->glow_blur_setting, 0, sizeof(game->glow_blur_setting));
        graphics->set_buffer(game->blur_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->blur_program);
        graphics->set_pipeline(game->default_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->glow_a, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    // NOTE: Post pass rendering to backbuffer.
    graphics->begin_pass(graphics->get_backbuffer_target(), &(graphics_pass_desc_t){ .clear_color = false });
    {
        game->post_setting.inverse_dst_size[0] = 1.0f / platform->width;
        game->post_setting.inverse_dst_size[1] = 1.0f / platform->height;
        game->post_setting.inverse_src_size[0] = 1.0f / platform->width;
        game->post_setting.inverse_src_size[1] = 1.0f / platform->height;
        game->post_setting.aspect_ratio = (f32)platform->width / (f32)platform->height;
        game->post_setting.vignette_soft = 0.45f;

        if (platform->input->keys[KEY_I].action == KEY_ACTION_RELEASE)
        {
            game->post_setting.invert = game->post_setting.invert == 0.0f ? 1.0f : 0.0f;
        }

        if (platform->input->keys[KEY_V].action == KEY_ACTION_RELEASE)
        {
            game->post_setting.vignette = game->post_setting.vignette == 0.0f ? 1.0f : 0.0f;
        }

        if (platform->input->keys[KEY_G].action == KEY_ACTION_RELEASE)
        {
            game->post_setting.glow_intensity = game->post_setting.glow_intensity == 0.0f ? 1.0f : 0.0f;
        }

        graphics->update_buffer(game->post_buffer, &game->post_setting, 0, sizeof(game->post_setting));
        graphics->set_buffer(game->post_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->post_program);
        graphics->set_pipeline(game->default_pipeline);
        graphics_sampler_t samplers[] = { game->point_sampler, game->linear_sampler };
        graphics->set_samplers(STAGE_PIXEL_SHADER, samplers, 2, 0);
        // NOTE: Offscreen scene, horizontal and vertical blur, unblurred version (for filtering core part and leaving the halo).
        graphics_texture_t srvs[] = { game->offscreen_scene, game->glow_b, game->glow_mask };
        graphics->set_srvs(STAGE_PIXEL_SHADER, srvs, 3, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

#if FONT_ENABLE
    graphics->begin_draw();
    {
        char frame_ms_text[32] = { 0 };
        size_t frame_ms_length = 0;

        if ((frame_ms_length = snprintf(frame_ms_text, sizeof(frame_ms_text), "%.2f ms", platform->delta_time * 1000)) > 0)
        {
            graphics->draw_text(game->font, game->font_color, TEXT_ALIGNMENT_TRAILING,
                                0.0f - 8.0f, 0.0f + 8.0f, (f32)platform->width, (f32)platform->height,
                                frame_ms_text, frame_ms_length);   
        }

        if (game->country_index != 0xFF)
        {
            graphics->draw_text(game->font, game->font_color, TEXT_ALIGNMENT_LEADING,
                                8.0f, 8.0f, (f32)platform->width, (f32)platform->height,
                                global_shape_country_names[game->country_index],
                                strlen(global_shape_country_names[game->country_index]));
        }
    }
    graphics->end_draw();
#endif
}
