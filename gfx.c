// TODO: This is just random value for now.
#define GFX_MAX_RESOUCE 32

typedef struct gfx_buffer_t
{
    ID3D11Buffer* buffer;
    usize size;
} gfx_buffer_t;

typedef struct gfx_shader_t
{
    void* shader;
    const void* bytecode;
    usize bytecode_size;
} gfx_shader_t;

typedef struct gfx_program_t
{
    ID3D11VertexShader* vertex_shader;
    ID3D11PixelShader* pixel_shader;
    ID3D11InputLayout* input_layout;
} gfx_program_t;

typedef struct gfx_pipeline_t
{
    ID3D11RasterizerState* rasterizer_state;
} gfx_pipeline_t;

static gfx_buffer_t global_buffers[GFX_MAX_RESOUCE];
static gfx_shader_t global_shaders[GFX_MAX_RESOUCE];
static gfx_program_t global_programs[GFX_MAX_RESOUCE];
static gfx_pipeline_t global_pipelines[GFX_MAX_RESOUCE];

static usize global_buffer_count;
static usize global_shader_count;
static usize global_program_count;
static usize global_pipeline_count;

static inline map_dxgi_format(graphics_format_t format)
{
    DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;
    
    switch (format)
    {
        case FORMAT_R32G32_FLOAT:
        {
            dxgi_format = DXGI_FORMAT_R32G32_FLOAT;
        } break;

        case FORMAT_R32G32B32_FLOAT:
        {
            dxgi_format = DXGI_FORMAT_R32G32B32_FLOAT;
        } break;

        default:
        {
            assert(!"[GFX] Failed to map format.");
        } break;
    }

    return dxgi_format;
}

static graphics_create_buffer_function(gfx_create_buffer)
{
    graphics_buffer_t graphics_buffer = { 0 };
    usize buffer_index = global_buffer_count++;
    gfx_buffer_t* gfx_buffer = global_buffers + buffer_index;

    gfx_buffer->buffer = d3d11_create_buffer(global_d3d11.device,
                                             buffer_desc->data, buffer_desc->size,
                                             buffer_desc->usage, buffer_desc->bind);

    graphics_buffer.platform = buffer_index;

    return graphics_buffer;
}

static graphics_create_shader_function(gfx_create_shader)
{
    graphics_shader_t graphics_shader = { 0 };
    usize shader_index = global_shader_count++;
    gfx_shader_t* gfx_shader = global_shaders + shader_index;

    if (shader_desc->type == VERTEX_SHADER_TYPE)
    {
        gfx_shader->shader = d3d11_create_vertex_shader(global_d3d11.device, shader_desc->bytecode, shader_desc->bytecode_size);
        
    }
    else if (shader_desc->type == PIXEL_SHADER_TYPE)
    {
        gfx_shader->shader = d3d11_create_pixel_shader(global_d3d11.device, shader_desc->bytecode, shader_desc->bytecode_size);
    }

    gfx_shader->bytecode = shader_desc->bytecode;
    gfx_shader->bytecode_size = shader_desc->bytecode_size;

    graphics_shader.platform = shader_index;
    
    return graphics_shader;
}

static graphics_create_program_function(gfx_create_program)
{
    graphics_program_t graphics_program = { 0 };
    usize program_index = global_program_count++;
    gfx_program_t* gfx_program = global_programs + program_index;
    usize vertex_shader_index = (usize)program_desc->vertex_shader.platform;
    usize pixel_shader_index = (usize)program_desc->pixel_shader.platform;
    gfx_shader_t* vertex_shader = global_shaders + vertex_shader_index;
    gfx_shader_t* pixel_shader = global_shaders + pixel_shader_index;

    D3D11_INPUT_ELEMENT_DESC descs[32] = { 0 };
    assert(program_desc->attribute_count <= sizeof(descs) && "[GFX] Failed to map program attributes.");

    for (u32 attribute_index = 0; attribute_index < program_desc->attribute_count; ++attribute_index)
    {
        graphics_vertex_attribute_t* attribute = program_desc->attributes + attribute_index;
        D3D11_INPUT_ELEMENT_DESC* desc = descs + attribute_index;

        desc->SemanticName = attribute->semantic;
        desc->SemanticIndex = attribute->index;
        desc->Format = map_dxgi_format(attribute->format);
        desc->InputSlot = attribute->slot;
        desc->AlignedByteOffset = attribute->offset;
        desc->InputSlotClass = attribute->per_instance ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
    }

    gfx_program->vertex_shader = vertex_shader->shader;
    gfx_program->pixel_shader = pixel_shader->shader;
    
    gfx_program->input_layout = d3d11_create_input_layout(global_d3d11.device, descs, program_desc->attribute_count, vertex_shader->bytecode, vertex_shader->bytecode_size);

    graphics_program.platform = program_index;

    return graphics_program;
}

static graphics_create_pipeline_function(gfx_create_pipeline)
{
    graphics_pipeline_t graphics_pipeline = { 0 };
    usize pipeline_index = global_pipeline_count++;
    gfx_pipeline_t* gfx_pipeline = global_pipelines + pipeline_index;

    // NOTE: Disable culling.
    // Meaning every triangle will be drawn regardless of
    // facing direction (clock-wise or counter clock-wise).
    D3D11_RASTERIZER_DESC desc =
    {
        .FillMode = pipeline_desc->wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID,
        .CullMode = pipeline_desc->cull ? D3D11_CULL_BACK : D3D11_CULL_NONE,
        .DepthClipEnable = TRUE,
    };

    ID3D11Device_CreateRasterizerState(global_d3d11.device, &desc, &gfx_pipeline->rasterizer_state);

    graphics_pipeline.platform = pipeline_index;
    
    return graphics_pipeline;
}

static graphics_set_program_function(gfx_set_program)
{
    usize program_index = (usize)program.platform;
    gfx_program_t* gfx_program = global_programs + program_index;

    ID3D11DeviceContext_VSSetShader(global_d3d11.context, gfx_program->vertex_shader, 0, 0);
    ID3D11DeviceContext_PSSetShader(global_d3d11.context, gfx_program->pixel_shader, 0, 0);
    ID3D11DeviceContext_IASetInputLayout(global_d3d11.context, gfx_program->input_layout);
}
