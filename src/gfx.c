// TODO: This is just random value for now.
#define GFX_MAX_RESOUCE 32
#define GFX_BACKBUFFER_TARGET_INDEX 0xFFFFFFFF

#define GFX_GENERATION_SHIFT 32
#define GFX_INDEX_MASK ((1ull << GFX_GENERATION_SHIFT) - 1ull)

typedef struct gfx_buffer_t
{
    u32 generation;
    ID3D11Buffer* buffer;
    usize size;
    D3D11_USAGE usage;
    D3D11_BIND_FLAG bind;
    DXGI_FORMAT index_format; // NOTE: For index buffer.
    u32 next_free_index;
} gfx_buffer_t;

typedef struct gfx_texture_t
{
    u32 generation;
    ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* srv;
    DXGI_FORMAT format;
    UINT width;
    UINT height;
    u32 next_free_index;
} gfx_texture_t;

typedef struct gfx_target_t
{
    u32 generation;
    // TODO: Multiple render targets and optional depth stencil?
    ID3D11RenderTargetView* render_target;
    i32 width;
    i32 height;
    u32 next_free_index;
} gfx_target_t;

typedef struct gfx_sampler_t
{
    ID3D11SamplerState* sampler_state;
} gfx_sampler_t;

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

// TODO: We should manage the lifetime of these resouces.
// Now we only create new resources, we never release.
static gfx_buffer_t global_buffers[GFX_MAX_RESOUCE];
static gfx_texture_t global_textures[GFX_MAX_RESOUCE];
static gfx_target_t global_targets[GFX_MAX_RESOUCE];
static gfx_sampler_t global_samplers[GFX_MAX_RESOUCE];
static gfx_shader_t global_shaders[GFX_MAX_RESOUCE];
static gfx_program_t global_programs[GFX_MAX_RESOUCE];
static gfx_pipeline_t global_pipelines[GFX_MAX_RESOUCE];

static u32 global_buffer_count = 1;
static u32 global_texture_count = 1;
static u32 global_target_count = 1;
static u32 global_sampler_count;
static u32 global_shader_count;
static u32 global_program_count;
static u32 global_pipeline_count;
static u32 global_pass_count;

static gfx_target_t global_backbuffer_target;

#define next_index_function(name)                                                                                    \
static inline u32 next_##name##_index(void)                                                                          \
{                                                                                                                    \
    u32 name##_index = 0;                                                                                            \
                                                                                                                     \
    if (global_##name##s[0].next_free_index)                                                                         \
    {                                                                                                                \
        name##_index = global_##name##s[0].next_free_index;                                                          \
        global_##name##s[0].next_free_index = global_##name##s[global_##name##s[0].next_free_index].next_free_index; \
    }                                                                                                                \
    else                                                                                                             \
    {                                                                                                                \
        name##_index = global_##name##_count++;                                                                      \
                                                                                                                     \
        if (name##_index == 0 || name##_index >= array_count(global_##name##s))                                      \
        {                                                                                                            \
            assert(!"Max resource count exceeded.");                                                                 \
        }                                                                                                            \
    }                                                                                                                \
                                                                                                                     \
    return name##_index;                                                                                             \
}

#define free_index_function(name)                                                                                    \
static inline void free_##name##_index(u32 free_index)                                                               \
{                                                                                                                    \
    u32 current_free_##name##_index = global_##name##s[0].next_free_index;                                           \
                                                                                                                     \
    global_##name##s[0].next_free_index = free_index;                                                                \
    global_##name##s[global_##name##s[0].next_free_index].next_free_index = current_free_##name##_index;             \
}                                                                                                                    

next_index_function(buffer);
next_index_function(texture);
next_index_function(target);

free_index_function(buffer);
free_index_function(texture);
free_index_function(target);

static inline u32 get_generation(u64 packed)
{
    u32 generation = (u32)(packed >> GFX_GENERATION_SHIFT);
    return generation;
}

static inline u32 get_index(u64 packed)
{
    u32 index = (u32)(packed & GFX_INDEX_MASK);
    return index;
}

static inline u64 pack_generation_index(u32 generation, u32 index)
{
    u64 generation_index = ((u64)generation << GFX_GENERATION_SHIFT) | ((u64)index & GFX_INDEX_MASK);
    return generation_index;
}

static DXGI_FORMAT map_dxgi_format(graphics_format_t format)
{
    DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;
    
    switch (format)
    {
        case FORMAT_R8G8B8A8_UNORM:
        {
            dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        } break;

        case FORMAT_R8G8B8A8_UNORM_SRGB:
        {
            dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        } break;
        
        case FORMAT_R32G32_FLOAT:
        {
            dxgi_format = DXGI_FORMAT_R32G32_FLOAT;
        } break;

        case FORMAT_R32G32B32_FLOAT:
        {
            dxgi_format = DXGI_FORMAT_R32G32B32_FLOAT;
        } break;

        case FORMAT_R16_UINT:
        {
            dxgi_format = DXGI_FORMAT_R16_UINT;
        } break;

        case FORMAT_R32_UINT:
        {
            dxgi_format = DXGI_FORMAT_R32_UINT;
        } break;

        default:
        {
            assert(!"[GFX] Failed to map format.");
        } break;
    }

    return dxgi_format;
}

static D3D11_BIND_FLAG map_bind(graphics_bind_t bind)
{
    D3D11_BIND_FLAG bind_map = 0;

    if (bind & BIND_VERTEX_BUFFER)
        bind_map |= D3D11_BIND_VERTEX_BUFFER;

    if (bind & BIND_INDEX_BUFFER)
        bind_map |= D3D11_BIND_INDEX_BUFFER;

    if (bind & BIND_CONSTANT_BUFFER)
        bind_map |= D3D11_BIND_CONSTANT_BUFFER;

    if (bind & BIND_SHADER_RESOURCE)
        bind_map |= D3D11_BIND_SHADER_RESOURCE;

    if (bind & BIND_RENDER_TARGET)
        bind_map |= D3D11_BIND_RENDER_TARGET;

    if (bind & BIND_DEPTH_STENCIL)
        bind_map |= D3D11_BIND_DEPTH_STENCIL;

    assert(bind_map != 0 && "[GFX] Failed to map bind.");

    return bind_map;
}

static D3D11_FILTER map_filter(graphics_filter_t filter)
{
    D3D11_FILTER filter_map = 0;

    switch (filter)
    {
        case FILTER_MIN_MAG_MIP_POINT:
        {
            filter_map = D3D11_FILTER_MIN_MAG_MIP_POINT;
        } break;

        case FILTER_MIN_MAG_MIP_LINEAR:
        {
            filter_map = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        } break;

        default:
        {
            assert(!"[GFX] Failed to map filter.");
        } break;
    }

    return filter_map;
}

static D3D11_USAGE map_usage(graphics_usage_t usage)
{
    D3D11_USAGE usage_map = 0;

    switch (usage)
    {
        case USAGE_DEFAULT:
        {
            usage_map = D3D11_USAGE_DEFAULT;
        } break;

        case USAGE_IMMUTABLE:
        {
            usage_map = D3D11_USAGE_IMMUTABLE;
        } break;

        case USAGE_DYNAMIC:
        {
            usage_map = D3D11_USAGE_DYNAMIC;
        } break;

        default:
        {
            assert(!"[GFX] Failed to map usage.");
        } break;
    }

    return usage_map;
}

static D3D11_TEXTURE_ADDRESS_MODE map_texture_address(graphics_texture_address_t texture_address)
{
    D3D11_TEXTURE_ADDRESS_MODE texture_address_map = 0;

    switch (texture_address)
    {
        case TEXTURE_ADDRESS_WRAP:
        {
            texture_address_map = D3D11_TEXTURE_ADDRESS_WRAP;
        } break;
        
        case TEXTURE_ADDRESS_MIRROR:
        {
            texture_address_map = D3D11_TEXTURE_ADDRESS_MIRROR;
        } break;
        
        case TEXTURE_ADDRESS_CLAMP:
        {
            texture_address_map = D3D11_TEXTURE_ADDRESS_CLAMP;
        } break;
        
        case TEXTURE_ADDRESS_BORDER:
        {
            texture_address_map = D3D11_TEXTURE_ADDRESS_BORDER;
        } break;
        
        case TEXTURE_ADDRESS_MIRROR_ONCE:
        {
            texture_address_map = D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
        } break;
        
        default:
        {
            assert(!"[GFX] Failed to map filter.");
        } break;
    }

    return texture_address_map;
}

static D3D11_PRIMITIVE_TOPOLOGY map_primitive_topology(graphics_topology_t topology)
{
    D3D11_PRIMITIVE_TOPOLOGY primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    
    switch (topology)
    {
        case TOPOLOGY_POINT_LIST:
        {
            primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        } break;
        case TOPOLOGY_LINE_LIST:
        {
            primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        } break;
        
        case TOPOLOGY_LINE_STRIP:
        {
            primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        } break;
        
        case TOPOLOGY_TRIANGLE_LIST:
        {
            primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        } break;
        
        case TOPOLOGY_TRIANGLE_STRIP:
        {
            primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        } break;
        
        case TOPOLOGY_LINE_LIST_ADJ:
        {
            primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
        } break;
        
        case TOPOLOGY_TRIANGLE_LIST_ADJ:
        {
            primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
        } break;
        
        default:
        {
            assert(!"[GFX] Failed to map topology.");
        } break;
    }

    return primitive_topology;
}

static gfx_target_t* get_gfx_target(usize target_index)
{
    gfx_target_t* gfx_target = (target_index == GFX_BACKBUFFER_TARGET_INDEX ?
                                &global_backbuffer_target : global_targets + target_index);

    return gfx_target;
}

static graphics_create_buffer_function(gfx_create_buffer)
{
    graphics_buffer_t graphics_buffer = { 0 };
    D3D11_USAGE usage = map_usage(buffer_desc->usage);
    D3D11_BIND_FLAG bind = map_bind(buffer_desc->bind);
    usize size = 0;
    DXGI_FORMAT index_format = 0;
    
    if (bind == D3D11_BIND_INDEX_BUFFER)
    {
        index_format = map_dxgi_format(buffer_desc->index_format);
        assert(index_format != DXGI_FORMAT_UNKNOWN && "[GFX] Invalid index format.");
        u32 stride = index_format == DXGI_FORMAT_R16_UINT ? 2u : 4u;
        // NOTE: If index buffer buffer_desc->size is index count.
        size = buffer_desc->size * stride;
    }
    else
    {
        size = buffer_desc->size;
    }
    
    D3D11_BUFFER_DESC desc =
    {
        .ByteWidth = (UINT)size,
        .Usage = usage,
        .BindFlags = bind,
        .CPUAccessFlags = (usage == D3D11_USAGE_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0,
    };

    D3D11_SUBRESOURCE_DATA initial = { 0 };
    D3D11_SUBRESOURCE_DATA* ptr_initial = 0;

    // NOTE: buffer_desc->data cannot be null if buffer is created as immutable.
    if (buffer_desc->data)
    {
        initial.pSysMem = buffer_desc->data;
        ptr_initial = &initial;
    }

    ID3D11Buffer* buffer = 0;
    HRESULT result = ID3D11Device_CreateBuffer(global_d3d11.device, &desc, ptr_initial, &buffer);
    assert(SUCCEEDED(result) && "[GFX ]Failed to create buffer.");

    u32 buffer_index = next_buffer_index();
    gfx_buffer_t* gfx_buffer = global_buffers + buffer_index;
    u32 buffer_generation = gfx_buffer->generation;

    *gfx_buffer = (gfx_buffer_t)
    {
        .generation = buffer_generation,
        .buffer = buffer,
        .size = size,
        .usage = usage,
        .bind = bind,
        .index_format = index_format,
    };
    
    graphics_buffer.platform = pack_generation_index(buffer_generation, buffer_index);

    return graphics_buffer;
}

static graphics_create_texture_2d_function(gfx_create_texture_2d)
{
    graphics_texture_t graphics_texture = { 0 };
    D3D11_TEXTURE2D_DESC desc =
    {
        .Width = texture_2d_desc->width,
        .Height = texture_2d_desc->height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = map_dxgi_format(texture_2d_desc->format),
        // NOTE: No AA.
        .SampleDesc =
        {
            .Count = 1,
            .Quality = 0,
        },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = map_bind(texture_2d_desc->bind),
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
    };

    D3D11_SUBRESOURCE_DATA initial = { 0 };
    D3D11_SUBRESOURCE_DATA* ptr_initial = 0;

    if (initial_data)
    {
        initial.pSysMem = initial_data;
        initial.SysMemPitch = pitch;
        ptr_initial = &initial;
    }
    
    ID3D11Texture2D* texture_2d = 0;
    HRESULT result = ID3D11Device_CreateTexture2D(global_d3d11.device, &desc, ptr_initial, &texture_2d);
    // TODO: Probably we should think about failure cases.
    assert(SUCCEEDED(result) && "[GFX] Failed to create texture 2d.");

    u32 texture_index = next_texture_index();
    gfx_texture_t* gfx_texture = global_textures + texture_index;
    u32 texture_generation = gfx_texture->generation;

    *gfx_texture = (gfx_texture_t)
    {
        .generation = texture_generation,
        .texture = texture_2d,
        .srv = 0,
        .format = desc.Format,
        .width = desc.Width,
        .height = desc.Height,
    };

    if (texture_2d_desc->bind & BIND_SHADER_RESOURCE)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc =
        {
            .Format = desc.Format,
            .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
            .Texture2D = { .MostDetailedMip = 0, .MipLevels = 1, },
        };

        result = ID3D11Device_CreateShaderResourceView(global_d3d11.device, (ID3D11Resource*)texture_2d, &srv_desc, &gfx_texture->srv);
        // TODO: Maybe this is not fatal but leave it for checking.
        assert(SUCCEEDED(result) && "[GFX] Failed to create shader resource view.");
    }
    
    graphics_texture.platform = pack_generation_index(texture_generation, texture_index);
    graphics_texture.width = gfx_texture->width;
    graphics_texture.height = gfx_texture->height;

    return graphics_texture;
}

static graphics_create_target_function(gfx_create_target)
{
    u32 texture_generation = get_generation(texture.platform);
    u32 texture_index = get_index(texture.platform);
    gfx_texture_t* gfx_texture = global_textures + texture_index;

    if (texture_generation != gfx_texture->generation)
    {
        assert(!"[GFX] Texture generation does not match.");
    }

    assert(gfx_texture->texture && "[GFX] Invalid texture for target.");
    
    graphics_target_t graphics_target = { 0 };
    u32 target_index = next_target_index();
    gfx_target_t* gfx_target = get_gfx_target(target_index);
    u32 target_generation = gfx_target->generation;

    *gfx_target = (gfx_target_t){ 0 };

    D3D11_RENDER_TARGET_VIEW_DESC desc =
    {
        .Format = gfx_texture->format,
        .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
        .Texture2D = { .MipSlice = 0, },
    };

    HRESULT result = ID3D11Device_CreateRenderTargetView(global_d3d11.device, (ID3D11Resource*)gfx_texture->texture, &desc, &gfx_target->render_target);
    assert(SUCCEEDED(result) && "[GFX] Failed to create render target view.");

    gfx_target->width = gfx_texture->width;
    gfx_target->height = gfx_texture->height;
    gfx_target->generation = target_generation;
    
    graphics_target.platform = pack_generation_index(target_generation, target_index);

    return graphics_target;
}

static graphics_create_sampler_function(gfx_create_sampler)
{
    graphics_sampler_t graphics_sampler = { 0 };
    usize sampler_index = global_sampler_count++;
    gfx_sampler_t* gfx_sampler = global_samplers + sampler_index;

    D3D11_SAMPLER_DESC desc =
    {
        .Filter = map_filter(sampler_desc->filter),
        .AddressU = map_texture_address(sampler_desc->address_u),
        .AddressV = map_texture_address(sampler_desc->address_v),
        .AddressW = map_texture_address(sampler_desc->address_w),
        // NOTE: These parameters are fixed for now.
        .MipLODBias = 0,
        .MaxAnisotropy = 1,
        .MinLOD = 0,
        .MaxLOD = D3D11_FLOAT32_MAX,
    };

    HRESULT result = ID3D11Device_CreateSamplerState(global_d3d11.device, &desc, &gfx_sampler->sampler_state);
    assert(SUCCEEDED(result) && "[GFX] Failed to create sampler.");

    graphics_sampler.platform = sampler_index;

    return graphics_sampler;
}

static graphics_create_shader_function(gfx_create_shader)
{
    graphics_shader_t graphics_shader = { 0 };
    usize shader_index = global_shader_count++;
    gfx_shader_t* gfx_shader = global_shaders + shader_index;

    if (shader_desc->stage == STAGE_VERTEX_SHADER)
    {
        ID3D11Device_CreateVertexShader(global_d3d11.device, shader_desc->bytecode, shader_desc->bytecode_size, 0, (ID3D11VertexShader**)&gfx_shader->shader);
        
    }
    else if (shader_desc->stage == STAGE_PIXEL_SHADER)
    {
        ID3D11Device_CreatePixelShader(global_d3d11.device, shader_desc->bytecode, shader_desc->bytecode_size, 0, (ID3D11PixelShader**)&gfx_shader->shader);
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

    if (program_desc->attributes && program_desc->attribute_count > 0)
    {
        HRESULT result = ID3D11Device_CreateInputLayout(global_d3d11.device, descs, (UINT)program_desc->attribute_count, vertex_shader->bytecode, vertex_shader->bytecode_size, &gfx_program->input_layout);
        assert(SUCCEEDED(result) && "[GFX] Failed to create input layout.");
    }

    graphics_program.platform = program_index;

    return graphics_program;
}

static graphics_create_pipeline_function(gfx_create_pipeline)
{
    graphics_pipeline_t graphics_pipeline = { 0 };
    usize pipeline_index = global_pipeline_count++;
    gfx_pipeline_t* gfx_pipeline = global_pipelines + pipeline_index;

    D3D11_RASTERIZER_DESC desc =
    {
        .FillMode = pipeline_desc->wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID,
        .CullMode = pipeline_desc->cull ? D3D11_CULL_BACK : D3D11_CULL_NONE,
        .FrontCounterClockwise = TRUE,
        .DepthClipEnable = TRUE,
    };

    ID3D11Device_CreateRasterizerState(global_d3d11.device, &desc, &gfx_pipeline->rasterizer_state);

    graphics_pipeline.platform = pipeline_index;
    
    return graphics_pipeline;
}

static graphics_update_buffer_function(gfx_update_buffer)
{
    usize buffer_index = (usize)buffer.platform;
    gfx_buffer_t* gfx_buffer = global_buffers + buffer_index;
    
    if (gfx_buffer->usage == D3D11_USAGE_DYNAMIC)
    {
        D3D11_MAPPED_SUBRESOURCE mapped = { 0 };
        UINT map_type = (offset == 0 && size == gfx_buffer->size) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
        HRESULT result = ID3D11DeviceContext_Map(global_d3d11.context, (ID3D11Resource*)gfx_buffer->buffer, 0, map_type, 0, &mapped);
        assert(SUCCEEDED(result) && "[GFX] Failed to map buffer.");
        memcpy((u8*)mapped.pData + offset, src, size);
        ID3D11DeviceContext_Unmap(global_d3d11.context, (ID3D11Resource*)gfx_buffer->buffer, 0);
    }
    else
    {
        ID3D11DeviceContext_UpdateSubresource(global_d3d11.context, (ID3D11Resource*)gfx_buffer->buffer, 0, 0, src, 0, 0);
    }
}

static graphics_is_valid_texture_2d_function(gfx_is_valid_texture_2d)
{
    u32 texture_generation = get_generation(texture.platform);
    u32 texture_index = get_index(texture.platform);
    gfx_texture_t* gfx_texture = global_textures + texture_index;
    bool is_valid = (texture_generation == gfx_texture->generation) && gfx_texture->texture;

    return is_valid;
}

static graphics_is_valid_target_function(gfx_is_valid_target)
{
    u32 target_generation = get_generation(target.platform);
    u32 target_index = get_index(target.platform);
    gfx_target_t* gfx_target = get_gfx_target(target_index);
    bool is_valid = (target_generation == gfx_target->generation) && gfx_target->render_target;

    return is_valid;
}

static graphics_delete_buffer_function(gfx_delete_buffer)
{
    u32 buffer_generation = get_generation(buffer.platform);
    u32 buffer_index = get_index(buffer.platform);
    gfx_buffer_t* gfx_buffer = global_buffers + buffer_index;

    if (gfx_buffer->generation != buffer_generation)
    {
        // TODO: Can we just silently skip?
        assert(!"[GFX] Buffer generation does not match.");
        return;
    }

    ID3D11Buffer_Release(gfx_buffer->buffer);

    *gfx_buffer = (gfx_buffer_t){ 0 };
    gfx_buffer->generation = buffer_generation + 1;

    free_buffer_index(buffer_index);
}

// TODO: We need some kind of generation counter to handle deleted objects.
static graphics_delete_texture_2d_function(gfx_delete_texture_2d)
{
    u32 texture_generation = get_generation(texture.platform);
    u32 texture_index = get_index(texture.platform);
    gfx_texture_t* gfx_texture = global_textures + texture_index;

    if (gfx_texture->generation != texture_generation)
    {
        // TODO: Can we just silently skip?
        assert(!"[GFX] Texture generation does not match.");
        return;
    }

    ID3D11ShaderResourceView_Release(gfx_texture->srv);
    ID3D11Texture2D_Release(gfx_texture->texture);

    *gfx_texture = (gfx_texture_t){ 0 };
    gfx_texture->generation = texture_generation + 1;

    free_texture_index(texture_index);
}

static graphics_delete_target_function(gfx_delete_target)
{
    u32 target_generation = get_generation(target.platform);
    u32 target_index = get_index(target.platform);
    gfx_target_t* gfx_target = get_gfx_target(target_index);

    if (target_generation != gfx_target->generation)
    {
        // TODO: Can we just silently skip?
        assert(!"[GFX] Target generation does not match.");
        return;
    }

    ID3D11RenderTargetView_Release(gfx_target->render_target);
    
    *gfx_target = (gfx_target_t){ 0 };
    gfx_target->generation = target_generation + 1;

    free_target_index(target_index);
}

static graphics_set_buffer_function(gfx_set_buffer)
{
    usize buffer_index = (usize)buffer.platform;
    gfx_buffer_t* gfx_buffer = global_buffers + buffer_index;

    switch (gfx_buffer->bind)
    {
        case D3D11_BIND_VERTEX_BUFFER:
        {
            ID3D11DeviceContext_IASetVertexBuffers(global_d3d11.context, slot, 1, &gfx_buffer->buffer, &stride, &offset);
        } break;

        case D3D11_BIND_INDEX_BUFFER:
        {
            ID3D11DeviceContext_IASetIndexBuffer(global_d3d11.context, gfx_buffer->buffer, gfx_buffer->index_format, offset);
        } break;

        case D3D11_BIND_CONSTANT_BUFFER:
        {
            if (stage == STAGE_VERTEX_SHADER)
            {
                ID3D11DeviceContext_VSSetConstantBuffers(global_d3d11.context, slot, 1, &gfx_buffer->buffer);
            }
            else if (stage == STAGE_PIXEL_SHADER)
            {
                ID3D11DeviceContext_PSSetConstantBuffers(global_d3d11.context, slot, 1, &gfx_buffer->buffer);
            }
        } break;

        default:
        {
            assert(!"[GFX] Failed to set buffer.");
        } break;
    }
}

static graphics_set_program_function(gfx_set_program)
{
    usize program_index = (usize)program.platform;
    gfx_program_t* gfx_program = global_programs + program_index;

    ID3D11DeviceContext_VSSetShader(global_d3d11.context, gfx_program->vertex_shader, 0, 0);
    ID3D11DeviceContext_PSSetShader(global_d3d11.context, gfx_program->pixel_shader, 0, 0);
    ID3D11DeviceContext_IASetInputLayout(global_d3d11.context, gfx_program->input_layout);
}

static graphics_set_pipeline_function(gfx_set_pipeline)
{
    usize pipeline_index = (usize)pipeline.platform;
    gfx_pipeline_t* gfx_pipeline = global_pipelines + pipeline_index;

    ID3D11DeviceContext_RSSetState(global_d3d11.context, gfx_pipeline->rasterizer_state);
}

static graphics_set_samplers_function(gfx_set_samplers)
{
    // NOTE: This is just a made up limit.
    ID3D11SamplerState* samplers_states[16] = { 0 };

    for (u32 index = 0; index < count; ++index)
    {
        if (!samplers)
            // || !samplers[index].platform)
        {
            samplers_states[index] = 0;
            continue;
        }

        gfx_sampler_t* gfx_sampler = global_samplers + samplers[index].platform;
        samplers_states[index] = gfx_sampler->sampler_state ? gfx_sampler->sampler_state : 0;
    }

    switch (stage)
    {
        case STAGE_VERTEX_SHADER:
        {
            ID3D11DeviceContext_VSSetSamplers(global_d3d11.context, first_slot, count, samplers_states);
        } break;

        case STAGE_PIXEL_SHADER:
        {
            ID3D11DeviceContext_PSSetSamplers(global_d3d11.context, first_slot, count, samplers_states);
        } break;

        default:
        {
            assert(!"[GFX] Failed to set samplers.");
        } break;
    }
}

static graphics_set_srvs_function(gfx_set_srvs)
{
    // NOTE: This is just a made up limit.
    ID3D11ShaderResourceView* srvs[16] = { 0 };

    for (u32 index = 0; index < count; ++index)
    {
        if (!textures)
            // || !textures[index].platform)
        {
            srvs[index] = 0;
            continue;
        }
        u32 texture_generation = get_generation(textures[index].platform);
        u32 texture_index = get_index(textures[index].platform);
        gfx_texture_t* gfx_texture = global_textures + texture_index;

        if (texture_generation != gfx_texture->generation)
        {
            assert(!"[GFX] Texture generation does not match.");
        }
        
        srvs[index] = gfx_texture->srv ? gfx_texture->srv : 0;
    }

    switch (stage)
    {
        case STAGE_VERTEX_SHADER:
        {
            ID3D11DeviceContext_VSSetShaderResources(global_d3d11.context, first_slot, count, srvs);
        } break;

        case STAGE_PIXEL_SHADER:
        {
            ID3D11DeviceContext_PSSetShaderResources(global_d3d11.context, first_slot, count, srvs);
        } break;

        default:
        {
            assert(!"[GFX] Failed to set srvs.");
        } break;
    }
}

static graphics_get_backbuffer_target_function(gfx_get_backbuffer_target)
{
    graphics_target_t graphics_target = { 0 };
    usize target_index = GFX_BACKBUFFER_TARGET_INDEX;
    gfx_target_t* gfx_target = get_gfx_target(target_index);

    gfx_target->render_target = global_d3d11.rt_view;
    gfx_target->width = global_d3d11.width;
    gfx_target->height = global_d3d11.height;

    graphics_target.platform = target_index;

    return graphics_target;
}

static graphics_get_target_size_function(gfx_get_target_size)
{
    u32 target_generation = get_generation(target.platform);
    u32 target_index = get_index(target.platform);
    gfx_target_t* gfx_target = get_gfx_target(target_index);

    if (target_generation != gfx_target->generation)
    {
        assert(!"[GFX] Target generation does not match.");
        return;
    }
    
    assert(gfx_target->render_target && "[GFX] Invalid target.");

    *width = (u32)gfx_target->width;
    *height = (u32)gfx_target->height;
}

static graphics_begin_pass_function(gfx_begin_pass)
{
    u32 target_generation = get_generation(target.platform);
    u32 target_index = get_index(target.platform);
    gfx_target_t* gfx_target = get_gfx_target(target_index);

    if (target_generation != gfx_target->generation)
    {
        assert(!"[GFX] Target generation does not match.");
        return;
    }
    
    if (gfx_target->render_target)
    {
        ID3D11DeviceContext_OMSetRenderTargets(global_d3d11.context, 1, &gfx_target->render_target, 0);
        // TODO: We do not use any blend or depth/stencil states for now.
        ID3D11DeviceContext_OMSetBlendState(global_d3d11.context, 0, 0, 0xffffffff);
        ID3D11DeviceContext_OMSetDepthStencilState(global_d3d11.context, 0, 0);  

        assert(gfx_target->width > 0 && gfx_target->height > 0 && "[GFX] Invalid target size.");
        
        D3D11_VIEWPORT viewport =
        {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width = (FLOAT)gfx_target->width,
            .Height = (FLOAT)gfx_target->height,
            .MinDepth = 0,
            .MaxDepth = 1,
        };
        ID3D11DeviceContext_RSSetViewports(global_d3d11.context, 1, &viewport);

        if (pass_desc->clear_color)
        {
            ID3D11DeviceContext_ClearRenderTargetView(global_d3d11.context, gfx_target->render_target, pass_desc->clear_rgba);
        }

        // TODO: Depth/stencil etc.
    }

    assert(global_pass_count == 0 && "[GFX] Previous pass is still open.");
    ++global_pass_count;
}

static graphics_end_pass_function(gfx_end_pass)
{
    assert(global_pass_count == 1 && "[GFX] Multiple begin pass.");
    --global_pass_count;

    ID3D11ShaderResourceView* null_srvs[16] = { 0 };
    ID3D11DeviceContext_VSSetShaderResources(global_d3d11.context, 0, 16, null_srvs);
    ID3D11DeviceContext_PSSetShaderResources(global_d3d11.context, 0, 16, null_srvs);

    ID3D11RasterizerState* null_rs = 0;
    ID3D11DeviceContext_RSSetState(global_d3d11.context, null_rs);

    ID3D11DeviceContext_OMSetRenderTargets(global_d3d11.context, 0, 0, 0);

    ID3D11DeviceContext_IASetInputLayout(global_d3d11.context, 0);
    
    // ID3D11Buffer* null_buffer = 0;
    // UINT stride = 0;
    // UINT offset = 0;
    // ID3D11DeviceContext_IASetVertexBuffers(global_d3d11.context, 0, 1, &null_buffer, &stride, &offset);
}

static graphics_draw_function(gfx_draw)
{
    ID3D11DeviceContext_IASetPrimitiveTopology(global_d3d11.context, map_primitive_topology(topology));
    ID3D11DeviceContext_Draw(global_d3d11.context, vertex_count, start_vertex);
}

static graphics_draw_indexed_function(gfx_draw_indexed)
{
    ID3D11DeviceContext_IASetPrimitiveTopology(global_d3d11.context, map_primitive_topology(topology));
    ID3D11DeviceContext_DrawIndexed(global_d3d11.context, index_count, start_index, base_vertex);
}
