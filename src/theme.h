#pragma once

typedef struct theme_t
{
    vec4 bg_color;
    vec4 fg_color;
    vec4 highlight_color;
    vec4 dim_color;
    vec4 border_color;
    
    graphics_2d_font_t font_text;
    graphics_2d_font_t font_header;

    bool dark_mode;
} theme_t;

typedef struct themes_t
{
    theme_t themes[16];
    u32 current_theme_index;
} themes_t;

static inline void theme_add(themes_t* themes, theme_t* theme, u32 index);
static inline theme_set(themes_t* themes, u32 index);
static inline u32 theme_get_current_index(themes_t* themes);
static inline theme_t* theme_get_current(themes_t* themes);
