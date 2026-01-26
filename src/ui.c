
typedef struct ui_key_t
{
    u64 value;
} ui_key_t;

typedef struct ui_widget_t
{
    ui_key_t key;
    i32 x;
    i32 y;
    i32 width;
    i32 height;

    struct ui_widget_t* parent;
    struct ui_widget_t* next;
    struct ui_widget_t* prev;
} ui_widget_t;

typedef struct ui_widget_list_t
{
    ui_widget_t* first;
    ui_widget_t* last;
} ui_widget_list_t;

typedef struct ui_t
{
    memory_arena_t* arena;
    ui_widget_list_t widget_list[64];
} ui_t;

#define UI_WIDGET_INITIAL_HASH 0xCBF29CE484222325ULL

static ui_t global_ui;

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

static ui_key_t ui_get_key(ui_key_t parent_key, const char* data, u64 size)
{
    ui_key_t key = { 0 };
    u64 seed = UI_WIDGET_INITIAL_HASH;

    if (parent_key.value != 0)
    {
        seed = parent_key.value;
    }

    key.value = ui_hash(seed, data, size);

    return key;
}

static ui_key_t ui_get_key_from_string(ui_key_t parent_key, const char* string)
{
    u64 length = strlen(string);
    ui_key_t key = ui_get_key(parent_key, string, length);

    return key;
}

static inline bool ui_are_keys_equal(ui_key_t a_key, ui_key_t b_key)
{
    bool result = a_key.value == b_key.value;

    return result;
}

static ui_widget_t* ui_get_widget(ui_key_t key)
{
    ui_widget_t* widget = 0;
    u64 list_index = key.value & (array_count(global_ui.widget_list) - 1);
    ui_widget_list_t * widget_list = global_ui.widget_list + list_index;

    for (ui_widget_t* widget_iter = widget_list->first; widget_iter; widget_iter = widget_iter->next)
    {
        if (ui_are_keys_equal(widget_iter->key, key))
        {
            widget = widget_iter;
            break;
        }
    }

    if (!widget)
    {
        // TODO: Free list?
        widget = ma_push_size_zero(global_ui.arena, sizeof(ui_widget_t));
    }

    // TODO: parent?

    return widget;
}

static void ui_begin(memory_arena_t* memory_arena, i32 width, i32 height)
{
    if (!global_ui.arena)
    {
        global_ui.arena = ma_create_sub_arena(memory_arena, MIBIBYTES(4));
    }

    ui_key_t root_key = ui_get_key_from_string((ui_key_t){ 0 }, "root_widget");
    ui_widget_t* widget = ui_get_widget(root_key);
}

