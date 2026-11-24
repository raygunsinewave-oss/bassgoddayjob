#include "Shapes.h"

namespace bgdj::shapes
{
CurvePoints lfoPreset (int shape)
{
    switch (shape)
    {
        case 0: { CurvePoints p; for (int i = 0; i <= 32; ++i) { const float x = (float) i / 32.0f; p.push_back ({ x, 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * x), 0.0f }); } return p; }
        case 1:  return { { 0.0f, 0.0f, 0 }, { 0.5f, 1.0f, 0 }, { 1.0f, 0.0f, 0 } };
        case 2:  return { { 0.0f, 0.0f, 0 }, { 1.0f, 1.0f, 0 } };
        case 3:  return { { 0.0f, 1.0f, 0 }, { 1.0f, 0.0f, 0 } };
        case 4:  return { { 0.0f, 1.0f, 0 }, { 0.5f, 1.0f, 0 }, { 0.5001f, 0.0f, 0 }, { 1.0f, 0.0f, 0 } };
        case 5:  return { { 0.0f, 1.0f, 0 }, { 0.2f, 1.0f, 0 }, { 0.2001f, 0.0f, 0 }, { 1.0f, 0.0f, 0 } };
        case 8:  return { { 0.0f, 0.0f, 0 }, { 0.12f, 1.0f, -0.6f }, { 0.55f, 0.35f, 0.5f }, { 1.0f, 0.0f, 0.6f } };          // Wobble
        case 9:  { CurvePoints p; for (int s = 0; s < 8; ++s) { p.push_back ({ s / 8.0f, s / 7.0f, 0 }); p.push_back ({ (s + 1) / 8.0f - 0.0001f, s / 7.0f, 0 }); } return p; }
        case 10: { CurvePoints p; for (int i = 0; i <= 32; ++i) { const float x = (float) i / 32.0f; p.push_back ({ x, std::abs (std::sin (juce::MathConstants<float>::pi * x * 3.0f)) * (1.0f - x * 0.6f), 0.0f }); } return p; }
        case 11: return { { 0.0f, 0.0f, 0 }, { 0.25f, 1.0f, -0.4f }, { 0.75f, 0.2f, 0.3f }, { 1.0f, 0.0f, 0 } };             // Custom starting point
        default: return { { 0.0f, 0.5f, 0 }, { 1.0f, 0.5f, 0 } };
    }
}

const std::vector<NamedShape>& wobblePresets()
{
    static const std::vector<NamedShape> w = {
        { "Wobble 1/4 Classic", 7,  { { 0, 0, 0 }, { 0.15f, 1, -0.5f }, { 0.6f, 0.3f, 0.4f }, { 1, 0, 0.5f } } },
        { "Wobble 1/8 Yoy", 10,     { { 0, 0.1f, 0 }, { 0.3f, 1, -0.7f }, { 1, 0.1f, 0.7f } } },
        { "Wobble 1/8T Triplet", 12, { { 0, 0, 0 }, { 0.5f, 1, 0 }, { 1, 0, 0 } } },
        { "Wobble 1/16 Rapid", 13,  { { 0, 1, 0 }, { 0.5f, 0.1f, 0.6f }, { 1, 1, -0.6f } } },
        { "Wobble 1/16T Riddim", 15, { { 0, 0, 0 }, { 0.05f, 1, 0 }, { 0.45f, 1, 0 }, { 0.5f, 0, 0 }, { 1, 0, 0 } } },
        { "Wobble 1/2 Slow Yawn", 4, { { 0, 0, 0 }, { 0.7f, 1, -0.3f }, { 1, 0, 0.8f } } },
        { "Wobble 1/4D Lurch", 8,   { { 0, 0.2f, 0 }, { 0.25f, 0.9f, -0.5f }, { 0.5f, 0.4f, 0.2f }, { 0.75f, 1.0f, -0.3f }, { 1, 0.2f, 0.4f } } },
        { "Wobble 1/8D Bounce", 11, { { 0, 0, 0 }, { 0.1f, 1, -0.8f }, { 0.4f, 0.0f, 0.8f }, { 0.55f, 0.6f, -0.6f }, { 1, 0, 0.6f } } },
        { "Wobble 1/32 Buzz", 16,   { { 0, 0, 0 }, { 0.5f, 1, 0 }, { 1, 0, 0 } } },
        { "Wobble 1 Bar Swell", 3,  { { 0, 0, 0 }, { 1, 1, 0.5f } } },
    };
    return w;
}

CurvePoints duckPreset (int shape)
{
    switch (shape)
    {
        case 0:  return { { 0, 0, 0 }, { 0.02f, 0, 0 }, { 0.35f, 1, -0.6f }, { 1, 1, 0 } };           // Kick Duck Hard
        case 1:  return { { 0, 0.2f, 0 }, { 0.55f, 1, -0.4f }, { 1, 1, 0 } };                        // Kick Duck Soft
        case 2:  return { { 0, 0, 0 }, { 0.85f, 1, -0.2f }, { 1, 1, 0 } };                           // Long Pump
        case 3:  return { { 0, 0, 0 }, { 0.45f, 0, 0 }, { 0.46f, 1, 0 }, { 0.95f, 1, 0 }, { 1, 0, 0 } }; // Gate Chop
        default: return { { 0, 0, 0 }, { 0.3f, 1, -0.5f }, { 1, 1, 0 } };
    }
}

juce::var pointsToVar (const CurvePoints& p)
{
    juce::Array<juce::var> arr;
    for (auto& pt : p)
        arr.add (juce::Array<juce::var> { pt.x, pt.y, pt.curve });
    return arr;
}

CurvePoints pointsFromVar (const juce::var& v)
{
    CurvePoints p;
    if (auto* arr = v.getArray())
        for (auto& e : *arr)
            if (auto* t = e.getArray(); t != nullptr && t->size() >= 2)
                p.push_back ({ juce::jlimit (0.0f, 1.0f, (float) (*t)[0]), juce::jlimit (0.0f, 1.0f, (float) (*t)[1]),
                               t->size() > 2 ? juce::jlimit (-1.0f, 1.0f, (float) (*t)[2]) : 0.0f });
    std::sort (p.begin(), p.end(), [] (auto& a, auto& b) { return a.x < b.x; });
    return p;
}
} // namespace bgdj::shapes
