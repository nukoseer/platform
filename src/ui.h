#ifndef H_UI_H

#define ui_measure_text_width_function(name) f32 name(void* font, const char* text, usize text_length, void* parameter)
typedef ui_measure_text_width_function(ui_measure_text_width_f);

#define ui_get_line_height_function(name) f32 name(void* font, void* parameter)
typedef ui_get_line_height_function(ui_get_line_height_f);

#define UI_STACK_SIZE 32

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

typedef enum ui_axis_t
{
    UI_AXIS_X,
    UI_AXIS_Y,

    UI_AXIS_COUNT,
} ui_axis_t;

typedef struct ui_size_t
{
    ui_size_kind_t kind;
    f32 value;
    f32 strictness;
} ui_size_t;

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

typedef struct ui_rect_t
{
    f32 x, y;
    f32 width, height;
} ui_rect_t;

typedef enum ui_draw_kind_t
{
    UI_DRAW_RECT,
    UI_DRAW_BORDER,
    UI_DRAW_TEXT,
    UI_DRAW_CUSTOM,
} ui_draw_kind_t;

typedef struct ui_draw_command_t
{
    ui_draw_kind_t kind;
    f32 x, y;
    f32 width, height;
    vec4 color;
    f32 thickness;
    void* font;
    const char* text;
    u32 length;
} ui_draw_command_t;

typedef struct ui_draw_command_list_t
{
    ui_draw_command_t* commands;
    u32 command_count;
} ui_draw_command_list_t;

typedef struct ui_border_t
{
    bool enabled;
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

typedef struct ui_list_widget_t
{
    struct ui_widget_t* first;
    struct ui_widget_t* last;
} ui_widget_list_t;

typedef struct ui_text_line_t
{
    i32 offset;
    i32 length;
    f32 position[UI_AXIS_COUNT];
    f32 size[UI_AXIS_COUNT];
    struct ui_text_line_t* prev;
    struct ui_text_line_t* next;
} ui_text_line_t;

typedef enum ui_flags_t
{
    UI_FLAG_DRAW_BACKGROUND  = (1 << 0),
    UI_FLAG_DRAW_BORDER      = (1 << 1),
    UI_FLAG_DRAW_TEXT        = (1 << 2),
    UI_FLAG_DRAW_DROP_SHADOW = (1 << 3),
    UI_FLAG_DRAW_CUSTOM      = (1 << 4),
    UI_FLAG_CLICKABLE        = (1 << 5),
    UI_FLAG_SCROLLABLE_X     = (1 << 6),
    UI_FLAG_SCROLLABLE_Y     = (1 << 7),
    UI_FLAG_CLIP             = (1 << 8),
    UI_FLAG_HOT_ANIMATION    = (1 << 9),
    UI_FLAG_ACTIVE_ANIMATION = (1 << 10),
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
    void* font;
    vec4 font_color;
    ui_text_line_t* text_line;
    
    char label[32];
    u32 label_length;
    f32 label_size[UI_AXIS_COUNT];
    ui_alignment_t label_alignment;

    struct ui_widget_t* parent;
    struct ui_widget_t* hash_next;
    struct ui_widget_t* hash_prev;
    
    ui_widget_list_t child_list;
    struct ui_widget_t* child_next;
    struct ui_widget_t* child_prev;

    f32 fixed_size[UI_AXIS_COUNT];
    ui_rect_t rect;
    ui_flags_t flags;
} ui_widget_t;

typedef struct ui_stack_t
{
    u8* data;
    u32 stride;
    u32 count;
    u32 size;
    bool auto_pop;
} ui_stack_t;

typedef struct ui_stacks_t
{
    
    ui_stack_t parent;
    ui_stack_t size_x;
    ui_stack_t size_y;
    ui_stack_t layout_axis;
    ui_stack_t padding;
    ui_stack_t color;
    ui_stack_t border;
    ui_stack_t flags;
    
    ui_stack_t font;
    ui_stack_t font_color;
    ui_stack_t label_alignment;
} ui_stacks_t;

typedef struct ui_measure_text_width_t
{
    ui_measure_text_width_f* function;
    void* parameter;
} ui_measure_text_width_t;

typedef struct ui_get_line_height_t
{
    ui_get_line_height_f* function;
    void* parameter;
} ui_get_line_height_t;

typedef struct ui_callback_t
{
    ui_measure_text_width_t measure_text_width;
    ui_get_line_height_t get_line_height;
} ui_callback_t;

static inline ui_size_t ui_pixel(f32 pixel, f32 strictness);
static inline ui_size_t ui_percent(f32 parent_percent, f32 strictness);
static inline ui_size_t ui_content(f32 strictness);
static inline ui_size_t ui_children(f32 strictness);
static inline ui_axis_t ui_axis_x(void);
static inline ui_axis_t ui_axis_y(void);
static inline ui_axis_t ui_axis_flip(ui_axis_t axis);

static ui_widget_t* ui_widget_build_from_key(ui_key_t key);
static ui_widget_t* ui_widget_build_from_string(const char* widget_name);

#define H_UI_H
#endif
