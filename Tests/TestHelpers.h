#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Common/Parameters.h"

namespace bgdj::test
{
/** Minimal processor that only hosts the real parameter layout. */
struct LayoutHost : public juce::AudioProcessor
{
    LayoutHost()
        : juce::AudioProcessor (BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo())),
          apvts (*this, nullptr, "S", params::createLayout()) {}
    const juce::String getName() const override { return "LayoutHost"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    void set (const juce::String& id, float realValue)
    {
        auto* p = apvts.getParameter (id);
        jassert (p != nullptr);
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
    }

    juce::AudioProcessorValueTreeState apvts;
};

inline bool allFinite (const std::vector<float>& v)
{
    for (float x : v) if (! std::isfinite (x)) return false;
    return true;
}
} // namespace bgdj::test
