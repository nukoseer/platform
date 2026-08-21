#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "dwrite_c.h"

#include "utils.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#pragma comment(lib, "dwrite")
#pragma comment(lib, "gdi32")

#undef assert
#define assert(x) do { if (!(x)) { fprintf(stderr, "%s\n", #x); __debugbreak(); } } while (0)

typedef struct { u32 lo, hi; } codepoint_range_t;

static const codepoint_range_t global_latin_codepoint_ranges[] =
{
    { 0x0020, 0x007E }, // Basic Latin (printable ASCII)
    { 0x00A0, 0x00FF }, // Latin-1 Supplement:  ö ü ç ó é ñ à ...
    { 0x0100, 0x017F }, // Latin Extended-A:    ł ą ę ś ż ź ć ń  (Polish)
                        //                      ğ ş İ ı          (Turkish)
};

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

    u32 codepoints[512] = { 0 };
    i32 codepoint_count = 0;

    for (i32 i = 0; i < array_count(global_latin_codepoint_ranges); ++i)
    {
        codepoint_range_t range = global_latin_codepoint_ranges[i];

        for (u32 codepoint = range.lo; codepoint <= range.hi; ++codepoint)
        {
            codepoints[codepoint_count++] = codepoint;
        }
    }
    assert(codepoint_count <= array_count(codepoints) && "[GFX2D] Codepoint buffer overflow.");

    u16 glyph_indices[512] = { 0 };
    result = IDWriteFontFace_GetGlyphIndices(font_face, codepoints, codepoint_count, glyph_indices);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph indices.");

    // NOTE: Glyph index 0 is reserved for the missing glyph.
    // Use bitset if this gets too large.
    u16 seen_glyph_indices[65536] = { 0 };
    u16 baked_glyph_indices[512] = { 0 };
    i32 baked_glyph_count = 0;

    baked_glyph_indices[baked_glyph_count++] = 0;

    for (i32 i = 0; i < codepoint_count; ++i)
    {
        u16 glyph_index = glyph_indices[i];

        if (glyph_index == 0)
        {
            continue;
        }

        
        if (seen_glyph_indices[glyph_index])
        {
            continue;
        }

        seen_glyph_indices[glyph_index] = 1;
        baked_glyph_indices[baked_glyph_count++] = glyph_index;
    }
    
    i32 atlas_glyph_count = baked_glyph_count;
    i32 num_rects = atlas_glyph_count;
    stbrp_rect* rects = (stbrp_rect*)malloc(num_rects * sizeof(stbrp_rect));
    memset(rects, 0, num_rects * sizeof(stbrp_rect));
    
    fprintf(stderr, "atlas_glyph_count: %d\n", atlas_glyph_count);

    i32 padding = 1;
    u64 total_area = 0;

    for (u16 i = 0; i < atlas_glyph_count; ++i)
    {
        u16 glyph_index = baked_glyph_indices[i];
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

        fprintf(stderr, "glyph_index: %u, bounding_box: (%d, %d, %d, %d)\n", glyph_index, bounding_box.left, bounding_box.top, bounding_box.right, bounding_box.bottom);
        
        i32 glyph_width = bounding_box.right - bounding_box.left;
        i32 glyph_height = bounding_box.bottom - bounding_box.top;
        stbrp_rect* rect = rects + i;

        rect->id = i;
        rect->w = glyph_width + padding * 2;
        rect->h = glyph_height + padding * 2;

        total_area += (u64)rect->w * (u64)rect->h;
    }
    
    i32 atlas_width = 128;
    while ((u64)atlas_width * atlas_width < total_area)
    {
        atlas_width *= 2;
    }
    i32 atlas_height = atlas_width;

    i32 num_nodes = atlas_width;
    stbrp_node* nodes = (stbrp_node*)malloc(num_nodes * sizeof(stbrp_node));
    memset(nodes, 0, num_nodes * sizeof(stbrp_node));
    
    stbrp_context context = { 0 };
    stbrp_init_target(&context, atlas_width, atlas_height, nodes, num_nodes);
    stbrp_setup_heuristic(&context, STBRP_HEURISTIC_Skyline_BL_sortHeight);

    result = stbrp_pack_rects(&context, rects, num_rects);
    assert(result && "[GFX2D] Failed to pack rectangles.");

    i32 used_height = 0;

    for (i32 i = 0; i < atlas_glyph_count; ++i)
    {
        stbrp_rect* rect = rects + i;
        i32 bottom = rect->y + rect->h;

        if (bottom > used_height)
        {
            used_height = bottom;
        }
    }

    i32 final_height = 1;
    while (final_height < used_height)
    {
        final_height *= 2;
    }
    atlas_height = final_height;

    fprintf(stderr, "atlas_width: %d, atlas_height: %d\n", atlas_width, atlas_height);

    i32 bytes_per_pixel = 3;
    i32 atlas_memory_size = atlas_width * atlas_height * bytes_per_pixel;
    u8* atlas_memory = (u8*)malloc(atlas_memory_size);
    memset(atlas_memory, 0, atlas_memory_size);

    // Clear render target
    {
        HGDIOBJ original = SelectObject(device_context, GetStockObject(DC_PEN));
        SetDCPenColor(device_context, background_color);
        SelectObject(device_context, GetStockObject(DC_BRUSH));
        SetDCBrushColor(device_context, background_color);
        Rectangle(device_context, 0, 0, raster_width, raster_height);
        SelectObject(device_context, original);
    }

    for (u16 i = 0; i < atlas_glyph_count; ++i)
    {
        u16 glyph_index = baked_glyph_indices[i];
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
        // fprintf(stderr, "texture_width: %d, texture_height: %d\n", texture_width, texture_height);

        // Get bitmap
        HBITMAP bitmap = (HBITMAP)GetCurrentObject(device_context, OBJ_BITMAP);
        DIBSECTION dib = { 0 };
        GetObject(bitmap, sizeof(dib), &dib);

        stbrp_rect* rect = rects + i;
        u8* atlas_glyph_line = atlas_memory + rect->x * bytes_per_pixel + rect->y * atlas_width * bytes_per_pixel;

        assert(dib.dsBm.bmBitsPixel == 32);
        i32 in_pitch  = dib.dsBm.bmWidthBytes;
        i32 out_pitch = atlas_width * bytes_per_pixel;
        u8* in_line  = (u8*)dib.dsBm.bmBits + bounding_box.left * 4 + bounding_box.top * in_pitch;
        u8* out_line = atlas_glyph_line + padding * out_pitch + padding * bytes_per_pixel;

        for (i32 y = 0; y < texture_height; y += 1)
        {
            u8* in_pixel  = in_line;
            u8* out_pixel = out_line;

            for (i32 x = 0; x < texture_width; x += 1)
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
