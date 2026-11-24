#pragma once

#include "../Common/LockFree.h"
#include "../DSP/Filter.h"

namespace bgdj
{
/** Tiny procedural sound-effect synth for the club scene (cheers, boos, air horn...).
    Cues arrive from the UI through a lock-free FIFO; rendering runs on the audio thread.
    The processor only mixes it into the output when "Crowd SFX to output" is enabled. */
class CrowdSfx
{
public:
    enum Cue : int { None = 0, Cheer, Oooh, OneMore, Rewind, AirHorn, Boo, PlayElse, Ringtone, Gasp, Pigeon,
                     Meow, Crash, Cup, Chatter, Snore, Co2, Smoke, NumCues };

    static Cue cueFromName (const juce::String& name);

    void prepare (double sampleRate);
    /** UI thread. */
    void trigger (Cue c) noexcept { if (c != None) queue.push ((int) c); }
    /** Audio thread: adds into L/R with the given gain. */
    void render (float* L, float* R, int n, float gain) noexcept;

private:
    struct VoiceState
    {
        Cue cue = None;
        double t = 0.0, dur = 1.0, ph1 = 0, ph2 = 0, ph3 = 0;
        Svf f1, f2;
        float pan = 0.0f;
        FastRandom rng;
    };
    float renderVoice (VoiceState& v) noexcept;

    SpscFifo<int, 64> queue;
    std::array<VoiceState, 8> voices;
    double fs = 48000.0;
    FastRandom rng;
};
} // namespace bgdj
