#pragma once

#include <juce_dsp/juce_dsp.h>
#include "../Common/LockFree.h"

namespace bgdj
{
class BassGodsProcessor;

/** UI-thread analysis hub: drains the processor's sample FIFOs into history rings and
    computes spectra, scope, goniometer, tuner, correlation, sub and DC readings on demand. */
class AnalysisData
{
public:
    explicit AnalysisData (BassGodsProcessor& p);

    /** Call from a UI timer. Returns the number of new samples. */
    int update();

    double getSampleRate() const noexcept { return sampleRate; }

    /** Copies the most recent n samples (n <= kHistory). which: 0 = post FX, 1 = pre FX. */
    void getLatest (int which, float* L, float* R, int n) const;

    /** Magnitude spectrum in dB of the mono sum, with slope (dB/oct around 1 kHz). */
    void computeSpectrum (int which, int fftOrder, float slopeDbPerOct, std::vector<float>& outDb);

    float correlation (int numSamples = 4096) const;
    float subCorrelation() const noexcept { return subCorr; }
    float subRmsDb() const noexcept { return subRms; }
    float dcOffset (int ch) const noexcept { return dc[ch]; }

    /** Fundamental (Hz, 0 if none) of the pre-FX signal plus confidence 0..1. */
    float detectPitch (float& confidence);
    static juce::String noteName (float hz, float& centsOff);

    int samplesSinceStart() const noexcept { return totalSamples; }

    static constexpr int kHistory = 1 << 16;

private:
    struct Ring
    {
        std::vector<float> L = std::vector<float> ((size_t) kHistory), R = std::vector<float> ((size_t) kHistory);
        int pos = 0;
        void write (const float* l, const float* r, int n);
    };

    BassGodsProcessor& proc;
    Ring post, pre;
    std::vector<float> scratchL, scratchR;
    double sampleRate = 48000.0;
    int totalSamples = 0;
    std::map<int, std::unique_ptr<juce::dsp::FFT>> ffts;
    std::vector<float> fftBuf, window;
    float subRms = -100.0f, subCorr = 1.0f, dc[2] {};
    float subLpL[2] {}, subLpR[2] {}, subHpStateL = 0, subHpStateR = 0;
};
} // namespace bgdj
