#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Common/Parameters.h"
#include "../../Common/LockFree.h"
#include "../../Engine/Shapes.h"
#include "../Filter.h"

namespace bgdj
{
/** A parameter as seen by an FX module: raw value plus optional global modulation. */
struct FxParam
{
    std::atomic<float>* raw = nullptr;
    juce::RangedAudioParameter* param = nullptr;
    int dest = -1;

    void init (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        raw = s.getRawParameterValue (id);
        param = s.getParameter (id);
        dest = params::modDestIndexForParam (id);
        jassert (raw != nullptr);
    }

    float get (const float* offsets) const noexcept
    {
        const float v = raw->load (std::memory_order_relaxed);
        if (dest < 0 || offsets == nullptr || offsets[dest] == 0.0f)
            return v;
        return param->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, param->convertTo0to1 (v) + offsets[dest]));
    }
    bool on() const noexcept    { return raw->load (std::memory_order_relaxed) >= 0.5f; }
    int choice() const noexcept { return (int) std::lround (raw->load (std::memory_order_relaxed)); }
};

struct FxContext
{
    double sampleRate = 48000.0;
    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    int osFactor = 1;
    const float* modOffsets = nullptr;
    const float* scL = nullptr;
    const float* scR = nullptr;
};

// ---- small DSP helpers --------------------------------------------------------
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] {}, z2[2] {};

    enum class Kind { LowPass, HighPass, BandPass, Peak, Notch, LowShelf, HighShelf };
    void set (Kind k, double fs, float hz, float q, float gainDb) noexcept;
    float process (float x, int ch) noexcept
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
    void reset() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }
};

/** Linkwitz-Riley 4th-order crossover (two cascaded Butterworth biquads per side). */
struct Crossover
{
    Biquad lp1, lp2, hp1, hp2;
    float lastHz = -1.0f;
    void set (double fs, float hz) noexcept
    {
        if (std::abs (hz - lastHz) < 0.01f) return;
        lastHz = hz;
        lp1.set (Biquad::Kind::LowPass, fs, hz, 0.70710678f, 0); lp2 = lp1;
        hp1.set (Biquad::Kind::HighPass, fs, hz, 0.70710678f, 0); hp2 = hp1;
    }
    void split (float x, int ch, float& lo, float& hi) noexcept
    {
        lo = lp2.process (lp1.process (x, ch), ch);
        hi = hp2.process (hp1.process (x, ch), ch);
    }
    void reset() noexcept { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); lastHz = -1.0f; }
};

struct DcBlocker
{
    float x1[2] {}, y1[2] {}, r = 0.9995f;
    void prepare (double fs) noexcept { r = (float) std::exp (-juce::MathConstants<double>::twoPi * 8.0 / fs); }
    float process (float x, int ch) noexcept { const float y = x - x1[ch] + r * y1[ch]; x1[ch] = x; y1[ch] = y; return y; }
    void reset() noexcept { x1[0] = x1[1] = y1[0] = y1[1] = 0.0f; }
};

struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, pos = 0;
    void prepare (int maxSamples) { const int n = juce::nextPowerOfTwo (std::max (16, maxSamples)); buf.assign ((size_t) n, 0.0f); mask = n - 1; pos = 0; }
    void push (float x) noexcept { buf[(size_t) pos] = x; pos = (pos + 1) & mask; }
    float read (float delay) const noexcept   // delay in samples (>= 1)
    {
        const float rp = (float) pos - delay;
        const int i0 = (int) std::floor (rp);
        const float f = rp - (float) i0;
        const float a = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * f;
    }
    int capacity() const noexcept { return mask + 1; }
    void reset() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }
};

// ============================================================================
class FxModule
{
public:
    virtual ~FxModule() = default;
    virtual void init (juce::AudioProcessorValueTreeState& s, const juce::String& prefix)
    {
        on.init (s, prefix + "on");
        mix.init (s, prefix + "mix");
    }
    virtual void prepare (double sampleRate, int maxBlock) = 0;
    virtual void reset() = 0;
    /** Processes the wet signal in place. */
    virtual void process (float* L, float* R, int n, const FxContext& ctx) noexcept = 0;
    /** Whether the module wants FX-stage oversampling. */
    virtual bool isNonLinear() const noexcept { return false; }

    FxParam on, mix;
    std::atomic<float> meterGainReduction { 0.0f };   // dB, for UI
};

class DistortionFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
    bool isNonLinear() const noexcept override { return true; }
private:
    FxParam mode, drive, bias, tone, out, mbLow, mbMid, mbHigh, xLow, xHigh;
    Crossover xo1, xo2;
    OnePoleTpt toneLp[2];
    DcBlocker dc;
    float held[2] {};
    float holdCount = 0.0f;
    double fs = 48000.0;
};

class OttFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam depth, time, inGain, outGain, up[3], down[3];
    Crossover xo1, xo2;
    float env[3] {};
    double fs = 48000.0;
};

class CompressorFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam type, thresh, ratio, attack, release, knee, makeup, sc, scHpf;
    Biquad scFilter;
    float env = 0.0f, grSmooth = 0.0f, lastHpf = -1.0f;
    double fs = 48000.0;
};

class DuckerFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
    RtSharedObject<ShapeTable> customShape;
    std::atomic<float> uiPhase { 0.0f };
private:
    FxParam rate, depth, shape, smooth, offset;
    ShapeTable presets[5];
    double freePhase = 0.0;
    float gainSmooth = 1.0f;
    double fs = 48000.0;
};

class EqFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
    /** Magnitude response in dB at a frequency (UI thread, computed from current params). */
    float responseDb (float hz) const;
private:
    struct Band { FxParam on, type, freq, gain, q; Biquad f, f2; float last[4] { -1, -1, -1, -1 }; };
    std::array<Band, 6> bands;
    FxParam out;
    double fs = 48000.0;
    void updateBand (Band& b) noexcept;
};

class FilterFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam type, cutoff, reso, drive, lfoRate, lfoSync, lfoSyncRate, lfoDepth;
    Svf svf[2];
    double phase = 0.0;
    double fs = 48000.0;
};

class ModulationFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam mode, rate, depth, feedback, delay, stages;
    DelayLine dl[2];
    OnePoleTpt ap[2][12];
    float fbState[2] {};
    double phase = 0.0;
    double fs = 48000.0;
};

class DelayFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam mode, sync, syncL, syncR, timeL, timeR, feedback, hp, lp, wow;
    DelayLine dl[2];
    OnePoleTpt fbLp[2], fbHp[2];
    float smoothTime[2] { 1000, 1000 };
    double wowPhase = 0.0;
    double fs = 48000.0;
};

class ReverbFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    static constexpr int kLines = 8;
    FxParam type, size, decay, damp, predelay, width, lowcut;
    DelayLine lines[kLines], pre[2], early;
    OnePoleTpt dampLp[kLines], lowCut[2];
    DelayLine diffusers[4];
    double fs = 48000.0;
    double modPhase = 0.0;
};

class HyperFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam rate, detune, voices, dim, monoBelow;
    DelayLine dl[2];
    Crossover xo;
    double phase[7] {};
    double fs = 48000.0;
};

class SaturatorFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
    bool isNonLinear() const noexcept override { return true; }
private:
    FxParam drive, warmth, wow, flutter, tone, out;
    DelayLine dl[2];
    Biquad lowShelf, highShelf, tiltLow, tiltHigh;
    float last[4] { -99, -99, -99, -99 };
    double wowPhase = 0.0, flutPhase = 0.0;
    DcBlocker dc;
    double fs = 48000.0;
};

class UtilityFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam width, monoOn, monoBelow, gain, pan, dcOn, invL, invR;
    Crossover xo;
    DcBlocker dc;
    double fs = 48000.0;
};

class LimiterFx : public FxModule
{
public:
    void init (juce::AudioProcessorValueTreeState& s, const juce::String& p) override;
    void prepare (double sr, int) override;
    void reset() override;
    void process (float* L, float* R, int n, const FxContext& ctx) noexcept override;
private:
    FxParam mode, input, ceiling, release, truePeak;
    float gain = 1.0f;
    float hist[2][3] {};
    double fs = 48000.0;
};
} // namespace bgdj
