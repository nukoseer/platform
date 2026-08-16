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

#define ui_push_padding(x, y) ui_push(&global_ui->stacks.padding, ui_padding_t, ((ui_padding_t){ x, y }))
#define ui_next_padding(x, y) ui_next(&global_ui->stacks.padding, ui_padding_t, ((ui_padding_t){ x, y }))
#define ui_top_padding() ui_top(&global_ui->stacks.padding, ui_padding_t)
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

#define ui_push_text_alignment(x, y) ui_push(&global_ui->stacks.text_alignment, ui_alignment_t, ((ui_alignment_t){ x, y }))
#define ui_next_text_alignment(x, y) ui_next(&global_ui->stacks.text_alignment, ui_alignment_t, ((ui_alignment_t){ x, y }))
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
    bool changed;
    char* placeholder;
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
    ui_widget_t* widget = 0;

    ui_next_axis(ui_axis_y());
    widget = ui_widget_build_from_key(ui_key_zero());
    ui_push_parent(widget);

    return widget;
}

static void ui_widget_column_end(void)
{
    ui_widget_named_column_end();
}

// TODO: This function does not check if target is really inside in scrollable widget.
// If we would have a problem in future, we should implement a specific version of this.
// It directly can take widget parameter as a target instead of rect and check if it is 
// really descendant of scrollable widget.
static void ui_scroll_to_visible(ui_widget_t* scrollable_widget, ui_rect_t target, ui_axis_t axis)
{
    f32 target_start = target.xy[axis];
    f32 target_end = target_start + target.size[axis];

    f32 view_start = ui_widget_scroll(scrollable_widget, UI_AXIS_Y) + ui_widget_rect_position(scrollable_widget, UI_AXIS_Y);
    f32 view_end = view_start + ui_widget_rect_size(scrollable_widget, UI_AXIS_Y);

    if (target_end > view_end)
    {
        ui_widget_scroll_set(scrollable_widget, UI_AXIS_Y, ui_widget_scroll(scrollable_widget, UI_AXIS_Y) + (target_end - view_end));
    }
    else if (target_start < view_start)
    {
        ui_widget_scroll_set(scrollable_widget, UI_AXIS_Y, ui_widget_scroll(scrollable_widget, UI_AXIS_Y) - (view_start - target_start));
    }
}

static void ui_widget_scrollbar(ui_widget_t* parent_widget)
{
    ui_widget_t* widget = 0;

    if (parent_widget)
    {
        ui_push_parent(parent_widget);
        widget = parent_widget;
    }
    else
    {
        widget = ui_top_parent();
    }

    if (widget)
    {
        if (ui_is_flag_set(widget, UI_FLAG_SCROLLABLE_Y))
        {
            f32 viewport = ui_widget_rect_size(widget, UI_AXIS_Y);
            f32 content = widget->content_size[UI_AXIS_Y];

            if (content > viewport)
            {
                f32 max_scroll = content - viewport;
                f32 thumb_width = 6.0f;
                f32 thumb_height = max(viewport * (viewport / content), 20.0f);
                f32 track = viewport - thumb_height;
                f32 fraction = ui_widget_scroll(widget, UI_AXIS_Y) / max_scroll;
                f32 thumb_x = ui_widget_rect_position(widget, UI_AXIS_X) + ui_widget_rect_size(widget, UI_AXIS_X) - thumb_width;
                f32 thumb_y = ui_widget_rect_position(widget, UI_AXIS_Y) + fraction * track;

                ui_next_flags(UI_FLAG_BACKGROUND | UI_FLAG_FLOATING | UI_FLAG_CLICKABLE);
                ui_next_color(v4(0.5f, 0.5f, 0.5f, 0.5f));
                ui_next_size(ui_pixel(thumb_width, 1.0f), ui_pixel(thumb_height, 1.0f));
                ui_widget_t* thumb_widget = ui_widget_build_from_format_string("%s-scroll-y", widget->name);
                thumb_widget->position.x = thumb_x;
                thumb_widget->position.y = thumb_y;

                ui_signal_t thumb_signal = ui_signal_for(thumb_widget);

                if (thumb_signal.held)
                {
                    f32 delta = global_ui->input->mouse_delta.y;
                    f32 track = viewport - thumb_height;
                    f32 scroll_per_pixel = max_scroll / track;
                    ui_widget_scroll_set(widget, UI_AXIS_Y, ui_widget_scroll(widget, UI_AXIS_Y) + delta * scroll_per_pixel);
                    thumb_widget->color = v4(0.5f, 0.5f, 0.5f, 0.6f);
                }
            }
        }

        if (ui_is_flag_set(widget, UI_FLAG_SCROLLABLE_X))
        {
            f32 viewport = ui_widget_rect_size(widget, UI_AXIS_X);
            f32 content = widget->content_size[UI_AXIS_X];

            if (content > viewport)
            {
                f32 max_scroll = content - viewport;
                f32 thumb_width = max(viewport * (viewport / content), 20.0f);
                f32 thumb_height = 6.0f;
                f32 track = viewport - thumb_width;
                f32 fraction = ui_widget_scroll(widget, UI_AXIS_X) / max_scroll;
                f32 thumb_x = ui_widget_rect_position(widget, UI_AXIS_X) + fraction * track;
                f32 thumb_y = ui_widget_rect_position(widget, UI_AXIS_Y) + ui_widget_rect_size(widget, UI_AXIS_Y) - thumb_height;

                ui_next_flags(UI_FLAG_BACKGROUND | UI_FLAG_FLOATING | UI_FLAG_CLICKABLE);
                ui_next_color(v4(0.5f, 0.5f, 0.5f, 0.5f));
                ui_next_size(ui_pixel(thumb_width, 1.0f), ui_pixel(thumb_height, 1.0f));
                ui_widget_t* thumb_widget = ui_widget_build_from_format_string("%s-scroll-x", widget->name);
                thumb_widget->position.x = thumb_x;
                thumb_widget->position.y = thumb_y;

                ui_signal_t thumb_signal = ui_signal_for(thumb_widget);

                if (thumb_signal.held)
                {
                    f32 delta = global_ui->input->mouse_delta.x;
                    f32 track = viewport - thumb_width;
                    f32 scroll_per_pixel = max_scroll / track;
                    ui_widget_scroll_set(widget, UI_AXIS_X, ui_widget_scroll(widget, UI_AXIS_X) + delta * scroll_per_pixel);
                    thumb_widget->color = v4(0.5f, 0.5f, 0.5f, 0.6f);
                }
            }
        }
    }

    if (parent_widget)
    {
        ui_pop_parent();
    }
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
        text_edit->changed = true;
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
    if (!ui_text_edit_delete_selection(text_edit))
    {
        ui_text_edit_delete_range(text_edit, text_edit->cursor, text_edit->length);
    }
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
    text_edit->text[text_edit->cursor] = (unsigned char)key;
    text_edit->cursor++;
    text_edit->length++;
    text_edit->text[text_edit->length] = '\0';
    text_edit->changed = true;
    
    ui_text_edit_collapse(text_edit);
}

static inline void ui_text_edit_clear(ui_text_edit_t* text_edit)
{
    *text_edit = (ui_text_edit_t){ 0 };
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
        f32 widget_left = ui_widget_rect_position(widget, UI_AXIS_X);
        f32 widget_right = ui_widget_rect_position(widget, UI_AXIS_X) + ui_widget_rect_size(widget, UI_AXIS_X);

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
            f32 start_x = text_line->position[UI_AXIS_X] + start_measure_x;
            f32 size_x = end_measure_x - start_measure_x;

            ui_push_parent(widget);
            {
                ui_next_size(ui_pixel(size_x, 1.0f), ui_pixel(text_line->size[UI_AXIS_Y], 1.0f));
                ui_next_flags(UI_FLAG_FLOATING | UI_FLAG_BACKGROUND);
                ui_next_color(v4v(widget->font_color.rgb, 0.2f));
                ui_widget_t* selection_widget = ui_widget_build_from_key((ui_key_t){ 0 });
                selection_widget->position.x = start_x;
                selection_widget->position.y = text_line->position[UI_AXIS_Y];
            }
            ui_pop_parent();
        }
    }
}

static void ui_text_edit_update_scroll(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    if (text_edit->length <= 0)
    {
        return;
    }
    
    if (widget->text_wrap == UI_TEXT_WRAP_NONE)
    {
        f32 cursor_x = global_ui->graphics->measure_text_width(widget->font.font, text_edit->text, text_edit->cursor);
        f32 text_width = global_ui->graphics->measure_text_width(widget->font.font, text_edit->text, text_edit->length);
        f32 view_width = ui_widget_rect_size(widget, UI_AXIS_X);
        f32 cursor_width = 1.0f;
        f32 scroll_x = ui_widget_scroll(widget, UI_AXIS_X);

        if (cursor_x - scroll_x > view_width - cursor_width)
        {
            scroll_x = cursor_x - view_width + cursor_width;
        }
        
        if (cursor_x - scroll_x < 0.0f)
        {
            scroll_x = cursor_x;
        }

        if (scroll_x < 0.0f)
        {
            scroll_x = 0.0f;
        }

        // NOTE: widget->content_size is from last frame but our
        // scroll_x value calculated in this frame so when we call
        // ui_widget_scroll_set without setting content_size it will
        // clamp against the old lower value.  It makes cursor flicker
        // if it is at the end of line.
        widget->content_size[UI_AXIS_X] = text_width + cursor_width;
        ui_widget_scroll_set(widget, UI_AXIS_X, scroll_x);
    }
    else
    {
        f32 total_line_height = widget->text_line_count * widget->text_size[UI_AXIS_Y];
        f32 view_height = ui_widget_rect_size(widget, UI_AXIS_Y);
        f32 scroll_y = ui_widget_scroll(widget, UI_AXIS_Y);
        
        if (total_line_height <= view_height)
        {
            scroll_y = 0.0f;
        }
        else
        {
            ui_text_line_t* text_line = ui_text_line_from_cursor(widget, text_edit->cursor);
            i32 cursor_line_index = text_line->index;
            f32 cursor_top = cursor_line_index * widget->text_size[UI_AXIS_Y];
            f32 cursor_bottom = cursor_top + widget->text_size[UI_AXIS_Y];


            if (cursor_bottom - scroll_y > view_height)
            {
                scroll_y = cursor_bottom - view_height;
            }

            if (cursor_top - scroll_y < 0.0f)
            {
                scroll_y = cursor_top;
            }

            scroll_y = clamp(0.0f, scroll_y, total_line_height - view_height);
        }

        // NOTE: The same problem with scroll_x but I did not see this causing visual problems.
        // Nevertheless, it is better to set widget->content_size here again.
        widget->content_size[UI_AXIS_Y] = total_line_height * widget->text_size[UI_AXIS_Y];
        ui_widget_scroll_set(widget, UI_AXIS_Y, scroll_y);
    }
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
        ui_next_flags(UI_FLAG_BACKGROUND | UI_FLAG_FLOATING);
        ui_next_color(widget->font_color);
        ui_widget_t* cursor_widget = ui_widget_build_from_key(ui_key_zero());
        ui_text_line_t* text_line = ui_text_line_from_cursor(widget, text_edit->cursor);
        
        if (text_line)
        {
            cursor_x = text_line->position[UI_AXIS_X] + global_ui->graphics->measure_text_width(widget->font.font, text_edit->text + text_line->offset, text_edit->cursor - text_line->offset);
            cursor_y = text_line->position[UI_AXIS_Y];
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

static void ui_text_edit_default_input(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    input_t* input = global_ui->input;

    for (u32 i = 0; i < input->event_count; ++i)
    {
        input_event_t* event = input->events + i;

        if (event->consumed)
        {
            continue;
        }

        if (event->kind == INPUT_EVENT_TEXT && event->codepoint >= 32)
        {
            ui_text_edit_insert(text_edit, event->codepoint);
            input_consume_event(input, event);
        }
        else if (event->kind == INPUT_EVENT_KEY_PRESS)
        {
            key_t key = event->key;
            bool ctrl = event->modifiers & KEY_MODIFIER_CTRL;
            bool shift = event->modifiers & KEY_MODIFIER_SHIFT;
            bool alt = event->modifiers & KEY_MODIFIER_ALT;

            if (key == KEY_BACKSPACE || (key == KEY_H && ctrl))
            {
                ui_text_edit_backspace(text_edit);
                input_consume_event(input, event);
            }

            if (key == KEY_H && alt)
            {
                ui_text_edit_backspace_all(text_edit);
                input_consume_event(input, event);
            }

            if (key == KEY_UP)
            {
                ui_text_edit_move_up(widget, text_edit, shift);
                input_consume_event(input, event);
            }

            if (key == KEY_DOWN)
            {
                ui_text_edit_move_down(widget, text_edit, shift);
                input_consume_event(input, event);
            }
            
            if (key == KEY_LEFT)
            {
                ui_text_edit_move_left(text_edit, 1, shift);
                input_consume_event(input, event);
            }

            if (key == KEY_RIGHT)
            {
                ui_text_edit_move_right(text_edit, 1, shift);
                input_consume_event(input, event);
            }

            if (key == KEY_E && ctrl)
            {
                ui_text_edit_move_right(text_edit, text_edit->length - text_edit->cursor, shift);
                input_consume_event(input, event);
            }

            if (key == KEY_A && ctrl)
            {
                ui_text_edit_move_left(text_edit, text_edit->cursor, shift);
                input_consume_event(input, event);
            }

            if (key == KEY_D && ctrl)
            {
                ui_text_edit_delete(text_edit);
                input_consume_event(input, event);
            }

            if (key == KEY_D && alt)
            {
                ui_text_edit_delete_all(text_edit);
                input_consume_event(input, event);
            }
        }
    }
}

static ui_widget_t* ui_widget_text_edit_begin(const char* name, ui_text_edit_t* text_edit, char* placeholder)
{
    ui_next_flags(UI_FLAG_TEXT | UI_FLAG_FOCUSABLE);
    ui_widget_t* widget = ui_widget(name);

    text_edit->placeholder = placeholder;
    text_edit->changed = false;

    return widget;
}

static void ui_widget_text_edit_end(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    if (ui_is_focused(widget->key))
    {
        ui_text_edit_selection(widget, text_edit);
        ui_text_edit_update_scroll(widget, text_edit);
        ui_widget_text_edit_selection(widget, text_edit);
        ui_widget_text_edit_cursor(widget, text_edit);
    }

    bool show_placeholder = text_edit->placeholder && text_edit->length == 0 && !ui_is_focused(widget->key);
    
    if (show_placeholder)
    {
        ui_equip_text(widget, text_edit->placeholder, (i32)strlen(text_edit->placeholder));
        widget->font_color = v4v(widget->font_color.rgb, 0.3f);
    }
    else
    {
        ui_equip_text(widget, text_edit->text, text_edit->length);
    }
}

static ui_signal_t ui_widget_text_edit(const char* name, ui_text_edit_t* text_edit, char* placeholder)
{
    ui_widget_t* widget = ui_widget_text_edit_begin(name, text_edit, placeholder);
    ui_signal_t signal = ui_signal_for(widget);

    if (signal.clicked)
    {
        ui_set_focus(widget->key);
    }

    if (ui_is_focused(widget->key))
    {
        ui_text_edit_default_input(widget, text_edit);
    }

    ui_widget_text_edit_end(widget, text_edit);

    return signal;
}


#define H_UI_UTILS_H
#endif
