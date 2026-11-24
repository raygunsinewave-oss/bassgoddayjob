#include "AnalysisData.h"
#include "../PluginProcessor.h"

namespace bgdj
{
void AnalysisData::Ring::write (const float* l, const float* r, int n)
{
    for (int i = 0; i < n; ++i)
    {
        L[(size_t) pos] = l[i];
        R[(size_t) pos] = r[i];
        pos = (pos + 1) & (kHistory - 1);
    }
}

AnalysisData::AnalysisData (BassGodsProcessor& p) : proc (p)
{
    scratchL.resize (16384);
    scratchR.resize (16384);
}

int AnalysisData::update()
{
    sampleRate = proc.sampleRateForUi.load();
    int got = 0, n;
    while ((n = proc.postFxFifo.pop (scratchL.data(), scratchR.data(), (int) scratchL.size())) > 0)
    {
        post.write (scratchL.data(), scratchR.data(), n);
        // sub band (20-120 Hz) RMS + correlation using simple one-pole filters
        const float aLp = std::exp (-juce::MathConstants<float>::twoPi * 120.0f / (float) sampleRate);
        const float aHp = std::exp (-juce::MathConstants<float>::twoPi * 20.0f / (float) sampleRate);
        double e = 0, lr = 0, ll = 0, rr = 0, sumL = 0, sumR = 0;
        for (int i = 0; i < n; ++i)
        {
            subLpL[0] = scratchL[(size_t) i] + aLp * (subLpL[0] - scratchL[(size_t) i]);
            subLpL[1] = subLpL[0] + aLp * (subLpL[1] - subLpL[0]);
            subLpR[0] = scratchR[(size_t) i] + aLp * (subLpR[0] - scratchR[(size_t) i]);
            subLpR[1] = subLpR[0] + aLp * (subLpR[1] - subLpR[0]);
            subHpStateL = subLpL[1] + aHp * (subHpStateL - subLpL[1]);
            subHpStateR = subLpR[1] + aHp * (subHpStateR - subLpR[1]);
            const float sl = subLpL[1] - subHpStateL, sr = subLpR[1] - subHpStateR;
            e += 0.5 * (sl * sl + sr * sr);
            lr += sl * sr; ll += sl * sl; rr += sr * sr;
            sumL += scratchL[(size_t) i]; sumR += scratchR[(size_t) i];
        }
        const float db = (float) (10.0 * std::log10 (e / n + 1.0e-12));
        subRms = subRms * 0.7f + db * 0.3f;
        if (ll > 1.0e-9 && rr > 1.0e-9)
            subCorr = subCorr * 0.8f + 0.2f * (float) (lr / std::sqrt (ll * rr));
        dc[0] = dc[0] * 0.95f + 0.05f * (float) (sumL / n);
        dc[1] = dc[1] * 0.95f + 0.05f * (float) (sumR / n);
        got += n;
    }
    while ((n = proc.preFxFifo.pop (scratchL.data(), scratchR.data(), (int) scratchL.size())) > 0)
        pre.write (scratchL.data(), scratchR.data(), n);
    totalSamples += got;
    return got;
}

void AnalysisData::getLatest (int which, float* L, float* R, int n) const
{
    const auto& ring = which == 0 ? post : pre;
    n = std::min (n, kHistory);
    int start = (ring.pos - n + kHistory) & (kHistory - 1);
    for (int i = 0; i < n; ++i)
    {
        L[i] = ring.L[(size_t) start];
        R[i] = ring.R[(size_t) start];
        start = (start + 1) & (kHistory - 1);
    }
}

void AnalysisData::computeSpectrum (int which, int order, float slope, std::vector<float>& outDb)
{
    order = juce::jlimit (10, 15, order);
    const int N = 1 << order;
    auto& fft = ffts[order];
    if (fft == nullptr)
        fft = std::make_unique<juce::dsp::FFT> (order);
    if ((int) window.size() != N)
    {
        window.resize ((size_t) N);
        juce::dsp::WindowingFunction<float>::fillWindowingTables (window.data(), (size_t) N, juce::dsp::WindowingFunction<float>::blackmanHarris, false);
    }
    fftBuf.assign ((size_t) N * 2, 0.0f);
    std::vector<float> l ((size_t) N), r ((size_t) N);
    getLatest (which, l.data(), r.data(), N);
    for (int i = 0; i < N; ++i)
        fftBuf[(size_t) i] = 0.5f * (l[(size_t) i] + r[(size_t) i]) * window[(size_t) i];
    fft->performFrequencyOnlyForwardTransform (fftBuf.data(), true);

    outDb.resize ((size_t) N / 2);
    const float norm = 4.0f / (float) N;   // window gain compensation (approx.)
    for (int b = 0; b < N / 2; ++b)
    {
        const float hz = (float) b * (float) sampleRate / (float) N;
        float db = 20.0f * std::log10 (fftBuf[(size_t) b] * norm + 1.0e-9f);
        if (slope != 0.0f && hz > 1.0f)
            db += slope * std::log2 (hz / 1000.0f);
        outDb[(size_t) b] = db;
    }
}

float AnalysisData::correlation (int n) const
{
    std::vector<float> l ((size_t) n), r ((size_t) n);
    getLatest (0, l.data(), r.data(), n);
    double lr = 0, ll = 0, rr = 0;
    for (int i = 0; i < n; ++i) { lr += l[(size_t) i] * r[(size_t) i]; ll += l[(size_t) i] * l[(size_t) i]; rr += r[(size_t) i] * r[(size_t) i]; }
    if (ll < 1.0e-9 || rr < 1.0e-9) return 1.0f;
    return (float) (lr / std::sqrt (ll * rr));
}

float AnalysisData::detectPitch (float& confidence)
{
    // YIN on a 4x-decimated pre-FX window (bass fundamentals: 25 Hz .. 1 kHz)
    constexpr int dec = 4, W = 1024;
    std::vector<float> l ((size_t) W * dec * 2), r ((size_t) W * dec * 2);
    getLatest (1, l.data(), r.data(), (int) l.size());
    std::vector<float> x ((size_t) W * 2);
    for (int i = 0; i < W * 2; ++i)
    {
        float s = 0.0f;
        for (int k = 0; k < dec; ++k) s += l[(size_t) (i * dec + k)] + r[(size_t) (i * dec + k)];
        x[(size_t) i] = s;
    }
    const double fs = sampleRate / dec;
    const int minLag = std::max (2, (int) (fs / 1000.0)), maxLag = std::min (W - 1, (int) (fs / 25.0));
    std::vector<float> d ((size_t) maxLag + 1, 0.0f);
    double energy = 0;
    for (int i = 0; i < W; ++i) energy += x[(size_t) i] * x[(size_t) i];
    if (energy < 1.0e-6) { confidence = 0.0f; return 0.0f; }
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        double s = 0;
        for (int i = 0; i < W; ++i) { const float df = x[(size_t) i] - x[(size_t) (i + tau)]; s += df * df; }
        d[(size_t) tau] = (float) s;
    }
    double running = 0;
    int best = -1;
    std::vector<float> cmnd ((size_t) maxLag + 1, 1.0f);
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        running += d[(size_t) tau];
        cmnd[(size_t) tau] = running > 0 ? (float) (d[(size_t) tau] * tau / running) : 1.0f;
    }
    for (int tau = minLag; tau < maxLag; ++tau)
        if (cmnd[(size_t) tau] < 0.15f)
        {
            while (tau + 1 < maxLag && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
            best = tau;
            break;
        }
    if (best < 0) { confidence = 0.0f; return 0.0f; }
    const float a = cmnd[(size_t) best - 1], b = cmnd[(size_t) best], c = cmnd[(size_t) best + 1];
    const float denom = a - 2 * b + c;
    const float shift = std::abs (denom) > 1.0e-9f ? 0.5f * (a - c) / denom : 0.0f;
    confidence = juce::jlimit (0.0f, 1.0f, 1.0f - b);
    return (float) (fs / ((float) best + shift));
}

juce::String AnalysisData::noteName (float hz, float& cents)
{
    if (hz <= 0.0f) { cents = 0.0f; return "--"; }
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const float midi = 69.0f + 12.0f * std::log2 (hz / 440.0f);
    const int n = juce::roundToInt (midi);
    cents = (midi - (float) n) * 100.0f;
    return juce::String (names[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1);
}
} // namespace bgdj
