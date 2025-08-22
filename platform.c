#define COBJMACROS
#define WIN32_LEAN_AND_MEAN

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
    int width;
    int height;
    int current_width;
    int current_height;
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
    MessageBox(0, "Error", message, MB_ICONEXCLAMATION);
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
        case WM_DESTROY:
        case WM_CLOSE:
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
    bool result = (SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == S_OK);

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

        if (window->d2d1->solid_color_brush)
        {
            ID2D1SolidColorBrush_Release(window->d2d1->solid_color_brush);
            window->d2d1->solid_color_brush = 0;
        }

        if (window->d2d1->render_target)
        {
            ID2D1RenderTarget_Release(window->d2d1->render_target);
            window->d2d1->render_target = 0;
        }

        if (window->width != 0 && window->height != 0)
        {
            result = IDXGISwapChain1_ResizeBuffers(window->swap_chain, 0, window->width, window->height, DXGI_FORMAT_UNKNOWN, 0);
            fatal_system(SUCCEEDED(result), "[DXGI] Failed to resize swap chain.");

            ID3D11Texture2D* back_buffer = 0;
            IDXGISwapChain1_GetBuffer(window->swap_chain, 0, &IID_ID3D11Texture2D, (void**)&back_buffer);
            ID3D11Device_CreateRenderTargetView(window->d3d11->device, (ID3D11Resource*)back_buffer, 0, &window->d3d11->rt_view);

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
            ID2D1RenderTarget_SetTextAntialiasMode(window->d2d1->render_target, D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
            D2D1_TEXT_ANTIALIAS_MODE text_antialias_mode = ID2D1RenderTarget_GetTextAntialiasMode(window->d2d1->render_target);
            fatal(text_antialias_mode == D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE, "[D2D1] Failed to set text anti-alias mode.");

            D2D1_COLOR_F d2d1_color = { 1.0f, 1.0f, 1.0f, 1.0f };
            result = ID2D1RenderTarget_CreateSolidColorBrush(window->d2d1->render_target,
                                                             &d2d1_color,
                                                             0,
                                                             &window->d2d1->solid_color_brush);
            fatal_system(SUCCEEDED(result), "[D2D1] Failed to create solid color brush.");

            IDXGISurface_Release(dxgi_surface);
            ID3D11Texture2D_Release(back_buffer);
        }

        window->current_width = window->width;
        window->current_height = window->height;
    }
}

static bool process_thread_messages(window_t* window, input_t* input)
{
    bool quit = false;
    MSG message = { 0 };

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

    ID3D11Buffer* vertex_buffer = d3d11_create_buffer(window->d3d11->device, vertex_data, sizeof(vertex_data),
                                                      D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER);

    D3D11_INPUT_ELEMENT_DESC descs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

#if 1
    #include "d3d11_vshader.h"
    #include "d3d11_pshader.h"

    d3d11_shader_t vertex_shader = d3d11_create_shader(window->d3d11->device, d3d11_vshader, sizeof(d3d11_vshader), D3D11_VERTEX_SHADER_TYPE);
    d3d11_shader_t pixel_shader = d3d11_create_shader(window->d3d11->device, d3d11_pshader, sizeof(d3d11_pshader), D3D11_PIXEL_SHADER_TYPE);
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

    memory_t memory = { 0 };
    memory_init(&memory);

    input_t inputs[2] = { 0 };
    input_t* new_input = inputs + 0;
    input_t* old_input = inputs + 1;

    graphics_t graphics =
    {
        .create_buffer = gfx_create_buffer,
    };

    platform_t platform =
    {
        .memory = &memory,
        .input = new_input,
        .graphics = &graphics,
    };

    module_t module = load_module();
    module.init(&platform);
    
    u32 target_frame = 60;
    f32 target_secs_per_frame = 1.0f / target_frame;

    u64 time_last = get_ticks();
    bool quit = false;

    while (!quit)
    {
        quit = process_thread_messages(window, new_input);

        resize_back_buffer(window);

        platform.input = new_input;
        
        module.update(&platform);

        module.render(&platform);

        if (window->d3d11->rt_view)
        {
            FLOAT color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            ID3D11DeviceContext_ClearRenderTargetView(window->d3d11->context, window->d3d11->rt_view, color);

            ID3D11DeviceContext_IASetInputLayout(window->d3d11->context, input_layout.layout);
            ID3D11DeviceContext_IASetPrimitiveTopology(window->d3d11->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            UINT offset = 0;
            UINT stride = sizeof(Vertex);
            ID3D11DeviceContext_IASetVertexBuffers(window->d3d11->context, 0, 1, &vertex_buffer, &stride, &offset);

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

            ID2D1RenderTarget_BeginDraw(window->d2d1->render_target);

            IDWriteTextLayout* text_layout = 0;
            WCHAR text[] = L"Hello";
            // TODO: IDWriteFactory_CreateTextLayout should not be here I guess?
            IDWriteFactory_CreateTextLayout(window->d2d1->dwrite->factory,
                                            text,                                            
                                            sizeof(text) / 2 - 1,
                                            window->d2d1->dwrite->text_format,
                                            (FLOAT)window->width,
                                            (FLOAT)window->height,
                                            &text_layout);
            DWRITE_TEXT_METRICS text_metrics = { 0 };
            IDWriteTextLayout_GetMetrics(text_layout, &text_metrics);

            FLOAT left = (window->width) / 2.0f - (text_metrics.width / 2.0f); 
            FLOAT top = (window->height) / 2.0f - (text_metrics.height / 2.0f);
            D2D1_RECT_F layout =
            {
                .left = left,
                .top = top,
                .right = left + text_metrics.width,
                .bottom = top + text_metrics.height
            };
            D2D1_ROUNDED_RECT rounded_rect = {
                .rect = layout,
                .radiusX = 2.0f, .radiusY = 2.0f,
            };
            ID2D1RenderTarget_DrawRoundedRectangle(window->d2d1->render_target, &rounded_rect, (ID2D1Brush*)window->d2d1->solid_color_brush, 1.0f, 0);
            ID2D1RenderTarget_DrawText(window->d2d1->render_target, text, sizeof(text) / 2 - 1, window->d2d1->dwrite->text_format,
                                       &layout, (ID2D1Brush*)window->d2d1->solid_color_brush, D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT, DWRITE_MEASURING_MODE_NATURAL);
            
            ID2D1RenderTarget_EndDraw(window->d2d1->render_target, 0, 0);

            // TODO: IDWriteTextLayout_Release should not be here I guess?
            IDWriteTextLayout_Release(text_layout);
        }

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
            assert(!"Failed to present swap chain.");
        }

        input_t* temp_input = new_input;
        new_input = old_input;
        old_input = temp_input;

        for (u32 key = KEY_NULL; key < KEY_COUNT; ++key)
        {
            new_input->keys[key] = (old_input->keys[key].action == KEY_ACTION_PRESS) ? old_input->keys[key] : (key_input_t){ 0 };
        }

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

        char delta_time_str[32] = { 0 };

        if (snprintf(delta_time_str, sizeof(delta_time_str), "%.1f ms", platform.delta_time * 1000) > 0)
        {
            SetWindowText(window->hwnd, delta_time_str);
        }
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
