#pragma once

typedef struct d3d11_t
{
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11RenderTargetView* rt_view;
    // TODO: Not sure if this is a good idea.
    // NOTE: Backbuffer width and height.
    i32 width;
    i32 height;
} d3d11_t;

