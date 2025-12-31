#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <initguid.h>
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#include <timeapi.h>

#include <stdio.h> // snprintf

#include "utils.h"
#include "platform.h"

#include "d3d11_gfx.h"
#include "d2d1_gfx.h"

#include "d3d11_gfx.c"
#include "d2d1_gfx.c"
#include "gfx.c"
#include "gfx_2d.c"

#include "io.c"

#pragma comment(lib, "user32")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "winmm")

#define PLATFORM_WINDOW_CLASS "platform_window"

__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#define fatal_system(x, message) do { if (!(x)) { fatal_error_system(message); } } while (0)
#define fatal(x, message) do { if (!(x)) { fatal_error(message); } } while (0)

typedef struct module_t
{
    HMODULE module;
    init_f* init;
    update_f* update;
    render_f* render;
} module_t;

typedef struct window_t
{
    HWND hwnd;
    WINDOWPLACEMENT placement;
    i32 width;
    i32 height;
    i32 current_width;
    i32 current_height;
    IDXGISwapChain1* swap_chain;
    d3d11_t* d3d11;
    d2d1_t* d2d1;
} window_t;

static DWORD global_main_thread_id;

static void fatal_error_system(const char* message) 
{ 
    char message_buffer[256] = { 0 };
    DWORD error = GetLastError(); 

    if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, error, 0,
        (LPTSTR)message_buffer, sizeof(message_buffer), 0))
    {
        MessageBox(0, message_buffer, message, MB_ICONEXCLAMATION);
    }
    else
    {
        MessageBox(0, "Error", message, MB_ICONEXCLAMATION);
    }

    ExitProcess(error); 
}

static void fatal_error(const char* message) 
{
    MessageBox(0, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

// NOTE: This is the window procedure for our window.  entry_point
// thread calls this routine via DispatchMessage. We do not want to
// handle messages here because that could possibly cause race
// conditions. We pass them to our main_thread for processing.
static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_DESTROY:
        case WM_CLOSE:
        {
            PostThreadMessage(global_main_thread_id, message, wparam, lparam);
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint_struct = { 0 };
            BeginPaint(hwnd, &paint_struct);
            EndPaint(hwnd, &paint_struct);
        } break;

        case WM_ERASEBKGND:
        {
            result = 1;
        } break;

        case WM_MENUCHAR:
        {
            // NOTE: Prevent bing sound.
            result = MAKELRESULT(0, MNC_CLOSE);
        } break;

        default:
        {
            result = DefWindowProc(hwnd, message, wparam, lparam);
        } break;
    }

    return result;
}

static void toggle_fullscreen(window_t* window)
{
    // NOTE: This follows Raymond Chen's prescription for fullscreen toggling, see:
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    HWND hwnd = window->hwnd;
    WINDOWPLACEMENT* window_placement = &window->placement;
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);

    if (style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitor_info = { sizeof(monitor_info) };
        if (GetWindowPlacement(hwnd, window_placement) &&
            GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
        {
            SetWindowLong(hwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, window_placement);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE |
                         SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

static bool set_process_dpi_aware(void)
{
    bool result = (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == TRUE);
    
    if (!result)
    {
        result = SetProcessDPIAware();
    }

    assert(result && "Failed to set dpi awareness.");

    return result;
}

static bool set_min_timer_resolution(void)
{
    bool result = (timeBeginPeriod(1) == TIMERR_NOERROR);

    assert(result && "Failed to set minimum timer resolution.");

    return result;
}

static inline u64 get_ticks(void)
{
    bool result = false;
    LARGE_INTEGER ticks = { 0 };

    result = QueryPerformanceCounter(&ticks);

    assert(result && "Failed to get query performance counter value.");

    return ticks.QuadPart;
}

static inline f32 get_secs_elapsed(u64 begin_ticks, u64 end_ticks)
{
    static LARGE_INTEGER qp_frequency;

    if (qp_frequency.QuadPart == 0)
    {
        QueryPerformanceFrequency(&qp_frequency);
    }

    f32 result = (f32)((f64)(end_ticks - begin_ticks) / qp_frequency.QuadPart);

    return result;
}

static module_t load_module(void)
{
    module_t result = { 0 };

    result.module = LoadLibrary("game");
    fatal_system(result.module, "[MODULE] Failed to load.");

    result.init = (init_f*)GetProcAddress(result.module, "init");
    fatal_system(result.init, "[MODULE] Failed to get init function.");
    
    result.update = (update_f*)GetProcAddress(result.module, "update");
    fatal_system(result.update, "[MODULE] Failed to get update function.");
    
    result.render = (render_f*)GetProcAddress(result.module, "render");
    fatal_system(result.render, "[MODULE] Failed to get render function.");

    return result;
}

static window_t* create_window(i32 width, i32 height)
{
    static window_t window = { 0 };

    if (!window.hwnd)
    {
        WNDCLASSEX window_class =
        {
            .cbSize = sizeof(window_class),
            .lpfnWndProc = window_proc,
            .hInstance = GetModuleHandle(0),
            .hIcon = LoadIcon(0, IDI_APPLICATION),
            .hCursor = LoadCursor(0, IDC_ARROW),
            .lpszClassName = PLATFORM_WINDOW_CLASS,
        };

        ATOM window_class_atom = RegisterClassEx(&window_class);
        fatal_system(window_class_atom, "[WINDOW] Failed to register window class.");

        HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP,
                                   window_class.lpszClassName, "Platform Window",
                                   WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                   CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                   0, 0, window_class.hInstance, 0);
        fatal_system(hwnd, "[WINDOW] Failed to create window.");

        window.hwnd = hwnd;
        window.width = width;
        window.height = height;
    }

    return &window;
}

static bool resize_back_buffer(window_t* window)
{
    bool resized = false;
    // NOTE: Get current size for window client area.
    RECT rect = { 0 };
    GetClientRect(window->hwnd, &rect);
    window->width = rect.right - rect.left;
    window->height = rect.bottom - rect.top;

    if (window->d3d11->rt_view == 0 || window->width != window->current_width || window->height != window->current_height)
    {
        if (window->d3d11->rt_view)
        {
            ID3D11DeviceContext_ClearState(window->d3d11->context);
            ID3D11RenderTargetView_Release(window->d3d11->rt_view);
            ID3D11DepthStencilView_Release(window->d3d11->ds_view);
            window->d3d11->rt_view = 0;
            window->d3d11->ds_view = 0;
        }

        if (window->d2d1->render_target)
        {
            ID2D1RenderTarget_Release(window->d2d1->render_target);
            window->d2d1->render_target = 0;
        }

        if (window->width != 0 && window->height != 0)
        {
            HRESULT result = IDXGISwapChain1_ResizeBuffers(window->swap_chain, 0, window->width, window->height, DXGI_FORMAT_UNKNOWN, 0);
            fatal_system(SUCCEEDED(result), "[DXGI] Failed to resize swap chain.");

            ID3D11Texture2D* back_buffer = 0;
            IDXGISwapChain1_GetBuffer(window->swap_chain, 0, &IID_ID3D11Texture2D, (void**)&back_buffer);
            
            D3D11_RENDER_TARGET_VIEW_DESC backbuffer_target =
            {
                .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            };
            ID3D11Device_CreateRenderTargetView(window->d3d11->device, (ID3D11Resource*)back_buffer, &backbuffer_target, &window->d3d11->rt_view);

            D3D11_TEXTURE2D_DESC depth_desc =
            {
                .Width = window->width,
                .Height = window->height,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
                .SampleDesc = { 1, 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            };

            ID3D11Texture2D* depth = 0;
            ID3D11Device_CreateTexture2D(window->d3d11->device, &depth_desc, 0, &depth);
            ID3D11Device_CreateDepthStencilView(window->d3d11->device, (ID3D11Resource*)depth, 0, &window->d3d11->ds_view);
            ID3D11Texture2D_Release(depth);

#if FONT_ENABLE
            IDXGISurface* dxgi_surface = 0;
            result = ID3D11Texture2D_QueryInterface(back_buffer, &IID_IDXGISurface, (void**)&dxgi_surface);
            fatal_system(SUCCEEDED(result), "[DXGI] Failed to get surface.");
            
            D2D1_RENDER_TARGET_PROPERTIES d2d_render_target_props = 
            {
                .type = D2D1_RENDER_TARGET_TYPE_DEFAULT,
                .pixelFormat =
                {
                    .format = DXGI_FORMAT_UNKNOWN,
                    .alphaMode = D2D1_ALPHA_MODE_IGNORE,
                },
                .dpiX = 0,
                .dpiY = 0,
                .usage = D2D1_RENDER_TARGET_USAGE_NONE,
                .minLevel = D2D1_FEATURE_LEVEL_DEFAULT,
            };
            result = ID2D1Factory_CreateDxgiSurfaceRenderTarget(window->d2d1->factory,
                                                                dxgi_surface,
                                                                &d2d_render_target_props,
                                                                &window->d2d1->render_target);
            fatal_system(SUCCEEDED(result), "[D2D1] Failed to create render target.");

            // NOTE: This looks like it works but I am not sure we really do anti-aliasing?
            ID2D1RenderTarget_SetAntialiasMode(window->d2d1->render_target, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            D2D1_ANTIALIAS_MODE antialias_mode = ID2D1RenderTarget_GetAntialiasMode(window->d2d1->render_target);
            fatal(antialias_mode == D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, "[D2D1] Failed to set anti-alias mode.");
            
            ID2D1RenderTarget_SetTextAntialiasMode(window->d2d1->render_target, D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            D2D1_TEXT_ANTIALIAS_MODE text_antialias_mode = ID2D1RenderTarget_GetTextAntialiasMode(window->d2d1->render_target);
            fatal(text_antialias_mode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE, "[D2D1] Failed to set text anti-alias mode.");

            IDXGISurface_Release(dxgi_surface);
#endif
            ID3D11Texture2D_Release(back_buffer);
        }

        window->d3d11->width = window->width;
        window->d3d11->height = window->height;
        window->current_width = window->width;
        window->current_height = window->height;
        resized = true;
    }

    return resized;
}

static bool process_thread_messages(window_t* window, input_t* input)
{
    bool quit = false;
    MSG message = { 0 };
    f32 mouse_z = 0.0f;

    // NOTE: These messages come from PostThreadMessage in window_proc.
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
            case WM_QUIT:
            {
                quit = true;
            } break;

            case WM_DESTROY:
            case WM_CLOSE:
            {
                PostQuitMessage(0);
            } break;

            case WM_LBUTTONDOWN:
            {
                SetCapture(window->hwnd);
                input->keys[KEY_MOUSE_LEFT].action = (message.wParam & MK_LBUTTON) ? KEY_ACTION_PRESS : KEY_ACTION_NULL;
            } break;

            case WM_LBUTTONUP:
            {
                input->keys[KEY_MOUSE_LEFT].action = (message.wParam & MK_LBUTTON) ? KEY_ACTION_RELEASE : KEY_ACTION_NULL;
                ReleaseCapture();
            } break;
            
            case WM_MOUSEWHEEL:
            {
                mouse_z = (f32)GET_WHEEL_DELTA_WPARAM(message.wParam) / (f32)WHEEL_DELTA;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                static key_t key_map[256];
                i32 key_code = (i32)message.wParam;
                // int was_down = (message->lParam & (1 << 30));
                bool is_down = !(message.lParam & (1 << 31));
                bool alt_is_down = (message.lParam & (1 << 29));
                key_t key = KEY_NULL;

                if (!key_map['A'])
                {
                    for (i32 number = '0', key = KEY_0; number <= '9'; ++number, ++key)
                    {
                        key_map[number] = (key_t)key;
                    }
            
                    for (i32 character = 'A', key = KEY_A; character <= 'Z'; ++character, ++key)
                    {
                        key_map[character] = (key_t)key;
                    }

                    for (i32 vk_f = VK_F1, key = KEY_F1; vk_f <= VK_F24; ++vk_f, ++key)
                    {
                        key_map[vk_f] = (key_t)key;
                    }

                    key_map[VK_TAB] = KEY_TAB;
                    key_map[VK_SPACE] = KEY_SPACE;
                    key_map[VK_RETURN] = KEY_ENTER;
                    key_map[VK_CONTROL] = KEY_CTRL;
                    key_map[VK_SHIFT] = KEY_SHIFT;
                    key_map[VK_MENU] = KEY_ALT;
                    key_map[VK_UP] = KEY_UP;
                    key_map[VK_LEFT] = KEY_LEFT;
                    key_map[VK_DOWN] = KEY_DOWN;
                    key_map[VK_RIGHT] = KEY_RIGHT;
                }

                if (key_code < array_count(key_map))
                {
                    key = key_map[key_code];
                }

                input->keys[key].action = is_down ? KEY_ACTION_PRESS : KEY_ACTION_RELEASE;
            
                if (key_code == VK_RETURN && is_down && alt_is_down)
                {
                    toggle_fullscreen(window);
                }
                else if (key_code == VK_ESCAPE && is_down)
                {
                    PostQuitMessage(0);
                }
            } break;
        }
    }

    bool alt_is_down = (input->keys[KEY_ALT].action == KEY_ACTION_PRESS);
    bool shift_is_down = (input->keys[KEY_SHIFT].action == KEY_ACTION_PRESS);
    bool ctrl_is_down = (input->keys[KEY_CTRL].action == KEY_ACTION_PRESS);
    
    input->modifiers = ((alt_is_down * KEY_MODIFIER_ALT) |
                        (shift_is_down * KEY_MODIFIER_SHIFT) |
                        (ctrl_is_down * KEY_MODIFIER_CTRL));

    static POINT prev_mouse_point = { 0 };
    POINT mouse_point = { 0 };
    GetCursorPos(&mouse_point);
    ScreenToClient(window->hwnd, &mouse_point);

    input->mouse_position = v3((f32)mouse_point.x, (f32)mouse_point.y, mouse_z);
    input->mouse_delta = v2((f32)mouse_point.x - (f32)prev_mouse_point.x, (f32)mouse_point.y - prev_mouse_point.y);

    if (input->mouse_position.x < 0.0f || input->mouse_position.x > (f32)window->width ||
        input->mouse_position.y < 0.0f || input->mouse_position.y > (f32)window->height)
    {
        input->mouse_position.x = clamp(0.0f, input->mouse_position.x, (f32)window->width);
        input->mouse_position.y = clamp(0.0f, input->mouse_position.y, (f32)window->height);
        input->keys[KEY_MOUSE_LEFT] = (key_input_t){ 0 };
        input->mouse_delta = (vec2){ 0 };
    }

    prev_mouse_point = mouse_point;
    
    return quit;
}

static inline void memory_init(memory_t* memory)
{
    memory->permanent_size = MIBIBYTES(256);
    memory->transient_size = GIBIBYTES(1);

#ifdef _DEBUG
    void* base_address = (void*)TIBIBYTES(64);
#else
    void* base_address = 0;
#endif
    
    size_t total_memory_size = memory->permanent_size + memory->transient_size;
    void* total_memory = VirtualAlloc(base_address, total_memory_size,
				      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    fatal(total_memory, "[MEMORY] Failed to allocate enough memory.");
    
    memory->permanent = total_memory;
    memory->transient = (u8*)total_memory + memory->permanent_size;
}

// NOTE: This is our real main thread we do everything here.
// Processing thread messages, rendering etc.
static DWORD WINAPI main_thread(void* param)
{
    HRESULT result = S_OK;
    window_t* window = (window_t*)param;

    window->d3d11 = d3d11_init();
    window->d2d1 = d2d1_init();
    window->swap_chain = d3d11_create_swap_chain(window->hwnd, window->d3d11);

    resize_back_buffer(window);

    memory_t memory = { 0 };
    memory_init(&memory);

    input_t inputs[2] = { 0 };
    input_t* new_input = inputs + 0;
    input_t* old_input = inputs + 1;

    graphics_t graphics =
    {
        .create_buffer = gfx_create_buffer,
        .create_texture_2d = gfx_create_texture_2d,
        .resolve_texture = gfx_resolve_texture,
        .create_sampler = gfx_create_sampler,
        .create_target = gfx_create_target,
        .create_shader = gfx_create_shader,
        .create_program = gfx_create_program,
        .create_pipeline = gfx_create_pipeline,
        .update_buffer = gfx_update_buffer,
        .is_valid_texture_2d = gfx_is_valid_texture_2d,
        .is_valid_target = gfx_is_valid_target,
        .delete_buffer = gfx_delete_buffer,
        .delete_texture_2d = gfx_delete_texture_2d,
        .delete_target = gfx_delete_target,
        .delete_shader = gfx_delete_shader,
        .delete_program = gfx_delete_program,
        .set_buffer = gfx_set_buffer,
        .set_vertex_buffer = gfx_set_vertex_buffer,
        .set_index_buffer = gfx_set_index_buffer,
        .set_srvs = gfx_set_srvs,
        .set_samplers = gfx_set_samplers,
        .set_program = gfx_set_program,
        .set_pipeline = gfx_set_pipeline,
        .get_backbuffer_target = gfx_get_backbuffer_target,
        .get_target_size = gfx_get_target_size,
        .begin_pass = gfx_begin_pass,
        .end_pass = gfx_end_pass,
        .draw = gfx_draw,
        .draw_indexed = gfx_draw_indexed,
        .draw_instanced = gfx_draw_instanced,
        .draw_indexed_instanced = gfx_draw_indexed_instanced,

        // NOTE: 2D functions for text rendering.

        .create_font = gfx_2d_create_font,
        .create_font_color = gfx_2d_create_font_color,
        .delete_font = gfx_2d_delete_font,
        .delete_font_color = gfx_2d_delete_font_color,
        .begin_draw = gfx_2d_begin_draw,
        .end_draw = gfx_2d_end_draw,
        .draw_text = gfx_2d_draw_text,
    };

    io_t io =
    {
        .read_file = io_read_file,
        .release_file_memory = io_release_file_memory,
    };

    platform_t platform =
    {
        .memory = &memory,
        .input = new_input,
        .graphics = &graphics,
        .io = &io,
        .width = window->width,
        .height = window->height,
    };

    for (u32 function_index = 0; function_index < array_count(graphics.functions); ++function_index)
    {
        void* function = graphics.functions[function_index];

        fatal(function, "[PLATFORM] Unassigned graphics function.");
    }

    for (u32 function_index = 0; function_index < array_count(graphics.functions_2d); ++function_index)
    {
        void* function = graphics.functions_2d[function_index];

        fatal(function, "[PLATFORM] Unassigned 2D graphics function.");
    }

    for (u32 function_index = 0; function_index < array_count(io.functions); ++function_index)
    {
        void* function = io.functions[function_index];

        fatal(function, "[PLATFORM] Unassigned io function.");
    }

    module_t module = load_module();
    module.init(&platform);
    
    u32 target_frame = 60;
    f32 target_secs_per_frame = 1.0f / target_frame;

    u64 time_last = get_ticks();
    bool quit = false;

    while (!quit)
    {
        for (u32 key = KEY_NULL; key < KEY_COUNT; ++key)
        {
            new_input->keys[key] = (old_input->keys[key].action == KEY_ACTION_PRESS) ? old_input->keys[key] : (key_input_t){ 0 };
        }
        
        quit = process_thread_messages(window, new_input);

        platform.input = new_input;

        platform.resized = resize_back_buffer(window);

        platform.width = window->width;
        platform.height = window->height;
        
        module.update(&platform);

        module.render(&platform);

        BOOL vsync = 0;
        result = IDXGISwapChain1_Present(window->swap_chain, vsync ? 1 : 0, 0);

        if (result == DXGI_STATUS_OCCLUDED)
        {
            if (vsync)
            {
                Sleep(10);
            }
        }
        else if (FAILED(result))
        {
            assert(!"[DXGI] Failed to present swap chain.");
        }

        input_t* temp_input = new_input;
        new_input = old_input;
        old_input = temp_input;

        u64 time_end = get_ticks();
        f32 elapsed_secs = get_secs_elapsed(time_last, time_end);
        f32 remaining = target_secs_per_frame - elapsed_secs;

        if (remaining > 0)
        {
            // NOTE: Leave a small margin for jitter.
            if (remaining > 0.002f)
            {
                DWORD sleep_ms = (DWORD)((remaining - 0.001f) * 1000.0f);

                if (sleep_ms)
                {
                    Sleep(sleep_ms);
                }
            }

            do
            {
                time_end = get_ticks();
                elapsed_secs = get_secs_elapsed(time_last, time_end);
            } while (elapsed_secs < target_secs_per_frame);
        }

        platform.delta_time = get_secs_elapsed(time_last, time_end);
        time_last = time_end;
    }

    ExitProcess(0);
}

static i32 entry_point(void)
{
    set_min_timer_resolution();
    set_process_dpi_aware();
    window_t* window = create_window(CW_USEDEFAULT, CW_USEDEFAULT);
    
    CloseHandle(CreateThread(0, 0, main_thread, window, 0, &global_main_thread_id));

    for (;;)
    {
        MSG message = { 0 };

        GetMessage(&message, 0, 0, 0);
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return 0;
}

#ifndef _DEBUG
int WinMainCRTStartup(void)
{
    return entry_point();
}
#else
int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int show_cmd)
{
    (void)instance, (void)prev_instance; (void)cmdline; (void)show_cmd;
    return entry_point();
}
#endif
