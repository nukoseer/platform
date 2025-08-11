#pragma once

typedef enum shader_type_t
{
    NULL_SHADER_TYPE,

    VERTEX_SHADER_TYPE,
    PIXEL_SHADER_TYPE,

    COUNT_SHADER_TYPE,
} shader_type_t;

typedef struct d3d11_buffer_t
{
    ID3D11Buffer* buffer;
} d3d11_buffer_t;

typedef struct d3d11_shader_t
{
    shader_type_t type;

    union
    {
        ID3D11VertexShader* vertex;
        ID3D11PixelShader* pixel;
    };
} d3d11_shader_t;

typedef struct d3d11_input_layout_t
{
    ID3D11InputLayout* layout;
} d3d11_input_layout_t;

typedef struct d3d11_compile_t
{
    ID3DBlob* code;
    void* data;
    size_t size;
} d3d11_compile_t;

typedef struct d3d11_t
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11RenderTargetView* rt_view;
} d3d11_t;

