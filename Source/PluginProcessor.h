#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Common/Parameters.h"
#include "Common/LockFree.h"
#include "Engine/SynthEngine.h"
#include "DSP/FX/FxRack.h"
#include "Analysis/Loudness.h"
#include "Analysis/FeatureExtractor.h"
#include "Scene/CrowdSfx.h"
#include "Presets/PresetFormat.h"
#include "Settings/AppSettings.h"

namespace bgdj
{
class PresetManager;

class BassGodsProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    BassGodsProcessor();
    ~BassGodsProcessor() override;

    // ---- AudioProcessor ------------------------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BASSGODSDAYJOB"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return currentPresetName; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- public objects ----------------------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    SharedSettings settings;
    ModMatrix modMatrix;
    FxRack fxRack;
    SynthEngine engine;
    juce::MidiKeyboardState keyboardState;
    std::unique_ptr<PresetManager> presets;

    // ---- wavetables (message thread API) ---------------------------------------------------------
    void setFactoryTable (int osc, int index);
    void setUserTable (int osc, std::shared_ptr<const Wavetable> table);
    std::shared_ptr<const Wavetable> getTable (int osc) const { return oscTables[(size_t) osc].table.getShared(); }
    bool isUserTable (int osc) const { return oscTables[(size_t) osc].isUser; }
    juce::String getTableName (int osc) const;
    std::atomic<int> tableVersion { 0 };

    // ---- LFO / ducker drawn shapes -----------------------------------------------------------------
    void setLfoPoints (int lfo, const CurvePoints& pts);
    const CurvePoints& getLfoPoints (int lfo) const { return lfoPoints[(size_t) lfo]; }
    void setDuckPoints (const CurvePoints& pts);
    const CurvePoints& getDuckPoints() const { return duckPoints; }
    std::atomic<int> shapeVersion { 0 };

    // ---- macros labels ---------------------------------------------------------------------------------
    juce::StringArray macroLabels { "Macro 1", "Macro 2", "Macro 3", "Macro 4" };

    // ---- tuning -------------------------------------------------------------------------------------------
    bool loadTuningFile (const juce::File& f, juce::String& error);
    void resetTuning();
    juce::String getTuningName() const;

    // ---- presets ------------------------------------------------------------------------------------------------
    void applyPreset (const PresetData& p);
    PresetData capturePreset (const juce::String& name) const;
    juce::String currentPresetName { "Init" };
    juce::String currentPresetCharacter;
    std::atomic<int> presetVersion { 0 };

    // ---- MIDI learn ---------------------------------------------------------------------------------------------
    void startMidiLearn (const juce::String& paramID);
    void cancelMidiLearn()  { learnTarget.store (-1); }
    void clearMidiMapping (const juce::String& paramID);
    int getCcForParam (const juce::String& paramID) const;
    juce::var midiMapToVar() const;
    void midiMapFromVar (const juce::var& v);
    bool isLearning() const noexcept { return learnTarget.load() >= 0; }
    std::atomic<int> midiMapVersion { 0 };

    // ---- analysis (audio -> UI) ------------------------------------------------------------------------------------
    AudioSampleFifo preFxFifo, postFxFifo;
    SpscFifo<AudioFeatures, 512> featureFifo;
    LoudnessMeter loudness;
    std::atomic<bool> analyzersEnabled { true }, editorOpen { false };
    std::atomic<float> sampleRateForUi { 48000.0f };
    std::atomic<double> uiBpm { 120.0 }, uiPpq { 0.0 };
    std::atomic<bool> uiPlaying { false };

    // ---- scene SFX -----------------------------------------------------------------------------------------------------
    CrowdSfx crowdSfx;

    // ---- diagnostics -------------------------------------------------------------------------------------------------------
    juce::AudioProcessLoadMeasurer loadMeasurer;
    std::atomic<int> lastBlockSize { 0 };
    juce::String getHostDescription() const;
    void resetXruns() { xrunBase = loadMeasurer.getXRunCount(); }
    int getXruns() const { return loadMeasurer.getXRunCount() - xrunBase; }

    /** Applies "Safe Mode": scene off, oversampling 1x, analyzers off. */
    void applySafeMode (bool on);

private:
    void timerCallback() override;
    void ensureTables (bool force);
    void renderSynth (float* L, float* R, int n, const juce::MidiBuffer& midi, int osIdx);
    void handleMidiLearn (const juce::MidiBuffer& midi);
    void restoreExtras (const juce::ValueTree& extra);
    juce::ValueTree saveExtras() const;

    struct OscTable
    {
        RtSharedObject<Wavetable> table;
        bool isUser = false;
        int loadedFactory = -1;
        juce::String userName;
    };
    std::array<OscTable, kNumWtOscs> oscTables;
    std::array<CurvePoints, kNumLfos> lfoPoints;
    std::array<RtSharedObject<ShapeTable>, kNumLfos> lfoTables;
    CurvePoints duckPoints;
    RtSharedObject<TuningTable> tuning;

    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, kMaxOversampleExp> synthOversamplers;
    Crossover bassMono;
    DcBlocker dcBlock;
    juce::AudioBuffer<float> preFx, workBuffer;
    juce::MidiBuffer chunkMidi;
    FeatureExtractor features;
    float lastMacros[kNumMacros] {};
    float masterGainSmooth = 1.0f;

    std::array<std::atomic<int>, 128> ccMap;
    std::atomic<int> learnTarget { -1 };
    juce::Array<juce::AudioProcessorParameter*> paramList;

    std::atomic<float>* pMasterVol = nullptr; std::atomic<float>* pBassMonoOn = nullptr; std::atomic<float>* pBassMonoFreq = nullptr;
    std::atomic<float>* pOsSynth = nullptr;   std::atomic<float>* pOsFx = nullptr;       std::atomic<float>* pOsOffline = nullptr;
    std::atomic<float>* pDenormal = nullptr;  std::atomic<float>* pDc = nullptr;
    std::atomic<float>* pSfxOut = nullptr;    std::atomic<float>* pSfxLevel = nullptr;
    std::atomic<float>* pTable[kNumWtOscs] {};
    int masterDest = -1;

    double currentSampleRate = 48000.0;
    int maxBlock = 512;
    int xrunBase = 0;
    juce::String hostName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassGodsProcessor)
};
} // namespace bgdj
