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
#include "../shader/glow_merge_pixel_shader.h"
#include "../shader/post_vertex_shader.h"
#include "../shader/post_pixel_shader.h"

#include "../shader/vertex_shader_shape.h"
#include "../shader/pixel_shader_shape.h"

#include "../shader/vertex_shader_sphere.h"
#include "../shader/pixel_shader_sphere.h"

#include "../shader/vertex_shader_sphere_grid.h"
#include "../shader/pixel_shader_sphere_grid.h"

#include "../shader/skybox_vertex_shader.h"
#include "../shader/skybox_pixel_shader.h"

#include "sphere_data.inl"

#include "memory_arena.c"

#include "ray.h"
#include "country.h"

#include "ray.c"
#include "country.c"

#include "ui.c"

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
    graphics_shader_t vertex_shader_grid;
    graphics_shader_t pixel_shader_grid;
    graphics_program_t program;
    graphics_program_t program_grid;

    sphere_param_t param;
} sphere_info_t;

typedef struct shape_param_t
{
    f32 shape;
    f32 yaw;
    f32 pitch;
    f32 line_thickness;
    vec2 scale;
    vec2 viewport_size;
    vec4 color;
} shape_param_t;

typedef struct shape_info_t
{
    graphics_buffer_t param_buffer;
    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;

    shape_param_t param;
} shape_info_t;

typedef struct skybox_info_t
{
    graphics_texture_t texture;
    graphics_buffer_t param_buffer;
    graphics_buffer_t vertex_buffer;
    graphics_buffer_t index_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;

    skybox_param_t param;
} skybox_info_t;

typedef struct glow_merge_param_t
{
    vec2 viewport_size;
    f32 intensity;
    f32 _pad;
} glow_merge_param_t;

typedef struct post_param_t
{
    vec2 viewport_size;
    f32 aspect_ratio;
    f32 invert;

    f32 vignette;
    f32 vignette_soft;

    f32 _pad[2];
} post_param_t;

typedef struct glow_mask_param_t
{
    vec4 glow_color;
} glow_mask_param_t;

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
    memory_arena_t* memory_arena;
    
    graphics_texture_t offscreen_scene;
    graphics_texture_t offscreen_scene_msaa;
    graphics_texture_t offscreen_depth_msaa;
    graphics_target_t offscreen_target_msaa;
    
    graphics_pipeline_t default_pipeline;
    graphics_pipeline_t pre_multiplied_pipeline;
    graphics_pipeline_t alphaoff_pipeline;
    graphics_pipeline_t additive_pipeline;
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
    skybox_info_t skybox_info;

    glow_mask_param_t glow_mask_setting;
    graphics_buffer_t glow_mask_buffer;
    graphics_texture_t glow_mask_msaa;
    graphics_texture_t glow_mask;
    graphics_texture_t glow_a;
    graphics_texture_t glow_b;
    graphics_target_t glow_mask_msaa_target;
    graphics_target_t glow_a_target;
    graphics_target_t glow_b_target;
    graphics_shader_t glow_mask_pixel_shader;
    graphics_program_t glow_mask_program;

    glow_blur_setting_t glow_blur_setting;
    graphics_buffer_t blur_buffer;
    graphics_shader_t blur_vertex_shader;
    graphics_shader_t blur_pixel_shader;
    graphics_program_t blur_program;

    glow_merge_param_t glow_merge_param;
    graphics_buffer_t glow_merge_param_buffer;
    graphics_shader_t glow_merge_pixel_shader;
    graphics_program_t glow_merge_program;

    post_param_t post_param;
    graphics_buffer_t post_param_buffer;
    graphics_shader_t post_vertex_shader;
    graphics_shader_t post_pixel_shader;
    graphics_program_t post_program;

    graphics_texture_t skybox_texture;
    graphics_buffer_t skybox_param_buffer;
    graphics_buffer_t skybox_vertex_buffer;
    graphics_buffer_t skybox_index_buffer;
    graphics_shader_t skybox_vertex_shader;
    graphics_shader_t skybox_pixel_shader;
    graphics_program_t skybox_program;

#if FONT_ENABLE
    graphics_2d_font_t font;
    vec4 font_color;
#endif
    ui_widget_draw_command_list_t widget_draw_command_list;

    country_data_t country_data;
    u8 country_index;

    // NOTE: 1.0f is globe map, 0.0f flat map.
    f32 shape_value;
    f32 shape_speed;
    f32 shape_direction;

    f32 glow_intensity;
    f32 vignette;
    f32 invert;
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

static f32 global_earth_yaw;
static f32 global_earth_pitch;
static bool global_earth_reset;

static bmp_image_t load_bmp_image(memory_arena_t* memory_arena, const io_t* io, const char* file_name)
{
    bmp_image_t result = { 0 };
    io_file_read_result_t read_result = io->read_file(file_name);

    if (read_result.size != 0)
    {
        bmp_header_t* header = (bmp_header_t*)read_result.data;
        u32 bytes_per_pixel = header->bits_per_pixel / 8;
        u32* pixels = (u32*)((u8*)read_result.data + header->bitmap_offset);
        u32* memory = 0;

        assert(bytes_per_pixel == 4 && "[BMP] Unsupported bytes per pixel.");
        
        if (memory_arena)
        {
            memory = ma_push_size(memory_arena, header->width * header->height * bytes_per_pixel);
        }
        else
        {
            memory = malloc(header->width * header->height * bytes_per_pixel);
        }

        memset(memory, 0, header->width * header->height * bytes_per_pixel);

        result.data = (u8*)memory;
        result.width = header->width;
        result.height = header->height;

        assert(header->compression == 3 && "[BMP] Unsupported compression format.");
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
    bool is_valid = graphics->is_valid_target(game->offscreen_target_msaa);

    if (is_valid && resized)
    {
        graphics->delete_target(game->offscreen_target_msaa);
        graphics->delete_target(game->glow_mask_msaa_target);
        graphics->delete_target(game->glow_a_target);
        graphics->delete_target(game->glow_b_target);
        graphics->delete_texture_2d(game->offscreen_scene);
        graphics->delete_texture_2d(game->offscreen_scene_msaa);
        graphics->delete_texture_2d(game->offscreen_depth_msaa);
        graphics->delete_texture_2d(game->glow_mask_msaa);
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
        
        game->offscreen_scene_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = platform->width,
            .height = platform->height,
            .sample_count = 8,
        }, 0, 0);

        game->offscreen_depth_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_D24_UNORM_S8_UINT,
            .bind = BIND_DEPTH_STENCIL,
            .width = platform->width,
            .height = platform->height,
            .sample_count = 8,
        }, 0, 0);

        game->glow_mask_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = platform->width,
            .height = platform->height,
            .sample_count = 8,
        }, 0, 0);

        game->glow_mask = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = platform->width,
            .height = platform->height,
        }, 0, 0);

        game->glow_a = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            // FORMAT_R8G8B8A8_UNORM,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(platform->width * 0.5f),
            .height = (u32)(platform->height * 0.5f),
        }, 0, 0);

        game->glow_b = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            //FORMAT_R8G8B8A8_UNORM,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(platform->width * 0.5f),
            .height = (u32)(platform->height * 0.5f),
        }, 0, 0);

        // NOTE: Create RenderTargetView for offscreen_scene to render.
        game->offscreen_target_msaa = graphics->create_target(&(graphics_target_desc_t){ .color = game->offscreen_scene_msaa, .depth = game->offscreen_depth_msaa });
        game->glow_mask_msaa_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->glow_mask_msaa, .depth = game->offscreen_depth_msaa });
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
        .index_format = FORMAT_R16_UINT,
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

    sphere_info->vertex_shader_grid = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_sphere_grid,
        .bytecode_size = sizeof(vshader_sphere_grid),
        .stage = STAGE_VERTEX_SHADER,
    });

    sphere_info->pixel_shader_grid = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_sphere_grid,
        .bytecode_size = sizeof(pshader_sphere_grid),
        .stage = STAGE_PIXEL_SHADER,
    });

    sphere_info->program_grid = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = sphere_info->vertex_shader_grid,
        .pixel_shader = sphere_info->pixel_shader_grid,
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

    shape_info->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = shape_info->vertex_shader,
        .pixel_shader = shape_info->pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "TEXCOORD", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, prev), 0, 0, 0, 0 },
            { "POSITION", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, current), 0, 0, 0, 0 },
            { "TEXCOORD", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, next), 1, 0, 0, 0 },
            { "TEXCOORD", FORMAT_R32_FLOAT,       offsetof(country_border_mesh_vertex_t, side), 2, 0, 0, 0 },
        },
        .attribute_count = 4,
    });
}

typedef struct skybox_t
{
    memory_arena_t* memory_arena;
    io_t* io;
    bmp_image_t image;
    const char* file_name;
} skybox_t;

thread_pool_entry_function(skybox_load_entry)
{
    skybox_t* skybox = (skybox_t*)parameter;

    skybox->image = load_bmp_image(skybox->memory_arena, skybox->io, skybox->file_name);
}

static void init_skybox(memory_arena_t* memory_arena, graphics_t* graphics, io_t* io, thread_pool_t* thread_pool, skybox_info_t* skybox_info)
{
    const char* skybox_file_names[] =
    {
        "..\\resources\\px_bmp.bmp",
        "..\\resources\\nx_bmp.bmp",
        "..\\resources\\py_bmp.bmp",
        "..\\resources\\ny_bmp.bmp",
        "..\\resources\\pz_bmp.bmp",
        "..\\resources\\nz_bmp.bmp",
    };

    skybox_t* skyboxes[array_count(skybox_file_names)] = { 0 };
    
    memory_arena_span_t skybox_span = ma_span_begin(memory_arena);

    for (i32 i = 0; i < array_count(skybox_file_names); ++i)
    {
        memory_arena_t* skybox_arena = ma_create_sub_arena(memory_arena, MIBIBYTES(32));
        skybox_t* skybox = ma_push_size(skybox_arena, sizeof(skybox_t));
        skybox->memory_arena = skybox_arena;
        skybox->io = io;
        skybox->file_name = skybox_file_names[i];
        skyboxes[i] = skybox;
        thread_pool->add_entry(thread_pool->queue, skybox_load_entry, skybox);
    }

    thread_pool->complete_all_entries(thread_pool->queue);
    
    void* skybox_data[array_count(skyboxes)]  = { 0 };
    u32 skybox_pitches[array_count(skyboxes)] = { 0 };

    for (i32 i = 0; i < array_count(skyboxes); ++i)
    {
        skybox_data[i] = skyboxes[i]->image.data;
        skybox_pitches[i] = skyboxes[i]->image.pitch;
    }

    skybox_info->texture = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
    {
        .format = FORMAT_R8G8B8A8_UNORM_SRGB,
        .bind = BIND_SHADER_RESOURCE,
        .width = (u32)(skyboxes[0]->image.width),
        .height = (u32)(skyboxes[0]->image.height),
        .array_size = array_count(skybox_data),
        .misc = MISC_TEXTURE_CUBE,
    }, (const void**)skybox_data, skybox_pitches);

    
    ma_span_end(skybox_span);

    // NOTE: Unit cube centered at origin.
    const skybox_vertex_t skybox_vertices[] =
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
    
    const u16 skybox_indices[] =
    {
        0,  1, 2,    0,  2,  3, // +X
        4,  5, 6,    4,  6,  7, // -X
        8,  9, 10,   8, 10, 11, // +Y
        12, 13, 14,  12, 14, 15, // -Y
        16, 17, 18,  16, 18, 19, // +Z
        20, 21, 22,  20, 22, 23  // -Z
    };

    skybox_info->param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(skybox_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    skybox_info->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = skybox_vertices,
        .size = sizeof(skybox_vertices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    skybox_info->index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = skybox_indices,
        .size = array_count(skybox_indices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_INDEX_BUFFER,
        .index_format = FORMAT_R16_UINT,
    });

    skybox_info->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = skybox_vshader,
        .bytecode_size = sizeof(skybox_vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    skybox_info->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = skybox_pshader,
        .bytecode_size = sizeof(skybox_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });
    
    skybox_info->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = skybox_info->vertex_shader,
        .pixel_shader = skybox_info->pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });
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

static vec2 xy_to_lon_lat(f32 x, f32 y, f32 width, f32 height)
{
    vec2 result = { 0 };
    vec2 normalized_position = v2(x / width, y / height);
    // NOTE: Map to lon(-180, 180), lat(-90, 90).
    result.x = (2.0f * normalized_position.x - 1.0f) * 180.0f;
    result.y = (1.0f - 2.0f * normalized_position.y) * 90.0f;

    return result;
}

static ray_result_t ray_to_lon_lat(ray_t ray, vec2* lonlat)
{
    ray_result_t ray_result = ray_hit_unit_sphere(ray);
    
    if (ray_result.hit)
    {
        vec3 hit_position = v3_normalize(v3_add(ray.origin, v3_mulf(ray.direction, ray_result.t)));

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

        lonlat->x = lon / (f32)DEG2RAD;
        lonlat->y = lat / (f32)DEG2RAD;
    }

    return ray_result;
}

static u8 earth_find_country_index_under_cursor(const platform_t* platform, game_t* game)
{
    input_t* input = platform->input;
    camera_t* camera = &game->camera;

    game->shape_direction = (input_is_key_released(input, KEY_T) ?
                             -game->shape_direction : game->shape_direction);
    game->shape_speed = 1.0f * platform->delta_time * game->shape_direction;
    game->shape_value = clamp(0.0f, game->shape_value + game->shape_speed, 1.0f);

    vec2 lon_lat = { 0 };
    
    if (game->shape_value == 0.0f)
    {
        lon_lat = xy_to_lon_lat(input->mouse_position.x, input->mouse_position.y,
                                (f32)platform->width, (f32)platform->height);
    }
    else if (game->shape_value == 1.0f)
    {
        ray_t mouse_ray = ray_world(input->mouse_position.x, input->mouse_position.y,
                                    (f32)platform->width, (f32)platform->height, camera->fov_y,
                                    camera->position, camera->view);

        vec2 ray_lon_lat = { 0 };    
        ray_result_t ray_result = ray_to_lon_lat(mouse_ray, &ray_lon_lat);

        if (ray_result.hit)
        {
            lon_lat = ray_lon_lat;
        }
    }

    u8 country_index = country_cell_get_index(&game->country_data.query, lon_lat.x, lon_lat.y);
    
    return country_index;
}

static void earth_rotation(const input_t* input, f32 delta_time)
{
    if (input_is_key_pressed(input, KEY_MOUSE_LEFT))
    {
        global_earth_yaw += 3.0f * input->mouse_delta.x * delta_time;
        global_earth_pitch += 3.0f * -input->mouse_delta.y * delta_time;
    }

    global_earth_pitch = clamp(-90.0f, global_earth_pitch, 90.0f);

    if (input_is_key_released(input, KEY_R))
    {
        global_earth_reset = !global_earth_reset;
    }

    f32 earth_reset_speed = 200.0f * delta_time;
    
    if (global_earth_reset)
    {
        if (global_earth_yaw != 0.0f)
        {
            while (global_earth_yaw > 360.0f)
            {
                global_earth_yaw -= 360.0f;
            }

            while (global_earth_yaw < -360.0f)
            {
                global_earth_yaw += 360.0f;
            }

            if (fabs(global_earth_yaw) < earth_reset_speed)
            {
                global_earth_yaw = 0.0f;
            }
            else
            {
                global_earth_yaw += global_earth_yaw > 0.0f ? -earth_reset_speed : earth_reset_speed;
            }
        }
        if (global_earth_pitch != 0.0f)
        {
            if (fabs(global_earth_pitch) < earth_reset_speed)
            {
                global_earth_pitch = 0.0f;
            }
            else
            {
                global_earth_pitch += global_earth_pitch > 0.0f ? -earth_reset_speed : earth_reset_speed;
            }
        }

        if (global_earth_yaw == 0.0f && global_earth_pitch == 0.0f)
        {
            global_earth_reset = false;
        }
    }    
}

init_function(init)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    io_t* io = platform->io;
    thread_pool_t* thread_pool = platform->thread_pool;
    game_t* game = (game_t*)memory->permanent;
    memory_arena_t* memory_arena = ma_initialize(memory->permanent + sizeof(game_t), memory->permanent_size - sizeof(game_t));
    game->memory_arena = memory_arena;

    init_camera(&game->camera, v3(0.0f, 0.0f, 2.5f), v3(0.0f, 0.0f, 0.0f),
                60.0f, (f32)platform->width / (f32)platform->height);
    init_sphere(graphics, &game->sphere_info);
    init_shape(graphics, &game->shape_info);
    init_skybox(memory_arena, graphics, io, thread_pool, &game->skybox_info);
    init_country_data(memory_arena, graphics, io, &game->country_data);

    game->shape_value = 1.0f;
    game->shape_direction = 1.0f;

    game->default_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t){ .blend = BLEND_ALPHA, });
    game->pre_multiplied_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t){ .blend = BLEND_PRE_MULTIPLIED });
    game->alphaoff_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t){ .depth_test = true, });
    game->additive_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t){ .depth_test = true, .blend = BLEND_ADDITIVE });

    game->d_test_write_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = true,
        .depth_test = true,
        .depth_write = true,
        .blend = BLEND_ALPHA,
    });

    game->d_test_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = false,
        .depth_test = true,
        .depth_write = false,
        .blend = BLEND_ALPHA,
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

    game->glow_mask_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = 16,
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->glow_mask_pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = glow_mask_pshader,
        .bytecode_size = sizeof(glow_mask_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->glow_mask_program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->shape_info.vertex_shader,
        .pixel_shader = game->glow_mask_pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "TEXCOORD", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, prev), 0, 0, 0, 0 },
            { "POSITION", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, current), 0, 0, 0, 0 },
            { "TEXCOORD", FORMAT_R32G32B32_FLOAT, offsetof(country_border_mesh_vertex_t, next), 1, 0, 0, 0 },
            { "TEXCOORD", FORMAT_R32_FLOAT,       offsetof(country_border_mesh_vertex_t, side), 2, 0, 0, 0 },
        },
        .attribute_count = 4,
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

    game->glow_merge_param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(glow_merge_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });
    
    game->glow_merge_pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = glow_merge_pshader,
        .bytecode_size = sizeof(glow_merge_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->glow_merge_program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->blur_vertex_shader,
        .pixel_shader = game->glow_merge_pixel_shader,
        // NOTE: No input layout.
        .attributes = 0,
        .attribute_count = 0,
    });

    // NOTE: Post shaders and pipeline.
    game->post_param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(post_param_t),
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
    // game->font_color = vec4(0.8313f, 0.0f, 0.4705f, 1.0f);
    // game->font_color = vec4(0.38f, 0.38f, 0.38f, 1.0f);
    game->font_color = v4(0.6862f, 0.6862f, 0.6862f, 1.0f);
#endif
}

static ui_measure_text_width_function(measure_text_width)
{
    graphics_2d_font_t graphics_font = *(graphics_2d_font_t*)font;
    graphics_t* graphics = (graphics_t*)parameter;

    f32 text_width = graphics->measure_text_width(graphics_font, text, text_length);

    return text_width;
}

static ui_get_line_height_function(get_line_height)
{
    graphics_2d_font_t graphics_font = *(graphics_2d_font_t*)font;
    graphics_t* graphics = (graphics_t*)parameter;

    f32 line_height = graphics->get_line_height(graphics_font);

    return line_height;
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

    if (game->shape_value == 1.0f)
    {
        earth_rotation(input, platform->delta_time);
    }

    game->country_index = earth_find_country_index_under_cursor(platform, game);

    if (country_is_valid_index(game->country_index))
    {
        country_name_t country_name = country_get_name(game->country_index);

        if (country_name.name)
        {
            fprintf(stderr, "\rcountry: %s, id: %u, yaw: %f, pitch: %f",
                    country_name.name, game->country_index,
                    global_earth_yaw, global_earth_pitch);
        }
    }

    if (input_is_key_released(input, KEY_G))
    {
        game->glow_intensity = game->glow_intensity == 0.0f ? 0.3f : 0.0f;
    }

    if (input_is_key_released(input, KEY_I))
    {
        game->invert = game->invert == 0.0f ? 1.0f : 0.0f;
    }

    if (input_is_key_released(input, KEY_V))
    {
        game->vignette = game->vignette == 0.0f ? 1.0f : 0.0f;
    }

    ui_begin(game->memory_arena, (f32)platform->width, (f32)platform->height, (ui_callback_list_t)
    {
        .measure_text_width = { measure_text_width, platform->graphics },
        .get_line_height = { get_line_height, platform->graphics },
    });
    {
        ui_widget_group_begin("ui-widget-group-1", 220, 220, (ui_widget_desc_t)
        {
            .size = { ui_widget_pixel_size(360.0f), ui_widget_pixel_size(360.0f) },
            .child_axis = ui_widget_axis_y(),
            .padding = 4.0f,
            .border = ui_widget_border(true, 1.0f, ui_widget_color(0.4f, 0.4f, 0.4f, 0.4f)),
        });
        {
            ui_widget_group_begin("ui-widget-group-2", 0, 0, (ui_widget_desc_t)
            {
                .size = { ui_widget_parent_size(1.0f), ui_widget_parent_size(1.0f) },
                .child_axis = ui_widget_axis_y(),
                .padding = 1.0f,
                .border = ui_widget_border(true, 1.0f, ui_widget_color(0.4f, 0.4f, 0.4f, 0.4f)),
            });
            {
                ui_widget("ui-widget-1", (ui_widget_desc_t)
                {
                    .color = ui_widget_color(0.06f, 0.06f, 0.06f, 1.0f),
                    .size = { ui_widget_parent_size(1.0f), ui_widget_pixel_size(200.0f) },
                });
                country_name_t country_name = country_get_name(game->country_index);
                
                ui_widget("ui-widget-2", (ui_widget_desc_t)
                {
                    .size = { ui_widget_parent_size(1.0f), ui_widget_parent_size(1.0f) },
                    .color = ui_widget_color(0.04f, 0.04f, 0.04f, 1.0f),
                    .label = ui_widget_label(&game->font, country_name.name, country_name.length, ui_widget_align_center()),
                    .padding = 16.0f,
                });
                ui_widget("ui-widget-3", (ui_widget_desc_t)
                {
                    .color = ui_widget_color(0.06f, 0.06f, 0.06f, 1.0f),
                    .size = { ui_widget_parent_size(1.0f), ui_widget_pixel_size(40.0f) },
                });
                ui_widget("ui-widget-4", (ui_widget_desc_t)
                {
                    .color = ui_widget_color(0.06f, 0.06f, 0.06f, 1.0f),
                    .size = { ui_widget_parent_size(1.0f), ui_widget_parent_size(1.0f) },
                });
            }
            ui_widget_group_end();
        }
        ui_widget_group_end();
    }
    ui_widget_draw_command_list_t widget_draw_command_list = ui_end();
    game->widget_draw_command_list = widget_draw_command_list;
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

    graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t)
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

        graphics->update_buffer(game->skybox_info.param_buffer, &skybox_param, 0, sizeof(skybox_param_t));
        graphics->set_buffer(game->skybox_info.param_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(game->skybox_info.param_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_vertex_buffer(game->skybox_info.vertex_buffer, 0, sizeof(skybox_vertex_t), 0);
        graphics->set_index_buffer(game->skybox_info.index_buffer, 0);
        graphics->set_program(game->skybox_info.program);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->skybox_info.texture, 1, 0);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
        graphics->set_pipeline(game->pre_multiplied_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, game->skybox_info.index_buffer.size, 0, 0);
    }
    graphics->end_pass();
    
    graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t){ 0 });
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
    shape_param->line_thickness = 1.0f;
    shape_param->scale = v2(scale_x, scale_y);
    shape_param->viewport_size = v2((f32)game->offscreen_scene.width, (f32)game->offscreen_scene.height);

    graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t){ 0 });
    {
        shape_param->color = v4(0.006f, 0.006f, 0.006f, 1.0f);

        sphere_info_t* sphere_info = &game->sphere_info;

        graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
        graphics->update_buffer(shape_info->param_buffer, &shape_info->param, 0, sizeof(shape_info->param));
        graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
        graphics->set_vertex_buffer(sphere_info->vertex_buffer, 0, sizeof(vec3), 0);
        graphics->set_index_buffer(sphere_info->index_buffer, 0);
        graphics->set_program(sphere_info->program_grid);
        graphics->set_pipeline(game->d_test_pipeline);
        graphics->draw_indexed(TOPOLOGY_LINE_LIST, sphere_info->index_buffer.size, 0, 0);
    }
    graphics->end_pass();

    graphics->begin_pass(game->glow_mask_msaa_target, &(graphics_pass_desc_t) { .clear_color = true });
    {
        country_mesh_data_t* country_mesh_data = &game->country_data.mesh;
        
        shape_info->param.line_thickness = 2.0f;
        graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
        graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
        graphics->set_vertex_buffer(country_mesh_data->vertex_buffer, 0, country_mesh_data->vertex_stride, 0);
        graphics->set_index_buffer(country_mesh_data->index_buffer, 0);
        graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER, 1, 0, 0);
        
        // game->glow_mask_setting.glow_color = v4v(srgb_to_linear(v3(0.9964f, 0.8431f, 0.4941f)), 1.0f);
        game->glow_mask_setting.glow_color = v4v(srgb_to_linear(v3(0.1058f, 0.9921f, 0.6117f)), 1.0f);
        graphics->update_buffer(game->glow_mask_buffer, &game->glow_mask_setting, 0, sizeof(game->glow_mask_setting));
        graphics->set_buffer(game->glow_mask_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->glow_mask_program);
        graphics->set_pipeline(game->alphaoff_pipeline);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, country_mesh_data->index_count, 0, 0);
    }
    graphics->end_pass();

    graphics->resolve_texture(game->glow_mask, game->glow_mask_msaa);
    
    // NOTE: Horizontal blur. glow_mask -> glow_a.
    graphics->begin_pass(game->glow_a_target, &(graphics_pass_desc_t){ 0 });
    {
        game->glow_blur_setting = (glow_blur_setting_t)
        {
            .inverse_dst_size = { 1.0f / game->glow_a.width, 1.0f / game->glow_a.height },
            .direction = { 1.0f, 0.0f }
        };
        graphics->update_buffer(game->blur_buffer, &game->glow_blur_setting, 0, sizeof(game->glow_blur_setting));
        graphics->set_buffer(game->blur_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->blur_program);
        graphics->set_pipeline(game->alphaoff_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->glow_mask, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    // NOTE: Vertical blur. glow_a -> glow_b.
    graphics->begin_pass(game->glow_b_target, &(graphics_pass_desc_t){ 0 });
    {
        game->glow_blur_setting = (glow_blur_setting_t)
        {
            .inverse_dst_size = { 1.0f / game->glow_b.width, 1.0f / game->glow_b.height },
            .direction = { 0.0f, 1.0f }
        };
        graphics->update_buffer(game->blur_buffer, &game->glow_blur_setting, 0, sizeof(game->glow_blur_setting));
        graphics->set_buffer(game->blur_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->blur_program);
        graphics->set_pipeline(game->alphaoff_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->glow_a, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    // // NOTE: Merge pass. offscreen_scene + glow -> offscreen_scene.
    graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t){ 0 });
    {
        game->glow_merge_param.viewport_size = (vec2){ (f32)platform->width, (f32)platform->height };
        game->glow_merge_param.intensity = game->glow_intensity;
        graphics->update_buffer(game->glow_merge_param_buffer, &game->glow_merge_param, 0, sizeof(game->glow_merge_param));
        graphics->set_buffer(game->glow_merge_param_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->glow_merge_program);
        graphics->set_pipeline(game->additive_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->glow_b, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();

    graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t){ 0 });
    {
        country_mesh_data_t* country_mesh_data = &game->country_data.mesh;
        
        graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
        graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_vertex_buffer(country_mesh_data->vertex_buffer, 0, country_mesh_data->vertex_stride, 0);
        graphics->set_index_buffer(country_mesh_data->index_buffer, 0);
        graphics->set_program(shape_info->program);
        graphics->set_pipeline(game->d_test_pipeline);

        // shape_param->color = v4v(srgb_to_linear(v3(0.9964f, 0.8431f, 0.4941f)), 1.0f);
        shape_param->color = v4v(srgb_to_linear(v3(0.1058f, 0.9921f, 0.6117f)), 1.0f);
        shape_param->line_thickness = 2.0f;

        graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
        graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);

        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, country_mesh_data->index_count, 0, 0);

        if (country_is_valid_index(game->country_index))
        {
            // shape_param->color = v4(0.2f, 0.2f, 0.2f, 0.7f);
            shape_param->color = v4(1.0f, 1.0f, 1.0f, 1.0f);
            graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
            graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);

            graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST,
                                   country_mesh_data->index_ranges[game->country_index].index_count,
                                   country_mesh_data->index_ranges[game->country_index].index_offset, 0);
        }
    }
    graphics->end_pass();

    graphics->resolve_texture(game->offscreen_scene, game->offscreen_scene_msaa);
    
    // NOTE: Post pass rendering to backbuffer.
    graphics->begin_pass(graphics->get_backbuffer_target(), &(graphics_pass_desc_t){ .clear_color = true,  });
    {
        game->post_param.viewport_size = (vec2){ (f32)platform->width, (f32)platform->height };
        game->post_param.aspect_ratio = (f32)platform->width / (f32)platform->height;
        game->post_param.vignette_soft = 0.45f;
        game->post_param.invert = game->invert;
        game->post_param.vignette = game->vignette;

        graphics->update_buffer(game->post_param_buffer, &game->post_param, 0, sizeof(game->post_param));
        graphics->set_buffer(game->post_param_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->post_program);
        graphics->set_pipeline(game->alphaoff_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &game->point_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &game->offscreen_scene, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();


    graphics->begin_draw();
    {
#if FONT_ENABLE
        char frame_ms_text[32] = { 0 };
        size_t frame_ms_length = 0;

        if ((frame_ms_length = snprintf(frame_ms_text, sizeof(frame_ms_text), "%.2f ms", platform->delta_time * 1000)) > 0)
        {
            graphics->draw_text(game->font, frame_ms_text, frame_ms_length,
                                game->font_color.r, game->font_color.g, game->font_color.b, game->font_color.a,
                                TEXT_ALIGNMENT_TRAILING, 0.0f - 8.0f, 0.0f + 8.0f, (f32)platform->width, (f32)platform->height);
        }

        country_name_t country_name = country_get_name(game->country_index);
        if (country_name.name && country_name.length)
        {
            graphics->draw_text(game->font, country_name.name, country_name.length,
                                game->font_color.r, game->font_color.g, game->font_color.b, game->font_color.a,
                                TEXT_ALIGNMENT_LEADING, 8.0f, 8.0f, (f32)platform->width, (f32)platform->height);
        }
#endif

        ui_widget_draw_command_list_t* widget_draw_command_list = &game->widget_draw_command_list;
        for (u32 i = 0; i < widget_draw_command_list->command_count; ++i)
        {
            ui_widget_draw_command_t* command = widget_draw_command_list->commands + i;

            if (command->kind == UI_WIDGET_DRAW_RECT)
            {
                ui_widget_draw_rect_t* draw_rect = &command->rect;
                
                f32 x = draw_rect->x;
                f32 y = draw_rect->y;
                f32 width = draw_rect->width;
                f32 height = draw_rect->height;
                f32 color[4] = { draw_rect->r, draw_rect->g, draw_rect->b, draw_rect->a };
                
                graphics->draw_rect(x, y, width, height, true, 0.0f, color[0], color[1], color[2], color[3]);
                // graphics->draw_rect(x + (width - 1.0f) * 0.5f, y, 1.0f, height, true, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
                // graphics->draw_rect(x, y + (height - 1.0f) * 0.5f, width, 1.0f, true, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
            }
            else if (command->kind == UI_WIDGET_DRAW_BORDER)
            {
                ui_widget_draw_border_t* draw_border = &command->border;
                
                f32 x = draw_border->x;
                f32 y = draw_border->y;
                f32 width = draw_border->width;
                f32 height = draw_border->height;
                f32 color[4] = { draw_border->r, draw_border->g, draw_border->b, draw_border->a };
                f32 thickness = draw_border->thickness;

                graphics->draw_rect(x, y, width, height, false, thickness, color[0], color[1], color[2], color[3]);
            }
            else if (command->kind == UI_WIDGET_DRAW_TEXT)
            {
                ui_widget_draw_text_t* draw_text = &command->text;
                f32 x = draw_text->x;
                f32 y = draw_text->y;
                f32 width = draw_text->width;
                f32 height = draw_text->height;
                const char* text = draw_text->text;
                u32 length = draw_text->length;
                
                graphics->draw_text(game->font, text, length, 0.6f, 0.6f, 0.6f, 1.0f,
                                    TEXT_ALIGNMENT_LEADING, x, y, width, height);
            }
            else
            {
                assert(!"[UI] Invalid draw command.");
            }
        }
    }
    graphics->end_draw();

}
