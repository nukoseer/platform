#include "dwrite_c.h"
#include "d2d1_c.h"

#include "d2d1_gfx.h"

static d2d1_t global_d2d1 = { 0 };
static dwrite_t global_dwrite = { 0 };

static d2d1_t* d2d1_init(void)
{
    HRESULT result = S_OK;
    d2d1_t* d2d1 = &global_d2d1;
    dwrite_t* dwrite = &global_dwrite;

    d2d1->dwrite = dwrite;
    
    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&d2d1->dwrite->factory);
    assert(SUCCEEDED(result) && "[DWRITE] Failed to create factory.");

#ifdef _DEBUG
    D2D1_FACTORY_OPTIONS factory_options = { .debugLevel = D2D1_DEBUG_LEVEL_WARNING };
#else
    D2D1_FACTORY_OPTIONS factory_options = { .debugLevel = D2D1_DEBUG_LEVEL_NONE };
#endif
    result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, &factory_options, (void**)&d2d1->factory);
    assert(SUCCEEDED(result) && "[D2D1] Failed to create factory.");
   
    return d2d1;
}
