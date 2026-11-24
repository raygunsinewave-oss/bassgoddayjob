#pragma once

#include "Visualizers.h"

namespace bgdj
{
/** VISUALIZER / METERING tab: a resizable grid of analysis panels. Each panel can be
    frozen or detached into its own window. */
class VisualizerTab : public juce::Component
{
public:
    VisualizerTab (BassGodsProcessor& p, AnalysisData& a);
    ~VisualizerTab() override;
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void detach (int index);
    BassGodsProcessor& proc;
    AnalysisData& analysis;
    std::vector<std::unique_ptr<VizPanel>> panels;
    juce::OwnedArray<juce::DocumentWindow> windows;
    juce::StretchableLayoutManager rowsLayout;
    juce::StretchableLayoutResizerBar rowBar1 { &rowsLayout, 1, false }, rowBar2 { &rowsLayout, 3, false };
};
} // namespace bgdj
