#pragma once

#include "EngineTypes.h"

namespace bgdj
{
// ============================================================================
class Envelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void noteOn (bool fromCurrentLevel) noexcept
    {
        startLevel = fromCurrentLevel ? level : 0.0f;
        if (! fromCurrentLevel) level = 0.0f;
        stage = Stage::Attack;
        t = 0.0f;
    }
    void noteOff() noexcept
    {
        if (stage != Stage::Idle && stage != Stage::Release)
        {
            relStart = level;
            stage = Stage::Release;
            t = 0.0f;
        }
    }
    void kill() noexcept { stage = Stage::Idle; level = 0.0f; }
    bool isIdle() const noexcept { return stage == Stage::Idle; }
    Stage getStage() const noexcept { return stage; }
    float getLevel() const noexcept { return level; }

    /** Advances by dt seconds. Times in ms. Curves -1..1. */
    float advance (float dt, float attMs, float decMs, float sus, float relMs, float ac, float dc, float rc) noexcept;

private:
    Stage stage = Stage::Idle;
    float level = 0.0f, startLevel = 0.0f, relStart = 0.0f, t = 0.0f;
};

// ============================================================================
class Lfo
{
public:
    void noteOn (const LfoStatic& s, FastRandom& rng, bool retrigger) noexcept;
    /** Advances by dt seconds; returns unipolar (or bipolar if set) value. */
    float advance (float dt, float rateHz, float smooth, const LfoStatic& s, double ppqNow, bool playing, FastRandom& rng) noexcept;
    float getValue() const noexcept { return out; }
    float getPhase() const noexcept { return (float) phase; }

private:
    double phase = 0.0;
    float out = 0.0f, smoothed = 0.0f, riseT = 0.0f;
    float shPrev = 0.0f, shNext = 0.0f;
    bool finished = false, first = true;
};

// ============================================================================
class Voice
{
public:
    void prepare (double osSampleRate, double maxOsSampleRate);
    void setSampleRate (double osSampleRate) noexcept;
    void noteOn (int note, float velocity, int channel, bool legatoTransition, float glideFromNote,
                 const EngineSnapshot& s, uint32_t seed, bool retrigFromCurrent, uint64_t age);
    void retarget (int note, float velocity, const EngineSnapshot& s, bool retriggerEnvs, bool glide, float glideFromNote);
    void noteOff() noexcept;
    void kill() noexcept;

    bool isActive() const noexcept     { return active; }
    float ampLevel() const noexcept    { return env[0].getLevel(); }

    /** Adds this voice's output into L/R (oversampled rate). */
    void render (float* L, float* R, int numSamples, const EngineSnapshot& s) noexcept;

    // ---- public state for the engine -----------------------------------------
    int note = -1, channel = 1;
    float velocity = 0.0f;
    uint64_t age = 0;
    bool keyHeld = false, sustained = false;
    float mpeBend = 0.0f, mpePressure = 0.0f, mpeSlide = 0.0f, polyAftertouch = 0.0f;
    bool glideActive = false;

    std::array<float, (size_t) ModSource::Count> sources {};
    std::array<float, kNumModDestsMax> destOffset {};

    float lfoPhase (int i) const noexcept { return lfo[(size_t) i].getPhase(); }
    float envLevel (int i) const noexcept { return env[(size_t) i].getLevel(); }

private:
    void updateControl (const EngineSnapshot& s, int offsetInSegment) noexcept;
    float effective (const EngineSnapshot& s, int dest) const noexcept;
    float noteToHz (const EngineSnapshot& s, float note) const noexcept;
    void resetOscPhases (const EngineSnapshot& s) noexcept;

    struct WtOsc
    {
        std::array<double, kMaxUnison> phase {};
        std::array<double, kMaxUnison> inc {};
        std::array<float, kMaxUnison> gainL {}, gainR {}, warp {};
        int numUni = 1, mip = 0, frameA = 0, frameB = 0;
        float frameFrac = 0.0f, phaseOffset = 0.0f, levelL = 0.0f, levelR = 0.0f;
        float last = 0.0f;
        float driftCents = 0.0f;
    };

    std::array<WtOsc, kNumWtOscs> osc;
    double subPhase = 0.0, subInc = 0.0, noisePos = 0.0, noiseInc = 0.0;
    float subGainL = 0.0f, subGainR = 0.0f, subLast = 0.0f, noiseLast = 0.0f;
    float subBlep = 0.0f;

    std::array<Envelope, kNumEnvs> env;
    std::array<Lfo, kNumLfos> lfo;
    std::array<VoiceFilter, kNumFilters> filter;
    float filterMix[kNumFilters] {}, filterPanL[kNumFilters] {}, filterPanR[kNumFilters] {};

    double sr = 48000.0;
    int ctrlCountdown = 0;
    float ampStart = 0.0f, ampEnd = 0.0f, velGain = 1.0f;
    float currentNote = 60.0f, targetNote = 60.0f, glideRate = 0.0f;
    float noteRandom = 0.0f, driftState = 0.0f;
    double samplesSinceStart = 0.0;
    bool active = false;
    FastRandom rng;
};
} // namespace bgdj
