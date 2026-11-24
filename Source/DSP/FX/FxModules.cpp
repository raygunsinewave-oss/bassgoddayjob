#include "FxModules.h"

namespace bgdj
{
namespace
{
    inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
    inline float gainToDb (float g) noexcept  { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
    inline float fold (float v) noexcept
    {
        v = juce::jlimit (-64.0f, 64.0f, v);
        while (std::abs (v) > 1.0f) v = v > 0.0f ? 2.0f - v : -2.0f - v;
        return v;
    }
    inline float onePoleCoef (double fs, float ms) noexcept
    {
        return ms <= 0.0f ? 0.0f : (float) std::exp (-1.0 / (fs * ms * 0.001));
    }
    inline float tptG (double fs, float hz) noexcept
    {
        return std::tan (juce::MathConstants<float>::pi * juce::jlimit (10.0f, (float) fs * 0.45f, hz) / (float) fs);
    }
    double beatPhase (const FxContext& c, int sampleIndex, double beats)
    {
        const double ppq = c.ppq + (double) sampleIndex / c.sampleRate * c.bpm / 60.0;
        const double p = ppq / beats;
        return p - std::floor (p);
    }
}

// ============================================================================
void Biquad::set (Kind k, double fs, float hz, float q, float gainDb) noexcept
{
    const double w0 = juce::MathConstants<double>::twoPi * juce::jlimit (10.0, fs * 0.49, (double) hz) / fs;
    const double cw = std::cos (w0), sw = std::sin (w0);
    const double alpha = sw / (2.0 * std::max (0.025, (double) q));
    const double A = std::pow (10.0, gainDb / 40.0);
    double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;
    switch (k)
    {
        case Kind::LowPass:  B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
        case Kind::HighPass: B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = B0; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
        case Kind::BandPass: B0 = alpha; B1 = 0; B2 = -alpha; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
        case Kind::Notch:    B0 = 1; B1 = -2 * cw; B2 = 1; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
        case Kind::Peak:     B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A; A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A; break;
        case Kind::LowShelf:
        {
            const double s = 2 * std::sqrt (A) * alpha;
            B0 = A * ((A + 1) - (A - 1) * cw + s); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - s);
            A0 = (A + 1) + (A - 1) * cw + s;       A1 = -2 * ((A - 1) + (A + 1) * cw);    A2 = (A + 1) + (A - 1) * cw - s;
            break;
        }
        case Kind::HighShelf:
        {
            const double s = 2 * std::sqrt (A) * alpha;
            B0 = A * ((A + 1) + (A - 1) * cw + s); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - s);
            A0 = (A + 1) - (A - 1) * cw + s;       A1 = 2 * ((A - 1) - (A + 1) * cw);      A2 = (A + 1) - (A - 1) * cw - s;
            break;
        }
    }
    b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
    a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
}

// ============================================================================
// Distortion
void DistortionFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    mode.init (s, p + "mode"); drive.init (s, p + "drive"); bias.init (s, p + "bias"); tone.init (s, p + "tone");
    out.init (s, p + "out"); mbLow.init (s, p + "mb_low"); mbMid.init (s, p + "mb_mid"); mbHigh.init (s, p + "mb_high");
    xLow.init (s, p + "mb_xlow"); xHigh.init (s, p + "mb_xhigh");
}
void DistortionFx::prepare (double sr, int) { fs = sr; dc.prepare (sr); reset(); }
void DistortionFx::reset() { xo1.reset(); xo2.reset(); toneLp[0].reset(); toneLp[1].reset(); dc.reset(); held[0] = held[1] = 0; holdCount = 0; }

void DistortionFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    if (ctx.sampleRate != fs) { fs = ctx.sampleRate; dc.prepare (fs); xo1.reset(); xo2.reset(); }
    const int m = mode.choice();
    const float dr = drive.get (ctx.modOffsets);
    const float b = bias.get (ctx.modOffsets);
    const float pre = dbToGain (dr * 36.0f);
    const float outG = dbToGain (out.get (ctx.modOffsets));
    const float g = tptG (fs, tone.get (ctx.modOffsets));
    toneLp[0].set (g); toneLp[1].set (g);
    if (m == 9) { xo1.set (fs, xLow.get (ctx.modOffsets)); xo2.set (fs, xHigh.get (ctx.modOffsets)); }
    const float gl = dbToGain (mbLow.get (nullptr) * 30.0f), gm = dbToGain (mbMid.get (nullptr) * 30.0f), gh = dbToGain (mbHigh.get (nullptr) * 30.0f);
    const float bits = 16.0f - dr * 14.5f;
    const float q = std::pow (2.0f, bits) * 0.5f;
    const float holdFactor = 1.0f + dr * dr * 60.0f;

    for (int i = 0; i < n; ++i)
    {
        bool newHold = false;
        if (m == 6) { holdCount += 1.0f; if (holdCount >= holdFactor) { holdCount -= holdFactor; newHold = true; } }
        float* chans[2] = { L, R };
        for (int ch = 0; ch < 2; ++ch)
        {
            const float in = chans[ch][i];
            const float x = in * pre + b * 0.5f;
            float y;
            switch (m)
            {
                case 0:  y = x >= 0.0f ? std::tanh (x * 1.2f) : 0.85f * std::tanh (x * 0.9f); break;
                case 1:  y = x > 0.0f ? 1.0f - std::exp (-x * 1.5f) : -0.45f * (1.0f - std::exp (x * 0.6f)); break;
                case 2:  { const float c = juce::jlimit (-1.5f, 1.5f, x); y = c - c * c * c * (4.0f / 27.0f); break; }
                case 3:  y = juce::jlimit (-1.0f, 1.0f, x); break;
                case 4:  y = fold (x); break;
                case 5:  y = std::round ((in + b * 0.1f) * q) / q; break;
                case 6:  if (newHold) held[ch] = in; y = held[ch]; break;
                case 7:  { const float t = std::tanh (x); y = t * (1.0f - dr) + (std::abs (t) * 2.0f - 0.64f) * dr; break; }
                case 8:  y = x >= 0.0f ? std::tanh (x * (1.0f + b)) : std::tanh (x * (1.0f - 0.8f * b)) * (1.0f - 0.3f * b); break;
                case 9:
                {
                    float lo, rest, mid, hi;
                    xo1.split (in, ch, lo, rest);
                    xo2.split (rest, ch, mid, hi);
                    y = std::tanh (lo * gl) / std::sqrt (gl) * 1.2f + std::tanh (mid * gm) / std::sqrt (gm) * 1.2f + std::tanh (hi * gh) / std::sqrt (gh) * 1.2f;
                    break;
                }
                default: y = x; break;
            }
            y = toneLp[ch].lp (y);
            chans[ch][i] = dc.process (y, ch) * outG;
        }
    }
}

// ============================================================================
// OTT
void OttFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    depth.init (s, p + "depth"); time.init (s, p + "time"); inGain.init (s, p + "in"); outGain.init (s, p + "out");
    const char* names[] = { "low", "mid", "high" };
    for (int b = 0; b < 3; ++b) { up[b].init (s, p + names[b] + "_up"); down[b].init (s, p + names[b] + "_down"); }
}
void OttFx::prepare (double sr, int) { fs = sr; reset(); }
void OttFx::reset() { xo1.reset(); xo2.reset(); env[0] = env[1] = env[2] = 0.0f; }

void OttFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    xo1.set (fs, 120.0f);
    xo2.set (fs, 2500.0f);
    const float d = depth.get (ctx.modOffsets);
    const float t = time.get (nullptr);
    const float att = onePoleCoef (fs, 2.0f * (0.25f + t * 2.0f));
    const float rel = onePoleCoef (fs, 60.0f * (0.25f + t * 2.0f));
    const float gi = dbToGain (inGain.get (nullptr)), go = dbToGain (outGain.get (nullptr));
    float upA[3], downA[3];
    for (int b = 0; b < 3; ++b) { upA[b] = up[b].get (nullptr); downA[b] = down[b].get (nullptr); }
    float maxGr = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float bandL[3], bandR[3], rest;
        xo1.split (L[i] * gi, 0, bandL[0], rest); xo2.split (rest, 0, bandL[1], bandL[2]);
        xo1.split (R[i] * gi, 1, bandR[0], rest); xo2.split (rest, 1, bandR[1], bandR[2]);
        float outL = 0.0f, outR = 0.0f;
        for (int b = 0; b < 3; ++b)
        {
            const float lvl = std::max (std::abs (bandL[b]), std::abs (bandR[b]));
            env[b] = lvl > env[b] ? lvl + att * (env[b] - lvl) : lvl + rel * (env[b] - lvl);
            const float db = gainToDb (env[b]);
            float gDb = 0.0f;
            if (db > -24.0f) gDb -= (db + 24.0f) * 0.75f * downA[b];
            if (db < -42.0f && db > -80.0f) gDb += std::min (30.0f, (-42.0f - db) * 0.6667f) * upA[b];
            gDb *= d;
            maxGr = std::min (maxGr, gDb);
            const float g = dbToGain (gDb);
            outL += bandL[b] * g; outR += bandR[b] * g;
        }
        L[i] = outL * go; R[i] = outR * go;
    }
    meterGainReduction.store (maxGr);
}

// ============================================================================
// Compressor
void CompressorFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    type.init (s, p + "type"); thresh.init (s, p + "thresh"); ratio.init (s, p + "ratio"); attack.init (s, p + "attack");
    release.init (s, p + "release"); knee.init (s, p + "knee"); makeup.init (s, p + "makeup"); sc.init (s, p + "sc"); scHpf.init (s, p + "sc_hpf");
}
void CompressorFx::prepare (double sr, int) { fs = sr; reset(); }
void CompressorFx::reset() { env = 0.0f; grSmooth = 0.0f; scFilter.reset(); lastHpf = -1.0f; }

void CompressorFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    const int t = type.choice();
    const float T = thresh.get (nullptr), Rt = ratio.get (nullptr), W = std::max (0.01f, knee.get (nullptr) + (t == 2 ? 4.0f : 0.0f));
    const float attMs = attack.get (nullptr) * (t == 1 ? 3.0f : 1.0f);
    float relMs = release.get (nullptr);
    const float mk = dbToGain (makeup.get (nullptr));
    const bool useSc = sc.on() && ctx.scL != nullptr;
    const float hpf = scHpf.get (nullptr);
    if (std::abs (hpf - lastHpf) > 0.01f) { scFilter.set (Biquad::Kind::HighPass, fs, hpf, 0.707f, 0); lastHpf = hpf; }
    const float att = onePoleCoef (fs, attMs);
    float maxGr = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float dl = useSc ? ctx.scL[i] : L[i];
        const float drr = useSc ? (ctx.scR ? ctx.scR[i] : ctx.scL[i]) : R[i];
        const float det = hpf > 21.0f ? std::max (std::abs (scFilter.process (dl, 0)), std::abs (scFilter.process (drr, 1)))
                                      : std::max (std::abs (dl), std::abs (drr));
        float lvl;
        if (t == 0) lvl = det;
        else lvl = det * det;   // RMS-style detectors
        float relNow = relMs;
        if (t == 1) relNow = relMs * (1.0f + std::abs (grSmooth) / 6.0f);           // opto: release slows with GR
        if (t == 2) relNow = relMs * (std::abs (grSmooth) > 6.0f ? 2.0f : 0.6f);    // glue: auto-release
        const float rel = onePoleCoef (fs, relNow);
        env = lvl > env ? lvl + att * (env - lvl) : lvl + rel * (env - lvl);
        const float db = t == 0 ? gainToDb (env) : 10.0f * std::log10 (std::max (env, 1.0e-12f));
        const float over = db - T;
        float gr = 0.0f;
        const float slope = 1.0f / Rt - 1.0f;
        if (2.0f * over < -W) gr = 0.0f;
        else if (2.0f * std::abs (over) <= W) gr = slope * (over + W * 0.5f) * (over + W * 0.5f) / (2.0f * W);
        else gr = slope * over;
        grSmooth = gr;
        maxGr = std::min (maxGr, gr);
        const float g = dbToGain (gr) * mk;
        L[i] *= g; R[i] *= g;
    }
    meterGainReduction.store (maxGr);
}

// ============================================================================
// Ducker
void DuckerFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    rate.init (s, p + "rate"); depth.init (s, p + "depth"); shape.init (s, p + "shape"); smooth.init (s, p + "smooth"); offset.init (s, p + "offset");
    for (int i = 0; i < 5; ++i)
        presets[i] = renderCurve (shapes::duckPreset (i));
    customShape.publish (std::make_shared<ShapeTable> (presets[0]));
}
void DuckerFx::prepare (double sr, int) { fs = sr; reset(); }
void DuckerFx::reset() { freePhase = 0.0; gainSmooth = 1.0f; }

void DuckerFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    const double beats = params::duckRateBeats (rate.choice());
    const float d = depth.get (ctx.modOffsets);
    const int sh = shape.choice();
    const ShapeTable* table = sh == 4 ? customShape.get() : &presets[juce::jlimit (0, 4, sh)];
    if (table == nullptr) table = &presets[0];
    customShape.audioThreadAcknowledge();
    const float off = offset.get (nullptr);
    const float coef = onePoleCoef (fs, smooth.get (nullptr) * 30.0f);
    const double freeInc = ctx.bpm / 60.0 / beats / fs;
    double ph = 0.0;

    for (int i = 0; i < n; ++i)
    {
        if (ctx.playing) ph = beatPhase (ctx, i, beats);
        else { freePhase += freeInc; freePhase -= std::floor (freePhase); ph = freePhase; }
        double p = ph + off;
        p -= std::floor (p);
        const float target = 1.0f - d * (1.0f - table->lookup ((float) p));
        gainSmooth = target + coef * (gainSmooth - target);
        L[i] *= gainSmooth; R[i] *= gainSmooth;
    }
    uiPhase.store ((float) ph);
    meterGainReduction.store (gainToDb (gainSmooth));
}

// ============================================================================
// EQ
void EqFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    for (int b = 0; b < 6; ++b)
    {
        const auto bp = p + "b" + juce::String (b + 1) + "_";
        auto& band = bands[(size_t) b];
        band.on.init (s, bp + "on"); band.type.init (s, bp + "type"); band.freq.init (s, bp + "freq");
        band.gain.init (s, bp + "gain"); band.q.init (s, bp + "q");
    }
    out.init (s, p + "out");
}
void EqFx::prepare (double sr, int) { fs = sr; reset(); }
void EqFx::reset()
{
    for (auto& b : bands) { b.f.reset(); b.f2.reset(); for (auto& l : b.last) l = -1.0f; }
}

static void configureEqBand (Biquad& f, Biquad& f2, int type, double fs, float hz, float g, float q)
{
    switch (type)
    {
        case 0:  f.set (Biquad::Kind::HighPass, fs, hz, q, 0); f2.set (Biquad::Kind::HighPass, fs, hz, 0.707f, 0); break;
        case 1:  f.set (Biquad::Kind::LowShelf, fs, hz, q, g); break;
        case 2:  f.set (Biquad::Kind::Peak, fs, hz, q, g); break;
        case 3:  f.set (Biquad::Kind::Notch, fs, hz, q, 0); break;
        case 4:  f.set (Biquad::Kind::HighShelf, fs, hz, q, g); break;
        default: f.set (Biquad::Kind::LowPass, fs, hz, q, 0); f2.set (Biquad::Kind::LowPass, fs, hz, 0.707f, 0); break;
    }
}

void EqFx::updateBand (Band& b) noexcept
{
    const float t = (float) b.type.choice(), hz = b.freq.get (nullptr), g = b.gain.get (nullptr), q = b.q.get (nullptr);
    if (t == b.last[0] && hz == b.last[1] && g == b.last[2] && q == b.last[3])
        return;
    b.last[0] = t; b.last[1] = hz; b.last[2] = g; b.last[3] = q;
    configureEqBand (b.f, b.f2, (int) t, fs, hz, g, q);
}

void EqFx::process (float* L, float* R, int n, const FxContext&) noexcept
{
    const float go = dbToGain (out.get (nullptr));
    for (auto& b : bands)
    {
        if (! b.on.on())
            continue;
        updateBand (b);
        const int t = b.type.choice();
        const bool twoStage = t == 0 || t == 5;
        for (int i = 0; i < n; ++i)
        {
            L[i] = b.f.process (L[i], 0);
            R[i] = b.f.process (R[i], 1);
            if (twoStage) { L[i] = b.f2.process (L[i], 0); R[i] = b.f2.process (R[i], 1); }
        }
    }
    if (go != 1.0f)
        for (int i = 0; i < n; ++i) { L[i] *= go; R[i] *= go; }
}

float EqFx::responseDb (float hz) const
{
    double total = 0.0;
    for (auto& b : bands)
    {
        if (! b.on.on())
            continue;
        Biquad f, f2;
        const int t = b.type.choice();
        configureEqBand (f, f2, t, fs, b.freq.get (nullptr), b.gain.get (nullptr), b.q.get (nullptr));
        auto mag = [hz, this] (const Biquad& q)
        {
            const std::complex<double> z = std::polar (1.0, -juce::MathConstants<double>::twoPi * hz / fs);
            const auto num = (double) q.b0 + (double) q.b1 * z + (double) q.b2 * z * z;
            const auto den = 1.0 + (double) q.a1 * z + (double) q.a2 * z * z;
            return std::abs (num / den);
        };
        total += 20.0 * std::log10 (std::max (1.0e-9, mag (f)));
        if (t == 0 || t == 5)
            total += 20.0 * std::log10 (std::max (1.0e-9, mag (f2)));
    }
    return (float) total + out.get (nullptr);
}

// ============================================================================
// Filter FX
void FilterFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    type.init (s, p + "type"); cutoff.init (s, p + "cutoff"); reso.init (s, p + "reso"); drive.init (s, p + "drive");
    lfoRate.init (s, p + "lfo_rate"); lfoSync.init (s, p + "lfo_sync"); lfoSyncRate.init (s, p + "lfo_sync_rate"); lfoDepth.init (s, p + "lfo_depth");
}
void FilterFx::prepare (double sr, int) { fs = sr; reset(); }
void FilterFx::reset() { svf[0].reset(); svf[1].reset(); phase = 0.0; }

void FilterFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    const int t = type.choice();
    const float fc = cutoff.get (ctx.modOffsets);
    const float k = 2.0f - 1.95f * reso.get (ctx.modOffsets);
    const float dr = drive.get (nullptr);
    const float depth = lfoDepth.get (nullptr);
    const bool sync = lfoSync.on();
    const double beats = params::syncRateBeats (lfoSyncRate.choice());
    const double inc = sync ? ctx.bpm / 60.0 / beats / fs : lfoRate.get (nullptr) / fs;
    const float pre = 1.0f + dr * 8.0f;

    for (int i0 = 0; i0 < n; i0 += 16)
    {
        const int end = std::min (n, i0 + 16);
        if (sync && ctx.playing) phase = beatPhase (ctx, i0, beats);
        const float lfo = std::sin (juce::MathConstants<float>::twoPi * (float) phase);
        const float g = tptG (fs, fc * std::exp2 (lfo * depth * 3.0f));
        svf[0].set (g, k); svf[1].set (g, k);
        for (int i = i0; i < end; ++i)
        {
            float* ch[2] = { L, R };
            for (int c = 0; c < 2; ++c)
            {
                float x = ch[c][i];
                if (dr > 0.0f) x = std::tanh (x * pre) / std::sqrt (pre);
                const auto o = svf[c].process (x);
                ch[c][i] = t == 0 ? o.lp : (t == 1 ? o.hp : o.bp * k);
            }
        }
        if (! (sync && ctx.playing))
        {
            phase += inc * (end - i0);
            phase -= std::floor (phase);
        }
    }
}

// ============================================================================
// Chorus / Flanger / Phaser
void ModulationFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    mode.init (s, p + "mode"); rate.init (s, p + "rate"); depth.init (s, p + "depth"); feedback.init (s, p + "feedback");
    delay.init (s, p + "delay"); stages.init (s, p + "stages");
}
void ModulationFx::prepare (double sr, int) { fs = sr; dl[0].prepare ((int) (sr * 0.1)); dl[1].prepare ((int) (sr * 0.1)); reset(); }
void ModulationFx::reset()
{
    dl[0].reset(); dl[1].reset(); fbState[0] = fbState[1] = 0.0f; phase = 0.0;
    for (auto& c : ap) for (auto& a : c) a.reset();
}

void ModulationFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    const int m = mode.choice();
    const float r = rate.get (ctx.modOffsets), d = depth.get (nullptr), fb = feedback.get (nullptr), base = delay.get (nullptr);
    const int st = juce::jlimit (2, 12, stages.choice());
    const double inc = r / fs;

    for (int i = 0; i < n; ++i)
    {
        phase += inc;
        phase -= std::floor (phase);
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            const float lfo = std::sin (juce::MathConstants<float>::twoPi * (float) (phase + c * 0.25));
            const float x = ch[c][i];
            if (m == 0)        // chorus
            {
                const float t1 = (base + d * 5.0f * (1.0f + lfo)) * 0.001f * (float) fs;
                const float t2 = (base * 1.37f + d * 4.0f * (1.0f - lfo)) * 0.001f * (float) fs;
                const float y = 0.5f * (dl[c].read (std::max (2.0f, t1)) + dl[c].read (std::max (2.0f, t2)));
                dl[c].push (x + y * fb * 0.5f);
                ch[c][i] = y;
            }
            else if (m == 1)   // flanger
            {
                const float t = (0.3f + base * 0.25f * d * (1.0f + lfo)) * 0.001f * (float) fs;
                const float y = dl[c].read (std::max (1.5f, t));
                dl[c].push (x + y * fb);
                ch[c][i] = 0.5f * (x + y);
            }
            else               // phaser
            {
                const float hz = 200.0f * std::exp2 ((1.0f + lfo) * 2.5f * d);
                const float g = tptG (fs, hz);
                float a = x + fbState[c] * fb;
                for (int s = 0; s < st; ++s) { ap[c][s].set (g); a = ap[c][s].ap (a); }
                fbState[c] = juce::jlimit (-2.0f, 2.0f, a);
                ch[c][i] = 0.5f * (x + a);
            }
        }
    }
}

// ============================================================================
// Delay
void DelayFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    mode.init (s, p + "mode"); sync.init (s, p + "sync"); syncL.init (s, p + "sync_l"); syncR.init (s, p + "sync_r");
    timeL.init (s, p + "time_l"); timeR.init (s, p + "time_r"); feedback.init (s, p + "feedback");
    hp.init (s, p + "hp"); lp.init (s, p + "lp"); wow.init (s, p + "wow");
}
void DelayFx::prepare (double sr, int) { fs = sr; dl[0].prepare ((int) (sr * 4.2)); dl[1].prepare ((int) (sr * 4.2)); reset(); }
void DelayFx::reset() { dl[0].reset(); dl[1].reset(); for (auto& f : fbLp) f.reset(); for (auto& f : fbHp) f.reset(); smoothTime[0] = smoothTime[1] = -1.0f; }

void DelayFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    const int m = mode.choice();
    float target[2];
    if (sync.on())
    {
        target[0] = (float) (params::syncRateBeats (syncL.choice()) * 60.0 / ctx.bpm * fs);
        target[1] = (float) (params::syncRateBeats (syncR.choice()) * 60.0 / ctx.bpm * fs);
    }
    else
    {
        target[0] = timeL.get (nullptr) * 0.001f * (float) fs;
        target[1] = timeR.get (nullptr) * 0.001f * (float) fs;
    }
    const float maxD = (float) dl[0].capacity() - 8.0f;
    for (int c = 0; c < 2; ++c)
    {
        target[c] = juce::jlimit (2.0f, maxD, target[c]);
        if (smoothTime[c] < 0.0f) smoothTime[c] = target[c];
    }
    const float fb = feedback.get (ctx.modOffsets);
    for (int c = 0; c < 2; ++c) { fbLp[c].set (tptG (fs, lp.get (nullptr))); fbHp[c].set (tptG (fs, hp.get (nullptr))); }
    const float w = m == 2 ? wow.get (nullptr) : 0.0f;
    const float tc = onePoleCoef (fs, 80.0f);

    for (int i = 0; i < n; ++i)
    {
        wowPhase += 0.7 / fs;
        wowPhase -= std::floor (wowPhase);
        const float mod = w * 0.004f * (float) fs * std::sin (juce::MathConstants<float>::twoPi * (float) wowPhase);
        for (int c = 0; c < 2; ++c)
            smoothTime[c] = target[c] + tc * (smoothTime[c] - target[c]);
        const float yl = dl[0].read (juce::jlimit (2.0f, maxD, smoothTime[0] + mod));
        const float yr = dl[1].read (juce::jlimit (2.0f, maxD, smoothTime[1] + mod * 0.8f));
        auto shape = [&] (int c, float v)
        {
            v = fbLp[c].lp (fbHp[c].hp (v));
            return m == 2 ? std::tanh (v * 1.3f) / 1.3f : v;
        };
        const float inL = L[i], inR = R[i];
        if (m == 1)  // ping-pong
        {
            dl[0].push ((inL + inR) * 0.5f + shape (0, yr) * fb);
            dl[1].push (shape (1, yl) * fb);
        }
        else
        {
            dl[0].push (inL + shape (0, yl) * fb);
            dl[1].push (inR + shape (1, yr) * fb);
        }
        L[i] = yl; R[i] = yr;
    }
}

// ============================================================================
// Reverb (8-line FDN with Householder feedback)
void ReverbFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    type.init (s, p + "type"); size.init (s, p + "size"); decay.init (s, p + "decay"); damp.init (s, p + "damp");
    predelay.init (s, p + "predelay"); width.init (s, p + "width"); lowcut.init (s, p + "lowcut");
}
void ReverbFx::prepare (double sr, int)
{
    fs = sr;
    for (auto& l : lines) l.prepare ((int) (sr * 0.2));
    for (auto& d : diffusers) d.prepare ((int) (sr * 0.03));
    pre[0].prepare ((int) (sr * 0.25)); pre[1].prepare ((int) (sr * 0.25));
    early.prepare ((int) (sr * 0.1));
    reset();
}
void ReverbFx::reset()
{
    for (auto& l : lines) l.reset();
    for (auto& d : diffusers) d.reset();
    for (auto& f : dampLp) f.reset();
    pre[0].reset(); pre[1].reset(); early.reset(); lowCut[0].reset(); lowCut[1].reset();
}

void ReverbFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    static const float baseMs[kLines] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.7f, 73.1f };
    static const float typeScale[4] = { 0.6f, 0.45f, 1.3f, 0.8f };
    static const float diffMs[4] = { 4.7f, 3.6f, 12.7f, 9.3f };
    const int t = juce::jlimit (0, 3, type.choice());
    const float sz = 0.35f + size.get (ctx.modOffsets) * 1.2f;
    const float T60 = std::max (0.1f, decay.get (nullptr));
    const float dampHz = 500.0f + 19500.0f * std::pow (1.0f - damp.get (nullptr), 2.0f);
    const float preS = std::max (1.0f, predelay.get (nullptr) * 0.001f * (float) fs);
    const float wd = width.get (nullptr);
    const float diffG = t == 0 ? 0.72f : 0.6f;
    float len[kLines], g[kLines];
    const float maxLen = (float) lines[0].capacity() - 4.0f;
    for (int k = 0; k < kLines; ++k)
    {
        len[k] = juce::jlimit (16.0f, maxLen, baseMs[k] * typeScale[t] * sz * 0.001f * (float) fs);
        g[k] = std::pow (10.0f, -3.0f * len[k] / (T60 * (float) fs));
        dampLp[k].set (tptG (fs, dampHz));
    }
    lowCut[0].set (tptG (fs, lowcut.get (nullptr))); lowCut[1].set (tptG (fs, lowcut.get (nullptr)));

    for (int i = 0; i < n; ++i)
    {
        modPhase += 0.37 / fs;
        modPhase -= std::floor (modPhase);
        pre[0].push (L[i]); pre[1].push (R[i]);
        float inL = pre[0].read (preS), inR = pre[1].read (preS);
        float x = (inL + inR) * 0.5f;
        for (int dI = 0; dI < 4; ++dI)    // Schroeder all-pass diffusion
        {
            const float dd = diffMs[dI] * 0.001f * (float) fs * (t == 1 ? 0.6f : 1.0f);
            const float v = diffusers[dI].read (dd);
            const float w = x + diffG * v;
            diffusers[dI].push (w);
            x = v - diffG * w;
        }

        float outs[kLines], sum = 0.0f;
        const float wobble = (t == 2 ? 6.0f : 2.0f) * std::sin (juce::MathConstants<float>::twoPi * (float) modPhase);
        for (int k = 0; k < kLines; ++k)
        {
            outs[k] = lines[k].read (len[k] + (k % 2 == 0 ? wobble : -wobble));
            sum += outs[k];
        }
        const float hh = sum * (2.0f / kLines);
        for (int k = 0; k < kLines; ++k)
        {
            float fbv = (outs[k] - hh) * g[k];
            fbv = dampLp[k].lp (fbv);
            lines[k].push (fbv + x * (k % 2 == 0 ? 0.5f : -0.5f));
        }

        float yl = (outs[0] + outs[2] + outs[4] + outs[6]) * 0.35f;
        float yr = (outs[1] + outs[3] + outs[5] + outs[7]) * 0.35f;

        if (t == 3 || t == 1)   // club / room early reflections: slap off walls and the bar
        {
            early.push ((inL + inR) * 0.5f);
            static const float tapMs[5] = { 7.0f, 13.0f, 19.0f, 27.0f, 31.0f };
            static const float tapG[5] = { 0.5f, -0.38f, 0.3f, 0.22f, -0.18f };
            float erL = 0.0f, erR = 0.0f;
            for (int e = 0; e < 5; ++e)
            {
                const float v = early.read (tapMs[e] * sz * 0.001f * (float) fs) * tapG[e];
                if (e % 2 == 0) erL += v; else erR += v;
            }
            yl += erL * (t == 3 ? 0.8f : 0.5f);
            yr += erR * (t == 3 ? 0.8f : 0.5f);
        }

        yl = lowCut[0].hp (yl);
        yr = lowCut[1].hp (yr);
        const float mid = (yl + yr) * 0.5f, side = (yl - yr) * 0.5f * wd;
        L[i] = mid + side;
        R[i] = mid - side;
    }
}

// ============================================================================
// Hyper / Dimension
void HyperFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    rate.init (s, p + "rate"); detune.init (s, p + "detune"); voices.init (s, p + "voices"); dim.init (s, p + "dim"); monoBelow.init (s, p + "mono_below");
}
void HyperFx::prepare (double sr, int) { fs = sr; dl[0].prepare ((int) (sr * 0.08)); dl[1].prepare ((int) (sr * 0.08)); reset(); }
void HyperFx::reset() { dl[0].reset(); dl[1].reset(); xo.reset(); for (int v = 0; v < 7; ++v) phase[v] = v / 7.0; }

void HyperFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    xo.set (fs, monoBelow.get (nullptr));
    const float r = 0.05f + rate.get (nullptr) * 2.5f;
    const float dt = detune.get (ctx.modOffsets);
    const int nv = juce::jlimit (1, 7, voices.choice());
    const float dm = dim.get (nullptr);

    for (int i = 0; i < n; ++i)
    {
        float loL, hiL, loR, hiR;
        xo.split (L[i], 0, loL, hiL);
        xo.split (R[i], 1, loR, hiR);
        dl[0].push (hiL); dl[1].push (hiR);
        float accL = 0.0f, accR = 0.0f;
        for (int v = 0; v < nv; ++v)
        {
            phase[v] += r * (1.0 + v * 0.13) / fs;
            phase[v] -= std::floor (phase[v]);
            const float lfo = std::sin (juce::MathConstants<float>::twoPi * (float) phase[v]);
            const float t = (6.0f + (float) v * 2.7f + dt * 3.0f * (1.0f + lfo)) * 0.001f * (float) fs;
            const float tap = dl[v % 2].read (t);
            const float pan = nv == 1 ? 0.0f : (float) v / (float) (nv - 1) * 2.0f - 1.0f;
            accL += tap * (0.5f - 0.5f * pan);
            accR += tap * (0.5f + 0.5f * pan);
        }
        accL /= std::sqrt ((float) nv) * 0.7f;
        accR /= std::sqrt ((float) nv) * 0.7f;
        // dimension: cross-fed short delays widen without obvious chorusing
        const float dimL = dl[1].read ((12.0f + dm * 8.0f) * 0.001f * (float) fs);
        const float dimR = dl[0].read ((15.0f + dm * 9.0f) * 0.001f * (float) fs);
        accL += dimL * dm * 0.6f;
        accR += dimR * dm * 0.6f;
        const float lowMono = (loL + loR) * 0.5f;
        L[i] = lowMono + accL;
        R[i] = lowMono + accR;
    }
}

// ============================================================================
// Saturator / Tape
void SaturatorFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    drive.init (s, p + "drive"); warmth.init (s, p + "warmth"); wow.init (s, p + "wow"); flutter.init (s, p + "flutter");
    tone.init (s, p + "tone"); out.init (s, p + "out");
}
void SaturatorFx::prepare (double sr, int) { fs = sr; dl[0].prepare ((int) (sr * 16.0 * 0.03)); dl[1].prepare ((int) (sr * 16.0 * 0.03)); dc.prepare (sr); reset(); }
void SaturatorFx::reset() { dl[0].reset(); dl[1].reset(); lowShelf.reset(); highShelf.reset(); tiltLow.reset(); tiltHigh.reset(); dc.reset(); for (auto& l : last) l = -99.0f; }

void SaturatorFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    if (ctx.sampleRate != fs) { fs = ctx.sampleRate; dc.prepare (fs); for (auto& l : last) l = -99.0f; }
    const float dr = drive.get (ctx.modOffsets), wa = warmth.get (nullptr), tn = tone.get (nullptr);
    const float w = wow.get (nullptr), fl = flutter.get (nullptr);
    if (wa != last[0] || tn != last[1] || (float) fs != last[2])
    {
        last[0] = wa; last[1] = tn; last[2] = (float) fs;
        lowShelf.set (Biquad::Kind::LowShelf, fs, 120.0f, 0.7f, wa * 6.0f);
        highShelf.set (Biquad::Kind::HighShelf, fs, 6000.0f, 0.7f, -wa * 6.0f);
        tiltLow.set (Biquad::Kind::LowShelf, fs, 800.0f, 0.5f, -tn * 6.0f);
        tiltHigh.set (Biquad::Kind::HighShelf, fs, 800.0f, 0.5f, tn * 6.0f);
    }
    const float pre = dbToGain (dr * 24.0f);
    const float outG = dbToGain (out.get (nullptr)) / std::sqrt (pre) * 1.3f;
    const bool modOn = w > 0.0f || fl > 0.0f;
    const float maxD = (float) dl[0].capacity() - 4.0f;

    for (int i = 0; i < n; ++i)
    {
        wowPhase += 0.55 / fs;   wowPhase -= std::floor (wowPhase);
        flutPhase += 8.7 / fs;   flutPhase -= std::floor (flutPhase);
        const float mod = (w * 2.0f * std::sin (juce::MathConstants<float>::twoPi * (float) wowPhase)
                         + fl * 0.15f * std::sin (juce::MathConstants<float>::twoPi * (float) flutPhase)) * 0.001f * (float) fs;
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
        {
            float x = lowShelf.process (ch[c][i], c);
            x = x * pre;
            x = x >= 0.0f ? std::tanh (x) : std::tanh (x * 0.92f) * 1.02f;
            x = highShelf.process (x, c);
            if (modOn)
            {
                dl[c].push (x);
                x = dl[c].read (juce::jlimit (2.0f, maxD, 5.0f * 0.001f * (float) fs + mod));
            }
            x = tiltHigh.process (tiltLow.process (x, c), c);
            ch[c][i] = dc.process (x, c) * outG;
        }
    }
}

// ============================================================================
// Imager / Utility
void UtilityFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    width.init (s, p + "width"); monoOn.init (s, p + "mono_on"); monoBelow.init (s, p + "mono_below"); gain.init (s, p + "gain");
    pan.init (s, p + "pan"); dcOn.init (s, p + "dc"); invL.init (s, p + "inv_l"); invR.init (s, p + "inv_r");
}
void UtilityFx::prepare (double sr, int) { fs = sr; dc.prepare (sr); reset(); }
void UtilityFx::reset() { xo.reset(); dc.reset(); }

void UtilityFx::process (float* L, float* R, int n, const FxContext& ctx) noexcept
{
    const float wd = width.get (ctx.modOffsets);
    const bool mono = monoOn.on();
    if (mono) xo.set (fs, monoBelow.get (nullptr));
    const float g = dbToGain (gain.get (nullptr));
    const float p = pan.get (nullptr);
    const float pl = std::min (1.0f, 1.0f - p) * g * (invL.on() ? -1.0f : 1.0f);
    const float pr = std::min (1.0f, 1.0f + p) * g * (invR.on() ? -1.0f : 1.0f);
    const bool dcB = dcOn.on();

    for (int i = 0; i < n; ++i)
    {
        float l = L[i], r = R[i];
        if (mono)
        {
            float lo1, hi1, lo2, hi2;
            xo.split (l, 0, lo1, hi1);
            xo.split (r, 1, lo2, hi2);
            const float m = (lo1 + lo2) * 0.5f;
            l = m + hi1; r = m + hi2;
        }
        const float mid = (l + r) * 0.5f, side = (l - r) * 0.5f * wd;
        l = mid + side; r = mid - side;
        if (dcB) { l = dc.process (l, 0); r = dc.process (r, 1); }
        L[i] = l * pl; R[i] = r * pr;
    }
}

// ============================================================================
// Limiter / Clipper (final stage)
void LimiterFx::init (juce::AudioProcessorValueTreeState& s, const juce::String& p)
{
    FxModule::init (s, p);
    mode.init (s, p + "mode"); input.init (s, p + "input"); ceiling.init (s, p + "ceiling"); release.init (s, p + "release"); truePeak.init (s, p + "true_peak");
}
void LimiterFx::prepare (double sr, int) { fs = sr; reset(); }
void LimiterFx::reset() { gain = 1.0f; for (auto& h : hist) h[0] = h[1] = h[2] = 0.0f; }

void LimiterFx::process (float* L, float* R, int n, const FxContext&) noexcept
{
    const int m = mode.choice();
    const float in = dbToGain (input.get (nullptr));
    const float ceil = dbToGain (ceiling.get (nullptr));
    const float limCeil = m == 2 ? ceil * 1.19f : ceil;
    const float rel = onePoleCoef (fs, release.get (nullptr));
    const bool tp = truePeak.on();
    float minGain = 1.0f;

    for (int i = 0; i < n; ++i)
    {
        float l = L[i] * in, r = R[i] * in;
        float peak = std::max (std::abs (l), std::abs (r));
        if (tp)
        {
            // estimate inter-sample peaks with a 4-point Hermite spline through recent samples
            float* xs[2] = { &l, &r };
            for (int c = 0; c < 2; ++c)
            {
                const float xm1 = hist[c][2], x0 = hist[c][1], x1 = hist[c][0], x2 = *xs[c];
                for (float t : { 0.25f, 0.5f, 0.75f })
                {
                    const float c1 = 0.5f * (x1 - xm1), c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
                    peak = std::max (peak, std::abs (((c3 * t + c2) * t + c1) * t + x0));
                }
                hist[c][2] = hist[c][1]; hist[c][1] = hist[c][0]; hist[c][0] = x2;
            }
        }
        if (m != 1)
        {
            const float target = peak * gain > limCeil ? limCeil / std::max (peak, 1.0e-9f) : 1.0f;
            gain = target < gain ? target : target + rel * (gain - target);
            l *= gain; r *= gain;
            minGain = std::min (minGain, gain);
        }
        if (m != 0)
        {
            auto clip = [ceil] (float x)
            {
                const float k = 0.85f * ceil;
                const float a = std::abs (x);
                if (a <= k) return x;
                const float over = (a - k) / (ceil - k);
                const float y = k + (ceil - k) * std::tanh (over);
                return x < 0.0f ? -y : y;
            };
            l = clip (l); r = clip (r);
        }
        L[i] = juce::jlimit (-ceil, ceil, l);
        R[i] = juce::jlimit (-ceil, ceil, r);
    }
    meterGainReduction.store (gainToDb (minGain));
}
} // namespace bgdj
