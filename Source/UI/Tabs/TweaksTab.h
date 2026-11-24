#pragma once

#include "../Widgets.h"
#include "../CurveEditors.h"

namespace bgdj
{
/** TWEAKS tab: advanced engine options (oversampling, interpolation, unison curves, voice
    stealing, tuning / microtuning, velocity, MPE, filter options, denormals, seeds...). */
class TweaksTab : public juce::Component, private juce::Timer
{
public:
    explicit TweaksTab (BassGodsProcessor& p);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    struct Group { juce::String title; juce::Rectangle<int> bounds; std::vector<juce::Component*> items; std::vector<juce::String> labels; };
    juce::Component* addItem (Group& g, juce::Component* c, const juce::String& label);

    BassGodsProcessor& proc;
    juce::OwnedArray<juce::Component> owned;
    std::vector<Group> groups;
    std::unique_ptr<VelocityCurveView> velView;
    juce::TextButton loadTuning { "Load .scl / .tun..." }, resetTuning { "12-TET" };
    juce::Label tuningName;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace bgdj
