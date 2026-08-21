#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "dwrite_c.h"

#include "utils.h"

#pragma comment(lib, "dwrite")
#pragma comment(lib, "gdi32")

#undef assert
#define assert(x) do { if (!(x)) { fprintf(stderr, "%s\n", #x); __debugbreak(); } } while (0)

static inline u32 next_power_of_two(u32 value)
{
    if (value == 0)
    {
        value = 1;
    }
    else
    {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value++;
    }

    return value;
}

static void create_font(const char* font_path, f32 point_size)
{
    HRESULT result = S_OK;
    WCHAR font_path_wchar[128] = { 0 };
    i32 font_path_length = (i32)strlen(font_path);
    
    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, NULL, 0);
    assert(wchar_size + 1 < array_count(font_path_wchar) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, font_path_wchar, wchar_size);
    font_path_wchar[wchar_size] = L'\0';

    IDWriteFactory* factory = 0;
    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&factory);
    assert(SUCCEEDED(result) && "[DWRITE] Failed to create factory.");
    
    // IDWriteFontCollection* font_collection = 0;
    // result = IDWriteFactory_GetSystemFontCollection(global_d2d1.dwrite->factory, &font_collection, false);
    // assert(SUCCEEDED(result) && font_collection && "[GFX2D] Failed to get system font collection.");
    
    // u32 index = 0;
    // bool exists = 0;
    // result = IDWriteFontCollection_FindFamilyName(font_collection, font_name_wchar, &index, &exists);
    // assert(SUCCEEDED(result) && exists && "[GFX2D] Failed to find font family name.");

    // IDWriteFontFamily* font_family = 0;
    // result = IDWriteFontCollection_GetFontFamily(font_collection, index, &font_family);
    // assert(SUCCEEDED(result) && font_family && "[GFX2D] Failed to get font family.");

    // IDWriteFont* matching_font = 0;
    // result = IDWriteFontFamily_GetFirstMatchingFont(font_family, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &matching_font);
    // assert(SUCCEEDED(result) && matching_font && "[GFX2D] Failed to get first matching font.");

    // IDWriteFontFace* font_face = 0;
    // result = IDWriteFont_CreateFontFace(matching_font, &font_face);
    // assert(SUCCEEDED(result) && font_face && "[GFX2D] Failed to create font face.");

    ////////////////////
    IDWriteFontFile* font_file = 0;
    result = IDWriteFactory_CreateFontFileReference(factory, font_path_wchar, 0, &font_file);
    assert(SUCCEEDED(result) && font_file && "[GFX2D] Failed to create font file.");

    IDWriteFontFace* font_face = 0;
    result = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);
    assert(SUCCEEDED(result) && font_face && "[GFX2D] Failed to create font face.");

    IDWriteRenderingParams* default_rendering_params = 0;
    result = IDWriteFactory_CreateRenderingParams(factory, &default_rendering_params);
    assert(SUCCEEDED(result) && default_rendering_params && "[GFX2D] Failed to create default rendering params.");

    IDWriteRenderingParams* rendering_params = 0;
    result = IDWriteFactory_CreateCustomRenderingParams(factory,
                                                        1.0f,
                                                        IDWriteRenderingParams_GetEnhancedContrast(default_rendering_params),
                                                        IDWriteRenderingParams_GetClearTypeLevel(default_rendering_params),
                                                        IDWriteRenderingParams_GetPixelGeometry(default_rendering_params),
                                                        DWRITE_RENDERING_MODE_NATURAL,
                                                        &rendering_params);
    assert(SUCCEEDED(result) && rendering_params && "[GFX2D] Failed to create rendering params.");

    IDWriteGdiInterop* gdi_interop = 0;
    IDWriteFactory_GetGdiInterop(factory, &gdi_interop);
    assert(SUCCEEDED(result) && gdi_interop && "[GFX2D] Failed to get gdi interop.");

    DWRITE_FONT_METRICS font_metrics = { 0 };
    IDWriteFontFace_GetMetrics(font_face, &font_metrics);

    f32 dpi = 96.0f;
    f32 pixel_per_em = point_size * 1.0f / 72.0f * dpi;
    f32 pixel_per_design_unit = pixel_per_em / font_metrics.designUnitsPerEm;

    i32 raster_width = (i32)(8.0f * font_metrics.capHeight * pixel_per_design_unit);
    i32 raster_height = (i32)(8.0f * font_metrics.capHeight * pixel_per_design_unit);
    f32 raster_x = (f32)(raster_width / 2);
    f32 raster_y = (f32)(raster_height / 2);

    assert((f32)(i32)raster_x == raster_x);
    assert((f32)(i32)raster_y == raster_y);

    IDWriteBitmapRenderTarget* render_target = 0;
    result = IDWriteGdiInterop_CreateBitmapRenderTarget(gdi_interop, 0, raster_width, raster_height, &render_target);
    assert(SUCCEEDED(result) && render_target && "[GFX2D] Failed to create bitmap render target.");

    HDC device_context = IDWriteBitmapRenderTarget_GetMemoryDC(render_target);
    assert(device_context && "[GFX2D] Failed to get device context.");

    COLORREF background_color = RGB(0, 0, 0);
    COLORREF foreground_color = RGB(255, 255, 255);

    // Clear render target
    {
        HGDIOBJ original = SelectObject(device_context, GetStockObject(DC_PEN));
        SetDCPenColor(device_context, background_color);
        SelectObject(device_context, GetStockObject(DC_BRUSH));
        SetDCBrushColor(device_context, background_color);
        Rectangle(device_context, 0, 0, raster_width, raster_height);
        SelectObject(device_context, original);
    }

    i32 max_atlas_glyph_width = 0;
    i32 max_atlas_glyph_height = 0;

    for (u16 i = 0; i < IDWriteFontFace_GetGlyphCount(font_face); ++i)
    {
        u16 glyph_index = i;
        DWRITE_GLYPH_RUN glyph_run =
        {
            .fontFace = font_face,
            .fontEmSize = pixel_per_em,
            .glyphCount = 1,
            .glyphIndices = &glyph_index,
        };
        RECT bounding_box = { 0 };
        result = IDWriteBitmapRenderTarget_DrawGlyphRun(render_target, raster_x, raster_y,
                                                        DWRITE_MEASURING_MODE_NATURAL, &glyph_run, rendering_params,
                                                        foreground_color, &bounding_box);
        assert(SUCCEEDED(result) && "[GFX2D] Failed to draw glyph run.");

        i32 glyph_width = bounding_box.right - bounding_box.left;
        i32 glyph_height = bounding_box.bottom - bounding_box.top;

        if (glyph_width > max_atlas_glyph_width)
        {
            max_atlas_glyph_width = glyph_width;
        }

        if (glyph_height > max_atlas_glyph_height)
        {
            max_atlas_glyph_height = glyph_height;
        }
    }
    fprintf(stderr, "max_atlas_glyph_width: %d, max_atlas_glyph_height: %d\n", max_atlas_glyph_width, max_atlas_glyph_height);

    // Clear render target
    {
        HGDIOBJ original = SelectObject(device_context, GetStockObject(DC_PEN));
        SetDCPenColor(device_context, background_color);
        SelectObject(device_context, GetStockObject(DC_BRUSH));
        SetDCBrushColor(device_context, background_color);
        Rectangle(device_context, 0, 0, raster_width, raster_height);
        SelectObject(device_context, original);
    }

    // f32 ascent = (f32)font_metrics.ascent * pixel_per_design_unit;
    // f32 descent = (f32)font_metrics.descent * pixel_per_design_unit;
    // i32 cell_extent = (i32)ceilf(ascent + descent);
    // i32 padding = 2;
    // i32 atlas_glyph_width = cell_extent + 2 * padding;
    // i32 atlas_glyph_height = cell_extent + 2 * padding;

    i32 padding = 2;
    i32 atlas_glyph_width = max_atlas_glyph_width + 2 * padding;
    i32 atlas_glyph_height = max_atlas_glyph_height + 2 * padding;

    fprintf(stderr, "atlas_glyph_width: %d, atlas_glyph_height: %d\n", atlas_glyph_width, atlas_glyph_height);

    i32 bytes_per_pixel = 3;
    i32 atlas_glyph_count = IDWriteFontFace_GetGlyphCount(font_face);
    i32 atlas_glyph_column_count = (i32)(pow((f32)atlas_glyph_count, 0.5f) + 0.5f);
    i32 atlas_per_glyph_size = atlas_glyph_width * atlas_glyph_height * bytes_per_pixel;
    i32 atlas_glyph_row_count = ((atlas_glyph_count + atlas_glyph_column_count - 1) / atlas_glyph_column_count);
    i32 atlas_memory_size = atlas_per_glyph_size * atlas_glyph_row_count * atlas_glyph_column_count;
    u8* atlas_memory = (u8*)malloc(atlas_memory_size);
    memset(atlas_memory, 0, atlas_memory_size);

    for (u16 i = 0; i < atlas_glyph_count; ++i)
    {
        // u16 glyph_index = i + min_codepoint_index;
        u16 glyph_index = i;
        // u32 codepoint = i + 'a';
        // IDWriteFontFace_GetGlyphIndices(font_face, &codepoint, 1, &glyph_index);
        // fprintf(stderr, "glyph_index: %u\n", glyph_index);

        DWRITE_GLYPH_RUN glyph_run =
        {
            .fontFace = font_face,
            .fontEmSize = pixel_per_em,
            .glyphCount = 1,
            .glyphIndices = &glyph_index,
        };
        RECT bounding_box = { 0 };
        result = IDWriteBitmapRenderTarget_DrawGlyphRun(render_target, raster_x, raster_y,
                                                        DWRITE_MEASURING_MODE_NATURAL, &glyph_run, rendering_params,
                                                        foreground_color, &bounding_box);
        assert(SUCCEEDED(result) && "[GFX2D] Failed to draw glyph run.");

        i32 texture_width = bounding_box.right - bounding_box.left;
        i32 texture_height = bounding_box.bottom - bounding_box.top;
        fprintf(stderr, "texture_width: %d, texture_height: %d\n", texture_width, texture_height);

        // Get bitmap
        HBITMAP bitmap = (HBITMAP)GetCurrentObject(device_context, OBJ_BITMAP);
        DIBSECTION dib = { 0 };
        GetObject(bitmap, sizeof(dib), &dib);

        i32 x_index = i % atlas_glyph_column_count;
        i32 y_index = i / atlas_glyph_column_count;
        i32 x_offset = x_index * atlas_glyph_width * bytes_per_pixel;
        i32 y_offset = y_index * (atlas_glyph_column_count * atlas_per_glyph_size);
        u8* atlas_glyph_line = atlas_memory + x_offset + y_offset;

        assert(dib.dsBm.bmBitsPixel == 32);
        int32_t in_pitch  = dib.dsBm.bmWidthBytes;
        int32_t out_pitch = atlas_glyph_column_count * atlas_glyph_width * bytes_per_pixel;
        uint8_t *in_line  = (uint8_t*)dib.dsBm.bmBits + bounding_box.left*4 + bounding_box.top*in_pitch;
        uint8_t *out_line = atlas_glyph_line + padding * out_pitch + padding * bytes_per_pixel;

        for (int32_t y = 0; y < texture_height; y += 1)
        {
            uint8_t *in_pixel  = in_line;
            uint8_t *out_pixel = out_line;            for (int32_t x = 0; x < texture_width; x += 1)
            {
                out_pixel[0] = in_pixel[2];
                out_pixel[1] = in_pixel[1];
                out_pixel[2] = in_pixel[0];
                in_pixel += 4;
                out_pixel += 3;
            }
            in_line += in_pitch;
            out_line += out_pitch;
        }

        // Clear  render target
        {
            HGDIOBJ original = SelectObject(device_context, GetStockObject(DC_PEN));
            SetDCPenColor(device_context, background_color);
            SelectObject(device_context, GetStockObject(DC_BRUSH));
            SetDCBrushColor(device_context, background_color);
            Rectangle(device_context,
                      bounding_box.left, bounding_box.top,
                      bounding_box.right, bounding_box.bottom);
            SelectObject(device_context, original);
        }
    }

    FILE* ppm_file = fopen("font_atlas_test.ppm", "wb");
    assert(ppm_file && "[GFX2D] Failed to open ppm file");

    i32 atlas_width = atlas_glyph_column_count * atlas_glyph_width;
    i32 atlas_height = atlas_glyph_row_count * atlas_glyph_height;
    fprintf(ppm_file, "P3\n"
                      "%d %d\n"
                      "255\n",
            atlas_width, atlas_height);

    for (i32 y = 0; y < atlas_height; ++y)
    {
        u8* atlas_row = atlas_memory + y * atlas_width * bytes_per_pixel;

        for (i32 x = 0; x < atlas_width; ++x)
        {
            u8* atlas_pixel = atlas_row + x * bytes_per_pixel;
            fprintf(ppm_file, "%d %d %d ", atlas_pixel[0], atlas_pixel[1], atlas_pixel[2]);
        }
        fprintf(ppm_file, "\n");
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int show_cmd)
{
    (void)instance, (void)prev_instance; (void)cmdline; (void)show_cmd;
    create_font("C:\\Windows\\Fonts\\Arial.ttf", 12);

    return 0;
}
// int WinMainCRTStartup(void)
// {
//     return 0;
// }
