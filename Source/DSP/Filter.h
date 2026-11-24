#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

namespace bgdj
{
enum class FilterType : int
{
    LP12 = 0, LP24, LP36, HP12, HP24, BP12, BP24, Notch12, Notch24, Peak,
    AllPass, LowShelf, HighShelf, LadderLP24, LadderHP24, Acid303, DiodeLP,
    MS20LP, MS20HP, DirtyLP12, DirtyLP24, DirtyBP, CombPlus, CombMinus, FlangeComb,
    FormantVowel, FormantTalker, Phaser4, Phaser8, Phaser12, LPHPMorph, DualBP,
    Count
};

enum class SatMode : int { Tanh = 0, SoftClip, Asymmetric, Tube };

inline float saturate (float x, SatMode m) noexcept
{
    switch (m)
    {
        case SatMode::SoftClip:   { const float c = juce::jlimit (-1.5f, 1.5f, x); return c - c * c * c * (4.0f / 27.0f); }
        case SatMode::Asymmetric: return std::tanh (x + 0.35f) - 0.33637554f;
        case SatMode::Tube:       return x >= 0.0f ? 1.0f - std::exp (-x) : -0.8f * (1.0f - std::exp (1.25f * x));
        case SatMode::Tanh:
        default:                  return std::tanh (x);
    }
}

/** Zero-delay-feedback state-variable filter (Zavalishin topology). */
struct Svf
{
    float ic1 = 0.0f, ic2 = 0.0f;
    float a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, k = 1.414f;

    void set (float g, float kk) noexcept
    {
        k = kk;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    struct Out { float lp, bp, hp; };

    Out process (float v0) noexcept
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return { v2, v1, v0 - k * v1 - v2 };
    }

    Out processDirty (float v0, SatMode m) noexcept
    {
        auto o = process (v0);
        ic1 = saturate (ic1, m);
        return o;
    }

    void reset() noexcept { ic1 = ic2 = 0.0f; }
};

/** TPT one-pole. */
struct OnePoleTpt
{
    float s = 0.0f, G = 0.0f;
    void set (float g) noexcept { G = g / (1.0f + g); }
    float lp (float x) noexcept { const float v = (x - s) * G; const float y = v + s; s = y + v; return y; }
    float hp (float x) noexcept { return x - lp (x); }
    float ap (float x) noexcept { return 2.0f * lp (x) - x; }
    void reset() noexcept { s = 0.0f; }
};

/** One stereo filter slot inside a voice: 32 models. */
class VoiceFilter
{
public:
    void prepare (double sampleRate, int maxCombSamples);
    /** Changes the rate without reallocating (comb length is capped by the prepared size). */
    void setSampleRate (double sampleRate) noexcept { fs = sampleRate; }
    void reset() noexcept;

    /** Recomputes coefficients. Call once per control sub-block. */
    void setParams (FilterType type, float cutoffHz, float reso, float drive, float fat, SatMode sat) noexcept;

    void process (float& l, float& r) noexcept
    {
        l = processChannel (0, l);
        r = processChannel (1, r);
    }

    float processMono (float x) noexcept { return processChannel (0, x); }

private:
    struct Channel
    {
        std::array<Svf, 5> svf;
        std::array<OnePoleTpt, 12> stage;
        OnePoleTpt fbHp, fatLp;
        float lastOut = 0.0f;
        std::vector<float> comb;
        int combPos = 0;
    };

    float processChannel (int ch, float x) noexcept;
    float readComb (Channel& c, float delay) const noexcept;

    std::array<Channel, 2> chans;
    double fs = 48000.0;
    int combSize = 0;

    FilterType type = FilterType::LP24;
    SatMode sat = SatMode::Tanh;
    float g = 0.1f, k = 1.4f, reso = 0.0f, drive = 0.0f, fat = 0.0f, driveGain = 1.0f;
    float ladderFb = 0.0f, combDelay = 100.0f, combFb = 0.0f, shelfGain = 1.0f, morph = 0.0f;
    int numStages = 4;
    std::array<float, 5> formantG {}, formantGain {};
    float formantK = 0.2f;
    float g2 = 0.1f;
};
} // namespace bgdj
