#pragma once

#include "Voice.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace bgdj
{
/** Raw parameter pointers used to build the per-block snapshot. */
struct SynthParamRefs
{
    struct Osc { std::atomic<float> *on, *table, *warp, *warpSrc, *uni, *oct, *semi, *rand, *route, *phaseReset; };
    Osc osc[kNumWtOscs];
    std::atomic<float> *dOn, *dMode, *dSubShape, *dSubOct, *dSubLock, *dNoiseType, *dNoiseKey, *dRoute, *dPhaseReset;
    std::atomic<float> *fOn[kNumFilters], *fType[kNumFilters], *fKey[kNumFilters];
    std::atomic<float> *filterRouting;
    struct LfoR { std::atomic<float> *sync, *syncRate, *mode, *shape, *phase, *rise, *bipolar; };
    LfoR lfo[kNumLfos];
    std::atomic<float> *envCurve[kNumEnvs][3];
    std::atomic<float> *polyMode, *polyVoices, *portaMode, *portaType, *pbRange, *voiceSteal, *envRetrig, *legatoEnvReset;
    std::atomic<float> *interp, *aaQuality, *uniCurve, *uniPhase, *drift, *velCurve, *filtKeyRef, *filtSat;
    std::atomic<float> *macros[kNumMacros];
    std::atomic<float> *tuneA4, *seedLock, *seedValue, *mpeOn, *mpeZone, *mpeMembers, *mpePbRange;
    std::vector<std::atomic<float>*> destRaw;
    std::vector<juce::RangedAudioParameter*> destParam;

    void init (juce::AudioProcessorValueTreeState& apvts);
};

/** Per-block performance statistics consumed by the crowd feature extractor. */
struct EngineStats
{
    int noteOns = 0;
    int lowestHeld = -1, highestHeld = -1, heldCount = 0;
    int activeVoices = 0;
    float bendMovement = 0.0f;
    bool glideActive = false;
    int lastNote = -1;
    double sameNoteSeconds = 0.0;
    double secondsSinceNoteOn = 1.0e9;
};

/** Holds the external (non-parameter) objects the snapshot points at. */
struct EngineSources
{
    const Wavetable* tables[kNumWtOscs] {};
    const ShapeTable* lfoTables[kNumLfos] {};
    const NoiseSet* noise = nullptr;
    const TuningTable* tuning = nullptr;
    const ModMatrix* matrix = nullptr;
};

class SynthEngine
{
public:
    SynthEngine();

    void init (juce::AudioProcessorValueTreeState& apvts);
    void prepare (double baseSampleRate, int osFactor);
    void reset();

    /** Builds the snapshot from parameters. Call once per block before render(). */
    void beginBlock (const EngineSources& src, double bpm, double ppq, bool playing, int osFactor);

    /** Renders voices (oversampled) and consumes MIDI. osL/osR must hold numBaseSamples * osFactor samples, zeroed. */
    void render (float* osL, float* osR, int numBaseSamples, const juce::MidiBuffer& midi);

    void allNotesOff();
    void injectNoteOn (int note, float vel);       // on-screen keyboard is merged via MidiKeyboardState instead
    const EngineStats& getStats() const noexcept   { return stats; }
    const EngineSnapshot& getSnapshot() const noexcept { return snap; }

    /** Destination offsets from the most recently triggered voice (for global destinations). */
    const std::array<float, kNumModDestsMax>& getGlobalOffsets() const noexcept { return globalOffsets; }

    /** For the UI: current LFO phases / env levels of the newest voice (lock-free reads). */
    std::atomic<float> uiLfoPhase[kNumLfos] {};
    std::atomic<float> uiEnvLevel[kNumEnvs] {};
    std::atomic<float> uiLfoValue[kNumLfos] {};
    std::atomic<int> uiActiveVoices { 0 };

    float getModWheel() const noexcept { return snap.modWheel; }
    float getPitchBendNorm() const noexcept { return snap.pitchBend; }

private:
    void handleMidi (const juce::MidiMessage& m);
    void noteOn (int ch, int note, float vel);
    void noteOff (int ch, int note);
    void setSustain (bool down);
    int findVoiceToSteal();
    uint32_t seedFor (int note);
    bool isMpeMember (int ch) const noexcept;
    Voice* newestActiveVoice();

    void renderSegment (float* L, float* R, int startBase, int endBase);

    SynthParamRefs P;
    EngineSnapshot snap;
    std::array<Voice, kMaxVoices> voices;
    std::array<float, kNumModDestsMax> globalOffsets {};
    std::array<int, 128> noteStack {};
    int stackSize = 0;
    bool sustainDown = false;
    int lastPlayedNote = -1;
    uint64_t ageCounter = 0;
    double baseRate = 48000.0;
    int osFactor = 1;
    double blockPpq = 0.0;
    float lastBend = 0.0f;
    FastRandom rng;
    EngineStats stats;
    ShapeTable builtinLfo[16];
    bool initialised = false;
};
} // namespace bgdj
