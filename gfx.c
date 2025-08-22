graphics_create_buffer_function(gfx_create_buffer)
{
    graphics_buffer_t graphics_buffer = { 0 };

    graphics_buffer.platform = d3d11_create_buffer(global_d3d11.device, data, size, usage, bind_flags);

    return graphics_buffer;
}
