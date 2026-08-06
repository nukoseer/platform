#include "../shader/composite_vertex_shader.h"
#include "../shader/composite_pixel_shader.h"

typedef struct composite_graphics_t
{
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
} composite_graphics_t;

static void composite_graphics_create(const graphics_t* graphics, composite_graphics_t* composite_graphics)
{
    composite_graphics->vertex_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = composite_vshader,
        .bytecode_size = sizeof(composite_vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    composite_graphics->pixel_shader = graphics->create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = composite_pshader,
        .bytecode_size = sizeof(composite_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    composite_graphics->program = graphics->create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = composite_graphics->vertex_shader,
        .pixel_shader = composite_graphics->pixel_shader,
        // NOTE: No input layout.
        .attributes = 0,
        .attribute_count = 0,
    });
}
