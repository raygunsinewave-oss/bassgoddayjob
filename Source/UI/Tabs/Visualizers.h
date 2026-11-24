#pragma once

#include "../Widgets.h"
#include "../../Analysis/AnalysisData.h"

namespace bgdj
{
/** Base for analysis panels: header with Freeze and Detach, timer-driven refresh. */
class VizPanel : public juce::Component, private juce::Timer
{
public:
    VizPanel (const juce::String& title, BassGodsProcessor& p, AnalysisData& a);
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void()> onDetach;
    bool isFrozen() const noexcept { return frozen; }
    const juce::String& getTitle() const noexcept { return title; }

protected:
    virtual void updateData() = 0;
    virtual void draw (juce::Graphics& g, juce::Rectangle<float> area) = 0;
    virtual void layoutControls (juce::Rectangle<int>) {}
    juce::Rectangle<int> controlArea;
    BassGodsProcessor& proc;
    AnalysisData& analysis;

private:
    void timerCallback() override;
    juce::String title;
    juce::TextButton freezeBtn { "Freeze" }, detachBtn { "Detach" };
    bool frozen = false;
};

namespace viz
{
    const juce::StringArray& names();
    std::unique_ptr<VizPanel> create (int index, BassGodsProcessor& p, AnalysisData& a);
}
} // namespace bgdj
