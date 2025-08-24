#include <stdio.h>
#include "utils.h"
#include "platform.h"

#include "vertex_shader.h"
#include "pixel_shader.h"

init_function(init)
{
    // NOTE: Dummy example code.
    graphics_t* graphics = platform->graphics;

    f32 vertex_data[] =
    {
        +0.00f, +0.66f, 1.0f, 0.0f, 0.0f,
        -0.33f, -0.33f, 0.0f, 1.0f, 0.0f,
        +0.33f, -0.33f, 0.0f, 0.0f, 1.0f,
    };

    graphics_buffer_t vertex_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .data = vertex_data,
        .size = sizeof(vertex_data),
        .usage = BUFFER_USAGE_IMMUTABLE,
        .bind = BUFFER_BIND_VERTEX_BUFFER
    });
    (void)vertex_buffer;

    graphics_shader_t vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = vshader,
        .bytecode_size = sizeof(vshader),
        .type = VERTEX_SHADER_TYPE,
    });

    graphics_shader_t pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = pshader,
        .bytecode_size = sizeof(pshader),
        .type = PIXEL_SHADER_TYPE,
    });
        
    graphics_program_t program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = vertex_shader,
        .pixel_shader = pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32_FLOAT,    0,               0, 0, 0, 0 },
            { "COLOR",    FORMAT_R32G32B32_FLOAT, 2 * sizeof(f32), 0, 0, 0, 0 },
        },
        .attribute_count = 2,
    });

    (void)program;
}

update_function(update)
{

}

render_function(render)
{

}
