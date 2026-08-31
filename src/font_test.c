#define COBJMACROS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include <d3d11.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>

#include "dwrite_c.h"
#include "d2d1_c.h"

#include "utils.h"
#include "maths.h"

#include "d3d11_gfx.h"
#include "d3d11_gfx.c"
#include "d2d1_gfx.h"
#include "d2d1_gfx.c"

#include "../shader/font_test_vertex_shader.h"
#include "../shader/font_test_pixel_shader.h"

#pragma comment(lib, "dwrite")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")
#pragma comment(lib, "d2d1")

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
    d2d1_t* d2d1;
} window_t;

typedef struct codepoint_range_t
{
    u32 start;
    u32 end;
} codepoint_range_t;

typedef struct glyph_info_t
{
    u16 x, y;
    u16 width, height;
    i16 offset_x, offset_y;
    f32 advance;
} glyph_info_t;

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
    i32 width, height;
    f32 ascent;
    f32 descent;
    f32 line_gap;
    f32 line_advance;
    ID3D11Texture2D* atlas;
    ID3D11ShaderResourceView* atlas_srv;
    glyph_info_t* glyph_infos;
    u32 glyph_info_count;
    u16 codepoint_to_glyph_index[0x17F + 1];

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
        .CullMode = D3D11_CULL_BACK,
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
    u8* atlas_memory = 0;
    glyph_info_t* glyph_infos = 0;
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
                                                        1.5f,
                                                        1.0f,
                                                        0.0f,
                                                        DWRITE_PIXEL_GEOMETRY_FLAT,
                                                        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                                                        // DWRITE_RENDERING_MODE_GDI_NATURAL,
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
    f32 ascent = font_metrics.ascent * pixel_per_design_unit;
    f32 descent = font_metrics.descent * pixel_per_design_unit;
    f32 line_gap = font_metrics.lineGap * pixel_per_design_unit;
    f32 line_advance = ascent + descent + line_gap;

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

    DWRITE_GLYPH_METRICS glyph_metrics[512] = { 0 };
    result = IDWriteFontFace_GetGdiCompatibleGlyphMetrics(font_face, pixel_per_em, 1.0f, 0, true, glyph_indices, codepoint_count, glyph_metrics, false);
    font_atlas_cleanup_if_error(SUCCEEDED(result));
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph metrics.");

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

        baked_glyph_indices[baked_glyph_count++] = glyph_index;
    }

    for (i32 i = 0; i < codepoint_count; ++i)
    {
        font_atlas.codepoint_to_glyph_index[codepoints[i]] = glyph_indices[i];
    }
    
    const i32 padding = 1;
    const i32 atlas_width = 1024;
    const i32 atlas_height = 512;
    const i32 max_glyph_width = 32;
    const i32 max_glyph_height = 32;
    const i32 bytes_per_pixel = 4;
    i32 atlas_memory_size = atlas_width * atlas_height * bytes_per_pixel;
    i32 total_atlas_glyph_count = IDWriteFontFace_GetGlyphCount(font_face);
    i32 glyph_infos_size = total_atlas_glyph_count * sizeof(glyph_info_t);

    atlas_memory = (u8*)malloc(atlas_memory_size);
    font_atlas_cleanup_if_error(atlas_memory);
    
    glyph_infos = (glyph_info_t*)malloc(glyph_infos_size);

    memset(atlas_memory, 0, atlas_memory_size);
    memset(glyph_infos, 0, glyph_infos_size);

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

    for (u16 i = 0; i < baked_glyph_count; ++i)
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

        i32 glyph_x = (i * max_glyph_width) % atlas_width;
        i32 glyph_y = ((i * max_glyph_width) / atlas_width) * max_glyph_height;
        i32 glyph_width = bounding_box.right - bounding_box.left;
        i32 glyph_height = bounding_box.bottom - bounding_box.top;

        assert(glyph_width <= max_glyph_width);
        assert(glyph_height <= max_glyph_height);
        assert(glyph_x + glyph_width <= atlas_width);
        assert(glyph_y + glyph_height <= atlas_height);

        glyph_infos[glyph_index] = (glyph_info_t)
        {
            .x = (u16)(glyph_x + padding),
            .y = (u16)(glyph_y + padding),
            .width = (u16)glyph_width,
            .height = (u16)glyph_height,
            .offset_x = (i16)(bounding_box.left - raster_x),
            .offset_y = (i16)(bounding_box.top - raster_y),
            .advance = ceilf(glyph_metrics[i].advanceWidth * pixel_per_design_unit),
        };

        HBITMAP bitmap = (HBITMAP)GetCurrentObject(device_context, OBJ_BITMAP);
        DIBSECTION dib = { 0 };
        GetObject(bitmap, sizeof(dib), &dib);

        u8* atlas_glyph_line = atlas_memory + glyph_x * bytes_per_pixel + glyph_y * atlas_width * bytes_per_pixel;
        
        font_atlas_cleanup_if_error(dib.dsBm.bmBitsPixel == 32);
        assert(dib.dsBm.bmBitsPixel == 32);
        i32 in_pitch = dib.dsBm.bmWidthBytes;
        i32 out_pitch = atlas_width * bytes_per_pixel;
        u8* in_line = (u8*)dib.dsBm.bmBits + bounding_box.left * 4 + bounding_box.top * in_pitch;
        u8* out_line = atlas_glyph_line + padding * out_pitch + padding * bytes_per_pixel;

        for (i32 y = 0; y < glyph_height; y += 1)
        {
            u8* in_pixel  = in_line;
            u8* out_pixel = out_line;

            for (i32 x = 0; x < glyph_width; x += 1)
            {
                // out_pixel[0] = in_pixel[2];
                // out_pixel[1] = in_pixel[1];
                // out_pixel[2] = in_pixel[0];
                // out_pixel[3] = in_pixel[3];
                out_pixel[0] = 255;
                out_pixel[1] = 255;
                out_pixel[2] = 255;
                out_pixel[3] = in_pixel[0];

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

    // FILE* ppm_file = fopen("font_atlas_test.ppm", "wb");
    // assert(ppm_file && "[GFX2D] Failed to open ppm file");

    // fprintf(ppm_file, "P3\n"
    //                   "%d %d\n"
    //                   "255\n",
    //         atlas_width, atlas_height);

    // for (i32 y = 0; y < atlas_height; ++y)
    // {
    //     u8* atlas_row = atlas_memory + y * atlas_width * bytes_per_pixel;

    //     for (i32 x = 0; x < atlas_width; ++x)
    //     {
    //         u8* atlas_pixel = atlas_row + x * bytes_per_pixel;
    //         fprintf(ppm_file, "%d %d %d ", atlas_pixel[0], atlas_pixel[1], atlas_pixel[2]);
    //     }
    //     fprintf(ppm_file, "\n");
    // }

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
    font_atlas.width = atlas_width;
    font_atlas.height = atlas_height;
    font_atlas.ascent = ascent;
    font_atlas.descent = descent;
    font_atlas.line_gap = line_gap;
    font_atlas.line_advance = line_advance;
    font_atlas.atlas = atlas;
    font_atlas.atlas_srv = atlas_srv;
    font_atlas.glyph_infos = glyph_infos;
    font_atlas.glyph_info_count = baked_glyph_count;

 cleanup:
    if (font_file) IDWriteFontFile_Release(font_file);
    if (default_rendering_params) IDWriteRenderingParams_Release(default_rendering_params);
    if (rendering_params) IDWriteRenderingParams_Release(rendering_params);
    if (gdi_interop)  IDWriteGdiInterop_Release(gdi_interop);
    if (render_target) IDWriteBitmapRenderTarget_Release(render_target);
    if (atlas_memory) free(atlas_memory);
    // NOTE: Do not release these if there is no error.
    if (font_face && cleanup_error) IDWriteFontFace_Release(font_face);
    if (glyph_infos && cleanup_error) free(glyph_infos);
    if (atlas && cleanup_error) ID3D11Texture2D_Release(atlas);
    if (atlas_srv && cleanup_error) ID3D11ShaderResourceView_Release(atlas);

    return font_atlas;
}

typedef struct dwrite_font_t
{
    IDWriteFactory* factory;
    IDWriteTextFormat* text_format;
    f32 advance_line;
} dwrite_font_t;

static dwrite_font_t dwrite_create_font(const char* font_name, f32 point_size)
{
    dwrite_font_t font = { 0 };
    HRESULT result = S_OK;
    WCHAR font_name_wchar[32] = { 0 };
    i32 font_name_length = (i32)strlen(font_name);

    IDWriteFactory* factory = 0;
    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&factory);
    assert(SUCCEEDED(result) && "[DWRITE] Failed to create factory.");
    
    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, font_name, font_name_length, NULL, 0);
    assert(wchar_size + 1 < array_count(font_name_wchar) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, font_name, font_name_length, font_name_wchar, wchar_size);
    font_name_wchar[wchar_size] = L'\0';

    IDWriteTextFormat* text_format = 0;
    f32 pixel_size = point_size * 96.0f / 72.0f;
    result = IDWriteFactory_CreateTextFormat(factory, font_name_wchar, 0,
                                             DWRITE_FONT_WEIGHT_REGULAR,
                                             DWRITE_FONT_STYLE_NORMAL,
                                             DWRITE_FONT_STRETCH_NORMAL,
                                             pixel_size, L"en-us", &text_format);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create text format.");

    result = IDWriteTextFormat_SetWordWrapping(text_format, DWRITE_WORD_WRAPPING_NO_WRAP);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to set word wrapping.");

    IDWriteFontCollection* font_collection = 0;
    result = IDWriteFactory_GetSystemFontCollection(factory, &font_collection, false);
    assert(SUCCEEDED(result) && font_collection && "[GFX2D] Failed to get system font collection.");
    
    u32 index = 0;
    bool exists = 0;
    result = IDWriteFontCollection_FindFamilyName(font_collection, font_name_wchar, &index, &exists);
    assert(SUCCEEDED(result) && exists && "[GFX2D] Failed to find font family name.");

    IDWriteFontFamily* font_family = 0;
    result = IDWriteFontCollection_GetFontFamily(font_collection, index, &font_family);
    assert(SUCCEEDED(result) && font_family && "[GFX2D] Failed to get font family.");

    IDWriteFont* matching_font = 0;
    result = IDWriteFontFamily_GetFirstMatchingFont(font_family, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &matching_font);
    assert(SUCCEEDED(result) && matching_font && "[GFX2D] Failed to get first matching font.");

    IDWriteFontFace* font_face = 0;
    result = IDWriteFont_CreateFontFace(matching_font, &font_face);
    assert(SUCCEEDED(result) && font_face && "[GFX2D] Failed to create font face.");

    DWRITE_FONT_METRICS font_metrics = { 0 };
    IDWriteFontFace_GetMetrics(font_face, &font_metrics);

    u16 glyp_index = 0;
    u32 codepoint = 'A';
    result = IDWriteFontFace_GetGlyphIndices(font_face, &codepoint, 1, &glyp_index);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph index.");
    
    DWRITE_GLYPH_METRICS glyph_metrics = { 0 };
    result = IDWriteFontFace_GetDesignGlyphMetrics(font_face, &glyp_index, 1, &glyph_metrics, false);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph metrics.");

    f32 units_per_em = (f32)font_metrics.designUnitsPerEm;
    f32 advance_height_pixel = glyph_metrics.advanceHeight * pixel_size / units_per_em;

    font.factory = factory;
    font.text_format = text_format;
    font.advance_line = advance_height_pixel;
    
    return font;
}


static void font_atlas_draw_text(font_atlas_t* font_atlas, const char* text, u32 text_length, f32 x, f32 y)
{
    const u32 max_text_length = 256;
    f32 layout_x = x;
    f32 layout_y = y + font_atlas->ascent + font_atlas->line_gap;
    
    assert(text_length < max_text_length);

    if (text && text_length > 0)
    {
        // TODO: We just assume maximum text length is 256, until we use arena here.
        const u32 vertex_per_glyph = 6;
        font_atlas_vertex_data_t vertex_data[256 * 6];
        u32 vertex_data_count = 0;
    
        for (u32 index = 0; index < text_length; ++index)
        {
            u32 codepoint = text[index];
            u16 glyph_index = codepoint < array_count(font_atlas->codepoint_to_glyph_index) ? font_atlas->codepoint_to_glyph_index[codepoint] : 0;
            glyph_info_t* glyph_info = font_atlas->glyph_infos + glyph_index;

            f32 x0 = layout_x + glyph_info->offset_x;
            f32 y0 = layout_y + glyph_info->offset_y;
            f32 x1 = x0 + glyph_info->width;
            f32 y1 = y0 + glyph_info->height;

            f32 uv_x = (f32)glyph_info->x / font_atlas->width;
            f32 uv_y = (f32)glyph_info->y / font_atlas->height;
            f32 uv_w = (f32)glyph_info->width / font_atlas->width;
            f32 uv_h = (f32)glyph_info->height / font_atlas->height;

            f32 u0 = uv_x;
            f32 v0 = uv_y;
            f32 u1 = u0 + uv_w;
            f32 v1 = v0 + uv_h;

            vertex_data[vertex_data_count + 0] = (font_atlas_vertex_data_t){ x0, y0, u0, v0 };
            vertex_data[vertex_data_count + 1] = (font_atlas_vertex_data_t){ x0, y1, u0, v1 };
            vertex_data[vertex_data_count + 2] = (font_atlas_vertex_data_t){ x1, y1, u1, v1 };
            vertex_data[vertex_data_count + 3] = (font_atlas_vertex_data_t){ x1, y1, u1, v1 };
            vertex_data[vertex_data_count + 4] = (font_atlas_vertex_data_t){ x1, y0, u1, v0 };
            vertex_data[vertex_data_count + 5] = (font_atlas_vertex_data_t){ x0, y0, u0, v0 };

            vertex_data_count += vertex_per_glyph;
            layout_x += glyph_info->advance;
        }

        // TODO: We create vertex buffer and param buffer for every
        // text, probably it is not good idea.  Ideally we should have
        // one vertex buffer pre allocated and just update it.  If
        // text is longer, we can reallocate vertex buffer with bigger
        // size or have multiple vertex buffers for different size of
        // texts. 256, 512 etc.  Can we just set a text length limit
        // and issue more than one draw calls if it is bigger than
        // that?
        D3D11_BUFFER_DESC vertex_buffer_desc =
        {
            .ByteWidth = (UINT)sizeof(font_atlas_vertex_data_t) * vertex_data_count,
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

        font_atlas_pass_t* font_atlas_pass = &font_atlas->pass;
    
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

        ID3D11DeviceContext_Draw(global_window.d3d11->context, vertex_data_count, 0);

        ID3D11Buffer_Release(vertex_buffer);
        ID3D11Buffer_Release(param_buffer);
    }
}

static void d2d_draw_text(dwrite_font_t* font, const char* text, u32 text_length, f32 x, f32 y)
{
    // TODO: Does not look good.
    static WCHAR wchar_text[256] = { 0 };

    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, text, (i32)text_length, NULL, 0);
    assert(wchar_size + 1 < array_count(wchar_text) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, text, (i32)text_length, wchar_text, wchar_size);
    wchar_text[wchar_size] = L'\0';
    
    D2D1_RECT_F layout =
    {
        .left = x,
        .top = y,
        .right = x + 10000,
        .bottom = y + 10000,
    };
    
    ID2D1RenderTarget_DrawText(global_window.d2d1->render_target, wchar_text, (UINT32)wchar_size, font->text_format,
                               &layout, (ID2D1Brush*)global_window.d2d1->solid_color_brush,
                               D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
                               DWRITE_MEASURING_MODE_NATURAL);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_DESTROY:
        case WM_CLOSE:
        {
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

        if (window->d2d1->render_target)
        {
            ID2D1SolidColorBrush_Release(window->d2d1->solid_color_brush);
            ID2D1RenderTarget_Release(window->d2d1->render_target);
            window->d2d1->solid_color_brush = 0;
            window->d2d1->render_target = 0;
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

            IDXGISurface* dxgi_surface = 0;
            result = ID3D11Texture2D_QueryInterface(window->d3d11->back_buffer, &IID_IDXGISurface, (void**)&dxgi_surface);
            assert(SUCCEEDED(result) && "[DXGI] Failed to get surface.");
            
            D2D1_RENDER_TARGET_PROPERTIES d2d_render_target_props = 
            {
                .type = D2D1_RENDER_TARGET_TYPE_DEFAULT,
                .pixelFormat =
                {
                    .format = DXGI_FORMAT_UNKNOWN,
                    .alphaMode = D2D1_ALPHA_MODE_IGNORE,
                },
                .dpiX = 0,
                .dpiY = 0,
                .usage = D2D1_RENDER_TARGET_USAGE_NONE,
                .minLevel = D2D1_FEATURE_LEVEL_DEFAULT,
            };
            result = ID2D1Factory_CreateDxgiSurfaceRenderTarget(window->d2d1->factory,
                                                                dxgi_surface,
                                                                &d2d_render_target_props,
                                                                &window->d2d1->render_target);
            assert(SUCCEEDED(result) && "[D2D1] Failed to create render target.");

            D2D1_COLOR_F d2d1_color = { 1.0f, 1.0f, 1.0f, 1.0f };
            result = ID2D1RenderTarget_CreateSolidColorBrush(window->d2d1->render_target, &d2d1_color, 0, &window->d2d1->solid_color_brush);
            assert(SUCCEEDED(result) && "[D2D1] Failed to create solid color brush.");
    
            // NOTE: This looks like it works but I am not sure we really do anti-aliasing?
            // ID2D1RenderTarget_SetAntialiasMode(window->d2d1->render_target, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            ID2D1RenderTarget_SetAntialiasMode(window->d2d1->render_target, D2D1_ANTIALIAS_MODE_ALIASED);
            
            D2D1_ANTIALIAS_MODE antialias_mode = ID2D1RenderTarget_GetAntialiasMode(window->d2d1->render_target);
            // fatal_system(antialias_mode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, "[D2D1] Failed to set anti-alias mode.");
            assert(antialias_mode == D2D1_ANTIALIAS_MODE_ALIASED && "[D2D1] Failed to set anti-alias mode.");
            
            ID2D1RenderTarget_SetTextAntialiasMode(window->d2d1->render_target, D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            D2D1_TEXT_ANTIALIAS_MODE text_antialias_mode = ID2D1RenderTarget_GetTextAntialiasMode(window->d2d1->render_target);
            assert(text_antialias_mode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE && "[D2D1] Failed to set text anti-alias mode.");

            IDXGISurface_Release(dxgi_surface);
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
    global_window.d2d1 = d2d1_init(96.0f);
    global_window.swap_chain = d3d11_create_swap_chain(global_window.hwnd, global_window.d3d11);
    
    /* font_atlas_t font_atlas = font_atlas_create("C:\\Windows\\Fonts\\Arial.ttf", 12); */
    /* font_atlas_t font_atlas = font_atlas_create("C:\\Windows\\Fonts\\Consola.ttf", 12); */
    font_atlas_t font_atlas = font_atlas_create("C:\\Users\\nukoseer\\AppData\\Local\\Microsoft\\Windows\\Fonts\\IosevkaTermNerdFontMono-Regular.ttf", 12);
    font_atlas_create_pass(&font_atlas.pass);

    dwrite_font_t dwrite_font = dwrite_create_font("IosevkaTerm NFM", 12);

    while (!global_window.quit)
    {
        resize_back_buffer(&global_window);
        /* FLOAT clear_rgba[4] = { 0.34f, 0.3f, 0.34f, 1.0f }; */
        /* FLOAT clear_rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; */
        vec4 background = v4v(srgb_to_linear(v3(0.0705f, 0.0705f, 0.0705f)), 1.0f);
        FLOAT clear_rgba[4]= { background.r, background.g, background.b, background.a };

        ID3D11DeviceContext_ClearRenderTargetView(global_window.d3d11->context, global_window.d3d11->rt_view, clear_rgba);

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

        f32 x = 0.0f;
        f32 y = 0.0f;
        const char* text = "Algeria";
        u32 text_length = (u32)strlen(text);
        font_atlas_draw_text(&font_atlas, text, text_length, x, y);
        /* font_atlas_draw_text(&font_atlas, text, text_length, x, y +  font_atlas.line_advance); */
        
        ID2D1RenderTarget_BeginDraw(global_window.d2d1->render_target);
        d2d_draw_text(&dwrite_font, text, text_length, x, y + font_atlas.line_advance);
        ID2D1RenderTarget_EndDraw(global_window.d2d1->render_target, 0, 0);
        
        IDXGISwapChain1_Present(global_window.swap_chain, 0, 0);
    }

    fprintf(stderr, "Done\n");
    
    return 0;
}
// int WinMainCRTStartup(void)
// {
//     return 0;
// }
