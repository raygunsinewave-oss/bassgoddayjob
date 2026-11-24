#include "Wavetable.h"

namespace bgdj::factory
{
namespace
{
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
    using Gen = std::function<float (float t, float x)>;   // t = frame position 0..1, x = phase 0..1

    struct Def
    {
        juce::String name;
        int frames;
        Gen gen;
    };

    float sine (float x)                 { return std::sin (kTwoPi * x); }
    float saw (float x)                  { return 2.0f * (x - std::floor (x)) - 1.0f; }
    float square (float x, float pw = 0.5f) { x -= std::floor (x); return x < pw ? 1.0f : -1.0f; }
    float tri (float x)                  { x -= std::floor (x); return x < 0.5f ? 4.0f * x - 1.0f : 3.0f - 4.0f * x; }
    float fold (float v)                 { while (std::abs (v) > 1.0f) v = v > 0 ? 2.0f - v : -2.0f - v; return v; }
    float lerp (float a, float b, float t) { return a + (b - a) * t; }

    float additive (float x, int maxH, const std::function<float (int)>& amp)
    {
        float s = 0.0f;
        for (int h = 1; h <= maxH; ++h)
            if (const float a = amp (h); a != 0.0f)
                s += a * std::sin (kTwoPi * (float) h * x);
        return s;
    }

    /** Harmonic weights shaped by Gaussian formant peaks (bass fundamental ~ 55 Hz). */
    float formantSeries (float x, const float* formants, int nForm, float bw, float tilt = 1.0f)
    {
        return additive (x, 96, [&] (int h)
        {
            const float f = 55.0f * (float) h;
            float a = 0.0f;
            for (int i = 0; i < nForm; ++i)
            {
                const float d = (f - formants[i]) / (bw * (1.0f + (float) i * 0.6f));
                a += std::exp (-0.5f * d * d) / (1.0f + (float) i * 0.7f);
            }
            return a / std::pow ((float) h, 0.25f * tilt);
        });
    }

    struct Vowel { float f1, f2, f3; };
    const Vowel vowels[] = { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 280, 2250, 2900 }, { 450, 800, 2830 }, { 325, 700, 2530 } };

    Vowel vowelAt (float pos)   // pos 0..4 over A E I O U
    {
        pos = juce::jlimit (0.0f, 3.999f, pos);
        const int i = (int) pos;
        const float f = pos - (float) i;
        return { lerp (vowels[i].f1, vowels[i + 1].f1, f), lerp (vowels[i].f2, vowels[i + 1].f2, f), lerp (vowels[i].f3, vowels[i + 1].f3, f) };
    }

    float vowelWave (float x, float pos, float bw = 120.0f)
    {
        const auto v = vowelAt (pos);
        const float fs[] = { v.f1, v.f2, v.f3 };
        return formantSeries (x, fs, 3, bw);
    }

    /** CZ-style phase-distortion resonance. */
    float czReso (float x, float t, int window)
    {
        const float r = 1.0f + t * 15.0f;
        const float carrier = std::cos (kTwoPi * std::fmod (x * r, 1.0f));
        float w = 1.0f - x;                               // saw window
        if (window == 1) w = x < 0.5f ? 1.0f : 0.0f;      // square window
        if (window == 2) w = 1.0f - std::abs (2.0f * x - 1.0f);
        return (1.0f - carrier) * w - 0.5f;
    }

    float hashNoise (int i, int seed)
    {
        uint32_t h = (uint32_t) i * 374761393u + (uint32_t) seed * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return (float) ((h ^ (h >> 16)) & 0xffff) / 32767.5f - 1.0f;
    }

    std::vector<Def> makeDefs()
    {
        std::vector<Def> d;
        auto add = [&d] (juce::String n, int frames, Gen g) { d.push_back ({ std::move (n), frames, std::move (g) }); };

        // ---- Basics ----------------------------------------------------------------
        add ("Sine", 1, [] (float, float x) { return sine (x); });
        add ("Saw", 1, [] (float, float x) { return saw (x); });
        add ("Square", 1, [] (float, float x) { return square (x); });
        add ("Triangle", 1, [] (float, float x) { return tri (x); });
        add ("Basic Shapes", 32, [] (float t, float x)
        {
            const float p = t * 3.0f;
            if (p < 1.0f) return lerp (sine (x), tri (x), p);
            if (p < 2.0f) return lerp (tri (x), saw (x), p - 1.0f);
            return lerp (saw (x), square (x), p - 2.0f);
        });
        add ("Pulse Sweep", 32, [] (float t, float x) { return square (x, lerp (0.5f, 0.04f, t)); });
        add ("Sine to Saw", 32, [] (float t, float x) { return additive (x, 64, [t] (int h) { return h == 1 ? 1.0f : t / (float) h; }); });
        add ("Rounded Square", 16, [] (float t, float x) { return std::tanh ((1.5f + t * 8.0f) * sine (x)); });

        // ---- Reese stacks -----------------------------------------------------------
        add ("Reese Stack 1", 32, [] (float t, float x) { return 0.5f * (saw (x) + saw (x + t)); });
        add ("Reese Stack 2", 32, [] (float t, float x) { return 0.5f * (saw (x) + square (x + t * 0.5f)); });
        add ("Reese Stack 3", 32, [] (float t, float x) { return 0.45f * (saw (x) + 0.7f * saw (2.0f * x + t)); });
        add ("Reese Stack 4", 32, [] (float t, float x) { return (saw (x) + saw (x + t * 0.33f) + saw (x - t * 0.5f)) / 3.0f; });
        add ("Reese Stack 5", 32, [] (float t, float x) { return 0.5f * saw (x + t) + 0.5f * sine (x) + 0.3f * saw (x); });
        add ("Reese Stack 6", 32, [] (float t, float x) { return std::tanh (2.5f * (saw (x) + saw (x + t * 0.75f))); });

        // ---- Growl --------------------------------------------------------------------
        add ("Growl 1", 32, [] (float t, float x) { return std::sin (kTwoPi * x + t * 6.0f * sine (2.0f * x)); });
        add ("Growl 2", 32, [] (float t, float x) { return vowelWave (x, t * 3.0f, 150.0f) + 0.2f * saw (x); });
        add ("Growl 3", 32, [] (float t, float x) { return std::tanh (4.0f * std::sin (kTwoPi * x + t * 4.0f * sine (3.0f * x))); });
        add ("Growl 4", 32, [] (float t, float x) { return czReso (x, t, 0) + 0.3f * sine (x); });
        add ("Growl 5", 32, [] (float t, float x) { return square (x + 0.15f * t * sine (3.0f * x)); });
        add ("Growl 6", 32, [] (float t, float x) { return saw (std::fmod (x * (1.0f + t * 4.0f), 1.0f)) * (1.0f - x) + 0.3f * vowelWave (x, 3.0f - t * 3.0f); });
        add ("Growl 7", 32, [] (float t, float x) { return fold ((1.0f + t * 5.0f) * std::sin (kTwoPi * x + 2.0f * t * sine (2.0f * x))); });
        add ("Growl 8", 32, [] (float t, float x) { return vowelWave (x, 2.0f + 2.0f * std::sin (t * 6.2831f), 110.0f) + 0.25f * std::tanh (3.0f * saw (x)); });

        // ---- Formant / vowel ----------------------------------------------------------
        add ("Vowel AEIOU", 64, [] (float t, float x) { return vowelWave (x, t * 4.0f); });
        add ("Vowel Talk", 32, [] (float t, float x) { const float p = t < 0.5f ? lerp (3.0f, 0.0f, t * 2.0f) : lerp (0.0f, 1.0f, t * 2.0f - 1.0f); return vowelWave (x, p, 140.0f); });
        add ("Formant Sweep", 48, [] (float t, float x) { const float f[] = { 150.0f + t * 2800.0f }; return formantSeries (x, f, 1, 90.0f, 0.5f) + 0.15f * sine (x); });
        add ("Yoi", 32, [] (float t, float x) { return vowelWave (x, lerp (4.0f, 2.0f, t), 100.0f); });
        add ("Wah Formant", 32, [] (float t, float x) { const float f[] = { 300.0f + t * 1500.0f, 1100.0f + t * 1200.0f }; return formantSeries (x, f, 2, 160.0f); });
        add ("Choir Formant", 32, [] (float t, float x) { return vowelWave (x, 0.5f + t * 3.0f, 220.0f); });

        // ---- Organ (UKG) -------------------------------------------------------------
        auto drawbar = [] (float x, const std::array<float, 9>& bars)
        {
            static const float ratios[] = { 0.5f, 1.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
            float s = 0.0f;
            for (int i = 0; i < 9; ++i)
                s += bars[(size_t) i] * std::sin (kTwoPi * ratios[i] * 2.0f * x);   // x2: 16' footage is the fundamental
            return s;
        };
        add ("Garage Organ 1", 16, [drawbar] (float t, float x) { return drawbar (x, { 0.8f, 0.0f, 1.0f, 0.6f, 0.4f * t, 0.3f, 0.0f, 0.0f, 0.2f * t }); });
        add ("Garage Organ 2", 16, [drawbar] (float t, float x) { return std::tanh (1.6f * drawbar (x, { 1.0f, 0.5f, 0.8f, 0.0f, 0.6f, 0.0f, 0.3f * t, 0.0f, 0.0f })); });
        add ("Garage Organ 3", 16, [drawbar] (float t, float x) { return drawbar (x, { 0.6f, 0.6f, 0.6f, 0.6f * t, 0.6f, 0.2f, 0.0f, 0.3f, 0.3f * t }); });
        add ("Garage Organ 4", 16, [drawbar] (float t, float x) { return drawbar (x, { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f + 0.5f * t, 0.0f, 0.0f, 0.4f }) + 0.2f * square (2.0f * x); });
        add ("Organ Drawbars", 9, [drawbar] (float t, float x)
        {
            std::array<float, 9> bars {};
            const int n = 1 + (int) std::round (t * 8.0f);
            for (int i = 0; i < n; ++i) bars[(size_t) i] = 1.0f / (1.0f + (float) i * 0.4f);
            return drawbar (x, bars);
        });

        // ---- Acid ---------------------------------------------------------------------
        add ("Acid Saw", 32, [] (float t, float x)
        {
            const float peak = 2.0f + t * 30.0f;
            return additive (x, 64, [peak] (int h) { const float d = ((float) h - peak) / 2.0f; return (1.0f / (float) h) * (1.0f + 5.0f * std::exp (-d * d)) * ((float) h < peak + 3 ? 1.0f : 0.15f); });
        });
        add ("Acid Square", 32, [] (float t, float x)
        {
            const float peak = 3.0f + t * 28.0f;
            return additive (x, 63, [peak] (int h) { if (h % 2 == 0) return 0.0f; const float d = ((float) h - peak) / 2.5f; return (1.0f / (float) h) * (1.0f + 5.0f * std::exp (-d * d)) * ((float) h < peak + 3 ? 1.0f : 0.15f); });
        });
        add ("Acid Squelch", 32, [] (float t, float x) { return std::tanh (2.0f * czReso (x, 0.2f + t * 0.8f, 0)) + 0.4f * saw (x); });

        // ---- FM metal ------------------------------------------------------------------
        const int fmRatios[] = { 2, 3, 5, 7, 9, 13 };
        for (int i = 0; i < 6; ++i)
        {
            const int r = fmRatios[i];
            add ("FM Metal " + juce::String (i + 1), 32, [r, i] (float t, float x)
            {
                const float mod2 = i >= 3 ? std::sin (kTwoPi * 2.0f * x) * t * 2.0f : 0.0f;
                return std::sin (kTwoPi * x + t * (2.0f + (float) i) * std::sin (kTwoPi * (float) r * x + mod2));
            });
        }

        // ---- Pluck ------------------------------------------------------------------------
        add ("Pluck Bright", 32, [] (float t, float x) { return additive (x, 64, [t] (int h) { return 1.0f / std::pow ((float) h, 0.6f + t * 2.5f); }); });
        add ("Pluck Soft", 32, [] (float t, float x) { return additive (x, 32, [t] (int h) { return std::exp (-(float) (h - 1) * (0.05f + t * 0.8f)) / (float) h; }); });
        add ("Pluck Nasal", 32, [] (float t, float x) { return additive (x, 48, [t] (int h) { return (1.0f / (float) h) * (h % 3 == 0 ? 0.2f : 1.0f) * std::exp (-(float) h * t * 0.3f); }); });

        // ---- Rubber -----------------------------------------------------------------------
        add ("Rubber 1", 32, [] (float t, float x) { return std::sin (kTwoPi * std::pow (x, 1.0f + t * 3.0f)); });
        add ("Rubber 2", 32, [] (float t, float x) { return std::sin (kTwoPi * x + (0.5f + t * 2.5f) * sine (x)); });
        add ("Rubber 3", 32, [] (float t, float x) { return std::tanh ((1.0f + t * 6.0f) * std::sin (kTwoPi * x + 0.8f * sine (x))); });
        add ("Rubber Bounce", 32, [] (float t, float x) { return sine (x) * (1.0f - t) + std::sin (kTwoPi * x * 2.0f + 1.5f * sine (x)) * t * 0.7f; });

        // ---- Hoover ------------------------------------------------------------------------
        add ("Hoover 1", 32, [] (float t, float x) { return 0.4f * (saw (x) + saw (2.0f * x + t * 0.5f) + square (x, 0.5f - t * 0.4f)); });
        add ("Hoover 2", 32, [] (float t, float x) { return 0.35f * (saw (x) + saw (x + 0.33f * t) + saw (2.0f * x) + 0.5f * saw (0.5f + 3.0f * x)); });
        add ("Hoover 3", 32, [] (float t, float x) { return std::tanh (1.5f * (square (x, 0.15f + t * 0.35f) + saw (2.0f * x))); });

        // ---- Sync sweeps ---------------------------------------------------------------------
        add ("Sync Sweep Saw", 48, [] (float t, float x) { return saw (x * (1.0f + t * 7.0f)); });
        add ("Sync Sweep Square", 48, [] (float t, float x) { return square (x * (1.0f + t * 7.0f)); });
        add ("Sync Sweep Sine", 48, [] (float t, float x) { return sine (x * (1.0f + t * 7.0f)) * (1.0f - x * 0.5f); });

        // ---- Wavefolds ----------------------------------------------------------------------
        add ("Wavefold Sine", 32, [] (float t, float x) { return fold ((1.0f + t * 7.0f) * sine (x)); });
        add ("Wavefold Tri", 32, [] (float t, float x) { return fold ((1.0f + t * 6.0f) * tri (x)); });
        add ("Wavefold Saw", 32, [] (float t, float x) { return fold ((1.0f + t * 5.0f) * saw (x)); });
        add ("Wavefold Deep", 32, [] (float t, float x) { return fold ((1.0f + t * 12.0f) * (sine (x) + 0.4f * sine (2.0f * x))); });

        // ---- Misc character ----------------------------------------------------------------------
        add ("Tube Saw", 16, [] (float t, float x) { const float v = saw (x) + 0.3f * t; return std::tanh ((1.0f + 5.0f * t) * v) - std::tanh (1.5f * t); });
        add ("Supersaw Stack", 16, [] (float t, float x) { return (saw (x) + 0.6f * saw (2.0f * x + t) + 0.4f * saw (3.0f * x + 2.0f * t)) / 2.0f; });
        add ("Harmonic Climb", 64, [] (float t, float x) { const float h = 1.0f + t * 63.0f; const int a = (int) h; const float f = h - (float) a; return (1.0f - f) * std::sin (kTwoPi * (float) a * x) + f * std::sin (kTwoPi * (float) (a + 1) * x) + 0.5f * sine (x); });
        add ("Odd Harmonics", 32, [] (float t, float x) { const int n = 1 + (int) (t * 40.0f); return additive (x, 2 * n, [n] (int h) { return (h % 2 == 1 && h <= 2 * n - 1) ? 1.0f / (float) h : 0.0f; }); });
        add ("Even Harmonics", 32, [] (float t, float x) { const int n = 1 + (int) (t * 30.0f); return sine (x) + additive (x, 2 * n, [n] (int h) { return (h % 2 == 0 && h <= 2 * n) ? 1.0f / (float) h : 0.0f; }); });
        add ("Comb Sweep", 48, [] (float t, float x) { return saw (x) - saw (x + 0.02f + t * 0.48f); });
        add ("Phase Dist Reso 1", 32, [] (float t, float x) { return czReso (x, t, 0); });
        add ("Phase Dist Reso 2", 32, [] (float t, float x) { return czReso (x, t, 1); });
        add ("Phase Dist Reso 3", 32, [] (float t, float x) { return czReso (x, t, 2); });
        add ("Random Harmonics", 32, [] (float t, float x)
        {
            const int frame = (int) (t * 31.0f);
            return additive (x, 48, [frame] (int h) { return std::abs (hashNoise (h, frame + 7)) / std::sqrt ((float) h); });
        });

        // ---- Neuro / riddim / wobble ------------------------------------------------------------
        add ("Neuro Mid 1", 32, [] (float t, float x) { return std::tanh (3.0f * std::sin (kTwoPi * x + (1.0f + 5.0f * t) * saw (2.0f * x))); });
        add ("Neuro Mid 2", 32, [] (float t, float x) { return fold ((1.0f + t * 3.0f) * (saw (x) + square (2.0f * x, 0.3f) * 0.5f)); });
        add ("Neuro Mid 3", 32, [] (float t, float x) { return czReso (x, t, 1) * 0.7f + 0.5f * std::tanh (4.0f * sine (3.0f * x + t)); });
        add ("Riddim Square", 32, [] (float t, float x) { return additive (x, 63, [t] (int h) { if (h % 2 == 0) return 0.0f; const float notch = 3.0f + t * 20.0f; const float d = ((float) h - notch) / 3.0f; return (1.0f / (float) h) * (1.0f - 0.95f * std::exp (-d * d)); }); });
        add ("Wobble Classic", 64, [] (float t, float x) { const float cut = 1.0f + t * t * 60.0f; return additive (x, 64, [cut] (int h) { return (1.0f / (float) h) / (1.0f + std::pow ((float) h / cut, 4.0f)) * (std::abs ((float) h - cut) < 2.0f ? 2.5f : 1.0f); }); });
        add ("Talkbox", 48, [] (float t, float x) { return vowelWave (x, 1.5f + 1.5f * std::sin (t * kTwoPi), 90.0f) * 0.8f + 0.2f * square (x); });
        add ("Grit", 32, [] (float t, float x) { const float steps = 64.0f - t * 60.0f; return std::round (sine (x) * steps) / steps + 0.1f * t * saw (5.0f * x); });

        // ---- Instruments & colour ------------------------------------------------------------------
        add ("Brass Bass", 32, [] (float t, float x) { return additive (x, 40, [t] (int h) { return (1.0f / (float) h) * std::min (1.0f, (2.0f + t * 20.0f) / (float) h); }); });
        add ("Clav", 16, [] (float t, float x) { return square (x, 0.2f + 0.1f * t) * 0.6f + saw (2.0f * x) * 0.3f; });
        add ("Sub Harmonic", 16, [] (float t, float x) { return sine (x) + t * 0.3f * sine (2.0f * x) + t * 0.15f * sine (3.0f * x); });
        add ("Tape Saw", 16, [] (float t, float x) { return additive (x, 48, [t] (int h) { return (1.0f / (float) h) * std::exp (-(float) h * (0.08f - t * 0.06f)) * (h == 2 ? 1.3f : 1.0f); }); });
        add ("Fizz", 32, [] (float t, float x) { return saw (x) * 0.5f + additive (x, 96, [t] (int h) { return h > 24 ? t * 0.6f * std::abs (hashNoise (h, 3)) / std::sqrt ((float) h) : 0.0f; }); });
        add ("Glass", 16, [] (float t, float x) { return sine (x) + 0.5f * sine (4.0f * x) * t + 0.3f * sine (11.0f * x) * t; });
        add ("Bell", 32, [] (float t, float x) { return std::sin (kTwoPi * x + (1.0f + 3.0f * t) * std::sin (kTwoPi * 7.0f * x)); });
        add ("Chord Stack Major", 16, [] (float t, float x) { return (saw (4.0f * x) + saw (5.0f * x) + saw (6.0f * x)) / 3.0f * (0.5f + 0.5f * t) + sine (x) * (1.0f - t) * 0.5f; });
        add ("Chord Stack Minor", 16, [] (float t, float x) { return (saw (10.0f * x) + saw (12.0f * x) + saw (15.0f * x)) / 3.0f * (0.5f + 0.5f * t) + sine (x) * (1.0f - t) * 0.5f; });
        add ("Power Fifth", 16, [] (float t, float x) { return (saw (2.0f * x) + saw (3.0f * x)) * 0.5f * (0.4f + 0.6f * t) + sine (x) * 0.6f; });
        add ("Laser Zap", 48, [] (float t, float x) { const float h = 40.0f * (1.0f - t) + 1.0f; return std::sin (kTwoPi * std::floor (h) * x) * 0.7f + 0.3f * sine (x); });
        add ("Digital Steps", 32, [] (float t, float x) { const float steps = 2.0f + std::floor ((1.0f - t) * 30.0f); return std::floor (x * steps) / (steps - 1.0f) * 2.0f - 1.0f; });
        add ("Dirty Sub", 16, [] (float t, float x) { return std::tanh ((1.0f + t * 9.0f) * sine (x)); });
        add ("Hollow Pipe", 16, [] (float t, float x) { return additive (x, 31, [t] (int h) { return h % 2 == 1 ? 1.0f / ((float) h * (float) h) * (1.0f + t * (float) h * 0.3f) : 0.0f; }); });
        add ("Warehouse Buzz", 32, [] (float t, float x) { return std::tanh (3.0f * (saw (x) + 0.5f * square (2.0f * x, 0.25f + 0.2f * t))); });
        add ("Elastic", 32, [] (float t, float x) { const float b = 0.1f + t * 0.8f; const float p = x < b ? x / b * 0.5f : 0.5f + (x - b) / (1.0f - b) * 0.5f; return sine (p); });
        add ("Mirror Saw", 32, [] (float t, float x) { const float m = x < 0.5f ? saw (2.0f * x) : -saw (2.0f - 2.0f * x); return lerp (saw (x), m, t); });
        add ("Bit Rubber", 32, [] (float t, float x) { const float q = 4.0f + (1.0f - t) * 28.0f; return std::round (std::sin (kTwoPi * x + 0.8f * sine (x)) * q) / q; });
        add ("Nasal Reed", 32, [] (float t, float x) { const float f[] = { 600.0f + t * 900.0f, 1700.0f + t * 600.0f }; return formantSeries (x, f, 2, 80.0f) + 0.3f * square (x, 0.3f); });
        add ("Velvet Saw", 16, [] (float t, float x) { return additive (x, 64, [t] (int h) { return (1.0f / (float) h) / (1.0f + std::pow ((float) h / (3.0f + t * 30.0f), 2.0f)); }); });
        add ("Sweet Spot", 32, [] (float t, float x) { return sine (x) * 0.8f + std::sin (kTwoPi * 2.0f * x + t * 3.0f) * 0.35f + std::sin (kTwoPi * 3.0f * x) * t * 0.2f; });
        return d;
    }

    const std::vector<Def>& defs()
    {
        static const std::vector<Def> d = makeDefs();
        return d;
    }
} // namespace

int numTables() { return (int) defs().size(); }

const juce::StringArray& tableNames()
{
    static const juce::StringArray names = []
    {
        juce::StringArray s;
        for (auto& d : defs())
            s.add (d.name);
        return s;
    }();
    return names;
}

int indexOf (const juce::String& name) { return tableNames().indexOf (name, true); }

std::shared_ptr<Wavetable> build (int index)
{
    const auto& d = defs()[(size_t) juce::jlimit (0, numTables() - 1, index)];
    std::vector<std::vector<float>> frames ((size_t) d.frames, std::vector<float> ((size_t) Wavetable::kSize));
    for (int f = 0; f < d.frames; ++f)
    {
        const float t = d.frames > 1 ? (float) f / (float) (d.frames - 1) : 0.0f;
        for (int i = 0; i < Wavetable::kSize; ++i)
            frames[(size_t) f][(size_t) i] = d.gen (t, (float) i / (float) Wavetable::kSize);
    }
    return Wavetable::fromFrames (d.name, std::move (frames));
}
} // namespace bgdj::factory
