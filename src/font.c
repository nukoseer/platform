
typedef struct codepoint_range_t
{
    u32 start;
    u32 end;
} codepoint_range_t;

typedef struct font_atlas_t
{
    graphics_texture_t atlas;
    i32 width, height;
} font_atlas_t;

typedef struct font_def_t
{
    u32 generation;
    
    IDWriteFontFace* face;
    font_info_t info;
    font_atlas_t atlas;
    glyph_info_t* glyph_infos;
    u32 glyph_info_count;
    u16 codepoint_to_glyph_index[512];

    u32 next_free_index;
} font_def_t;

static font_def_t global_fonts[16];
static u32 global_font_count = 1;

next_index_function(font);
free_index_function(font);

static const codepoint_range_t global_latin_codepoint_ranges[] =
{
    { 0x0020, 0x007E }, // Basic Latin (printable ASCII)
    { 0x00A0, 0x00FF }, // Latin-1 Supplement:  ö ü ç ó é ñ à ...
    { 0x0100, 0x017F }, // Latin Extended-A:    ł ą ę ś ż ź ć ń  (Polish)
                        //                      ğ ş İ ı          (Turkish)
};

typedef struct font_parameters_t
{
    IDWriteRenderingParams* rendering_params;
    IDWriteGdiInterop* gdi_interop;
} font_parameters_t;

static font_parameters_t global_font_parameters;

static void create_font_parameters(font_parameters_t* font_parameters)
{
    if (font_parameters && (!font_parameters->rendering_params && !font_parameters->gdi_interop))
    {
        HRESULT result = 0;
        IDWriteRenderingParams* default_rendering_params = 0;
        IDWriteRenderingParams* rendering_params = 0;
        IDWriteGdiInterop* gdi_interop = 0;
        IDWriteFactory* factory = global_d2d1.dwrite->factory;

        result = IDWriteFactory_CreateRenderingParams(factory, &default_rendering_params);
        assert(SUCCEEDED(result) && default_rendering_params && "[FONT] Failed to create rendering params.");

        result = IDWriteFactory2_CreateCustomRenderingParams2((IDWriteFactory2*)factory,
                                                              1.5f,
                                                              1.0f,
                                                              1.0f,
                                                              0.0f,
                                                              DWRITE_PIXEL_GEOMETRY_FLAT,
                                                              DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                                                              DWRITE_GRID_FIT_MODE_ENABLED,
                                                              (IDWriteRenderingParams2**)&rendering_params);
        assert(SUCCEEDED(result) && rendering_params && "[FONT] Failed to create custom rendering params.");

        IDWriteFactory_GetGdiInterop(factory, &gdi_interop);
        assert(SUCCEEDED(result) && gdi_interop && "[FONT] Failed to get gdi interop.");

        font_parameters->rendering_params = rendering_params;
        font_parameters->gdi_interop = gdi_interop;
    }
}

static void create_font_from_path(font_def_t* font, const char* font_path, f32 point_size)
{
    HRESULT result = 0;
    WCHAR font_path_wchar[128] = { 0 };
    i32 font_path_length = (i32)strlen(font_path);
    IDWriteFontFace* font_face = 0;
    IDWriteFactory* factory = global_d2d1.dwrite->factory;

    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, NULL, 0);
    assert(wchar_size + 1 < array_count(font_path_wchar) && "[FONT] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, font_path, font_path_length, font_path_wchar, wchar_size);
    font_path_wchar[wchar_size] = L'\0';

    IDWriteFontCollection* font_collection = 0;
    result = IDWriteFactory_GetSystemFontCollection(global_d2d1.dwrite->factory, &font_collection, false);
    
    u32 index = 0;
    bool exists = 0;
    result = IDWriteFontCollection_FindFamilyName(font_collection, font_path_wchar, &index, &exists);

    IDWriteFontFamily* font_family = 0;
    result = IDWriteFontCollection_GetFontFamily(font_collection, index, &font_family);

    if (font_family)
    {
        IDWriteFont* matching_font = 0;
        result = IDWriteFontFamily_GetFirstMatchingFont(font_family, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &matching_font);
        assert(SUCCEEDED(result) && matching_font && "[FONT] Failed to get first matching font.");
        
        result = IDWriteFont_CreateFontFace(matching_font, &font_face);
    }
    else
    {
        IDWriteFontFile* font_file = 0;
        result = IDWriteFactory_CreateFontFileReference(factory, font_path_wchar, 0, &font_file);
        assert(SUCCEEDED(result) && font_file && "[FONT] Failed to create font file.");

    
        result = IDWriteFactory_CreateFontFace(factory, DWRITE_FONT_FACE_TYPE_TRUETYPE, 1, &font_file, 0, DWRITE_FONT_SIMULATIONS_NONE, &font_face);
    }
    
    assert(SUCCEEDED(result) && font_face && "[FONT] Failed to create font face.");
    
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
    font->info.ascent = ascent;
    font->info.descent = descent;
    font->info.line_gap = line_gap;
    font->info.line_advance = line_advance;
    font->info.pixel_per_em = pixel_per_em;
    font->info.pixel_per_design_unit = pixel_per_design_unit;
}

static void create_font_atlas(font_def_t* font, i32 atlas_width, i32 atlas_height)
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

    assert(codepoint_count <= array_count(codepoints) && "[FONT] Codepoint buffer overflow.");

    u16 glyph_indices[512] = { 0 };
    result = IDWriteFontFace_GetGlyphIndices(font->face, codepoints, codepoint_count, glyph_indices);
    assert(SUCCEEDED(result) && "[FONT] Failed to get glyph indices.");

    DWRITE_GLYPH_METRICS glyph_metrics[512] = { 0 };
    result = IDWriteFontFace_GetGdiCompatibleGlyphMetrics(font->face, font->info.pixel_per_em, 1.0f, 0, true, glyph_indices, codepoint_count, glyph_metrics, false);
    assert(SUCCEEDED(result) && "[FONT] Failed to get glyph metrics.");

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
        font->codepoint_to_glyph_index[codepoints[i]] = glyph_indices[i];
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
    result = IDWriteGdiInterop_CreateBitmapRenderTarget(global_font_parameters.gdi_interop, 0, raster_width, raster_height, &render_target);
    assert(SUCCEEDED(result) && render_target && "[FONT] Failed to create bitmap render target.");

    HDC device_context = IDWriteBitmapRenderTarget_GetMemoryDC(render_target);
    assert(device_context && "[FONT] Failed to get device context.");

    for (u16 i = 0; i < baked_glyph_count; ++i)
    {
        u16 glyph_index = baked_glyph_indices[i];
        DWRITE_GLYPH_RUN glyph_run =
        {
            .fontFace = font->face,
            .fontEmSize = font->info.pixel_per_em,
            .glyphCount = 1,
            .glyphIndices = &glyph_index,
        };
        RECT bounding_box = { 0 };
        result = IDWriteBitmapRenderTarget_DrawGlyphRun(render_target, raster_x, raster_y,
                                                        DWRITE_MEASURING_MODE_NATURAL, &glyph_run, global_font_parameters.rendering_params,
                                                        foreground_color, &bounding_box);
        assert(SUCCEEDED(result) && "[FONT] Failed to draw glyph run.");
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
            .advance = ceilf(glyph_metrics[i].advanceWidth * font->info.pixel_per_design_unit),
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
    graphics_texture_t atlas = gfx_create_texture(&(graphics_texture_desc_t)
    {
        .format = FORMAT_R8G8B8A8_UNORM,
        .bind = BIND_SHADER_RESOURCE,
        .width = atlas_width,
        .height = atlas_height,
        .array_size = 1,
    }, &atlas_memory, &pitch);

    font->glyph_infos = glyph_infos;
    font->glyph_info_count = baked_glyph_count;
    font_atlas->atlas = atlas;
    font_atlas->width = atlas_width;
    font_atlas->height = atlas_height;
}

static font_handle_t font_create(const char* font_path, f32 point_size)
{
    u32 font_index = next_font_index();
    font_def_t* font_def = global_fonts + font_index;
    u32 font_generation = font_def->generation;

    memset(font_def, 0, sizeof(font_t));
    font_def->generation = font_generation;

    create_font_parameters(&global_font_parameters);
    create_font_from_path(font_def, font_path, point_size);
    create_font_atlas(font_def, 1024, 512);
    
    font_handle_t font = { 0 };
    font.platform = pack_generation_index(font_generation, font_index);
    font.point_size = point_size;
    font.pixel_size = font_def->info.pixel_per_em;

    return font;
}

static void font_delete(font_handle_t font)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return;
    }

    IDWriteFontFace_Release(font_def->face);
    *font_def = (font_def_t){ 0 };
    font_def->generation = font_generation + 1;

    free_font_index(font_index);
}

static glyph_info_t font_get_glyph_info_from_codepoint(font_handle_t font, u32 codepoint)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return (glyph_info_t){ 0 };
    }

    u16 glyph_index = codepoint < array_count(font_def->codepoint_to_glyph_index) ? font_def->codepoint_to_glyph_index[codepoint] : 0;
    glyph_info_t glyph_info = font_def->glyph_infos[glyph_index];

    return glyph_info;
}

static graphics_texture_t font_get_atlas(font_handle_t font)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return (graphics_texture_t){ 0 };
    }

    graphics_texture_t atlas = font_def->atlas.atlas;

    return atlas;
}

static font_info_t font_get_font_info(font_handle_t font)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return (font_info_t){ 0 };
    }

    font_info_t font_info = font_def->info;

    return font_info;
}

static f32 font_get_point_size(font_handle_t font)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return 0.0f;
    }

    f32 point_size = font.point_size;

    return point_size;
}

static f32 font_get_pixel_size(font_handle_t font)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return 0.0f;
    }

    f32 pixel_size = font.pixel_size;

    return pixel_size;
}

static f32 font_get_text_width(font_handle_t font, const char* text, size_t text_length)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return 0.0f;
    }

    f32 text_width = 0.0f;
    
    if (text && text_length < (u32)-1)
    {
        for (u32 index = 0; index < text_length; ++index)
        {
            u32 codepoint = text[index];
            u16 glyph_index = codepoint < array_count(font_def->codepoint_to_glyph_index) ? font_def->codepoint_to_glyph_index[codepoint] : 0;
            glyph_info_t* glyph_info = font_def->glyph_infos + glyph_index;

            text_width += glyph_info->advance;
        }
    }
    
    return text_width;
}

static f32 font_get_line_height(font_handle_t font)
{
    u32 font_generation = get_generation(font.platform);
    u32 font_index = get_index(font.platform);
    font_def_t* font_def = global_fonts + font_index;

    if (font_generation != font_def->generation)
    {
        assert(!"[FONT] Font generation does not match.");
        return 0.0f;
    }

    return font_def->info.line_advance;
}
