#include "theme.h"

static inline void theme_add(themes_t* themes, theme_t* theme, u32 index)
{
    assert(index < array_count(themes->themes));

    themes->themes[index] = *theme;
}

static inline theme_set(themes_t* themes, u32 index)
{
    assert(index < array_count(themes->themes));

    themes->current_theme_index = index;
}

static inline u32 theme_get_current_index(themes_t* themes)
{
    u32 current_theme_index = themes->current_theme_index;

    return current_theme_index;
}

static inline theme_t* theme_get_current(themes_t* themes)
{
    theme_t* theme = themes->themes + themes->current_theme_index;

    return theme;
}
