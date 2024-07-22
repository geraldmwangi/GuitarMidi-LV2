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

class ScopedMultithread
{
public:
    explicit ScopedMultithread (ID2D1Multithread* multithreadIn)
        : multithread (addComSmartPtrOwner (multithreadIn))
    {
        multithreadIn->Enter();
    }

    ~ScopedMultithread()
    {
        multithread->Leave();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopedMultithread)

    ComSmartPtr<ID2D1Multithread> multithread;
};

/*  ScopedGeometryWithSink creates an ID2D1PathGeometry object with an open sink. */
struct ScopedGeometryWithSink
{
    ScopedGeometryWithSink (ID2D1Factory* factory, D2D1_FILL_MODE fillMode)
    {
        if (const auto hr = factory->CreatePathGeometry (geometry.resetAndGetPointerAddress()); FAILED (hr))
            return;

        if (const auto hr = geometry->Open (sink.resetAndGetPointerAddress()); FAILED (hr))
            return;

        sink->SetFillMode (fillMode);
    }

    ~ScopedGeometryWithSink()
    {
        if (sink == nullptr)
            return;

        const auto hr = sink->Close();
        jassertquiet (SUCCEEDED (hr));
    }

    ComSmartPtr<ID2D1PathGeometry> geometry;
    ComSmartPtr<ID2D1GeometrySink> sink;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopedGeometryWithSink)
};

class WindowsScopedEvent
{
public:
    explicit WindowsScopedEvent (HANDLE handleIn)
        : handle (handleIn)
    {
    }

    WindowsScopedEvent()
        : WindowsScopedEvent (CreateEvent (nullptr, FALSE, FALSE, nullptr))
    {
    }

    HANDLE getHandle() const noexcept
    {
        return handle.get();
    }

private:
    std::unique_ptr<std::remove_pointer_t<HANDLE>, FunctionPointerDestructor<CloseHandle>> handle;
};

//==============================================================================
struct D2DHelpers
{
    static bool isTransformAxisAligned (const AffineTransform& transform)
    {
        return transform.mat01 == 0.0f && transform.mat10 == 0.0f;
    }

    static void pathToGeometrySink (const Path& path, ID2D1GeometrySink* sink, const AffineTransform& transform, D2D1_FIGURE_BEGIN figureMode)
    {
        // Every call to BeginFigure must have a matching call to EndFigure. But - the Path does not necessarily
        // have matching startNewSubPath and closePath markers. The figureStarted flag indicates if an extra call
        // to BeginFigure or EndFigure is needed during the iteration loop or when exiting this function.
        Path::Iterator it (path);
        bool figureStarted = false;

        while (it.next())
        {
            switch (it.elementType)
            {
                case Path::Iterator::cubicTo:
                {
                    if (! figureStarted)
                        break;

                    transform.transformPoint (it.x1, it.y1);
                    transform.transformPoint (it.x2, it.y2);
                    transform.transformPoint (it.x3, it.y3);

                    sink->AddBezier ({ { it.x1, it.y1 }, { it.x2, it.y2 }, { it.x3, it.y3 } });
                    break;
                }

                case Path::Iterator::lineTo:
                {
                    if (! figureStarted)
                        break;

                    transform.transformPoint (it.x1, it.y1);
                    sink->AddLine ({ it.x1, it.y1 });
                    break;
                }

                case Path::Iterator::quadraticTo:
                {
                    if (! figureStarted)
                        break;

                    transform.transformPoint (it.x1, it.y1);
                    transform.transformPoint (it.x2, it.y2);
                    sink->AddQuadraticBezier ({ { it.x1, it.y1 }, { it.x2, it.y2 } });
                    break;
                }

                case Path::Iterator::closePath:
                {
                    if (figureStarted)
                    {
                        sink->EndFigure (D2D1_FIGURE_END_CLOSED);
                        figureStarted = false;
                    }
                    break;
                }

                case Path::Iterator::startNewSubPath:
                {
                    if (figureStarted)
                        sink->EndFigure (D2D1_FIGURE_END_OPEN);

                    transform.transformPoint (it.x1, it.y1);
                    sink->BeginFigure ({ it.x1, it.y1 }, figureMode);

                    figureStarted = true;
                    break;
                }
            }
        }

        if (figureStarted)
        {
            sink->EndFigure (D2D1_FIGURE_END_OPEN);
        }
    }

    static D2D1_POINT_2F pointTransformed (Point<float> pt, const AffineTransform& transform)
    {
        transform.transformPoint (pt.x, pt.y);
        return { (FLOAT) pt.x, (FLOAT) pt.y };
    }

    static void rectToGeometrySink (const Rectangle<float>& rect,
                                    ID2D1GeometrySink* sink,
                                    const AffineTransform& transform,
                                    D2D1_FIGURE_BEGIN figureMode)
    {
        const auto a = pointTransformed (rect.getTopLeft(),     transform);
        const auto b = pointTransformed (rect.getTopRight(),    transform);
        const auto c = pointTransformed (rect.getBottomRight(), transform);
        const auto d = pointTransformed (rect.getBottomLeft(),  transform);

        sink->BeginFigure (a, figureMode);
        sink->AddLine (b);
        sink->AddLine (c);
        sink->AddLine (d);
        sink->EndFigure (D2D1_FIGURE_END_CLOSED);
    }

    static ComSmartPtr<ID2D1Geometry> rectListToPathGeometry (ID2D1Factory* factory,
                                                              const RectangleList<float>& clipRegion,
                                                              const AffineTransform& transform,
                                                              D2D1_FILL_MODE fillMode,
                                                              D2D1_FIGURE_BEGIN figureMode,
                                                              [[maybe_unused]] Direct2DMetrics* metrics)
    {
        JUCE_D2DMETRICS_SCOPED_ELAPSED_TIME (metrics, createGeometryTime)
        ScopedGeometryWithSink objects { factory, fillMode };

        if (objects.sink == nullptr)
            return {};

        for (int i = clipRegion.getNumRectangles(); --i >= 0;)
            rectToGeometrySink (clipRegion.getRectangle (i), objects.sink, transform, figureMode);

        return objects.geometry;
    }

    static ComSmartPtr<ID2D1Geometry> pathToPathGeometry (ID2D1Factory* factory,
                                                          const Path& path,
                                                          const AffineTransform& transform,
                                                          D2D1_FIGURE_BEGIN figureMode,
                                                          [[maybe_unused]] Direct2DMetrics* metrics)
    {
        JUCE_D2DMETRICS_SCOPED_ELAPSED_TIME (metrics, createGeometryTime)
        ScopedGeometryWithSink objects { factory, path.isUsingNonZeroWinding() ? D2D1_FILL_MODE_WINDING : D2D1_FILL_MODE_ALTERNATE };

        if (objects.sink == nullptr)
            return {};

        pathToGeometrySink (path, objects.sink, transform, figureMode);

        return objects.geometry;
    }

    static ComSmartPtr<ID2D1StrokeStyle1> pathStrokeTypeToStrokeStyle (ID2D1Factory1* factory, const PathStrokeType& strokeType)
    {
        // JUCE JointStyle   ID2D1StrokeStyle
        // ---------------   ----------------
        // mitered           D2D1_LINE_JOIN_MITER
        // curved            D2D1_LINE_JOIN_ROUND
        // beveled           D2D1_LINE_JOIN_BEVEL
        //
        // JUCE EndCapStyle  ID2D1StrokeStyle
        // ----------------  ----------------
        // butt              D2D1_CAP_STYLE_FLAT
        // square            D2D1_CAP_STYLE_SQUARE
        // rounded           D2D1_CAP_STYLE_ROUND
        auto lineJoin = D2D1_LINE_JOIN_MITER;
        switch (strokeType.getJointStyle())
        {
            case PathStrokeType::JointStyle::mitered:
                // already set
                break;

            case PathStrokeType::JointStyle::curved:
                lineJoin = D2D1_LINE_JOIN_ROUND;
                break;

            case PathStrokeType::JointStyle::beveled:
                lineJoin = D2D1_LINE_JOIN_BEVEL;
                break;

            default:
                // invalid EndCapStyle
                jassertfalse;
                break;
        }

        auto capStyle = D2D1_CAP_STYLE_FLAT;
        switch (strokeType.getEndStyle())
        {
            case PathStrokeType::EndCapStyle::butt:
                // already set
                break;

            case PathStrokeType::EndCapStyle::square:
                capStyle = D2D1_CAP_STYLE_SQUARE;
                break;

            case PathStrokeType::EndCapStyle::rounded:
                capStyle = D2D1_CAP_STYLE_ROUND;
                break;

            default:
                // invalid EndCapStyle
                jassertfalse;
                break;
        }

        D2D1_STROKE_STYLE_PROPERTIES1 strokeStyleProperties
                {
                        capStyle,
                        capStyle,
                        capStyle,
                        lineJoin,
                        strokeType.getStrokeThickness(),
                        D2D1_DASH_STYLE_SOLID,
                        0.0f,
                        D2D1_STROKE_TRANSFORM_TYPE_NORMAL
                };
        ComSmartPtr<ID2D1StrokeStyle1> strokeStyle;
        factory->CreateStrokeStyle (strokeStyleProperties,
                                    nullptr,
                                    0,
                                    strokeStyle.resetAndGetPointerAddress());

        return strokeStyle;
    }

};

//==============================================================================
/*  UpdateRegion extracts the invalid region for a window
    UpdateRegion is used to service WM_PAINT to add the invalid region of a window to
    deferredRepaints. UpdateRegion marks the region as valid, and the region should be painted on the
    next vblank.
    This is similar to the invalid region update in HWNDComponentPeer::handlePaintMessage()
*/
class UpdateRegion
{
public:
    ~UpdateRegion()
    {
        clear();
    }

    void findRECTAndValidate (HWND windowHandle)
    {
        numRect = 0;

        auto regionHandle = CreateRectRgn (0, 0, 0, 0);

        if (regionHandle)
        {
            auto regionType = GetUpdateRgn (windowHandle, regionHandle, false);

            if (regionType == SIMPLEREGION || regionType == COMPLEXREGION)
            {
                auto regionDataBytes = GetRegionData (regionHandle, (DWORD) block.getSize(), (RGNDATA*) block.getData());

                if (regionDataBytes > block.getSize())
                {
                    block.ensureSize (regionDataBytes);
                    regionDataBytes = GetRegionData (regionHandle, (DWORD) block.getSize(), (RGNDATA*) block.getData());
                }

                if (regionDataBytes > 0)
                {
                    auto header = (RGNDATAHEADER const* const) block.getData();

                    if (header->iType == RDH_RECTANGLES)
                        numRect = header->nCount;
                }
            }

            if (numRect > 0)
                ValidateRgn (windowHandle, regionHandle);
            else
                ValidateRect (windowHandle, nullptr);

            DeleteObject (regionHandle);
            regionHandle = nullptr;

            return;
        }

        ValidateRect (windowHandle, nullptr);
    }

    void clear()
    {
        numRect = 0;
    }

    uint32 getNumRECT() const
    {
        return numRect;
    }

    RECT* getRECTArray()
    {
        auto header = (RGNDATAHEADER const* const) block.getData();
        return (RECT*) (header + 1);
    }

    static void forwardInvalidRegionToParent (HWND childHwnd)
    {
        auto regionHandle = CreateRectRgn (0, 0, 0, 0);

        if (regionHandle)
        {
            GetUpdateRgn (childHwnd, regionHandle, false);
            ValidateRgn (childHwnd, regionHandle);
            InvalidateRgn (GetParent (childHwnd), regionHandle, FALSE);
            DeleteObject (regionHandle);
        }
    }

private:
    MemoryBlock block { 1024 };
    uint32 numRect = 0;
};

//==============================================================================
/** Heap storage for a DirectWrite glyph run */
class DirectWriteGlyphRun
{
public:
    void replace (Span<const Point<float>> positions, float scale)
    {
        advances.resize (positions.size(), 0.0f);
        offsets.resize (positions.size());
        std::transform (positions.begin(), positions.end(), offsets.begin(), [&] (auto& g)
        {
            return DWRITE_GLYPH_OFFSET { g.x / scale, -g.y };
        });
    }

    auto* getAdvances() const { return advances.data(); }
    auto* getOffsets()  const { return offsets .data(); }

private:
    std::vector<float> advances;
    std::vector<DWRITE_GLYPH_OFFSET> offsets;
};

} // namespace juce
