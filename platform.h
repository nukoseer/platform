#pragma once

typedef enum key_modifier_t
{
    KEY_MODIFIER_CTRL = (1 << 0),
    KEY_MODIFIER_SHIFT = (1 << 1),
    KEY_MODIFIER_ALT = (1 << 2),
} key_modifier_t;

typedef enum key_action_t
{
    KEY_ACTION_NULL,

    KEY_ACTION_PRESS,
    KEY_ACTION_RELEASE,
    KEY_ACTION_MOUSE_SCROLL,

    KEY_ACTION_COUNT,
} key_action_t;

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

typedef struct key_input_t
{
    key_action_t action;
} key_input_t;

typedef struct input_t
{
    key_input_t keys[KEY_COUNT];
    key_modifier_t modifiers;
} input_t;

typedef struct memory_t
{
    u8* permanent;
    size_t permanent_size;

    u8* transient;
    size_t transient_size;
} memory_t;

typedef enum graphics_format_t
{
    FORMAT_R8G8B8A8_UNORM,
    FORMAT_R32G32_FLOAT,
    FORMAT_R32G32B32_FLOAT,
} graphics_format_t;

typedef enum graphics_buffer_usage_t
{
    BUFFER_USAGE_DEFAULT = 0,
    BUFFER_USAGE_IMMUTABLE = 1,
    BUFFER_USAGE_DYNAMIC = 2,
} graphics_buffer_usage_t;

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

typedef enum graphics_stage_t
{
    STAGE_NULL,
    STAGE_VERTEX_SHADER,
    STAGE_PIXEL_SHADER,
} graphics_stage_t;

// NOTE: We can add as we need.
typedef enum graphics_filter_t
{
    FILTER_MIN_MAG_MIP_POINT,
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

typedef struct graphics_buffer_desc_t
{
    void* data;
    usize size;
    graphics_buffer_usage_t usage;
    graphics_bind_t bind;
} graphics_buffer_desc_t;

typedef struct graphics_buffer_t
{
    usize platform;
} graphics_buffer_t;

typedef struct graphics_texture_2d_desc_t
{
    graphics_format_t format;
    graphics_bind_t bind;
    u32 width;
    u32 height;
} graphics_texture_2d_desc_t;

typedef struct graphics_texture_t
{
    usize platform;
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

typedef enum graphics_shader_type_t
{
    NULL_SHADER_TYPE,
    
    VERTEX_SHADER_TYPE,
    PIXEL_SHADER_TYPE,
    
    COUNT_SHADER_TYPE,
} graphics_shader_type_t;

typedef struct graphics_shader_desc_t
{
    const void* bytecode;
    usize bytecode_size;
    graphics_shader_type_t type;
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
} graphics_pipeline_desc_t;

typedef struct graphics_pipeline_t
{
    usize platform;
} graphics_pipeline_t;

typedef struct graphics_target_t
{
    usize platform;
} graphics_target_t;

typedef struct graphics_pass_desc_t
{
    bool clear_color;
    f32 clear_rgba[4];

    // TODO: clear_depth/stencil, use_depth/stencil
    // color_load/store, depth_load/store, stencil_load/store
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

/****************************************************************************************/
/* IMPORTANT: This functions are defined in platform layer and called from game layer. */
/****************************************************************************************/

#define graphics_create_buffer_function(name) graphics_buffer_t name(const graphics_buffer_desc_t* buffer_desc)
typedef graphics_create_buffer_function(graphics_create_buffer_f);

#define graphics_create_texture_2d_function(name) graphics_texture_t name(const graphics_texture_2d_desc_t* texture_2d_desc, const void* initial_data, u32 pitch)
typedef graphics_create_texture_2d_function(graphics_create_texture_2d_f);

#define graphics_create_sampler_function(name) graphics_sampler_t name(const graphics_sampler_desc_t* sampler_desc)
typedef graphics_create_sampler_function(graphics_create_sampler_f);

#define graphics_create_target_function(name) graphics_target_t name(const graphics_texture_t texture)
typedef graphics_create_target_function(graphics_create_target_f);

#define graphics_create_shader_function(name) graphics_shader_t name(const graphics_shader_desc_t* shader_desc)
typedef graphics_create_shader_function(graphics_create_shader_f);

#define graphics_create_program_function(name) graphics_program_t name(const graphics_program_desc_t* program_desc)
typedef graphics_create_program_function(graphics_create_program_f);

#define graphics_create_pipeline_function(name) graphics_pipeline_t name(const graphics_pipeline_desc_t* pipeline_desc)
typedef graphics_create_pipeline_function(graphics_create_pipeline_f);

#define graphics_set_vertex_buffer_function(name) void name(graphics_buffer_t vertex_buffer, u32 slot, u32 stride, u32 offset)
typedef graphics_set_vertex_buffer_function(graphics_set_vertex_buffer_f);

#define graphics_set_program_function(name) void name(graphics_program_t program)
typedef graphics_set_program_function(graphics_set_program_f);

#define graphics_set_pipeline_function(name) void name(graphics_pipeline_t pipeline)
typedef graphics_set_pipeline_function(graphics_set_pipeline_f);

#define graphics_set_samplers_function(name) void name(graphics_stage_t stage, const graphics_sampler_t* samplers, u32 count, u32 first_slot)
typedef graphics_set_samplers_function(graphics_set_samplers_f);

#define graphics_set_srvs_function(name) void name(graphics_stage_t stage, const graphics_texture_t* textures, u32 count, u32 first_slot)
typedef graphics_set_srvs_function(graphics_set_srvs_f);

#define graphics_get_backbuffer_target_function(name) graphics_target_t name(void)
typedef graphics_get_backbuffer_target_function(graphics_get_backbuffer_target_f);

#define graphics_get_target_size_function(name) void name(graphics_target_t target, u32* width, u32* height)
typedef graphics_get_target_size_function(graphics_get_target_size_f);

#define graphics_begin_pass_function(name) void name(graphics_target_t target, const graphics_pass_desc_t* pass_desc)
typedef graphics_begin_pass_function(graphics_begin_pass_f);

#define graphics_end_pass_function(name) void name(void)
typedef graphics_end_pass_function(graphics_end_pass_f);

#define graphics_draw_function(name) void name(graphics_topology_t topology, u32 vertex_count, u32 start_vertex)
typedef graphics_draw_function(graphics_draw_f);

typedef struct graphics_t
{
    union
    {
        struct functions
        {
            graphics_create_buffer_f* create_buffer;
            graphics_create_texture_2d_f* create_texture_2d;
            graphics_create_sampler_f* create_sampler;
            graphics_create_target_f* create_target;
            graphics_create_shader_f* create_shader;
            graphics_create_program_f* create_program;
            graphics_create_pipeline_f* create_pipeline;
            graphics_set_vertex_buffer_f* set_vertex_buffer;
            graphics_set_srvs_f* set_srvs;
            graphics_set_samplers_f* set_samplers;
            graphics_set_program_f* set_program;
            graphics_set_pipeline_f* set_pipeline;
            graphics_get_backbuffer_target_f* get_backbuffer_target;
            graphics_get_target_size_f* get_target_size;
            graphics_begin_pass_f* begin_pass;
            graphics_end_pass_f* end_pass;
            graphics_draw_f* draw;
        };

        // IMPORTANT: As far as I remember function pointers are not guaranteed
        // to be the same size as data pointers but what can I do?
        void* functions[sizeof(struct functions) / sizeof(void*)];
    };
} graphics_t;

typedef struct platform_t
{
    memory_t* memory;
    input_t* input;
    graphics_t* graphics;

    u32 width;
    u32 height;
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
