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
#include "theme.c"
#include "ui.c"
#include "fuzzy_match.c"

typedef enum theme_type_t
{
    THEME_TYPE_LIGHT,
    THEME_TYPE_DARK,

    THEME_TYPE_COUNT,
} theme_type_t;

typedef struct camera_t
{
    vec3 position;
    vec3 target;
    vec3 up;

    f32 fov_y; // NOTE: Radians.
    f32 aspect_ratio;

    mat4x4 view;
    mat4x4 projection;
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

typedef struct game_t
{
    memory_arena_t* memory_arena;
    graphics_state_t graphics_state;
    
    camera_t camera;
    themes_t themes;
    earth_t earth;
} game_t;

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

static void init_themes(graphics_t* graphics, themes_t* themes)
{
    graphics_2d_font_t font_text = graphics->create_font("IosevkaTerm NFM", 12);
    graphics_2d_font_t font_header = graphics->create_font("IosevkaTerm NFM", 16);
    
    // NOTE: Light and dark themes.
    theme_add(themes, &(theme_t)
    {
        .bg_color = v4(0.9450f, 0.9215f, 0.8941f, 1.0f),
        .fg_color = v4(0.3098f, 0.2784f, 0.2235f, 1.0f),
        .highlight_color = v4(0.7098f, 0.2784f, 0.2235f, 1.0f),
        .dim_color = v4(0.55f, 0.55f, 0.55f, 1.0f),
        .border_color = v4(0.5647f, 0.5647f, 0.5647f, 1.0f),
        .font_text = font_text,
        .font_header = font_header,
        .dark_mode = false,
    }, THEME_TYPE_LIGHT);

    theme_add(themes, &(theme_t)
    {
        .bg_color = v4(0.0705f, 0.0705f, 0.0705f, 1.0f),
        .fg_color = v4(0.8f, 0.8f, 0.8f, 1.0f),
        .highlight_color = v4(0.1058f, 0.9921f, 0.6117f, 1.0f),
        .dim_color = v4(0.012f, 0.012f, 0.012f, 1.0f),
        .border_color = v4(0.1647f, 0.1647f, 0.1647f, 1.0f),
        .font_text = font_text,
        .font_header = font_header,
        .dark_mode = true,
    }, THEME_TYPE_DARK);
    
    theme_set(themes, (u32)THEME_TYPE_DARK);
}

static void update_theme(themes_t* themes)
{
    theme_type_t theme_type = (theme_type_t)theme_get_current_index(themes);
    
    switch (theme_type)
    {
        case THEME_TYPE_LIGHT:
        {
            theme_set(themes, (u32)THEME_TYPE_DARK);
        } break;

        case THEME_TYPE_DARK:
        {
            theme_set(themes, (u32)THEME_TYPE_LIGHT);
        } break;

        default:
        {
            assert("[THEME] Invalid theme type.");
        }
    }
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
}

static inline void init_graphics_state(graphics_t* graphics, graphics_state_t* graphics_state)
{
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

    init_graphics_state(graphics, &game->graphics_state);
    init_camera(&game->camera, v3(0.0f, 0.0f, 2.5f), v3(0.0f, 0.0f, 0.0f),
                60.0f, (f32)platform->width / (f32)platform->height);
    init_themes(graphics, &game->themes);
    earth_init(memory_arena, graphics, &game->graphics_state, io, &game->earth);
    ui_init(memory_arena);
}

update_function(update)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    input_t* input = platform->input;
    game_t* game = (game_t*)memory->permanent;
    camera_t* camera = &game->camera;
    theme_t* theme = theme_get_current(&game->themes);

    if (input_is_key_released(input, KEY_C, 0))
    {
        update_theme(&game->themes);
    }
    
    update_camera(camera);

    ui_begin(graphics, input, platform->delta_time, (f32)platform->width, (f32)platform->height);
    {
        earth_ui_update(input, theme, &game->earth);
    }
    ui_end();

    earth_update(input, camera, theme_get_current(&game->themes), platform->delta_time, (f32)platform->width, (f32)platform->height, &game->earth);
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
    theme_t* theme = theme_get_current(&game->themes);
    
    resize_offscreen_buffer(platform, game);

    earth_render(graphics, graphics_state, camera, theme, &game->earth);

    graphics->begin_pass(graphics->get_backbuffer_target(), &(graphics_pass_desc_t)
    {
        .clear_color = true,
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

    graphics->begin_draw();
    
    ui_draw_command_list_t* command_list = ui_draw_command_list();
    for (i32 i = 0; i < command_list->command_count; ++i)
    {
        ui_draw_command_t* command = command_list->commands[i];
        f32 x = command->x;
        f32 y = command->y;
        f32 width = command->width;
        f32 height = command->height;
        vec4 color = command->color;

        switch (command->kind)
        {
            case UI_DRAW_RECT:
            {
                graphics->draw_rect(x, y, width, height, true, 0.0f, color.r, color.g, color.b, color.a);
            } break;
                
            case UI_DRAW_BORDER:
            {
                f32 thickness = command->thickness;
                graphics->draw_rect(x, y, width, height, false, thickness, color.r, color.g, color.b, color.a);
            } break;
                
            case UI_DRAW_TEXT:
            {
                graphics_2d_font_t font = command->font;
                const char* text = command->text;
                u32 length = command->length;
                ui_rect_t clip = command->clip;
                    
                graphics->push_axis_aligned_clip(clip.x, clip.y, clip.width, clip.height);
                graphics->draw_text(font, text, length, color.r, color.g, color.b, color.a,
                                    TEXT_ALIGNMENT_LEADING, x, y, width, height);
                graphics->pop_axis_aligned_clip();
            } break;
                
            default: 
            {
                assert(!"[UI] Invalid draw command.");
            } break;
        }
    }

    graphics->end_draw();
}
