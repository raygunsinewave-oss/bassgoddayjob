#pragma once

#include "../Analysis/FeatureExtractor.h"
#include <juce_core/juce_core.h>

namespace bgdj
{
enum class CrowdPersonality { Warehouse = 0, Festival, UndergroundBasement, BeachBar, Afterhours };
const juce::StringArray& crowdPersonalityNames();

/** Smoothed listening state + derived trigger signals, updated on the UI thread. */
struct MoodState
{
    float hype = 25.0f;        // 0..100
    float energy = 0.0f;       // 0..1
    float annoyance = 0.0f;    // 0..100
    float boredom = 0.0f;      // 0..100
    float crowdSize = 0.75f;   // 0..1 fraction of the floor occupied

    // smoothed features
    float loudness = 0.0f, groove = 0.0f, noteRate = 0.0f;
    float subShare = 0.0f, mudShare = 0.0f, harshShare = 0.0f, airShare = 0.0f;
    float centroid = 0.0f, subCorrelation = 1.0f, clipFraction = 0.0f;
    double bpm = 120.0;
    bool hostPlaying = false;
    bool idle = true;          // transport stopped and no notes
    bool silent = true;
    bool walkout = false;      // full walkout in progress (lights on, floor empty)

    // durations
    double silenceSeconds = 0.0, hypeHighSeconds = 0.0, hypeLowSeconds = 0.0, sameNoteSeconds = 0.0;
    double secondsSinceNoteOn = 1.0e9, sweepAge = 1.0e9, quietSeconds = 0.0;

    // edge-triggered signals for this frame
    bool subDropAfterSilence = false;
    bool dropAfterQuiet = false;
    bool resonanceScream = false;
    bool sweepThenDownbeat = false;
    bool clipping = false;
    bool mud = false;
    bool phaseProblem = false;
    bool bored = false;
    bool subSpike = false;
    int noteOnsThisFrame = 0;
    int totalNoteOns = 0;

    double barsHypeHigh() const noexcept { return hypeHighSeconds / (240.0 / std::max (40.0, bpm)); }
    double barsHypeLow() const noexcept  { return hypeLowSeconds / (240.0 / std::max (40.0, bpm)); }
    juce::String moodWord() const;
};

class CrowdMood
{
public:
    void setPersonality (CrowdPersonality p) noexcept { personality = p; }
    CrowdPersonality getPersonality() const noexcept  { return personality; }
    void setCharacter (const juce::String& c)         { character = c.toLowerCase(); }
    const juce::String& getCharacter() const noexcept { return character; }

    /** Feed every block of features received since the last frame. */
    void addFeatures (const AudioFeatures& f);
    /** Advances inertia by dt seconds and computes this frame's edge signals. */
    void tick (float dt);
    /** External boosts (bouncer lets the queue in, etc.). */
    void bumpCrowdSize (float amount) noexcept { state.crowdSize = juce::jlimit (0.05f, 1.0f, state.crowdSize + amount); }
    void endWalkout() noexcept { state.walkout = false; }

    const MoodState& get() const noexcept { return state; }
    MoodState& getMutable() noexcept { return state; }

private:
    MoodState state;
    CrowdPersonality personality = CrowdPersonality::Warehouse;
    juce::String character;

    // accumulators since last tick
    double accSec = 0, accRms = 0, accSub = 0, accMud = 0, accMid = 0, accHarsh = 0, accAir = 0, accFlux = 0, accCentroid = 0;
    double accSubCorr = 0, accClip = 0;
    int accBlocks = 0, accNotes = 0;
    AudioFeatures last;
    float centroidHistory[16] {};
    int centroidIdx = 0;
    float prevSub = 0.0f;
};
} // namespace bgdj
