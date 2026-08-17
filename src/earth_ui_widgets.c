
static ui_widget_t* earth_widget_card_group_begin(const char* widget_name, const char* header_text, f32 position_x, f32 position_y, const theme_t* theme)
{
    ui_next_padding(16.0f, 16.0f);
    ui_next_border(1.0f, theme->border_color);
    ui_widget_t* widget_group = ui_widget_group_begin(widget_name, position_x, position_y);

    ui_next_color(theme->highlight_color);
    ui_next_size(ui_percent(0.4f, 1.0f), ui_pixel(4.0f, 1.0f));
    ui_next_flags(UI_FLAG_BACKGROUND);
    ui_widget_t* mark = ui_widget_build_from_format_string("%s-mark", widget_name);

    ui_widget_spacer(ui_pixel(8.0f, 1.0f));
                    
    ui_next_size(ui_content(1.0f), ui_content(1.0f));
    ui_next_flags(UI_FLAG_BACKGROUND | UI_FLAG_TEXT);
    ui_next_text_alignment(ui_align_center(), ui_align_center());
    ui_next_font(theme->font_header, theme->font_header.pixel_size);
    ui_widget_t* header_widget = ui_widget_build_from_format_string("%s-header", widget_name);
    ui_equip_text(header_widget, header_text, (i32)strlen(header_text));

    ui_widget_spacer(ui_pixel(8.0f, 1.0f));
    
    return widget_group;
}

static void earth_widget_card_group_end(void)
{
    ui_widget_group_end();
}

static ui_widget_t* earth_widget_text_row(const char* widget_name, const char* text)
{
    ui_next_size(ui_percent(1.0f, 1.0f), ui_content(1.0f));
    ui_next_padding(8.0f, 0.0f);
    ui_next_text_alignment(ui_align_leading(), ui_align_center());
    ui_widget_t* widget = ui_widget_text(widget_name, text);

    return widget;
}
