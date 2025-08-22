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

    result = IDWriteFactory_CreateTextFormat(d2d1->dwrite->factory, L"Georgia", 0,
                                             DWRITE_FONT_WEIGHT_REGULAR,
                                             DWRITE_FONT_STYLE_NORMAL,
                                             DWRITE_FONT_STRETCH_NORMAL,
                                             36.0f, L"en-us", &d2d1->dwrite->text_format);
    assert(SUCCEEDED(result) && "[DWRITE] Failed to create text format.");

    // result = IDWriteTextFormat_SetTextAlignment(d2d1->dwrite->text_format, DWRITE_TEXT_ALIGNMENT_CENTER);
    // assert(SUCCEEDED(result) && "[DWRITE] Failed to set text alignment.");

    // result = IDWriteTextFormat_SetParagraphAlignment(d2d1->dwrite->text_format, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    // assert(SUCCEEDED(result) && "[DWRITE] Failed to set paragraph alignment.");

    result = IDWriteTextFormat_SetWordWrapping(d2d1->dwrite->text_format, DWRITE_WORD_WRAPPING_NO_WRAP);
    assert(SUCCEEDED(result) && "[DWRITE] Failed to set word wrapping.");

#ifdef _DEBUG
    D2D1_FACTORY_OPTIONS factory_options = { .debugLevel = D2D1_DEBUG_LEVEL_WARNING };
#else
    D2D1_FACTORY_OPTIONS factory_options = { .debugLevel = D2D1_DEBUG_LEVEL_NONE };
#endif
    result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, &factory_options, (void**)&d2d1->factory);
    assert(SUCCEEDED(result) && "[D2D1] Failed to create factory.");
   
    return d2d1;
}
