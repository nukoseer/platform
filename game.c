#include <stdio.h>
#include "utils.h"
#include "platform.h"

#include "vertex_shader.h"
#include "pixel_shader.h"

typedef struct game_t
{
    graphics_buffer_t vertex_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
    graphics_pipeline_t pipeline;
} game_t;

typedef struct vertex_t
{
    f32 position[2];
    f32 color[3];
} vertex_t;

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
    
    game->vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = vertex_data,
        .size = sizeof(vertex_data),
        .usage = BUFFER_USAGE_IMMUTABLE,
        .bind = BUFFER_BIND_VERTEX_BUFFER
    });

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

    graphics->set_program(game->program);
    graphics->set_pipeline(game->pipeline);
    graphics->set_vertex_buffer(game->vertex_buffer, 0, 2 * sizeof(f32) + 3 * sizeof(f32), 0);
}
