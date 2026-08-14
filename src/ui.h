#ifndef H_UI_H

typedef struct ui_key_t
{
    u64 value;
} ui_key_t;

typedef enum ui_size_kind_t
{
    UI_SIZE_PIXEL,
    UI_SIZE_PARENT,
    UI_SIZE_CHILDREN,
    UI_SIZE_CONTENT,
    
    UI_SIZE_COUNT,
} ui_size_kind_t;

typedef struct ui_size_t
{
    ui_size_kind_t kind;
    f32 value;
    f32 strictness;
} ui_size_t;

typedef enum ui_axis_t
{
    UI_AXIS_X,
    UI_AXIS_Y,

    UI_AXIS_COUNT,
} ui_axis_t;

typedef struct ui_position_t
{
    union
    {
        struct
        {
            f32 x;
            f32 y;
        };

        f32 xy[UI_AXIS_COUNT];
    };
} ui_position_t;

typedef struct ui_border_t
{
    f32 thickness;
    vec4 color;
} ui_border_t;

typedef enum ui_alignment_kind_t
{
    UI_ALIGNMENT_LEADING,
    UI_ALIGNMENT_TRAILING,
    UI_ALIGNMENT_CENTER,
} ui_alignment_kind_t;

typedef struct ui_alignment_t
{
    u32 value[UI_AXIS_COUNT];
} ui_alignment_t;

typedef struct ui_font_t
{
    graphics_2d_font_t font;
    f32 pixel_size;
} ui_font_t;

typedef struct ui_rect_t
{
    union
    {
        struct
        {
            f32 x, y;
        };

        f32 xy[UI_AXIS_COUNT];
    };

    union
    {
        struct
        {
            f32 width, height;
        };

        f32 size[UI_AXIS_COUNT];
    };
} ui_rect_t;

typedef enum ui_draw_kind_t
{
    UI_DRAW_RECT,
    UI_DRAW_BORDER,
    UI_DRAW_TEXT,
    UI_DRAW_CUSTOM,
} ui_draw_kind_t;

typedef struct ui_draw_command_iter_t
{
    struct ui_draw_command_chunk_t* chunk;
    u32 index;
} ui_draw_command_iter_t;

typedef struct ui_draw_command_t
{
    ui_draw_kind_t kind;
    f32 x, y;
    f32 width, height;
    vec4 color;
    ui_rect_t clip;
    f32 thickness;
    graphics_2d_font_t font;
    const char* text;
    u32 length;
} ui_draw_command_t;

typedef struct ui_draw_command_chunk_t
{
    ui_draw_command_t commands[256];
    u32 count;
    struct ui_draw_command_chunk_t* next;
} ui_draw_command_chunk_t;

typedef struct ui_widget_list_t
{
    struct ui_widget_t* first;
    struct ui_widget_t* last;
} ui_widget_list_t;

typedef struct ui_text_line_list_t
{
    struct ui_text_line_t* first;
    struct ui_text_line_t* last;
} ui_text_line_list_t;

typedef struct ui_text_line_t
{
    i32 index;
    i32 offset;
    i32 length;
    f32 position[UI_AXIS_COUNT];
    f32 size[UI_AXIS_COUNT];
    struct ui_text_line_t* prev;
    struct ui_text_line_t* next;
} ui_text_line_t;

typedef enum ui_anchor_kind_t
{
    UI_ANCHOR_TOP_LEFT,
    UI_ANCHOR_TOP_CENTER,
    UI_ANCHOR_TOP_RIGHT,
    UI_ANCHOR_CENTER_LEFT,
    UI_ANCHOR_CENTER,
    UI_ANCHOR_CENTER_RIGHT,
    UI_ANCHOR_BOTTOM_LEFT,
    UI_ANCHOR_BOTTOM_CENTER,
    UI_ANCHOR_BOTTOM_RIGHT,
} ui_anchor_kind_t;

typedef struct ui_anchor_t
{
    ui_anchor_kind_t parent;
    ui_anchor_kind_t self;
} ui_anchor_t;

typedef struct ui_anchor_offset_t
{
    f32 x, y;
} ui_anchor_offset_t;

typedef enum ui_text_wrap_t
{
    UI_TEXT_WRAP_NONE,
    UI_TEXT_WRAP_CHAR,
    UI_TEXT_WRAP_WORD,
} ui_text_wrap_t;

typedef enum ui_flags_t
{
    UI_FLAG_BACKGROUND       = (1 << 0),
    UI_FLAG_BORDER           = (1 << 1),
    UI_FLAG_TEXT             = (1 << 2),
    UI_FLAG_TEXT_EDIT        = (1 << 3),
    UI_FLAG_DROP_SHADOW      = (1 << 4),
    UI_FLAG_CUSTOM           = (1 << 5),
    UI_FLAG_FLOATING         = (1 << 6),
    UI_FLAG_ESCAPE_CLIP      = (1 << 7),
    UI_FLAG_ANCHORED         = (1 << 8),
    UI_FLAG_CLICKABLE        = (1 << 9),
    UI_FLAG_FOCUSABLE        = (1 << 10),
    UI_FLAG_SCROLLABLE_X     = (1 << 11),
    UI_FLAG_SCROLLABLE_Y     = (1 << 12),
    UI_FLAG_CLIP             = (1 << 13),
    UI_FLAG_HOT_ANIMATION    = (1 << 14),
    UI_FLAG_ACTIVE_ANIMATION = (1 << 15),

} ui_flags_t;

typedef struct ui_widget_t
{
    const char* name;
    ui_key_t key;
    ui_position_t position;
    ui_size_t size[UI_AXIS_COUNT];
    ui_axis_t layout_axis;
    f32 padding;
    vec4 color;
    ui_border_t border;
    ui_flags_t flags;
    
    ui_anchor_t anchor;
    f32 anchor_offset[UI_AXIS_COUNT];
    
    ui_font_t font;
    vec4 font_color;
    ui_text_line_list_t text_line_list;
    u32 text_line_count;
    
    char* text;
    i32 text_length;
    f32 text_size[UI_AXIS_COUNT];
    ui_alignment_t text_alignment;
    ui_text_wrap_t text_wrap;

    f32 scroll[UI_AXIS_COUNT];
    f32 scroll_target[UI_AXIS_COUNT];

    struct ui_widget_t* parent;
    struct ui_widget_t* hash_next;
    struct ui_widget_t* hash_prev;
    
    ui_widget_list_t child_list;
    struct ui_widget_t* child_next;
    struct ui_widget_t* child_prev;

    f32 content_size[UI_AXIS_COUNT];
    f32 fixed_size[UI_AXIS_COUNT];
    ui_rect_t rect;

    bool hot;
    bool active;
    bool pressed;
    bool clicked;
    bool released;

    f32 hot_t;
    f32 active_t;
} ui_widget_t;

typedef struct ui_signal_t
{
    ui_widget_t* widget;
    bool hovering;
    bool pressed;
    bool held;
    bool clicked;
    bool released;
    f32 hot_t;
    f32 active_t;
} ui_signal_t;

static inline ui_size_t ui_pixel(f32 pixel, f32 strictness);
static inline ui_size_t ui_percent(f32 parent_percent, f32 strictness);
static inline ui_size_t ui_content(f32 strictness);
static inline ui_size_t ui_children(f32 strictness);
static inline ui_axis_t ui_axis_x(void);
static inline ui_axis_t ui_axis_y(void);
static inline ui_axis_t ui_axis_flip(ui_axis_t axis);
static inline ui_alignment_t ui_align_center(void);
static inline ui_alignment_t ui_align_leading(void);
static inline ui_alignment_t ui_align_trailing(void);

static inline ui_key_t ui_key_zero(void);
static ui_widget_t* ui_widget_build_from_key(ui_key_t key);
static ui_widget_t* ui_widget_build_from_string(const char* widget_name);
static ui_widget_t* ui_widget_build_from_format_string(const char* widget_name_format, ...);
static ui_key_t ui_get_key_from_string(ui_key_t key, const char* string);
static ui_widget_t* ui_widget_from_key(ui_key_t key);
static ui_signal_t ui_signal_for(ui_widget_t* widget);

static inline f32 ui_widget_max_scroll(ui_widget_t* widget, ui_axis_t axis);
static inline f32 ui_widget_scroll(ui_widget_t* widget, ui_axis_t axis);
static inline void ui_widget_scroll_set(ui_widget_t* widget, ui_axis_t axis, f32 value);
static inline void ui_widget_scroll_to(ui_widget_t* widget, ui_axis_t axis, f32 value);

static inline void ui_set_focus(ui_key_t key);
static inline void ui_clear_focus(ui_key_t key);
static inline bool ui_is_focused(ui_key_t key);

static inline ui_is_flag_set(ui_widget_t* widget, ui_flags_t flag);

static ui_widget_t* ui_widget_group_begin(const char* widget_name, f32 x, f32 y);
static inline void ui_widget_group_end(void);
static ui_widget_t* ui_widget(const char* widget_name);
static ui_widget_t* ui_widget_text(const char* widget_name, const char* text);
static ui_widget_t* ui_widget_text_with_length(const char* widget_name, const char* text, i32 text_length);
static void ui_equip_text(ui_widget_t* widget, const char* text, i32 text_length);

static inline f32 ui_widget_rect_position(ui_widget_t* widget, ui_axis_t axis);
static inline f32 ui_widget_rect_size(ui_widget_t* widget, ui_axis_t axis);
static inline f32 ui_resolve_alignment(ui_widget_t* widget, f32 size, ui_axis_t axis);

static void ui_init(memory_arena_t* memory_arena);
static void ui_begin(graphics_t* graphics, input_t* input, f32 delta_time, f32 width, f32 height);
static void ui_end(void);

static ui_draw_command_iter_t ui_draw_command_iter(void);
static ui_draw_command_t* ui_draw_command_next(ui_draw_command_iter_t* iter);

#define H_UI_H
#endif
