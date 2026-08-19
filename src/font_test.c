#define _CRT_SECURE_NO_WARNINGS

#include "dwrite_c.h"

#include "utils.h"

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
    f32 pixel_per_design_unit = pixel_per_em * font_metrics.designUnitsPerEm;

    i32 raster_width = (i32)(8.0f * font_metrics.capHeight * pixel_per_design_unit);
    i32 raster_height = (i32)(8.0f * font_metrics.capHeight * pixel_per_design_unit);
    f32 raster_x = (f32)raster_width * 0.5f;
    f32 raster_y = (f32)raster_height * 0.5f;

    assert((f32)(i32)raster_x == raster_x);
    assert((f32)(i32)raster_y == raster_y);

    IDWriteBitmapRenderTarget* render_target = 0;
    result = IDWriteGdiInterop_CreateBitmapRenderTarget(gdi_interop, 0, raster_width, raster_height, &render_target);
    assert(SUCCEEDED(result) && render_target && "[GFX2D] Failed to create bitmap render target.");

    HDC device_context = IDWriteBitmapRenderTarget_GetMemoryDC(render_target);
    assert(device_context && "[GFX2D] Failed to get device context.");
    
    ////////////////////

    u16 glyp_index = 0;
    u32 codepoint = 'A';
    result = IDWriteFontFace_GetGlyphIndices(font_face, &codepoint, 1, &glyp_index);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph index.");
    
    DWRITE_GLYPH_METRICS glyph_metrics = { 0 };
    result = IDWriteFontFace_GetDesignGlyphMetrics(font_face, &glyp_index, 1, &glyph_metrics, false);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph metrics.");

    
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int show_cmd)
{
    (void)instance, (void)prev_instance; (void)cmdline; (void)show_cmd;
    return 0;
}
// int WinMainCRTStartup(void)
// {
//     return 0;
// }
