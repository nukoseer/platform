#define COBJMACROS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h> // snprintf

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <dxgidebug.h>
#include <timeapi.h>

#include "utils.h"
#include "d3d11_gfx.h"

#include "d3d11_gfx.c"

#pragma comment(lib, "user32")
#pragma comment(lib, "kernel32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "winmm")

#define PLATFORM_WINDOW_CLASS       "platform_window"
#define SERVICE_WINDOW_CLASS        "service_window"

#define SERVICE_WINDOW_CREATE_MSG   (WM_USER + 1)
#define SERVICE_WINDOW_CLOSE_MSG    (SERVICE_WINDOW_CREATE_MSG + 1)

typedef struct window_param_t
{
    DWORD     ex_style;
    LPCSTR   class_name;
    LPCSTR   window_name;
    DWORD     style;
    int       x;
    int       y;
    int       width;
    int       height;
    HWND      hwnd_parent;
    HMENU     menu;
    HINSTANCE instance;
    LPVOID    param;
} window_param_t;

typedef struct window_t
{
    HWND hwnd;
    WINDOWPLACEMENT placement;
    int width;
    int height;
    int current_width;
    int current_height;
    IDXGISwapChain1* swap_chain;
    d3d11_t* d3d11;
} window_t;

static HWND global_service_hwnd;
static DWORD global_main_thread_id;

// NOTE: This window procedure handles only 2 special messages to create and destroy windows.
// The thread (entry_point) which calls this window procedure owns the windows.
// entry_point's thread message queue will get all the messages for all of the created windows.
static LRESULT CALLBACK service_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (message)
    {
        case SERVICE_WINDOW_CREATE_MSG:
        {
            window_param_t* window_param = (window_param_t*)wparam;
            result = (LRESULT)CreateWindowEx(window_param->ex_style, window_param->class_name,
                                             window_param->window_name, window_param->style,
                                             window_param->x, window_param->y,
                                             window_param->width, window_param->height,
                                             window_param->hwnd_parent, window_param->menu,
                                             window_param->instance, window_param->param);
        } break;

        case SERVICE_WINDOW_CLOSE_MSG:
        {
            DestroyWindow((HWND)wparam);
        } break;

        default:
        {
            result = DefWindowProc(hwnd, message, wparam, lparam);
        } break;
    }

    return result;
}

// NOTE: This is the window procedure for our visible window.
// entry_point thread calls this routine (DispatchMessage)
// if hwnd matches. We still do not want to handle messages here
// because that could possibly cause race conditions. We
// pass them to our main_thread for processing.
static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_CLOSE:
        {
            PostThreadMessage(global_main_thread_id, message, (WPARAM)hwnd, lparam);
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_DESTROY:
        {
            PostThreadMessage(global_main_thread_id, message, wparam, lparam);
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
    bool result = 0;

    result = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

    if (!result)
    {
        result = !!SetProcessDPIAware();
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

// NOTE: We are able to create multiple windows but we do not support it.
// There should be only one window.
static void create_window(window_t* window, i32 width, i32 height)
{
    static WNDCLASSEX window_class = { 0 };

    if (!window_class.lpszClassName)
    {
        window_class = (WNDCLASSEX)
        {
            .cbSize = sizeof(window_class),
            .lpfnWndProc = window_proc,
            .hInstance = GetModuleHandle(0),
            .hIcon = LoadIcon(0, IDI_APPLICATION),
            .hCursor = LoadCursor(0, IDC_ARROW),
            .lpszClassName = PLATFORM_WINDOW_CLASS,
        };

        ATOM window_class_atom = RegisterClassEx(&window_class);
        assert(window_class_atom && "Failed to register window class.");
    }

    window_param_t window_param =
    {
        .ex_style = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP,
        .class_name = window_class.lpszClassName,
        .window_name = "Platform Window",
        .style = WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        .x = CW_USEDEFAULT,
        .y = CW_USEDEFAULT,
        .width = width,
        .height = height,
        .hwnd_parent = 0,
        .menu = 0,
        .instance = window_class.hInstance,
        .param = 0,
    };

    HWND hwnd = (HWND)SendMessage(global_service_hwnd, SERVICE_WINDOW_CREATE_MSG, (WPARAM)&window_param, 0);
    assert(hwnd && "Failed to create window.");

    window->hwnd = hwnd;
    window->width = width;
    window->height = height;
}

static void resize_back_buffer(window_t* window)
{
    HRESULT result = 0;

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
            window->d3d11->rt_view = 0;
        }

        if (window->width != 0 && window->height != 0)
        {
            result = IDXGISwapChain1_ResizeBuffers(window->swap_chain, 0, window->width, window->height, DXGI_FORMAT_UNKNOWN, 0);
            assert(SUCCEEDED(result) && "Failed to resize swap chain.");

            ID3D11Texture2D* back_buffer = 0;
            IDXGISwapChain1_GetBuffer(window->swap_chain, 0, &IID_ID3D11Texture2D, (void**)&back_buffer);
            ID3D11Device_CreateRenderTargetView(window->d3d11->device, (ID3D11Resource*)back_buffer, 0, &window->d3d11->rt_view);
            ID3D11Texture2D_Release(back_buffer);
        }

        window->current_width = window->width;
        window->current_height = window->height;
    }
}

static bool process_thread_message(MSG* message, window_t* window)
{
    bool quit = false;

    switch (message->message)
    {
        case WM_QUIT:
        {
            quit = true;
        } break;

        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;

        case WM_CLOSE:
        {
            SendMessage(global_service_hwnd, SERVICE_WINDOW_CLOSE_MSG, message->wParam, 0);
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            i32 key_code = (i32)message->wParam;
            // int was_down = (message->lParam & (1 << 30));
            bool is_down = !(message->lParam & (1 << 31));
            bool alt_is_down = (message->lParam & (1 << 29));

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

    return quit;
}

// NOTE: This is our real main thread we do everything here.
// Processing thread messages, rendering etc.
static DWORD WINAPI main_thread(void* param)
{
    HRESULT result = S_OK;
    window_t* window = (window_t*)param;

    set_min_timer_resolution();
    set_process_dpi_aware();
    create_window(window, CW_USEDEFAULT, CW_USEDEFAULT);

    window->d3d11 = d3d11_init();
    window->swap_chain = d3d11_create_swap_chain(window->hwnd, window->d3d11);

    typedef struct Vertex
    {
        f32 position[2];
        f32 color[3];
    } Vertex;

    Vertex vertex_data[] =
    {
        { { +0.00f, +0.66f }, { 1.0f, 0.0f, 0.0f, } },
        { { -0.33f, -0.33f }, { 0.0f, 1.0f, 0.0f, } },
        { { +0.33f, -0.33f }, { 0.0f, 0.0f, 1.0f, } },
    };

    d3d11_buffer_t vertex_buffer = d3d11_create_buffer(window->d3d11->device, vertex_data, sizeof(vertex_data),
                                                       D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);

    D3D11_INPUT_ELEMENT_DESC descs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

#if 1
    #include "d3d11_vshader.h"
    #include "d3d11_pshader.h"

    d3d11_shader_t vertex_shader = d3d11_create_shader(window->d3d11->device, d3d11_vshader, sizeof(d3d11_vshader), VERTEX_SHADER_TYPE);
    d3d11_shader_t pixel_shader = d3d11_create_shader(window->d3d11->device, d3d11_pshader, sizeof(d3d11_pshader), PIXEL_SHADER_TYPE);
    d3d11_input_layout_t input_layout = d3d11_create_input_layout(window->d3d11->device, descs, array_count(descs),
                                                                  d3d11_vshader, sizeof(d3d11_vshader));
#else

    const char hlsl[] =
    "#line " stringfy(__LINE__) "                               \n\n" // actual line number in this file for nicer error messages
    "                                                           \n"
    "struct VS_INPUT                                            \n"
    "{                                                          \n"
    "    float2 pos   : POSITION;                               \n" // these names must match D3D11_INPUT_ELEMENT_DESC array
    "    float3 color : COLOR;                                  \n"
    "};                                                         \n"
    "                                                           \n"
    "struct PS_INPUT                                            \n"
    "{                                                          \n"
    "    float4 pos   : SV_POSITION;                            \n" // these names do not matter, except SV_... ones
    "    float4 color : COLOR;                                  \n"
    "};                                                         \n"
    "                                                           \n"
    "PS_INPUT vs(VS_INPUT input)                                \n"
    "{                                                          \n"
    "    PS_INPUT output;                                       \n"
    "    output.pos = float4(input.pos, 0, 1);                  \n"
    "    output.color = float4(input.color, 1);                 \n"
    "    return output;                                         \n"
    "}                                                          \n"
    "                                                           \n"
    "float4 ps(PS_INPUT input) : SV_TARGET                      \n"
    "{                                                          \n"
    "    return input.color;                                    \n"
    "}                                                          \n";

    UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    d3d11_compile_t vertex_compile = d3d11_compile(hlsl, sizeof(hlsl), "vs", "vs_5_0", flags);
    d3d11_compile_t pixel_compile = d3d11_compile(hlsl, sizeof(hlsl), "ps", "ps_5_0", flags);

    d3d11_shader_t vertex_shader = d3d11_create_shader(window->d3d11->device, vertex_compile.data, vertex_compile.size, VERTEX_SHADER_TYPE);
    d3d11_shader_t pixel_shader = d3d11_create_shader(window->d3d11->device, pixel_compile.data, pixel_compile.size, PIXEL_SHADER_TYPE);
    d3d11_input_layout_t input_layout = d3d11_create_input_layout(window->d3d11->device, descs, array_count(descs),
                                                                  vertex_compile.data, vertex_compile.size);

    d3d11_compile_release(&vertex_compile);
    d3d11_compile_release(&pixel_compile);

#endif

    ID3D11RasterizerState* rasterizer_state;
    {
        // NOTE: Disable culling.
        // Meaning every triangle will be drawn regardless of
        // facing direction (clock-wise or counter clock-wise).
        D3D11_RASTERIZER_DESC desc =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_NONE,
            .DepthClipEnable = TRUE,
        };
        ID3D11Device_CreateRasterizerState(window->d3d11->device, &desc, &rasterizer_state);
    }

    u32 target_frame = 60;
    f32 target_secs_per_frame = 1.0f / target_frame;

    u64 time_last = get_ticks();
    bool quit = false;

    while (!quit)
    {
        // NOTE: These messages come from PostThreadMessage in window_proc.
        MSG message = { 0 };

        if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
        {
            quit = process_thread_message(&message, window);

            continue;
        }

        resize_back_buffer(window);

        if (window->d3d11->rt_view)
        {
            FLOAT color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            ID3D11DeviceContext_ClearRenderTargetView(window->d3d11->context, window->d3d11->rt_view, color);

            ID3D11DeviceContext_IASetInputLayout(window->d3d11->context, input_layout.layout);
            ID3D11DeviceContext_IASetPrimitiveTopology(window->d3d11->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            UINT offset = 0;
            UINT stride = sizeof(Vertex);
            ID3D11DeviceContext_IASetVertexBuffers(window->d3d11->context, 0, 1, &vertex_buffer.buffer, &stride, &offset);

            ID3D11DeviceContext_VSSetShader(window->d3d11->context, vertex_shader.vertex, 0, 0);

            // NOTE: Output viewport covering all client area of window.
            D3D11_VIEWPORT viewport =
            {
                .TopLeftX = 0,
                .TopLeftY = 0,
                .Width = (FLOAT)window->width,
                .Height = (FLOAT)window->height,
                .MinDepth = 0,
                .MaxDepth = 1,
            };

            ID3D11DeviceContext_RSSetViewports(window->d3d11->context, 1, &viewport);
            ID3D11DeviceContext_RSSetState(window->d3d11->context, rasterizer_state);

            ID3D11DeviceContext_PSSetShader(window->d3d11->context, pixel_shader.pixel, 0, 0);

            ID3D11DeviceContext_OMSetRenderTargets(window->d3d11->context, 1, &window->d3d11->rt_view, 0);

            ID3D11DeviceContext_Draw(window->d3d11->context, array_count(vertex_data), 0);
        }

        BOOL vsync = 1;
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
            assert(!"Failed to present swap chain.");
        }

        u64 time_passed = get_ticks();
        f32 time_passed_in_secs = get_secs_elapsed(time_last, time_passed);

        if (time_passed_in_secs < target_secs_per_frame)
        {
            DWORD sleep_time = (DWORD)((target_secs_per_frame - time_passed_in_secs) * 1000);

            if (sleep_time > 0)
            {
                Sleep(sleep_time);
            }

            do
            {
                time_passed_in_secs = get_secs_elapsed(time_passed, get_ticks());
            } while (time_passed_in_secs < target_secs_per_frame);
        }

        u64 time_end = get_ticks();
        f32 delta_time = get_secs_elapsed(time_last, time_end);
        time_last = time_end;

        char delta_time_str[32] = { 0 };

        if (snprintf(delta_time_str, sizeof(delta_time_str), "Frame time: %.1f ms", delta_time * 1000) > 0)
        {
            SetWindowText(window->hwnd, delta_time_str);
        }
    }

    ExitProcess(0);
}

static int entry_point(void)
{
    WNDCLASSEX service_window_class =
    {
        .cbSize = sizeof(service_window_class),
        .lpfnWndProc = service_window_proc,
        .hInstance = GetModuleHandle(0),
        .lpszClassName = SERVICE_WINDOW_CLASS,
    };

    ATOM service_window_class_atom = RegisterClassEx(&service_window_class);
    assert(service_window_class_atom && "Failed to register service window class.");

    global_service_hwnd = CreateWindowEx(0, service_window_class.lpszClassName, "Service Window", 0,
                                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                         0, 0, service_window_class.hInstance, 0);
    assert(global_service_hwnd && "Failed to create service window.");

    window_t window = { 0 };
    CloseHandle(CreateThread(0, 0, main_thread, &window, 0, &global_main_thread_id));

    for (;;)
    {
        MSG message = { 0 };

        GetMessage(&message, 0, 0, 0);
        TranslateMessage(&message);
        // NOTE: This thread owns the service window and the visible window,
        // so this message queue gets every message for both of the windows.
        // DispatchMessage checks message.hwnd and calls the correct window
        // procedure (service_window_proc or window_proc).
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
