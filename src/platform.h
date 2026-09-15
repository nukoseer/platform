#pragma once

#include "maths.h"

#define FONT_ENABLE 1

typedef enum key_modifier_t
{
    KEY_MODIFIER_CTRL = (1 << 0),
    KEY_MODIFIER_SHIFT = (1 << 1),
    KEY_MODIFIER_ALT = (1 << 2),
} key_modifier_t;

typedef enum key_t
{
    KEY_NULL,
    
    KEY_RESERVED_0,
    KEY_ESC,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_SPACE,
    KEY_ENTER,
    KEY_CTRL,
    KEY_SHIFT,
    KEY_ALT,
    KEY_UP,
    KEY_LEFT,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_MOUSE_LEFT,
    KEY_MOUSE_MIDDLE,
    KEY_MOUSE_RIGHT,
    
    KEY_COUNT
} key_t;

typedef enum input_event_kind_t
{
    INPUT_EVENT_NULL,
    
    INPUT_EVENT_TEXT,
    INPUT_EVENT_KEY_PRESS,
    INPUT_EVENT_KEY_RELEASE,
    INPUT_EVENT_MOUSE_PRESS,
    INPUT_EVENT_MOUSE_RELEASE,
    INPUT_EVENT_MOUSE_WHEEL,
    
    INPUT_EVENT_COUNT,
} input_event_kind_t;

typedef struct input_event_t
{
    input_event_kind_t kind;
    bool consumed;
    key_modifier_t modifiers;

    union
    {
        key_t key;
        u32 codepoint;
        f32 wheel;
    };
} input_event_t;

typedef enum input_owner_t
{
    INPUT_OWNER_NULL,

    INPUT_OWNER_UI,
    INPUT_OWNER_SCENE,
} input_owner_t;

typedef struct input_t
{
    input_event_t events[256];
    u32 event_count;

    input_owner_t owner;
    
    vec2 mouse_position;
    f32 mouse_wheel;
    vec2 mouse_delta;
    bool key_down[KEY_COUNT];
} input_t;

static inline void input_set_owner(input_t* input, input_owner_t owner)
{
    input->owner = owner;
}

static inline bool input_is_owner(input_t* input, input_owner_t owner)
{
    bool result = input->owner == owner;

    return result;
}

static inline bool input_is_key_down(const input_t* input, key_t key)
{
    bool result = input->key_down[key];

    return result;
}

static inline input_event_t* input_find_event(const input_t* input, input_event_kind_t kind, usize value, key_modifier_t modifiers)
{
    input_event_t* result = 0;
    
    for (u32 i = 0; i < input->event_count; ++i)
    {
        const input_event_t* event = input->events + i;

        if (event->consumed)
        {
            continue;
        }

        if (event->kind != kind)
        {
            continue;
        }

        if (event->kind == INPUT_EVENT_KEY_PRESS || event->kind == INPUT_EVENT_KEY_RELEASE ||
            event->kind == INPUT_EVENT_MOUSE_PRESS || event->kind == INPUT_EVENT_MOUSE_RELEASE)
        {
            if (event->key != (u32)value || event->modifiers != modifiers)
            {
                continue;
            }
        }

        result = (input_event_t*)event;
        break;
    }

    return result;
}

static inline bool input_is_key_pressed(const input_t* input, key_t key, key_modifier_t modifiers)
{
    bool result = input_find_event(input, INPUT_EVENT_KEY_PRESS, key, modifiers) != 0;

    return result;
}

static inline bool input_is_key_released(const input_t* input, key_t key, key_modifier_t modifiers)
{
    bool result = input_find_event(input, INPUT_EVENT_KEY_RELEASE, key, modifiers) != 0;

    return result;
}

static inline bool input_is_mouse_pressed(const input_t* input, key_t key)
{
    bool result = input_find_event(input, INPUT_EVENT_MOUSE_PRESS, key, 0) != 0;

    return result;
}

static inline bool input_is_mouse_released(const input_t* input, key_t key)
{
    bool result = input_find_event(input, INPUT_EVENT_MOUSE_RELEASE, key, 0) != 0;

    return result;
}


static inline bool input_is_mouse_wheeled(const input_t* input)
{
    bool result = input_find_event(input, INPUT_EVENT_MOUSE_WHEEL, 0, 0) != 0;

    return result;
}

static inline bool input_consume_matching_event(input_t* input, input_event_kind_t kind, key_t key, key_modifier_t modifiers)
{
    bool result = false;
    input_event_t* event = input_find_event(input, kind, key, modifiers);

    if (event)
    {
        event->consumed = true;
        result = true;
    }

    return result;
}

static inline void input_consume_event(input_t* input, input_event_t* event)
{
    event->consumed = true;
}

static inline bool input_consume_key_press(input_t* input, key_t key, key_modifier_t modifiers)
{
    bool result = input_consume_matching_event(input, INPUT_EVENT_KEY_PRESS, key, modifiers);
    
    return result;
}

static inline bool input_consume_key_release(input_t* input, key_t key, key_modifier_t modifiers)
{
    bool result = input_consume_matching_event(input, INPUT_EVENT_KEY_RELEASE, key, modifiers);
    
    return result;
}

static inline bool input_consume_mouse_press(input_t* input, key_t key)
{
    bool result = input_consume_matching_event(input, INPUT_EVENT_MOUSE_PRESS, key, 0);
    
    return result;
}

static inline bool input_consume_mouse_release(input_t* input, key_t key)
{
    bool result = input_consume_matching_event(input, INPUT_EVENT_MOUSE_RELEASE, key, 0);
    
    return result;
}

static inline bool input_consume_mouse_wheel(input_t* input)
{
    bool result = input_consume_matching_event(input, INPUT_EVENT_MOUSE_WHEEL, 0, 0);
    
    return result;
}

typedef struct memory_t
{
    u8* permanent;
    usize permanent_size;

    u8* transient;
    usize transient_size;
} memory_t;

typedef enum graphics_format_t
{
    FORMAT_R8_UNORM,
    FORMAT_R8G8B8A8_UNORM,
    FORMAT_R8G8B8A8_UNORM_SRGB,
    FORMAT_R16G16B16A16_FLOAT,
    FORMAT_R32_FLOAT,
    FORMAT_R32G32_FLOAT,
    FORMAT_R32G32B32_FLOAT,
    FORMAT_R32G32B32A32_FLOAT,
    FORMAT_R16_UINT,
    FORMAT_R32_UINT,
    FORMAT_D24_UNORM_S8_UINT,
    FORMAT_D32_FLOAT,
} graphics_format_t;

typedef enum graphics_usage_t
{
    USAGE_DEFAULT = 0,
    USAGE_IMMUTABLE = 1,
    USAGE_DYNAMIC = 2,
} graphics_usage_t;

typedef enum graphics_bind_t
{
    BIND_NULL,
    
    BIND_VERTEX_BUFFER = (1 << 0),
    BIND_INDEX_BUFFER = (1 << 1),
    BIND_CONSTANT_BUFFER = (1 << 2),
    BIND_SHADER_RESOURCE = (1 << 3),
    BIND_RENDER_TARGET = (1 << 4),
    BIND_DEPTH_STENCIL = (1 << 5),
} graphics_bind_t;

typedef enum graphics_misc_t
{
    MISC_NULL,
    
    MISC_GENERATE_MIPS = (1 << 0),
    MISC_TEXTURE_CUBE = (1 << 1),
} graphics_misc_t;

typedef enum graphics_stage_t
{
    STAGE_NULL,
    STAGE_VERTEX_SHADER = (1 << 0),
    STAGE_PIXEL_SHADER = (1 << 1),
    STAGE_GEOMETRY_SHADER = (1 << 2),
} graphics_stage_t;

// NOTE: We can add as we need.
typedef enum graphics_filter_t
{
    FILTER_MIN_MAG_MIP_POINT,
    FILTER_MIN_MAG_MIP_LINEAR,
} graphics_filter_t;

typedef enum graphics_texture_address_t
{
    TEXTURE_ADDRESS_NULL,
    TEXTURE_ADDRESS_WRAP,
    TEXTURE_ADDRESS_MIRROR,
    TEXTURE_ADDRESS_CLAMP,
    TEXTURE_ADDRESS_BORDER,
    TEXTURE_ADDRESS_MIRROR_ONCE
} graphics_texture_address_t;

typedef enum graphics_blend_t
{
    BLEND_NULL,
    BLEND_ALPHA,
    BLEND_ADDITIVE,
    BLEND_PRE_MULTIPLIED,
} graphics_blend_t;

typedef struct graphics_buffer_desc_t
{
    const void* data;
    u32 size;
    graphics_usage_t usage;
    graphics_bind_t bind;
    graphics_format_t index_format;
} graphics_buffer_desc_t;

typedef struct graphics_buffer_t
{
    u64 platform;
    u32 size;
} graphics_buffer_t;

typedef struct graphics_texture_desc_t
{
    graphics_format_t format;
    graphics_bind_t bind;
    u32 width;
    u32 height;
    u32 sample_count;
    u32 array_size;
    graphics_misc_t misc;
} graphics_texture_desc_t;

typedef struct graphics_texture_t
{
    u64 platform;
    u32 width;
    u32 height;
} graphics_texture_t;

typedef struct graphics_sampler_desc_t
{
    graphics_filter_t filter;
    graphics_texture_address_t address_u;
    graphics_texture_address_t address_v;
    graphics_texture_address_t address_w;
} graphics_sampler_desc_t;

typedef struct graphics_sampler_t
{
    usize platform;
} graphics_sampler_t;

typedef struct graphics_shader_desc_t
{
    const void* bytecode;
    usize bytecode_size;
    graphics_stage_t stage;
} graphics_shader_desc_t;

typedef struct graphics_shader_t
{
    usize platform;
} graphics_shader_t;

typedef struct graphics_vertex_attribute_t
{
    const char* semantic;
    graphics_format_t format;
    u32 offset;
    u8 index;
    u8 slot;
    u8 per_instance;
    u8 step_rate;
} graphics_vertex_attribute_t;

typedef struct graphics_program_desc_t
{
    graphics_shader_t vertex_shader;
    graphics_shader_t pixel_shader;
    graphics_shader_t geometry_shader;
    graphics_vertex_attribute_t* attributes;
    usize attribute_count;
} graphics_program_desc_t;

typedef struct graphics_program_t
{
    usize platform;
} graphics_program_t;

typedef struct graphics_pipeline_desc_t
{
    // NOTE: Rasterizer State.
    bool cull;
    bool wireframe;
    bool depth_test;
    bool depth_write;
    graphics_blend_t blend;
} graphics_pipeline_desc_t;

typedef struct graphics_pipeline_t
{
    usize platform;
} graphics_pipeline_t;

typedef struct graphics_target_desc_t
{
    graphics_texture_t color;
    graphics_texture_t depth;
} graphics_target_desc_t;

typedef struct graphics_target_t
{
    u64 platform;
    u32 width;
    u32 height;
} graphics_target_t;

typedef struct graphics_pass_desc_t
{
    bool clear_color;
    vec4 clear_rgba;

    // TODO: clear_depth/stencil, use_depth/stencil
    // color_load/store, depth_load/store, stencil_load/store
    bool clear_depth;
    f32 clear_depth_value;
} graphics_pass_desc_t;

typedef enum graphics_topology_t
{
    TOPOLOGY_POINT_LIST,
    TOPOLOGY_LINE_LIST,
    TOPOLOGY_LINE_STRIP,
    TOPOLOGY_TRIANGLE_LIST,
    TOPOLOGY_TRIANGLE_STRIP,
    TOPOLOGY_LINE_LIST_ADJ,
    TOPOLOGY_TRIANGLE_LIST_ADJ,
} graphics_topology_t;

typedef struct graphics_t
{
    union
    {
        // NOTE: Graphics functions.
        struct graphics_functions_t
        {
            graphics_buffer_t   (*create_buffer)(const graphics_buffer_desc_t* buffer_desc);
            graphics_texture_t  (*create_texture)(const graphics_texture_desc_t* texture_desc, const void** initial_data, const u32* pitches);
            void                (*resolve_texture)(graphics_texture_t dst_texture, graphics_texture_t src_texture);
            void                (*copy_texture)(graphics_texture_t dst_texture, graphics_texture_t src_texture);
            graphics_texture_t  (*texture_from_target)(graphics_target_t target);
            graphics_sampler_t  (*create_sampler)(const graphics_sampler_desc_t* sampler_desc);
            graphics_target_t   (*create_target)(const graphics_target_desc_t* target_desc);
            graphics_shader_t   (*create_shader)(const graphics_shader_desc_t* shader_desc);
            graphics_program_t  (*create_program)(const graphics_program_desc_t* program_desc);
            graphics_pipeline_t (*create_pipeline)(const graphics_pipeline_desc_t* pipeline_desc);
            void                (*update_buffer)(graphics_buffer_t buffer, const void* src, u32 offset, u32 size);
            bool                (*is_valid_texture)(graphics_texture_t texture);
            bool                (*is_valid_target)(graphics_target_t target);
            void                (*delete_buffer)(graphics_buffer_t buffer);
            void                (*delete_texture)(graphics_texture_t texture);
            void                (*delete_target)(graphics_target_t target);
            void                (*delete_shader)(graphics_shader_t shader);
            void                (*delete_program)(graphics_program_t program);
            void                (*set_buffer)(graphics_buffer_t buffer, graphics_stage_t stage, u32 slot, u32 stride, u32 offset);
            void                (*set_vertex_buffer)(graphics_buffer_t buffer, u32 slot, u32 stride, u32 offset);
            void                (*set_index_buffer)(graphics_buffer_t buffer, u32 offset);
            void                (*set_program)(graphics_program_t program);
            void                (*set_pipeline)(graphics_pipeline_t pipeline);
            void                (*set_samplers)(graphics_stage_t stage, const graphics_sampler_t* samplers, u32 count, u32 first_slot);
            void                (*set_srvs)(graphics_stage_t stage, const graphics_texture_t* textures, u32 count, u32 first_slot);
            graphics_target_t   (*get_backbuffer_target)(void);
            void                (*get_target_size)(graphics_target_t target, u32* width, u32* height);
            void                (*set_viewport)(f32 x, f32 y, f32 width, f32 height);
            void                (*begin_pass)(graphics_target_t target, const graphics_pass_desc_t* pass_desc);
            void                (*end_pass)(void);
            void                (*draw)(graphics_topology_t topology, u32 vertex_count, u32 start_vertex);
            void                (*draw_indexed)(graphics_topology_t topology, u32 index_count, u32 start_index, u32 base_vertex);
            void                (*draw_instanced)(graphics_topology_t topology, u32 vertex_count, u32 instance_count, u32 start_vertex, u32 start_instance);
            void                (*draw_indexed_instanced)(graphics_topology_t topology, u32 index_count, u32 instance_count, u32 start_index, u32 base_vertex, u32 start_instance);
        };

        // IMPORTANT: As far as I remember function pointers are not guaranteed
        // to be the same size as data pointers but what can I do?
        void* functions[sizeof(struct graphics_functions_t) / sizeof(void*)];
    };
} graphics_t;

typedef struct io_file_read_result_t
{
    u8* data;
    size_t size;
} io_file_read_result_t;

typedef struct io_t
{
    union
    {
        struct io_functions_t
        {
            io_file_read_result_t (*read_file)(const char* file_name);
            void                  (*release_file_memory)(u8* memory);
        };

        // IMPORTANT: As far as I remember function pointers are not guaranteed
        // to be the same size as data pointers but what can I do?
        void* functions[sizeof(struct io_functions_t) / sizeof(void*)];
    };
} io_t;

typedef struct font_handle_t
{
    u64 platform;
    f32 point_size;
    f32 pixel_size;
} font_handle_t;

typedef struct font_info_t
{
    f32 ascent;
    f32 descent;
    f32 line_gap;
    f32 line_advance;
    f32 pixel_per_em;
    f32 pixel_per_design_unit;
} font_info_t;

typedef struct glyph_info_t
{
    u16 x, y;
    u16 width, height;
    i16 offset_x, offset_y;
    f32 advance;
} glyph_info_t;

typedef struct font_t
{
    union
    {
        struct font_functions_t
        {
            font_handle_t      (*create)(const char* font_path, f32 point_size);
            void               (*delete)(font_handle_t font);
            graphics_texture_t (*get_atlas)(font_handle_t font);
            font_info_t        (*get_font_info)(font_handle_t font);
            f32                (*get_point_size)(font_handle_t font);
            f32                (*get_pixel_size)(font_handle_t font);
            f32                (*get_text_width)(font_handle_t font, const char* text, size_t text_length);
            f32                (*get_line_height)(font_handle_t font);
            glyph_info_t       (*get_glyph_info_from_codepoint)(font_handle_t font, u32 codepoint);
        };

        // IMPORTANT: As far as I remember function pointers are not guaranteed
        // to be the same size as data pointers but what can I do?
        void* functions[sizeof(struct font_functions_t) / sizeof(void*)];
    };
} font_t;

typedef struct thread_pool_queue_t
{
    u64 platform;
} thread_pool_queue_t;

#define thread_pool_entry_function(name) void name(void* parameter)
typedef thread_pool_entry_function(thread_pool_entry_f);

typedef struct thread_pool_t
{
    thread_pool_queue_t queue;
    
    union
    {
        struct thread_pool_functions_t
        {
            void (*add_entry)(thread_pool_queue_t queue, thread_pool_entry_f* function, void* parameter);
            void (*complete_all_entries)(thread_pool_queue_t queue);
        };

        // IMPORTANT: As far as I remember function pointers are not guaranteed
        // to be the same size as data pointers but what can I do?
        void* functions[sizeof(struct thread_pool_functions_t) / sizeof(void*)];
    };
} thread_pool_t;

typedef struct platform_t
{
    memory_t* memory;
    input_t* input;
    graphics_t* graphics;
    font_t* font;
    io_t* io;
    thread_pool_t* thread_pool;

    u32 width;
    u32 height;
    bool resized;
    f32 delta_time;
} platform_t;

/***************************************************************************************/
/* IMPORTANT: This functions are defined in game layer and called from platform layer. */
/***************************************************************************************/

#define init_function(name) void name(platform_t* platform)
typedef init_function(init_f);

#define update_function(name) void name(platform_t* platform)
typedef update_function(update_f);

#define render_function(name) void name(platform_t* platform)
typedef render_function(render_f);
