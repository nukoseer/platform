#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <intrin.h>
#include "utils.h"
#include "platform.h"
#include "maths.h"

#include "../shader/vertex_shader_shape_ui.h"
#include "../shader/pixel_shader_shape_ui.h"

#include "sphere_data.inl"

#include "memory_arena.c"
#include "ray.c"
#include "country.c"
#include "blur.c"
#include "composite.c"

#include "ui.c"

#include "fuzzy_match.c"

typedef enum theme_type_t
{
    THEME_TYPE_LIGHT,
    THEME_TYPE_DARK,

    THEME_TYPE_COUNT,
} theme_type_t;

typedef struct theme_t
{
    vec4 bg_color;
    vec4 fg_color;
    vec4 sphere_grid_color;
    vec4 highlight_color;
    vec4 font_color;
    bool dark_mode;
} theme_t;

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

typedef struct graphics_state_t
{
    graphics_texture_t offscreen_scene;
    graphics_texture_t offscreen_scene_msaa;
    graphics_texture_t offscreen_depth_msaa;
    graphics_target_t offscreen_target_msaa;

    graphics_sampler_t point_sampler;
    graphics_sampler_t linear_sampler;

    graphics_pipeline_t no_alpha_pipeline;
    graphics_pipeline_t depth_test_no_alpha_pipeline;
    graphics_pipeline_t depth_test_write_pipeline;
    graphics_pipeline_t depth_test_pipeline;

    blur_graphics_t blur_graphics;
    composite_graphics_t composite_graphics;
} graphics_state_t;

#include "earth.c"

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
    vec4 color;
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
    f32 lift;
    f32 _pad[3];
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

typedef struct shape_ui_param_t
{
    vec4 color;
    vec2 viewport_size;
    vec2 center;
    f32 zoom;
    f32 _pad[3];
} shape_ui_param_t;

typedef struct shape_info_ui_t
{
    graphics_buffer_t param_buffer;
    graphics_buffer_t vertex_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
    shape_ui_param_t param;
} shape_info_ui_t;

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
    f32 glow;
} glow_merge_param_t;

typedef struct glow_mask_param_t
{
    vec3 glow_color;
    f32 glow;
} glow_mask_param_t;

typedef struct glow_blur_param_t
{
    f32 inverse_dst_size[2];
    f32 direction[2];
} glow_blur_param_t;

typedef struct skybox_vertex_t
{
    vec3 position;
} skybox_vertex_t;

typedef struct game_t
{
    memory_arena_t* memory_arena;

    graphics_state_t graphics_state;
    
    camera_t camera;
    theme_t themes[THEME_TYPE_COUNT];
    theme_type_t current_theme_type;

    earth_t* earth;
    
#if FONT_ENABLE
    graphics_2d_font_t font_12;
    graphics_2d_font_t font_16;
    vec4 font_color;
#endif

    country_data_t country_data;
    u8 country_index;
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

// static void resize_offscreen_buffer(platform_t* platform, game_t* game)
// {
//     graphics_t* graphics = platform->graphics;
//     bool resized = platform->resized;
//     bool is_valid = graphics->is_valid_target(game->offscreen_target_msaa);

//     if (is_valid && resized)
//     {
//         graphics->delete_target(game->offscreen_target_msaa);
//         graphics->delete_target(game->glow_mask_msaa_target);
//         graphics->delete_target(game->merge_target_msaa);
//         graphics->delete_target(game->glow_a_target);
//         graphics->delete_target(game->glow_b_target);
//         graphics->delete_texture_2d(game->offscreen_scene);
//         graphics->delete_texture_2d(game->offscreen_scene_msaa);
//         graphics->delete_texture_2d(game->offscreen_depth_msaa);
//         graphics->delete_texture_2d(game->merge_scene);
//         graphics->delete_texture_2d(game->merge_scene_msaa);
//         graphics->delete_texture_2d(game->glow_mask_msaa);
//         graphics->delete_texture_2d(game->glow_mask);
//         graphics->delete_texture_2d(game->glow_a);
//         graphics->delete_texture_2d(game->glow_b);
//     }

//     if (!is_valid || resized)
//     {
//         game->offscreen_scene = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = platform->width,
//             .height = platform->height,
//         }, 0, 0);
        
//         game->offscreen_scene_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = platform->width,
//             .height = platform->height,
//             .sample_count = 8,
//         }, 0, 0);

//         game->offscreen_depth_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_D24_UNORM_S8_UINT,
//             .bind = BIND_DEPTH_STENCIL,
//             .width = platform->width,
//             .height = platform->height,
//             .sample_count = 8,
//         }, 0, 0);

//         game->merge_scene_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = platform->width,
//             .height = platform->height,
//             .sample_count = 8,
//         }, 0, 0);

//         game->merge_scene = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = platform->width,
//             .height = platform->height,
//         }, 0, 0);

//         game->glow_mask_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = platform->width,
//             .height = platform->height,
//             .sample_count = 8,
//         }, 0, 0);

//         game->glow_mask = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = platform->width,
//             .height = platform->height,
//         }, 0, 0);

//         game->glow_a = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             // FORMAT_R8G8B8A8_UNORM,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = (u32)(platform->width * 0.5f),
//             .height = (u32)(platform->height * 0.5f),
//         }, 0, 0);

//         game->glow_b = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
//         {
//             .format = FORMAT_R16G16B16A16_FLOAT,
//             //FORMAT_R8G8B8A8_UNORM,
//             .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
//             .width = (u32)(platform->width * 0.5f),
//             .height = (u32)(platform->height * 0.5f),
//         }, 0, 0);

//         // NOTE: Create RenderTargetView for offscreen_scene to render.
//         game->offscreen_target_msaa = graphics->create_target(&(graphics_target_desc_t){ .color = game->offscreen_scene_msaa, .depth = game->offscreen_depth_msaa });
//         game->merge_target_msaa = graphics->create_target(&(graphics_target_desc_t){ .color = game->merge_scene_msaa, .depth = game->offscreen_depth_msaa });
//         game->glow_mask_msaa_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->glow_mask_msaa, .depth = game->offscreen_depth_msaa });
//         game->glow_a_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->glow_a });
//         game->glow_b_target = graphics->create_target(&(graphics_target_desc_t){ .color = game->glow_b });
//     }
// }

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

static void init_shape_ui(const graphics_t* graphics, shape_info_ui_t* shape_info_ui, country_data_t* country_data)
{
    shape_info_ui->param_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(shape_ui_param_t),
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    shape_info_ui->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = country_data->query.border_points,
        .size = country_data->query.border_point_count * sizeof(country_border_point_t),
        .usage = USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    shape_info_ui->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader_shape_ui,
        .bytecode_size = sizeof(vshader_shape_ui),
        .stage = STAGE_VERTEX_SHADER,
    });

    shape_info_ui->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader_shape_ui,
        .bytecode_size = sizeof(pshader_shape_ui),
        .stage = STAGE_PIXEL_SHADER,
    });

    shape_info_ui->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = shape_info_ui->vertex_shader,
        .pixel_shader = shape_info_ui->pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32_FLOAT, offsetof(country_border_point_t, lonlat), 0, 0, 0, 0 },
        },
        .attribute_count = 1,
    });
}

static void init_theme(game_t* game)
{
    // NOTE: Light and dark themes.
    game->themes[THEME_TYPE_LIGHT] = (theme_t)
    {
        .bg_color = v4(0.9450f, 0.9215f, 0.8941f, 1.0f),
        .fg_color = v4(0.3098f, 0.2784f, 0.2235f, 1.0f),
        .sphere_grid_color = v4(0.55f, 0.55f, 0.55f, 1.0f),
        // .highlight_color = v4(0.4156f, 0.2823f, 0.2f, 1.0f),
        .highlight_color = v4(0.3098f, 0.2784f, 0.2235f, 1.0f),
        .font_color = v4(0.2745f, 0.2431f, 0.2941f, 1.0f),
        .dark_mode = false,
    };

    game->themes[THEME_TYPE_DARK] = (theme_t)
    {
        .bg_color = v4(0.005f, 0.005f, 0.005f, 1.0f),
        .fg_color = v4(0.1058f, 0.9921f, 0.6117f, 1.0f),
        .sphere_grid_color = v4(0.008f, 0.008f, 0.008f, 1.0f),
        // .highlight_color = v4(1.0f, 1.0f, 1.0f, 1.0f),
        .highlight_color = v4(0.1058f, 0.9921f, 0.6117f, 1.0f),
        .font_color = v4(0.1058f, 0.9921f, 0.6117f, 1.0f),
        .dark_mode = true,
    };

    game->current_theme_type = THEME_TYPE_LIGHT;
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
    thread_pool_t* thread_pool = platform->thread_pool;
    game_t* game = (game_t*)memory->permanent;
    memory_arena_t* memory_arena = ma_initialize(memory->permanent + sizeof(game_t), memory->permanent_size - sizeof(game_t));
    game->memory_arena = memory_arena;

    graphics_state_t* graphics_state = &game->graphics_state;

    blur_graphics_create(graphics, &graphics_state->blur_graphics);
    composite_graphics_create(graphics, &graphics_state->composite_graphics);

    graphics_state->no_alpha_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t){ 0 });

    graphics_state->depth_test_no_alpha_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t){ .depth_test = true, });

    graphics_state->depth_test_write_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = true,
        .depth_test = true,
        .depth_write = true,
        .blend = BLEND_ALPHA,
    });

    graphics_state->depth_test_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = false,
        .depth_test = true,
        .depth_write = false,
        .blend = BLEND_ALPHA,
    });

    graphics_state->linear_sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_LINEAR,
        .address_u = TEXTURE_ADDRESS_CLAMP,
        .address_v = TEXTURE_ADDRESS_CLAMP,
        .address_w = TEXTURE_ADDRESS_CLAMP,
    });

    graphics_state->point_sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_POINT,
        .address_u = TEXTURE_ADDRESS_WRAP,
        .address_v = TEXTURE_ADDRESS_WRAP,
        .address_w = TEXTURE_ADDRESS_WRAP,
    });

    init_camera(&game->camera, v3(0.0f, 0.0f, 2.2f), v3(0.0f, 0.0f, 0.0f),
                60.0f, (f32)platform->width / (f32)platform->height);
    game->earth = earth_init(memory_arena, graphics, graphics_state, io);
    // init_shape_ui(graphics, &game->shape_info_ui, &game->country_data);
    init_theme(game);
    ui_init(memory_arena);

#if FONT_ENABLE
    game->font_12 = graphics->create_font("IosevkaTerm NFM", 12);
    game->font_16 = graphics->create_font("IosevkaTerm NFM", 16);
#endif
}

static theme_t* get_current_theme(game_t* game)
{
    return &game->themes[game->current_theme_type];
}

static void update_theme(game_t* game, theme_type_t theme_type)
{
    theme_type_t new_theme_type = THEME_TYPE_LIGHT;
    
    switch (theme_type)
    {
        case THEME_TYPE_LIGHT:
        {
            new_theme_type = THEME_TYPE_DARK;
        } break;

        case THEME_TYPE_DARK:
        {
            new_theme_type = THEME_TYPE_LIGHT;
        } break;

        default:
        {
            assert("[THEME] Invalid theme type.");
        }
    }

    game->current_theme_type = new_theme_type;
}

// update_function(update)
// {
//     memory_t* memory = platform->memory;
//     graphics_t* graphics = platform->graphics;
//     input_t* input = platform->input;
//     game_t* game = (game_t*)memory->permanent;
//     camera_t* camera = &game->camera;

//     if (input->wheel != 0.0f)
//     {
//         camera->position.z += 3.0f * platform->delta_time * -input->wheel;
//     }

//     update_camera(camera);

//     if (game->shape_value == 1.0f)
//     {
//         // earth_rotation(input, platform->delta_time);
//     }

//     // game->country_index = earth_find_country_index_under_cursor(platform, game);

//     if (country_is_valid_index(game->country_index))
//     {
//         country_name_t country_name = country_get_name(game->country_index);

//         if (country_name.name)
//         {
//             fprintf(stderr, "\rcountry: %s, id: %u, yaw: %f, pitch: %f",
//                     country_name.name, game->country_index,
//                     global_earth_yaw, global_earth_pitch);
//         }
//     }

//     game->glow_intensity = 2.0f;
    
//     if (get_current_theme(game)->dark_mode)
//     {
//         game->glow_intensity = 0.3f;
//     }

//     if (input_is_key_released(input, KEY_C))
//     {
//         update_theme(game, game->current_theme_type);
//     }
    
//     if (input_is_key_released(input, KEY_I))
//     {
//         game->invert = game->invert == 0.0f ? 1.0f : 0.0f;
//     }

//     if (input_is_key_released(input, KEY_V))
//     {
//         game->vignette = game->vignette == 0.0f ? 1.0f : 0.0f;
//     }
    
//     theme_t* theme = get_current_theme(game);
    
// #if FONT_ENABLE
//     ui_begin(graphics, input, platform->delta_time, (f32)platform->width, (f32)platform->height);
//     ui_push_color(theme->bg_color);
//     ui_push_font(game->font_12, graphics->get_font_pixel_size(game->font_12));
//     ui_push_font_color(theme->font_color);
//     ui_push_text_wrap(UI_TEXT_WRAP_WORD);

//     ui_next_size(ui_pixel(430.0f, 1.0f), ui_children(1.0f));
//     ui_next_padding(4.0f); ui_next_axis(ui_axis_y());
//     ui_next_border(1.0f, theme->fg_color);
//     ui_next_flags(UI_FLAG_BACKGROUND);
//     ui_widget_group("container", 10.0f, 40.0f)
//     {
//         ui_push_size(ui_content(1.0f), ui_content(1.0f));
//         ui_push_padding(4.0f);
//         ui_push_flags(UI_FLAG_BACKGROUND);
//         ui_push_text_alignment(ui_align_center());

//         ui_next_anchored(UI_ANCHOR_TOP_LEFT, UI_ANCHOR_CENTER_LEFT, 16.0f, 2.0f);
//         ui_widget_text("country-data", "COUNTRY DATA");

//         ui_next_anchored(UI_ANCHOR_TOP_RIGHT, UI_ANCHOR_CENTER_RIGHT, -16.0f, 2.0f);
//         ui_next_border(1.0f, theme->fg_color);
//         ui_widget_text("active", "[ACTIVE]");

//         ui_pop_text_alignment();
//         ui_pop_flags();
//         ui_pop_padding();
//         ui_pop_size();

//         ui_next_size(ui_percent(1.0f, 1.0f), ui_children(1.0f));
//         ui_next_padding(8.0f); ui_next_flags(UI_FLAG_BACKGROUND);
//         ui_next_border(1.0f, theme->fg_color);
//         ui_widget_group("inner-container", 0.0f, 0.0f)
//         {
//             ui_widget_spacer(ui_pixel(8.0f, 1.0f));
            
//             ui_next_size(ui_percent(1.0f, 1.0f), ui_children(1.0f));
//             ui_widget_named_row("query-row")
//             {
//                 ui_next_size(ui_percent(1.0f, 1.0f), ui_children(1.0f));
//                 ui_widget_named_column("query-column")
//                 {
//                     ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
//                     ui_next_text_alignment(ui_align_leading());
//                     ui_widget_text("query", "> QUERY: country.geo");
                    
//                     ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
//                     ui_next_text_alignment(ui_align_leading());
//                     ui_widget_text("region-id", "> REGION ID: 0xA4F2");
//                 }
//             }

//             ui_widget_spacer(ui_pixel(8.0f, 1.0f));
        
//             ui_next_size(ui_percent(1.0f, 1.0f), ui_em(8.0f, 1.0f));
//             ui_widget_named_row("country-shape-row")
//             {
//                 ui_next_size(ui_percent(1.0f, 0.0f), ui_percent(1.0f, 1.0f));
//                 ui_widget_named_column("country-shape-column")
//                 {
//                     ui_next_size(ui_percent(1.0f, 1.0f), ui_percent(1.0f, 1.0f));
//                     ui_next_padding(4.0f); ui_next_border(1.0f, theme->fg_color);
//                     ui_widget_group("country-shape-group", 0.0f, 0.0f)
//                     {
//                         ui_next_size(ui_percent(1.0f, 1.0f), ui_percent(1.0f, 1.0f));
//                         ui_next_flags(UI_FLAG_CUSTOM);
//                         ui_widget("country-shape");
//                     }
//                 }

//                 ui_widget_spacer(ui_pixel(8.0f, 1.0f));
                        
//                 ui_next_size(ui_percent(1.0f, 0.0f), ui_children(1.0f));
//                 ui_widget_named_column("country-block-column")
//                 {
//                     ui_next_size(ui_percent(1.0f, 0.0f), ui_children(1.0f));
//                     ui_widget_named_row("country-block-row")
//                     {
//                         ui_next_size(ui_percent(1.0f, 0.0f), ui_children(1.0f));
//                         ui_widget_group("country-block", 0, 0)
//                         {
//                             country_name_t country_name = country_get_name(game->country_index);
                        
//                             ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
//                             ui_next_text_alignment(ui_align_leading());
//                             ui_widget_text("value", country_name.name ? country_name.name : "Not Selected");

//                             ui_next_size(ui_percent(1.0f, 1.0f), ui_pixel(1.0f, 1.0f));
//                             ui_next_color(theme->font_color); ui_next_flags(UI_FLAG_BACKGROUND);
//                             ui_widget("separator");

//                             ui_widget_spacer(ui_pixel(8.0f, 1.0f));

//                             ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
//                             ui_next_text_alignment(ui_align_leading());
//                             ui_widget_text("pop", "POP:  67.4M");

//                             ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
//                             ui_next_text_alignment(ui_align_leading());
//                             ui_widget_text("area", "AREA: 643K km2");

//                             ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
//                             ui_next_text_alignment(ui_align_leading());
//                             ui_widget_text("gov", "GOV: REPUBLIC");
//                         }
//                     }
//                 }
//             }

//             ui_widget_spacer(ui_pixel(8.0f, 1.0f));
            
//             ui_next_size(ui_percent(1.0f, 1.0f), ui_em(1.25f, 1.0f));
//             ui_widget_named_column("status-column")
//             {
//                 ui_next_size(ui_percent(1.0f, 1.0f), ui_percent(1.0f, 1.0f));
//                 ui_widget_named_row("status-row")
//                 {
//                     ui_next_size(ui_content(1.0f), ui_percent(1.0f, 1.0f));
//                     ui_next_text_alignment(ui_align_center());
//                     ui_widget_text("status", "STATUS:");

//                     ui_widget_spacer(ui_pixel(8.0f, 1.0f));

//                     ui_next_size(ui_percent(1.0f, 0.0f), ui_percent(1.0f, 0.0f));
//                     ui_widget_named_column("status-bar-column")
//                     {
//                         ui_next_size(ui_percent(1.0f, 1.0f), ui_percent(1.0f, 0.0f));
//                         ui_next_border(1.0f, theme->fg_color);
//                         ui_widget_group("status-bar", 0, 0)
//                         {
//                             ui_next_size(ui_percent(0.8f, 1.0f), ui_percent(1.0f, 0.0f));
//                             ui_next_flags(UI_FLAG_BACKGROUND);
//                             ui_next_color(theme->fg_color);
//                             ui_widget("status-fill");
//                         }
//                     }

//                     ui_widget_spacer(ui_pixel(8.0f, 1.0f));

//                     ui_next_size(ui_content(1.0f), ui_percent(1.0f, 1.0f));
//                     ui_next_text_alignment(ui_align_center());
//                     ui_widget_text("percent", "80%");
//                 }
//             }

//             ui_widget_spacer(ui_pixel(8.0f, 1.0f));
//         }

//         ui_next_size(ui_content(1.0f), ui_content(1.0f)); ui_next_padding(4.0f);
//         ui_next_anchored(UI_ANCHOR_BOTTOM_RIGHT, UI_ANCHOR_CENTER_RIGHT, -16.0f, -2.0f);
//         ui_next_flags(UI_FLAG_BACKGROUND); ui_next_text_alignment(ui_align_center());
//         ui_widget_text("number-code", "0x7F.A1.42");
//     }
    
//     static ui_text_edit_t search_text_edit = { 0 };
//     f32 search_container_width = platform->width * 0.25f;
//     bool show_icon = search_container_width >= 200.0f;
//     ui_next_size(ui_pixel(search_container_width, 1.0f), ui_children(1.0f));
//     ui_next_axis(ui_axis_x());
//     ui_next_border(1.0f, theme->fg_color);
//     ui_next_flags(UI_FLAG_BACKGROUND);
//     ui_next_padding(4.0f);
//     ui_widget_group("search-bar-container", (platform->width - search_container_width) * 0.5f, 40.0f)
//     {
//         ui_next_size(ui_content(1.0f), ui_content(1.0f));
//         ui_next_padding(4.0f);
//         ui_next_flags(UI_FLAG_BACKGROUND);
//         ui_next_anchored(UI_ANCHOR_TOP_LEFT, UI_ANCHOR_CENTER_LEFT, 16.0f, 2.0f);
//         ui_widget_text("search-header", "SEARCH");
    
//         ui_next_size(ui_percent(1.0f, 1.0f), ui_children(1.0f));
//         ui_next_border(1.0f, theme->fg_color);
//         ui_next_padding(4.0f);
//         ui_widget_named_column("search-bar-column")
//         {
//             ui_next_size(ui_percent(1.0f, 1.0f), ui_em(2.0f, 1.0f));
//             ui_widget_named_row("search-bar-row")
//             {
//                 if (show_icon)
//                 {
//                     ui_next_size(ui_em(1.5f, 1.0f), ui_percent(1.0f, 1.0f));
//                     ui_next_flags(UI_FLAG_CLICKABLE);
//                     ui_next_text_alignment(ui_align_center());
//                     ui_widget_text("icon-field", ">");
//                 }

//                 ui_signal_t cancel_last_signal = ui_widget_last_signal("cancel-field");
                
//                 if (cancel_last_signal.clicked)
//                 {
//                     search_text_edit.text[0] = '\0';
//                     search_text_edit.cursor = 0;
//                     search_text_edit.length = 0;
//                 }
                
//                 ui_next_size(ui_percent(1.0f, 0.0f), ui_percent(1.0f, 1.0f));
//                 ui_next_flags(UI_FLAG_CLICKABLE | UI_FLAG_BACKGROUND);
//                 ui_next_text_wrap(UI_TEXT_WRAP_NONE);
//                 ui_next_text_alignment((ui_alignment_t){ UI_ALIGNMENT_LEADING, UI_ALIGNMENT_CENTER });
//                 ui_widget_text_edit("search-bar", &search_text_edit);

//                 ui_next_size(ui_em(1.5f, 1.0f), ui_percent(1.0f, 1.0f));
//                 ui_next_flags(UI_FLAG_CLICKABLE);
//                 ui_next_border(1.0f, theme->fg_color);
//                 ui_next_show_border(cancel_last_signal.hovering);
//                 ui_next_text_alignment(ui_align_center());
//                 ui_widget_text("cancel-field", "-");
//             }

//             if (search_text_edit.length > 0)
//             {
//                 i32 country_indices[array_count(global_shape_country_names)] = { 0 };
//                 i32 country_scores[array_count(global_shape_country_names)] = { 0 };
//                 i32 country_score_count = 0;

//                 for (i32 i = 0; i < array_count(global_shape_country_names); ++i)
//                 {
//                     country_name_t country_name = global_shape_country_names[i];
//                     i32 score = 0;

//                     if (fuzzy_match(game->memory_arena, search_text_edit.text, search_text_edit.length, country_name.name, country_name.length, &score))
//                     {
//                         i32 j = country_score_count++;

//                         if (j > 0 && score > country_scores[j - 1])
//                         {
//                             country_scores[j] = country_scores[j - 1];
//                             country_indices[j] = country_indices[j - 1];
//                             j--;
//                         }

//                         country_scores[j] = score;
//                         country_indices[j] = i;
//                     }
//                 }

//                 if (country_score_count > 0)
//                 {
//                     ui_next_size(ui_percent(1.0f, 1.0f), ui_pixel(1.0f, 1.0f));
//                     ui_next_color(v4v(theme->font_color.rgb, 0.8f)); ui_next_flags(UI_FLAG_BACKGROUND);
//                     ui_widget_build_from_key(ui_key_zero());

//                     ui_next_size(ui_percent(1.0f, 1.0f), country_score_count <= 10 ? ui_children(1.0f) : ui_em(20.0f, 1.0f));
//                     ui_next_axis(ui_axis_y());
//                     ui_next_padding(4.0f);
//                     ui_next_flags(UI_FLAG_BACKGROUND | UI_FLAG_SCROLLABLE_Y);
//                     ui_widget_named_column("search-results-column")
//                     {
//                         ui_widget_scrollbar(0);

//                         for (i32 i = 0; i < clamp_i32(0, country_score_count, 20); ++i)
//                         {
//                             country_name_t country_name = global_shape_country_names[country_indices[i]];

//                             if (i != 0)
//                             {
//                                 ui_next_size(ui_percent(0.98f, 1.0f), ui_pixel(1.0f, 1.0f));
//                                 ui_next_color(v4v(theme->font_color.rgb, 0.8f)); ui_next_flags(UI_FLAG_BACKGROUND);
//                                 ui_widget_build_from_key(ui_key_zero());
//                             }

//                             ui_signal_t last_signal = ui_widget_last_signal(country_name.name);
//                             ui_next_border(1.0f, v4v(theme->font_color.rgb, 0.8f));
//                             ui_next_show_border(last_signal.hovering);
//                             ui_next_color(last_signal.hovering ? v4v(theme->font_color.rgb, 0.05f) : theme->bg_color);
//                             ui_next_size(ui_percent(0.98f, 1.0f), ui_em(2.0f, 1.0f));
//                             ui_next_flags(UI_FLAG_CLICKABLE | UI_FLAG_BACKGROUND);
//                             ui_next_text_alignment((ui_alignment_t){ UI_ALIGNMENT_LEADING, UI_ALIGNMENT_CENTER });
//                             ui_next_padding(4.0f);
//                             ui_widget_text(country_name.name, country_name.name);
//                         }   
//                     }
//                 }
//             }
//         }
//     }

//     ui_pop_text_wrap();
//     ui_pop_font_color();
//     ui_pop_font();
//     ui_pop_color();
    
//     ui_end();
// #endif
// }

// render_function(render)
// {
//     memory_t* memory = platform->memory;
//     graphics_t* graphics = platform->graphics;
//     game_t* game = (game_t*)memory->permanent;
//     camera_t* camera = &game->camera;
//     theme_t* theme = get_current_theme(game);

//     // resize_offscreen_buffer(platform, game);

//     game->transform_param.world = m4x4d(1.0f);
//     game->transform_param.view = camera->view;
//     game->transform_param.projection = camera->projection;
//     game->transform_param.camera_world = v4v(camera->position, 0.0f);
    
//     graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t)
//     {
//         .clear_color = true,
//         .clear_rgba = v4v(srgb_to_linear(theme->bg_color.rgb), theme->bg_color.a),
//         .clear_depth = true, .clear_depth_value = 1.0f
//     });
//     // {
//     //     skybox_param_t skybox_param =
//     //     {
//     //         .view_no_translation = camera->view_no_translation,
//     //         .projection = camera->projection,
//     //         .yaw = global_earth_yaw,
//     //         .pitch = global_earth_pitch,
//     //         .shape = game->shape_value,
//     //     };

//     //     graphics->update_buffer(game->skybox_info.param_buffer, &skybox_param, 0, sizeof(skybox_param_t));
//     //     graphics->set_buffer(game->skybox_info.param_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
//     //     graphics->set_buffer(game->skybox_info.param_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
//     //     graphics->set_vertex_buffer(game->skybox_info.vertex_buffer, 0, sizeof(skybox_vertex_t), 0);
//     //     graphics->set_index_buffer(game->skybox_info.index_buffer, 0);
//     //     graphics->set_program(game->skybox_info.program);
//     //     graphics->set_srvs(STAGE_PIXEL_SHADER, &game->skybox_info.texture, 1, 0);
//     //     graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
//     //     graphics->set_pipeline(game->pre_multiplied_pipeline);
//     //     graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, game->skybox_info.index_buffer.size, 0, 0);
//     // }
//     graphics->end_pass();
    
//     graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t){ 0 });
//     {
//         sphere_info_t* sphere_info = &game->sphere_info;
//         // sphere_info->param.color = v4(0.0f, 0.0f, 0.0f, game->shape_value);
//         sphere_info->param.color = v4v(srgb_to_linear(theme->bg_color.rgb), game->shape_value);
//         graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
//         graphics->update_buffer(sphere_info->param_buffer, &sphere_info->param, 0, sizeof(sphere_info->param));
//         graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
//         graphics->set_buffer(game->transform_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
//         graphics->set_buffer(sphere_info->param_buffer, STAGE_VERTEX_SHADER, 1, 0, 0);
//         graphics->set_buffer(sphere_info->param_buffer, STAGE_PIXEL_SHADER, 1, 0, 0);
//         graphics->set_vertex_buffer(sphere_info->vertex_buffer, 0, sizeof(vec3), 0);
//         graphics->set_index_buffer(sphere_info->index_buffer, 0);
//         graphics->set_program(sphere_info->program);
//         graphics->set_pipeline(game->d_test_write_pipeline);
//         graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, sphere_info->index_buffer.size, 0, 0);
//     }
//     graphics->end_pass();

//     shape_info_t* shape_info = &game->shape_info;
//     shape_param_t* shape_param = &shape_info->param;

//     f32 half_height = camera->position.z * tanf(camera->fov_y * (f32)DEG2RAD * 0.5f);
//     f32 half_width = half_height * camera->aspect_ratio;
//     f32 scale_x = half_width / (f32)PI;
//     f32 scale_y = half_height / (0.5f * (f32)PI);
    
//     shape_param->shape = game->shape_value;
//     shape_param->yaw = global_earth_yaw;
//     shape_param->pitch = global_earth_pitch;
//     shape_param->line_thickness = 1.0f;
//     shape_param->scale = v2(scale_x, scale_y);
//     shape_param->viewport_size = v2((f32)game->offscreen_scene.width, (f32)game->offscreen_scene.height);
//     shape_param->lift = 0.0f;

//     graphics->begin_pass(game->offscreen_target_msaa, &(graphics_pass_desc_t){ 0 });
//     {
//         shape_param->color = theme->sphere_grid_color;

//         sphere_info_t* sphere_info = &game->sphere_info;

//         graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
//         graphics->update_buffer(shape_info->param_buffer, &shape_info->param, 0, sizeof(shape_info->param));
//         graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
//         graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);
//         graphics->set_vertex_buffer(sphere_info->vertex_buffer, 0, sizeof(vec3), 0);
//         graphics->set_index_buffer(sphere_info->index_buffer, 0);
//         graphics->set_program(sphere_info->program_grid);
//         graphics->set_pipeline(game->d_test_pipeline);
//         graphics->draw_indexed(TOPOLOGY_LINE_LIST, sphere_info->index_buffer.size, 0, 0);
//     }
//     graphics->end_pass();

//     graphics->begin_pass(game->glow_mask_msaa_target, &(graphics_pass_desc_t) { .clear_color = true });
//     {
//         country_mesh_data_t* country_mesh_data = &game->country_data.mesh;
        
//         shape_info->param.line_thickness = 2.5f;
//         graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
//         graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
//         graphics->set_vertex_buffer(country_mesh_data->vertex_buffer, 0, country_mesh_data->vertex_stride, 0);
//         graphics->set_index_buffer(country_mesh_data->index_buffer, 0);
//         graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
//         graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER, 1, 0, 0);
        
//         // game->glow_mask_setting.glow_color = v4v(srgb_to_linear(v3(0.9964f, 0.8431f, 0.4941f)), 1.0f);
//         game->glow_mask_param.glow_color = srgb_to_linear(theme->fg_color.rgb);
//         game->glow_mask_param.glow = theme->dark_mode ? 1.0f : 0.0f;
        
//         graphics->update_buffer(game->glow_mask_buffer, &game->glow_mask_param, 0, sizeof(game->glow_mask_param));
//         graphics->set_buffer(game->glow_mask_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
//         graphics->set_program(game->glow_mask_program);
//         graphics->set_pipeline(game->alphaoff_pipeline);
//         graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, country_mesh_data->index_count, 0, 0);
//     }
//     graphics->end_pass();

//     graphics->resolve_texture(game->glow_mask, game->glow_mask_msaa);
    
//     // NOTE: Horizontal blur. glow_mask -> glow_a.
//     graphics->begin_pass(game->glow_a_target, &(graphics_pass_desc_t){ 0 });
//     {
//         game->glow_blur_param = (glow_blur_param_t)
//         {
//             .inverse_dst_size = { 1.0f / game->glow_a.width, 1.0f / game->glow_a.height },
//             .direction = { 1.0f, 0.0f }
//         };
//         graphics->update_buffer(game->blur_buffer, &game->glow_blur_param, 0, sizeof(game->glow_blur_param));
//         graphics->set_buffer(game->blur_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
//         graphics->set_program(game->blur_program);
//         graphics->set_pipeline(game->alphaoff_pipeline);
//         graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
//         graphics->set_srvs(STAGE_PIXEL_SHADER, &game->glow_mask, 1, 0);
//         graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
//     }
//     graphics->end_pass();

//     // NOTE: Vertical blur. glow_a -> glow_b.
//     graphics->begin_pass(game->glow_b_target, &(graphics_pass_desc_t){ 0 });
//     {
//         game->glow_blur_param = (glow_blur_param_t)
//         {
//             .inverse_dst_size = { 1.0f / game->glow_b.width, 1.0f / game->glow_b.height },
//             .direction = { 0.0f, 1.0f }
//         };
//         graphics->update_buffer(game->blur_buffer, &game->glow_blur_param, 0, sizeof(game->glow_blur_param));
//         graphics->set_buffer(game->blur_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
//         graphics->set_program(game->blur_program);
//         graphics->set_pipeline(game->alphaoff_pipeline);
//         graphics->set_samplers(STAGE_PIXEL_SHADER, &game->linear_sampler, 1, 0);
//         graphics->set_srvs(STAGE_PIXEL_SHADER, &game->glow_a, 1, 0);
//         graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
//     }
//     graphics->end_pass();

//     graphics->resolve_texture(game->offscreen_scene, game->offscreen_scene_msaa);
    
//     // // NOTE: Merge pass. offscreen_scene + glow -> offscreen_scene.
//     graphics->begin_pass(game->merge_target_msaa, &(graphics_pass_desc_t){ 0 });
//     {
//         game->glow_merge_param.viewport_size = (vec2){ (f32)platform->width, (f32)platform->height };
//         game->glow_merge_param.intensity = game->glow_intensity;
//         game->glow_merge_param.glow = theme->dark_mode ? 1.0f : 0.0f;
//         graphics->update_buffer(game->glow_merge_param_buffer, &game->glow_merge_param, 0, sizeof(game->glow_merge_param));
//         graphics->set_buffer(game->glow_merge_param_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
//         graphics->set_program(game->glow_merge_program);
//         // graphics->set_pipeline(game->additive_pipeline);
//         graphics->set_pipeline(game->alphaoff_pipeline);
//         graphics_sampler_t samplers[] = { game->point_sampler, game->linear_sampler };
//         graphics->set_samplers(STAGE_PIXEL_SHADER, samplers, 2, 0);
//         graphics_texture_t srvs[] = { game->offscreen_scene, game->glow_b };
//         graphics->set_srvs(STAGE_PIXEL_SHADER, srvs, 2, 0);
//         graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
//     }
//     graphics->end_pass();

//     graphics->begin_pass(game->merge_target_msaa, &(graphics_pass_desc_t){ 0 });
//     {
//         country_mesh_data_t* country_mesh_data = &game->country_data.mesh;
        
//         graphics->update_buffer(game->transform_buffer, &game->transform_param, 0, sizeof(game->transform_param));
//         graphics->set_buffer(game->transform_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
//         graphics->set_vertex_buffer(country_mesh_data->vertex_buffer, 0, country_mesh_data->vertex_stride, 0);
//         graphics->set_index_buffer(country_mesh_data->index_buffer, 0);
//         graphics->set_program(shape_info->program);
//         graphics->set_pipeline(game->d_test_pipeline);

//         // shape_param->color = v4v(srgb_to_linear(v3(0.9964f, 0.8431f, 0.4941f)), 1.0f);
//         // shape_param->color = v4v(srgb_to_linear(v3(0.1058f, 0.9921f, 0.6117f)), 1.0f);
//         // shape_param->color = v4v(srgb_to_linear(v3(0.070f, 0.070f, 0.070f)), 1.0f);
//         shape_param->color = v4v(srgb_to_linear(theme->fg_color.rgb), theme->fg_color.a);
//         shape_param->line_thickness = 2.5f;

//         graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
//         graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);

//         graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST, country_mesh_data->index_count, 0, 0);

//         if (country_is_valid_index(game->country_index))
//         {
//             shape_param->line_thickness = 4.0f;
//             shape_param->lift = 0.01f;
//             shape_param->color = v4v(srgb_to_linear(theme->highlight_color.rgb), theme->highlight_color.a);
//             graphics->update_buffer(shape_info->param_buffer, shape_param, 0, sizeof(shape_param_t));
//             graphics->set_buffer(shape_info->param_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 1, 0, 0);

//             graphics->draw_indexed(TOPOLOGY_TRIANGLE_LIST,
//                                    country_mesh_data->index_ranges[game->country_index].index_count,
//                                    country_mesh_data->index_ranges[game->country_index].index_offset, 0);
//         }
//     }
//     graphics->end_pass();

//     graphics->resolve_texture(game->merge_scene, game->merge_scene_msaa);
    
//     // NOTE: Post pass rendering to backbuffer.
//     graphics->begin_pass(graphics->get_backbuffer_target(), &(graphics_pass_desc_t){ .clear_color = true,  });
//     {
//         game->post_param.viewport_size = (vec2){ (f32)platform->width, (f32)platform->height };
//         game->post_param.aspect_ratio = (f32)platform->width / (f32)platform->height;
//         game->post_param.vignette_soft = 0.45f;
//         game->post_param.invert = game->invert;
//         game->post_param.vignette = game->vignette;

//         graphics->update_buffer(game->post_param_buffer, &game->post_param, 0, sizeof(game->post_param));
//         graphics->set_buffer(game->post_param_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
//         graphics->set_program(game->post_program);
//         graphics->set_pipeline(game->alphaoff_pipeline);
//         graphics->set_samplers(STAGE_PIXEL_SHADER, &game->point_sampler, 1, 0);
//         graphics->set_srvs(STAGE_PIXEL_SHADER, &game->merge_scene, 1, 0);
//         graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
//     }
//     graphics->end_pass();

    

// #if FONT_ENABLE
//     ui_draw_command_t* country_draw_command = 0;
        
//     graphics->begin_draw();
//     {
//         char frame_ms_text[32] = { 0 };
//         size_t frame_ms_length = 0;

//         if ((frame_ms_length = snprintf(frame_ms_text, sizeof(frame_ms_text), "%.2f ms", platform->delta_time * 1000)) > 0)
//         {
//             graphics->draw_text(game->font_16, frame_ms_text, frame_ms_length,
//                                 theme->font_color.r, theme->font_color.g, theme->font_color.b, theme->font_color.a,
//                                 TEXT_ALIGNMENT_TRAILING, 0.0f - 8.0f, 0.0f + 8.0f, (f32)platform->width, (f32)platform->height);
//         }

//         country_name_t country_name = country_get_name(game->country_index);
//         if (country_name.name && country_name.length)
//         {
//             graphics->draw_text(game->font_16, country_name.name, country_name.length,
//                                 theme->font_color.r, theme->font_color.g, theme->font_color.b, theme->font_color.a,
//                                 TEXT_ALIGNMENT_LEADING, 8.0f, 8.0f, (f32)platform->width, (f32)platform->height);
//         }

//         ui_draw_command_iter_t iter = ui_draw_command_iter();
        
//         for (ui_draw_command_t* command = ui_draw_command_next(&iter);
//              command;
//              command = ui_draw_command_next(&iter))
//         {
//             f32 x = command->x;
//             f32 y = command->y;
//             f32 width = command->width;
//             f32 height = command->height;
//             vec4 color = command->color;
            
//             switch (command->kind)
//             {
//                 case UI_DRAW_RECT:
//                 {
//                     graphics->draw_rect(x, y, width, height, true, 0.0f, color.r, color.g, color.b, color.a);
//                 } break;
                
//                 case UI_DRAW_BORDER:
//                 {
//                     f32 thickness = command->thickness;

//                     graphics->draw_rect(x, y, width, height, false, thickness, color.r, color.g, color.b, color.a);
//                 } break;
                
//                 case UI_DRAW_TEXT:
//                 {
//                     graphics_2d_font_t font = command->font;
//                     const char* text = command->text;
//                     u32 length = command->length;
//                     ui_rect_t clip = command->clip;
                    
//                     graphics->push_axis_aligned_clip(clip.x, clip.y, clip.width, clip.height);
//                     graphics->draw_text(font, text, length, color.r, color.g, color.b, color.a,
//                                         TEXT_ALIGNMENT_LEADING, x, y, width, height);
//                     graphics->pop_axis_aligned_clip();
//                 } break;
                
//                 case UI_DRAW_CUSTOM:
//                 {
//                     country_draw_command = command;
//                 } break;

//                 default: 
//                 {
//                     assert(!"[UI] Invalid draw command.");
//                 } break;
//             }
//         }
//     }
//     graphics->end_draw();

//     if (country_is_valid_index(game->country_index) && country_draw_command)
//     {
//         f32 x = country_draw_command->x;
//         f32 y = country_draw_command->y;
//         f32 width = country_draw_command->width;
//         f32 height = country_draw_command->height;
        
//         graphics->begin_pass(graphics->get_backbuffer_target(), &(graphics_pass_desc_t) { 0 });
//         {
//             shape_info_ui_t* shape_info_ui = &game->shape_info_ui;
//             shape_ui_param_t* shape_ui_param = &shape_info_ui->param;

//             shape_ui_param->color = v4v(srgb_to_linear(theme->highlight_color.rgb), theme->highlight_color.a);
//             shape_ui_param->viewport_size = v2((f32)platform->width, (f32)platform->height);

//             graphics->update_buffer(shape_info_ui->param_buffer, shape_ui_param, 0, sizeof(shape_ui_param_t));
//             graphics->set_buffer(shape_info_ui->param_buffer, STAGE_VERTEX_SHADER | STAGE_PIXEL_SHADER, 0, 0, 0);

//             graphics->set_vertex_buffer(shape_info_ui->vertex_buffer, 0, sizeof(country_border_point_t), 0);
//             graphics->set_program(shape_info_ui->program);
//             graphics->set_pipeline(game->default_pipeline);

//             graphics->set_viewport(x, y, width, height);

//             country_range_t country_range = game->country_data.query.ranges[game->country_index];
            
//             u32 part_offset = country_range.part_offset;
//             u32 part_count = country_range.part_count;

//             f32 lon_min = 1000.0f;
//             f32 lon_max = -1000.0f;
//             f32 lat_min = 1000.0f;
//             f32 lat_max = -1000.0f;

//             country_range_t* country = &game->country_data.query.ranges[game->country_index];
            
//             for (u32 i = 0; i < country->part_count; ++i)
//             {
//                 country_border_part_range_t* part_range = &game->country_data.query.border_part_ranges[i + country->part_offset];

//                 for (u32 i = 0; i < part_range->point_count; ++i)
//                 {
//                     country_border_point_t border_point = game->country_data.query.border_points[i + part_range->point_index];

//                     if (border_point.lonlat.x < lon_min) lon_min = border_point.lonlat.x;
//                     if (border_point.lonlat.x > lon_max) lon_max = border_point.lonlat.x;
//                     if (border_point.lonlat.y < lat_min) lat_min = border_point.lonlat.y;
//                     if (border_point.lonlat.y > lat_max) lat_max = border_point.lonlat.y;
//                 }
//             }

//             f32 lon_span = lon_max - lon_min;
//             f32 lat_span = lat_max - lat_min;
//             f32 max_span = fmaxf(lon_span, lat_span) * 0.5f;
//             f32 zoom = 1.0f / (max_span * (f32)DEG2RAD);
//             vec2 center = game->country_data.query.centers[game->country_index];

//             // NOTE: Exclude antarctica.
//             if (lon_span > 180.0f && game->country_index != 159)
//             {
//                 lon_min = 1000.0f;
//                 lon_max = -1000.0f;

//                 for (u32 i = 0; i < part_count; ++i)
//                 {
//                     country_border_part_range_t part_range = game->country_data.query.border_part_ranges[i + part_offset];
                    
//                     for (u32 j = 0; j < part_range.point_count; ++j)
//                     {
//                         country_border_point_t border_point = game->country_data.query.border_points[j + part_range.point_index];
            
//                         if (border_point.lonlat.x > 0.0f)
//                         {
//                             if (border_point.lonlat.x < lon_min) lon_min = border_point.lonlat.x;
//                             if (border_point.lonlat.x > lon_max) lon_max = border_point.lonlat.x;
//                         }
//                     }
//                 }

//                 center = v2((lon_min + lon_max) * 0.5f, (lat_min + lat_max) * 0.5f);
//                 max_span = fmaxf((lon_max - lon_min) * (f32)DEG2RAD, (lat_max - lat_min) * (f32)DEG2RAD);
//                 zoom = 2.35f / max_span;
//             }

//             shape_ui_param->center = center;
//             shape_ui_param->zoom = zoom;

//             for (u32 part_index = 0; part_index < part_count; ++part_index)
//             {
//                 country_border_part_range_t part_range = game->country_data.query.border_part_ranges[part_index + part_offset];

//                 graphics->draw(TOPOLOGY_LINE_STRIP, part_range.point_count, part_range.point_index);
//             }
//         }
//         graphics->end_pass();
//     }
// #endif

// }

update_function(update)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    input_t* input = platform->input;
    game_t* game = (game_t*)memory->permanent;
    camera_t* camera = &game->camera;

    if (input_is_key_released(input, KEY_C))
    {
        update_theme(game, game->current_theme_type);
    }
    
    theme_t* theme = get_current_theme(game);
    
    if (input->wheel != 0.0f)
    {
        camera->position.z += 3.0f * platform->delta_time * -input->wheel;
    }

    update_camera(camera);

    earth_update(input, camera, theme, platform->delta_time, (u32)(platform->width * 0.66f), (u32)(platform->height * 0.66f), game->earth);
}

static void resize_offscreen_buffer(platform_t* platform, game_t* game)
{
    graphics_t* graphics = platform->graphics;
    graphics_state_t* graphics_state = &game->graphics_state;
    bool resized = platform->resized;
    bool is_valid = graphics->is_valid_target(graphics_state->offscreen_target_msaa);

    if (is_valid && resized)
    {
        graphics->delete_target(graphics_state->offscreen_target_msaa);
        graphics->delete_texture_2d(graphics_state->offscreen_scene_msaa);
        graphics->delete_texture_2d(graphics_state->offscreen_depth_msaa);
        graphics->delete_texture_2d(graphics_state->offscreen_scene);
    }

    if (!is_valid || resized)
    {
        graphics_state->offscreen_scene = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = platform->width,
            .height = platform->height,
        }, 0, 0);

        graphics_state->offscreen_scene_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R16G16B16A16_FLOAT,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = platform->width,
            .height = platform->height,
            .sample_count = 8,
        }, 0, 0);

        graphics_state->offscreen_depth_msaa = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_D24_UNORM_S8_UINT,
            .bind = BIND_DEPTH_STENCIL,
            .width = platform->width,
            .height = platform->height,
            .sample_count = 8,
        }, 0, 0);

        graphics_state->offscreen_target_msaa = graphics->create_target(&(graphics_target_desc_t)
        {
            .color = graphics_state->offscreen_scene_msaa,
            .depth = graphics_state->offscreen_depth_msaa
        });
    }
}

render_function(render)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    game_t* game = (game_t*)memory->permanent;
    camera_t* camera = &game->camera;
    graphics_state_t* graphics_state = &game->graphics_state;
    theme_t* theme = get_current_theme(game);
    earth_t* earth = game->earth;
    
    resize_offscreen_buffer(platform, game);

    earth_render(graphics, graphics_state, camera, theme, (u32)(platform->width * 0.66f), (u32)(platform->height * 0.66f), earth);

    graphics->begin_pass(graphics->get_backbuffer_target(), &(graphics_pass_desc_t)
    {
        .clear_color = true,
        .clear_rgba = v4v(srgb_to_linear(theme->bg_color.rgb), theme->bg_color.a),
        .clear_depth = true, .clear_depth_value = 1.0f
    });
    {
        graphics->set_program(graphics_state->composite_graphics.program);
        graphics->set_pipeline(graphics_state->depth_test_no_alpha_pipeline);
        graphics->set_samplers(STAGE_PIXEL_SHADER, &graphics_state->point_sampler, 1, 0);
        graphics->set_srvs(STAGE_PIXEL_SHADER, &graphics_state->offscreen_scene, 1, 0);
        graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    }
    graphics->end_pass();
}
