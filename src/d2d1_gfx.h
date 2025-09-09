#pragma once

typedef struct dwrite_t
{
    struct IDWriteFactory* factory;
    // NOTE: This is not about setup. Text format can be change but for now it is here.
    struct IDWriteTextFormat* text_format;
} dwrite_t;

typedef struct d2d1_t
{
    struct ID2D1Factory* factory;
    struct ID2D1RenderTarget* render_target;
    struct ID2D1SolidColorBrush* solid_color_brush;
    dwrite_t* dwrite;
} d2d1_t;
