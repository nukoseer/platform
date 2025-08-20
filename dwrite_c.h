#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmicrosoft-flexible-array"
#endif

/*******************************************************************************************************************/
/* IMPORTANT: This is the C compatible version of dwrite.h. All of the enums, types and functions directly copied. */
/*******************************************************************************************************************/

// NOTE: Enums.

typedef enum DWRITE_FONT_WEIGHT
{
    DWRITE_FONT_WEIGHT_THIN = 100,
    DWRITE_FONT_WEIGHT_EXTRA_LIGHT = 200,
    DWRITE_FONT_WEIGHT_ULTRA_LIGHT = 200,
    DWRITE_FONT_WEIGHT_LIGHT = 300,
    DWRITE_FONT_WEIGHT_SEMI_LIGHT = 350,
    DWRITE_FONT_WEIGHT_NORMAL = 400,
    DWRITE_FONT_WEIGHT_REGULAR = 400,
    DWRITE_FONT_WEIGHT_MEDIUM = 500,
    DWRITE_FONT_WEIGHT_DEMI_BOLD = 600,
    DWRITE_FONT_WEIGHT_SEMI_BOLD = 600,
    DWRITE_FONT_WEIGHT_BOLD = 700,
    DWRITE_FONT_WEIGHT_EXTRA_BOLD = 800,
    DWRITE_FONT_WEIGHT_ULTRA_BOLD = 800,
    DWRITE_FONT_WEIGHT_BLACK = 900,
    DWRITE_FONT_WEIGHT_HEAVY = 900,
    DWRITE_FONT_WEIGHT_EXTRA_BLACK = 950,
    DWRITE_FONT_WEIGHT_ULTRA_BLACK = 950
} DWRITE_FONT_WEIGHT;

typedef enum DWRITE_FONT_STRETCH
{
    DWRITE_FONT_STRETCH_UNDEFINED = 0,
    DWRITE_FONT_STRETCH_ULTRA_CONDENSED = 1,
    DWRITE_FONT_STRETCH_EXTRA_CONDENSED = 2,
    DWRITE_FONT_STRETCH_CONDENSED = 3,
    DWRITE_FONT_STRETCH_SEMI_CONDENSED = 4,
    DWRITE_FONT_STRETCH_NORMAL = 5,
    DWRITE_FONT_STRETCH_MEDIUM = 5,
    DWRITE_FONT_STRETCH_SEMI_EXPANDED = 6,
    DWRITE_FONT_STRETCH_EXPANDED = 7,
    DWRITE_FONT_STRETCH_EXTRA_EXPANDED = 8,
    DWRITE_FONT_STRETCH_ULTRA_EXPANDED = 9
} DWRITE_FONT_STRETCH;

typedef enum DWRITE_FONT_STYLE
{
    DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STYLE_OBLIQUE,
    DWRITE_FONT_STYLE_ITALIC
} DWRITE_FONT_STYLE;

typedef enum DWRITE_TEXT_ALIGNMENT
{
    DWRITE_TEXT_ALIGNMENT_LEADING,
    DWRITE_TEXT_ALIGNMENT_TRAILING,
    DWRITE_TEXT_ALIGNMENT_CENTER,
    DWRITE_TEXT_ALIGNMENT_JUSTIFIED
} DWRITE_TEXT_ALIGNMENT;

typedef enum DWRITE_PARAGRAPH_ALIGNMENT
{
    DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
    DWRITE_PARAGRAPH_ALIGNMENT_FAR,
    DWRITE_PARAGRAPH_ALIGNMENT_CENTER
} DWRITE_PARAGRAPH_ALIGNMENT;

typedef enum DWRITE_WORD_WRAPPING
{
    DWRITE_WORD_WRAPPING_WRAP = 0,
    DWRITE_WORD_WRAPPING_NO_WRAP = 1,
    DWRITE_WORD_WRAPPING_EMERGENCY_BREAK = 2,
    DWRITE_WORD_WRAPPING_WHOLE_WORD = 3,
    DWRITE_WORD_WRAPPING_CHARACTER = 4,
} DWRITE_WORD_WRAPPING;

typedef enum DWRITE_FACTORY_TYPE
{
    DWRITE_FACTORY_TYPE_SHARED,
    DWRITE_FACTORY_TYPE_ISOLATED
} DWRITE_FACTORY_TYPE;

// NOTE: Types.

typedef struct DWRITE_TEXT_METRICS
{
    FLOAT left;
    FLOAT top;
    FLOAT width;
    FLOAT widthIncludingTrailingWhitespace;
    FLOAT height;
    FLOAT layoutWidth;
    FLOAT layoutHeight;
    UINT32 maxBidiReorderingDepth;
    UINT32 lineCount;
} DWRITE_TEXT_METRICS;

typedef struct IDWriteFactoryVtbl { void* table[]; } IDWriteFactoryVtbl;
typedef struct IDWriteTextFormatVtbl { void* table[]; } IDWriteTextFormatVtbl;
typedef struct IDWriteTextLayoutVtbl { void* table[]; } IDWriteTextLayoutVtbl;

typedef struct IDWriteFactory { IDWriteFactoryVtbl* vtbl; } IDWriteFactory;
typedef struct IDWriteTextFormat { IDWriteTextFormatVtbl* vtbl; } IDWriteTextFormat;
typedef struct IDWriteTextLayout { IDWriteTextLayoutVtbl* vtbl; } IDWriteTextLayout;

static inline ULONG IDWriteFactory_Release(IDWriteFactory* self)
{
    return ((ULONG (WINAPI*)(IDWriteFactory*))self->vtbl->table[2])(self);
}

static inline HRESULT IDWriteFactory_CreateTextFormat(IDWriteFactory* self,
                                                      WCHAR const* fontFamilyName,
                                                      IUnknown* fontCollection,
                                                      DWRITE_FONT_WEIGHT fontWeight,
                                                      DWRITE_FONT_STYLE fontStyle,
                                                      DWRITE_FONT_STRETCH fontStretch,
                                                      FLOAT fontSize,
                                                      WCHAR const* localeName,
                                                      IDWriteTextFormat** textFormat)
{
    return ((HRESULT (WINAPI*)(IDWriteFactory*, WCHAR const*, IUnknown*, DWRITE_FONT_WEIGHT, DWRITE_FONT_STYLE, DWRITE_FONT_STRETCH, FLOAT, WCHAR const*, IDWriteTextFormat**))self->vtbl->table[15])(self, fontFamilyName, fontCollection, fontWeight, fontStyle, fontStretch, fontSize, localeName, textFormat);
}

static inline HRESULT IDWriteTextFormat_SetTextAlignment(IDWriteTextFormat* self, DWRITE_TEXT_ALIGNMENT textAlignment)
{
    return ((HRESULT (WINAPI*)(IDWriteTextFormat*, DWRITE_TEXT_ALIGNMENT))self->vtbl->table[3])(self, textAlignment);
}

static inline HRESULT IDWriteTextFormat_SetParagraphAlignment(IDWriteTextFormat* self, DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment)
{
    return ((HRESULT (WINAPI*)(IDWriteTextFormat*, DWRITE_PARAGRAPH_ALIGNMENT))self->vtbl->table[4])(self, paragraphAlignment);
}

static inline HRESULT IDWriteTextFormat_SetWordWrapping(IDWriteTextFormat* self, DWRITE_WORD_WRAPPING wordWrapping)
{
    return ((HRESULT (WINAPI*)(IDWriteTextFormat*, DWRITE_WORD_WRAPPING))self->vtbl->table[5])(self, wordWrapping);
}

static inline ULONG IDWriteTextFormat_Release(IDWriteTextFormat* self)
{
    return ((ULONG (WINAPI*)(IDWriteTextFormat*))self->vtbl->table[2])(self);
}

static inline HRESULT IDWriteFactory_CreateTextLayout(IDWriteFactory* self,
                                                      WCHAR const* string,
                                                      UINT32 stringLength,
                                                      IDWriteTextFormat* textFormat,
                                                      FLOAT maxWidth,
                                                      FLOAT maxHeight,
                                                      IDWriteTextLayout** textLayout)
{
    return ((HRESULT (WINAPI*)(IDWriteFactory*, WCHAR const*, UINT32, IDWriteTextFormat*, FLOAT,  FLOAT, IDWriteTextLayout**))self->vtbl->table[18])(self, string, stringLength, textFormat, maxWidth, maxHeight, textLayout);
}

static inline ULONG IDWriteTextLayout_Release(IDWriteTextLayout* self)
{
    return ((ULONG (WINAPI*)(IDWriteTextLayout*))self->vtbl->table[2])(self);
}

static inline HRESULT IDWriteTextLayout_GetMetrics(IDWriteTextLayout* self, DWRITE_TEXT_METRICS* textMetrics)
{
    return ((HRESULT (WINAPI*)(IDWriteTextLayout*, DWRITE_TEXT_METRICS*))self->vtbl->table[60])(self, textMetrics);
}

EXTERN_C HRESULT DECLSPEC_IMPORT DWriteCreateFactory(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown** factory);

// NOTE: GUIDs.

DEFINE_GUID(IID_IDWriteFactory, 0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48);

#ifdef __clang__
#pragma clang diagnostic pop
#endif
