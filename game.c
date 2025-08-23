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

    graphics_buffer_t vertex_buffer = graphics->create_buffer(vertex_data, sizeof(vertex_data), BUFFER_USAGE_IMMUTABLE, BUFFER_BIND_VERTEX_BUFFER);
    (void)vertex_buffer;

    graphics_shader_t vertex_shader = graphics->create_shader(vshader, sizeof(vshader), VERTEX_SHADER_TYPE);
    graphics_shader_t pixel_shader = graphics->create_shader(pshader, sizeof(pshader), PIXEL_SHADER_TYPE);
    (void)vertex_shader; (void)pixel_shader;
}

update_function(update)
{

}

render_function(render)
{

}
