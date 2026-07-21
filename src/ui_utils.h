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
    
    memcpy(stack->data + stack->stride * stack->count, &flags, stack->stride);
    stack->count++;
}

static void ui_stack_set_flags(ui_flags_t flags)
{
    ui_stack_t* stack = &global_ui->stacks.flags;

    ui_flags_t top_flags = *(ui_flags_t*)ui_stack_top(&global_ui->stacks.flags);
    
    if (stack->auto_pop)
    {
        memcpy(stack->data + stack->stride * (stack->count - 1), &flags, stack->stride);
    }
    else
    {
        assert(stack->count < stack->size && "[UI] Invalid stack count.");
        memcpy(stack->data + stack->stride * stack->count, &flags, stack->stride);
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

#define ui_push_flags(value) ui_stack_push_flags(ui_top_flags() | (value))
#define ui_next_flags(value) ui_stack_set_flags(ui_top_flags() | (value))
#define ui_push_clear_flags(value) ui_stack_push_flags(ui_top_flags() & ~(value))
#define ui_next_clear_flags(value) ui_stack_set_flags(ui_top_flags() & ~(value))
#define ui_top_flags() ui_top(&global_ui->stacks.flags, ui_flags_t)
#define ui_pop_flags() ui_pop(&global_ui->stacks.flags)

#define ui_push_anchor(...) ui_push(&global_ui->stacks.anchor, ui_anchor_t, (ui_anchor_t){ __VA_ARGS__ })
#define ui_next_anchor(...) ui_next(&global_ui->stacks.anchor, ui_anchor_t, (ui_anchor_t){ __VA_ARGS__ })
#define ui_top_anchor() ui_top(&global_ui->stacks.anchor, ui_anchor_t)
#define ui_pop_anchor() ui_pop(&global_ui->stacks.anchor);

#define ui_push_anchor_offset(...) ui_push(&global_ui->stacks.anchor_offset, ui_anchor_offset_t, (ui_anchor_offset_t){ __VA_ARGS__ })
#define ui_next_anchor_offset(...) ui_next(&global_ui->stacks.anchor_offset, ui_anchor_offset_t, (ui_anchor_offset_t){ __VA_ARGS__ })
#define ui_top_anchor_offset() ui_top(&global_ui->stacks.anchor_offset, ui_anchor_offset_t)
#define ui_pop_anchor_offset() ui_pop(&global_ui->stacks.anchor_offset);

#define ui_push_font(...) ui_push(&global_ui->stacks.font, ui_font_t, (ui_font_t){ __VA_ARGS__ })
#define ui_next_font(...) ui_next(&global_ui->stacks.font, ui_font_t, (ui_font_t){ __VA_ARGS__ })
#define ui_top_font() ui_top(&global_ui->stacks.font, ui_font_t)
#define ui_pop_font() ui_pop(&global_ui->stacks.font)

#define ui_push_font_color(value) ui_push(&global_ui->stacks.font_color, vec4, value)
#define ui_next_font_color(value) ui_next(&global_ui->stacks.font_color, vec4, value)
#define ui_top_font_color() ui_top(&global_ui->stacks.font_color, vec4)
#define ui_pop_font_color() ui_pop(&global_ui->stacks.font_color)

#define ui_push_text_alignment(...) ui_push(&global_ui->stacks.text_alignment, ui_alignment_t, __VA_ARGS__)
#define ui_next_text_alignment(...) ui_next(&global_ui->stacks.text_alignment, ui_alignment_t, __VA_ARGS__)
#define ui_top_text_alignment() ui_top(&global_ui->stacks.text_alignment, ui_alignment_t)
#define ui_pop_text_alignment() ui_pop(&global_ui->stacks.text_alignment)

#define ui_push_text_wrap(...) ui_push(&global_ui->stacks.text_wrap, ui_text_wrap_t, __VA_ARGS__)
#define ui_next_text_wrap(...) ui_next(&global_ui->stacks.text_wrap, ui_text_wrap_t, __VA_ARGS__)
#define ui_top_text_wrap() ui_top(&global_ui->stacks.text_wrap, ui_text_wrap_t)
#define ui_pop_text_wrap() ui_pop(&global_ui->stacks.text_wrap)

typedef struct ui_text_edit_t
{
    char text[256];
    i32 length;
    i32 cursor;
    i32 selection;
    bool selecting;
    f32 scroll_x;    
    f32 scroll_y;
} ui_text_edit_t;

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

#define ui_next_border(thickness, color) do { ui_next_flags(UI_FLAG_BORDER); ui_next_border_props((thickness), (color)); } while (0)
#define ui_next_show_border(value) (value) ? ui_next_flags(UI_FLAG_BORDER) : ui_next_clear_flags(UI_FLAG_BORDER)
#define ui_next_anchored(parent_anchor, self_anchor, x, y) do { ui_next_flags(UI_FLAG_ANCHORED | UI_FLAG_FLOATING | UI_FLAG_ESCAPE_CLIP); ui_next_anchor(parent_anchor, self_anchor); ui_next_anchor_offset(x, y); } while (0) 
#define ui_push_anchored(parent_anchor, self_anchor, x, y) do { ui_push_flags(UI_FLAG_ANCHORED | UI_FLAG_FLOATING | UI_FLAG_ESCAPE_CLIP); ui_push_anchor(parent_anchor, self_anchor); ui_push_anchor_offset(x, y); } while (0)
#define ui_pop_anchored() do { ui_pop_flags(); ui_pop_anchor(); ui_pop_anchor_offset(); } while (0)

static ui_signal_t ui_widget_last_signal(const char* widget_name)
{
    ui_widget_t* widget = ui_widget_from_key(ui_get_key_from_string(ui_top_parent()->key, widget_name));
    ui_signal_t signal = widget ? ui_signal_for(widget) : (ui_signal_t){ 0 };

    return signal;
}

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

static inline void ui_text_edit_collapse(ui_text_edit_t* text_edit)
{
    text_edit->selection = text_edit->cursor;
}

static void ui_text_edit_delete_range(ui_text_edit_t* text_edit, i32 start, i32 end)
{
    start = clamp_i32(0, start, text_edit->length);
    end = clamp_i32(0, end, text_edit->length);

    if (start < end)
    {
        memmove(text_edit->text + start, text_edit->text + end, text_edit->length - end);
        text_edit->length -= (end - start);
        text_edit->text[text_edit->length] = '\0';
        text_edit->cursor = clamp_i32(0, start, text_edit->length);
        text_edit->selection = clamp_i32(0, start, text_edit->length);
    }
}

static bool ui_text_edit_delete_selection(ui_text_edit_t* text_edit)
{
    bool result = false;
    
    if (text_edit->selection != text_edit->cursor)
    {
        result = true;
        ui_text_edit_delete_range(text_edit, min(text_edit->selection, text_edit->cursor), max(text_edit->selection, text_edit->cursor));
    }

    return result;
}

static void ui_text_edit_backspace(ui_text_edit_t* text_edit)
{
    if (!ui_text_edit_delete_selection(text_edit))
    {
        ui_text_edit_delete_range(text_edit, text_edit->cursor - 1, text_edit->cursor);
    }
}

static void ui_text_edit_backspace_all(ui_text_edit_t* text_edit)
{
    ui_text_edit_delete_range(text_edit, 0, text_edit->cursor);
}

static void ui_text_edit_delete(ui_text_edit_t* text_edit)
{
    if (!ui_text_edit_delete_selection(text_edit))
    {
        ui_text_edit_delete_range(text_edit, text_edit->cursor, text_edit->cursor + 1);
    }
}

static void ui_text_edit_delete_all(ui_text_edit_t* text_edit)
{
    ui_text_edit_delete_range(text_edit, text_edit->cursor, text_edit->length);
}

static inline void ui_text_edit_move_left(ui_text_edit_t* text_edit, i32 offset, bool selection)
{
    text_edit->cursor = max(0, text_edit->cursor - offset);

    if (!selection)
    {
        ui_text_edit_collapse(text_edit);
    }
}

static inline void ui_text_edit_move_right(ui_text_edit_t* text_edit, i32 offset, bool selection)
{
    text_edit->cursor = (i32)min(text_edit->length, text_edit->cursor + offset);

    if (!selection)
    {
        ui_text_edit_collapse(text_edit);   
    }
}

static void ui_text_edit_insert(ui_text_edit_t* text_edit, key_t key)
{
    if (text_edit->length >= array_count(text_edit->text) - 1)
    {
        return;
    }

    ui_text_edit_delete_selection(text_edit);

    memmove(text_edit->text + text_edit->cursor + 1, text_edit->text + text_edit->cursor, text_edit->length - text_edit->cursor);
    text_edit->text[text_edit->cursor] = (char)key;
    text_edit->cursor++;
    text_edit->length++;
    text_edit->text[text_edit->length] = '\0';
    
    ui_text_edit_collapse(text_edit);
}

static ui_text_line_t* ui_text_line_from_cursor(ui_widget_t* widget, i32 cursor)
{
    ui_text_line_t* result = 0;

    for (ui_text_line_t* text_line = widget->text_line_list.first; text_line; text_line = text_line->next)
    {
        result = text_line;
        i32 next_start = text_line->next ? text_line->next->offset : widget->text_length;

        if (cursor < next_start)
        {
            break;
        }
    }

    return result;
}

static void ui_text_edit_move_up(ui_widget_t* widget, ui_text_edit_t* text_edit, bool selection)
{
    ui_text_line_t* text_line = ui_text_line_from_cursor(widget, text_edit->cursor);

    if (!text_line || !text_line->prev)
    {
        return;
    }

    ui_text_line_t* target_line = text_line->prev;
    i32 cursor_offset = text_edit->cursor - text_line->offset;
    text_edit->cursor = clamp_i32(target_line->offset,
                                  target_line->offset + cursor_offset,
                                  target_line->offset + target_line->length);
    if (!selection)
    {
        ui_text_edit_collapse(text_edit);
    }
}

static void ui_text_edit_move_down(ui_widget_t* widget, ui_text_edit_t* text_edit, bool selection)
{
    ui_text_line_t* text_line = ui_text_line_from_cursor(widget, text_edit->cursor);

    if (!text_line || !text_line->next)
    {
        return;
    }

    ui_text_line_t* target_line = text_line->next;
    i32 cursor_offset = text_edit->cursor - text_line->offset;
    text_edit->cursor = clamp_i32(target_line->offset,
                                  target_line->offset + cursor_offset,
                                  target_line->offset + target_line->length);
    if (!selection)
    {
        ui_text_edit_collapse(text_edit);
    }
}

static i32 ui_text_index_from_screen_x(ui_widget_t* widget, ui_text_edit_t* text_edit, ui_text_line_t* text_line, f32 screen_x)
{
    f32 origin_x = text_line ? text_line->position[UI_AXIS_X] : ui_resolve_alignment(widget, 0.0f, UI_AXIS_X);
    i32 line_start = text_line ? text_line->offset : 0;
    i32 line_end = text_edit->length;
    f32 local_x = screen_x - origin_x + widget->scroll[UI_AXIS_X];
    i32 result = line_end;

    if (local_x <= 0.0f)
    {
        result = line_start;
    }
    else
    {
        f32 total_width = 0.0f;

        for (i32 i = line_start; i < line_end; ++i)
        {
            f32 char_width = global_ui->graphics->measure_text_width(widget->font.font, text_edit->text + i, 1);

            if (local_x < total_width + char_width * 0.5f)
            {
                result = i;
                break;
            }

            total_width += char_width;
        }
    }

    return result;
}

static ui_text_line_t* ui_text_line_from_screen_y(ui_widget_t* widget, f32 screen_y)
{
    ui_text_line_t* result = 0;

    for (ui_text_line_t* text_line = widget->text_line_list.first; text_line; text_line = text_line->next)
    {
        f32 local_y = screen_y - text_line->position[UI_AXIS_Y] + widget->scroll[UI_AXIS_Y];

        if (local_y < 0.0f)
        {
            result = widget->text_line_list.first;
        }
        else if (local_y > text_line->size[UI_AXIS_Y])
        {
            result = widget->text_line_list.last;
        }
        else
        {
            result = text_line;
            break;
        }
    }

    return result;
}

static void ui_text_edit_selection(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    ui_signal_t signal = ui_signal_for(widget);
    f32 mouse_x = global_ui->input->mouse_position.x;
    f32 mouse_y = global_ui->input->mouse_position.y;

    if (signal.pressed)
    {
        ui_text_line_t* text_line = ui_text_line_from_screen_y(widget, mouse_y);
        i32 text_index = ui_text_index_from_screen_x(widget, text_edit, text_line, mouse_x);

        text_edit->selecting = true;
        text_edit->cursor = text_index;
        text_edit->selection = text_index;
    }
    
    if (text_edit->selecting)
    {
        ui_text_line_t* text_line = ui_text_line_from_screen_y(widget, mouse_y);
        f32 widget_left = ui_content_position(widget, UI_AXIS_X);
        f32 widget_right = ui_content_position(widget, UI_AXIS_X) + ui_content_size(widget, UI_AXIS_X);

        if (mouse_x > widget_right && text_edit->cursor < text_edit->length)
        {
            text_edit->cursor = widget->text_wrap == UI_TEXT_WRAP_NONE ?
                text_edit->cursor + 1 : text_line-> offset + text_line->length;
        }
        else if (mouse_x < widget_left && text_edit->cursor > 0)
        {
            text_edit->cursor = widget->text_wrap == UI_TEXT_WRAP_NONE ?
                text_edit->cursor - 1 : text_line->offset;
        }
        else
        {
            text_edit->cursor = ui_text_index_from_screen_x(widget, text_edit, text_line, mouse_x);
        }
    }

    if (signal.released)
    {
        text_edit->selecting = false;
    }
}

static void ui_widget_text_edit_selection(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    if (text_edit->selection != text_edit->cursor)
    {
        i32 selection_start = min(text_edit->selection, text_edit->cursor);
        i32 selection_end = max(text_edit->selection, text_edit->cursor);

        for (ui_text_line_t* text_line = widget->text_line_list.first; text_line; text_line = text_line->next)
        {
            if (selection_start >= text_line->offset + text_line->length || selection_end <= text_line->offset)
            {
                continue;
            }

            f32 start_measure_x = global_ui->graphics->measure_text_width(widget->font.font, text_edit->text + text_line->offset, selection_start - text_line->offset);
            f32 end_measure_x = global_ui->graphics->measure_text_width(widget->font.font, text_edit->text + text_line->offset, selection_end - text_line->offset);
            f32 start_x = text_line->position[UI_AXIS_X] + start_measure_x - widget->scroll[UI_AXIS_X];
            f32 size_x = end_measure_x - start_measure_x;

            ui_push_parent(widget);
            {
                ui_next_size(ui_pixel(size_x, 1.0f), ui_pixel(text_line->size[UI_AXIS_Y], 1.0f));
                ui_next_flags(UI_FLAG_FLOATING | UI_FLAG_BACKGROUND);
                ui_next_color(v4v(widget->font_color.rgb, 0.2f));
                ui_widget_t* selection_widget = ui_widget_build_from_key((ui_key_t){ 0 });
                selection_widget->position.x = start_x;
                selection_widget->position.y = text_line->position[UI_AXIS_Y] - widget->scroll[UI_AXIS_Y];
            }
            ui_pop_parent();
        }
    }
}

static void ui_text_edit_update_scroll(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    if (widget->text_wrap == UI_TEXT_WRAP_NONE)
    {
        f32 cursor_x = global_ui->graphics->measure_text_width(widget->font.font, text_edit->text, text_edit->cursor);
        f32 view_width = ui_content_size(widget, UI_AXIS_X);
        f32 cursor_width = 1.0f;

        if (cursor_x - text_edit->scroll_x > view_width - cursor_width)
        {
            text_edit->scroll_x = cursor_x - view_width + cursor_width;
        }
        
        if (cursor_x - text_edit->scroll_x < 0.0f)
        {
            text_edit->scroll_x = cursor_x;
        }

        if (text_edit->scroll_x < 0.0f)
        {
            text_edit->scroll_x = 0.0f;
        }
    }
    else
    {
        f32 total_line_height = widget->text_line_count * widget->text_size[UI_AXIS_Y];
        f32 view_height = ui_content_size(widget, UI_AXIS_Y);

        if (total_line_height <= view_height)
        {
            text_edit->scroll_y = 0.0f;
        }
        else
        {
            ui_text_line_t* text_line = ui_text_line_from_cursor(widget, text_edit->cursor);
            i32 cursor_line_index = text_line->index;
            f32 cursor_top = cursor_line_index * widget->text_size[UI_AXIS_Y];
            f32 cursor_bottom = cursor_top + widget->text_size[UI_AXIS_Y];

            if (cursor_bottom - text_edit->scroll_y > view_height)
            {
                text_edit->scroll_y = cursor_bottom - view_height;
            }

            if (cursor_top - text_edit->scroll_y < 0.0f)
            {
                text_edit->scroll_y = cursor_top;
            }

            text_edit->scroll_y = clamp(0.0f, text_edit->scroll_y, total_line_height);
        }
    }

    widget->scroll[UI_AXIS_X] = text_edit->scroll_x;   
    widget->scroll[UI_AXIS_Y] = text_edit->scroll_y;   
}

static void ui_widget_text_edit_cursor(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    f32 cursor_x = 0.0f;
    f32 cursor_y = 0.0f;
    f32 cursor_width = 1.0f;
    f32 cursor_height = widget->font.pixel_size;

    ui_push_parent(widget);
    {
        ui_next_size(ui_pixel(cursor_width, 1.0f), ui_pixel(cursor_height, 1.0f));
        ui_next_flags(UI_FLAG_FLOATING | UI_FLAG_BACKGROUND);
        ui_next_color(widget->font_color);
        ui_widget_t* cursor_widget = ui_widget_build_from_key((ui_key_t){ 0 });
        ui_text_line_t* text_line = ui_text_line_from_cursor(widget, text_edit->cursor);
        
        if (text_line)
        {
            cursor_x = text_line->position[UI_AXIS_X] + global_ui->graphics->measure_text_width(widget->font.font, text_edit->text + text_line->offset, text_edit->cursor - text_line->offset) - widget->scroll[UI_AXIS_X];
            cursor_y = text_line->position[UI_AXIS_Y] - widget->scroll[UI_AXIS_Y];
        }
        else
        {
            cursor_x = ui_resolve_alignment(widget, 0.0f, UI_AXIS_X);
            cursor_y = ui_resolve_alignment(widget, cursor_height, UI_AXIS_Y);
        }

        // TODO: Setting position manually is exceptional but this
        // is floating widget anyway.  We should make it better.
        cursor_widget->position.x = cursor_x;
        cursor_widget->position.y = cursor_y;
    }
    ui_pop_parent();
}

static ui_signal_t ui_widget_text_edit(const char* name, ui_text_edit_t* text_edit)
{
    input_t* input = global_ui->input;
    ui_next_flags(UI_FLAG_TEXT);
    ui_widget_t* widget = ui_widget(name);
    ui_signal_t signal = ui_signal_for(widget);

    if (signal.pressed)
    {
        ui_set_focus(widget->key);
    }

    if (ui_is_focused(widget->key))
    {
        u32 codepoint_index = 0;
        u32 codepoint = 0;
        u32 key_index = 0;
        key_t key = KEY_NULL;
        
        while ((codepoint = input_consume_next_text_event(input, &codepoint_index)) != 0)
        {
            ui_text_edit_insert(text_edit, codepoint);
        }

        while ((key = input_consume_next_event(input, INPUT_EVENT_KEY_PRESS, &key_index)) != KEY_NULL)
        {
            if (key == KEY_BACKSPACE || (key == KEY_H && (input->modifiers & KEY_MODIFIER_CTRL)))
            {
                ui_text_edit_backspace(text_edit);
            }

            if (key == KEY_H && input->modifiers & KEY_MODIFIER_ALT)
            {
                ui_text_edit_backspace_all(text_edit);
            }

            if (key == KEY_UP)
            {
                ui_text_edit_move_up(widget, text_edit, input->modifiers & KEY_MODIFIER_SHIFT);
            }

            if (key == KEY_DOWN)
            {
                ui_text_edit_move_down(widget, text_edit, input->modifiers & KEY_MODIFIER_SHIFT);
            }
            
            if (key == KEY_LEFT)
            {
                ui_text_edit_move_left(text_edit, 1, input->modifiers & KEY_MODIFIER_SHIFT);
            }

            if (key == KEY_RIGHT)
            {
                ui_text_edit_move_right(text_edit, 1, input->modifiers & KEY_MODIFIER_SHIFT);
            }

            if (key == KEY_E && input->modifiers & KEY_MODIFIER_CTRL)
            {
                ui_text_edit_move_right(text_edit, text_edit->length - text_edit->cursor, input->modifiers & KEY_MODIFIER_SHIFT);
            }

            if (key == KEY_A && input->modifiers & KEY_MODIFIER_CTRL)
            {
                ui_text_edit_move_left(text_edit, text_edit->cursor, input->modifiers & KEY_MODIFIER_SHIFT);
            }

            if (key == KEY_D && input->modifiers & KEY_MODIFIER_CTRL)
            {
                ui_text_edit_delete(text_edit);
            }

            if (key == KEY_D && input->modifiers & KEY_MODIFIER_ALT)
            {
                ui_text_edit_delete_all(text_edit);
            }
        }

        ui_text_edit_selection(widget, text_edit);
        ui_text_edit_update_scroll(widget, text_edit);
        ui_widget_text_edit_selection(widget, text_edit);
        ui_widget_text_edit_cursor(widget, text_edit);
    }

    ui_equip_text(widget, text_edit->text, text_edit->length);

    return signal;
}

#define H_UI_UTILS_H
#endif
