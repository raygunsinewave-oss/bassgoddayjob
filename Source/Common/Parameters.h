#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace bgdj
{
// ============================================================================
// Engine-wide constants
// ============================================================================
constexpr int kNumWtOscs        = 3;    // OSC A, B, C
constexpr int kNumEnvs          = 4;
constexpr int kNumLfos          = 8;
constexpr int kNumMacros        = 4;
constexpr int kNumFilters       = 2;
constexpr int kMaxVoices        = 16;
constexpr int kMaxUnison        = 16;
constexpr int kNumModSlots      = 64;
constexpr int kNumFxSlots       = 12;   // reorderable rack slots
constexpr int kNumFxModules     = 13;   // 12 rack modules + final limiter
constexpr int kLfoTableSize     = 256;
constexpr int kMaxOversampleExp = 4;    // 2^4 = 16x

// ============================================================================
// Mod sources
// ============================================================================
enum class ModSource : int
{
    None = 0,
    Env1, Env2, Env3, Env4,
    Lfo1, Lfo2, Lfo3, Lfo4, Lfo5, Lfo6, Lfo7, Lfo8,
    Macro1, Macro2, Macro3, Macro4,
    Velocity, NoteRandom, KeyTrack, ModWheel, PitchBend, Aftertouch,
    MpeSlide, MpePressure, Noise,
    Count
};

// ============================================================================
// FX modules (order is the parameter-prefix order, not the rack order)
// ============================================================================
enum class FxType : int
{
    Distortion = 0, Ott, Compressor, Ducker, Eq, FilterFx, Modulation,
    Delay, Reverb, Hyper, Saturator, Utility, Limiter, Count
};

namespace params
{
    // ---- choice lists (presets store choices by these names) -------------
    const juce::StringArray& warpModes();
    const juce::StringArray& warpSources();
    const juce::StringArray& oscRoutes();
    const juce::StringArray& subShapes();
    const juce::StringArray& oscDModes();
    const juce::StringArray& filterTypes();
    const juce::StringArray& filterRoutings();
    const juce::StringArray& polyModes();
    const juce::StringArray& portaModes();
    const juce::StringArray& portaTypes();
    const juce::StringArray& lfoModes();
    const juce::StringArray& lfoShapes();
    const juce::StringArray& syncRates();
    double syncRateBeats (int index);   // length of one cycle in quarter notes
    const juce::StringArray& oversampleFactors();
    const juce::StringArray& offlineOversample();
    const juce::StringArray& interpModes();
    const juce::StringArray& aaQualities();
    const juce::StringArray& unisonCurves();
    const juce::StringArray& unisonPhaseModes();
    const juce::StringArray& voiceStealModes();
    const juce::StringArray& envRetrigModes();
    const juce::StringArray& filterSatModes();
    const juce::StringArray& mpeZones();
    const juce::StringArray& modSourceNames();

    const juce::StringArray& fxModuleIds();     // parameter prefixes "dist", "ott", ...
    const juce::StringArray& fxModuleNames();   // display names
    const juce::StringArray& distModes();
    const juce::StringArray& compTypes();
    const juce::StringArray& duckRates();
    double duckRateBeats (int index);
    const juce::StringArray& duckShapes();
    const juce::StringArray& eqTypes();
    const juce::StringArray& ffxTypes();
    const juce::StringArray& modFxModes();
    const juce::StringArray& delayModes();
    const juce::StringArray& verbTypes();
    const juce::StringArray& limModes();

    // ---- ID helpers --------------------------------------------------------
    juce::String oscPrefix (int osc);             // "oA_", "oB_", "oC_"
    inline juce::String oscD()                    { return "oD_"; }
    juce::String filterPrefix (int f);            // "f1_", "f2_"
    juce::String envPrefix (int e);               // "e1_".."e4_"
    juce::String lfoPrefix (int l);               // "l1_".."l8_"
    juce::String macroId (int m);                 // "macro1".."macro4"
    juce::String fxPrefix (FxType t);             // "fx_dist_" ...

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // ---- modulation destinations --------------------------------------------
    struct ModDest
    {
        juce::String paramID;
        juce::String displayName;
        bool perVoice;
    };
    const std::vector<ModDest>& modDestinations();
    int modDestIndexForParam (const juce::String& paramID);   // -1 if not modulatable

    // ---- sections, for "randomize section" in the preset browser ------------
    juce::StringArray paramsInSection (const juce::String& section,
                                       const juce::AudioProcessorValueTreeState& state);
} // namespace params
} // namespace bgdj
