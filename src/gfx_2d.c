
typedef struct gfx_2d_font_t
{
    u32 generation;
    IDWriteTextFormat* text_format;
    char font_name[32];
    float font_size;
    u32 next_free_index;
} gfx_2d_font_t;

typedef struct gfx_2d_font_color_t
{
    u32 generation;
    ID2D1SolidColorBrush* solid_color_brush;
    D2D1_COLOR_F color;
    u32 next_free_index;
} gfx_2d_font_color_t;

static gfx_2d_font_t global_fonts[16];
static gfx_2d_font_color_t global_font_colors[16];

static u32 global_font_count = 1;
static u32 global_font_color_count = 1;

next_index_function(font);
next_index_function(font_color);

free_index_function(font);
free_index_function(font_color);

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

    u32 font_index = next_font_index();
    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;
    u32 font_generation = gfx_2d_font->generation;

    *gfx_2d_font = (gfx_2d_font_t)
    {
        .generation = font_generation,
        .text_format = text_format,
        .font_size = font_size,
    };
    
    memcpy(gfx_2d_font->font_name, font_name, length);

    font.platform = pack_generation_index(font_generation, font_index);
    
    return font;
}

static graphics_2d_create_font_color_function(gfx_2d_create_font_color)
{
    graphics_2d_font_color_t font_color = { 0 };
    HRESULT result = S_OK;

    ID2D1SolidColorBrush* solid_color_brush = 0;
    D2D1_COLOR_F d2d1_color = { r, g, b, a };
    result = ID2D1RenderTarget_CreateSolidColorBrush(global_d2d1.render_target, &d2d1_color, 0, &solid_color_brush);
    assert(SUCCEEDED(result) && "[GFX2D] Failed to create solid color brush.");

    u32 font_color_index = next_font_color_index();
    gfx_2d_font_color_t* gfx_2d_font_color = global_font_colors + font_color_index;
    u32 font_color_generation = gfx_2d_font_color->generation;

    *gfx_2d_font_color = (gfx_2d_font_color_t)
    {
        .generation = font_color_generation,
        .solid_color_brush = solid_color_brush,
        .color = { r, g, b, a },
    };
    
    font_color.platform = pack_generation_index(font_color_generation, font_color_index);
    
    return font_color;
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

static graphics_2d_delete_font_color_function(gfx_2d_delete_font_color)
{
    u32 font_color_generation = get_generation(font_color.platform);
    u32 font_color_index = get_index(font_color.platform);
    gfx_2d_font_color_t* gfx_2d_font_color = global_font_colors + font_color_index;

    if (font_color_generation != gfx_2d_font_color->generation)
    {
        assert(!"[GFX2D] Font color generation does not match.");
        return;
    }

    ID2D1SolidColorBrush_Release(gfx_2d_font_color->solid_color_brush);

    *gfx_2d_font_color = (gfx_2d_font_color_t){ 0 };
    gfx_2d_font_color->generation = font_color_generation + 1;

    free_font_color_index(font_color_index);
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
    u32 font_color_index = (u32)font_color.platform;

    assert(font_index < array_count(global_fonts));
    assert(font_color_index < array_count(global_font_colors));

    gfx_2d_font_t* gfx_2d_font = global_fonts + font_index;
    gfx_2d_font_color_t* gfx_2d_font_color = global_font_colors + font_color_index;
    
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
    // D2D1_ROUNDED_RECT rounded_rect =
    // {
    //     .rect = layout,
    //     .radiusX = 2.0f, .radiusY = 2.0f,
    // };
    // ID2D1RenderTarget_DrawRoundedRectangle(window->d2d1->render_target, &rounded_rect, (ID2D1Brush*)window->d2d1->solid_color_brush, 1.0f, 0);
    ID2D1RenderTarget_DrawText(global_d2d1.render_target, wide_text, (UINT32)converted, gfx_2d_font->text_format,
                               &layout, (ID2D1Brush*)gfx_2d_font_color->solid_color_brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT, DWRITE_MEASURING_MODE_NATURAL);
}
