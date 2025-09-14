#pragma once

typedef struct dwrite_t
{
    struct IDWriteFactory* factory;
} dwrite_t;

typedef struct d2d1_t
{
    struct ID2D1Factory* factory;
    struct ID2D1RenderTarget* render_target;
    dwrite_t* dwrite;
} d2d1_t;
