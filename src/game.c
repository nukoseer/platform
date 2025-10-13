#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "platform.h"
#include "maths.h"

#include "../shader/vertex_shader.h"
#include "../shader/pixel_shader.h"
#include "../shader/glow_mask_pixel_shader.h"
#include "../shader/blur_vertex_shader.h"
#include "../shader/blur_pixel_shader.h"
#include "../shader/post_vertex_shader.h"
#include "../shader/post_pixel_shader.h"

#include "../shader/vertex_shader_3d.h"
#include "../shader/pixel_shader_3d.h"

#include "../shader/vertex_shader_globe.h"
#include "../shader/pixel_shader_globe.h"

#include "../shader/vertex_shader_sphere.h"
#include "../shader/pixel_shader_sphere.h"

#include "shape_data.inl"
#include "sphere_data.inl"
#include "landmask_data.inl"

typedef struct vertex_t
{
    vec2 position;
    vec3 color;
} vertex_t;

typedef struct vertex3d_t
{
    vec3 position;
    vec3 color;
} vertex3d_t;

typedef struct sphere_vertex_t
{
    vec3 position;
    vec3 normal;
} sphere_vertex_t;

typedef struct setting_3d_t
{
    mat4x4 world;
    mat4x4 view;
    mat4x4 projection;
    vec4 camera_world;
} setting_3d_t;

typedef struct globe_param_t
{
    f32 shape;
    vec2 scale;
    f32 _pad;
} globe_param_t;

typedef struct water_setting_t
{
    f32 time;
    f32 earth_angle;
    f32 tiling_a;
    f32 speed_a;
    f32 tiling_b;
    f32 speed_b;
    f32 ripple_amp;
    f32 _pad;
} water_setting_t;

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
    vec3 rotation;
    // TODO: Is scale necessary?
    vec3 scale;
} camera_t;

typedef struct game_t
{
    vertex_t vertex_data[3];
    graphics_buffer_t vertex_buffer;
    graphics_texture_t offscreen_scene;
    graphics_texture_t offscreen_depth;
    graphics_target_t offscreen_target;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
    graphics_pipeline_t default_pipeline;

    camera_t camera;
    setting_3d_t setting_3d;
    vertex3d_t vertex_data_3d[36];
    graphics_buffer_t vertex_buffer_3d;
    graphics_buffer_t setting_buffer_3d;
    graphics_shader_t vertex_shader_3d;
    graphics_shader_t pixel_shader_3d;
    graphics_program_t program_3d;
    graphics_pipeline_t pipeline_3d;
    graphics_pipeline_t pipeline_3d_;

    graphics_buffer_t sphere_vertex_buffer;
    graphics_buffer_t sphere_index_buffer;
    graphics_shader_t vertex_shader_sphere;
    graphics_shader_t pixel_shader_sphere;
    graphics_program_t program_sphere;

    graphics_buffer_t globe_param_buffer;
    graphics_buffer_t globe_vertex_buffer;
    graphics_buffer_t globe_vertex_buffer1;
    graphics_buffer_t globe_index_buffer;
    graphics_shader_t vertex_shader_globe;
    graphics_shader_t pixel_shader_globe;
    graphics_program_t program_globe;

    glow_mask_setting_t glow_mask_setting;
    graphics_sampler_t linear_sampler;
    graphics_buffer_t glow_buffer;
    graphics_texture_t glow_mask;
    graphics_texture_t glow_a;
    graphics_texture_t glow_b;
    graphics_target_t glow_mask_target;
    graphics_target_t glow_a_target;
    graphics_target_t glow_b_target;
    graphics_shader_t glow_pixel_shader;
    graphics_program_t glow_program;
    graphics_program_t glow_program_3d;

    glow_blur_setting_t glow_blur_setting;
    graphics_buffer_t blur_buffer;
    graphics_shader_t blur_vertex_shader;
    graphics_shader_t blur_pixel_shader;
    graphics_program_t blur_program;

    graphics_buffer_t post_buffer;
    graphics_sampler_t point_sampler;
    graphics_shader_t post_vertex_shader;
    graphics_shader_t post_pixel_shader;
    graphics_program_t post_program;
    post_setting_t post_setting;

    // graphics_2d_font_t font;
    // graphics_2d_font_color_t font_color;

    graphics_texture_t water_normal_a;
    graphics_texture_t water_normal_b;
    graphics_texture_t land_sea_mask;
    graphics_sampler_t linear_wrap_sampler;
    graphics_buffer_t water_buffer;
    water_setting_t water_setting;
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

        result.data = (u8*)pixels;
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
    
        bool red_found = _BitScanForward(&red_index, red_mask);
        bool green_found = _BitScanForward(&green_index, green_mask);
        bool blue_found = _BitScanForward(&blue_index, blue_mask);
        bool alpha_found = _BitScanForward(&alpha_index, alpha_mask);
    
        assert(red_found   && "[BMP] Invalid red channel mask.");
        assert(green_found && "[BMP] Invalid green channel mask.");
        assert(blue_found  && "[BMP] Invalid blue channel mask.");
        assert(alpha_found && "[BMP] Invalud alpha channel mask.");
    
        u32* memory = pixels;

        for (u32 y = 0; y < header->height; ++y)
        {
            for (u32 x = 0; x < header->width; ++x)
            {
    	        u32 color = *memory;

    	        f32 r = (f32)((color & red_mask) >> red_index);
    	        f32 g = (f32)((color & green_mask) >> green_index);
    	        f32 b = (f32)((color & blue_mask) >> blue_index);
    	        f32 a = (f32)((color & alpha_mask) >> alpha_index);

    	        *memory++ = (((u32)(a + 0.5f) << 24) |
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
            .format = FORMAT_R8G8B8A8_UNORM,
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

    // result.x = position.x;
    // result.y = cosf(rad) * position.y - sinf(rad) * position.z;
    // result.z = sinf(rad) * position.y + cosf(rad) * position.z;

    mat4x4 rotation_matrix =
    {
        .columns =
        {
            [0] = v4(1.0f, 0.0f,       0.0f,      0.0f),
            [1] = v4(0.0f, cosf(rad), -sinf(rad), 0.0f),
            [2] = v4(0.0f, sinf(rad),  cosf(rad), 0.0f),
            [3] = v4(0.0f, 0.0f,       0.0f,      1.0f)
        },
    };

    return rotation_matrix;
}

static inline mat4x4 rotate_y(f32 angle)
{
    f32 rad = angle * (f32)DEG2RAD;

    // result.x = cosf(rad) * position.x - sinf(rad) * position.z;
    // result.y = position.y;
    // result.z = sinf(rad) * position.x + cosf(rad) * position.z;

    mat4x4 rotation_matrix =
    {
        .columns =
        {
            [0] = v4(cosf(rad), 0.0f, -sinf(rad), 0.0f),
            [1] = v4(0.0f,      1.0f,  0.0f,      0.0f),
            [2] = v4(sinf(rad), 0.0f,  cosf(rad), 0.0f),
            [3] = v4(0.0f,      0.0f,  0.0f,      1.0f)
        },
    };

    return rotation_matrix;
}

static inline mat4x4 rotate_z(f32 angle)
{
    f32 rad = angle * (f32)DEG2RAD;

    // result.x = cosf(rad) * position.x - sinf(rad) * position.y;
    // result.y = sinf(rad) * position.x + cosf(rad) * position.y;
    // result.z = position.z;

    mat4x4 rotation_matrix =
    {
        .columns =
        {
            [0] = v4(cosf(rad),  -sinf(rad),  0.0f, 0.0f),
            [1] = v4(sinf(rad),   cosf(rad),  0.0f, 0.0f),
            [2] = v4(0.0f,        0.0f,       1.0f, 0.0f),
            [3] = v4(0.0f,        0.0f,       0.0f, 1.0f)
        },
    };


    return rotation_matrix;
}

// NOTE: View matrix, right-handed, -z
static inline mat4x4 view_matrix(vec3 reference_up, vec3 from, vec3 to)
{
    vec3 forward = v3_normalize(v3_sub(from, to));
    f32 cos_up = fabsf(v3_dot(reference_up, forward));
    vec3 up_hint = (cos_up > 0.999f) ? v3(0.0f, 0.0f, 1.0f) : reference_up;
    vec3 right = v3_normalize(v3_cross(up_hint, forward));
    vec3 up = v3_cross(forward, right);

    mat4x4 result =
    {
        .columns =
        {
            [0] = v4v(right, 0.0f),
            [1] = v4v(up, 0.0f),
            [2] = v4v(forward, 0.0f),
            [3] = v4(0.0f, 0.0f, 0.0f, 1.0f),
        },
    };

    result.columns[3].x = -v3_dot(right, from);
    result.columns[3].y = -v3_dot(up, from);
    result.columns[3].z = -v3_dot(forward, from);
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

init_function(init)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    io_t* io = platform->io;
    game_t* game = (game_t*)memory->permanent;

    vertex_t vertex_data[] =
    {
        { { +0.00f, +0.66f }, { 1.0f, 0.0f, 0.0f, } },
        { { -0.33f, -0.33f }, { 0.0f, 1.0f, 0.0f, } },
        { { +0.33f, -0.33f }, { 0.0f, 0.0f, 1.0f, } },
    };

    memcpy(game->vertex_data, vertex_data, sizeof(game->vertex_data));

    game->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = vertex_data,
        .size = sizeof(vertex_data),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    game->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader,
        .bytecode_size = sizeof(vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    game->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader,
        .bytecode_size = sizeof(pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->vertex_shader,
        .pixel_shader = game->pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32_FLOAT,    offsetof(vertex_t, position), 0, 0, 0, 0 },
            { "COLOR",    FORMAT_R32G32B32_FLOAT, offsetof(vertex_t, color),    0, 0, 0, 0 },
        },
        .attribute_count = 2,
    });

    game->default_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = false,
        .wireframe = false,
    });

    game->camera.position = v3(0.0f, 0.0f, 2.5f);
    game->setting_3d.world = m4x4d(1.0f);
    game->setting_3d.view = view_matrix(v3(0.0f, 1.0f, 0.0f), game->camera.position, v3(0.0f, 0.0f, 0.0f));
    // game->setting_3d.projection = projection_matrix(-1.0f, +1.0f, -1.0f, +1.0f, 0.1f, 100.0f);
    game->setting_3d.projection = perspective_projection_fov_y(60.0f, (f32)platform->width / (f32)platform->height, 0.1f, 100.0f);

    vertex3d_t unit_cube[] =
    {
        // NOTE: Front
        { { +0.1f, +0.1f, +0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { -0.1f, +0.1f, +0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { -0.1f, -0.1f, +0.1f }, { 0.0f, 0.0f, 1.0f }, },

        { { +0.1f, +0.1f, +0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { -0.1f, -0.1f, +0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { +0.1f, -0.1f, +0.1f }, { 0.0f, 0.0f, 1.0f }, },

        // NOTE: Back
        { { +0.1f, -0.1f, -0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { -0.1f, -0.1f, -0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { -0.1f, +0.1f, -0.1f }, { 0.0f, 0.0f, 1.0f }, },

        { { +0.1f, -0.1f, -0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { -0.1f, +0.1f, -0.1f }, { 0.0f, 0.0f, 1.0f }, },
        { { +0.1f, +0.1f, -0.1f }, { 0.0f, 0.0f, 1.0f }, },

        // NOTE: Right
        { { +0.1f, -0.1f, -0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { +0.1f, +0.1f, +0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { +0.1f, -0.1f, +0.1f }, { 1.0f, 0.0f, 0.0f }, },

        { { +0.1f, -0.1f, -0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { +0.1f, +0.1f, -0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { +0.1f, +0.1f, +0.1f }, { 1.0f, 0.0f, 0.0f }, },

        // NOTE: Left
        { { -0.1f, -0.1f, -0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { -0.1f, +0.1f, +0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { -0.1f, +0.1f, -0.1f }, { 1.0f, 0.0f, 0.0f }, },

        { { -0.1f, -0.1f, -0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { -0.1f, -0.1f, +0.1f }, { 1.0f, 0.0f, 0.0f }, },
        { { -0.1f, +0.1f, +0.1f }, { 1.0f, 0.0f, 0.0f }, },

        // NOTE: Top
        { { +0.1f, +0.1f, +0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { +0.1f, +0.1f, -0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { -0.1f, +0.1f, -0.1f }, { 0.0f, 1.0f, 0.0f, }, },

        { { +0.1f, +0.1f, +0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { -0.1f, +0.1f, -0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { -0.1f, +0.1f, +0.1f }, { 0.0f, 1.0f, 0.0f, }, },

        // NOTE: Bottom
        { { -0.1f, -0.1f, -0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { +0.1f, -0.1f, -0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { +0.1f, -0.1f, +0.1f }, { 0.0f, 1.0f, 0.0f, }, },

        { { -0.1f, -0.1f, -0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { +0.1f, -0.1f, +0.1f }, { 0.0f, 1.0f, 0.0f, }, },
        { { -0.1f, -0.1f, +0.1f }, { 0.0f, 1.0f, 0.0f, }, },
    };

    memcpy(game->vertex_data_3d, unit_cube, sizeof(unit_cube));

    game->vertex_buffer_3d = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = unit_cube,
        .size = sizeof(unit_cube),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    game->setting_buffer_3d = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(setting_3d_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->vertex_shader_3d = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_3d,
        .bytecode_size = sizeof(vshader_3d),
        .stage = STAGE_VERTEX_SHADER,
    });

    game->pixel_shader_3d = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_3d,
        .bytecode_size = sizeof(pshader_3d),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->program_3d = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->vertex_shader_3d,
        .pixel_shader = game->pixel_shader_3d,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, offsetof(vertex3d_t, position), 0, 0, 0, 0 },
            { "COLOR", FORMAT_R32G32B32_FLOAT, offsetof(vertex3d_t, color), 0, 0, 0, 0 },
        },
        .attribute_count = 2,
    });

    game->pipeline_3d = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = true,
        .wireframe = false,
        .depth_test = true,
        .depth_write = true,
    });

    game->pipeline_3d_ = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = true,
        .wireframe = false,
        .depth_test = true,
        .depth_write = false,
    });

    game->sphere_vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_sphere_vertices,
        .size = sizeof(global_sphere_vertices),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    game->sphere_index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_sphere_indices,
        .size = array_count(global_sphere_indices),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_INDEX_BUFFER,
        .index_format = array_count(global_sphere_indices) > 0xFFFF ? FORMAT_R32_UINT : FORMAT_R16_UINT,
    });

    game->vertex_shader_sphere = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_sphere,
        .bytecode_size = sizeof(vshader_sphere),
        .stage = STAGE_VERTEX_SHADER,
    });

    game->pixel_shader_sphere = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_sphere,
        .bytecode_size = sizeof(pshader_sphere),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->program_sphere = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->vertex_shader_sphere,
        .pixel_shader = game->pixel_shader_sphere,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, offsetof(sphere_vertex_t, position), 0, 0, 0, 0 },
            { "NORMAL",   FORMAT_R32G32B32_FLOAT, offsetof(sphere_vertex_t, normal), 0, 0, 0, 0 },
        },
        .attribute_count = 2,
    });

    game->globe_param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(globe_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    game->globe_vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_ocean_vectors,
        .size = sizeof(global_ocean_vectors),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    game->globe_vertex_buffer1 = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_globe_points,
        .size = sizeof(global_globe_points),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    game->globe_index_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = global_globe_part_indices,
        .size = array_count(global_globe_part_indices),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_INDEX_BUFFER,
        .index_format = array_count(global_globe_part_indices) > 0xFFFF ? FORMAT_R32_UINT : FORMAT_R16_UINT,
    });

    game->vertex_shader_globe = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_globe,
        .bytecode_size = sizeof(vshader_globe),
        .stage = STAGE_VERTEX_SHADER,
    });

    game->pixel_shader_globe = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_globe,
        .bytecode_size = sizeof(pshader_globe),
        .stage = STAGE_PIXEL_SHADER,
    });

    game->program_globe = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->vertex_shader_globe,
        .pixel_shader = game->pixel_shader_globe,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, 0, 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });

    game->linear_sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_LINEAR,
        .address_u = TEXTURE_ADDRESS_CLAMP,
        .address_v = TEXTURE_ADDRESS_CLAMP,
        .address_w = TEXTURE_ADDRESS_CLAMP,
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
        .vertex_shader = game->vertex_shader,
        .pixel_shader = game->glow_pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32_FLOAT,    offsetof(vertex_t, position), 0, 0, 0, 0 },
            { "COLOR",    FORMAT_R32G32B32_FLOAT, offsetof(vertex_t, color),    0, 0, 0, 0 },
        },
        .attribute_count = 2,
    });

    game->glow_program_3d = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->vertex_shader_3d,
        .pixel_shader = game->glow_pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32B32_FLOAT, offsetof(vertex3d_t, position), 0, 0, 0, 0 },
            { "COLOR",    FORMAT_R32G32B32_FLOAT, offsetof(vertex3d_t, color),    0, 0, 0, 0 },
        },
        .attribute_count = 2,
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

    game->point_sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_POINT,
        .address_u = TEXTURE_ADDRESS_WRAP,
        .address_v = TEXTURE_ADDRESS_WRAP,
        .address_w = TEXTURE_ADDRESS_WRAP,
    });

    // game->font = graphics->create_font("Consolas", 24);
    // // game->font_color = graphics->create_font_color(0.8313f, 0.0f, 0.4705f, 1.0f);
    // game->font_color = graphics->create_font_color(0.38f, 0.38f, 0.38f, 1.0f);

    io->release_file_memory(io->read_file("..\\src\\game.c").data);

    // bmp_image_t water_normal_a = load_bmp_image(io, "../src/water_normal_a.bmp");
    // bmp_image_t water_normal_b = load_bmp_image(io, "../src/water_normal_b.bmp");
    // bmp_image_t land_sea_mask = load_bmp_image(io, "../src/land_sea_mask.bmp");

    // game->water_normal_a = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
    // {
    //     .format = FORMAT_R8G8B8A8_UNORM,
    //     .bind = BIND_SHADER_RESOURCE,
    //     .width = water_normal_a.width,
    //     .height = water_normal_a.height,
    // }, water_normal_a.data, water_normal_a.pitch);

    // game->water_normal_b = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
    // {
    //     .format = FORMAT_R8G8B8A8_UNORM,
    //     .bind = BIND_SHADER_RESOURCE,
    //     .width = water_normal_b.width,
    //     .height = water_normal_b.height,
    // }, water_normal_b.data, water_normal_b.pitch);

    // game->land_sea_mask = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
    // {
    //     .format = FORMAT_R8G8B8A8_UNORM,
    //     .bind = BIND_SHADER_RESOURCE,
    //     .width = land_sea_mask.width,
    //     .height = land_sea_mask.height,
    // }, land_sea_mask.data, land_sea_mask.pitch);
    
    // game->linear_wrap_sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    // {
    //     .filter = FILTER_MIN_MAG_MIP_LINEAR,
    //     .address_u = TEXTURE_ADDRESS_WRAP,
    //     .address_v = TEXTURE_ADDRESS_WRAP,
    //     .address_w = TEXTURE_ADDRESS_WRAP,
    // });

    // game->water_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    // {
    //     .size = sizeof(water_setting_t),
    //     .usage = USAGE_DYNAMIC,
    //     .bind = BIND_CONSTANT_BUFFER,
    // });

    // game->water_setting = (water_setting_t)
    // {
    //     .tiling_a = 12.0f,
    //     .speed_a = 0.007f,
    //     .tiling_b = 8.0f,
    //     .speed_b = -0.011f,
    //     .ripple_amp = 0.08f
    // };
}

update_function(update)
{

}

render_function(render)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    game_t* game = (game_t*)memory->permanent;

    resize_offscreen_buffer(platform, game);
#if 0
    // NOTE: Offscreen rendering pass.
    graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.005f, 0.005f, 0.005f, 0.0f }});
    // graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 1.0f, 0.9098f, 0.9098f, 0.0f }});
    {
        graphics->set_buffer(game->vertex_buffer, STAGE_VERTEX_SHADER, 0, sizeof(vertex_t), 0);
        graphics->set_program(game->program);
        graphics->set_pipeline(game->default_pipeline);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, array_count(game->vertex_data), 0);
    }
    graphics->end_pass();

    // NOTE: Glow mask pass.
    graphics->begin_pass(game->glow_mask_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.0f, 0.0f, 0.0f, 0.0f } });
    {
        graphics->set_buffer(game->vertex_buffer, STAGE_VERTEX_SHADER, 0, sizeof(vertex_t), 0);
        game->glow_mask_setting = (glow_mask_setting_t){ .glow_color = { 0.9964f, 0.8431f, 0.4941f, 0.0f } };
        graphics->update_buffer(game->glow_buffer, &game->glow_mask_setting, 0, sizeof(game->glow_mask_setting));
        graphics->set_buffer(game->glow_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_program(game->glow_program);
        graphics->set_pipeline(game->default_pipeline);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, array_count(game->vertex_data), 0);
    }
    graphics->end_pass();

#else
    // game->camera_position = rotation_y(0.3f, game->camera_position);
    // game->camera_position = rotation_x(0.9f, game->camera_position);
    static float earth_angle = 0.0f;
    f32 omega_radians_per_sec = 10.0f;
    earth_angle += omega_radians_per_sec * platform->delta_time;
    mat4x4 rotation_y = rotate_y(earth_angle);
    (void)rotation_y;

    f32 fov_y = 60.0f;
    f32 half_height = game->camera.position.z * tanf(fov_y * (f32)DEG2RAD * 0.5f);
    f32 half_width = half_height * (f32)platform->width / (f32)platform->height;
    f32 scale_x = half_width / (f32)PI;
    f32 scale_y = half_height / (0.5f * (f32)PI);
    
    game->setting_3d.world = rotation_y;
    game->setting_3d.world = m4x4d(1.0f);
    game->setting_3d.view = view_matrix(v3(0.0f, 1.0f, 0.0f), game->camera.position, v3(0.0f, 0.0f, 0.0f));
    game->setting_3d.projection = perspective_projection_fov_y(60.0f, platform->width / (f32)platform->height, 0.0001f, 100.0f);
    game->setting_3d.camera_world = v4v(game->camera.position, 0.0f);

    // NOTE: Offscreen rendering pass.
    // graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.005f, 0.005f, 0.005f, 0.0f }});
    // {
    //     game->setting_3d.view = view_matrix(v3(0.0f, 1.0f, 0.0f), game->camera_position, v3(0.0f, 0.0f, 0.0f));
    //     game->setting_3d.projection = projection_matrix_fov_y(60.0f, (f32)platform->width / (f32)platform->height, 0.1f, 100.0f);
    //     graphics->update_buffer(game->setting_buffer_3d, &game->setting_3d, 0, sizeof(game->setting_3d));
    //     graphics->set_buffer(game->setting_buffer_3d, STAGE_VERTEX_SHADER, 0, 0, 0);
    //     graphics->set_buffer(game->vertex_buffer_3d, STAGE_VERTEX_SHADER, 0, sizeof(vertex3d_t), 0);
    //     graphics->set_program(game->program_3d);
    //     graphics->set_pipeline(game->pipeline_3d);
    //     graphics->draw(TOPOLOGY_TRIANGLE_LIST, array_count(game->vertex_data_3d), 0);
    // }
    // graphics->end_pass();

    // // NOTE: Glow mask pass.
    // graphics->begin_pass(game->glow_mask_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.0f, 0.0f, 0.0f, 0.0f } });
    // {
    //     graphics->set_buffer(game->setting_buffer_3d, STAGE_VERTEX_SHADER, 0, 0, 0);
    //     graphics->set_buffer(game->vertex_buffer_3d, STAGE_VERTEX_SHADER, 0, sizeof(vertex3d_t), 0);
    //     game->glow_mask_setting = (glow_mask_setting_t){ .glow_color = { 0.9964f, 0.8431f, 0.4941f, 0.0f } };
    //     graphics->update_buffer(game->glow_buffer, &game->glow_mask_setting, 0, sizeof(game->glow_mask_setting));
    //     graphics->set_buffer(game->glow_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
    //     graphics->set_program(game->glow_program_3d);
    //     graphics->set_pipeline(game->pipeline_3d);
    //     graphics->draw(TOPOLOGY_TRIANGLE_LIST, array_count(game->vertex_data_3d), 0);
    // }
    // graphics->end_pass();

    graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.005f, 0.005f, 0.005f, 0.0f },
                                                                          .clear_depth = true, .clear_depth_value = 1.0f });
    {
        graphics->update_buffer(game->setting_buffer_3d, &game->setting_3d, 0, sizeof(game->setting_3d));
        graphics->set_buffer(game->setting_buffer_3d, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(game->setting_buffer_3d, STAGE_PIXEL_SHADER, 0, 0, 0);
        graphics->set_buffer(game->sphere_vertex_buffer, STAGE_VERTEX_SHADER, 0, sizeof(sphere_vertex_t), 0);
        graphics->set_buffer(game->sphere_index_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_program(game->program_sphere);
        graphics->set_pipeline(game->pipeline_3d);
        // graphics_sampler_t samplers[] = { game->linear_wrap_sampler };
        // graphics->set_samplers(STAGE_PIXEL_SHADER, samplers, array_count(samplers), 0);
        // graphics_texture_t srvs[] = { game->water_normal_a, game->water_normal_b };
        // graphics->set_srvs(STAGE_PIXEL_SHADER, srvs, array_count(srvs), 0);
        graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, array_count(global_sphere_indices), 0, 0);
    }
    graphics->end_pass();
    
    // graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = false });
    // {
    //     graphics->update_buffer(game->setting_buffer_3d, &game->setting_3d, 0, sizeof(game->setting_3d));
    //     graphics->set_buffer(game->setting_buffer_3d, STAGE_VERTEX_SHADER, 0, 0, 0);
    //     graphics->set_buffer(game->globe_vertex_buffer, STAGE_VERTEX_SHADER, 0, sizeof(vec3), 0);
    //     graphics->set_program(game->program_globe);
    //     graphics->set_pipeline(game->pipeline_3d_);
    //     graphics->draw(TOPOLOGY_POINT_LIST, array_count(global_ocean_vectors), 0);
    // }
    // graphics->end_pass();

    static f32 globe_shape = 1.0f;
    static f32 globe_shape_morph_speed = 0.033f;

    if (platform->input->keys[KEY_T].action == KEY_ACTION_RELEASE)
    {
        globe_shape_morph_speed *= -1.0f;
    }

    globe_shape += globe_shape_morph_speed;
    globe_shape = clamp(0.0f, globe_shape, 1.0f);
    
    globe_param_t globe_param =
    {
        .shape = globe_shape,
        .scale = v2(scale_x, scale_y),
    };

    graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = false });
    {
        graphics->update_buffer(game->setting_buffer_3d, &game->setting_3d, 0, sizeof(game->setting_3d));
        graphics->update_buffer(game->globe_param_buffer, &globe_param, 0, sizeof(globe_param));
        graphics->set_buffer(game->setting_buffer_3d, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_buffer(game->globe_param_buffer, STAGE_VERTEX_SHADER, 1, 0, 0);
        graphics->set_buffer(game->globe_vertex_buffer1, STAGE_VERTEX_SHADER, 0, sizeof(vec3), 0);
        graphics->set_buffer(game->globe_index_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
        graphics->set_program(game->program_globe);
        graphics->set_pipeline(game->pipeline_3d_);
        // graphics->draw(TOPOLOGY_POINT_LIST, array_count(global_globe_vectors), 0);
        graphics->draw_indexed(TOPOLOGY_LINE_LIST, array_count(global_globe_part_indices), 0, 0);
    }
    graphics->end_pass();

    // graphics->begin_pass(game->glow_mask_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.0f, 0.0f, 0.0f, 0.0f } });
    // {
    //     graphics->set_buffer(game->setting_buffer_3d, STAGE_VERTEX_SHADER, 0, 0, 0);
    //     graphics->set_buffer(game->globe_vertex_buffer, STAGE_VERTEX_SHADER, 0, sizeof(vec3), 0);
    //     game->glow_mask_setting = (glow_mask_setting_t){ .glow_color = { 0.9964f, 0.8431f, 0.4941f, 0.0f } };
    //     graphics->update_buffer(game->glow_buffer, &game->glow_mask_setting, 0, sizeof(game->glow_mask_setting));
    //     graphics->set_buffer(game->glow_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
    //     graphics->set_program(game->glow_program_3d);
    //     graphics->set_pipeline(game->pipeline_3d);
    //     // graphics->draw(TOPOLOGY_LINE_LIST, array_count(global_globe_vectors), 0);
    //     graphics->draw_indexed(TOPOLOGY_LINE_LIST, array_count(global_globe_part_indices), 0, 0);
    // }
    // graphics->end_pass();
#endif
    
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
    
    // graphics->begin_draw();
    // {
    //     char frame_ms_text[32] = { 0 };
    //     size_t frame_ms_length = 0;

    //     if ((frame_ms_length = snprintf(frame_ms_text, sizeof(frame_ms_text), "%.2f ms", platform->delta_time * 1000)) > 0)
    //     {
    //         graphics->draw_text(game->font, game->font_color, TEXT_ALIGNMENT_TRAILING,
    //                             0.0f - 8.0f, 0.0f + 8.0f, (f32)platform->width, (f32)platform->height,
    //                             frame_ms_text, frame_ms_length);   
    //     }
        
    // }
    // graphics->end_draw();
}
