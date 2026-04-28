#ifndef H_UI_UTILS_H

static void ui_stack_init(memory_arena_t* arena, ui_stack_t* stack, u32 stride, u32 size)
{
    stack->data = ma_push_size(arena, stride * size);
    stack->stride = stride;
    stack->count = 0;
    stack->size = size;
    stack->auto_pop = false;
}

static void ui_stack_push(ui_stack_t* stack, void* value)
{
    assert(stack->count < stack->size && "[UI] Invalid stack count.");
    memcpy(stack->data + stack->stride * stack->count, value, stack->stride);
    stack->count++;
}

static void ui_stack_set(ui_stack_t* stack, void* value)
{
    if (stack->auto_pop)
    {
        memcpy(stack->data + stack->stride * (stack->count - 1), value, stack->stride);
    }
    else
    {
        assert(stack->count < stack->size && "[UI] Invalid stack count.");
        memcpy(stack->data + stack->stride * stack->count, value, stack->stride);
        stack->count++;
        stack->auto_pop = true;
    }
}

static void* ui_stack_top(ui_stack_t* stack)
{
    assert(stack->count > 0 && "[UI] Invalid stack count.");
    return stack->data + stack->stride * (stack->count - 1);
}

static void ui_stack_pop(ui_stack_t* stack)
{
    assert(stack->count > 0 && "[UI] Invalid stack count.");
    stack->count--;
}

static void ui_stack_auto_pop(ui_stack_t* stack)
{
    if (stack->auto_pop)
    {
        ui_stack_pop(stack);
        stack->auto_pop = false;
    }
}

static void ui_stack_push_flags(ui_flags_t flags)
{
    ui_stack_t* stack = &global_ui->stacks.flags;
    
    assert(stack->count < stack->size && "[UI] Invalid stack count.");
    
    ui_flags_t top_flags = *(ui_flags_t*)ui_stack_top(&global_ui->stacks.flags);
    ui_flags_t new_flags = top_flags | flags;
    
    memcpy(stack->data + stack->stride * stack->count, &new_flags, stack->stride);
    stack->count++;
}

static void ui_stack_set_flags(ui_flags_t flags)
{
    ui_stack_t* stack = &global_ui->stacks.flags;

    ui_flags_t top_flags = *(ui_flags_t*)ui_stack_top(&global_ui->stacks.flags);
    ui_flags_t new_flags = top_flags | flags;
    
    if (stack->auto_pop)
    {
        memcpy(stack->data + stack->stride * (stack->count - 1), &new_flags, stack->stride);
    }
    else
    {
        assert(stack->count < stack->size && "[UI] Invalid stack count.");
        memcpy(stack->data + stack->stride * stack->count, &new_flags, stack->stride);
        stack->count++;
        stack->auto_pop = true;
    }
}

#define ui_push(stack, type, value) do { type _t = (value); ui_stack_push((stack), &_t); } while (0)
#define ui_next(stack, type, value) do { type _t = (value); ui_stack_set((stack), &_t); } while (0)
#define ui_top(stack, type) (*(type*)ui_stack_top(stack))
#define ui_pop(stack) ui_stack_pop(stack)

#define ui_push_parent(value) ui_push(&global_ui->stacks.parent, ui_widget_t*, value)
#define ui_next_parent(value) ui_next(&global_ui->stacks.parent, ui_widget_t*, value)
#define ui_top_parent() ui_top(&global_ui->stacks.parent, ui_widget_t*)
#define ui_pop_parent() ui_pop(&global_ui->stacks.parent)

#define ui_push_size_x(value) ui_push(&global_ui->stacks.size_x, ui_size_t, value)
#define ui_next_size_x(value) ui_next(&global_ui->stacks.size_x, ui_size_t, value)
#define ui_top_size_x() ui_top(&global_ui->stacks.size_x, ui_size_t)
#define ui_pop_size_x() ui_pop(&global_ui->stacks.size_x)

#define ui_push_size_y(value) ui_push(&global_ui->stacks.size_y, ui_size_t, value)
#define ui_next_size_y(value) ui_next(&global_ui->stacks.size_y, ui_size_t, value)
#define ui_top_size_y() ui_top(&global_ui->stacks.size_y, ui_size_t)
#define ui_pop_size_y() ui_pop(&global_ui->stacks.size_y)

#define ui_push_size(value_x, value_y) ui_push_size_x(value_x); ui_push_size_y(value_y)
#define ui_next_size(value_x, value_y) ui_next_size_x(value_x); ui_next_size_y(value_y)
#define ui_pop_size() ui_pop_size_x(); ui_pop_size_y()

#define ui_push_axis(value) ui_push(&global_ui->stacks.layout_axis, ui_axis_t, value)
#define ui_next_axis(value) ui_next(&global_ui->stacks.layout_axis, ui_axis_t, value)
#define ui_top_axis() ui_top(&global_ui->stacks.layout_axis, ui_axis_t)
#define ui_pop_axis() ui_pop(&global_ui->stacks.layout_axis)

#define ui_push_padding(value) ui_push(&global_ui->stacks.padding, f32, value)
#define ui_next_padding(value) ui_next(&global_ui->stacks.padding, f32, value)
#define ui_top_padding() ui_top(&global_ui->stacks.padding, f32)
#define ui_pop_padding() ui_pop(&global_ui->stacks.padding)

#define ui_push_color(value) ui_push(&global_ui->stacks.color, vec4, value)
#define ui_next_color(value) ui_next(&global_ui->stacks.color, vec4, value)
#define ui_top_color() ui_top(&global_ui->stacks.color, vec4)
#define ui_pop_color() ui_pop(&global_ui->stacks.color)

#define ui_push_border_props(...) ui_push(&global_ui->stacks.border, ui_border_t, (ui_border_t){ __VA_ARGS__ })
#define ui_next_border_props(...) ui_next(&global_ui->stacks.border, ui_border_t, (ui_border_t){ __VA_ARGS__ })
#define ui_top_border_props() ui_top(&global_ui->stacks.border, ui_border_t)
#define ui_pop_border_props() ui_pop(&global_ui->stacks.border)

#define ui_next_border(thickness, color) \
    ui_next_flags(ui_top_flags() | UI_FLAG_DRAW_BORDER); \
    ui_next_border_props((thickness), (color))

#define ui_next_show_border(thickness, color) \
    ui_next_flags(ui_top_flags() | UI_FLAG_DRAW_BORDER)

#define ui_next_hide_border(thickness, color) \
    ui_next_flags(ui_top_flags() & ~UI_FLAG_DRAW_BORDER)

#define ui_push_font(value) ui_push(&global_ui->stacks.font, void*, value)
#define ui_next_font(value) ui_next(&global_ui->stacks.font, void*, value)
#define ui_top_font() ui_top(&global_ui->stacks.font, void*)
#define ui_pop_font() ui_pop(&global_ui->stacks.font)

#define ui_push_font_color(value) ui_push(&global_ui->stacks.font_color, vec4, value)
#define ui_next_font_color(value) ui_next(&global_ui->stacks.font_color, vec4, value)
#define ui_top_font_color() ui_top(&global_ui->stacks.font_color, vec4)
#define ui_pop_font_color() ui_pop(&global_ui->stacks.font_color)

#define ui_push_label_alignment(value) ui_push(&global_ui->stacks.label_alignment, ui_alignment_t, value)
#define ui_next_label_alignment(value) ui_next(&global_ui->stacks.label_alignment, ui_alignment_t, value)
#define ui_top_label_alignment() ui_top(&global_ui->stacks.label_alignment, ui_alignment_t)
#define ui_pop_label_alignment() ui_pop(&global_ui->stacks.label_alignment)

#define ui_push_flags(value) ui_stack_push_flags(value)
#define ui_next_flags(value) ui_stack_set_flags(value)
#define ui_top_flags() ui_top(&global_ui->stacks.flags, ui_flags_t)
#define ui_pop_flags() ui_pop(&global_ui->stacks.flags)

static void ui_push_size_axis(ui_axis_t axis, ui_size_t size)
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

static void ui_next_size_axis(ui_axis_t axis, ui_size_t size)
{
    switch (axis)
    {
        case UI_AXIS_X:
        {
            ui_next_size_x(size);
        } break;

        case UI_AXIS_Y:
        {
            ui_next_size_y(size);
        } break;

        default:
        {
            assert(!"[UI] Invalid axis.");
        }
    }
}

static void ui_pop_size_axis(ui_axis_t axis)
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

static ui_size_t ui_top_size_axis(ui_axis_t axis)
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

#define ui_widget_group(...) defer_loop(ui_widget_group_begin(__VA_ARGS__), ui_widget_group_end())
#define ui_widget_row() defer_loop(ui_widget_row_begin(), ui_widget_row_end())
#define ui_widget_column() defer_loop(ui_widget_column_begin(), ui_widget_column_end())
#define ui_widget_named_row(name) defer_loop(ui_widget_named_row_begin(name), ui_widget_named_row_end())
#define ui_widget_named_column(name) defer_loop(ui_widget_named_column_begin(name), ui_widget_named_column_end())
#define ui_widget_center() defer_loop(ui_widget_spacer(ui_percent(1.0f, 0.0f)), ui_widget_spacer(ui_percent(1.0f, 0.0f)))

static ui_widget_t* ui_widget_spacer(ui_size_t size)
{
    ui_widget_t* widget = 0;
    ui_widget_t* parent = ui_top_parent();

    ui_next_size_axis(parent->layout_axis, size);
    ui_next_size_axis(ui_axis_flip(parent->layout_axis), ui_percent(1.0f, 0.0f));
    widget = ui_widget_build_from_key((ui_key_t){ 0 });

    return widget;
}

static ui_widget_t* ui_widget_named_row_begin(const char* widget_name)
{
    ui_widget_t* widget = 0;

    ui_next_axis(ui_axis_x());
    widget = ui_widget_build_from_string(widget_name);
    ui_push_parent(widget);

    return widget;
}

static void ui_widget_named_row_end(void)
{
    ui_pop_parent();
}

static ui_widget_t* ui_widget_row_begin(void)
{
    ui_widget_t* widget = 0;

    ui_next_axis(ui_axis_x());
    widget = ui_widget_build_from_key((ui_key_t){ 0 });
    ui_push_parent(widget);

    return widget;
}

static void ui_widget_row_end(void)
{
    ui_pop_parent();
}

static ui_widget_t* ui_widget_named_column_begin(const char* widget_name)
{
    ui_widget_t* widget = 0;

    ui_next_axis(ui_axis_y());
    widget = ui_widget_build_from_string(widget_name);
    ui_push_parent(widget);

    return widget;
}

static void ui_widget_named_column_end(void)
{
    ui_pop_parent();
}

static ui_widget_t* ui_widget_column_begin(void)
{
    // TODO: Using an empty string will make a problem if there are multiple unnamed columns. We can either disallow unnamed columns or generate unique keys for unnamed columns.
    ui_widget_t* widget = ui_widget_named_column_begin("");

    return widget;
}

static void ui_widget_column_end(void)
{
    ui_widget_named_column_end();
}

#define H_UI_UTILS_H
#endif
