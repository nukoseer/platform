
typedef struct platform_thread_pool_entry_t
{
    thread_pool_entry_f* function;
    void* parameter;
} platform_thread_pool_entry_t;

typedef struct platform_thread_pool_queue_t
{
    volatile u32 read_index;
    volatile u32 write_index;
    volatile u32 completion_goal;
    volatile u32 completion_count;
    HANDLE semaphore_handle;
    platform_thread_pool_entry_t entries[32];
} platform_thread_pool_queue_t;

static bool thread_pool_execute_entry(platform_thread_pool_queue_t* thread_pool_queue)
{
    bool sleep = false;
    
    u32 read_index = thread_pool_queue->read_index;
    u32 write_index = thread_pool_queue->write_index;
    u32 new_read_index = (read_index + 1) % array_count(thread_pool_queue->entries);

    if (read_index != write_index)
    {
        u32 read = InterlockedCompareExchange((volatile LONG*)&thread_pool_queue->read_index, new_read_index, read_index);

        if (read == read_index)
        {
            platform_thread_pool_entry_t thread_pool_entry = thread_pool_queue->entries[read];
            thread_pool_entry.function(thread_pool_entry.parameter);
            InterlockedIncrement((volatile LONG*)&thread_pool_queue->completion_count);
        }
    }
    else
    {
        sleep = true;
    }

    return sleep;
}

thread_pool_add_entry_function(thread_pool_add_entry)
{
    platform_thread_pool_queue_t* thread_pool_queue = (platform_thread_pool_queue_t*)queue.platform;

    u32 write_index = thread_pool_queue->write_index;
    u32 new_write_index = (write_index + 1) % array_count(thread_pool_queue->entries);
    assert(new_write_index != thread_pool_queue->read_index);
    platform_thread_pool_entry_t* thread_pool_entry = thread_pool_queue->entries + write_index;
    thread_pool_entry->function = function;
    thread_pool_entry->parameter = parameter;
    thread_pool_queue->completion_goal++;
    _WriteBarrier();
    thread_pool_queue->write_index = new_write_index;
    ReleaseSemaphore(thread_pool_queue->semaphore_handle, 1, 0);
}

thread_pool_complete_all_entries_function(thread_pool_complete_all_entries)
{
    platform_thread_pool_queue_t* thread_pool_queue = (platform_thread_pool_queue_t*)queue.platform;

    while (thread_pool_queue->completion_goal != thread_pool_queue->completion_count)
    {
        thread_pool_execute_entry(thread_pool_queue);
    }

    thread_pool_queue->completion_goal = 0;
    thread_pool_queue->completion_count = 0;
}

static DWORD WINAPI thread_pool_thread(void* param)
{
    platform_thread_pool_queue_t* thread_pool_queue = (platform_thread_pool_queue_t*)param;
    
    for (;;)
    {
        if (thread_pool_execute_entry(thread_pool_queue))
        {
            WaitForSingleObjectEx(thread_pool_queue->semaphore_handle, INFINITE, false);
        }
    }
}

static void thread_pool_init(platform_thread_pool_queue_t* thread_pool_queue, u32 thread_count)
{
    memset(thread_pool_queue, 0, sizeof(platform_thread_pool_queue_t));

    thread_pool_queue->semaphore_handle = CreateSemaphore(0, 0, thread_count, 0);

    for (u32 thread_index = 0; thread_index < thread_count; ++thread_index)
    {
        CloseHandle(CreateThread(0, 0, thread_pool_thread, thread_pool_queue, 0, 0));
    }
}
