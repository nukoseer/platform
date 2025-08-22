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
    void* permanent;
    size_t permanent_size;

    void* transient;
    size_t transient_size;
} memory_t;

typedef enum graphics_buffer_usage_t
{
    BUFFER_USAGE_DEFAULT = 0,
    BUFFER_USAGE_IMMUTABLE = 1,
    BUFFER_USAGE_DYNAMIC = 2,
} graphics_buffer_usage_t;

typedef enum graphics_buffer_bind_t
{
    BUFFER_BIND_VERTEX_BUFFER = 0x1L,
    BUFFER_BIND_INDEX_BUFFER = 0x2L,
    BUFFER_BIND_CONSTANT_BUFFER = 0x4L,
} graphics_buffer_bind_t;

typedef struct graphics_buffer_t
{
    void* platform;
} graphics_buffer_t;

/****************************************************************************************/
/* IMPORTANT: This functions are defined in platform layer and called from gamel layer. */
/****************************************************************************************/

#define graphics_create_buffer_function(name) graphics_buffer_t name(const void* data, size_t size, graphics_buffer_usage_t usage, graphics_buffer_bind_t bind_flags)
typedef graphics_create_buffer_function(graphics_create_buffer_f);

typedef struct graphics_t
{
    graphics_create_buffer_f* create_buffer;
} graphics_t;

typedef struct platform_t
{
    memory_t* memory;
    input_t* input;
    graphics_t* graphics;

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
