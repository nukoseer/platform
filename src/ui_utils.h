#ifndef H_UI_UTILS_H

static inline void ui_push_parent(ui_widget_t* parent)
{
    assert(global_ui->stack.parent_count < array_count(global_ui->stack.parent));
    global_ui->stack.parent[global_ui->stack.parent_count++] = parent;
}

static inline void ui_pop_parent(void)
{
    assert(global_ui->stack.parent_count > 0);
    --global_ui->stack.parent_count;
}

static inline ui_widget_t* ui_top_parent(void)
{
    assert(global_ui->stack.parent_count > 0);
    return global_ui->stack.parent[global_ui->stack.parent_count - 1];
}

static inline void ui_push_size_x(ui_size_t size_x)
{
    assert(global_ui->stack.size_x_count < array_count(global_ui->stack.size_x));
    global_ui->stack.size_x[global_ui->stack.size_x_count++] = size_x;
}

static inline void ui_pop_size_x(void)
{
    assert(global_ui->stack.size_x_count > 0);
    --global_ui->stack.size_x_count;
}

static inline ui_size_t ui_top_size_x(void)
{
    assert(global_ui->stack.size_x_count > 0);
    return global_ui->stack.size_x[global_ui->stack.size_x_count - 1];
}

static inline void ui_push_size_y(ui_size_t size_y)
{
    assert(global_ui->stack.size_y_count < array_count(global_ui->stack.size_y));
    global_ui->stack.size_y[global_ui->stack.size_y_count++] = size_y;
}

static inline void ui_pop_size_y(void)
{
    assert(global_ui->stack.size_y_count > 0);
    --global_ui->stack.size_y_count;
}

static inline ui_size_t ui_top_size_y(void)
{
    assert(global_ui->stack.size_y_count > 0);
    return global_ui->stack.size_y[global_ui->stack.size_y_count - 1];
}

static inline void ui_push_size(ui_axis_t axis, ui_size_t size)
{
    switch (axis)
    {
        case UI_AXIS_X:
        {
            ui_push_size_x(size);
        } break;

        case UI_AXIS_Y:
        {
            ui_push_size_y(size);
        } break;

        default:
        {
            assert(!"[UI] Invalid axis.");
        }
    }
}

static inline void ui_pop_size(ui_axis_t axis)
{
    switch (axis)
    {
        case UI_AXIS_X:
        {
            ui_pop_size_x();
        } break;

        case UI_AXIS_Y:
        {
            ui_pop_size_y();
        } break;

        default:
        {
            assert(!"[UI] Invalid axis.");
        }
    }
}

static inline ui_size_t ui_top_size(ui_axis_t axis)
{
    ui_size_t size = { 0 };
    
    switch (axis)
    {
        case UI_AXIS_X:
        {
            size = ui_top_size_x();
        } break;

        case UI_AXIS_Y:
        {
            size = ui_top_size_y();
        } break;

        default:
        {
            assert(!"[UI] Invalid axis.");
        }
    }

    return size;
}

static inline void ui_push_layout_axis(ui_axis_t layout_axis)
{
    assert(global_ui->stack.layout_axis_count < array_count(global_ui->stack.layout_axis));
    global_ui->stack.layout_axis[global_ui->stack.layout_axis_count++] = layout_axis;
}

static inline void ui_pop_layout_axis(void)
{
    assert(global_ui->stack.layout_axis_count > 0);
    --global_ui->stack.layout_axis_count;
}

static inline ui_axis_t ui_top_layout_axis(void)
{
    assert(global_ui->stack.layout_axis_count > 0);
    return global_ui->stack.layout_axis[global_ui->stack.layout_axis_count - 1];
}

static inline void ui_push_padding(f32 padding)
{
    assert(global_ui->stack.padding_count < array_count(global_ui->stack.padding));
    global_ui->stack.padding[global_ui->stack.padding_count++] = padding;
}

static inline void ui_pop_padding(void)
{
    assert(global_ui->stack.padding_count > 0);
    --global_ui->stack.padding_count;
}

static inline f32 ui_top_padding(void)
{
    assert(global_ui->stack.padding_count > 0);
    return global_ui->stack.padding[global_ui->stack.padding_count - 1];
}

static inline void ui_push_color(vec4 color)
{
    assert(global_ui->stack.color_count < array_count(global_ui->stack.color));
    global_ui->stack.color[global_ui->stack.color_count++] = color;
}

static inline void ui_pop_color(void)
{
    assert(global_ui->stack.color_count > 0);
    --global_ui->stack.color_count;
}

static inline vec4 ui_top_color(void)
{
    assert(global_ui->stack.color_count > 0);
    return global_ui->stack.color[global_ui->stack.color_count - 1];
}

static inline void ui_push_border(ui_border_t border)
{
    assert(global_ui->stack.border_count < array_count(global_ui->stack.border));
    global_ui->stack.border[global_ui->stack.border_count++] = border;
}

static inline void ui_pop_border(void)
{
    assert(global_ui->stack.border_count > 0);
    --global_ui->stack.border_count;
}

static inline ui_border_t ui_top_border(void)
{
    assert(global_ui->stack.border_count > 0);
    return global_ui->stack.border[global_ui->stack.border_count - 1];
}

static inline void ui_push_font(void* font)
{
    assert(global_ui->stack.font_count < array_count(global_ui->stack.font));
    global_ui->stack.font[global_ui->stack.font_count++] = font;
}

static inline void ui_pop_font(void)
{
    assert(global_ui->stack.font_count > 0);
    --global_ui->stack.font_count;
}

static inline void* ui_top_font(void)
{
    assert(global_ui->stack.font_count > 0);
    return global_ui->stack.font[global_ui->stack.font_count - 1];
}

static inline void ui_push_font_color(vec4 font_color)
{
    assert(global_ui->stack.font_color_count < array_count(global_ui->stack.font_color));
    global_ui->stack.font_color[global_ui->stack.font_color_count++] = font_color;
}

static inline void ui_pop_font_color(void)
{
    assert(global_ui->stack.font_color_count > 0);
    --global_ui->stack.font_color_count;
}

static inline vec4 ui_top_font_color(void)
{
    assert(global_ui->stack.font_color_count > 0);
    return global_ui->stack.font_color[global_ui->stack.font_color_count - 1];
}

static inline void ui_push_label_alignment(ui_alignment_t label_alignment)
{
    assert(global_ui->stack.label_alignment_count < array_count(global_ui->stack.label_alignment));
    global_ui->stack.label_alignment[global_ui->stack.label_alignment_count++] = label_alignment;
}

static inline void ui_pop_label_alignment(void)
{
    assert(global_ui->stack.label_alignment_count > 0);
    --global_ui->stack.label_alignment_count;
}

static inline ui_alignment_t ui_top_label_alignment(void)
{
    assert(global_ui->stack.label_alignment_count > 0);
    return global_ui->stack.label_alignment[global_ui->stack.label_alignment_count - 1];
}

static inline void ui_push_flags(ui_flags_t flags)
{
    assert(global_ui->stack.flags_count < array_count(global_ui->stack.flags));
    global_ui->stack.flags[global_ui->stack.flags_count++] = flags;
}

static inline void ui_pop_flags(void)
{
    assert(global_ui->stack.flags_count > 0);
    --global_ui->stack.flags_count;
}

static inline ui_flags_t ui_top_flags(void)
{
    assert(global_ui->stack.flags_count > 0);
    return global_ui->stack.flags[global_ui->stack.flags_count - 1];
}

#define ui_widget_group(...) defer_loop(ui_widget_group_begin(__VA_ARGS__), ui_widget_group_end())
#define ui_size_x(s) defer_loop(ui_push_size_x(s), ui_pop_size_x())
#define ui_size_y(s) defer_loop(ui_push_size_y(s), ui_pop_size_y())
#define ui_size_axis(axis, size) defer_loop(ui_push_size(axis, size), ui_pop_size(axis)) 
#define ui_axis(a) defer_loop(ui_push_layout_axis(a), ui_pop_layout_axis())
#define ui_padding(p) defer_loop(ui_push_padding(p), ui_pop_padding())
#define ui_color(c) defer_loop(ui_push_color(c), ui_pop_color())
#define ui_border(e, t, c) defer_loop(ui_push_border((ui_border_t){ .enabled=e, .thickness=t, .color=c }), ui_pop_border())
#define ui_font(f) defer_loop(ui_push_font(f), ui_pop_font())
#define ui_font_color(fc) defer_loop(ui_push_font_color(fc), ui_pop_font_color())
#define ui_label_alignment(f) defer_loop(ui_push_label_alignment(f), ui_pop_label_alignment())
#define ui_flags(f) defer_loop(ui_push_flags(f), ui_pop_flags())

#define ui_size(x, y) ui_size_x(x) ui_size_y(y)
#define ui_widget_row() defer_loop(ui_widget_row_push(), ui_widget_row_pop())
#define ui_widget_column() defer_loop(ui_widget_column_push(), ui_widget_column_pop())
#define ui_widget_named_row(name) defer_loop(ui_widget_named_row_push(name), ui_widget_named_row_pop())
#define ui_widget_named_column(name) defer_loop(ui_widget_named_column_push(name), ui_widget_named_column_pop())
#define ui_widget_center() defer_loop(ui_widget_spacer(ui_percent(1.0f, 0.0f)), ui_widget_spacer(ui_percent(1.0f, 0.0f)))

static ui_widget_t* ui_widget_spacer(ui_size_t size)
{
    ui_widget_t* widget = 0;
    ui_widget_t* parent = ui_top_parent();

    ui_size_axis(parent->layout_axis, size)
    ui_size_axis(ui_axis_flip(parent->layout_axis), ui_percent(1.0f, 0.0f))
    widget = ui_widget_build_from_key((ui_key_t){ 0 });

    return widget;
}

static ui_widget_t* ui_widget_named_row_push(const char* widget_name)
{
    ui_widget_t* widget = 0;

    ui_axis(ui_axis_x())
    widget = ui_widget_build_from_string(widget_name);
    ui_push_parent(widget);

    return widget;
}

static void ui_widget_named_row_pop(void)
{
    ui_pop_parent();
}

static ui_widget_t* ui_widget_row_push(void)
{
    ui_widget_t* widget = ui_widget_named_row_push("");

    return widget;
}

static void ui_widget_row_pop(void)
{
    ui_widget_named_row_pop();
}

static ui_widget_t* ui_widget_named_column_push(const char* widget_name)
{
    ui_widget_t* widget = 0;

    ui_axis(ui_axis_y())
    widget = ui_widget_build_from_string(widget_name);
    ui_push_parent(widget);

    return widget;
}

static void ui_widget_named_column_pop(void)
{
    ui_pop_parent();
}

static ui_widget_t* ui_widget_column_push(void)
{
    ui_widget_t* widget = ui_widget_named_column_push("");

    return widget;
}

static void ui_widget_column_pop(void)
{
    ui_widget_named_column_pop();
}

#define H_UI_UTILS_H
#endif
