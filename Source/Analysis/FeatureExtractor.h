#pragma once

#include "../Common/LockFree.h"
#include "../DSP/Filter.h"

namespace bgdj
{
/** One block's worth of "what the crowd hears". Trivially copyable; travels through a FIFO. */
struct AudioFeatures
{
    double timeSec = 0.0;          // audio-thread clock
    float blockSec = 0.0f;
    float rms = 0.0f, peak = 0.0f, lufsM = -100.0f;
    float clipFraction = 0.0f;     // samples over full scale (pre-limiter) / block length
    int clipCount = 0;
    float sub = 0.0f, mud = 0.0f, mid = 0.0f, harsh = 0.0f, air = 0.0f;   // band energies (linear, mean square)
    float centroidHz = 0.0f, flux = 0.0f;
    float correlation = 1.0f, subCorrelation = 1.0f;
    int noteOns = 0, lowestNote = -1, highestNote = -1, heldNotes = 0, activeVoices = 0;
    float bendMovement = 0.0f;
    bool glide = false;
    float macroMovement = 0.0f;
    double bpm = 120.0, ppq = 0.0;
    bool hostPlaying = false;
    double sameNoteSeconds = 0.0, secondsSinceNoteOn = 0.0;
};

/** Lightweight per-block analysis, safe for the audio thread. */
class FeatureExtractor
{
public:
    void prepare (double sampleRate);
    /** preFx = synth output (for clipping), out = final output. */
    AudioFeatures analyse (const float* preL, const float* preR, const float* L, const float* R, int n) noexcept;

private:
    struct Band
    {
        Svf svf;
        void set (double fs, float hz, float q)
        {
            svf.set (std::tan (juce::MathConstants<float>::pi * hz / (float) fs), 1.0f / q);
        }
    };
    Band subLp, subHp, mud, mid, harsh, air, corrLpL, corrLpR;
    double fs = 48000.0;
    float prevLog[5] {};
    double clock = 0.0;
};
} // namespace bgdj
