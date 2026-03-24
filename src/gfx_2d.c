
typedef struct gfx_2d_font_t
{
    u32 generation;
    IDWriteTextFormat* text_format;
    char font_name[32];
    f32 font_size;
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

static graphics_2d_create_font_function(gfx_2d_create_font)
{
    graphics_2d_font_t font = { 0 };
    HRESULT result = S_OK;
    WCHAR font_name_wide[32] = { 0 };
    size_t length = strlen(font_name);
    
    size_t required_wide_length = mbstowcs(0, font_name, length);
    assert(required_wide_length < array_count(font_name_wide));

    size_t converted = mbstowcs(font_name_wide, font_name, length);
    assert(converted == required_wide_length);

    IDWriteTextFormat* text_format = 0;
    f32 font_size = point_size * 96.0f / 72.0f;
    result = IDWriteFactory_CreateTextFormat(global_d2d1.dwrite->factory, font_name_wide, 0,
                                             DWRITE_FONT_WEIGHT_REGULAR,
                                             DWRITE_FONT_STYLE_NORMAL,
                                             DWRITE_FONT_STRETCH_NORMAL,
                                             font_size, L"en-us", &text_format);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create text format.");

    result = IDWriteTextFormat_SetWordWrapping(text_format, DWRITE_WORD_WRAPPING_NO_WRAP);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to set word wrapping.");

    IDWriteTextLayout* text_layout = 0;
    const f32 max_width = 10000.0f;
    const f32 max_height = 10000.0f;

    result = IDWriteFactory_CreateTextLayout(global_d2d1.dwrite->factory, L"A", 1,
                                             text_format,
                                             max_width, max_height, &text_layout);
    assert(SUCCEEDED(result) && text_layout && "[GFX2D] Failed to create text layout.");

    DWRITE_TEXT_METRICS text_metrics = { 0 };
    IDWriteTextLayout_GetMetrics(text_layout, &text_metrics);
    f32 line_height = text_metrics.height * (global_d2d1.dpi / 96.0f);
    IDWriteTextLayout_Release(text_layout);

    u32 font_index = next_font_index();
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;
    u32 font_generation = gfx_2d_font->generation;

    *gfx_2d_font = (gfx_2d_font_t)
    {
        .generation = font_generation,
        .text_format = text_format,
        .font_size = font_size,
        .line_height = line_height,
    };
    
    memcpy(gfx_2d_font->font_name, font_name, length);

    font.platform = pack_generation_index(font_generation, font_index);
    
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
    static WCHAR wide_text[256] = { 0 };

    size_t required_wide_length = mbstowcs(0, text, text_length);
    assert(required_wide_length < array_count(wide_text));

    size_t converted = mbstowcs(wide_text, text, text_length);
    assert(converted == required_wide_length);

    wide_text[converted] = '\0';
    
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

    ID2D1RenderTarget_DrawText(global_d2d1.render_target, wide_text, (UINT32)converted, gfx_2d_font->text_format,
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

    // TODO: This is really inefficient. For each text we create a new text layout,
    // get text metrics and release the layout. We also convert from UTF-8 to
    // UTF-16. Probably this is still okay if we only have small amount of texts with
    // small text length. Ideally, we at least need some kind of LRU cache structure.
    // I believe if we generate font atlas at some point we can calculate text width
    // without creating text layout.
    
    IDWriteTextLayout* text_layout = 0;
    const f32 max_width = 10000.0f;
    const f32 max_height = 10000.0f;

    assert(text && text_length < (u32)-1 && "[GFX2D] Invalid text or text length.");

    i32 wchar_size = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t wchar_text[64];

    assert(wchar_size < array_count(wchar_text) && "[GFX2D] Failed to convert from UTF-8 to UTF-16.");

    MultiByteToWideChar(CP_UTF8, 0, text, -1, wchar_text, wchar_size);
    u32 wchar_length = (u32)(wchar_size - 1);

    HRESULT result = IDWriteFactory_CreateTextLayout(global_d2d1.dwrite->factory, wchar_text, wchar_length,
                                                     gfx_2d_font->text_format,
                                                     max_width, max_height, &text_layout);
    assert(SUCCEEDED(result) && text_layout && "[GFX2D] Failed to create text layout.");

    DWRITE_TEXT_METRICS text_metrics = { 0 };
    IDWriteTextLayout_GetMetrics(text_layout, &text_metrics);

    f32 text_width_dip = text_metrics.widthIncludingTrailingWhitespace;
    f32 text_width = text_width_dip * (global_d2d1.dpi / 96.0f);

    IDWriteTextLayout_Release(text_layout);

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


