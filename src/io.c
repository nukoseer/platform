
static io_release_file_memory_function(io_release_file_memory)
{
    assert(memory && "[IO] File memory pointer is null.");
    VirtualFree(memory, 0, MEM_RELEASE);
}

static io_read_file_function(io_read_file)
{
    io_file_read_result_t io_file_read_result = { 0 };

    HANDLE file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    assert(file_handle != INVALID_HANDLE_VALUE && "[IO] Failed to open the file.");

    if (file_handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size = { 0 };

        if (GetFileSizeEx(file_handle, &size))
        {
            u64 file_size = size.QuadPart;
            u8* data = VirtualAlloc(0, file_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            assert(data && "[IO] Failed to allocate memory for file.");

            if (data)
            {
                u64 offset = 0;

                while (offset < file_size)
                {
                    DWORD bytes_read = 0;
                    u64 read_size = file_size - offset;

                    if (read_size > 0xFFFFFFFF)
                    {
                        read_size = 0xFFFFFFFF;
                    }

                    BOOL result = ReadFile(file_handle, data + offset, (DWORD)read_size, &bytes_read, 0);
                    assert(result && "[IO] Failed to read file.");

                    if (!result)
                    {
                        io_release_file_memory(data);
                        data = 0;
                        file_size = 0;
                        break;
                    }
                    
                    offset += read_size;
                }

                CloseHandle(file_handle);
                io_file_read_result.data = data;
                io_file_read_result.size = file_size;
            }
        }
    }

    return io_file_read_result;
}
