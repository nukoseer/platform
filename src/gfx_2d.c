
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

static u32 global_font_count = 1;

next_index_function(font);

free_index_function(font);

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

