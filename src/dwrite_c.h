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

typedef enum DWRITE_FONT_FACE_TYPE
{
    DWRITE_FONT_FACE_TYPE_CFF,
    DWRITE_FONT_FACE_TYPE_TRUETYPE,
    DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION,
    DWRITE_FONT_FACE_TYPE_TYPE1,
    DWRITE_FONT_FACE_TYPE_VECTOR,
    DWRITE_FONT_FACE_TYPE_BITMAP,
    DWRITE_FONT_FACE_TYPE_UNKNOWN,
    DWRITE_FONT_FACE_TYPE_RAW_CFF,
    DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION = DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION,
} DWRITE_FONT_FACE_TYPE;

typedef enum DWRITE_FONT_SIMULATIONS
{
    DWRITE_FONT_SIMULATIONS_NONE    = 0x0000,
    DWRITE_FONT_SIMULATIONS_BOLD    = 0x0001,
    DWRITE_FONT_SIMULATIONS_OBLIQUE = 0x0002
} DWRITE_FONT_SIMULATIONS;

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

typedef struct IDWriteFontVtbl { void* table[]; } IDWriteFontVtbl;
typedef struct IDWriteFontFamilyVtbl { void* table[]; } IDWriteFontFamilyVtbl;
typedef struct IDWriteFontCollectionVtbl { void* table[]; } IDWriteFontCollectionVtbl;
typedef struct IDWriteFontFileVtbl { void* table[]; } IDWriteFontFileVtbl;
typedef struct IDWriteFontFaceVtbl { void* table[]; } IDWriteFontFaceVtbl;
typedef struct IDWriteFactoryVtbl { void* table[]; } IDWriteFactoryVtbl;
typedef struct IDWriteTextFormatVtbl { void* table[]; } IDWriteTextFormatVtbl;
typedef struct IDWriteTextLayoutVtbl { void* table[]; } IDWriteTextLayoutVtbl;
typedef struct IDWriteRenderingParamsVtbl { void* table[]; } IDWriteRenderingParamsVtbl;

typedef struct IDWriteFont { IDWriteFontVtbl* vtbl; } IDWriteFont;
typedef struct IDWriteFontFamily { IDWriteFontFamilyVtbl* vtbl; } IDWriteFontFamily;
typedef struct IDWriteFontCollection { IDWriteFontCollectionVtbl* vtbl; } IDWriteFontCollection;
typedef struct IDWriteFontFile { IDWriteFontFileVtbl* vtbl; } IDWriteFontFile;
typedef struct IDWriteFontFace { IDWriteFontFaceVtbl* vtbl; } IDWriteFontFace;
typedef struct IDWriteFactory { IDWriteFactoryVtbl* vtbl; } IDWriteFactory;
typedef struct IDWriteTextFormat { IDWriteTextFormatVtbl* vtbl; } IDWriteTextFormat;
typedef struct IDWriteTextLayout { IDWriteTextLayoutVtbl* vtbl; } IDWriteTextLayout;
typedef struct IDWriteRenderingParams { IDWriteRenderingParamsVtbl* vtbl; } IDWriteRenderingParams;

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

static inline HRESULT IDWriteFactory_CreateRenderingParams(IDWriteFactory* self, IDWriteRenderingParams** renderingParams)
{
    return ((HRESULT (WINAPI*)(IDWriteFactory*, IDWriteRenderingParams**))self->vtbl->table[10])(self, renderingParams);
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

static inline HRESULT IDWriteFactory_CreateFontFileReference(IDWriteFactory* self, WCHAR const* filePath, void* lastWriteTime, IDWriteFontFile** fontFile)
{
    return ((HRESULT (WINAPI*)(IDWriteFactory*, WCHAR const*, void*, IDWriteFontFile**))self->vtbl->table[7])(self, filePath, lastWriteTime, fontFile);
}

static inline HRESULT IDWriteFactory_CreateFontFace(IDWriteFactory* self, DWRITE_FONT_FACE_TYPE fontFaceType, UINT32 numberOfFiles, IDWriteFontFile* const* fontFiles, UINT32 faceIndex, DWRITE_FONT_SIMULATIONS fontFaceSimulationFlags, IDWriteFontFace** fontFace)
{
    return ((HRESULT (WINAPI*)(IDWriteFactory*, DWRITE_FONT_FACE_TYPE, UINT32, IDWriteFontFile* const*, UINT32, DWRITE_FONT_SIMULATIONS, IDWriteFontFace**))self->vtbl->table[9])(self, fontFaceType, numberOfFiles, fontFiles, faceIndex, fontFaceSimulationFlags, fontFace);
}

static inline HRESULT IDWriteFactory_GetSystemFontCollection(IDWriteFactory* self, IDWriteFontCollection** fontCollection, BOOL checkForUpdates)
{
    return ((HRESULT (WINAPI*)(IDWriteFactory*, IDWriteFontCollection**, BOOL))self->vtbl->table[3])(self, fontCollection, checkForUpdates);
}

static inline ULONG IDWriteTextLayout_Release(IDWriteTextLayout* self)
{
    return ((ULONG (WINAPI*)(IDWriteTextLayout*))self->vtbl->table[2])(self);
}

static inline HRESULT IDWriteTextLayout_GetMetrics(IDWriteTextLayout* self, DWRITE_TEXT_METRICS* textMetrics)
{
    return ((HRESULT (WINAPI*)(IDWriteTextLayout*, DWRITE_TEXT_METRICS*))self->vtbl->table[60])(self, textMetrics);
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

static inline FLOAT IDWriteRenderingParams_GetClearTypeLevel(IDWriteRenderingParams* self)
{
    return ((FLOAT (WINAPI*)(IDWriteRenderingParams*))self->vtbl->table[5])(self);
}

static inline ULONG IDWriteRenderingParams_Release(IDWriteRenderingParams* self)
{
    return ((ULONG (WINAPI*)(IDWriteRenderingParams*))self->vtbl->table[2])(self);
}

static inline HRESULT IDWriteFont_CreateFontFace(IDWriteFont* self, IDWriteFontFace** fontFace)
{
    return ((HRESULT (WINAPI*)(IDWriteFont*, IDWriteFontFace**))self->vtbl->table[13])(self, fontFace);
}

static inline HRESULT IDWriteFont_GetFontFamily(IDWriteFont* self, IDWriteFontFamily** fontFamily)
{
    return ((HRESULT (WINAPI*)(IDWriteFont*, IDWriteFontFamily**))self->vtbl->table[3])(self, fontFamily);
}

static inline HRESULT IDWriteFontFamily_GetFirstMatchingFont(IDWriteFontFamily* self, DWRITE_FONT_WEIGHT weight, DWRITE_FONT_STRETCH stretch, DWRITE_FONT_STYLE style, IDWriteFont** matchingFont)
{
    return ((HRESULT (WINAPI*)(IDWriteFontFamily*, DWRITE_FONT_WEIGHT, DWRITE_FONT_STRETCH, DWRITE_FONT_STYLE, IDWriteFont**))self->vtbl->table[7])(self, weight, stretch, style, matchingFont);
}

static inline HRESULT IDWriteFontCollection_GetFontFamily(IDWriteFontCollection* self, UINT32 index, IDWriteFontFamily** fontFamily)
{
    return ((HRESULT (WINAPI*)(IDWriteFontCollection*, UINT32, IDWriteFontFamily**))self->vtbl->table[4])(self, index, fontFamily);
}

static inline HRESULT IDWriteFontCollection_FindFamilyName(IDWriteFontCollection* self, WCHAR const* familyName, UINT32* index, BOOL* exists)
{
    return ((HRESULT (WINAPI*)(IDWriteFontCollection*, WCHAR const*, UINT32*, BOOL*))self->vtbl->table[5])(self, familyName, index, exists);
}

static inline void IDWriteFontFace_GetMetrics(IDWriteFontFace* self, DWRITE_FONT_METRICS* fontMetrics)
{
    ((void (WINAPI*)(IDWriteFontFace*, DWRITE_FONT_METRICS*))self->vtbl->table[8])(self, fontMetrics);
}

static inline HRESULT IDWriteFontFace_GetDesignGlyphMetrics(IDWriteFontFace* self, const UINT16* glyphIndices, UINT32 glyphCount, DWRITE_GLYPH_METRICS* glyphMetrics, BOOL isSideways)
{
    return ((HRESULT (WINAPI*)(IDWriteFontFace*, UINT16 const*, UINT32, DWRITE_GLYPH_METRICS*, BOOL))self->vtbl->table[10])(self, glyphIndices, glyphCount, glyphMetrics, isSideways);
}

static inline HRESULT IDWriteFontFace_GetGlyphIndices(IDWriteFontFace* self, const UINT32* codePoints, UINT32 codePointCount, UINT16* glyphIndices)
{
    return ((HRESULT (WINAPI*)(IDWriteFontFace*, const UINT32*, UINT32, UINT16*))self->vtbl->table[11])(self, codePoints, codePointCount, glyphIndices);
}

EXTERN_C HRESULT DECLSPEC_IMPORT DWriteCreateFactory(DWRITE_FACTORY_TYPE factoryType, REFIID iid, IUnknown** factory);

// NOTE: GUIDs.

DEFINE_GUID(IID_IDWriteFactory, 0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48);

#ifdef __clang__
#pragma clang diagnostic pop
#endif
