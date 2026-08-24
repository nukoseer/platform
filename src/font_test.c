#define COBJMACROS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include <d3d11.h>
#include "dwrite_c.h"

#include "utils.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#pragma comment(lib, "dwrite")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "gdi32")

#undef assert
#define assert(x) do { if (!(x)) { fprintf(stderr, "%s\n", #x); __debugbreak(); } } while (0)

typedef struct codepoint_range_t
{
    u32 start;
    u32 end;
} codepoint_range_t;

typedef struct glyph_metrics_t
{
    f32 x, y;
    f32 offset_x, offset_y;
    f32 advance;
    f32 width;
    f32 height;
    f32 uv_width;
    f32 uv_height;
} glyph_metrics_t;

typedef struct font_atlas_t
{
    IDWriteFontFace* face;
    ID3D11Texture2D* atlas;
    glyph_metrics_t* metrics;
    u32 metrics_count;
} font_atlas_t;

static const codepoint_range_t global_latin_codepoint_ranges[] =
{
    { 0x0020, 0x007E }, // Basic Latin (printable ASCII)
    { 0x00A0, 0x00FF }, // Latin-1 Supplement:  ö ü ç ó é ñ à ...
    { 0x0100, 0x017F }, // Latin Extended-A:    ł ą ę ś ż ź ć ń  (Polish)
                        //                      ğ ş İ ı          (Turkish)
};

static ID3D11Device* global_d3d11_device = 0;
static ID3D11DeviceContext* global_d3d11_context = 0;

static void d3d11_init(void)
{
    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0,
                                       0, feature_levels, array_count(feature_levels),
                                       D3D11_SDK_VERSION, &global_d3d11_device, 0, &global_d3d11_context);
    assert(SUCCEEDED(result) && "Failed to create D3D11 device.");
}

#define font_atlas_cleanup_if_error(cond) do { if (!(cond)) { cleanup_error = true; goto cleanup; } } while (0)

static font_atlas_t font_atlas_create(const char* font_path, f32 point_size)
{
    font_atlas_t font_atlas = { 0 };
    HRESULT result = S_OK;
    bool cleanup_error = false;
    IDWriteFontFile* font_file = 0;
    IDWriteFontFace* font_face = 0;
    IDWriteRenderingParams* default_rendering_params = 0;
    IDWriteRenderingParams* rendering_params = 0;
    IDWriteGdiInterop* gdi_interop = 0;
    IDWriteBitmapRenderTarget* render_target = 0;
    stbrp_rect* rects = 0;
    stbrp_node* nodes = 0;
    u8* atlas_memory = 0;
    glyph_metrics_t* glyph_metrics = 0;
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
    result = IDWriteFactory_CreateFontFileReference(factory, font_path_wchar, 0, &font_file);
    font_atlas_cleanup_if_error(SUCCEEDED(result) && font_file);
    assert(SUCCEEDED(result) && font_file && "[GFX2D] Failed to create font file.");

    result = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);
    font_atlas_cleanup_if_error(SUCCEEDED(result) && font_face);
    assert(SUCCEEDED(result) && font_face && "[GFX2D] Failed to create font face.");

    result = IDWriteFactory_CreateRenderingParams(factory, &default_rendering_params);
    font_atlas_cleanup_if_error(SUCCEEDED(result) && default_rendering_params);
    assert(SUCCEEDED(result) && default_rendering_params && "[GFX2D] Failed to create default rendering params.");

    result = IDWriteFactory_CreateCustomRenderingParams(factory,
                                                        1.0f,
                                                        IDWriteRenderingParams_GetEnhancedContrast(default_rendering_params),
                                                        IDWriteRenderingParams_GetClearTypeLevel(default_rendering_params),
                                                        IDWriteRenderingParams_GetPixelGeometry(default_rendering_params),
                                                        DWRITE_RENDERING_MODE_NATURAL,
                                                        &rendering_params);
    font_atlas_cleanup_if_error(SUCCEEDED(result) && rendering_params);
    assert(SUCCEEDED(result) && rendering_params && "[GFX2D] Failed to create rendering params.");

    IDWriteFactory_GetGdiInterop(factory, &gdi_interop);
    font_atlas_cleanup_if_error(gdi_interop);
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

    result = IDWriteGdiInterop_CreateBitmapRenderTarget(gdi_interop, 0, raster_width, raster_height, &render_target);
    font_atlas_cleanup_if_error(SUCCEEDED(result) && render_target);
    assert(SUCCEEDED(result) && render_target && "[GFX2D] Failed to create bitmap render target.");

    HDC device_context = IDWriteBitmapRenderTarget_GetMemoryDC(render_target);
    font_atlas_cleanup_if_error(device_context);
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

        for (u32 codepoint = range.start; codepoint <= range.end; ++codepoint)
        {
            codepoints[codepoint_count++] = codepoint;
        }
    }
    font_atlas_cleanup_if_error(codepoint_count <= array_count(codepoints));
    assert(codepoint_count <= array_count(codepoints) && "[GFX2D] Codepoint buffer overflow.");

    u16 glyph_indices[512] = { 0 };
    result = IDWriteFontFace_GetGlyphIndices(font_face, codepoints, codepoint_count, glyph_indices);
    font_atlas_cleanup_if_error(SUCCEEDED(result));
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
    rects = (stbrp_rect*)malloc(num_rects * sizeof(stbrp_rect));
    font_atlas_cleanup_if_error(rects);
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
        font_atlas_cleanup_if_error(SUCCEEDED(result));
        assert(SUCCEEDED(result) && "[GFX2D] Failed to draw glyph run.");

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
    nodes = (stbrp_node*)malloc(num_nodes * sizeof(stbrp_node));
    font_atlas_cleanup_if_error(nodes);
    memset(nodes, 0, num_nodes * sizeof(stbrp_node));

    stbrp_context context = { 0 };
    stbrp_init_target(&context, atlas_width, atlas_height, nodes, num_nodes);
    stbrp_setup_heuristic(&context, STBRP_HEURISTIC_Skyline_BL_sortHeight);

    result = stbrp_pack_rects(&context, rects, num_rects);
    font_atlas_cleanup_if_error(result);
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

    const i32 bytes_per_pixel = 3;
    i32 atlas_memory_size = atlas_width * atlas_height * bytes_per_pixel;
    i32 glyph_metrics_size = atlas_glyph_count * sizeof(glyph_metrics_t);

    atlas_memory = (u8*)malloc(atlas_memory_size);
    font_atlas_cleanup_if_error(atlas_memory);
    
    glyph_metrics = (glyph_metrics_t*)malloc(glyph_metrics_size);

    memset(atlas_memory, 0, atlas_memory_size);
    memset(glyph_metrics, 0, glyph_metrics_size);

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
        font_atlas_cleanup_if_error(SUCCEEDED(result));
        assert(SUCCEEDED(result) && "[GFX2D] Failed to draw glyph run.");
        assert(0 <= bounding_box.left);
        assert(0 <= bounding_box.top);
        assert(bounding_box.right <= raster_width);
        assert(bounding_box.bottom <= raster_height);

        stbrp_rect* rect = rects + i;
        u8* atlas_glyph_line = atlas_memory + rect->x * bytes_per_pixel + rect->y * atlas_width * bytes_per_pixel;

        glyph_metrics_t* metrics = glyph_metrics + i;

        DWRITE_GLYPH_METRICS design_glyph_metrics = { 0 };
        result = IDWriteFontFace_GetDesignGlyphMetrics(font_face, &glyph_index, 1, &design_glyph_metrics, false);
        font_atlas_cleanup_if_error(SUCCEEDED(result));
        assert(SUCCEEDED(result) && "[GFX2D] Failed to get design glyph metrics.");

        i32 glyph_width = bounding_box.right - bounding_box.left;
        i32 glyph_height = bounding_box.bottom - bounding_box.top;

        metrics->x = (f32)rect->x;
        metrics->y = (f32)rect->y;
        metrics->offset_x = bounding_box.left - raster_x;
        metrics->offset_y = bounding_box.top - raster_y;
        metrics->advance = ceilf(design_glyph_metrics.advanceWidth * pixel_per_design_unit);
        metrics->width = (f32)glyph_width;
        metrics->height = (f32)glyph_height;
        metrics->uv_width = (f32)glyph_width / (f32)atlas_width;
        metrics->uv_height = (f32)glyph_height / (f32)atlas_height;

        HBITMAP bitmap = (HBITMAP)GetCurrentObject(device_context, OBJ_BITMAP);
        DIBSECTION dib = { 0 };
        GetObject(bitmap, sizeof(dib), &dib);

        font_atlas_cleanup_if_error(dib.dsBm.bmBitsPixel == 32);
        assert(dib.dsBm.bmBitsPixel == 32);
        i32 in_pitch  = dib.dsBm.bmWidthBytes;
        i32 out_pitch = atlas_width * bytes_per_pixel;
        u8* in_line  = (u8*)dib.dsBm.bmBits + bounding_box.left * 4 + bounding_box.top * in_pitch;
        u8* out_line = atlas_glyph_line + padding * out_pitch + padding * bytes_per_pixel;

        for (i32 y = 0; y < glyph_height; y += 1)
        {
            u8* in_pixel  = in_line;
            u8* out_pixel = out_line;

            for (i32 x = 0; x < glyph_width; x += 1)
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

    d3d11_init();

    
    D3D11_TEXTURE2D_DESC atlas_desc =
    {
        .Width = atlas_width,
        .Height = atlas_height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = { .Count = 1, .Quality = 0, },
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
    };

    D3D11_SUBRESOURCE_DATA atlas_data =
    {
        .pSysMem = atlas_memory,
        .SysMemPitch = bytes_per_pixel * atlas_width,
    };

    ID3D11Texture2D* atlas = 0;
    result = ID3D11Device_CreateTexture2D(global_d3d11_device, &atlas_desc, &atlas_data, &atlas);
    font_atlas_cleanup_if_error(SUCCEEDED(result));
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create font atlas texture.");

    font_atlas.face = font_face;
    font_atlas.atlas = atlas;
    font_atlas.metrics = glyph_metrics;

 cleanup:
    if (font_file) IDWriteFontFile_Release(font_file);
    if (default_rendering_params) IDWriteRenderingParams_Release(default_rendering_params);
    if (rendering_params) IDWriteRenderingParams_Release(rendering_params);
    if (gdi_interop)  IDWriteGdiInterop_Release(gdi_interop);
    if (render_target) IDWriteBitmapRenderTarget_Release(render_target);
    if (atlas_memory) free(atlas_memory);
    if (rects) free(rects);
    if (nodes) free(nodes);
    // NOTE: Do not release these if there is no error.
    if (font_face && cleanup_error) IDWriteFontFace_Release(font_face);
    if (glyph_metrics && cleanup_error) free(glyph_metrics);
    if (atlas && cleanup_error) ID3D11Texture2D_Release(atlas);

    return font_atlas;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int show_cmd)
{
    (void)instance, (void)prev_instance; (void)cmdline; (void)show_cmd;
    font_atlas_create("C:\\Windows\\Fonts\\Arial.ttf", 12);

    return 0;
}
// int WinMainCRTStartup(void)
// {
//     return 0;
// }
