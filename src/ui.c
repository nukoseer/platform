#include "ui.h"

typedef struct ui_t
{
    memory_arena_t* arena;
    memory_arena_t* transient_arena;
    memory_arena_t* text_line_arena;
    ui_widget_t* root_widget;
    ui_widget_list_t widget_lists[64];
    ui_stack_t stack;
    ui_draw_command_t widget_draw_commands[64];
    u32 widget_draw_command_count;
    ui_widget_t* free_widgets;

    ui_callback_t callback;
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

static inline f32 ui_label_content_position(ui_widget_t* widget, ui_axis_t axis)
{
    f32 content_position = 0.0f;

    if (axis == UI_AXIS_X)
    {
        content_position = widget->rect.x + widget->padding;
    }
    else if (axis == UI_AXIS_Y)
    {
        content_position = widget->rect.y + widget->padding;
    }
    else
    {
        assert(!"[UI] Invalid axis.");
    }

    return content_position;
}

static inline f32 ui_label_content_size(ui_widget_t* widget, ui_axis_t axis)
{
    f32 content_size = 0.0f;

    if (axis == UI_AXIS_X)
    {
        content_size = widget->rect.width - widget->padding * 2.0f;
    }
    else if (axis == UI_AXIS_Y)
    {
        content_size = widget->rect.height - widget->padding * 2.0f;
    }
    else
    {
        assert(!"[UI] Invalid axis.");
    }

    return content_size > 0.0f ? content_size : 0.0f;
}

#define UI_INITIAL_HASH 0xCBF29CE484222325ULL

static ui_key_t ui_get_key(ui_key_t parent_widget_key, const char* data, u64 size)
{
    ui_key_t widget_key = { 0 };
    u64 seed = UI_INITIAL_HASH;

    if (parent_widget_key.value != 0)
    {
        seed = parent_widget_key.value;
    }

    widget_key.value = ui_hash(seed, data, size);

    return widget_key;
}

static ui_key_t ui_get_key_from_string(ui_key_t parent_widget_key, const char* string)
{
    u64 length = strlen(string);
    ui_key_t key = ui_get_key(parent_widget_key, string, length);

    return key;
}

static inline bool ui_keys_are_equal(ui_key_t a_widget_key, ui_key_t b_widget_key)
{
    bool result = a_widget_key.value == b_widget_key.value;

    return result;
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

// TODO: Not tested.
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
            if (ui_keys_are_equal(widget_iter->key, key))
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
            widget = ma_push_size(global_ui->transient_arena, sizeof(ui_widget_t));
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
    widget->rect = (ui_rect_t){ 0 };
    widget->key = key;
    widget->position = (ui_position_t){ 0 };
    widget->label_length = 0;
    widget->text_line = 0;

    // NOTE: The root widget does not have parent.
    ui_widget_t* parent_widget = global_ui->root_widget != 0 ? ui_top_parent() : 0;
    widget->parent = parent_widget;

    if (parent_widget)
    {
        ui_widget_list_child_insert_back(&parent_widget->child_list, widget);
    }

    widget->size[UI_AXIS_X] = ui_top_size_x();
    widget->size[UI_AXIS_Y] = ui_top_size_y();
    widget->layout_axis = ui_top_layout_axis();
    widget->padding = ui_top_padding();
    widget->color = ui_top_color();
    widget->border = ui_top_border();
    widget->flags = ui_top_flags();

    return widget;
}

// TODO: Find matching non null parent key.
static ui_widget_t* ui_widget_build_from_string(const char* widget_name)
{
    ui_key_t parent_key = global_ui->stack.parent_count == 0 ? (ui_key_t){ 0 } : ui_top_parent()->key;
    ui_key_t widget_key = ui_get_key_from_string(parent_key, widget_name);
    ui_widget_t* widget = ui_widget_build_from_key(widget_key);
    widget->name = widget_name;

    return widget;
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

static void ui_set_label(ui_widget_t* widget, const char* label, u32 label_length)
{
    assert(label && label_length < array_count(widget->label) && "[UI] Invalid label.");

    memcpy(widget->label, label, label_length);
    widget->label[label_length] = '\0';
    widget->label_length = label_length;

    widget->font = ui_top_font();
    widget->font_color = ui_top_font_color();
    widget->label_alignment = ui_top_label_alignment();
    
    ui_callback_t* callback = &global_ui->callback;
    ui_measure_text_width_t* measure_text_width = &callback->measure_text_width;
    ui_get_line_height_t* get_line_height = &callback->get_line_height;
        
    widget->label_size[UI_AXIS_X] = measure_text_width->function(widget->font, widget->label,
                                                                        widget->label_length,
                                                                        measure_text_width->parameter);
    widget->label_size[UI_AXIS_Y] = get_line_height->function(widget->font,
                                                                     get_line_height->parameter);
}

static ui_widget_t* ui_widget_labeled(const char* widget_name, const char* label)
{
    ui_widget_t* widget = ui_widget_build_from_string(widget_name);
    assert(label && "[UI] Invalid label.");
    ui_set_label(widget, label, (u32)strlen(label));

    return widget;
}

static void ui_calculate_pixel_sizes(ui_widget_t* root_widget, ui_axis_t axis)
{
    if (root_widget->size[axis].kind == UI_SIZE_PIXEL)
    {
        root_widget->fixed_size[axis] = root_widget->size[axis].value;
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_pixel_sizes(child_widget, axis);
    }
}

static void ui_calculate_parent_dependent_sizes(ui_widget_t* root_widget, ui_axis_t axis)
{
    ui_widget_t* found_parent_widget = 0;
    
    if (root_widget->size[axis].kind == UI_SIZE_PARENT)
    {
        for (ui_widget_t* parent_widget = root_widget->parent; parent_widget; parent_widget = parent_widget->parent)
        {
            if (parent_widget->size[axis].kind == UI_SIZE_PARENT ||
                parent_widget->size[axis].kind == UI_SIZE_PIXEL)
            {
                found_parent_widget = parent_widget;
                break;
            }
        }

        if (found_parent_widget)
        {
            f32 parent_interior = found_parent_widget->fixed_size[axis] - found_parent_widget->padding * 2.0f;
            if (parent_interior < 0.0f) parent_interior = 0.0f;
            root_widget->fixed_size[axis] = root_widget->size[axis].value * parent_interior;
            // root_widget->fixed_size[axis] = root_widget->size[axis].value * found_parent_widget->fixed_size[axis];
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_parent_dependent_sizes(child_widget, axis);
    }
}

static void ui_calculate_children_dependent_sizes(ui_widget_t* root_widget, ui_axis_t axis)
{
    if (root_widget->size[axis].kind == UI_SIZE_CHILDREN)
    {
        f32 children_size = 0.0f;
        
        for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
        {
            if (child_widget->layout_axis == axis)
            {
                children_size += child_widget->fixed_size[axis];
            }
            else
            {
                children_size = max(children_size, child_widget->fixed_size[axis]);
            }
        }

        root_widget->fixed_size[axis] = children_size - root_widget->padding * 2.0f;
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_children_dependent_sizes(child_widget, axis);
    }
}

static void ui_calculate_text_sizes(ui_widget_t* root_widget, ui_axis_t axis)
{
    if (root_widget->label_length)
    {
        f32 total_size = 0.0f;
        
        if (axis == UI_AXIS_X)
        {
            ui_measure_text_width_t* measure_text_width = &global_ui->callback.measure_text_width;
            i32 start_offset = 0;
            i32 end_offset = root_widget->label_length;
            ui_text_line_t* tail_text_line = 0;
            
            while (start_offset < end_offset)
            {
                i32 fits_end = start_offset;
                i32 space = string_find_leading_char(root_widget->label, start_offset, end_offset - start_offset, ' ');
                i32 word_end = space == -1 ? end_offset : space + 1;

                while (word_end != -1 && word_end <= end_offset)
                {
                    f32 width = measure_text_width->function(root_widget->font, root_widget->label + start_offset, word_end - start_offset, measure_text_width->parameter);

                    if (width > root_widget->fixed_size[UI_AXIS_X] - root_widget->padding * 2.0f)
                    {
                        break;
                    }

                    fits_end = word_end;

                    if (word_end == end_offset)
                    {
                        break;
                    }

                    space = string_find_leading_char(root_widget->label, word_end, end_offset - word_end, ' ');
                    word_end = space == -1 ? end_offset : space + 1;
                }

                if (fits_end == start_offset)
                {
                    for (i32 i = start_offset + 1; i <= end_offset; ++i)
                    {
                        if (measure_text_width->function(root_widget->font, root_widget->label + start_offset, i - start_offset, measure_text_width->parameter) > root_widget->fixed_size[UI_AXIS_X] - root_widget->padding * 2.0f)
                        {
                            fits_end = max(i - 1, start_offset + 1);
                            break;
                        }

                        fits_end = i;
                    }
                    break;
                }

                //NOTE: Remove leading spaces from the line.
                while (start_offset < fits_end && root_widget->label[start_offset] == ' ')
                {
                    ++start_offset;
                }
                
                // NOTE: Remove trailing spaces from the line.
                i32 fits_end_wo_trailing_spaces = fits_end;
                while (fits_end_wo_trailing_spaces > start_offset && root_widget->label[fits_end_wo_trailing_spaces - 1] == ' ')
                {
                    --fits_end_wo_trailing_spaces;
                }

                ui_text_line_t* text_line = ma_push_struct_zero(global_ui->text_line_arena, ui_text_line_t);
                text_line->offset = start_offset;
                text_line->length = fits_end_wo_trailing_spaces - start_offset;
                text_line->size[axis] = measure_text_width->function(root_widget->font, root_widget->label + start_offset,
                                                                                 fits_end_wo_trailing_spaces - start_offset,
                                                                                 measure_text_width->parameter);
                total_size += text_line->size[axis];

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
        else if (axis == UI_AXIS_Y)
        {
            f32 line_height = root_widget->label_size[axis];
                
            for (ui_text_line_t* text_line = root_widget->text_line; text_line; text_line = text_line->next)
            {
                text_line->size[axis] = line_height;
                total_size += text_line->size[axis];
            }
        }

        if (root_widget->size[axis].kind == UI_SIZE_CONTENT)
        {
            if (total_size == 0.0f)
            {
                total_size = root_widget->label_size[axis];
            }

            root_widget->fixed_size[axis] = total_size;
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_text_sizes(child_widget, axis);
    }
}

static void ui_calculate_size_violations(ui_widget_t* root_widget, ui_axis_t axis)
{
    if (root_widget->layout_axis != axis)
    {
        for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
        {
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
            total_size += child_widget->fixed_size[axis];
            total_weighted_size += child_widget->fixed_size[axis] * (1.0f - child_widget->size[axis].strictness);
        }

        f32 violation_amount = total_size - (root_widget->fixed_size[axis] - root_widget->padding * 2.0f);

        if (violation_amount > 0.0f && total_weighted_size > 0.0f)
        {
            f32 shrink_ratio = min(1.0f, violation_amount / total_weighted_size);

            for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
            {
                f32 giveable = child_widget->fixed_size[axis] * (1.0f - child_widget->size[axis].strictness);
                child_widget->fixed_size[axis] -= giveable * shrink_ratio;
            }
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_size_violations(child_widget, axis);
    }
}

static inline f32 ui_calculate_content_alignment(ui_widget_t* widget, ui_text_line_t* text_line, ui_axis_t axis)
{
    f32 position = 0.0f;
    f32 content_position = ui_label_content_position(widget, axis);
    f32 content_size = ui_label_content_size(widget, axis);
        
    switch (widget->label_alignment.value[axis])
    {
        case UI_ALIGNMENT_LEADING:
        {
            position = content_position;
        } break;

        case UI_ALIGNMENT_TRAILING:
        {
            position = content_position + content_size - text_line->size[axis];
        } break;

        case UI_ALIGNMENT_CENTER:
        {
            position = content_position + (content_size - text_line->size[axis]) * 0.5f;
        } break;

        default:
        {
            assert(!"[UI] Invalid label alignment.");
        } break;
    }

    if (position < content_position)
    {
        position = content_position;
    }

    return position;
}

static void ui_calculate_label_alignment(ui_widget_t* widget, ui_axis_t axis)
{
    if (widget->label_length)
    {
        if (axis == UI_AXIS_X)
        {
            for (ui_text_line_t* text_line = widget->text_line; text_line; text_line = text_line->next)
            {
                text_line->position[UI_AXIS_X] = ui_calculate_content_alignment(widget, text_line, UI_AXIS_X);
            }
        }
        else if (axis == UI_AXIS_Y)
        {
            f32 line_height = widget->label_size[UI_AXIS_Y];
            i32 line_count = 0;
                
            for (ui_text_line_t* text_line = widget->text_line; text_line; text_line = text_line->next)
            {
                text_line->position[UI_AXIS_Y] = (ui_calculate_content_alignment(widget, text_line, UI_AXIS_Y) +
                                                         line_count * line_height);
                ++line_count;
            }
        }
    }
}

static void ui_calculate_layout(ui_widget_t* root_widget, ui_axis_t axis)
{
    f32 layout_at = root_widget->padding;
    
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        child_widget->position.xy[axis] += layout_at;

        if (root_widget->layout_axis == axis)
        {
            layout_at += child_widget->fixed_size[axis];
        }

        if (axis == UI_AXIS_X)
        {
            child_widget->rect.x = root_widget->rect.x + child_widget->position.xy[axis];
            child_widget->rect.width = child_widget->fixed_size[axis];

            ui_calculate_label_alignment(child_widget, axis);
        }
        else if (axis == UI_AXIS_Y)
        {
            child_widget->rect.y = root_widget->rect.y + child_widget->position.xy[axis];
            child_widget->rect.height = child_widget->fixed_size[axis];

            ui_calculate_label_alignment(child_widget, axis);
        }
        else
        {
            assert(!"[UI] Invalid axis.");
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_layout(child_widget, axis);
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

    if (root_widget->label_length)
    {
        fprintf(stderr, 
            "   - label: %s\n"
            "   - label length: %u\n"
            "   - label width: %f\n"
            "   - label height: %f\n",
            root_widget->label, root_widget->label_length,
            root_widget->label_size[UI_AXIS_X], root_widget->label_size[UI_AXIS_Y]);

        fprintf(stderr, "   - text lines:\n");

        while (root_widget->text_line)
        {
            fprintf(stderr,
                    "      - text line: %.*s\n"
                    "      - length: %d\n"
                    "      - width: %f\n"
                    "      - height: %f\n",
                    root_widget->text_line->length, root_widget->label + root_widget->text_line->offset,
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
            "   - arena remaining size: %zu\n"
            "   - draw command count: %u\n",
                ma_get_remaining_size(global_ui->arena),
                global_ui->widget_draw_command_count);
    }
}

static void ui_calculate_draw_rect_commands(ui_widget_t* root_widget)
{
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        f32 rect_x = child_widget->rect.x;
        f32 rect_y = child_widget->rect.y;
        f32 rect_width = child_widget->rect.width;
        f32 rect_height = child_widget->rect.height;

        if (rect_x > root_widget->rect.x + root_widget->rect.width ||
            rect_y > root_widget->rect.y + root_widget->rect.height)
        {
            continue;
        }
        else if (rect_x + rect_width > root_widget->rect.x ||
                 rect_y + rect_height > root_widget->rect.y)
        {
            f32 x_overlap = max(0.0f, min(rect_x + rect_width, root_widget->rect.x + root_widget->rect.width) -
                max(rect_x, root_widget->rect.x));
            f32 y_overlap = max(0.0f, min(rect_y + rect_height, root_widget->rect.y + root_widget->rect.height) -
                max(rect_y, root_widget->rect.y));

            if (x_overlap <= 0.0f || y_overlap <= 0.0f)
            {
                continue;
            }

            rect_x = max(rect_x, root_widget->rect.x);
            rect_y = max(rect_y, root_widget->rect.y);
            rect_width = x_overlap;
            rect_height = y_overlap;
        }
        
        assert(global_ui->widget_draw_command_count < array_count(global_ui->widget_draw_commands) && "[UI] Invalid draw command count.");
        ui_draw_command_t* widget_draw_command = global_ui->widget_draw_commands + global_ui->widget_draw_command_count++;
        
        *widget_draw_command = (ui_draw_command_t)
        {
            .kind = child_widget->flags == 0 ? UI_DRAW_RECT : UI_DRAW_CUSTOM,
            .x = rect_x,
            .y = rect_y,
            .width = rect_width,
            .height = rect_height,
            .color = child_widget->color,
        };

        for (ui_text_line_t* text_line = child_widget->text_line; text_line; text_line = text_line->next)
        {
            f32 border_x = text_line->position[0] - child_widget->padding;
            f32 border_y = text_line->position[1] - child_widget->padding;
            f32 border_width = text_line->size[0] + child_widget->padding * 2.0f;
            f32 border_height = text_line->size[1] + child_widget->padding * 2.0f;

            if (border_x < child_widget->rect.x || border_y < child_widget->rect.y ||
                border_x + border_width > child_widget->rect.x + child_widget->rect.width ||
                border_y + border_height > child_widget->rect.y + child_widget->rect.height)
            {
                continue;
            }
            
            assert(global_ui->widget_draw_command_count < array_count(global_ui->widget_draw_commands) && "[UI] Invalid draw command count.");
            ui_draw_command_t* widget_draw_text_command = global_ui->widget_draw_commands + global_ui->widget_draw_command_count++;

            *widget_draw_text_command = (ui_draw_command_t)
            {
                .kind = UI_DRAW_TEXT,
                .x = text_line->position[0],
                .y = text_line->position[1],
                .width = text_line->size[0],
                .height = text_line->size[1],
                .color = child_widget->font_color,
                .font = child_widget->font,
                .text = child_widget->label + text_line->offset,
                .length = text_line->length,
            };
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_draw_rect_commands(child_widget);
    }
}

static void ui_calculate_draw_border_commands(ui_widget_t* root_widget)
{
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        if (child_widget->border.enabled)
        {
            assert(global_ui->widget_draw_command_count < array_count(global_ui->widget_draw_commands) && "[UI] Invalid draw command count.");
            ui_draw_command_t* widget_draw_command = global_ui->widget_draw_commands + global_ui->widget_draw_command_count++;

            *widget_draw_command = (ui_draw_command_t)
            {
                .kind = UI_DRAW_BORDER,
                .x = child_widget->rect.x - child_widget->border.thickness * 0.5f,
                .y = child_widget->rect.y - child_widget->border.thickness * 0.5f,
                .width = child_widget->rect.width + child_widget->border.thickness * 0.5f,
                .height = child_widget->rect.height + child_widget->border.thickness * 0.5f,
                .color = child_widget->border.color,
                .thickness = child_widget->border.thickness,
            };  
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_calculate_draw_border_commands(child_widget);
    }
}

static void ui_begin(memory_arena_t* memory_arena, f32 width, f32 height, ui_callback_t callback)
{
    if (!global_ui)
    {
        memory_arena_t* ui_arena = ma_create_sub_arena(memory_arena, MIBIBYTES(4));
        memory_arena_t* ui_transient_arena = ma_create_sub_arena(ui_arena, MIBIBYTES(1));
        memory_arena_t* ui_text_line_arena = ma_create_sub_arena(ui_arena, MIBIBYTES(1));
        global_ui = (ui_t*)ma_push_size_zero(ui_arena, sizeof(ui_t));
        global_ui->arena = ui_arena;
        global_ui->transient_arena = ui_transient_arena;
        global_ui->text_line_arena = ui_text_line_arena;
        global_ui->callback = callback;
    }

    ma_reset(global_ui->transient_arena);
    ma_reset(global_ui->text_line_arena);

    // NOTE: This is for making sure that ui_end() is called at the end of previous frame.
    assert(global_ui->stack.parent_count == 0 && "[UI] Invalid parent stack count.");

    ui_push_size_x(ui_pixel(width, 1.0f));
    ui_push_size_y(ui_pixel(height, 1.0f));
    ui_push_layout_axis(UI_AXIS_Y);
    ui_push_padding(0.0f);
    ui_push_color(v4(0.0f, 0.0f, 0.0f, 0.0f));
    ui_push_border((ui_border_t){ false, 0.0f, v4(0.0f, 0.0f, 0.0f, 0.0f) });
    ui_push_font(0);
    ui_push_font_color(v4(0.0f, 0.0f, 0.0f, 0.0f));
    ui_push_label_alignment(ui_align_leading());
    ui_push_flags(0);

    ui_widget_t* root_widget = ui_widget_group_begin("root_widget", 0.0f, 0.0f);
    global_ui->root_widget = root_widget;
    global_ui->widget_draw_command_count = 0; 
}

static ui_draw_command_list_t ui_end(void)
{
    assert(global_ui->stack.parent_count == 1 && "[UI] Invalid parent stack count.");

    for (i32 axis = 0; axis < UI_AXIS_COUNT; ++axis)
    {
        ui_calculate_pixel_sizes(global_ui->root_widget, axis);
        ui_calculate_parent_dependent_sizes(global_ui->root_widget, axis);
        ui_calculate_children_dependent_sizes(global_ui->root_widget, axis);
        ui_calculate_size_violations(global_ui->root_widget, axis);
        // TODO: FInal size of widgets are determined in ui_calculate_size_violations, so
        // text sizes should be calculated afterwards but this may introduces bugs.
        ui_calculate_text_sizes(global_ui->root_widget, axis);
        ui_calculate_layout(global_ui->root_widget, axis);
    }

    ui_calculate_draw_rect_commands(global_ui->root_widget);
    ui_calculate_draw_border_commands(global_ui->root_widget);
    ui_print_info(global_ui->root_widget);
    
    ui_draw_command_list_t widget_draw_command_list =
    {
        .commands = global_ui->widget_draw_commands,
        .command_count = global_ui->widget_draw_command_count,
    };

    ui_pop_size_x();
    ui_pop_size_y();
    ui_pop_layout_axis();
    ui_pop_padding();
    ui_pop_color();
    ui_pop_border();
    ui_pop_font();
    ui_pop_font_color();
    ui_pop_label_alignment();
    ui_pop_flags();

    ui_widget_group_end();
    global_ui->root_widget = 0;

    return widget_draw_command_list;
}
