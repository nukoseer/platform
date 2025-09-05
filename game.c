#include <stdio.h>
#include "utils.h"
#include "platform.h"

#include "vertex_shader.h"
#include "pixel_shader.h"

#include "post_vertex_shader.h"
#include "post_pixel_shader.h"

typedef struct vertex_t
{
    f32 position[2];
    f32 color[3];
} vertex_t;

typedef struct post_setting_t
{
    f32 inverse_dst_size[2];
    f32 inverse_src_size[2];

    f32 invert;
    f32 _pad[3];
} post_setting_t;

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

    graphics_buffer_t post_buffer;
    graphics_sampler_t point_sampler;
    graphics_shader_t post_vertex_shader;
    graphics_shader_t post_pixel_shader;
    graphics_program_t post_program;
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
        u32 offscreen_width = 0;
        u32 offscreen_height = 0;

        graphics->get_target_size(game->offscreen_target, &offscreen_width, &offscreen_height);

        if (offscreen_width != backbuffer_width || offscreen_height != backbuffer_height)
        {
            graphics->delete_target(game->offscreen_target);
            graphics->delete_texture_2d(game->offscreen_scene);
            resize = true;
        }
    }

    if (!is_valid || resize)
    {
        game->offscreen_scene = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
        {
            .format = FORMAT_R8G8B8A8_UNORM,
            .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
            .width = backbuffer_width,
            .height = backbuffer_height,
        }, 0, 0);

        // NOTE: Create RenderTargetView for offscreen_scene to render.
        game->offscreen_target = graphics->create_target(game->offscreen_scene);   
    }
}

render_function(render)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    game_t* game = (game_t*)memory->permanent;

    resize_offscreen_buffer(graphics, game);
    
    // NOTE: Offscreen rendering pass.
    graphics->begin_pass(game->offscreen_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 1.0f, 0.9098f, 0.9098f, 0.0f }});
    graphics->set_buffer(game->vertex_buffer, STAGE_VERTEX_SHADER, 0, sizeof(vertex_t), 0);
    graphics->set_program(game->program);
    graphics->set_pipeline(game->default_pipeline);
    graphics->draw(TOPOLOGY_TRIANGLE_LIST, array_count(game->vertex_data), 0);
    graphics->end_pass();

    // NOTE: Post pass rendering to backbuffer.
    graphics->begin_pass(graphics->get_backbuffer_target(), &(graphics_pass_desc_t){ .clear_color = false });

    post_setting_t post_setting =
    {
        .inverse_dst_size = { [0] = 1.0f / platform->width, [1] = 1.0f / platform->height },
        .inverse_src_size = { [0] = 1.0f / platform->width, [1] = 1.0f / platform->height },
        .invert = (platform->input->keys[KEY_T].action == KEY_ACTION_PRESS) ? 1.0f : 0.0f,
    };
    
    graphics->update_buffer(game->post_buffer, &post_setting, 0, sizeof(post_setting));
    graphics->set_buffer(game->post_buffer, STAGE_PIXEL_SHADER, 0, 0, 0);
    graphics->set_program(game->post_program);
    graphics->set_pipeline(game->default_pipeline);
    graphics->set_samplers(STAGE_PIXEL_SHADER, &game->point_sampler, 1, 0);
    graphics->set_srvs(STAGE_PIXEL_SHADER, &game->offscreen_scene, 1, 0);
    graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    graphics->end_pass();
}
