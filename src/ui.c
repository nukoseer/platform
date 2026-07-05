#include "ui.h"

#define UI_INITIAL_HASH 0xCBF29CE484222325ULL
#define UI_STACK_SIZE 32

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
    ui_stack_t anchor;
    ui_stack_t anchor_offset;
    
    ui_stack_t font;
    ui_stack_t font_color;
    ui_stack_t text_alignment;
} ui_stacks_t;

typedef struct ui_t
{
    // NOTE: These are from platform layer.
    graphics_t* graphics;
    input_t* input;
    
    memory_arena_t* arena;
    memory_arena_t* frame_arena;
    ui_widget_t* root_widget;
    ui_widget_t* free_widgets;
    ui_stacks_t stacks;
    ui_widget_list_t widget_lists[64];
    ui_draw_command_chunk_t* draw_commands_first;
    ui_draw_command_chunk_t* draw_commands_last;
    u32 draw_command_count;

    ui_key_t hot_key;
    ui_key_t active_key;
    ui_key_t clicked_key;
    ui_key_t released_key;
    ui_key_t focus_key;
} ui_t;

static ui_t* global_ui;

#include "ui_utils.h"

static inline u64 ui_hash(u64 seed, const char* data, u64 size)
{
    u64 hash = seed;
    const u64 prime = 0x00000100000001B3ULL;

    for (u64 i = 0; i < size; ++i)
    {
	hash = hash ^ (u64)data[i];
	hash = hash * prime;
    }

    return hash;
}

static inline ui_is_flag_set(ui_widget_t* widget, ui_flags_t flag)
{
    bool result = (widget->flags & flag) != 0;

    return result;
}

static inline ui_size_t ui_pixel(f32 pixel, f32 strictness)
{
    ui_size_t size =
    {
        .kind = UI_SIZE_PIXEL,
        .value = pixel,
        .strictness = strictness
    };

    return size;
}

static inline ui_size_t ui_em(f32 coefficient, f32 strictness)
{
    ui_font_t font = ui_top_font();
    
    ui_size_t size =
    {
        .kind = UI_SIZE_PIXEL,
        .value = font.pixel_size * coefficient,
        .strictness = strictness
    };

    return size;
}

static inline ui_size_t ui_percent(f32 parent_percent, f32 strictness)
{
    assert(parent_percent >= 0.0f && parent_percent <= 1.0f && "[UI] Invalid parent size percentage.");

    ui_size_t size =
    {
        .kind = UI_SIZE_PARENT,
        .value = parent_percent,
        .strictness = strictness
    };

    return size;
}

static inline ui_size_t ui_content(f32 strictness)
{
    ui_size_t size =
    {
        .kind = UI_SIZE_CONTENT,
        .value = 0.0f,
        .strictness = strictness,
    };

    return size;
}

static inline ui_size_t ui_children(f32 strictness)
{
    ui_size_t size =
    {
        .kind = UI_SIZE_CHILDREN,
        .value = 0.0f,
        .strictness = strictness,
    };

    return size;
}

static inline ui_axis_t ui_axis_x(void)
{
    ui_axis_t axis = UI_AXIS_X;

    return axis;
}

static inline ui_axis_t ui_axis_y(void)
{
    ui_axis_t axis = UI_AXIS_Y;

    return axis;
}

static inline ui_axis_t ui_axis_flip(ui_axis_t axis)
{
    ui_axis_t flipped_axis = 0;

    switch (axis)
    {
        case UI_AXIS_X:
        {
            flipped_axis = UI_AXIS_Y;
        } break;

        case UI_AXIS_Y:
        {
            flipped_axis = UI_AXIS_X;
        } break;

        default:
        {
            assert(!"[UI] Invalid axis.");
        } break;
    }

    return flipped_axis;
}

static inline ui_alignment_t ui_align_center(void)
{
    ui_alignment_t alignment =
    {
        .value = { UI_ALIGNMENT_CENTER, UI_ALIGNMENT_CENTER },
    };

    return alignment;
}

static inline ui_alignment_t ui_align_leading(void)
{
    ui_alignment_t alignment =
    {
        .value = { UI_ALIGNMENT_LEADING, UI_ALIGNMENT_LEADING },
    };

    return alignment;
}

static inline ui_alignment_t ui_align_trailing(void)
{
    ui_alignment_t widget_alignment =
    {
        .value = { UI_ALIGNMENT_TRAILING, UI_ALIGNMENT_TRAILING },
    };

    return widget_alignment;
}

static inline f32 ui_content_position(ui_widget_t* widget, ui_axis_t axis)
{
    f32 content_position = widget->rect.xy[axis] + widget->padding;

    return content_position;
}

static inline f32 ui_content_size(ui_widget_t* widget, ui_axis_t axis)
{
    f32 content_size = widget->rect.size[axis] - widget->padding * 2.0f;

    return content_size > 0.0f ? content_size : 0.0f;
}

static bool ui_keys_equal(ui_key_t key_a, ui_key_t key_b)
{
    bool result = key_a.value == key_b.value;

    return result;
}

static bool ui_rect_contains_point(ui_rect_t rect, vec2 point)
{
    bool result = (point.x >= rect.x &&
                   point.y >= rect.y &&
                   point.x <= rect.x + rect.width &&
                   point.y <= rect.y + rect.height);

    return result;
}

static bool ui_rect_contains(ui_rect_t outer_rect, ui_rect_t inner_rect)
{
    const f32 epsilon = 0.5f;
    bool result = (inner_rect.x >= outer_rect.x - epsilon &&
                   inner_rect.y >= outer_rect.y - epsilon &&
                   inner_rect.x + inner_rect.width <= outer_rect.x + outer_rect.width + epsilon &&
                   inner_rect.y + inner_rect.height <= outer_rect.y + outer_rect.height + epsilon);
    
    return result;
}

static ui_rect_t ui_rect_intersect(ui_rect_t outer_rect, ui_rect_t inner_rect)
{
    ui_rect_t result = { 0 };

    f32 left = outer_rect.x > inner_rect.x ? outer_rect.x : inner_rect.x;
    f32 top = outer_rect.y > inner_rect.y ? outer_rect.y : inner_rect.y;
    f32 right = (outer_rect.x + outer_rect.width < inner_rect.x + inner_rect.width ?
                 outer_rect.x + outer_rect.width : inner_rect.x + inner_rect.width);
    f32 bottom = (outer_rect.y + outer_rect.height < inner_rect.y + inner_rect.height ?
                  outer_rect.y + outer_rect.height : inner_rect.y + inner_rect.height);

    result.x = left;
    result.y = top;
    result.width = right - left;
    result.height = bottom - top;

    return result;
}

static ui_key_t ui_get_key(ui_key_t key, const char* data, u64 size)
{
    ui_key_t new_key = { 0 };
    u64 seed = UI_INITIAL_HASH;

    if (key.value != 0)
    {
        seed = key.value;
    }

    new_key.value = ui_hash(seed, data, size);

    return new_key;
}

static ui_key_t ui_get_key_from_string(ui_key_t key, const char* string)
{
    u64 length = strlen(string);
    ui_key_t new_key = ui_get_key(key, string, length);

    return new_key;
}

static void ui_widget_list_hash_insert_front(ui_widget_list_t* widget_list, ui_widget_t* widget)
{
    if (!widget_list->first && !widget_list->last)
    {
        widget_list->first = widget_list->last = widget;
    }
    else
    {
        widget->hash_prev = 0;
        widget->hash_next = widget_list->first;
        widget_list->first->hash_prev = widget;
        widget_list->first = widget;
    }
}

static void ui_widget_list_hash_insert_back(ui_widget_list_t* widget_list, ui_widget_t* widget)
{
    if (!widget_list->first && !widget_list->last)
    {
        widget_list->first = widget_list->last = widget;
    }
    else
    {
        widget->hash_next = 0;
        widget->hash_prev = widget_list->last;
        widget_list->last->hash_next = widget;
        widget_list->last = widget;
    }
}

static void ui_widget_list_hash_remove(ui_widget_list_t* widget_list, ui_widget_t* widget)
{
    if (widget_list->first == widget)
    {
        widget_list->first = widget_list->first->hash_next;
    }

    if (widget_list->last == widget)
    {
        widget_list->last = widget_list->last->hash_prev;
    }

    if (widget->hash_next)
    {
        widget->hash_next->hash_prev = widget->hash_prev;
    }

    if (widget->hash_prev)
    {
        widget->hash_prev->hash_next = widget->hash_next;
    }
    
    widget->hash_next = 0;
    widget->hash_prev = 0;
}

static void ui_widget_list_child_insert_front(ui_widget_list_t* widget_list, ui_widget_t* child_widget)
{
    if (!widget_list->first && !widget_list->last)
    {
        widget_list->first = widget_list->last = child_widget;
    }
    else
    {
        child_widget->child_prev = 0;
        child_widget->child_next = widget_list->first;
        widget_list->first->child_prev = child_widget;
        widget_list->first = child_widget;
    }
}

static void ui_widget_list_child_insert_back(ui_widget_list_t* widget_list, ui_widget_t* child_widget)
{
    if (!widget_list->first && !widget_list->last)
    {
        widget_list->first = widget_list->last = child_widget;
    }
    else
    {
        child_widget->child_next = 0;
        child_widget->child_prev = widget_list->last;
        widget_list->last->child_next = child_widget;
        widget_list->last = child_widget;
    }
}

static void ui_widget_list_child_remove(ui_widget_list_t* widget_list, ui_widget_t* child_widget)
{
    if (widget_list->first == child_widget)
    {
        widget_list->first = widget_list->first->child_next;
    }

    if (widget_list->last == child_widget)
    {
        widget_list->last = widget_list->last->child_prev;
    }

    if (child_widget->child_next)
    {
        child_widget->child_next->child_prev = child_widget->child_prev;
    }

    if (child_widget->child_prev)
    {
        child_widget->child_prev->child_next = child_widget->child_next;
    }
    
    child_widget->child_next = 0;
    child_widget->child_prev = 0;
}

static ui_widget_t* ui_widget_from_key(ui_key_t key)
{
    ui_widget_t* widget = 0;
    u64 list_index = key.value & (array_count(global_ui->widget_lists) - 1);
    ui_widget_list_t* widget_list = global_ui->widget_lists + list_index;

    if (key.value != 0)
    {
        for (ui_widget_t* widget_iter = widget_list->first; widget_iter; widget_iter = widget_iter->hash_next)
        {
            if (widget_iter->key.value == key.value)
            {
                widget = widget_iter;
                break;
            }
        }
    } 

    return widget;
}

static ui_widget_t* ui_widget_build_from_key(ui_key_t key)
{
    ui_widget_t* widget = ui_widget_from_key(key);
    bool is_transient = key.value == 0;
    
    if (!widget)
    {
        if (is_transient)
        {
            widget = ma_push_size(global_ui->frame_arena, sizeof(ui_widget_t));
        }
        else
        {
            if (!global_ui->free_widgets)
            {
                widget = ma_push_size(global_ui->arena, sizeof(ui_widget_t));
            }
            else
            {
                widget = global_ui->free_widgets;
                global_ui->free_widgets = global_ui->free_widgets->hash_next;
            }

            u64 list_index = key.value & (array_count(global_ui->widget_lists) - 1);
            ui_widget_list_t* widget_list = global_ui->widget_lists + list_index;
            ui_widget_list_hash_insert_back(widget_list, widget);
        }
        memset(widget, 0, sizeof(ui_widget_t));
    }

    assert(widget && "[UI] Invalid widget.");

    widget->parent = widget->child_list.first = widget->child_list.last = widget->child_next = widget->child_prev = 0;
    widget->key = key;
    widget->position = (ui_position_t){ 0 };
    widget->text = 0;
    widget->text_length = 0;
    widget->text_line = 0;
    widget->text_line_count = 0;
    widget->fixed_size[UI_AXIS_X] = 0.0f;
    widget->fixed_size[UI_AXIS_Y] = 0.0f;
    widget->rect = (ui_rect_t){ 0 };
    
    // NOTE: The root widget does not have parent.
    ui_widget_t* parent_widget = ui_top_parent();
    widget->parent = parent_widget;

    if (parent_widget)
    {
        ui_widget_list_child_insert_back(&parent_widget->child_list, widget);
    }

    widget->position.x = 0.0f;
    widget->position.y = 0.0f;
    widget->size[UI_AXIS_X] = ui_top_size_x();
    widget->size[UI_AXIS_Y] = ui_top_size_y();
    widget->layout_axis = ui_top_axis();
    widget->padding = ui_top_padding();
    widget->color = ui_top_color();
    widget->border = ui_top_border_props();
    widget->flags = ui_top_flags();
    widget->anchor = ui_top_anchor();
    widget->anchor_offset[UI_AXIS_X] = ui_top_anchor_offset().x;
    widget->anchor_offset[UI_AXIS_Y] = ui_top_anchor_offset().y;

    ui_stack_auto_pop(&global_ui->stacks.parent);
    ui_stack_auto_pop(&global_ui->stacks.size_x);
    ui_stack_auto_pop(&global_ui->stacks.size_y);
    ui_stack_auto_pop(&global_ui->stacks.layout_axis);
    ui_stack_auto_pop(&global_ui->stacks.padding);
    ui_stack_auto_pop(&global_ui->stacks.color);
    ui_stack_auto_pop(&global_ui->stacks.border);
    ui_stack_auto_pop(&global_ui->stacks.flags);
    ui_stack_auto_pop(&global_ui->stacks.anchor);
    ui_stack_auto_pop(&global_ui->stacks.anchor_offset);

    return widget;
}

// TODO: Find matching non null parent key.
static ui_widget_t* ui_widget_build_from_string(const char* widget_name)
{
    ui_widget_t* parent = ui_top_parent();
    ui_key_t parent_key = parent ? parent->key : (ui_key_t){ 0 };
    ui_key_t key = ui_get_key_from_string(parent_key, widget_name);
    ui_widget_t* widget = ui_widget_build_from_key(key);
    widget->name = widget_name;

    return widget;
}

static inline void ui_set_focus(ui_key_t key)
{
    global_ui->focus_key = key;
}

static inline void ui_clear_focus(ui_key_t key)
{
    global_ui->focus_key = (ui_key_t){ 0 };
}

static inline bool ui_is_focused(ui_key_t key)
{
    bool result = false;

    if (ui_keys_equal(global_ui->focus_key, key))
    {
        result = true;
    }

    return result;
}

static inline ui_signal_t ui_signal_for(ui_widget_t* widget)
{
    ui_signal_t signal =
    {
        .widget = widget,
        .hovering = widget->hot,
        .pressed = widget->active,
        .clicked = widget->clicked,
        .released = widget->released,
    };

    return signal;
}

static ui_widget_t* ui_widget_group_begin(const char* widget_name, f32 x, f32 y)
{
    ui_widget_t* widget = ui_widget_build_from_string(widget_name);

    // TODO: Right now, it is not supported to position widgets inside a widget group except it is directly under the root widget.
    if ((x != 0 || y != 0) && (widget->parent != global_ui->root_widget))
    {
        // TODO: We can also silently ignore position parameters but for now keep the assert.
        assert(!"[UI] Only widgets directly under the root widget can specify position.");
        widget->position.x = 0.0f;
        widget->position.y = 0.0f;
    }
    
    widget->position.x = x;
    widget->position.y = y;

    ui_push_parent(widget);

    return widget;
}

static inline void ui_widget_group_end(void)
{
    ui_pop_parent();
}

static ui_widget_t* ui_widget(const char* widget_name)
{
    ui_widget_t* widget = ui_widget_build_from_string(widget_name);

    return widget;
}

static void ui_equip_text(ui_widget_t* widget, const char* text, u32 text_length)
{
    widget->font = ui_top_font().font;
    widget->font_color = ui_top_font_color();
    widget->text_alignment = ui_top_text_alignment();

    graphics_2d_font_t font = *(graphics_2d_font_t*)widget->font;
        
    if (text && text_length)
    {
        widget->text = ma_push_size(global_ui->frame_arena, text_length + 1);

        memcpy(widget->text, text, text_length);
        widget->text[text_length] = '\0';
        widget->text_length = text_length;

        widget->text_size[UI_AXIS_X] = global_ui->graphics->measure_text_width(font, widget->text, widget->text_length);
    }

    widget->text_size[UI_AXIS_Y] = global_ui->graphics->get_line_height(font);

    ui_stack_auto_pop(&global_ui->stacks.font);
    ui_stack_auto_pop(&global_ui->stacks.font_color);
    ui_stack_auto_pop(&global_ui->stacks.text_alignment);
}

static void ui_equip_text_edit(ui_widget_t* widget, ui_text_edit_t* text_edit)
{
    if (text_edit)
    {
        ui_equip_text(widget, text_edit->text, text_edit->length);
        widget->text_edit = text_edit;
    }
}

static ui_widget_t* ui_widget_text(const char* widget_name, const char* text)
{
    ui_next_flags(UI_FLAG_TEXT);
    ui_widget_t* widget = ui_widget_build_from_string(widget_name);
    ui_equip_text(widget, text, (u32)strlen(text));

    return widget;
}

static ui_widget_t* ui_widget_text_edit(const char* widget_name, ui_text_edit_t* text_edit)
{
    ui_next_flags(UI_FLAG_TEXT_EDIT);
    ui_widget_t* widget = ui_widget_build_from_string(widget_name);
    ui_equip_text_edit(widget, text_edit);

    return widget;
}

static void ui_resolve_text_lines(ui_widget_t* root_widget, ui_axis_t axis)
{
    if (root_widget->text)
    {
        graphics_2d_font_t font = *(graphics_2d_font_t*)root_widget->font;
        f32 wrap_width = root_widget->fixed_size[UI_AXIS_X] - root_widget->padding * 2.0f;

        if (wrap_width < 0.0f)
        {
            wrap_width = 0.0f;
        }

        if (ui_is_flag_set(root_widget, UI_FLAG_WRAP_TEXT))
        {
            i32 start_offset = 0;
            i32 end_offset = root_widget->text_length;
            ui_text_line_t* tail_text_line = 0;
            
            while (start_offset < end_offset)
            {
                i32 fits_end = start_offset;
                i32 space = string_find_leading_char(root_widget->text, start_offset, end_offset - start_offset, ' ');
                i32 word_end = space == -1 ? end_offset : space + 1;

                while (word_end != -1 && word_end <= end_offset)
                {
                    f32 width = global_ui->graphics->measure_text_width(font, root_widget->text + start_offset, word_end - start_offset);

                    if (width > wrap_width)
                    {
                        break;
                    }

                    fits_end = word_end;

                    if (word_end == end_offset)
                    {
                        break;
                    }

                    space = string_find_leading_char(root_widget->text, word_end, end_offset - word_end, ' ');
                    word_end = space == -1 ? end_offset : space + 1;
                }

                if (fits_end == start_offset)
                {
                    for (i32 i = start_offset + 1; i <= end_offset; ++i)
                    {
                        if (global_ui->graphics->measure_text_width(font, root_widget->text + start_offset, i - start_offset) > wrap_width)
                        {
                            fits_end = max(i - 1, start_offset + 1);
                            break;
                        }

                        fits_end = i;
                    }
                    // break;
                }

                //NOTE: Remove leading spaces from the line.
                while (start_offset < fits_end && root_widget->text[start_offset] == ' ')
                {
                    ++start_offset;
                }
                
                // NOTE: Remove trailing spaces from the line.
                i32 fits_end_wo_trailing_spaces = fits_end;
                while (fits_end_wo_trailing_spaces > start_offset && root_widget->text[fits_end_wo_trailing_spaces - 1] == ' ')
                {
                    --fits_end_wo_trailing_spaces;
                }

                ui_text_line_t* text_line = ma_push_struct_zero(global_ui->frame_arena, ui_text_line_t);
                text_line->offset = start_offset;
                text_line->length = fits_end_wo_trailing_spaces - start_offset;
                text_line->size[UI_AXIS_X] = global_ui->graphics->measure_text_width(font, root_widget->text + start_offset,
                                                                                     fits_end_wo_trailing_spaces - start_offset);
                text_line->size[UI_AXIS_Y] = root_widget->text_size[UI_AXIS_Y];
                text_line->max_width = wrap_width;
                text_line->max_end_offset = text_line->length;

                ++root_widget->text_line_count;

                if (!root_widget->text_line)
                {
                    root_widget->text_line = text_line;
                }
                else
                {
                    tail_text_line->next = text_line;
                    text_line->prev = tail_text_line;
                }
                tail_text_line = text_line;

                start_offset = fits_end;
            }
        }
        else
        {
            i32 start_offset = 0;
            i32 end_offset = root_widget->text_length;
            i32 max_end_offset = 0;
            f32 max_width = 0.0f;

            while (start_offset < end_offset)
            {
                f32 width = global_ui->graphics->measure_text_width(font, root_widget->text + start_offset, end_offset - start_offset);
                
                if (width < wrap_width)
                {
                    max_end_offset = end_offset;
                    break;
                }
                
                --end_offset;
            }
            
            ui_text_line_t* text_line = ma_push_struct_zero(global_ui->frame_arena, ui_text_line_t);
            text_line->offset = 0;
            text_line->length = root_widget->text_length;
            text_line->size[UI_AXIS_X] = global_ui->graphics->measure_text_width(font, root_widget->text, root_widget->text_length);
            text_line->size[UI_AXIS_Y] = root_widget->text_size[UI_AXIS_Y];
            text_line->max_width = wrap_width;
            text_line->max_end_offset = max_end_offset;
            root_widget->text_line = text_line;
            root_widget->text_line_count = 1;
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_resolve_text_lines(child_widget, axis);
    }
}

static inline f32 ui_resolve_alignment(ui_widget_t* widget, f32 size, ui_axis_t axis)
{
    f32 position = 0.0f;
    f32 content_position = ui_content_position(widget, axis);
    f32 content_size = ui_content_size(widget, axis);
        
    switch (widget->text_alignment.value[axis])
    {
        case UI_ALIGNMENT_LEADING:
        {
            position = content_position;
        } break;

        case UI_ALIGNMENT_TRAILING:
        {
            position = content_position + content_size - size;
        } break;

        case UI_ALIGNMENT_CENTER:
        {
            position = content_position + (content_size - size) * 0.5f;
        } break;

        default:
        {
            assert(!"[UI] Invalid alignment.");
        } break;
    }

    if (position < content_position)
    {
        position = content_position;
    }

    return position;
}

static void ui_resolve_text_alignment(ui_widget_t* widget, ui_axis_t axis)
{
    if (widget->text)
    {
        if (axis == UI_AXIS_X)
        {
            for (ui_text_line_t* text_line = widget->text_line; text_line; text_line = text_line->next)
            {
                text_line->position[axis] = ui_resolve_alignment(widget, text_line->size[axis], axis);
            }
        }
        else if (axis == UI_AXIS_Y)
        {
            f32 line_height = widget->text_size[axis];
            f32 text_line_size = widget->text_line_count * line_height;
            f32 start_position = ui_resolve_alignment(widget, text_line_size, axis);
            i32 line_index = 0;

            for (ui_text_line_t* text_line = widget->text_line; text_line; text_line = text_line->next)
            {
                text_line->position[axis] = start_position + (line_index * line_height);
                ++line_index;
            }
        }
    }
}

static inline f32 ui_resolve_anchor_position(ui_rect_t rect, ui_anchor_kind_t anchor_kind, ui_axis_t axis)
{
    static const f32 fractions[9][2] =
    {
        [UI_ANCHOR_TOP_LEFT]      = { 0.0f, 0.0f },
        [UI_ANCHOR_TOP_CENTER]    = { 0.5f, 0.0f },
        [UI_ANCHOR_TOP_RIGHT]     = { 1.0f, 0.0f },
        [UI_ANCHOR_CENTER_LEFT]   = { 0.0f, 0.5f },
        [UI_ANCHOR_CENTER]        = { 0.5f, 0.5f },
        [UI_ANCHOR_CENTER_RIGHT]  = { 1.0f, 0.5f },
        [UI_ANCHOR_BOTTOM_LEFT]   = { 0.0f, 1.0f },
        [UI_ANCHOR_BOTTOM_CENTER] = { 0.5f, 1.0f },
        [UI_ANCHOR_BOTTOM_RIGHT]  = { 1.0f, 1.0f },
    };
    f32 anchor_position = rect.xy[axis] + rect.size[axis] * fractions[anchor_kind][axis];

    return anchor_position;
}

static void ui_resolve_size_constraints(ui_widget_t* root_widget, ui_axis_t axis)
{
    if (root_widget->layout_axis != axis)
    {
        for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
        {
            if (ui_is_flag_set(child_widget, UI_FLAG_FLOATING))
            {
                continue;
            }

            f32 child_size = child_widget->fixed_size[axis];
            f32 violation_amount = child_size - (root_widget->fixed_size[axis] - root_widget->padding * 2.0f);
            f32 fix_amount = clamp(0.0f, violation_amount, child_size);

            if (fix_amount > 0.0f)
            {
                child_widget->fixed_size[axis] -= fix_amount;
            }
        }
    }

    if (root_widget->layout_axis == axis)
    {
        f32 total_size = 0.0f;
        f32 total_weighted_size = 0.0f;
        
        for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
        {
            if (ui_is_flag_set(child_widget, UI_FLAG_FLOATING))
            {
                continue;
            }
            
            total_size += child_widget->fixed_size[axis];
            total_weighted_size += child_widget->fixed_size[axis] * (1.0f - child_widget->size[axis].strictness);
        }

        f32 violation_amount = total_size - (root_widget->fixed_size[axis] - root_widget->padding * 2.0f);

        if (violation_amount > 0.0f && total_weighted_size > 0.0f)
        {
            f32 shrink_ratio = min(1.0f, violation_amount / total_weighted_size);

            for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
            {
                if (ui_is_flag_set(child_widget, UI_FLAG_FLOATING))
                {
                    continue;
                }

                f32 giveable = child_widget->fixed_size[axis] * (1.0f - child_widget->size[axis].strictness);

                child_widget->fixed_size[axis] -= giveable * shrink_ratio;
            }
        }
    }
}

static void ui_resolve_sizes(ui_widget_t* root_widget, ui_axis_t axis)
{
    // NOTE: Resolve children's sizes against this parent's final size.
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        switch (child_widget->size[axis].kind)
        {
            case UI_SIZE_PIXEL:
            {
                child_widget->fixed_size[axis] = child_widget->size[axis].value;
            } break;

            case UI_SIZE_CONTENT:
            {
                if (axis == UI_AXIS_X)
                {
                    graphics_2d_font_t font = *(graphics_2d_font_t*)child_widget->font;
                    f32 width = global_ui->graphics->measure_text_width(font, child_widget->text, child_widget->text_length);
                    child_widget->fixed_size[axis] = width + child_widget->padding * 2.0f;
                }
                else if (axis == UI_AXIS_Y)
                {
                    i32 line_count = 0;

                    for (ui_text_line_t* text_line = child_widget->text_line; text_line; text_line = text_line->next)
                    {
                        ++line_count;
                    }
                        
                    if (line_count == 0)
                    {
                        line_count = 1;
                    }
                    
                    child_widget->fixed_size[axis] = line_count * child_widget->text_size[axis] + child_widget->padding * 2.0f;
                }
            } break;

            case UI_SIZE_PARENT:
            {
                f32 interior = root_widget->fixed_size[axis] - root_widget->padding * 2.0f;
                if (interior < 0.0f) interior = 0.0f;
                child_widget->fixed_size[axis] = child_widget->size[axis].value * interior;
            } break;

            case UI_SIZE_CHILDREN:
            {
                // NOTE: Will be resolved after recursing.
                child_widget->fixed_size[axis] = 0.0f;
            } break;
        }
    }

    // NOTE: Resolve children sized kids by recursing first.
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        if (child_widget->size[axis].kind == UI_SIZE_CHILDREN)
        {
            // NOTE: Recurse to resolve grandchildren
            ui_resolve_sizes(child_widget, axis);

            f32 children_size = 0.0f;

            for (ui_widget_t* grand_child_widget = child_widget->child_list.first; grand_child_widget; grand_child_widget = grand_child_widget->child_next)
            {
                if (ui_is_flag_set(grand_child_widget, UI_FLAG_FLOATING))
                {
                    continue;
                }
            
                if (child_widget->layout_axis == axis)
                {
                    children_size += grand_child_widget->fixed_size[axis];
                }
                else
                {
                    children_size = max(children_size, grand_child_widget->fixed_size[axis]);
                }
            }
            
            child_widget->fixed_size[axis] = children_size + child_widget->padding * 2.0f;
        }
    }

    if (root_widget->size[axis].kind != UI_SIZE_CHILDREN)
    {
        // NOTE: Resolve size contraints for parent's children (they all have sizes now).
        ui_resolve_size_constraints(root_widget, axis);
    }

    // NOTE: Recurse into non children sized widgets, their size is now final.
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        if (child_widget->size[axis].kind != UI_SIZE_CHILDREN)
        {
            ui_resolve_sizes(child_widget, axis);
        }
    }
}

static void ui_resolve_layout(ui_widget_t* root_widget, ui_axis_t axis)
{
    bool is_root = root_widget == global_ui->root_widget;
    f32 layout_at = root_widget->padding;

    // NOTE: Normal pass.
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        if (ui_is_flag_set(child_widget, UI_FLAG_FLOATING))
        {
            continue;
        }

        child_widget->rect.xy[axis] = root_widget->rect.xy[axis] + child_widget->position.xy[axis] + layout_at;
        child_widget->rect.size[axis] = child_widget->fixed_size[axis];

        ui_resolve_text_alignment(child_widget, axis);

        if (!is_root && root_widget->layout_axis == axis)
        {
            layout_at += child_widget->fixed_size[axis];
        }
    }

    // NOTE: Floating pass.
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        if (!ui_is_flag_set(child_widget, UI_FLAG_FLOATING) || root_widget->rect.size[axis] <= 0.0f)
        {
            continue;
        }

        ui_rect_t child_rect = { 0.0f, 0.0f, child_widget->fixed_size[0], child_widget->fixed_size[1] };
        f32 root_anchor = ui_resolve_anchor_position(root_widget->rect, child_widget->anchor.parent, axis);
        f32 child_anchor = ui_resolve_anchor_position(child_rect, child_widget->anchor.self, axis);

        child_widget->rect.xy[axis] = root_anchor - child_anchor + child_widget->anchor_offset[axis];
        child_widget->rect.size[axis] = child_widget->fixed_size[axis];
        
        ui_resolve_text_alignment(child_widget, axis);
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_resolve_layout(child_widget, axis);
    }
}

static void ui_print_info(ui_widget_t* root_widget)
{
    fprintf(stderr, "[UI] <%s>:\n"
            "   - fixed size: (%f, %f)\n"
            "   - rect: (%f, %f, %f, %f)\n",
            root_widget->name,
            root_widget->fixed_size[0], root_widget->fixed_size[1],
            root_widget->rect.x, root_widget->rect.y,
            root_widget->rect.width, root_widget->rect.height);

    if (root_widget->text_length)
    {
        fprintf(stderr, 
            "   - text: %s\n"
            "   - text length: %u\n"
            "   - text width: %f\n"
            "   - text height: %f\n",
            root_widget->text, root_widget->text_length,
            root_widget->text_size[UI_AXIS_X], root_widget->text_size[UI_AXIS_Y]);

        fprintf(stderr, "   - text lines:\n");

        while (root_widget->text_line)
        {
            fprintf(stderr,
                    "      - text line: %.*s\n"
                    "      - length: %d\n"
                    "      - width: %f\n"
                    "      - height: %f\n",
                    root_widget->text_line->length, root_widget->text + root_widget->text_line->offset,
                    root_widget->text_line->length, root_widget->text_line->size[0], root_widget->text_line->size[1]);
            
            root_widget->text_line = root_widget->text_line->next;
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_print_info(child_widget);
    }

    if (root_widget == global_ui->root_widget)
    {
        fprintf(stderr, "[UI] State:\n"
                "   - arena remaining size: %.2f MB\n"
                "   - draw command count: %u\n",
                ((f32)ma_get_remaining_size(global_ui->arena) / MIBIBYTES(1)),
                global_ui->draw_command_count);

        ui_widget_t* hot_widget = ui_widget_from_key(global_ui->hot_key);
        ui_widget_t* active_widget = ui_widget_from_key(global_ui->active_key);

        if (hot_widget)
        {
            fprintf(stderr, 
                    "   - hot: (%llu, %s)\n",
                    global_ui->hot_key.value, hot_widget->name);
        }

        if (active_widget)
        {
            fprintf(stderr, 
                    "   - active: (%llu, %s)\n",
                    global_ui->active_key.value, active_widget->name);
        }
    }
}

static inline ui_draw_command_t* ui_push_draw_command(void)
{
    ui_draw_command_chunk_t* chunk = global_ui->draw_commands_last;
    
    if (!chunk || chunk->count == array_count(chunk->commands))
    {
        chunk = ma_push_struct_zero(global_ui->frame_arena, ui_draw_command_chunk_t);

        if (global_ui->draw_commands_last)
        {
            global_ui->draw_commands_last->next = chunk;
        }
        else
        {
            global_ui->draw_commands_first = chunk;
        }

        global_ui->draw_commands_last = chunk;
    }

    global_ui->draw_command_count++;

    ui_draw_command_t* draw_command = &chunk->commands[chunk->count++];
    
    return draw_command;
}

static void ui_emit_draw_commands(ui_widget_t* widget, ui_rect_t clip_rect)
{
    ui_rect_t draw_rect = ui_is_flag_set(widget, UI_FLAG_FLOATING) ?
        widget->rect : ui_rect_intersect(clip_rect, widget->rect);

    if (draw_rect.width <= 0.0f || draw_rect.height <= 0.0f)
    {
        return;
    }
    
    if (ui_is_flag_set(widget, UI_FLAG_BACKGROUND))
    {
        ui_draw_command_t* draw_command = ui_push_draw_command();
        draw_command->kind = UI_DRAW_RECT;
        draw_command->x = draw_rect.x;
        draw_command->y = draw_rect.y;
        draw_command->width = draw_rect.width;
        draw_command->height = draw_rect.height;
        draw_command->color = widget->color;
    }

    if (ui_is_flag_set(widget, UI_FLAG_BORDER))
    {
        ui_rect_t border_rect =
        {
            widget->rect.x - widget->border.thickness * 0.5f,
            widget->rect.y - widget->border.thickness * 0.5f,
            widget->rect.width + widget->border.thickness,
            widget->rect.height + widget->border.thickness,
        };

        ui_rect_t draw_border_rect = ui_is_flag_set(widget, UI_FLAG_FLOATING) ?
            border_rect : ui_rect_intersect(clip_rect, border_rect);

        if (draw_border_rect.width > 0.0f && draw_border_rect.height > 0.0f)
        {
            ui_draw_command_t* draw_command = ui_push_draw_command();
            draw_command->kind = UI_DRAW_BORDER;
            draw_command->x = draw_border_rect.x;
            draw_command->y = draw_border_rect.y;
            draw_command->width = draw_border_rect.width;
            draw_command->height = draw_border_rect.height;
            draw_command->color = widget->border.color;
            draw_command->thickness = widget->border.thickness;
        }
    }

    if (ui_is_flag_set(widget, UI_FLAG_CUSTOM))
    {
        ui_draw_command_t* draw_command = ui_push_draw_command();
        draw_command->kind = UI_DRAW_CUSTOM;
        draw_command->x = draw_rect.x;
        draw_command->y = draw_rect.y;
        draw_command->width = draw_rect.width;
        draw_command->height = draw_rect.height;
    }

    if (ui_is_flag_set(widget, UI_FLAG_TEXT))
    {
        for (ui_text_line_t* text_line = widget->text_line; text_line; text_line = text_line->next)
        {
            ui_rect_t line_rect = (ui_rect_t)
            {
                text_line->position[0], text_line->position[1],
                text_line->size[0], text_line->size[1]
            };
            ui_rect_t draw_line_rect = ui_is_flag_set(widget, UI_FLAG_FLOATING) ?
                line_rect : ui_rect_intersect(clip_rect, line_rect);

            if (draw_line_rect.width <= 0.0f || draw_line_rect.height <= 0.0f)
            {
                continue;
            }

            ui_draw_command_t* draw_command = ui_push_draw_command();
            draw_command->kind = UI_DRAW_TEXT;
            draw_command->x = draw_line_rect.x;
            draw_command->y = draw_line_rect.y;
            draw_command->width = draw_line_rect.width;
            draw_command->height = draw_line_rect.height;
            draw_command->color = widget->font_color;
            draw_command->font = widget->font;
            draw_command->text = widget->text + text_line->offset;
            draw_command->length = text_line->length;
        }
    }
    else if (ui_is_flag_set(widget, UI_FLAG_TEXT_EDIT))
    {
        i32 text_overflow_offset = 0;
        ui_text_line_t* cursor_line = 0;
        i32 cursor_x = widget->text_edit->cursor;
        i32 cursor_y = 0;
        
        for (ui_text_line_t* text_line = widget->text_line; text_line; text_line = text_line->next)
        {
            ui_rect_t line_rect = (ui_rect_t)
            {
                text_line->position[0], text_line->position[1],
                text_line->size[0], text_line->size[1]
            };
            ui_rect_t draw_line_rect = ui_is_flag_set(widget, UI_FLAG_FLOATING) ?
                line_rect : ui_rect_intersect(clip_rect, line_rect);

            if (draw_line_rect.width > 0.0f && draw_line_rect.height > 0.0f)
            {
                f32 text_width = draw_line_rect.width;
                i32 text_length = text_line->length;

                // NOTE: Text is longer than total widget width.
                if (text_line->size[UI_AXIS_X] >= text_width)
                {
                    text_width = text_line->max_width;
                    text_length = text_line->max_end_offset;
                    text_overflow_offset = text_line->length - text_line->max_end_offset;
                }

                // NOTE: If we have text overflow and cursor is behind the overflow offset,
                // we should clip the overflow offset to cursor to be able to see correct
                // part of the text.
                if (cursor_x < text_overflow_offset)
                {
                    text_overflow_offset = cursor_x;
                }

                // TODO: What should we do when there is a wrapped text but not enough size to display it correctly?
                // It should be similar to how unwrapped version slides according or cursor.
                // Right now we just draw the part that fits and if we type more nothing changes.
                // It is just clipped and cursor is not visible.
                if (ui_is_flag_set(widget, UI_FLAG_WRAP_TEXT))
                {
                    if (!cursor_line)
                    {
                        if (cursor_x > text_line->length)
                        {
                            cursor_x -= text_line->length;
                            ++cursor_y;
                        }
                        else
                        {
                            cursor_line = text_line;
                        }
                    }
                }
                else
                {
                    cursor_line = text_line;
                }

                ui_draw_command_t* draw_command = ui_push_draw_command();
                draw_command->kind = UI_DRAW_TEXT;
                draw_command->x = draw_line_rect.x;
                draw_command->y = draw_line_rect.y;
                draw_command->width = draw_line_rect.width;
                draw_command->height = draw_line_rect.height;
                draw_command->color = widget->font_color;
                draw_command->font = widget->font;
                draw_command->text = widget->text + text_line->offset + text_overflow_offset;
                draw_command->length = text_length;
            }
        }

        if (ui_keys_equal(widget->key, global_ui->focus_key))
        {
            u32 text_line_count = max(1, widget->text_line_count);
            
            ui_rect_t cursor_rect = (ui_rect_t)
            {
                ui_resolve_alignment(widget, 1.0f, UI_AXIS_X),
                ui_resolve_alignment(widget, text_line_count * widget->text_size[UI_AXIS_Y], UI_AXIS_Y) + (cursor_y * widget->text_size[UI_AXIS_Y]),
                1.0f, widget->text_size[UI_AXIS_Y]
            };
            ui_rect_t draw_cursor_rect = ui_is_flag_set(widget, UI_FLAG_FLOATING) ?
                cursor_rect : ui_rect_intersect(clip_rect, cursor_rect);

            if (draw_cursor_rect.width > 0.0f && draw_cursor_rect.height > 0)
            {
                f32 cursor_offset = 0.0f;

                if (cursor_line)
                {
                    graphics_2d_font_t font = *(graphics_2d_font_t*)widget->font;
                    cursor_offset = global_ui->graphics->measure_text_width(font, widget->text + text_overflow_offset + cursor_line->offset, cursor_x - text_overflow_offset);
                }
                
                ui_draw_command_t* draw_command = ui_push_draw_command();
                draw_command->kind = UI_DRAW_RECT;
                draw_command->x = draw_cursor_rect.x + cursor_offset;
                draw_command->y = draw_cursor_rect.y;
                draw_command->width = draw_cursor_rect.width;
                draw_command->height = draw_cursor_rect.height;
                draw_command->color = widget->font_color;
            }
        }
    }
}

static void ui_emit_subtree(ui_widget_t* root_widget, ui_rect_t clip_rect)
{
    ui_emit_draw_commands(root_widget, clip_rect);

    ui_rect_t child_clip_rect = ui_rect_intersect(clip_rect, root_widget->rect);

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_emit_subtree(child_widget, child_clip_rect);
    }
}

static void ui_emit_normal_pass(ui_widget_t* root_widget, ui_rect_t clip_rect)
{
    if (ui_is_flag_set(root_widget, UI_FLAG_FLOATING))
    {
        return;
    }

    ui_emit_draw_commands(root_widget, clip_rect);

    ui_rect_t child_clip_rect = ui_rect_intersect(clip_rect, root_widget->rect);

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_emit_normal_pass(child_widget, child_clip_rect);
    }
}

static void ui_emit_floating_pass(ui_widget_t* root_widget, ui_rect_t clip_rect)
{
    if (ui_is_flag_set(root_widget, UI_FLAG_FLOATING))
    {
        ui_emit_subtree(root_widget, clip_rect);
        return;
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_emit_floating_pass(child_widget, clip_rect);
    }
}

static void ui_emit_all_draw_commands(ui_widget_t* root_widget)
{
    ui_rect_t clip_rect = global_ui->root_widget->rect;
    ui_emit_normal_pass(root_widget, clip_rect);
    ui_emit_floating_pass(root_widget, clip_rect);
}

static void ui_init(memory_arena_t* memory_arena)
{
    assert(!global_ui && "[UI] Already initialized.");

    memory_arena_t* ui_arena = ma_create_sub_arena(memory_arena, MIBIBYTES(4));
    memory_arena_t* ui_frame_arena = ma_create_sub_arena(ui_arena, MIBIBYTES(2));
    global_ui = (ui_t*)ma_push_size_zero(ui_arena, sizeof(ui_t));
    global_ui->arena = ui_arena;
    global_ui->frame_arena = ui_frame_arena;

    // IMPORTANT: We need to be sure to use correct types here because there is no way to 
    // catch it reliably and it can silently corrupt stacks without visible error.
    ui_stack_init(global_ui->arena, &global_ui->stacks.parent, sizeof(ui_widget_t*), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.size_x, sizeof(ui_size_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.size_y, sizeof(ui_size_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.layout_axis, sizeof(ui_axis_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.padding, sizeof(f32), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.color, sizeof(vec4), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.border, sizeof(ui_border_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.flags, sizeof(ui_flags_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.anchor, sizeof(ui_anchor_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.anchor_offset, sizeof(ui_anchor_offset_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.font, sizeof(ui_font_t), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.font_color, sizeof(vec4), UI_STACK_SIZE);
    ui_stack_init(global_ui->arena, &global_ui->stacks.text_alignment, sizeof(ui_alignment_t), UI_STACK_SIZE);

}

static ui_widget_t* ui_hit_test(ui_widget_t* root_widget, vec2 mouse_position)
{
    for (ui_widget_t* child_widget = root_widget->child_list.last; child_widget; child_widget = child_widget->child_prev)
    {
        if (!ui_is_flag_set(child_widget, UI_FLAG_FLOATING))
        {
            continue;
        }

        ui_widget_t* hit_widget = ui_hit_test(child_widget, mouse_position);

        if (hit_widget)
        {
            return hit_widget;
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.last; child_widget; child_widget = child_widget->child_prev)
    {
        if (ui_is_flag_set(child_widget, UI_FLAG_FLOATING))
        {
            continue;
        }

        ui_widget_t* hit_widget = ui_hit_test(child_widget, mouse_position);

        if (hit_widget)
        {
            return hit_widget;
        }
    }

    if (ui_is_flag_set(root_widget, UI_FLAG_CLICKABLE) && ui_rect_contains_point(root_widget->rect, mouse_position))
    {
        return root_widget;
    }

    return 0;
}

static void ui_resolve_hot(void)
{
    global_ui->hot_key = (ui_key_t){ 0 };
    ui_widget_t* widget = ui_hit_test(global_ui->root_widget, global_ui->input->mouse_position);

    if (widget)
    {
        global_ui->hot_key = widget->key;
        widget->hot = true;
    }
}

static void ui_resolve_active(void)
{
    bool pressed = input_is_mouse_pressed(global_ui->input, KEY_MOUSE_LEFT);
    bool released = input_is_mouse_released(global_ui->input, KEY_MOUSE_LEFT);

    if (pressed)
    {
        global_ui->active_key = global_ui->hot_key;
    }

    if (released)
    {
        ui_widget_t* widget = ui_widget_from_key(global_ui->active_key);
 
        if (widget)
        {
            global_ui->released_key = global_ui->active_key;
            widget->released = true;
            
            if (ui_keys_equal(global_ui->active_key, global_ui->hot_key))
            {
                global_ui->clicked_key = global_ui->active_key;
                widget->clicked = true;
            }
        }
            
        global_ui->active_key = (ui_key_t){ 0 };
    }

    ui_widget_t* widget = ui_widget_from_key(global_ui->active_key);

    if (widget)
    {
        widget->active = true;
    }
}

static void ui_resolve_focus(void)
{
    bool pressed = input_is_key_pressed(global_ui->input, KEY_MOUSE_LEFT);

    if (pressed)
    {
        ui_widget_t* hot_widget = ui_widget_from_key(global_ui->hot_key);

        if (hot_widget && ui_is_flag_set(hot_widget, UI_FLAG_FOCUSABLE))
        {
            global_ui->focus_key = hot_widget->key;
        }
        else if (hot_widget && ui_keys_equal(hot_widget->key, global_ui->focus_key))
        {
            global_ui->focus_key = hot_widget->key;
        }
        else
        {
            global_ui->focus_key = (ui_key_t){ 0 };
        }
    }
}

static void ui_resolve_interaction(void)
{
    ui_widget_t* widget = 0;

    if ((widget = ui_widget_from_key(global_ui->hot_key)))
    {
        widget->hot = false;
    }

    if ((widget = ui_widget_from_key(global_ui->active_key)))
    {
        widget->active = false;
    }

    if ((widget = ui_widget_from_key(global_ui->clicked_key)))
    {
        widget->clicked = false;
    }

    if ((widget = ui_widget_from_key(global_ui->released_key)))
    {
        widget->released = false;
    }

    global_ui->clicked_key = (ui_key_t){ 0 };
    global_ui->released_key = (ui_key_t){ 0 };
    
    ui_resolve_hot();
    ui_resolve_active();
    ui_resolve_focus();
}

static void ui_begin(graphics_t* graphics, input_t* input, f32 width, f32 height)
{
    assert(global_ui && "[UI] Not initialized.");
    assert(input && "[UI] Input is required.");
    assert(graphics && "[UI] Graphics is required.");

    global_ui->graphics = graphics;
    global_ui->input = input;
    
    ma_reset(global_ui->frame_arena);
    global_ui->draw_commands_first = 0;
    global_ui->draw_commands_last = 0;
    global_ui->draw_command_count = 0;

    // NOTE: This is for making sure that ui_end() is called at the end of previous frame.
    assert(global_ui->stacks.parent.count == 0 && "[UI] Invalid parent stack count.");

    ui_stack_push(&global_ui->stacks.parent, &(void*){ 0 });
    ui_stack_push(&global_ui->stacks.size_x, &(ui_size_t){ 0 });
    ui_stack_push(&global_ui->stacks.size_y, &(ui_size_t){ 0 });
    ui_stack_push(&global_ui->stacks.layout_axis, &(ui_axis_t){ UI_AXIS_Y });
    ui_stack_push(&global_ui->stacks.padding, &(f32){ 0 });
    ui_stack_push(&global_ui->stacks.color, &(vec4){ 0 });
    ui_stack_push(&global_ui->stacks.border, &(ui_border_t){ 0 });
    ui_stack_push(&global_ui->stacks.flags, &(void*){ 0 });
    ui_stack_push(&global_ui->stacks.anchor, &(ui_anchor_t){ 0 });
    ui_stack_push(&global_ui->stacks.anchor_offset, &(ui_anchor_offset_t){ 0 });
    ui_stack_push(&global_ui->stacks.font, &(ui_font_t){ 0 });
    ui_stack_push(&global_ui->stacks.font_color, &(vec4){ 0 });
    ui_stack_push(&global_ui->stacks.text_alignment, &(ui_alignment_t){ 0 });

    global_ui->root_widget = ui_widget_group_begin("root_widget", 0.0f, 0.0f);
    global_ui->root_widget->fixed_size[UI_AXIS_X] = width;
    global_ui->root_widget->fixed_size[UI_AXIS_Y] = height;
    global_ui->root_widget->rect = (ui_rect_t){ 0.0f, 0.0f, width, height };
}

static void ui_end(void)
{
    for (i32 axis = 0; axis < UI_AXIS_COUNT; ++axis)
    {
        ui_resolve_sizes(global_ui->root_widget, axis);

        if (axis == UI_AXIS_X)
        {
            ui_resolve_text_lines(global_ui->root_widget, axis);
        }

        ui_resolve_layout(global_ui->root_widget, axis);
    }

    ui_resolve_interaction();
    ui_emit_all_draw_commands(global_ui->root_widget);
    ui_print_info(global_ui->root_widget);
    
    ui_widget_group_end();

    ui_pop_parent();
    ui_pop_size_x();
    ui_pop_size_y();
    ui_pop_axis();
    ui_pop_padding();
    ui_pop_color();
    ui_pop_border_props();
    ui_pop_flags();
    ui_pop_anchor();
    ui_pop_anchor_offset();
    ui_pop_font();
    ui_pop_font_color();
    ui_pop_text_alignment();

    global_ui->root_widget = 0;

    assert(global_ui->stacks.parent.count == 0 && "[UI] Invalid parent stack count.");
    assert(global_ui->stacks.size_x.count == 0 && "[UI] Invalid size_x stack count.");
    assert(global_ui->stacks.size_y.count == 0 && "[UI] Invalid size_y stack count.");
    assert(global_ui->stacks.layout_axis.count == 0 && "[UI] Invalid layout axis stack count.");
    assert(global_ui->stacks.padding.count == 0 && "[UI] Invalid padding stack count.");
    assert(global_ui->stacks.color.count == 0 && "[UI] Invalid color stack count.");
    assert(global_ui->stacks.border.count == 0 && "[UI] Invalid border stack count.");
    assert(global_ui->stacks.flags.count == 0 && "[UI] Invalid flags stack count.");
    assert(global_ui->stacks.anchor.count == 0 && "[UI] Invalid anchor stack count.");
    assert(global_ui->stacks.anchor_offset.count == 0 && "[UI] Invalid anchor offset stack count.");
    assert(global_ui->stacks.font.count == 0 && "[UI] Invalid font stack count.");
    assert(global_ui->stacks.font_color.count == 0 && "[UI] Invalid font_color stack count.");
    assert(global_ui->stacks.text_alignment.count == 0 && "[UI] Invalid text_alignment stack count.");
}

static ui_draw_command_iter_t ui_draw_command_iter(void)
{
    ui_draw_command_iter_t iter = (ui_draw_command_iter_t)
    {
        .chunk = global_ui->draw_commands_first,
        .index = 0,
    };

    return iter;
}

static ui_draw_command_t* ui_draw_command_next(ui_draw_command_iter_t* iter)
{
    ui_draw_command_t* draw_command = 0;
    
    while (iter->chunk)
    {
        if (iter->index < iter->chunk->count)
        {
            draw_command = &iter->chunk->commands[iter->index++];
            break;
        }

        iter->chunk = iter->chunk->next;
        iter->index = 0;
    }

    return draw_command;
}
