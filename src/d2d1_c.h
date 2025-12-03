#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-flexible-array"
#pragma clang diagnostic ignored "-Wmicrosoft-enum-value"
#endif

#include <d2dbasetypes.h>

/*******************************************************************************************************************/
/* IMPORTANT: This is the C compatible version of d2d1.h. All of the enums, types and functions directly copied. */
/*******************************************************************************************************************/

// NOTE: Enums.

typedef enum D2D1_FACTORY_TYPE
{
    D2D1_FACTORY_TYPE_SINGLE_THREADED = 0,
    D2D1_FACTORY_TYPE_MULTI_THREADED = 1,
    D2D1_FACTORY_TYPE_FORCE_DWORD = 0xffffffff
} D2D1_FACTORY_TYPE;

typedef enum D2D1_DEBUG_LEVEL
{
    D2D1_DEBUG_LEVEL_NONE = 0,
    D2D1_DEBUG_LEVEL_ERROR = 1,
    D2D1_DEBUG_LEVEL_WARNING = 2,
    D2D1_DEBUG_LEVEL_INFORMATION = 3,
    D2D1_DEBUG_LEVEL_FORCE_DWORD = 0xffffffff
} D2D1_DEBUG_LEVEL;

typedef enum D2D1_CAP_STYLE
{
    D2D1_CAP_STYLE_FLAT = 0,
    D2D1_CAP_STYLE_SQUARE = 1,
    D2D1_CAP_STYLE_ROUND = 2,
    D2D1_CAP_STYLE_TRIANGLE = 3,
    D2D1_CAP_STYLE_FORCE_DWORD = 0xffffffff
} D2D1_CAP_STYLE;

typedef enum D2D1_LINE_JOIN
{
    D2D1_LINE_JOIN_MITER = 0,
    D2D1_LINE_JOIN_BEVEL = 1,
    D2D1_LINE_JOIN_ROUND = 2,
    D2D1_LINE_JOIN_MITER_OR_BEVEL = 3,
    D2D1_LINE_JOIN_FORCE_DWORD = 0xffffffff
} D2D1_LINE_JOIN;

typedef enum D2D1_DASH_STYLE
{
    D2D1_DASH_STYLE_SOLID = 0,
    D2D1_DASH_STYLE_DASH = 1,
    D2D1_DASH_STYLE_DOT = 2,
    D2D1_DASH_STYLE_DASH_DOT = 3,
    D2D1_DASH_STYLE_DASH_DOT_DOT = 4,
    D2D1_DASH_STYLE_CUSTOM = 5,
    D2D1_DASH_STYLE_FORCE_DWORD = 0xffffffff
} D2D1_DASH_STYLE;

typedef enum D2D1_RENDER_TARGET_TYPE
{
    D2D1_RENDER_TARGET_TYPE_DEFAULT = 0,
    D2D1_RENDER_TARGET_TYPE_SOFTWARE = 1,
    D2D1_RENDER_TARGET_TYPE_HARDWARE = 2,
    D2D1_RENDER_TARGET_TYPE_FORCE_DWORD = 0xffffffff
} D2D1_RENDER_TARGET_TYPE;

typedef enum D2D1_RENDER_TARGET_USAGE
{
    D2D1_RENDER_TARGET_USAGE_NONE = 0x00000000,
    D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING = 0x00000001,
    D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE = 0x00000002,
    D2D1_RENDER_TARGET_USAGE_FORCE_DWORD = 0xffffffff
} D2D1_RENDER_TARGET_USAGE;

typedef enum D2D1_FEATURE_LEVEL
{
    D2D1_FEATURE_LEVEL_DEFAULT = 0,
    D2D1_FEATURE_LEVEL_9 = D3D_FEATURE_LEVEL_9_1,
    D2D1_FEATURE_LEVEL_10 = D3D_FEATURE_LEVEL_10_0,
    D2D1_FEATURE_LEVEL_FORCE_DWORD = 0xffffffff
} D2D1_FEATURE_LEVEL;

typedef enum D2D1_DRAW_TEXT_OPTIONS
{
    D2D1_DRAW_TEXT_OPTIONS_NO_SNAP = 0x00000001,
    D2D1_DRAW_TEXT_OPTIONS_CLIP = 0x00000002,
    D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT = 0x00000004,
    D2D1_DRAW_TEXT_OPTIONS_DISABLE_COLOR_BITMAP_SNAPPING = 0x00000008,
    D2D1_DRAW_TEXT_OPTIONS_NONE = 0x00000000,
    D2D1_DRAW_TEXT_OPTIONS_FORCE_DWORD = 0xffffffff
} D2D1_DRAW_TEXT_OPTIONS;

typedef enum D2D1_ANTIALIAS_MODE
{
    
    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE = 0,
    D2D1_ANTIALIAS_MODE_ALIASED = 1,
    D2D1_ANTIALIAS_MODE_FORCE_DWORD = 0xffffffff
} D2D1_ANTIALIAS_MODE;

typedef enum D2D1_TEXT_ANTIALIAS_MODE
{
    D2D1_TEXT_ANTIALIAS_MODE_DEFAULT = 0,
    D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE = 1,
    D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE = 2,
    D2D1_TEXT_ANTIALIAS_MODE_ALIASED = 3,
    D2D1_TEXT_ANTIALIAS_MODE_FORCE_DWORD = 0xffffffff
} D2D1_TEXT_ANTIALIAS_MODE;

// NOTE: Types.

typedef struct D2D1_FACTORY_OPTIONS
{
    D2D1_DEBUG_LEVEL debugLevel;
} D2D1_FACTORY_OPTIONS;

typedef struct D2D1_STROKE_STYLE_PROPERTIES
{
    D2D1_CAP_STYLE startCap;
    D2D1_CAP_STYLE endCap;
    D2D1_CAP_STYLE dashCap;
    D2D1_LINE_JOIN lineJoin;
    FLOAT miterLimit;
    D2D1_DASH_STYLE dashStyle;
    FLOAT dashOffset;
} D2D1_STROKE_STYLE_PROPERTIES;

typedef struct D2D1_RENDER_TARGET_PROPERTIES
{
    D2D1_RENDER_TARGET_TYPE type;
    D2D1_PIXEL_FORMAT pixelFormat;
    FLOAT dpiX;
    FLOAT dpiY;
    D2D1_RENDER_TARGET_USAGE usage;
    D2D1_FEATURE_LEVEL minLevel;
} D2D1_RENDER_TARGET_PROPERTIES;

typedef struct D2D1_ROUNDED_RECT
{
    D2D1_RECT_F rect;
    FLOAT radiusX;
    FLOAT radiusY;
} D2D1_ROUNDED_RECT;

typedef D2D_COLOR_F D2D1_COLOR_F;
typedef struct ID2D1Brush ID2D1Brush;
typedef struct IDWriteRenderingParams IDWriteRenderingParams;
typedef UINT64 D2D1_TAG;

typedef struct D2D1_BRUSH_PROPERTIES
{
    FLOAT opacity;
    D2D1_MATRIX_3X2_F transform;
} D2D1_BRUSH_PROPERTIES;

typedef struct ID2D1FactoryVtbl { void* table[]; } ID2D1FactoryVtbl;
typedef struct ID2D1StrokeStyleVtbl { void* table[]; } ID2D1StrokeStyleVtbl;
typedef struct ID2D1RenderTargetVtbl { void* table[]; } ID2D1RenderTargetVtbl;
typedef struct ID2D1SolidColorBrushVtbl { void* table[]; } ID2D1SolidColorBrushVtbl;

typedef struct ID2D1Factory { ID2D1FactoryVtbl* vtbl; } ID2D1Factory;
typedef struct ID2D1StrokeStyle { ID2D1StrokeStyleVtbl* vtbl; } ID2D1StrokeStyle;
typedef struct ID2D1RenderTarget { ID2D1RenderTargetVtbl* vtbl; } ID2D1RenderTarget;
typedef struct ID2D1SolidColorBrush { ID2D1SolidColorBrushVtbl* vtbl; } ID2D1SolidColorBrush;

static inline ULONG ID2D1Factory_Release(ID2D1Factory* self)
{
    return ((ULONG (WINAPI*)(ID2D1Factory*))self->vtbl->table[2])(self);
}

static inline HRESULT ID2D1Factory_CreateStrokeStyle(ID2D1Factory* self,
                                                     CONST D2D1_STROKE_STYLE_PROPERTIES* strokeStyleProperties,
                                                     CONST FLOAT* dashes,
                                                     UINT32 dashesCount,
                                                     ID2D1StrokeStyle** strokeStyle)
{
    return ((HRESULT (WINAPI*)(ID2D1Factory*, CONST D2D1_STROKE_STYLE_PROPERTIES*, CONST FLOAT*, UINT32, ID2D1StrokeStyle**))self->vtbl->table[11])(self, strokeStyleProperties, dashes, dashesCount, strokeStyle);
}

static inline HRESULT ID2D1Factory_CreateDxgiSurfaceRenderTarget(ID2D1Factory* self,
                                                                 IDXGISurface* dxgiSurface,
                                                                 CONST D2D1_RENDER_TARGET_PROPERTIES* renderTargetProperties,
                                                                 ID2D1RenderTarget** renderTarget)
{
    return ((HRESULT (WINAPI*)(ID2D1Factory*, IDXGISurface*, CONST D2D1_RENDER_TARGET_PROPERTIES*, ID2D1RenderTarget**))self->vtbl->table[15])(self, dxgiSurface, renderTargetProperties, renderTarget);
}

static inline ULONG ID2D1RenderTarget_Release(ID2D1RenderTarget* self)
{
    return ((ULONG (WINAPI*)(ID2D1RenderTarget*))self->vtbl->table[2])(self);
}

static inline HRESULT ID2D1RenderTarget_CreateSolidColorBrush(ID2D1RenderTarget* self,
                                                              CONST D2D1_COLOR_F* color,
                                                              CONST D2D1_BRUSH_PROPERTIES* brushProperties,
                                                              ID2D1SolidColorBrush** solidColorBrush)
{
    return ((HRESULT (WINAPI*)(ID2D1RenderTarget*, CONST D2D1_COLOR_F*, CONST D2D1_BRUSH_PROPERTIES*, ID2D1SolidColorBrush**))self->vtbl->table[8])(self, color, brushProperties, solidColorBrush);
}

static inline void ID2D1RenderTarget_DrawRoundedRectangle(ID2D1RenderTarget* self,
                                                          CONST D2D1_ROUNDED_RECT* roundedRect,
                                                          ID2D1Brush* brush,
                                                          FLOAT strokeWidth,
                                                          ID2D1StrokeStyle* strokeStyle)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, CONST D2D1_ROUNDED_RECT*, ID2D1Brush*, FLOAT, ID2D1StrokeStyle*))self->vtbl->table[18])(self, roundedRect, brush, strokeWidth, strokeStyle);
}

static inline void ID2D1RenderTarget_DrawText(ID2D1RenderTarget* self,
                                              CONST WCHAR* string,
                                              UINT32 stringLength,
                                              IDWriteTextFormat* textFormat,
                                              CONST D2D1_RECT_F* layoutRect,
                                              ID2D1Brush* defaultFillBrush,
                                              D2D1_DRAW_TEXT_OPTIONS options,
                                              DWRITE_MEASURING_MODE measuringMode)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, CONST WCHAR*, UINT32, IDWriteTextFormat*, CONST D2D1_RECT_F*, ID2D1Brush*, D2D1_DRAW_TEXT_OPTIONS, DWRITE_MEASURING_MODE))self->vtbl->table[27])(self, string, stringLength, textFormat, layoutRect, defaultFillBrush, options, measuringMode);
}

static inline void ID2D1RenderTarget_DrawTextLayout(ID2D1RenderTarget* self,
                                                    D2D1_POINT_2F origin,
                                                    IDWriteTextLayout* textLayout,
                                                    ID2D1Brush* defaultFillBrush,
                                                    D2D1_DRAW_TEXT_OPTIONS options)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, D2D1_POINT_2F, IDWriteTextLayout*, ID2D1Brush*, D2D1_DRAW_TEXT_OPTIONS))self->vtbl->table[28])(self, origin, textLayout, defaultFillBrush, options);
}

static inline void ID2D1RenderTarget_SetAntialiasMode(ID2D1RenderTarget* self, D2D1_ANTIALIAS_MODE antialiasMode)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, D2D1_ANTIALIAS_MODE))self->vtbl->table[32])(self, antialiasMode);
}

static inline D2D1_ANTIALIAS_MODE ID2D1RenderTarget_GetAntialiasMode(ID2D1RenderTarget* self)
{
    return ((D2D1_ANTIALIAS_MODE (WINAPI*)(ID2D1RenderTarget*))self->vtbl->table[33])(self);
}

static inline void ID2D1RenderTarget_SetTextAntialiasMode(ID2D1RenderTarget* self, D2D1_TEXT_ANTIALIAS_MODE textAntialiasMode)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, D2D1_TEXT_ANTIALIAS_MODE))self->vtbl->table[34])(self, textAntialiasMode);
}

static inline D2D1_TEXT_ANTIALIAS_MODE ID2D1RenderTarget_GetTextAntialiasMode(ID2D1RenderTarget* self)
{
    return ((D2D1_TEXT_ANTIALIAS_MODE (WINAPI*)(ID2D1RenderTarget*))self->vtbl->table[35])(self);
}

static inline void ID2D1RenderTarget_SetTextRenderingParams(ID2D1RenderTarget* self, IDWriteRenderingParams* textRenderingParams)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, IDWriteRenderingParams*))self->vtbl->table[36])(self, textRenderingParams);
}

static inline void ID2D1RenderTarget_GetTextRenderingParams(ID2D1RenderTarget* self, IDWriteRenderingParams** textRenderingParams)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, IDWriteRenderingParams**))self->vtbl->table[37])(self, textRenderingParams);
}

static inline void ID2D1RenderTarget_BeginDraw(ID2D1RenderTarget* self)
{
    ((void (WINAPI*)(ID2D1RenderTarget*))self->vtbl->table[48])(self);
}

static inline void ID2D1RenderTarget_EndDraw(ID2D1RenderTarget* self, D2D1_TAG* tag1, D2D1_TAG* tag2)
{
    ((void (WINAPI*)(ID2D1RenderTarget*, D2D1_TAG*, D2D1_TAG*))self->vtbl->table[49])(self, tag1, tag2);
}

static inline ULONG ID2D1SolidColorBrush_Release(ID2D1SolidColorBrush* self)
{
    return ((ULONG (WINAPI*)(ID2D1SolidColorBrush*))self->vtbl->table[2])(self);
}

EXTERN_C HRESULT DECLSPEC_IMPORT D2D1CreateFactory(D2D1_FACTORY_TYPE factoryType, REFIID riid, CONST D2D1_FACTORY_OPTIONS* pFactoryOptions, void** ppIFactory);

// NOTE: GUIDs.

DEFINE_GUID(IID_ID2D1Factory, 0x06152247, 0x6f50, 0x465a, 0x92, 0x45, 0x11, 0x8b, 0xfd, 0x3b, 0x60, 0x07);

#ifdef __clang__
#pragma clang diagnostic pop
#endif
