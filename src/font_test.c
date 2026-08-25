#define COBJMACROS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include <d3d11.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#include "dwrite_c.h"

#include "utils.h"
#include "maths.h"

#include "d3d11_gfx.h"
#include "d3d11_gfx.c"

#include "../shader/font_test_vertex_shader.h"
#include "../shader/font_test_pixel_shader.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#pragma comment(lib, "dwrite")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")

#undef assert
#define assert(x) do { if (!(x)) { fprintf(stderr, "%s\n", #x); __debugbreak(); } } while (0)

typedef struct window_t
{
    HWND hwnd;
    WINDOWPLACEMENT placement;
    i32 width;
    i32 height;
    i32 current_width;
    i32 current_height;
    bool quit;
    IDXGISwapChain1* swap_chain;
    d3d11_t* d3d11;
} window_t;

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
    f32 uv_x;
    f32 uv_y;
    f32 uv_width;
    f32 uv_height;
} glyph_metrics_t;

typedef struct font_atlas_vertex_data_t
{
    vec2 position;
    vec2 uv;
} font_atlas_vertex_data_t;

typedef struct font_atlas_pass_t
{
    ID3D11VertexShader* vertex_shader;
    ID3D11PixelShader* pixel_shader;
    ID3D11InputLayout* input_layout;

    ID3D11SamplerState* sampler_state;
    ID3D11RasterizerState* rasterizer_state;
    ID3D11BlendState* blend_state;
} font_atlas_pass_t;

typedef struct font_atlas_t
{
    IDWriteFontFace* face;
    ID3D11Texture2D* atlas;
    ID3D11ShaderResourceView* atlas_srv;
    glyph_metrics_t* metrics;
    u32 metrics_count;

    font_atlas_pass_t pass;
} font_atlas_t;

static const codepoint_range_t global_latin_codepoint_ranges[] =
{
    { 0x0020, 0x007E }, // Basic Latin (printable ASCII)
    { 0x00A0, 0x00FF }, // Latin-1 Supplement:  ö ü ç ó é ñ à ...
    { 0x0100, 0x017F }, // Latin Extended-A:    ł ą ę ś ż ź ć ń  (Polish)
                        //                      ğ ş İ ı          (Turkish)
};

static window_t global_window;

static u32 font_atlas_calculate_glyph_metrics_index(u16 glyph_index)
{
    u32 glyph_metrics_index = 0;
    
    for (u32 i = 0; i < array_count(global_latin_codepoint_ranges); ++i)
    {
        codepoint_range_t codepoint_range = global_latin_codepoint_ranges[i];

        if (glyph_index >= codepoint_range.start && glyph_index <= codepoint_range.end)
        {
            glyph_metrics_index = glyph_index - codepoint_range.start;
            break;
        }

        glyph_metrics_index += codepoint_range.end - codepoint_range.start;
    }

    return glyph_metrics_index;
}

static void font_atlas_create_pass(font_atlas_pass_t* font_atlas_pass)
{
    HRESULT result = 0;
    D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(font_atlas_vertex_data_t, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(font_atlas_vertex_data_t, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    
    ID3D11Device_CreateVertexShader(global_window.d3d11->device, font_test_vshader, sizeof(font_test_vshader), 0, &font_atlas_pass->vertex_shader);
    ID3D11Device_CreatePixelShader(global_window.d3d11->device, font_test_pshader, sizeof(font_test_pshader), 0, &font_atlas_pass->pixel_shader);

    result = ID3D11Device_CreateInputLayout(global_window.d3d11->device, input_element_desc, array_count(input_element_desc), font_test_vshader, sizeof(font_test_vshader), &font_atlas_pass->input_layout);
    assert(SUCCEEDED(result) && "Failed to create input layout.");

    D3D11_SAMPLER_DESC sampler_desc =
    {
        .Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
        .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
        .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
        .MipLODBias = 0,
        .MaxAnisotropy = 1,
        .MinLOD = 0,
        .MaxLOD = D3D11_FLOAT32_MAX,
    };
    result = ID3D11Device_CreateSamplerState(global_window.d3d11->device, &sampler_desc, &font_atlas_pass->sampler_state);
    assert(SUCCEEDED(result) && "Failed to create sampler state.");

    D3D11_RASTERIZER_DESC rasterizer_desc =
    {
        .FillMode = D3D11_FILL_SOLID,
        .CullMode = D3D11_CULL_NONE,
        .FrontCounterClockwise = TRUE,
        .DepthClipEnable = FALSE,
    };

    result = ID3D11Device_CreateRasterizerState(global_window.d3d11->device, &rasterizer_desc, &font_atlas_pass->rasterizer_state);
    assert(SUCCEEDED(result) && "Failed to create rasterizer state.");
    
    D3D11_BLEND_DESC blend_desc =
    {
        .RenderTarget[0].BlendEnable = TRUE,
        .RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
        .RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA,
        .RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
        .RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD,
        .RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE,
        .RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
        .RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD,
    };

    result = ID3D11Device_CreateBlendState(global_window.d3d11->device, &blend_desc, &font_atlas_pass->blend_state);
    assert(SUCCEEDED(result) && "Failed to create blend state.");
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

    i32 padding = 0;
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

    const i32 bytes_per_pixel = 4;
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

        u32 glyph_metrics_index = font_atlas_calculate_glyph_metrics_index(glyph_index);
        glyph_metrics_t* metrics = glyph_metrics + glyph_metrics_index;

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
        metrics->uv_x = (f32)rect->x / (f32)atlas_width;
        metrics->uv_y = (f32)rect->y / (f32)atlas_height;
        metrics->uv_width = (f32)glyph_width / (f32)atlas_width;
        metrics->uv_height = (f32)glyph_height / (f32)atlas_height;

        if (glyph_index == 36)
        {
        fprintf(stderr, "atlas: %u x %u\n", atlas_width, atlas_height);
        fprintf(stderr, "rect: x=%d y=%d w=%d h=%d\n", rect->x, rect->y, rect->w, rect->h);
        fprintf(stderr, "glyph_width=%d glyph_height=%d\n", glyph_width, glyph_height);
        fprintf(stderr, "metrics->width=%f height=%f\n", metrics->width, metrics->height);
        fprintf(stderr, "uv_width*atlas_w=%f uv_height*atlas_h=%f\n",
                metrics->uv_width * atlas_width, metrics->uv_height * atlas_height);
        }

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
                out_pixel[3] = in_pixel[3];

                in_pixel += 4;
                out_pixel += 4;
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

    FILE* file = fopen("font_atlas_test.bin", "wb");
    size_t file_result = 0;
    file_result = fwrite(&atlas_width, sizeof(atlas_width), 1, file);
    assert(file_result == 1);
    file_result = fwrite(&atlas_height, sizeof(atlas_height), 1, file);
    assert(file_result == 1);
    file_result = fwrite(&bytes_per_pixel, sizeof(bytes_per_pixel), 1, file);
    assert(file_result == 1);
    file_result = fwrite(&atlas_memory_size, sizeof(atlas_memory_size), 1, file);
    assert(file_result == 1);
    file_result = fwrite(&atlas_glyph_count, sizeof(atlas_glyph_count), 1, file);
    assert(file_result == 1);
    file_result = fwrite(atlas_memory, atlas_memory_size, 1, file);
    assert(file_result == 1);
    file_result = fwrite(glyph_metrics, sizeof(glyph_metrics_t) * atlas_glyph_count, 1, file);
    assert(file_result == 1);

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
    result = ID3D11Device_CreateTexture2D(global_window.d3d11->device, &atlas_desc, &atlas_data, &atlas);
    font_atlas_cleanup_if_error(SUCCEEDED(result));
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create font atlas texture.");

    D3D11_SHADER_RESOURCE_VIEW_DESC atlas_srv_desc = (D3D11_SHADER_RESOURCE_VIEW_DESC)
    {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
        .Texture2D = { .MostDetailedMip = 0, .MipLevels = 1, },
    };

    ID3D11ShaderResourceView* atlas_srv = 0;
    result = ID3D11Device_CreateShaderResourceView(global_window.d3d11->device, (ID3D11Resource*)atlas, &atlas_srv_desc, &atlas_srv);
    font_atlas_cleanup_if_error(SUCCEEDED(result));
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create font atlas srv.");
    
    font_atlas.face = font_face;
    font_atlas.atlas = atlas;
    font_atlas.atlas_srv = atlas_srv;
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
    if (atlas_srv && cleanup_error) ID3D11ShaderResourceView_Release(atlas);

    return font_atlas;
}

static font_atlas_t font_atlas_create_from_file(const char* font_path, f32 point_size)
{
    font_atlas_t font_atlas = { 0 };
    HRESULT result = S_OK;
    bool cleanup_error = false;
    IDWriteFontFile* font_file = 0;
    IDWriteFontFace* font_face = 0;
    WCHAR font_path_wchar[128] = { 0 };
    i32 font_path_length = (i32)strlen(font_path);

    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, NULL, 0);
    assert(wchar_size + 1 < array_count(font_path_wchar) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, font_path_wchar, wchar_size);
    font_path_wchar[wchar_size] = L'\0';

    IDWriteFactory* factory = 0;
    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&factory);
    assert(SUCCEEDED(result) && "[DWRITE] Failed to create factory.");

    result = IDWriteFactory_CreateFontFileReference(factory, font_path_wchar, 0, &font_file);
    assert(SUCCEEDED(result) && font_file && "[GFX2D] Failed to create font file.");

    result = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);
    assert(SUCCEEDED(result) && font_face && "[GFX2D] Failed to create font face.");
    
    FILE* file = fopen("font_atlas_test.bin", "rb");
    size_t file_result = 0;
    // fseek(file, 0, SEEK_END);
    // i32 file_size = ftell(file);
    // fseek(file, 0, SEEK_SET);
    // fprintf(stderr, "file_size: %d\n", file_size);

    i32 atlas_width = 0;
    i32 atlas_height = 0;
    i32 bytes_per_pixel = 0;
    i32 atlas_memory_size = 0;
    u32 metrics_count = 0;
    
    file_result = fread(&atlas_width, sizeof(atlas_width), 1, file);
    assert(file_result == 1 && "fread: atlas_width");
    file_result = fread(&atlas_height, sizeof(atlas_height), 1, file);
    assert(file_result == 1 && "fread: atlas_height");
    file_result = fread(&bytes_per_pixel, sizeof(bytes_per_pixel), 1, file);
    assert(file_result == 1 && "fread: bytes_per_pixel");
    file_result = fread(&atlas_memory_size, sizeof(atlas_memory_size), 1, file);
    assert(file_result == 1 && "fread: atlas_memory_size");
    file_result = fread(&metrics_count, sizeof(metrics_count), 1, file);
    assert(file_result == 1 && "fread: metrics_count");

    fprintf(stderr, "atlas_width: %d, atlas_height: %d, bytes_per_pixel: %d, atlas_memory_size: %d, metrics_count: %u\n",
            atlas_width, atlas_height, bytes_per_pixel, atlas_memory_size, metrics_count);

    u8* atlas_memory = malloc(atlas_memory_size);
    glyph_metrics_t* glyph_metrics = malloc(metrics_count * sizeof(glyph_metrics_t));
    
    file_result = fread(atlas_memory, atlas_memory_size, 1, file);
    assert(file_result == 1 && "fread: atlas_memory");
    file_result = fread(glyph_metrics, metrics_count * sizeof(glyph_metrics_t), 1, file);
    assert(file_result == 1 && "fread: glyph_metrics");
    
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
    result = ID3D11Device_CreateTexture2D(global_window.d3d11->device, &atlas_desc, &atlas_data, &atlas);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create font atlas texture.");

    D3D11_SHADER_RESOURCE_VIEW_DESC atlas_srv_desc = (D3D11_SHADER_RESOURCE_VIEW_DESC)
    {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
        .Texture2D = { .MostDetailedMip = 0, .MipLevels = 1, },
    };

    ID3D11ShaderResourceView* atlas_srv = 0;
    result = ID3D11Device_CreateShaderResourceView(global_window.d3d11->device, (ID3D11Resource*)atlas, &atlas_srv_desc, &atlas_srv);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create font atlas srv.");

    font_atlas.face = font_face;
    font_atlas.atlas = atlas;
    font_atlas.atlas_srv = atlas_srv;
    font_atlas.metrics = glyph_metrics;
    font_atlas.metrics_count = metrics_count;

    return font_atlas;
}

static void font_atlas_draw_glyph(font_atlas_t* font_atlas)
{
    font_atlas_pass_t* font_atlas_pass = &font_atlas->pass;

    u32 codepoint = 'A';
    u16 glyph_index = 0;
    IDWriteFontFace_GetGlyphIndices(font_atlas->face, &codepoint, 1, &glyph_index);

    glyph_metrics_t* metrics = font_atlas->metrics + font_atlas_calculate_glyph_metrics_index(glyph_index);

    f32 pen_x = 100.0f;
    f32 pen_y = 100.0f;

    f32 x0 = pen_x + metrics->offset_x;
    f32 y0 = pen_y + metrics->offset_y;
    f32 x1 = x0 + metrics->width;
    f32 y1 = y0 + metrics->height;
    
    f32 u0 = metrics->uv_x;
    f32 v0 = metrics->uv_y;
    f32 u1 = metrics->uv_x + metrics->uv_width;
    f32 v1 = metrics->uv_y + metrics->uv_height;

    font_atlas_vertex_data_t vertex_data[6] =
    {
        // { metrics->offset_x,                  metrics->offset_y,                   0.0f,                     0.0f },
        // { metrics->offset_x,                  metrics->offset_y + metrics->height, 0.0f,                     0.5f + metrics->uv_height },
        // { metrics->offset_x + metrics->width, metrics->offset_y + metrics->height, 0.5f + metrics->uv_width, 0.5f + metrics->uv_height},
        // { metrics->offset_x + metrics->width, metrics->offset_y + metrics->height, 0.5f + metrics->uv_width, 0.5f + metrics->uv_height},
        // { metrics->offset_x + metrics->width, metrics->offset_y,                   0.5f + metrics->uv_width, 0.0f },
        // { metrics->offset_x,                  metrics->offset_y,                   0.0f,                     0.0f },
        
        // { metrics->offset_x,                  metrics->offset_y,                   u0, v0 },
        // { metrics->offset_x,                  metrics->offset_y + metrics->height, u0, v1 },
        // { metrics->offset_x + metrics->width, metrics->offset_y + metrics->height, u1, v1 },
        // { metrics->offset_x + metrics->width, metrics->offset_y + metrics->height, u1, v1 },
        // { metrics->offset_x + metrics->width, metrics->offset_y,                   u1, v0 },
        // { metrics->offset_x,                  metrics->offset_y,                   u0, v0 },

        { x0, x0, u0, v0 },
        { x0, y1, u0, v1 },
        { x1, y1, u1, v1 },
        { x1, y1, u1, v1 },
        { x1, y0, u1, v0 },
        { x0, y0, u0, v0 },
    };

    static bool print = true;

    if (print)
    {
        for (i32 i = 0; i < array_count(vertex_data); ++i)
        {
            fprintf(stderr, "(x, y): (%f, %f), (u, v): (%f, %f)\n", vertex_data[i].position.x, vertex_data[i].position.y, vertex_data[i].uv.u, vertex_data[i].uv.v);
        }

        print = false;
    }

    D3D11_BUFFER_DESC vertex_buffer_desc =
    {
        .ByteWidth = (UINT)sizeof(vertex_data),
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = 0,
    };

    D3D11_SUBRESOURCE_DATA vertex_buffer_data =
    {
        .pSysMem = vertex_data,
    };

    ID3D11Buffer* vertex_buffer = 0;
    HRESULT result = ID3D11Device_CreateBuffer(global_window.d3d11->device, &vertex_buffer_desc, &vertex_buffer_data, &vertex_buffer);
    assert(SUCCEEDED(result) && "Failed to create vertex buffer.");

    vec4 viewport_size = v4((f32)global_window.width, (f32)global_window.height, 0.0f, 0.0f);
    
    D3D11_BUFFER_DESC param_buffer_desc =
    {
        .ByteWidth = (UINT)sizeof(viewport_size),
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = 0,
    };

    D3D11_SUBRESOURCE_DATA param_buffer_data =
    {
        .pSysMem = &viewport_size,
    };

    ID3D11Buffer* param_buffer = 0;
    result = ID3D11Device_CreateBuffer(global_window.d3d11->device, &param_buffer_desc, &param_buffer_data, &param_buffer);
    assert(SUCCEEDED(result) && "Failed to create buffer.");

    // ID3D11DeviceContext_OMSetRenderTargets(global_window.d3d11->context, 1, &global_window.d3d11->rt_view, global_window.d3d11->ds_view);
    ID3D11DeviceContext_OMSetRenderTargets(global_window.d3d11->context, 1, &global_window.d3d11->rt_view, 0);

    FLOAT clear_rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    ID3D11DeviceContext_ClearRenderTargetView(global_window.d3d11->context, global_window.d3d11->rt_view, clear_rgba);

    D3D11_VIEWPORT viewport =
    {
        .TopLeftX = 0,
        .TopLeftY = 0,
        .Width = (FLOAT)global_window.width,
        .Height = (FLOAT)global_window.height,
        .MinDepth = 0,
        .MaxDepth = 1,
    };
    ID3D11DeviceContext_RSSetViewports(global_window.d3d11->context, 1, &viewport);
    
    ID3D11DeviceContext_VSSetShader(global_window.d3d11->context, font_atlas_pass->vertex_shader, 0, 0);
    ID3D11DeviceContext_PSSetShader(global_window.d3d11->context, font_atlas_pass->pixel_shader, 0, 0);
    ID3D11DeviceContext_IASetInputLayout(global_window.d3d11->context, font_atlas_pass->input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(global_window.d3d11->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    u32 stride = sizeof(font_atlas_vertex_data_t);
    u32 offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(global_window.d3d11->context, 0, 1, &vertex_buffer, &stride, &offset);
    ID3D11DeviceContext_VSSetConstantBuffers(global_window.d3d11->context, 0, 1, &param_buffer);
    
    ID3D11DeviceContext_PSSetShaderResources(global_window.d3d11->context, 0, 1, &font_atlas->atlas_srv);
    ID3D11DeviceContext_PSSetSamplers(global_window.d3d11->context, 0, 1, &font_atlas_pass->sampler_state);

    ID3D11DeviceContext_RSSetState(global_window.d3d11->context, font_atlas_pass->rasterizer_state);
    ID3D11DeviceContext_OMSetBlendState(global_window.d3d11->context, font_atlas_pass->blend_state, 0, ~0U);

    ID3D11DeviceContext_Draw(global_window.d3d11->context, array_count(vertex_data), 0);

    ID3D11Buffer_Release(vertex_buffer);
    ID3D11Buffer_Release(param_buffer);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_DESTROY:
        case WM_CLOSE:
        {
            fprintf(stderr, "destroy\n");
            PostQuitMessage(0);
        } break;
        
        case WM_KEYDOWN:
        {
            if (wparam == VK_ESCAPE)
            {
                PostQuitMessage(0);
            }
        } break;

        default:
        {
            result = DefWindowProc(hwnd, message, wparam, lparam);
        } break;
    }

    return result;
}

static bool resize_back_buffer(window_t* window)
{
    bool resized = false;
    // NOTE: Get current size for window client area.
    RECT rect = { 0 };
    GetClientRect(window->hwnd, &rect);
    window->width = rect.right - rect.left;
    window->height = rect.bottom - rect.top;

    if (window->d3d11->rt_view == 0 || window->width != window->current_width || window->height != window->current_height)
    {
        if (window->d3d11->rt_view)
        {
            ID3D11DeviceContext_ClearState(window->d3d11->context);
            ID3D11Texture2D_Release(window->d3d11->back_buffer);
            ID3D11RenderTargetView_Release(window->d3d11->rt_view);
            // ID3D11DepthStencilView_Release(window->d3d11->ds_view);
            window->d3d11->back_buffer = 0;
            window->d3d11->rt_view = 0;
            // window->d3d11->ds_view = 0;
        }

        if (window->width != 0 && window->height != 0)
        {
            HRESULT result = IDXGISwapChain1_ResizeBuffers(window->swap_chain, 0, window->width, window->height, DXGI_FORMAT_UNKNOWN, 0);
            assert(SUCCEEDED(result) && "[DXGI] Failed to resize swap chain.");

            IDXGISwapChain1_GetBuffer(window->swap_chain, 0, &IID_ID3D11Texture2D, (void**)&window->d3d11->back_buffer);
            
            D3D11_RENDER_TARGET_VIEW_DESC backbuffer_target =
            {
                .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            };
            ID3D11Device_CreateRenderTargetView(window->d3d11->device, (ID3D11Resource*)window->d3d11->back_buffer, &backbuffer_target, &window->d3d11->rt_view);

            // D3D11_TEXTURE2D_DESC depth_desc =
            // {
            //     .Width = window->width,
            //     .Height = window->height,
            //     .MipLevels = 1,
            //     .ArraySize = 1,
            //     .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
            //     .SampleDesc = { 1, 0 },
            //     .Usage = D3D11_USAGE_DEFAULT,
            //     .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            // };

            // ID3D11Texture2D* depth = 0;
            // ID3D11Device_CreateTexture2D(window->d3d11->device, &depth_desc, 0, &depth);
            // ID3D11Device_CreateDepthStencilView(window->d3d11->device, (ID3D11Resource*)depth, 0, &window->d3d11->ds_view);
            // ID3D11Texture2D_Release(depth);
        }

        window->d3d11->width = window->width;
        window->d3d11->height = window->height;
        window->current_width = window->width;
        window->current_height = window->height;
        resized = true;
    }

    return resized;
}

static void create_window(window_t* window, i32 width, i32 height)
{
    if (window && !window->hwnd)
    {
        WNDCLASSEX window_class =
        {
            .cbSize = sizeof(window_class),
            .lpfnWndProc = window_proc,
            .hInstance = GetModuleHandle(0),
            .hIcon = LoadIcon(0, IDI_APPLICATION),
            .hCursor = LoadCursor(0, IDC_ARROW),
            .lpszClassName = "font_test_window",
        };

        ATOM window_class_atom = RegisterClassEx(&window_class);
        assert(window_class_atom && "[WINDOW] Failed to register window class.");

        HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP,
                                   window_class.lpszClassName, "Font Test Window",
                                   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                   CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                   0, 0, window_class.hInstance, 0);
        assert(hwnd && "[WINDOW] Failed to create window.");

        window->hwnd = hwnd;
        window->width = width;
        window->height = height;
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int show_cmd)
{
    (void)instance, (void)prev_instance; (void)cmdline; (void)show_cmd;

    create_window(&global_window, CW_USEDEFAULT, CW_USEDEFAULT);
    global_window.d3d11 = d3d11_init();
    global_window.swap_chain = d3d11_create_swap_chain(global_window.hwnd, global_window.d3d11);
    
    font_atlas_t font_atlas = font_atlas_create("C:\\Windows\\Fonts\\Arial.ttf", 12);
    // font_atlas_t font_atlas = font_atlas_create_from_file("C:\\Windows\\Fonts\\Arial.ttf", 12);
    font_atlas_create_pass(&font_atlas.pass);

    while (!global_window.quit)
    {
        resize_back_buffer(&global_window);

        MSG message = { 0 };

        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                global_window.quit = true;
            }
        
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        
        font_atlas_draw_glyph(&font_atlas);
        
        IDXGISwapChain1_Present(global_window.swap_chain, 0, 0);
    }

    fprintf(stderr, "Done\n");
    
    return 0;
}
// int WinMainCRTStartup(void)
// {
//     return 0;
// }
