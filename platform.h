#pragma once

typedef enum key_modifier_t
{
    KEY_MODIFIER_CTRL = (1 << 0),
    KEY_MODIFIER_SHIFT = (1 << 1),
    KEY_MODIFIER_ALT = (1 << 2),
} key_modifier_t;

typedef enum key_type_t
{
    KEY_TYPE_NULL,

    KEY_TYPE_PRESS,
    KEY_TYPE_HOLD,
    KEY_TYPE_RELEASE,
    KEY_TYPE_MOUSE_SCROLL,

    KEY_TYPE_COUNT,
} key_type_t;

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

typedef struct key_action_t
{
    key_t key;
    key_type_t type;
    key_modifier_t modifier;
} key_action_t;

typedef struct input_t
{
    key_action_t key_actions[KEY_COUNT];
} input_t;

typedef struct memory_t
{
    void* permanent;
    size_t permanent_size;

    void* transient;
    size_t transient_size;
} memory_t;

typedef struct platform_t
{
    memory_t memory;
    input_t input;

    f32 delta_time;
} platform_t;

#define init_function(name) void name(platform_t* platform)
typedef init_function(init_f);

#define update_function(name) void name(platform_t* platform)
typedef update_function(update_f);

#define render_function(name) void name(platform_t* platform)
typedef render_function(render_f);
