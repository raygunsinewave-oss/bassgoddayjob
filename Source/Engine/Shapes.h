#pragma once

#include "../Common/Parameters.h"
#include <array>
#include <vector>

namespace bgdj
{
/** Breakpoint of a drawable curve. x,y in 0..1, curve -1..1 bends the segment that ends here. */
struct CurvePoint
{
    float x = 0.0f, y = 0.0f, curve = 0.0f;
};

using CurvePoints = std::vector<CurvePoint>;

/** 256-entry lookup (plus guard) rendered from breakpoints, shared with the audio thread. */
struct ShapeTable
{
    std::array<float, kLfoTableSize + 1> v {};

    float lookup (float phase) const noexcept
    {
        const float p = juce::jlimit (0.0f, 1.0f, phase) * (float) kLfoTableSize;
        const int i = std::min ((int) p, kLfoTableSize - 1);
        const float f = p - (float) i;
        return v[(size_t) i] + (v[(size_t) i + 1] - v[(size_t) i]) * f;
    }
};

inline float bendSegment (float t, float curve) noexcept
{
    if (std::abs (curve) < 1.0e-3f)
        return t;
    const float k = curve * 6.0f;
    return (std::exp (k * t) - 1.0f) / (std::exp (k) - 1.0f);
}

inline ShapeTable renderCurve (const CurvePoints& pts)
{
    ShapeTable t;
    if (pts.empty())
        return t;
    for (int i = 0; i <= kLfoTableSize; ++i)
    {
        const float x = (float) i / (float) kLfoTableSize;
        size_t seg = 0;
        while (seg + 1 < pts.size() && pts[seg + 1].x < x)
            ++seg;
        if (x <= pts.front().x) { t.v[(size_t) i] = pts.front().y; continue; }
        if (seg + 1 >= pts.size()) { t.v[(size_t) i] = pts.back().y; continue; }
        const auto& a = pts[seg];
        const auto& b = pts[seg + 1];
        const float span = std::max (1.0e-6f, b.x - a.x);
        const float u = bendSegment ((x - a.x) / span, b.curve);
        t.v[(size_t) i] = a.y + (b.y - a.y) * u;
    }
    return t;
}

namespace shapes
{
    /** Built-in LFO shapes, by params::lfoShapes() index. Random shapes return flat tables. */
    CurvePoints lfoPreset (int shapeIndex);

    struct NamedShape { const char* name; int syncRate; CurvePoints points; };

    /** "Wobble Builder" presets: rate + curve combinations for classic bass wobbles. */
    const std::vector<NamedShape>& wobblePresets();

    /** Ducker shapes, by params::duckShapes() index (gain curve over one duck cycle). */
    CurvePoints duckPreset (int shapeIndex);

    juce::var pointsToVar (const CurvePoints& p);
    CurvePoints pointsFromVar (const juce::var& v);
}
} // namespace bgdj
