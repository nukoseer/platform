#include "d3d11_gfx.h"

static d3d11_t global_d3d11 = { 0 };

static ID3D11Buffer* d3d11_create_buffer(ID3D11Device* device, const void* data, size_t size, u32 usage, u32 bind_flags)
{
    HRESULT result = S_OK;
    ID3D11Buffer* buffer = 0;

    D3D11_BUFFER_DESC desc =
    {
        .ByteWidth = (UINT)size,
        .Usage = usage,
        .BindFlags = bind_flags,
    };

    D3D11_SUBRESOURCE_DATA initial = { .pSysMem = data };
    result = ID3D11Device_CreateBuffer(device, &desc, &initial, &buffer);
    assert(SUCCEEDED(result) && "Failed to create buffer.");

    return buffer;
}

static d3d11_compile_t d3d11_compile(const char* buffer, size_t size, const char* entry_point, const char* target, u32 flags)
{
    HRESULT result = S_OK;
    ID3DBlob* error = 0;
    d3d11_compile_t compile = { 0 };

    result = D3DCompile(buffer, size, NULL, NULL, NULL, entry_point, target, flags, 0, &compile.code, &error);

    if (FAILED(result))
    {
        const char* error_message = ID3D10Blob_GetBufferPointer(error);
        OutputDebugString(error_message);
        assert(SUCCEEDED(result) && "Failed to compile shader.");
    }

    compile.data = ID3D10Blob_GetBufferPointer(compile.code);
    compile.size = ID3D10Blob_GetBufferSize(compile.code);

    return compile;
}

static void d3d11_compile_release(d3d11_compile_t* compile)
{
    assert(compile && "Failed to release compile blob.");
    
    ID3D10Blob_Release(compile->code);
    *compile = (d3d11_compile_t){ 0 };
}

static d3d11_input_layout_t d3d11_create_input_layout(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC* descs, size_t desc_count,
                                               const void* data, size_t size)
{
    HRESULT result = S_OK;
    d3d11_input_layout_t input_layout = { 0 };
    
    result = ID3D11Device_CreateInputLayout(device, descs, (UINT)desc_count, data, size, &input_layout.layout);
    assert(SUCCEEDED(result) && "Failed to create input layout.");

    return input_layout;
}

static inline ID3D11VertexShader* d3d11_create_vertex_shader(ID3D11Device* device, const void* buffer, size_t size)
{
    ID3D11VertexShader* shader = 0;

    ID3D11Device_CreateVertexShader(device, buffer, size, NULL, &shader);

    return shader;
}

static inline ID3D11PixelShader* d3d11_create_pixel_shader(ID3D11Device* device, const void* buffer, size_t size)
{
    ID3D11PixelShader* shader = 0;

    ID3D11Device_CreatePixelShader(device, buffer, size, NULL, &shader);

    return shader;
}

static d3d11_t* d3d11_init(void)
{
    HRESULT result = S_OK;
    d3d11_t* d3d11 = &global_d3d11;
    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
    // NOTE: To work with Direct2D, the Direct3D device that provides the IDXGISurface must be created with the D3D11_CREATE_DEVICE_BGRA_SUPPORT flag.
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0,
                               flags, feature_levels, array_count(feature_levels),
                               D3D11_SDK_VERSION, &d3d11->device, 0, &d3d11->context);

    assert(SUCCEEDED(result) && "Failed to create D3D11 device.");

#ifdef _DEBUG
    // NOTE: Enable debug layer for D3D11.
    {
        ID3D11InfoQueue* infoq = 0;
        ID3D11Device_QueryInterface(d3d11->device, &IID_ID3D11InfoQueue, (void**)&infoq);
        ID3D11InfoQueue_SetBreakOnSeverity(infoq, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        ID3D11InfoQueue_SetBreakOnSeverity(infoq, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        ID3D11InfoQueue_Release(infoq);
    }

    // NOTE: Enable debug layer for DXGI.
    {
        IDXGIInfoQueue* infoq = 0;
        result = DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (void**)&infoq);
        assert(SUCCEEDED(result) && "Failed to get DXGI debug interface.");

        IDXGIInfoQueue_SetBreakOnSeverity(infoq, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        IDXGIInfoQueue_SetBreakOnSeverity(infoq, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        IDXGIInfoQueue_Release(infoq);
    }
#endif

    return d3d11;
}

static IDXGISwapChain1* d3d11_create_swap_chain(HWND hwnd, d3d11_t* d3d11)
{
    HRESULT result = S_OK;
    IDXGISwapChain1* swap_chain = 0;

    // NOTE: Get DXGI device from D3D11 device.
    IDXGIDevice* dxgi_device = 0;
    result = ID3D11Device_QueryInterface(d3d11->device, &IID_IDXGIDevice, (void**)&dxgi_device);
    assert(SUCCEEDED(result) && "Failed to get DXGI device.");

    // NOTE: Get DXGI adapter from DXGI device.
    IDXGIAdapter* dxgi_adapter = 0;
    result = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
    assert(SUCCEEDED(result) && "Failed to get DXGI adapter.");

    // NOTE: Get DXGI factory from DXGI adapter.
    IDXGIFactory2* dxgi_factory = 0;
    result = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, (void**)&dxgi_factory);
    assert(SUCCEEDED(result) && "Failed to get DXGI factory.");

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc =
    {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,

        // NOTE: FLIP presentation model does not allow MSAA framebuffer
        // if you want MSAA then you'll need to render offscreen and manually
        // resolve to non-MSAA framebuffer.
        .SampleDesc = { 1, 0, },

        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        // NOTE: Double buffering.
        .BufferCount = 2,

        // NOTE: We don't want any automatic scaling of window content
        // this is supported only on FLIP presentation model.
        .Scaling = DXGI_SCALING_NONE,

        // NOTE: use more efficient FLIP presentation model
        // Windows 10 allows to use DXGI_SWAP_EFFECT_FLIP_DISCARD
        // for Windows 8 compatibility use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
        // for Windows 7 compatibility use DXGI_SWAP_EFFECT_DISCARD
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    result = IDXGIFactory2_CreateSwapChainForHwnd(dxgi_factory, (IUnknown*)d3d11->device, hwnd, &swap_chain_desc, 0, 0, &swap_chain);
    assert(SUCCEEDED(result) && "Failed to create swap chain.");

    result = IDXGIFactory_MakeWindowAssociation(dxgi_factory, hwnd, DXGI_MWA_NO_ALT_ENTER);
    assert(SUCCEEDED(result) && "Failed to disable alt enter behavior.");

    IDXGIFactory2_Release(dxgi_factory);
    IDXGIAdapter_Release(dxgi_adapter);
    IDXGIDevice_Release(dxgi_device);

    return swap_chain;
}
