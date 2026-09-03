#include "../shader/font_test_vertex_shader.h"
#include "../shader/font_test_pixel_shader.h"

typedef struct glyph_info_t
{
    u16 x, y;
    u16 width, height;
    i16 offset_x, offset_y;
    f32 advance;
} glyph_info_t;

typedef struct font_atlas_t
{
    graphics_texture_t atlas;
    i32 width, height;
    glyph_info_t* glyph_infos;
    u32 glyph_info_count;
    u16 codepoint_to_glyph_index[0x17F + 1];
} font_atlas_t;

typedef struct font_t
{
    u32 generation;
    
    IDWriteFontFace* face;
    f32 ascent;
    f32 descent;
    f32 line_gap;
    f32 line_advance;
    f32 pixel_per_em;
    f32 pixel_per_design_unit;
    font_atlas_t atlas;

    u32 next_free_index;
} font_t;

typedef struct gfx_2d_font_t
{
    u32 generation;
    // NOTE: Right now for ascii only.
    f32 char_advance_list[128];
    IDWriteFontFace* font_face;
    IDWriteTextFormat* text_format;
    char font_name[32];
    f32 units_per_em;
    f32 point_size;
    f32 pixel_size;
    f32 line_height;
    u32 next_free_index;
} gfx_2d_font_t;

static gfx_2d_font_t global_fonts[16];
static font_t global_fontts[16];

static u32 global_font_count = 1;
static u32 global_fontt_count = 1;

next_index_function(font);
next_index_function(fontt);

free_index_function(font);
free_index_function(fontt);

static DWRITE_TEXT_ALIGNMENT map_text_alignment(graphics_2d_text_alignment_t text_alignment)
{
    DWRITE_TEXT_ALIGNMENT text_alignment_map = 0;

    switch (text_alignment)
    {
        case TEXT_ALIGNMENT_LEADING:
        {
            text_alignment_map = DWRITE_TEXT_ALIGNMENT_LEADING;
        } break;

        case TEXT_ALIGNMENT_TRAILING:
        {
            text_alignment_map = DWRITE_TEXT_ALIGNMENT_TRAILING;
        } break;

        case TEXT_ALIGNMENT_CENTER:
        {
            text_alignment_map = DWRITE_TEXT_ALIGNMENT_CENTER;
        } break;

        default:
        {
            assert(!"[GFX2D] Failed to map text alignment.");
        } break;
    }

    return text_alignment_map;
}

static inline f32 measure_char_advance_width(gfx_2d_font_t* font, u8 codepoint)
{
    u16 glyph_index = 0;
    HRESULT result = IDWriteFontFace_GetGlyphIndices(font->font_face, (u32*)&codepoint, 1, &glyph_index);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph index.");

    DWRITE_GLYPH_METRICS glyph_metrics = { 0 };
    result = IDWriteFontFace_GetDesignGlyphMetrics(font->font_face, &glyph_index, 1, &glyph_metrics, false);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph metrics.");

    f32 advance_width_pixel = glyph_metrics.advanceWidth * font->pixel_size / font->units_per_em;

    return advance_width_pixel;
}

static inline void generate_char_advance_list(gfx_2d_font_t* font)
{
    for (u8 codepoint = 32; codepoint < 127; ++codepoint)
    {
        font->char_advance_list[(u8)codepoint] = measure_char_advance_width(font, codepoint);
    }
}

static inline f32 measure_text_width_cached(gfx_2d_font_t* font, const char* text, u32 text_length)
{
    f32 text_width = 0.0f;

    for (u32 i = 0; i < text_length; ++i)
    {
        u8 codepoint = (u8)text[i];

        if (codepoint >= 32 && codepoint < 127)
        {
            text_width += font->char_advance_list[codepoint];
        }
    }

    return text_width;
}

static graphics_2d_create_font_function(gfx_2d_create_font)
{
    graphics_2d_font_t font = { 0 };
    HRESULT result = S_OK;
    WCHAR font_name_wchar[32] = { 0 };
    i32 font_name_length = (i32)strlen(font_name);
    
    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, font_name, font_name_length, NULL, 0);
    assert(wchar_size + 1 < array_count(font_name_wchar) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, font_name, font_name_length, font_name_wchar, wchar_size);
    font_name_wchar[wchar_size] = L'\0';

    IDWriteTextFormat* text_format = 0;
    f32 pixel_size = point_size * 96.0f / 72.0f;
    result = IDWriteFactory_CreateTextFormat(global_d2d1.dwrite->factory, font_name_wchar, 0,
                                             DWRITE_FONT_WEIGHT_REGULAR,
                                             DWRITE_FONT_STYLE_NORMAL,
                                             DWRITE_FONT_STRETCH_NORMAL,
                                             pixel_size, L"en-us", &text_format);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create text format.");

    result = IDWriteTextFormat_SetWordWrapping(text_format, DWRITE_WORD_WRAPPING_NO_WRAP);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to set word wrapping.");

    IDWriteFontCollection* font_collection = 0;
    result = IDWriteFactory_GetSystemFontCollection(global_d2d1.dwrite->factory, &font_collection, false);
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

    u32 font_index = next_font_index();
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;
    u32 font_generation = gfx_2d_font->generation;

    *gfx_2d_font = (gfx_2d_font_t)
    {
        .generation = font_generation,
        .font_face = font_face,
        .text_format = text_format,
        .units_per_em = units_per_em,
        .point_size = point_size,
        .pixel_size = pixel_size,
        .line_height = advance_height_pixel,
    };
    memcpy(gfx_2d_font->font_name, font_name, font_name_length);

    generate_char_advance_list(gfx_2d_font);

    font.platform = pack_generation_index(font_generation, font_index);
    font.point_size = point_size;
    font.pixel_size = pixel_size;

    return font;
}

typedef struct font_atlas_vertex_data_t
{
    vec2 position;
    vec2 uv;
} font_atlas_vertex_data_t;

typedef struct font_system_t
{
    IDWriteRenderingParams* rendering_params;
    IDWriteGdiInterop* gdi_interop;

    graphics_buffer_t parameter_buffer;
    graphics_buffer_t vertex_buffer;
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_program_t program;
    graphics_sampler_t sampler;
    graphics_pipeline_t pipeline;
} font_system_t;

static font_system_t global_font_system;

static void create_font_system_dwrite_parameters(font_system_t* font_system)
{
    HRESULT result = 0;
    IDWriteRenderingParams* default_rendering_params = 0;
    IDWriteRenderingParams* rendering_params = 0;
    IDWriteGdiInterop* gdi_interop = 0;
    IDWriteFactory* factory = global_d2d1.dwrite->factory;

    result = IDWriteFactory_CreateRenderingParams(factory, &default_rendering_params);
    assert(SUCCEEDED(result) && default_rendering_params && "[GFX2D] Failed to create rendering params.");

    result = IDWriteFactory2_CreateCustomRenderingParams2((IDWriteFactory2*)factory,
                                                          1.5f,
                                                          1.0f,
                                                          1.0f,
                                                          0.0f,
                                                          DWRITE_PIXEL_GEOMETRY_FLAT,
                                                          DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                                                          DWRITE_GRID_FIT_MODE_ENABLED,
                                                          (IDWriteRenderingParams2**)&rendering_params);
    assert(SUCCEEDED(result) && rendering_params && "[GFX2D] Failed to create custom rendering params.");

    IDWriteFactory_GetGdiInterop(factory, &gdi_interop);
    assert(SUCCEEDED(result) && gdi_interop && "[GFX2D] Failed to get gdi interop.");

    font_system->rendering_params = rendering_params;
    font_system->gdi_interop = gdi_interop;
}

static void create_font_system_graphics(font_system_t* font_system)
{
    graphics_buffer_t vertex_buffer = gfx_create_buffer(&(graphics_buffer_desc_t)
    {
        .size = sizeof(font_atlas_vertex_data_t) * 6 * 256,
        .usage = USAGE_DYNAMIC,
        .bind = BIND_VERTEX_BUFFER,
    });

    graphics_buffer_t parameter_buffer = gfx_create_buffer(&(graphics_buffer_desc_t)
    {
        .size = 16,
        .usage = USAGE_DYNAMIC,
        .bind = BIND_CONSTANT_BUFFER,
    });
    
    graphics_shader_t vertex_shader = gfx_create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = font_test_vshader,
        .bytecode_size = sizeof(font_test_vshader),
        .stage = STAGE_VERTEX_SHADER,
    });

    graphics_shader_t pixel_shader = gfx_create_shader(&(graphics_shader_desc_t)
    {
        .bytecode = font_test_pshader,
        .bytecode_size = sizeof(font_test_pshader),
        .stage = STAGE_PIXEL_SHADER,
    });

    graphics_program_t program = gfx_create_program(&(graphics_program_desc_t)
    {
        .vertex_shader = vertex_shader,
        .pixel_shader = pixel_shader,
        .attributes = (graphics_vertex_attribute_t[])
        {
            { "POSITION", FORMAT_R32G32_FLOAT, offsetof(font_atlas_vertex_data_t, position), 0, 0, 0, 0 },
            { "TEXCOORD", FORMAT_R32G32_FLOAT, offsetof(font_atlas_vertex_data_t, uv), 0, 0, 0, 0 }
        },
        .attribute_count = 2,
    });

    graphics_sampler_t sampler = gfx_create_sampler(&(graphics_sampler_desc_t)
    {
        .filter = FILTER_MIN_MAG_MIP_POINT,
        .address_u = TEXTURE_ADDRESS_WRAP,
        .address_v = TEXTURE_ADDRESS_WRAP,
        .address_w = TEXTURE_ADDRESS_WRAP,
    });

    graphics_pipeline_t pipeline = gfx_create_pipeline(&(graphics_pipeline_desc_t)
    {
        .wireframe = false,
        .cull = true,
        .depth_test = false,
        .depth_write = false,
        .blend = BLEND_ALPHA,
    });

    font_system->parameter_buffer = parameter_buffer;
    font_system->vertex_buffer = vertex_buffer;
    font_system->vertex_shader = vertex_shader;
    font_system->pixel_shader = pixel_shader;
    font_system->program = program;
    font_system->sampler = sampler;
    font_system->pipeline = pipeline;
}

static void font_system_init(void)
{
    create_font_system_dwrite_parameters(&global_font_system);
    create_font_system_graphics(&global_font_system);
}

static void create_font_from_path(font_t* font, const char* font_path, f32 point_size)
{
    HRESULT result = 0;
    WCHAR font_path_wchar[128] = { 0 };
    i32 font_path_length = (i32)strlen(font_path);
    IDWriteFontFile* font_file = 0;
    IDWriteFontFace* font_face = 0;
    IDWriteFactory* factory = global_d2d1.dwrite->factory;

    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, NULL, 0);
    assert(wchar_size + 1 < array_count(font_path_wchar) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, font_path_wchar, wchar_size);
    font_path_wchar[wchar_size] = L'\0';
    
    result = IDWriteFactory_CreateFontFileReference(factory, font_path_wchar, 0, &font_file);
    assert(SUCCEEDED(result) && font_file && "[GFX2D] Failed to create font file.");

    result = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);
    assert(SUCCEEDED(result) && font_face && "[GFX2D] Failed to create font face.");

    DWRITE_FONT_METRICS font_metrics = { 0 };
    IDWriteFontFace_GetMetrics(font_face, &font_metrics);

    f32 dpi = 96.0f;
    f32 pixel_per_em = point_size * 1.0f / 72.0f * dpi;
    f32 pixel_per_design_unit = pixel_per_em / font_metrics.designUnitsPerEm;
    f32 ascent = font_metrics.ascent * pixel_per_design_unit;
    f32 descent = font_metrics.descent * pixel_per_design_unit;
    f32 line_gap = font_metrics.lineGap * pixel_per_design_unit;
    f32 line_advance = ascent + descent + line_gap;

    font->face = font_face;
    font->ascent = ascent;
    font->descent = descent;
    font->line_gap = line_gap;
    font->line_advance = line_advance;
    font->pixel_per_em = pixel_per_em;
    font->pixel_per_design_unit = pixel_per_design_unit;
}

typedef struct codepoint_range_t
{
    u32 start;
    u32 end;
} codepoint_range_t;

static const codepoint_range_t global_latin_codepoint_ranges[] =
{
    { 0x0020, 0x007E }, // Basic Latin (printable ASCII)
    { 0x00A0, 0x00FF }, // Latin-1 Supplement:  ö ü ç ó é ñ à ...
    { 0x0100, 0x017F }, // Latin Extended-A:    ł ą ę ś ż ź ć ń  (Polish)
                        //                      ğ ş İ ı          (Turkish)
};

static void create_font_atlas(font_t* font, i32 atlas_width, i32 atlas_height)
{
    HRESULT result = 0;
    font_atlas_t* font_atlas = &font->atlas;
    
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

    assert(codepoint_count <= array_count(codepoints) && "[GFX2D] Codepoint buffer overflow.");

    u16 glyph_indices[512] = { 0 };
    result = IDWriteFontFace_GetGlyphIndices(font->face, codepoints, codepoint_count, glyph_indices);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to get glyph indices.");

    DWRITE_GLYPH_METRICS glyph_metrics[512] = { 0 };
    result = IDWriteFontFace_GetGdiCompatibleGlyphMetrics(font->face, font->pixel_per_em, 1.0f, 0, true, glyph_indices, codepoint_count, glyph_metrics, false);
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
        font_atlas->codepoint_to_glyph_index[codepoints[i]] = glyph_indices[i];
    }

    const i32 bytes_per_pixel = 4;
    i32 atlas_memory_size = atlas_width * atlas_height * bytes_per_pixel;
    i32 total_atlas_glyph_count = IDWriteFontFace_GetGlyphCount(font->face);
    i32 glyph_infos_size = total_atlas_glyph_count * sizeof(glyph_info_t);

    u8* atlas_memory = (u8*)malloc(atlas_memory_size);
    glyph_info_t* glyph_infos = (glyph_info_t*)malloc(glyph_infos_size);
    memset(atlas_memory, 0, atlas_memory_size);
    memset(glyph_infos, 0, glyph_infos_size);

    COLORREF background_color = RGB(0, 0, 0);
    COLORREF foreground_color = RGB(255, 255, 255);

    const i32 glyph_padding = 1;
    const i32 max_glyph_width = 32;
    const i32 max_glyph_height = 32;
    
    // NOTE: We just make sure we have enough space for per glyph rasterization.
    i32 raster_width = (i32)(8.0f * max_glyph_width);
    i32 raster_height = (i32)(8.0f * max_glyph_height);
    f32 raster_x = (f32)(raster_width / 2);
    f32 raster_y = (f32)(raster_height / 2);

    assert((f32)(i32)raster_x == raster_x);
    assert((f32)(i32)raster_y == raster_y);

    IDWriteBitmapRenderTarget* render_target = 0;
    result = IDWriteGdiInterop_CreateBitmapRenderTarget(global_font_system.gdi_interop, 0, raster_width, raster_height, &render_target);
    assert(SUCCEEDED(result) && render_target && "[GFX2D] Failed to create bitmap render target.");

    HDC device_context = IDWriteBitmapRenderTarget_GetMemoryDC(render_target);
    assert(device_context && "[GFX2D] Failed to get device context.");

    for (u16 i = 0; i < baked_glyph_count; ++i)
    {
        u16 glyph_index = baked_glyph_indices[i];
        DWRITE_GLYPH_RUN glyph_run =
        {
            .fontFace = font->face,
            .fontEmSize = font->pixel_per_em,
            .glyphCount = 1,
            .glyphIndices = &glyph_index,
        };
        RECT bounding_box = { 0 };
        result = IDWriteBitmapRenderTarget_DrawGlyphRun(render_target, raster_x, raster_y,
                                                        DWRITE_MEASURING_MODE_NATURAL, &glyph_run, global_font_system.rendering_params,
                                                        foreground_color, &bounding_box);
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
            .x = (u16)(glyph_x + glyph_padding),
            .y = (u16)(glyph_y + glyph_padding),
            .width = (u16)glyph_width,
            .height = (u16)glyph_height,
            .offset_x = (i16)(bounding_box.left - raster_x),
            .offset_y = (i16)(bounding_box.top - raster_y),
            .advance = ceilf(glyph_metrics[i].advanceWidth * font->pixel_per_design_unit),
        };

        HBITMAP bitmap = (HBITMAP)GetCurrentObject(device_context, OBJ_BITMAP);
        DIBSECTION dib = { 0 };
        GetObject(bitmap, sizeof(dib), &dib);
        assert(dib.dsBm.bmBitsPixel == 32);
        
        u8* atlas_glyph_line = atlas_memory + glyph_x * bytes_per_pixel + glyph_y * atlas_width * bytes_per_pixel;

        i32 in_pitch = dib.dsBm.bmWidthBytes;
        i32 out_pitch = atlas_width * bytes_per_pixel;
        u8* in_line = (u8*)dib.dsBm.bmBits + bounding_box.left * 4 + bounding_box.top * in_pitch;
        u8* out_line = atlas_glyph_line + glyph_padding * out_pitch + glyph_padding * bytes_per_pixel;

        for (i32 y = 0; y < glyph_height; y += 1)
        {
            u8* in_pixel  = in_line;
            u8* out_pixel = out_line;

            for (i32 x = 0; x < glyph_width; x += 1)
            {
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

    u32 pitch = atlas_width * bytes_per_pixel;
    graphics_texture_t atlas = gfx_create_texture_2d(&(graphics_texture_2d_desc_t)
    {
        .format = FORMAT_R8G8B8A8_UNORM,
        .bind = BIND_SHADER_RESOURCE,
        .width = atlas_width,
        .height = atlas_height,
        .array_size = 1,
    }, &atlas_memory, &pitch);

    font_atlas->atlas = atlas;
    font_atlas->width = atlas_width;
    font_atlas->height = atlas_height;
    font_atlas->glyph_infos = glyph_infos;
    font_atlas->glyph_info_count = baked_glyph_count;
}

static graphics_2d_create_fontt_function(gfx_2d_create_fontt)
{
    u32 font_index = next_font_index();
    font_t* font = global_fontts + font_index;
    u32 font_generation = font->generation;

    memset(font, 0, sizeof(font_t));
    font->generation = font_generation;
    
    create_font_from_path(font, font_path, point_size);
    create_font_atlas(font, 1024, 512);
    
    graphics_2d_font_t fontt = { 0 };
    fontt.platform = pack_generation_index(font_generation, font_index);
    fontt.point_size = point_size;
    fontt.pixel_size = point_size * font->pixel_per_em;

    return fontt;
}

static graphics_2d_delete_font_function(gfx_2d_delete_font)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;

    if (font_generation != gfx_2d_font->generation)
    {
        assert(!"[GFX2D] Font generation does not match.");
        return;
    }

    IDWriteTextFormat_Release(gfx_2d_font->text_format);
    *gfx_2d_font = (gfx_2d_font_t){ 0 };
    gfx_2d_font->generation = font_generation + 1;

    free_font_index(font_index);
}

static graphics_2d_get_font_point_size_function(gfx_2d_get_font_point_size)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;

    if (font_generation != gfx_2d_font->generation)
    {
        assert(!"[GFX2D] Font generation does not match.");
        return 0.0f;
    }

    f32 point_size = gfx_2d_font->point_size;

    return point_size;
}

static graphics_2d_get_font_pixel_size_function(gfx_2d_get_font_pixel_size)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;

    if (font_generation != gfx_2d_font->generation)
    {
        assert(!"[GFX2D] Font generation does not match.");
        return 0.0f;
    }

    f32 pixel_size = gfx_2d_font->pixel_size;

    return pixel_size;
}

static graphics_2d_measure_text_width_function(gfx_2d_measure_text_width)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;

    if (font_generation != gfx_2d_font->generation)
    {
        assert(!"[GFX2D] Font generation does not match.");
        return 0.0f;
    }

    f32 text_width = 0.0f;
    
    if (text && text_length < (u32)-1)
    {
        // IMPORTANT: This function measures character advance width per character and sums them up. This is not the same as measuring text width using text layout. For example, if we have a text "fi", the character advance width of 'f' and 'i' are different than the text width of "fi" because of kerning. So this function is not accurate for measuring text width. But it is fast and good enough for most cases.
        text_width = measure_text_width_cached(gfx_2d_font, text, (u32)text_length);

        // TODO: This is really inefficient. For each text we create a new text layout,
        // get text metrics and release the layout. We also convert from UTF-8 to
        // UTF-16. Probably this is still okay if we only have small amount of texts with
        // small text length. Ideally, we at least need some kind of LRU cache structure.
        // I believe if we generate font atlas at some point we can calculate text width
        // without creating text layout.
        if (text_width == 0.0f)
        {
            IDWriteTextLayout* text_layout = 0;
            const f32 max_width = 10000.0f;
            const f32 max_height = 10000.0f;

            wchar_t wchar_text[256];
            i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, text, (int)text_length, NULL, 0);
            assert(wchar_size + 1 < array_count(wchar_text) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

            MultiByteToWideChar(CP_UTF8, 0, text, (i32)text_length, wchar_text, wchar_size);
            wchar_text[wchar_size] = '\0';

            HRESULT result = IDWriteFactory_CreateTextLayout(global_d2d1.dwrite->factory, wchar_text, wchar_size,
                                                             gfx_2d_font->text_format,
                                                             max_width, max_height, &text_layout);
            assert(SUCCEEDED(result) && text_layout && "[GFX2D] Failed to create text layout.");

            DWRITE_TEXT_METRICS text_metrics = { 0 };
            IDWriteTextLayout_GetMetrics(text_layout, &text_metrics);

            f32 text_width_dip = text_metrics.widthIncludingTrailingWhitespace;
            text_width = text_width_dip * (global_d2d1.dpi / 96.0f);

            IDWriteTextLayout_Release(text_layout);
        }
    }
    
    return text_width;
}

static graphics_2d_get_line_height_function(gfx_2d_get_line_height)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;

    if (font_generation != gfx_2d_font->generation)
    {
        assert(!"[GFX2D] Font generation does not match.");
        return 0.0f;
    }

    return gfx_2d_font->line_height;
}

static graphics_2d_begin_draw_function(gfx_2d_begin_draw)
{
    ID2D1RenderTarget_BeginDraw(global_d2d1.render_target);
}

static graphics_2d_end_draw_function(gfx_2d_end_draw)
{
    ID2D1RenderTarget_EndDraw(global_d2d1.render_target, 0, 0);
}

static graphics_2d_draw_text_function(gfx_2d_draw_text)
{
    // TODO: Does not look good.
    static WCHAR wchar_text[256] = { 0 };

    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, text, (i32)text_length, NULL, 0);
    assert(wchar_size + 1 < array_count(wchar_text) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, text, (i32)text_length, wchar_text, wchar_size);
    wchar_text[wchar_size] = L'\0';
    
    u32 font_index = (u32)font.platform;
    assert(font_index < array_count(global_fonts));

    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;
    
    D2D1_RECT_F layout =
    {
        .left = x,
        .top = y,
        .right = x + width,
        .bottom = y + height,
    };
    
    DWRITE_TEXT_ALIGNMENT text_alignment = map_text_alignment(alignment);
    HRESULT result = IDWriteTextFormat_SetTextAlignment(gfx_2d_font->text_format, text_alignment);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to set text alignment.");

    D2D1_COLOR_F color = { r, g, b, a };
    ID2D1SolidColorBrush_SetColor(global_d2d1.solid_color_brush, &color);

    ID2D1RenderTarget_DrawText(global_d2d1.render_target, wchar_text, (UINT32)wchar_size, gfx_2d_font->text_format,
                               &layout, (ID2D1Brush*)global_d2d1.solid_color_brush,
                               D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT,
                               DWRITE_MEASURING_MODE_NATURAL);
}

static graphics_2d_draw_textt_function(gfx_2d_draw_textt)
{
    u32 font_index = (u32)font.platform;
    assert(font_index < array_count(global_fontts));

    font_t* fontt = global_fontts + font_index;
    
    const u32 max_text_length = 256;
    f32 layout_x = x;
    f32 layout_y = y + fontt->ascent + fontt->line_gap;
    
    assert(text_length < max_text_length);

    if (text && text_length > 0)
    {
        font_atlas_t* font_atlas = &fontt->atlas;
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

        graphics_target_t backbuffer = gfx_get_backbuffer_target();

        gfx_begin_pass(backbuffer, &(graphics_pass_desc_t){ 0 });
        {
            vec4 viewport_size = v4((f32)backbuffer.width, (f32)backbuffer.height, 0.0f, 0.0f);
            gfx_update_buffer(global_font_system.parameter_buffer, &viewport_size, 0, sizeof(viewport_size));
            gfx_update_buffer(global_font_system.vertex_buffer, vertex_data, 0, sizeof(font_atlas_vertex_data_t) * vertex_data_count);
            gfx_set_buffer(global_font_system.parameter_buffer, STAGE_VERTEX_SHADER, 0, 0, 0);
            gfx_set_vertex_buffer(global_font_system.vertex_buffer, 0, sizeof(font_atlas_vertex_data_t), 0);
            gfx_set_program(global_font_system.program);
            gfx_set_pipeline(global_font_system.pipeline);
            gfx_set_srvs(STAGE_PIXEL_SHADER, &font_atlas->atlas, 1, 0);
            gfx_draw(TOPOLOGY_TRIANGLE_LIST, vertex_data_count, 0);
            
        }
        gfx_end_pass();
    }
}

static graphics_2d_draw_rect_function(gfx_2d_draw_rect)
{
    D2D1_RECT_F rect =
    {
        .left = x,
        .top = y,
        .right = x + width,
        .bottom = y + height,
    };

    D2D1_COLOR_F color = { r, g, b, a };
    ID2D1SolidColorBrush_SetColor(global_d2d1.solid_color_brush, &color);

    if (fill)
    {
        ID2D1RenderTarget_FillRectangle(global_d2d1.render_target, &rect, (ID2D1Brush*)global_d2d1.solid_color_brush);
    }
    else
    {
        ID2D1RenderTarget_DrawRectangle(global_d2d1.render_target, &rect, (ID2D1Brush*)global_d2d1.solid_color_brush, thickness, 0);

        // D2D1_ROUNDED_RECT rounded_rect =
        // {
        //     .rect = rect,
        //     .radiusX = 16.0f, .radiusY = 16.0f,
        // };
        // ID2D1RenderTarget_DrawRoundedRectangle(global_d2d1.render_target, &rounded_rect, (ID2D1Brush*)global_d2d1.solid_color_brush, thickness, 0);
    }
}

static graphics_2d_push_axis_aligned_clip_function(gfx_2d_push_axis_aligned_clip)
{
    D2D1_RECT_F clip_rect =
    {
        .left = x,
        .top = y,
        .right = x + width,
        .bottom = y + height,
    };

    ID2D1RenderTarget_PushAxisAlignedClip(global_d2d1.render_target, &clip_rect, D2D1_ANTIALIAS_MODE_ALIASED);
}

static graphics_2d_pop_axis_aligned_clip_function(gfx_2d_pop_axis_aligned_clip)
{
    ID2D1RenderTarget_PopAxisAlignedClip(global_d2d1.render_target);
}

