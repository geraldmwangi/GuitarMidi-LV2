/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

using FontForRange = std::pair<Range<int64>, Font>;

class ShapedTextOptions
{
public:
    [[nodiscard]] ShapedTextOptions withJustification (Justification x) const
    {
        return withMember (*this, &ShapedTextOptions::justification, x);
    }

    [[nodiscard]] ShapedTextOptions withMaxWidth (float x) const
    {
        return withMember (*this, &ShapedTextOptions::maxWidth, x);
    }

    [[nodiscard]] ShapedTextOptions withHeight (float x) const
    {
        return withMember (*this, &ShapedTextOptions::height, x);
    }

    [[nodiscard]] ShapedTextOptions withFont (Font x) const
    {
        return withMember (*this, &ShapedTextOptions::fontsForRange,
                           std::vector<FontForRange> { { { 0, std::numeric_limits<int64>::max() },
                                                         x } });
    }

    [[nodiscard]] ShapedTextOptions withFontsForRange (const std::vector<FontForRange>& x) const
    {
        return withMember (*this, &ShapedTextOptions::fontsForRange, x);
    }

    [[nodiscard]] ShapedTextOptions withLanguage (StringRef x) const
    {
        return withMember (*this, &ShapedTextOptions::language, x);
    }

    [[nodiscard]] ShapedTextOptions withFirstLineIndent (float x) const
    {
        return withMember (*this, &ShapedTextOptions::firstLineIndent, x);
    }

    [[nodiscard]] ShapedTextOptions withLeading (float x) const
    {
        return withMember (*this, &ShapedTextOptions::leading, x);
    }

    [[nodiscard]] ShapedTextOptions withBaselineAtZero (bool x = true) const
    {
        return withMember (*this, &ShapedTextOptions::baselineAtZero, x);
    }

    [[nodiscard]] ShapedTextOptions withTrailingWhitespacesShouldFit (bool x = true) const
    {
        return withMember (*this, &ShapedTextOptions::trailingWhitespacesShouldFit, x);
    }

    [[nodiscard]] ShapedTextOptions withMaxNumLines (int64 x) const
    {
        return withMember (*this, &ShapedTextOptions::maxNumLines, x);
    }

    [[nodiscard]] ShapedTextOptions withEllipsis (String x = String::charToString ((juce_wchar) 0x2026)) const
    {
        return withMember (*this, &ShapedTextOptions::ellipsis, std::move (x));
    }

    [[nodiscard]] ShapedTextOptions withReadingDirection (std::optional<TextDirection> x) const
    {
        return withMember (*this, &ShapedTextOptions::readingDir, x);
    }

    const auto& getReadingDirection() const             { return readingDir; }
    const auto& getJustification() const                { return justification; }
    const auto& getMaxWidth() const                     { return maxWidth; }
    const auto& getHeight() const                       { return height; }
    const auto& getFontsForRange() const                { return fontsForRange; }
    const auto& getLanguage() const                     { return language; }
    const auto& getFirstLineIndent() const              { return firstLineIndent; }
    const auto& getLeading() const                      { return leading; }
    const auto& isBaselineAtZero() const                { return baselineAtZero; }
    const auto& getTrailingWhitespacesShouldFit() const { return trailingWhitespacesShouldFit; }
    const auto& getMaxNumLines() const                  { return maxNumLines; }
    const auto& getEllipsis() const                     { return ellipsis; }

private:
    Justification justification { Justification::topLeft };
    std::optional<TextDirection> readingDir;
    std::optional<float> maxWidth;
    std::optional<float> height;
    std::vector<FontForRange> fontsForRange { { { 0, std::numeric_limits<int64>::max() },
                                                FontOptions { 15.0f } } };
    String language = SystemStats::getDisplayLanguage();
    float firstLineIndent = 0.0f;
    float leading = 1.0f;
    bool baselineAtZero = false;
    bool trailingWhitespacesShouldFit;
    int64 maxNumLines = std::numeric_limits<int64>::max();
    String ellipsis;
};

struct ShapedGlyph
{
    uint32_t glyphId;
    int64 cluster;
    bool unsafeToBreak;
    bool whitespace;
    Point<float> advance;
    Point<float> offset;
};

class SimpleShapedText
{
public:
    /*  Shapes and lays out the first contiguous sequence of ranges specified in the fonts
        parameter.
    */
    SimpleShapedText (const String* data,
                      const ShapedTextOptions& options);

    /*  The returned container associates line numbers with the range of glyphs (not input codepoints)
        that make up the line.
    */
    const auto& getLineNumbers() const { return lineNumbers; }

    const auto& getResolvedFonts() const { return resolvedFonts; }

    Range<int64> getTextRange (int64 glyphIndex) const;

    int64 getNumLines() const { return (int64) lineNumbers.getRanges().size(); }
    int64 getNumGlyphs() const { return (int64) glyphsInVisualOrder.size(); }

    juce_wchar getCodepoint (int64 glyphIndex) const;

    Range<int64> getGlyphRangeForLine (size_t line) const;

    std::vector<FontForRange> getResolvedFontsIntersectingGlyphRange (Range<int64> glyphRange) const;

    Span<const ShapedGlyph> getGlyphs (Range<int64> glyphRange) const;

    Span<const ShapedGlyph> getGlyphs() const;

private:
    void shape (const String& data,
                const ShapedTextOptions& options);

    struct GlyphLookupEntry
    {
        Range<int64> glyphRange;
        bool ltr = true;
    };

    const String& string;
    std::vector<ShapedGlyph> glyphsInVisualOrder;
    detail::RangedValues<int64> lineNumbers;
    detail::RangedValues<Font> resolvedFonts;
    detail::RangedValues<GlyphLookupEntry> glyphLookup;

    JUCE_LEAK_DETECTOR (SimpleShapedText)
};

//==============================================================================
using namespace detail;

constexpr hb_script_t getScriptTag (TextScript type)
{
    JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wswitch-enum")
    switch (type)
    {
    case TextScript::common:     return HB_SCRIPT_COMMON;
    case TextScript::arabic:     return HB_SCRIPT_ARABIC;
    case TextScript::armenian:   return HB_SCRIPT_ARMENIAN;
    case TextScript::bengali:    return HB_SCRIPT_BENGALI;
    case TextScript::bopomofo:   return HB_SCRIPT_BOPOMOFO;
    case TextScript::cyrillic:   return HB_SCRIPT_CYRILLIC;
    case TextScript::devanagari: return HB_SCRIPT_DEVANAGARI;
    case TextScript::ethiopic:   return HB_SCRIPT_ETHIOPIC;
    case TextScript::georgian:   return HB_SCRIPT_GEORGIAN;
    case TextScript::greek:      return HB_SCRIPT_GREEK;
    case TextScript::gujarati:   return HB_SCRIPT_GUJARATI;
    case TextScript::gurmukhi:   return HB_SCRIPT_GURMUKHI;
    case TextScript::hangul:     return HB_SCRIPT_HANGUL;
    case TextScript::han:        return HB_SCRIPT_HAN;
    case TextScript::hebrew:     return HB_SCRIPT_HEBREW;
    case TextScript::hiragana:   return HB_SCRIPT_HIRAGANA;
    case TextScript::katakana:   return HB_SCRIPT_KATAKANA;
    case TextScript::kannada:    return HB_SCRIPT_KANNADA;
    case TextScript::khmer:      return HB_SCRIPT_KHMER;
    case TextScript::lao:        return HB_SCRIPT_LAO;
    case TextScript::latin:      return HB_SCRIPT_LATIN;
    case TextScript::malayalam:  return HB_SCRIPT_MALAYALAM;
    case TextScript::oriya:      return HB_SCRIPT_ORIYA;
    case TextScript::sinhala:    return HB_SCRIPT_SINHALA;
    case TextScript::tamil:      return HB_SCRIPT_TAMIL;
    case TextScript::telugu:     return HB_SCRIPT_TELUGU;
    case TextScript::thaana:     return HB_SCRIPT_THAANA;
    case TextScript::thai:       return HB_SCRIPT_THAI;
    case TextScript::tibetan:    return HB_SCRIPT_TIBETAN;
    case TextScript::adlam:      return HB_SCRIPT_ADLAM;
    case TextScript::balinese:   return HB_SCRIPT_BALINESE;
    case TextScript::bamum:      return HB_SCRIPT_BAMUM;
    case TextScript::batak:      return HB_SCRIPT_BATAK;
    case TextScript::chakma:     return HB_SCRIPT_CHAKMA;
    case TextScript::cham:       return HB_SCRIPT_CHAM;
    case TextScript::cherokee:   return HB_SCRIPT_CHEROKEE;
    case TextScript::javanese:   return HB_SCRIPT_JAVANESE;
    case TextScript::kayahLi:    return HB_SCRIPT_KAYAH_LI;
    case TextScript::taiTham:    return HB_SCRIPT_TAI_THAM;
    case TextScript::lepcha:     return HB_SCRIPT_LEPCHA;
    case TextScript::limbu:      return HB_SCRIPT_LIMBU;
    case TextScript::lisu:       return HB_SCRIPT_LISU;
    case TextScript::mandaic:    return HB_SCRIPT_MANDAIC;
    case TextScript::meeteiMayek:return HB_SCRIPT_MEETEI_MAYEK;
    case TextScript::newa:       return HB_SCRIPT_NEWA;
    case TextScript::nko:        return HB_SCRIPT_NKO;
    case TextScript::olChiki:    return HB_SCRIPT_OL_CHIKI;
    case TextScript::osage:      return HB_SCRIPT_OSAGE;
    case TextScript::miao:       return HB_SCRIPT_MIAO;
    case TextScript::saurashtra: return HB_SCRIPT_SAURASHTRA;
    case TextScript::sundanese:  return HB_SCRIPT_SUNDANESE;
    case TextScript::sylotiNagri:return HB_SCRIPT_SYLOTI_NAGRI;
    case TextScript::syriac:     return HB_SCRIPT_SYRIAC;
    case TextScript::taiLe:      return HB_SCRIPT_TAI_LE;
    case TextScript::newTaiLue:  return HB_SCRIPT_NEW_TAI_LUE;
    case TextScript::tifinagh:   return HB_SCRIPT_TIFINAGH;
    case TextScript::vai:        return HB_SCRIPT_VAI;
    case TextScript::wancho:     return HB_SCRIPT_WANCHO;
    case TextScript::yi:         return HB_SCRIPT_YI;

    case TextScript::hanifiRohingya:               return HB_SCRIPT_HANIFI_ROHINGYA;
    case TextScript::canadianAboriginalSyllabics:  return HB_SCRIPT_CANADIAN_SYLLABICS;
    case TextScript::nyiakengPuachueHmong:         return HB_SCRIPT_NYIAKENG_PUACHUE_HMONG;

    default: break;
    }
    JUCE_END_IGNORE_WARNINGS_GCC_LIKE

    return HB_SCRIPT_COMMON;
}

struct ResolvedFontRun
{
    Font font;
    Span<const Unicode::Codepoint> run;
};

SimpleShapedText::SimpleShapedText (const String* data,
                                    const ShapedTextOptions& options)
    : string (*data)
{
    shape (string, options);
}

struct Utf8Lookup
{
    Utf8Lookup (const String& s)
    {
        const auto base = s.toUTF8();

        for (auto cursor = base; ! cursor.isEmpty(); ++cursor)
            indices.push_back ((size_t) std::distance (base.getAddress(), cursor.getAddress()));

        beyondEnd = s.getNumBytesAsUTF8();
    }

    auto getByteIndex (int64 codepointIndex) const
    {
        jassert (codepointIndex <= (int64) indices.size());

        if (codepointIndex == (int64) indices.size())
            return beyondEnd;

        return indices[(size_t) codepointIndex];
    }

    auto getCodepointIndex (size_t byteIndex) const
    {
        auto it = std::lower_bound (indices.cbegin(), indices.cend(), byteIndex);

        jassert (it != indices.end());

        return (int64) std::distance (indices.begin(), it);
    }

    auto getByteRange (Range<int64> range)
    {
        return Range<size_t> { getByteIndex (range.getStart()),
                               getByteIndex (range.getEnd()) };
    }

private:
    std::vector<size_t> indices;
    size_t beyondEnd{};
};

/*  Returns glyphs in logical order as that favours wrapping. */
static std::vector<ShapedGlyph> lowLevelShape (const String& string,
                                               Range<int64> range,
                                               const Font& font,
                                               TextScript script,
                                               const String& language,
                                               TextDirection direction)
{
    HbBuffer buffer { hb_buffer_create() };
    hb_buffer_clear_contents (buffer.get());

    hb_buffer_set_cluster_level (buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
    hb_buffer_set_script (buffer.get(), getScriptTag (script));
    hb_buffer_set_language (buffer.get(), hb_language_from_string (language.toRawUTF8(), -1));

    hb_buffer_set_direction (buffer.get(),
                             direction == TextDirection::ltr ? HB_DIRECTION_LTR : HB_DIRECTION_RTL);

    Utf8Lookup utf8Lookup { string };

    const auto preContextByteRange = utf8Lookup.getByteRange (Range<int64> { 0, range.getStart() });

    hb_buffer_add_utf8 (buffer.get(),
                        string.toRawUTF8() + preContextByteRange.getStart(),
                        (int) preContextByteRange.getLength(),
                        0,
                        0);

    // Adding the converted portion of the text with hb_buffer_add_utf32() or especially with
    // hb_buffer_add() gives us control over cluster numbers. hb_buffer_add_utf32() will increment
    // cluster numbers by unicode codepoints (as opposed to UTF8 bytes) starting from 0.
    auto utf32Span = Span { string.toUTF32().getAddress() + (size_t) range.getStart(),
                            (size_t) range.getLength() };

    // We're using a word joiner (zero width non-breaking space) followed by a non-breaking space
    // for visual representation. This is so that it's not possible to break the glyph representing
    // the line breaking glyph on its own.
    static constexpr uint32_t crLf[] = { 0x2060, 0x00A0 };

    const auto numLineEndsToReplace = [&]
    {
        constexpr auto lf = 0x0a;
        constexpr auto cr = 0x0d;

        if (! utf32Span.empty() && (utf32Span.back() == lf || utf32Span.back() == cr))
        {
            if (utf32Span.size() >= 2 && utf32Span[utf32Span.size() - 2] == cr)
                return 2;

            return 1;
        }

        return 0;
    }();

    hb_buffer_add_utf32 (buffer.get(),
                         (uint32_t*) utf32Span.data(),
                         (int) range.getLength() - numLineEndsToReplace,
                         (unsigned int) 0,
                         (int) range.getLength() - numLineEndsToReplace);

    for (int i = 0; i < numLineEndsToReplace; ++i)
    {
        // The following gets cluster values right, but this does not follow clearly from harfbuzz documentation.
        // Add at least a regression test checking the correctness of cluster values.
        hb_buffer_add (buffer.get(),
                       static_cast<hb_codepoint_t> (*(crLf + (2 - numLineEndsToReplace) + i)),
                       (unsigned int) ((int) range.getLength() - numLineEndsToReplace + i));
    }

    const auto postContextByteRange = utf8Lookup.getByteRange (Range<int64> { range.getEnd(), (int64) string.length() });

    hb_buffer_add_utf8 (buffer.get(),
                        string.toRawUTF8() + postContextByteRange.getStart(),
                        (int) postContextByteRange.getLength(),
                        0,
                        0);

    std::vector<hb_feature_t> features;

    // Disable ligatures if we're using non-standard tracking
    const auto tracking           = font.getExtraKerningFactor();
    const auto trackingIsDefault  = approximatelyEqual (tracking, 0.0f, absoluteTolerance (0.001f));

    if (! trackingIsDefault)
        for (const auto key : { hbTag ("liga"), hbTag ("clig"), hbTag ("hlig"), hbTag ("dlig"), hbTag ("calt") })
            features.push_back (hb_feature_t { key, 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END });

    hb_buffer_guess_segment_properties (buffer.get());

    auto nativeFont = font.getNativeDetails().font;

    if (nativeFont == nullptr)
    {
        jassertfalse;
        return {};
    }

    hb_shape (nativeFont.get(), buffer.get(), features.data(), (unsigned int) features.size());

    const auto [infos, positions] = [&buffer]
    {
        unsigned int count{};

        return std::make_pair (Span { hb_buffer_get_glyph_infos     (buffer.get(), &count), (size_t) count },
                               Span { hb_buffer_get_glyph_positions (buffer.get(), &count), (size_t) count });
    }();

    jassert (infos.size() == positions.size());

    const auto missingGlyph = hb_buffer_get_not_found_glyph (buffer.get());

    [[maybe_unused]] const auto unknownGlyph = std::find_if (infos.begin(), infos.end(), [missingGlyph] (hb_glyph_info_t inf)
    {
        return inf.codepoint == missingGlyph;
    });

    // It this is hit, the typeface can't display one or more characters.
    // This normally shouldn't happen if font fallback is enabled, unless the String contains
    // control characters that JUCE doesn't know how to handle appropriately.
    jassert (unknownGlyph == infos.end());

    [[maybe_unused]] const auto trackingAmount = (! HB_DIRECTION_IS_VERTICAL (direction) && ! trackingIsDefault)
                                               ? font.getHeight() * tracking
                                               : 0;

    std::vector<size_t> clusterLookup;
    std::vector<size_t> characterLookup;
    std::vector<ShapedGlyph> glyphs;

    for (size_t i = 0; i < infos.size(); ++i)
    {
        const auto j = direction == TextDirection::ltr ? i : infos.size() - 1 - i;

        const auto glyphId = infos[j].codepoint;
        const auto xAdvance = positions[j].x_advance;

        glyphs.push_back ({
            glyphId,
            (int64) infos[j].cluster + range.getStart(),
            (infos[j].mask & HB_GLYPH_FLAG_UNSAFE_TO_BREAK) != 0,
            font.getTypefacePtr()->getGlyphBounds (font.getMetricsKind(), (int) glyphId).isEmpty() && xAdvance > 0,
            Point<float> { HbScale::hbToJuce (xAdvance) + trackingAmount, -HbScale::hbToJuce (positions[j].y_advance) },
            Point<float> { HbScale::hbToJuce (positions[j].x_offset), -HbScale::hbToJuce (positions[j].y_offset) },
        });
    }

    return glyphs;
}

template <typename T>
struct SubSpanLookup
{
    explicit SubSpanLookup (Span<T> enclosingSpan)
        : enclosing (enclosingSpan)
    {}

    auto getRange (Span<T> span) const
    {
        jassert (enclosing.begin() <= span.begin() && enclosing.size() >= span.size());

        return Range<int64>::withStartAndLength ((int64) std::distance (enclosing.begin(), span.begin()),
                                                 (int64) span.size());
    }

    auto getSpan (Range<int64> r) const
    {
        jassert (r.getStart() + r.getLength() <= (int64) enclosing.size());

        return Span<T> { enclosing.begin() + r.getStart(), (size_t) r.getLength() };
    }

private:
    Span<T> enclosing;
};

template <typename T>
static auto makeSubSpanLookup (Span<T> s) { return SubSpanLookup<T> { s }; }

struct CanBreakBeforeIterator
{
    explicit CanBreakBeforeIterator (Span<const Unicode::Codepoint> s)
        : span (s),
          cursor (span.begin())
    {}

    const Unicode::Codepoint* next()
    {
        while (++cursor < span.begin() + span.size())
        {
            // Disallow soft break before a hard break
            const auto nextCodepointIsLinebreak = [&]
            {
                const auto nextCursor = cursor + 1;

                if (! (nextCursor < span.begin() + span.size()))
                    return false;

                return nextCursor->codepoint == 0x0a || nextCursor->codepoint == 0x0d;
            }();

            if (cursor->breaking == TextBreakType::soft && ! nextCodepointIsLinebreak)
                return cursor + 1;  // Use the same "can break before" semantics as Harfbuzz
        }

        return nullptr;
    }

    Span<const Unicode::Codepoint> span;
    const Unicode::Codepoint* cursor;
};

/*  Returns integers relative to the initialising Span's begin(), before which a linebreak is
    possible.

    Can be restricted to a sub-range using reset().
*/
struct IntegralCanBreakBeforeIterator
{
    explicit IntegralCanBreakBeforeIterator (Span<const Unicode::Codepoint> s)
        : span (s),
          it (span)
    {}

    void reset()
    {
        reset ({ std::numeric_limits<int64>::min(),  std::numeric_limits<int64>::max() });
    }

    void reset (Range<int64> r)
    {
        jassert ((size_t) r.getLength() <= span.size());

        restrictedTo = r;
        it = CanBreakBeforeIterator { span };
        rangeEndReturned = false;
    }

    std::optional<int64> next()
    {
        const auto intValue = [&] (auto p) { return (int64) std::distance (span.begin(), p); };

        for (auto* ptr = it.next(); ptr != nullptr; ptr = it.next())
        {
            const auto v = intValue (ptr);

            if (v > restrictedTo.getEnd())
                break;

            if (restrictedTo.getStart() < v && v <= restrictedTo.getEnd())
                return v;
        }

        if (std::exchange (rangeEndReturned, true) == false)
            return std::min ((int64) span.size(), restrictedTo.getEnd());

        return std::nullopt;
    }

private:
    Span<const Unicode::Codepoint> span;
    CanBreakBeforeIterator it;
    Range<int64> restrictedTo { std::numeric_limits<int64>::min(),  std::numeric_limits<int64>::max() };

    bool rangeEndReturned = false;
};

struct ShapingParams
{
    TextScript script;
    String language;
    TextDirection direction;
    Font resolvedFont;
};

struct LineAdvance
{
    float includingTrailingWhitespace;
    float maybeIgnoringWhitespace;
};

struct ConsumableGlyphs
{
public:
    ConsumableGlyphs (const String& stringIn,
                      Range<int64> rangeIn,
                      ShapingParams params)
        : string (stringIn),
          range (rangeIn),
          shapingParams (std::move (params))
    {
        reshape();
    }

    /*  If the break happens at a safe-to-break point as per HB, it will just discard the consumed
        range. Otherwise, it reshapes the remaining text.
    */
    void breakBeforeAndConsume (int64 codepointIndex)
    {
        jassert (codepointIndex >= range.getStart());

        range = range.withStart (codepointIndex);

        if (isSafeToBreakBefore (codepointIndex))
        {
            glyphs.erase (glyphs.begin(), iteratorWithAdvance (glyphs.begin(), *getGlyphIndexForCodepoint (codepointIndex)));
            recalculateAdvances();
        }
        else if (! range.isEmpty())
        {
            reshape();
        }
    }

    /*  Returns the glyphs starting from the first unconsumed glyph, and ending with the one that
        covers the requested input codepoint range.

        If the provided range end corresponds to an unsafe break, an empty Span will be returned.

        Should only be called with indices for which isSafeToBreakBefore() returns true.

        There is an exception to this, which is the "beyond end codepoint index", which returns all
        glyphs without reshaping.
    */
    Span<const ShapedGlyph> getGlyphs (int64 beyondEndCodepointIndex) const
    {
        if (beyondEndCodepointIndex == range.getEnd())
            return { glyphs };

        if (isSafeToBreakBefore (beyondEndCodepointIndex))
            return { glyphs.data(), *getGlyphIndexForCodepoint (beyondEndCodepointIndex) };

        return {};
    }

    /*  Returns false for the beyond end index, because the safety of breaking cannot be determined
        at this point.
    */
    bool isSafeToBreakBefore (int64 codepointIndex) const
    {
        if (auto i = getGlyphIndexForCodepoint (codepointIndex))
            return ! glyphs[*i].unsafeToBreak;

        return false;
    }

    /*  If this function returns a value that also means that it's safe to break before the provided
        codepoint. Otherwise, we couldn't meaningfully calculate the requested value.
    */
    std::optional<LineAdvance> getAdvanceXUpToBreakPointIfSafe (int64 breakBefore,
                                                                bool whitespaceShouldFitInLine) const
    {
        const auto breakBeforeGlyphIndex = [&]() -> std::optional<size_t>
        {
            if (breakBefore == range.getEnd())
                return cumulativeAdvanceX.size() - 1;

            if (isSafeToBreakBefore (breakBefore))
                return *getGlyphIndexForCodepoint (breakBefore);

            return std::nullopt;
        }();

        if (! breakBeforeGlyphIndex.has_value())
            return std::nullopt;

        const auto includingTrailingWhitespace = cumulativeAdvanceX [*breakBeforeGlyphIndex];

        if (! whitespaceShouldFitInLine)
        {
            for (auto i = (int64) *breakBeforeGlyphIndex; --i >= 0;)
            {
                if (! glyphs[(size_t) i].whitespace)
                    return LineAdvance { includingTrailingWhitespace, cumulativeAdvanceX[(size_t) i + 1] };
            }
        }

        return LineAdvance { includingTrailingWhitespace, includingTrailingWhitespace };
    }

    auto isEmpty() const
    {
        return range.getLength() == 0;
    }

    auto getCodepointRange() const
    {
        return range;
    }

private:
    std::optional<size_t> getGlyphIndexForCodepoint (int64 codepointIndex) const
    {
        const auto it = std::lower_bound (glyphs.cbegin(),
                                          glyphs.cend(),
                                          codepointIndex,
                                          [] (auto& elem, auto& value) { return elem.cluster < value; });

        if (it != glyphs.cend() && it->cluster == codepointIndex)
            return (size_t) std::distance (glyphs.cbegin(), it);

        return std::nullopt;
    }

    void reshape()
    {
        glyphs = lowLevelShape (string,
                                getCodepointRange(),
                                shapingParams.resolvedFont,
                                shapingParams.script,
                                shapingParams.language,
                                shapingParams.direction);

        recalculateAdvances();
    }

    void recalculateAdvances()
    {
        cumulativeAdvanceX.clear();
        cumulativeAdvanceX.reserve (glyphs.size() + 1);
        cumulativeAdvanceX.push_back (0.0f);

        for (const auto& glyph : glyphs)
            cumulativeAdvanceX.push_back (glyph.advance.getX()
                                          + (cumulativeAdvanceX.empty() ? 0.0f : cumulativeAdvanceX.back()));
    }

    const String& string;
    Range<int64> range;
    ShapingParams shapingParams;
    std::vector<ShapedGlyph> glyphs;
    std::vector<float> cumulativeAdvanceX;
};

static bool isLtr (int bidiNestingLevel)
{
    return (bidiNestingLevel & 1) == 0;
}

struct LineChunkInLogicalOrder
{
    Range<int64> textRange;
    std::vector<ShapedGlyph> glyphs;
    Font resolvedFont;
    int bidiLevel{};
};

static auto getStartingVisualIndices (const std::vector<LineChunkInLogicalOrder>& chunks,
                                      const Array<Unicode::Codepoint>& analysis)
{
    std::vector<size_t> indices (chunks.size());

    std::transform (chunks.cbegin(),
                    chunks.cend(),
                    indices.begin(),
                    [&analysis] (auto& c) { return analysis[(int) c.textRange.getStart()].visualIndex; });

    return indices;
}

static auto getChunkIndicesInVisualOrder (const std::vector<LineChunkInLogicalOrder>& chunks,
                                          const Array<Unicode::Codepoint>& analysis)
{
    const auto startingVisualIndices = getStartingVisualIndices (chunks, analysis);

    struct ChunkIndexWithVisualIndex
    {
        size_t chunkIndex{};
        size_t visualIndex{};
    };

    std::vector<ChunkIndexWithVisualIndex> sortableIndices;
    sortableIndices.reserve (std::size (startingVisualIndices));

    for (const auto [i, visualIndex] : enumerate (startingVisualIndices))
        sortableIndices.push_back (ChunkIndexWithVisualIndex { (size_t) i, visualIndex });

    std::sort (sortableIndices.begin(),
               sortableIndices.end(),
               [] (const auto& a, const auto& b) { return a.visualIndex < b.visualIndex; });

    std::vector<size_t> result (std::size (sortableIndices));

    std::transform (sortableIndices.begin(),
                    sortableIndices.end(),
                    result.begin(),
                    [] (auto x) { return x.chunkIndex; });

    return result;
}

// Used to avoid signedness warning for types for which std::size() is int
template <typename T>
static auto makeSpan (T& array)
{
    return Span { array.getRawDataPointer(), (size_t) array.size() };
}

static std::vector<std::pair<Range<int64>, Font>> findSuitableFontsForText (const Font& font,
                                                                            const String& text,
                                                                            const String& language = {})
{
    detail::RangedValues<std::optional<Font>> fonts;
    fonts.set ({ 0, (int64) text.length() }, font);

    const auto getResult = [&]
    {
        std::vector<std::pair<Range<int64>, Font>> result;

        for (const auto [r, v] : fonts)
            result.emplace_back (r, v.value_or (font));

        return result;
    };

    if (! font.getFallbackEnabled())
        return getResult();

    const auto markMissingGlyphs = [&]
    {
        auto it = text.begin();
        std::vector<int64> fontNotFound;

        for (const auto [r, f] : fonts)
        {
            for (auto i = r.getStart(); i < r.getEnd(); ++i)
            {
                if (f.has_value() && ! isFontSuitableForCodepoint (*f, *it))
                    fontNotFound.push_back (i);

                ++it;
            }
        }

        for (const auto i : fontNotFound)
            fonts.set ({ i, i + 1 }, std::nullopt);

        return fontNotFound.size();
    };

    // We keep calling findSuitableFontForText for sub-ranges without a suitable font until we
    // can't find any more suitable fonts or all codepoints have one
    for (auto numMissingGlyphs = markMissingGlyphs(); numMissingGlyphs > 0;)
    {
        std::vector<std::pair<Range<int64>, Font>> changes;

        for (const auto [r, f] : fonts)
        {
            if (! f.has_value())
            {
                changes.emplace_back (r, font.findSuitableFontForText (text.substring ((int) r.getStart(),
                                                                                       (int) r.getEnd()),
                                                                       language));
            }
        }

        for (const auto& c : changes)
            fonts.set (c.first, c.second);

        if (const auto newNumMissingGlyphs = markMissingGlyphs();
            std::exchange (numMissingGlyphs, newNumMissingGlyphs) == newNumMissingGlyphs)
        {
            // We failed to resolve any more fonts during the last pass
            break;
        }
    }

    return getResult();
}

// TODO(ati) Use glyphNotFoundCharacter
void SimpleShapedText::shape (const String& data,
                              const ShapedTextOptions& options)
{
    const auto fonts = [&]
    {
        RangedValues<Font> result;

        for (const auto& [range, font] : options.getFontsForRange())
            result.insert ({ (int64) range.getStart(), (int64) range.getEnd() }, font);

        return result;
    }();

    std::vector<LineChunkInLogicalOrder> lineChunks;
    int64 numGlyphsInLine = 0;

    const auto analysis = Unicode::performAnalysis (data, options.getReadingDirection());

    IntegralCanBreakBeforeIterator softBreakIterator { makeSpan (analysis) };

    const auto spanLookup = makeSubSpanLookup (makeSpan (analysis));

    auto remainingWidth = options.getMaxWidth().has_value() ? (*options.getMaxWidth() - options.getFirstLineIndent())
                                                            : std::optional<float>{};

    const auto commitLine = [&]
    {
        const auto indicesInVisualOrder = getChunkIndicesInVisualOrder (lineChunks, analysis);

        for (auto chunkIndex : indicesInVisualOrder)
        {
            auto& chunk = lineChunks[chunkIndex];

            const auto glyphRange = Range<int64>::withStartAndLength ((int64) glyphsInVisualOrder.size(),
                                                                      (int64) chunk.glyphs.size());

            if (isLtr (chunk.bidiLevel))
                glyphsInVisualOrder.insert (glyphsInVisualOrder.end(), chunk.glyphs.begin(), chunk.glyphs.end());
            else
                glyphsInVisualOrder.insert (glyphsInVisualOrder.end(), chunk.glyphs.rbegin(), chunk.glyphs.rend());

            resolvedFonts.insert ({ glyphRange.getStart(), glyphRange.getEnd() }, chunk.resolvedFont);

            glyphLookup.set<MergeEqualItems::no> (chunk.textRange,
                                                  { glyphRange, isLtr (chunk.bidiLevel) });
        }

        lineChunks.clear();

        const auto [lineRange, lineNumber] = [&]
        {
            const auto lineRangeStart = lineNumbers.isEmpty() ? (int64) 0 : lineNumbers.getRanges().get (lineNumbers.getRanges().size() - 1).getEnd();
            const auto lineRangeEnd = lineRangeStart + numGlyphsInLine;
            const auto numLine = lineNumbers.isEmpty() ? (int64) 0 : lineNumbers.getItem (lineNumbers.size() - 1).value + 1;

            return std::make_pair (Range<int64> { lineRangeStart, lineRangeEnd }, numLine);
        }();

        if (const auto numLines = (int64) lineNumbers.size();
            numLines == 0 || numLines < options.getMaxNumLines())
        {
            lineNumbers.insert (lineRange, lineNumber);
        }
        else
        {
            const auto lastLine = lineNumbers.getItem (lineNumbers.size() - 1);

            jassert (lineRange.getStart() >= lastLine.range.getEnd());

            lineNumbers.set ({ lastLine.range.getStart(), lineRange.getEnd() }, lastLine.value);
        }

        numGlyphsInLine = 0;
        remainingWidth = options.getMaxWidth();
    };

    const auto append = [&] (Range<int64> range, const ShapingParams& shapingParams)
    {
        jassert (! range.isEmpty());

        auto glyphsToConsume = ConsumableGlyphs { data, range, shapingParams };

        while (! glyphsToConsume.isEmpty())
        {
            const auto remainingCodepointsToConsume = glyphsToConsume.getCodepointRange();
            softBreakIterator.reset (remainingCodepointsToConsume);

            struct BestMatch
            {
                int64 breakBefore{};

                // We need to use maybeIgnoringWhitespace in comparisions, but
                // includingTrailingWhitespace when using subtraction to calculate the remaining
                // space.
                LineAdvance advance{};

                bool unsafe{};
                std::vector<ShapedGlyph> unsafeGlyphs;
            };

            std::optional<BestMatch> bestMatch;

            for (auto breakBefore = softBreakIterator.next();
                 breakBefore.has_value() && (lineNumbers.size() == 0
                                             || (int64) lineNumbers.size() < options.getMaxNumLines() - 1);
                 breakBefore = softBreakIterator.next())
            {
                if (auto safeAdvance = glyphsToConsume.getAdvanceXUpToBreakPointIfSafe (*breakBefore,
                                                                                        options.getTrailingWhitespacesShouldFit()))
                {
                    if (safeAdvance->maybeIgnoringWhitespace < remainingWidth || ! bestMatch.has_value())
                        bestMatch = BestMatch { *breakBefore, *safeAdvance, false, std::vector<ShapedGlyph> {} };
                    else
                        break;  // We found a safe break that is too large to fit. Anything beyond
                                // this point would be too large to fit.
                }
                else
                {
                    auto glyphs = lowLevelShape (data,
                                                 remainingCodepointsToConsume.withEnd (*breakBefore),
                                                 shapingParams.resolvedFont,
                                                 shapingParams.script,
                                                 shapingParams.language,
                                                 shapingParams.direction);

                    const auto beyondEnd = [&]
                    {
                        if (options.getTrailingWhitespacesShouldFit())
                            return glyphs.cend();

                        auto it = glyphs.cend();

                        while (--it != glyphs.begin())
                        {
                            if (! it->whitespace)
                                return it;
                        }

                        return it;
                    }();

                    const auto advance = std::accumulate (glyphs.cbegin(),
                                                          beyondEnd,
                                                          float {},
                                                          [] (auto acc, const auto& elem) { return acc + elem.advance.getX(); });

                    if (advance < remainingWidth || ! bestMatch.has_value())
                        bestMatch = BestMatch { *breakBefore, { advance, advance }, true, std::move (glyphs) };
                }
            }

            // Failed to break anywhere, we need to consume all that's left
            if (! bestMatch.has_value())
            {
                bestMatch = BestMatch { glyphsToConsume.getCodepointRange().getEnd(),
                                        *glyphsToConsume.getAdvanceXUpToBreakPointIfSafe (glyphsToConsume.getCodepointRange().getEnd(),
                                                                                          options.getTrailingWhitespacesShouldFit()),
                                        false,
                                        std::vector<ShapedGlyph> {} };
            }

            jassert (bestMatch.has_value());

            const auto consumeBestMatch = [&]
            {
                auto glyphs = [&]
                {
                    if (bestMatch->unsafe)
                        return Span<const ShapedGlyph> { bestMatch->unsafeGlyphs };

                    return glyphsToConsume.getGlyphs (bestMatch->breakBefore);
                }();

                const auto textRange = glyphsToConsume.getCodepointRange().withEnd (bestMatch->breakBefore);

                const auto createFakeBidiNestingLevel = [] (TextDirection dir) { return dir == TextDirection::ltr ? 0 : 1; };

                lineChunks.push_back ({ textRange,
                                        { glyphs.begin(), glyphs.end() },
                                        shapingParams.resolvedFont,
                                        createFakeBidiNestingLevel (shapingParams.direction) });

                numGlyphsInLine += (int64) glyphs.size();

                if (remainingWidth.has_value())
                    remainingWidth = *remainingWidth - bestMatch->advance.includingTrailingWhitespace;

                glyphsToConsume.breakBeforeAndConsume (bestMatch->breakBefore);
            };

            if (bestMatch->advance.maybeIgnoringWhitespace >= remainingWidth)
            {
                // Even an empty line is too short to fit any of the text
                if (numGlyphsInLine == 0 && exactlyEqual (remainingWidth, options.getMaxWidth()))
                    consumeBestMatch();

                commitLine();
            }
            else
            {
                consumeBestMatch();

                if (! glyphsToConsume.isEmpty())
                    commitLine();
            }
        }
    };

    const auto fontsWithFallback = [&]
    {
        RangedValues<Font> resolved;

        for (const auto [r, f] : fonts)
        {
            auto rf = findSuitableFontsForText (f, data.substring ((int) r.getStart(),
                                                                   (int) std::min (r.getEnd(), (int64) data.length())));

            for (auto& item : rf)
                item.first += r.getStart();

            resolved.setForEach<MergeEqualItems::no> (rf.begin(), rf.end());
        }

        return resolved;
    }();

    for (Unicode::LineBreakIterator lineIter { makeSpan (analysis) }; auto lineRun = lineIter.next();)
    {
        for (Unicode::ScriptRunIterator scriptIter { *lineRun };
             auto scriptRun = scriptIter.next();)
        {
            for (Unicode::BidiRunIterator bidiIter { *scriptRun }; auto bidiRun = bidiIter.next();)
            {
                for (const auto& [range, font] : fontsWithFallback.getIntersectionsWith (spanLookup.getRange (*bidiRun)))
                {
                    append (range, { scriptRun->front().script,
                                     options.getLanguage(),
                                     bidiRun->front().direction,
                                     font });
                }
            }
        }

        if (! lineChunks.empty())
            commitLine();
    }

    if (! lineChunks.empty())
        commitLine();
}

Range<int64> SimpleShapedText::getGlyphRangeForLine (size_t line) const
{
    jassert (line <= lineNumbers.size());
    return lineNumbers.getItem (line).range;
}

std::vector<FontForRange> SimpleShapedText::getResolvedFontsIntersectingGlyphRange (Range<int64> glyphRange) const
{
    std::vector<FontForRange> result;

    for (const auto& item : resolvedFonts.getIntersectionsWith (glyphRange))
        result.push_back ({ item.range, item.value });

    return result;
}

Span<const ShapedGlyph> SimpleShapedText::getGlyphs (Range<int64> glyphRange) const
{
    const auto r = glyphRange.getIntersectionWith ({ 0, (int64) glyphsInVisualOrder.size() });

    return { glyphsInVisualOrder.data() + r.getStart(), (size_t) r.getLength() };
}

Span<const ShapedGlyph> SimpleShapedText::getGlyphs() const
{
    return glyphsInVisualOrder;
}

juce_wchar SimpleShapedText::getCodepoint (int64 glyphIndex) const
{
    return string[(int) glyphsInVisualOrder[(size_t) glyphIndex].cluster];
}

Range<int64> SimpleShapedText::getTextRange (int64 glyphIndex) const
{
    jassert (isPositiveAndBelow (glyphIndex, getNumGlyphs()));

    // A single glyph can span multiple input codepoints. We can discover this by checking the
    // neighbouring glyphs cluster values. If neighbouring values differ by more than one, then the
    // missing clusters belong to a single glyph.
    //
    // However, we only have to check glyphs that are in the same bidi run as this one, hence the
    // lookup.
    const auto startingCodepoint = glyphsInVisualOrder[(size_t) glyphIndex].cluster;
    const auto glyphRange = glyphLookup.getItemWithEnclosingRange (startingCodepoint)->value.glyphRange;

    const auto glyphRun = Span<const ShapedGlyph> { glyphsInVisualOrder.data() + glyphRange.getStart(),
                                                    (size_t) glyphRange.getLength() };

    const auto indexInRun = glyphIndex - glyphRange.getStart();

    const auto cluster = glyphRun[(size_t) indexInRun].cluster;

    const auto nextAdjacentCluster = [&]
    {
        auto left = [&]
        {
            for (auto i = indexInRun; i >= 0; --i)
                if (auto c = glyphRun[(size_t) i].cluster; c != cluster)
                    return c;

            return cluster;
        }();

        auto right = [&]
        {
            for (auto i = indexInRun; i < (decltype (i)) glyphRun.size(); ++i)
                if (auto c = glyphRun[(size_t) i].cluster; c != cluster)
                    return c;

            return cluster;
        }();

        return std::max (left, right);
    }();

    return Range<int64>::withStartAndLength (cluster, std::max ((int64) 1, nextAdjacentCluster - cluster));
}

} // namespace juce
