#include "../shader/blur_vertex_shader.h"
#include "../shader/blur_pixel_shader.h"

#include "blur.h"

static void blur_graphics_create(const graphics_t* graphics, blur_graphics_t* blur_graphics)
{
    blur_graphics->parameter_buffer = graphics->create_buffer(&(graphics_buffer_desc_t)
    {
        .size = 16,
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });

    blur_graphics->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = blur_vshader,
        .bytecode_size = sizeof(blur_vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    blur_graphics->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = blur_pshader,
        .bytecode_size = sizeof(blur_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    blur_graphics->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = blur_graphics->vertex_shader,
        .pixel_shader = blur_graphics->pixel_shader,
        // NOTE: No input layout.
        .attributes = 0,
        .attribute_count = 0,
    });
}
