#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <vector>

namespace bgdj
{
/** ITU-R BS.1770 / EBU R128 loudness meter that runs on the audio thread without
    allocating: momentary (400 ms), short-term (3 s), gated integrated loudness,
    loudness range, true-peak estimate, RMS and crest factor. Results are atomics. */
class LoudnessMeter
{
public:
    void prepare (double sampleRate);
    void process (const float* L, const float* R, int n) noexcept;
    void requestReset() noexcept { resetRequested.store (true); }

    std::atomic<float> momentary { -100.0f }, shortTerm { -100.0f }, integrated { -100.0f }, lra { 0.0f };
    std::atomic<float> truePeakMaxDb { -100.0f }, rmsDbL { -100.0f }, rmsDbR { -100.0f }, crestDb { 0.0f };
    std::atomic<float> peakDbL { -100.0f }, peakDbR { -100.0f };

private:
    struct Biq
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] {}, z2[2] {};
        double p (double x, int c) noexcept
        {
            const double y = b0 * x + z1[c];
            z1[c] = b1 * x - a1 * y + z2[c];
            z2[c] = b2 * x - a2 * y;
            return y;
        }
    };

    void finishHop() noexcept;
    void doReset() noexcept;
    static constexpr int kBins = 1500;   // -70 .. +5 LUFS in 0.05 LU bins
    static int binFor (double lufs) noexcept { return juce::jlimit (0, kBins - 1, (int) ((lufs + 70.0) * 20.0)); }
    static double lufsForBin (int b) noexcept { return -70.0 + (b + 0.5) / 20.0; }

    Biq pre, rlb;
    double fs = 48000.0;
    int hopLen = 4800, hopCount = 0;
    double hopSum = 0.0;
    std::array<double, 30> hops {};
    int hopIndex = 0, hopsFilled = 0;
    std::array<uint32_t, kBins> intHist {}, stHist {};
    double rmsAccL = 0, rmsAccR = 0, peakHoldL = 0, peakHoldR = 0, tpMax = 0;
    float hist[2][3] {};
    std::atomic<bool> resetRequested { false };
};
} // namespace bgdj
