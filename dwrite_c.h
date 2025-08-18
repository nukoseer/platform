#ifndef H_DWRITE_C_H
#define H_DWRITE_C_H

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

typedef struct DWRITE_FONT_METRICS
{
    UINT16 designUnitsPerEm;
    UINT16 ascent;
    UINT16 descent;
    INT16 lineGap;
    UINT16 capHeight;
    UINT16 xHeight;
    INT16 underlinePosition;
    UINT16 underlineThickness;
    INT16 strikethroughPosition;
    UINT16 strikethroughThickness;
} DWRITE_FONT_METRICS;

typedef struct DWRITE_GLYPH_METRICS
{
    INT32 leftSideBearing;
    UINT32 advanceWidth;
    INT32 rightSideBearing;
    INT32 topSideBearing;
    UINT32 advanceHeight;
    INT32 bottomSideBearing;
    INT32 verticalOriginY;
} DWRITE_GLYPH_METRICS;

typedef struct DWRITE_GLYPH_OFFSET
{
    FLOAT advanceOffset;
    FLOAT ascenderOffset;
} DWRITE_GLYPH_OFFSET;

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

static inline ULONG IDWriteTextFormat_Release(IDWriteTextFormat* self)
{
    return ((ULONG (WINAPI*)(IDWriteTextFormat*))self->vtbl->table[2])(self);
}

EXTERN_C HRESULT DECLSPEC_IMPORT DWriteCreateFactory(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown** factory);

DEFINE_GUID(IID_IDWriteFactory, 0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48);

#endif
