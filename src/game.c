#include <stdio.h>
#include "utils.h"
#include "platform.h"

#include "../shader/vertex_shader.h"
#include "../shader/pixel_shader.h"
#include "../shader/glow_mask_pixel_shader.h"
#include "../shader/blur_vertex_shader.h"
#include "../shader/blur_pixel_shader.h"
#include "../shader/post_vertex_shader.h"
#include "../shader/post_pixel_shader.h"

typedef struct vertex_t
{
    f32 position[2];
    f32 color[3];
} vertex_t;

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

typedef struct game_t
{
    vertex_t vertex_data[3];
    graphics_buffer_t vertex_buffer;
    graphics_texture_t offscreen_scene;
    graphics_target_t offscreen_target;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
    graphics_pipeline_t default_pipeline;

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
} game_t;

init_function(init)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
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
}

update_function(update)
{

}

static void resize_offscreen_buffer(graphics_t* graphics, game_t* game)
{
    bool is_valid = graphics->is_valid_target(game->offscreen_target);
    bool resize = false;
    u32 backbuffer_width = 0;
    u32 backbuffer_height = 0;

    graphics->get_target_size(graphics->get_backbuffer_target(), &backbuffer_width, &backbuffer_height);

    if (is_valid)
    {
        u32 offscreen_width = game->offscreen_scene.width;
        u32 offscreen_height = game->offscreen_scene.height;

        if (offscreen_width != backbuffer_width || offscreen_height != backbuffer_height)
        {
            graphics->delete_target(game->offscreen_target);
            graphics->delete_target(game->glow_mask_target);
            graphics->delete_target(game->glow_a_target);
            graphics->delete_target(game->glow_b_target);
            graphics->delete_texture_2d(game->offscreen_scene);
            graphics->delete_texture_2d(game->glow_mask);
            graphics->delete_texture_2d(game->glow_a);
            graphics->delete_texture_2d(game->glow_b);

            resize = true;
        }
    }

    if (!is_valid || resize)
    {
        game->offscreen_scene = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM_SRGB,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = backbuffer_width,
            .height = backbuffer_height,
        }, 0, 0);

        game->glow_mask = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM_SRGB,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(backbuffer_width * 0.5f),
            .height = (u32)(backbuffer_height * 0.5f),
        }, 0, 0);

        game->glow_a = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM_SRGB,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(backbuffer_width * 0.5f),
            .height = (u32)(backbuffer_height * 0.5f),
        }, 0, 0);

        game->glow_b = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM_SRGB,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = (u32)(backbuffer_width * 0.5f),
            .height = (u32)(backbuffer_height * 0.5f),
        }, 0, 0);

        // NOTE: Create RenderTargetView for offscreen_scene to render.
        game->offscreen_target = graphics->create_target(game->offscreen_scene);
        game->glow_mask_target = graphics->create_target(game->glow_mask);
        game->glow_a_target = graphics->create_target(game->glow_a);
        game->glow_b_target = graphics->create_target(game->glow_b);
    }
}

render_function(render)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    game_t* game = (game_t*)memory->permanent;

    resize_offscreen_buffer(graphics, game);

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

        if ((platform->input->keys[KEY_V].action == KEY_ACTION_RELEASE))
        {
            game->post_setting.vignette = game->post_setting.vignette == 0.0f ? 1.0f : 0.0f;
        }

        if ((platform->input->keys[KEY_G].action == KEY_ACTION_RELEASE))
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
}
