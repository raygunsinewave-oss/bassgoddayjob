#include "FeatureExtractor.h"

namespace bgdj
{
void FeatureExtractor::prepare (double sampleRate)
{
    fs = sampleRate;
    subLp.set (fs, 80.0f, 0.707f);
    subHp.set (fs, 20.0f, 0.707f);
    mud.set (fs, 245.0f, 1.2f);     // ~150-400 Hz
    mid.set (fs, 1000.0f, 0.9f);
    harsh.set (fs, 3500.0f, 1.0f);  // ~2-6 kHz
    air.set (fs, 10000.0f, 0.707f);
    corrLpL.set (fs, 120.0f, 0.707f);
    corrLpR.set (fs, 120.0f, 0.707f);
    for (auto& p : prevLog) p = -10.0f;
    clock = 0.0;
}

AudioFeatures FeatureExtractor::analyse (const float* preL, const float* preR, const float* L, const float* R, int n) noexcept
{
    AudioFeatures f;
    if (n <= 0)
        return f;

    double eSub = 0, eMud = 0, eMid = 0, eHarsh = 0, eAir = 0, eTot = 0;
    double lr = 0, ll = 0, rr = 0, slr = 0, sll = 0, srr = 0;
    float peak = 0.0f;
    int clips = 0;

    for (int i = 0; i < n; ++i)
    {
        const float l = L[i], r = R[i];
        const float m = 0.5f * (l + r);
        const float s = subHp.svf.process (subLp.svf.process (m).lp).hp;
        const float md = mud.svf.process (m).bp;
        const float mi = mid.svf.process (m).bp;
        const float h = harsh.svf.process (m).bp;
        const float a = air.svf.process (m).hp;
        eSub += s * s; eMud += md * md; eMid += mi * mi; eHarsh += h * h; eAir += a * a; eTot += m * m;
        lr += l * r; ll += l * l; rr += r * r;
        const float sl = corrLpL.svf.process (l).lp, sr = corrLpR.svf.process (r).lp;
        slr += sl * sr; sll += sl * sl; srr += sr * sr;
        peak = std::max ({ peak, std::abs (l), std::abs (r) });
        if (std::abs (preL[i]) >= 1.0f || std::abs (preR[i]) >= 1.0f)
            ++clips;
    }

    const float inv = 1.0f / (float) n;
    f.blockSec = (float) (n / fs);
    clock += n / fs;
    f.timeSec = clock;
    f.rms = std::sqrt ((float) eTot * inv);
    f.peak = peak;
    f.lufsM = f.rms > 1.0e-6f ? 20.0f * std::log10 (f.rms) - 0.691f : -100.0f;
    f.clipCount = clips;
    f.clipFraction = (float) clips * inv;
    f.sub = (float) eSub * inv; f.mud = (float) eMud * inv; f.mid = (float) eMid * inv;
    f.harsh = (float) eHarsh * inv; f.air = (float) eAir * inv;

    const float bandE[5] = { f.sub, f.mud, f.mid, f.harsh, f.air };
    const float bandHz[5] = { 45.0f, 250.0f, 1000.0f, 3500.0f, 12000.0f };
    float wsum = 0.0f, esum = 0.0f, flux = 0.0f;
    for (int b = 0; b < 5; ++b)
    {
        wsum += bandE[b] * bandHz[b];
        esum += bandE[b];
        const float lg = std::log10 (bandE[b] + 1.0e-10f);
        flux += std::max (0.0f, lg - prevLog[b]);
        prevLog[b] = lg;
    }
    f.centroidHz = esum > 1.0e-10f ? wsum / esum : 0.0f;
    f.flux = flux;
    f.correlation = (ll > 1.0e-12 && rr > 1.0e-12) ? (float) (lr / std::sqrt (ll * rr)) : 1.0f;
    f.subCorrelation = (sll > 1.0e-12 && srr > 1.0e-12) ? (float) (slr / std::sqrt (sll * srr)) : 1.0f;
    return f;
}
} // namespace bgdj
