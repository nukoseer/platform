#include <stdio.h>
#include "utils.h"
#include "platform.h"

#include "vertex_shader.h"
#include "pixel_shader.h"

typedef struct vertex_t
{
    f32 position[2];
    f32 color[3];
} vertex_t;

typedef struct game_t
{
    vertex_t vertex_data[3];
    graphics_buffer_t vertex_buffer;
    graphics_texture_t offscreen_scene;
    graphics_sampler_t sampler;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
    graphics_pipeline_t pipeline;
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
        .usage = BUFFER_USAGE_IMMUTABLE,
        .bind = BIND_VERTEX_BUFFER,
    });

    // TODO: This is for offscreen rendering.
    // It should be same format, width and height with backbuffer.
    // Probably we should update this when backbuffer is resized.
    game->offscreen_scene = graphics->create_texture_2d(&(graphics_texture_2d_desc_t)
    {
        .format = FORMAT_R8G8B8A8_UNORM,
        .bind = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET,
        .width = platform->width,
        .height = platform->height,
    }, 0, 0);

    game->sampler = graphics->create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_POINT,
        .address_u = TEXTURE_ADDRESS_WRAP,
        .address_v = TEXTURE_ADDRESS_WRAP,
        .address_w = TEXTURE_ADDRESS_WRAP,
    });

    // TODO: Create RenderTargetView for offscreen_scene to render.

    game->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader,
        .bytecode_size = sizeof(vshader),
        .type = VERTEX_SHADER_TYPE,
    });

    game->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader,
        .bytecode_size = sizeof(pshader),
        .type = PIXEL_SHADER_TYPE,
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

    game->pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
    {
        .cull = false,
        .wireframe = false,
    });
}

update_function(update)
{
    
}

render_function(render)
{
    memory_t* memory = platform->memory;
    graphics_t* graphics = platform->graphics;
    game_t* game = (game_t*)memory->permanent;

    graphics_target_t backbuffer_target = graphics->get_backbuffer_target();

    graphics->begin_pass(backbuffer_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 1.0f, 0.9098f, 0.9098f, 0.0f }});
    graphics->set_vertex_buffer(game->vertex_buffer, 0, sizeof(vertex_t), 0);
    graphics->set_program(game->program);
    graphics->set_pipeline(game->pipeline);
    graphics->draw(TOPOLOGY_TRIANGLE_LIST, array_count(game->vertex_data), 0);
    graphics->end_pass();
}
