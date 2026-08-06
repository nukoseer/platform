#pragma once

typedef struct blur_parameters_t
{
    vec2 inverse_viewport_size;
    vec2 direction;
} blur_parameters_t;

typedef struct blur_graphics_t
{
    graphics_buffer_t parameter_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
} blur_graphics_t;

static void blur_graphics_create(const graphics_t* graphics, blur_graphics_t* blur_graphics);
