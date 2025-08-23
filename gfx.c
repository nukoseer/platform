
graphics_create_buffer_function(gfx_create_buffer)
{
    graphics_buffer_t graphics_buffer = { 0 };

    graphics_buffer.platform = d3d11_create_buffer(global_d3d11.device, data, size, usage, bind_flags);

    return graphics_buffer;
}

graphics_create_shader_function(gfx_create_shader)
{
    graphics_shader_t graphics_shader = { 0 };

    if (shader_type == VERTEX_SHADER_TYPE)
    {
        graphics_shader.platform = d3d11_create_vertex_shader(global_d3d11.device, buffer, size);
    }
    else if (shader_type == PIXEL_SHADER_TYPE)
    {
        graphics_shader.platform = d3d11_create_pixel_shader(global_d3d11.device, buffer, size);
    }
    
    return graphics_shader;
}
