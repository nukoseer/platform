
typedef struct ui_widget_key
{
    u64 value;
} ui_widget_key;

typedef enum ui_widget_size_kind_t
{
    UI_WIDGET_SIZE_PIXEL,
    UI_WIDGET_SIZE_PARENT,
    
    UI_WIDGET_SIZE_COUNT,
} ui_widget_size_kind_t;

typedef enum ui_widget_axis_t
{
    UI_WIDGET_AXIS_X,
    UI_WIDGET_AXIS_Y,

    UI_WIDGET_AXIS_COUNT,
} ui_widget_axis_t;

typedef struct ui_widget_size_t
{
    ui_widget_size_kind_t kind;
    f32 value;
} ui_widget_size_t;

typedef struct ui_widget_position_t
{
    union
    {
        struct
        {
            f32 x;
            f32 y;
        };

        f32 xy[UI_WIDGET_AXIS_COUNT];
    };
} ui_widget_position_t;

typedef struct ui_widget_rect_t
{
    f32 x, y;
    f32 width, height;
} ui_widget_rect_t;

typedef struct ui_widget_desc_t
{
    ui_widget_size_t size[UI_WIDGET_AXIS_COUNT];
    ui_widget_axis_t child_axis;
} ui_widget_desc_t;

typedef struct ui_widget_list_t
{
    struct ui_widget_t* first;
    struct ui_widget_t* last;
} ui_widget_list_t;

typedef struct ui_widget_t
{
    const char* name;
    ui_widget_key key;
    ui_widget_position_t position;
    ui_widget_size_t size[UI_WIDGET_AXIS_COUNT];
    ui_widget_axis_t child_axis;

    struct ui_widget_t* parent;
    struct ui_widget_t* hash_next;
    struct ui_widget_t* hash_prev;
    
    ui_widget_list_t child_list;
    struct ui_widget_t* child_next;
    struct ui_widget_t* child_prev;

    f32 fixed_size[UI_WIDGET_AXIS_COUNT];
    ui_widget_rect_t rect;
} ui_widget_t;

typedef struct ui_stack_t
{
    ui_widget_t* parent[32];
    u32 parent_count;
} ui_stack_t;

typedef struct ui_t
{
    memory_arena_t* arena;
    ui_widget_t* root_widget;
    ui_widget_list_t widget_list_array[64];

    ui_stack_t stack;

    ui_widget_t* free_widgets;
} ui_t;

#define UI_WIDGET_INITIAL_HASH 0xCBF29CE484222325ULL

static ui_t* global_ui;

static u64 ui_hash(u64 seed, const char* data, u64 size)
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

static inline void ui_push_parent_widget(ui_widget_t* widget)
{
    assert(global_ui->stack.parent_count < array_count(global_ui->stack.parent) && "[UI] Cannot push to parent stack.");

    global_ui->stack.parent[global_ui->stack.parent_count++] = widget;
}

static inline void ui_pop_parent_widget(void)
{
    assert(global_ui->stack.parent_count > 0 && "[UI] Cannot pop from parent stack.");
    global_ui->stack.parent[global_ui->stack.parent_count--] = 0;
}

static inline ui_widget_t* ui_top_parent_widget(void)
{
    ui_widget_t* widget = 0;
    
    if (global_ui->stack.parent_count > 0)
    {
        widget = global_ui->stack.parent[global_ui->stack.parent_count - 1];
    }

    return widget;
}

static inline ui_widget_size_t ui_widget_pixel_size(f32 pixel_value)
{
    ui_widget_size_t widget_size =
    {
        .kind = UI_WIDGET_SIZE_PIXEL,
        .value = pixel_value,
    };

    return widget_size;
}

static inline ui_widget_size_t ui_widget_parent_size(f32 parent_size_percentage)
{
    assert(parent_size_percentage >= 0.0f && parent_size_percentage <= 1.0f && "[UI] Invalid parent size percentage.");

    ui_widget_size_t widget_size =
    {
        .kind = UI_WIDGET_SIZE_PARENT,
        .value = parent_size_percentage,
    };

    return widget_size;
}

static ui_widget_key ui_widget_get_key(ui_widget_key parent_widget_key, const char* data, u64 size)
{
    ui_widget_key widget_key = { 0 };
    u64 seed = UI_WIDGET_INITIAL_HASH;

    if (parent_widget_key.value != 0)
    {
        seed = parent_widget_key.value;
    }

    widget_key.value = ui_hash(seed, data, size);

    return widget_key;
}

static ui_widget_key ui_widget_get_key_from_string(ui_widget_key parent_widget_key, const char* string)
{
    u64 length = strlen(string);
    ui_widget_key key = ui_widget_get_key(parent_widget_key, string, length);

    return key;
}

static inline bool ui_widget_keys_are_equal(ui_widget_key a_widget_key, ui_widget_key b_widget_key)
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

static ui_widget_t* ui_widget_get(ui_widget_key widget_key)
{
    ui_widget_t* widget = 0;
    u64 list_index = widget_key.value & (array_count(global_ui->widget_list_array) - 1);
    ui_widget_list_t* widget_list = global_ui->widget_list_array + list_index;

    for (ui_widget_t* widget_iter = widget_list->first; widget_iter; widget_iter = widget_iter->hash_next)
    {
        if (ui_widget_keys_are_equal(widget_iter->key, widget_key))
        {
            widget = widget_iter;
            break;
        }
    }

    if (!widget)
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

        memset(widget, 0, sizeof(ui_widget_t));
        ui_widget_list_hash_insert_back(widget_list, widget);
    }

    assert(widget && "[UI] Invalid widget.");

    widget->parent = widget->child_list.first = widget->child_list.last = widget->child_next = widget->child_prev = 0;
    widget->rect = (ui_widget_rect_t){ 0 };
    widget->key = widget_key;

    // NOTE: The root widget does not have parent.
    ui_widget_t* parent_widget = ui_top_parent_widget();
    widget->parent = parent_widget;

    if (parent_widget)
    {
        ui_widget_list_child_insert_back(&parent_widget->child_list, widget);
    }

    return widget;
}

static void ui_widget_group_begin(const char* widget_name, f32 x, f32 y, ui_widget_desc_t widget_desc)
{
    ui_widget_key widget_key = ui_widget_get_key_from_string(ui_top_parent_widget()->key, widget_name);
    ui_widget_t* widget = ui_widget_get(widget_key);

    assert(widget && "[UI] Couldn't get widget.");

    widget->name = widget_name;
    widget->position.x = x;
    widget->position.y = y;
    
    for (i32 axis = 0; axis < UI_WIDGET_AXIS_COUNT; ++axis)
    {
        widget->size[axis] = widget_desc.size[axis];
        
    }

    widget->child_axis = widget_desc.child_axis;

    // TODO: Right now, it is not supported to position widgets inside a widget group except it is directly under the root widget.
    if (widget->parent != global_ui->root_widget)
    {
        // TODO: We can also silently ignore position parameters but for now keep the assert.
        assert(!"[UI] Only widgets directly under the root widget can specify position.");
        widget->position.x = 0.0f;
        widget->position.y = 0.0f;
    }

    ui_push_parent_widget(widget);
}

static inline ui_widget_group_end(void)
{
    ui_pop_parent_widget();
}

static void ui_widget(const char* widget_name, ui_widget_desc_t widget_desc)
{
    ui_widget_key widget_key = ui_widget_get_key_from_string(ui_top_parent_widget()->key, widget_name);
    ui_widget_t* widget = ui_widget_get(widget_key);

    assert(widget && "[UI] Couldn't get widget.");

    widget->name = widget_name;
    widget->position.x = 0.0f;
    widget->position.y = 0.0f;

    for (i32 axis = 0; axis < UI_WIDGET_AXIS_COUNT; ++axis)
    {
        widget->size[axis] = widget_desc.size[axis];
    }
}

static void ui_widget_calculate_pixel_sizes(ui_widget_t* root_widget, ui_widget_axis_t axis)
{
    if (root_widget->size[axis].kind == UI_WIDGET_SIZE_PIXEL)
    {
        root_widget->fixed_size[axis] = root_widget->size[axis].value;
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_widget_calculate_pixel_sizes(child_widget, axis);
    }
}

static void ui_widget_calculate_parent_dependent_sizes(ui_widget_t* root_widget, ui_widget_axis_t axis)
{
    ui_widget_t* found_parent_widget = 0;
    
    if (root_widget->size[axis].kind == UI_WIDGET_SIZE_PARENT)
    {
        for (ui_widget_t* parent_widget = root_widget->parent; parent_widget; parent_widget = parent_widget->parent)
        {
            if (parent_widget->size[axis].kind == UI_WIDGET_SIZE_PARENT ||
                parent_widget->size[axis].kind == UI_WIDGET_SIZE_PIXEL)
            {
                found_parent_widget = parent_widget;
                break;
            }
        }

        if (found_parent_widget)
        {
            root_widget->fixed_size[axis] = root_widget->size[axis].value * found_parent_widget->fixed_size[axis];
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_widget_calculate_parent_dependent_sizes(child_widget, axis);
    }
}

static void ui_widget_calculate_size_violations(ui_widget_t* root_widget, ui_widget_axis_t axis)
{
    if (root_widget->child_axis != axis)
    {
        for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
        {
            f32 child_size = child_widget->fixed_size[axis];
            f32 violation_amount = child_size - root_widget->fixed_size[axis];
            f32 fix_amount = clamp(0.0f, violation_amount, child_size);

            if (fix_amount > 0.0f)
            {
                child_widget->fixed_size[axis] -= fix_amount;
            }
        }
    }

    if (root_widget->child_axis == axis)
    {
        f32 total_child_size = 0.0f;
        
        for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
        {
            total_child_size += child_widget->fixed_size[axis];
        }

        f32 violation_amount = total_child_size - root_widget->fixed_size[axis];
        f32 fix_percentage = violation_amount / total_child_size;

        if (violation_amount > 0.0f)
        {
            for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
            {
                f32 fix_amount = fix_percentage * child_widget->fixed_size[axis];
                child_widget->fixed_size[axis] -= fix_amount;
            }
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_widget_calculate_size_violations(child_widget, axis);
    }
}

static void ui_widget_calculate_layout(ui_widget_t* root_widget, ui_widget_axis_t axis)
{
    f32 layout_at = 0.0f;
    
    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        child_widget->position.xy[axis] += layout_at;

        if (root_widget->child_axis == axis)
        {
            layout_at += child_widget->fixed_size[axis];
        }
        
        if (axis == UI_WIDGET_AXIS_X)
        {
            child_widget->rect.x = root_widget->rect.x + child_widget->position.xy[axis];
            child_widget->rect.width = child_widget->fixed_size[axis];
        }
        else if (axis == UI_WIDGET_AXIS_Y)
        {
            child_widget->rect.y = root_widget->rect.y + child_widget->position.xy[axis];
            child_widget->rect.height = child_widget->fixed_size[axis];
        }
        else
        {
            assert(!"[UI] Invalid axis.");
        }
    }

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_widget_calculate_layout(child_widget, axis);
    }
}

static void ui_widget_print_sizes(ui_widget_t* root_widget)
{
    fprintf(stderr, "[%s] fixed size: (%f, %f) rect: (%f, %f, %f, %f)\n",
            root_widget->name,
            root_widget->fixed_size[0], root_widget->fixed_size[1],
            root_widget->rect.x, root_widget->rect.y,
            root_widget->rect.width, root_widget->rect.height);

    for (ui_widget_t* child_widget = root_widget->child_list.first; child_widget; child_widget = child_widget->child_next)
    {
        ui_widget_print_sizes(child_widget);
    }
}

static void ui_begin(memory_arena_t* memory_arena, f32 width, f32 height)
{
    if (!global_ui)
    {
        memory_arena_t* ui_arena = ma_create_sub_arena(memory_arena, MIBIBYTES(4));
        global_ui = (ui_t*)ma_push_size_zero(ui_arena, sizeof(ui_t));
        global_ui->arena = ui_arena;
    }

    // NOTE: This is for making sure that ui_end() is called at the end of previous frame.
    assert(global_ui->stack.parent_count == 0 && "[UI] Invalid parent stack count.");

    ui_widget_key root_widget_key = ui_widget_get_key_from_string((ui_widget_key){ 0 }, "root_widget");
    ui_widget_t* root_widget = ui_widget_get(root_widget_key);

    root_widget->name = "root_widget";
    root_widget->size[0] = ui_widget_pixel_size(width);
    root_widget->size[1] = ui_widget_pixel_size(height);
    root_widget->child_axis = UI_WIDGET_AXIS_Y;

    global_ui->root_widget = root_widget;
    
    ui_push_parent_widget(root_widget);
}

static void ui_end(void)
{
    // NOTE: Pop the root widget.
    assert(global_ui->stack.parent_count == 1 && "[UI] Invalid parent stack count.");
    ui_pop_parent_widget();

    for (i32 axis = 0; axis < UI_WIDGET_AXIS_COUNT; ++axis)
    {
        ui_widget_calculate_pixel_sizes(global_ui->root_widget, axis);
        ui_widget_calculate_parent_dependent_sizes(global_ui->root_widget, axis);
        ui_widget_calculate_size_violations(global_ui->root_widget, axis);
        ui_widget_calculate_layout(global_ui->root_widget, axis);
    }

    ui_widget_print_sizes(global_ui->root_widget);

    // fprintf(stderr, "\n\r[UI] Arena remaining size: %zu", ma_get_remaining_size(global_ui->arena));
}
