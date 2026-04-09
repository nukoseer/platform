#include "utils.h"

#define ma_push_struct(memory_arena, type) (type*)ma_push_size(memory_arena, sizeof(type))
#define ma_push_struct_zero(memory_arena, type) (type*)ma_push_size_zero(memory_arena, sizeof(type))

typedef struct memory_arena_t
{
    u8* base;
    usize used;
    usize size;
} memory_arena_t;

typedef struct memory_arena_span_t
{
    memory_arena_t* memory_arena;
    usize used;
} memory_arena_span_t;

static memory_arena_t* ma_initialize(u8* memory, usize size)
{
    assert(memory && "[MEMORY ARENA] Invalid memory during initialization.");
    
    memory_arena_t* memory_arena = (memory_arena_t*)memory;
    memory_arena->base = memory + sizeof(memory_arena_t);
    memory_arena->used = 0;
    memory_arena->size = size - sizeof(memory_arena_t);

    return memory_arena;
}

static inline void ma_reset(memory_arena_t* memory_arena)
{
    assert(memory_arena && "[MEMORY ARENA] Invalid memory arena during reset.");
    memory_arena->used = 0;
}

static inline usize ma_get_remaining_size(memory_arena_t* memory_arena)
{
    usize result = memory_arena->size - memory_arena->used;

    return result;
}

static void* ma_push_size(memory_arena_t* memory_arena, usize size)
{
    assert(memory_arena && "[MEMORY ARENA] Invalid memory arena during push size.");
    assert(memory_arena->used + size <= memory_arena->size);

    void* memory = memory_arena->base + memory_arena->used;
    memory_arena->used += size;

    return memory;
}

static void* ma_push_size_zero(memory_arena_t* memory_arena, usize size)
{
    assert(memory_arena && "[MEMORY ARENA] Invalid memory arena during push size.");
    assert(memory_arena->used + size <= memory_arena->size);

    void* memory = memory_arena->base + memory_arena->used;
    memory_arena->used += size;

    memset(memory, 0, size);

    return memory;
}

static memory_arena_t* ma_create_sub_arena(memory_arena_t* base_memory_arena, usize size)
{
    memory_arena_t* sub_arena = ma_push_size(base_memory_arena, size);
    sub_arena->base = (u8*)sub_arena + sizeof(memory_arena_t);
    sub_arena->used = 0;
    sub_arena->size = size - sizeof(memory_arena_t);

    return sub_arena;
}

static memory_arena_span_t ma_span_begin(memory_arena_t* memory_arena)
{
    memory_arena_span_t memory_arena_span =
    {
        .memory_arena = memory_arena,
        .used = memory_arena->used,
    };

    return memory_arena_span;
}

static void ma_span_end(memory_arena_span_t memory_arena_span)
{
    assert(memory_arena_span.memory_arena->used >= memory_arena_span.used);
    memory_arena_span.memory_arena->used = memory_arena_span.used;
}
