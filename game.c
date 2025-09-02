#include <stdio.h>
#include "utils.h"
#include "platform.h"

#include "vertex_shader.h"
#include "pixel_shader.h"

#include "composite_vertex_shader.h"
#include "composite_pixel_shader.h"

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
    graphics_target_t offscreen_target;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
    graphics_pipeline_t pipeline;

    graphics_shader_t composite_vertex_shader;
    graphics_shader_t composite_pixel_shader;
    graphics_program_t composite_program;
    graphics_pipeline_t composite_pipeline;
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

    // NOTE: Create RenderTargetView for offscreen_scene to render.
    game->offscreen_target = graphics->create_target(game->offscreen_scene);

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

    // NOTE: Composite shaders and pipeline.
    game->composite_vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = composite_vshader,
        .bytecode_size = sizeof(composite_vshader),
        .type = VERTEX_SHADER_TYPE,
    });

    game->composite_pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = composite_pshader,
        .bytecode_size = sizeof(composite_pshader),
        .type = PIXEL_SHADER_TYPE,
    });
        
    game->composite_program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = game->composite_vertex_shader,
        .pixel_shader = game->composite_pixel_shader,
        // NOTE: No input layout.
        .attributes = 0,
        .attribute_count = 0,
    });

    game->composite_pipeline = graphics->create_pipeline(&(graphics_pipeline_desc_t)
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
    graphics_target_t offscreen_target = game->offscreen_target;

    u32 backbuffer_width = 0;
    u32 backbuffer_height = 0;
    u32 offscreen_width = 0;
    u32 offscreen_height = 0;

    graphics->get_target_size(backbuffer_target, &backbuffer_width, &backbuffer_height);
    graphics->get_target_size(offscreen_target, &offscreen_width, &offscreen_height);

    if (offscreen_width != backbuffer_width || offscreen_height != backbuffer_height)
    {
        // TODO: Recreate offscreen texture and target.
    }

    // NOTE: Offscreen rendering pass.
    graphics->begin_pass(offscreen_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 1.0f, 0.9098f, 0.9098f, 0.0f }});
    graphics->set_vertex_buffer(game->vertex_buffer, 0, sizeof(vertex_t), 0);
    graphics->set_program(game->program);
    graphics->set_pipeline(game->pipeline);
    graphics->draw(TOPOLOGY_TRIANGLE_LIST, array_count(game->vertex_data), 0);
    graphics->end_pass();

    // NOTE: Composite pass rendering to backbuffer.
    graphics->begin_pass(backbuffer_target, &(graphics_pass_desc_t){ .clear_color = true, .clear_rgba = { 0.0f, 0.0f, 0.0f, 1.0f }});
    graphics->set_program(game->composite_program);
    graphics->set_pipeline(game->composite_pipeline);
    graphics->set_samplers(STAGE_PIXEL_SHADER, &game->sampler, 1, 0);
    graphics->set_srvs(STAGE_PIXEL_SHADER, &game->offscreen_scene, 1, 0);
    graphics->draw(TOPOLOGY_TRIANGLE_LIST, 3, 0);
    graphics->end_pass();
}
