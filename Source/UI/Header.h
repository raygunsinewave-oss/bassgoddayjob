#pragma once

#include "Widgets.h"

namespace bgdj
{
class PresetBrowser;

/** Top bar: logo | < preset name > star | Save | A/B | CPU | Master. */
class Header : public juce::Component, private juce::Timer, private juce::ChangeListener
{
public:
    Header (BassGodsProcessor& p);
    ~Header() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

    std::function<void()> onBrowse;
    std::function<std::vector<int>()> getBrowseList;   // filtered list for prev/next

    void showSaveDialog();

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    juce::Rectangle<int> nameArea() const;

    BassGodsProcessor& proc;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, favBtn { "*" }, saveBtn { "SAVE" }, aBtn { "A" }, bBtn { "B" }, copyBtn { "A>B" }, browseBtn { "BROWSE" };
    Knob master;
    juce::String cpuText;
    int lastPresetVersion = -1;
};
} // namespace bgdj
