#pragma once

#include "../Widgets.h"
#include "../CurveEditors.h"
#include "../../Scene/ClubScene.h"

namespace bgdj
{
class BassGodsProcessor;

/** SYNTH tab: macros + club scene + crowd readouts, the four oscillators and filters,
    envelopes / LFOs / mod matrix, and the performance strip with the on-screen keyboard. */
class SynthTab : public juce::Component
{
public:
    explicit SynthTab (BassGodsProcessor& p);
    ~SynthTab() override;
    void resized() override;
    void paint (juce::Graphics&) override;

    ClubScene& getScene() { return *scene; }
    std::function<void (int osc)> onEditWavetable;
    void layoutScene (bool expanded, juce::Rectangle<int> fullArea);

private:
    BassGodsProcessor& proc;
    std::unique_ptr<ClubScene> scene;
    juce::OwnedArray<juce::Component> panels;
    juce::Component* macroLeft = nullptr;
    juce::Component* macroRight = nullptr;
    juce::Component* oscRow[5] {};
    juce::Component* filterPanel = nullptr;
    juce::Component* envPanel = nullptr;
    juce::Component* lfoPanel = nullptr;
    juce::Component* matrixPanel = nullptr;
    juce::Component* playPanel = nullptr;
    bool matrixCollapsed = false;
};
} // namespace bgdj
