#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "Theme.h"
#include "../Common/Parameters.h"

namespace bgdj
{
class BassGodsProcessor;

/** Slider that lets the owning Knob intercept Alt-drag (mod depth) and right-click. */
class KnobSlider : public juce::Slider
{
public:
    std::function<bool (const juce::MouseEvent&)> onMouseDownIntercept;
    std::function<void (const juce::MouseEvent&)> onModDrag, onModDragEnd;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
private:
    bool intercepting = false;
};

/** Parameter knob with label, hover value popup, double-click reset, right-click menu
    (MIDI learn / modulation), and a drop target for modulation sources. */
class Knob : public juce::Component, public juce::DragAndDropTarget, private juce::ChangeListener, private juce::Timer
{
public:
    Knob (BassGodsProcessor& p, const juce::String& paramID, const juce::String& label, juce::Colour accent = {});
    ~Knob() override;

    void resized() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override { dragHover = true; repaint(); }
    void itemDragExit (const SourceDetails&) override  { dragHover = false; repaint(); }
    void itemDropped (const SourceDetails&) override;

    juce::Slider& getSlider() { return slider; }
    const juce::String& getParamID() const { return paramID; }
    void setLabelText (const juce::String& t) { labelText = t; repaint(); }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void applyBehaviour();
    void showMenu();
    juce::Rectangle<float> knobArea() const;

    BassGodsProcessor& proc;
    juce::String paramID, labelText;
    KnobSlider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    int dest = -1, modSlotDragging = -1, lastMatrixVersion = -1, lastMidiVersion = -1;
    float modDragStart = 0.0f;
    bool dragHover = false;
    juce::String ccText;
};

/** Small pill that can be dragged onto knobs to create a modulation routing. */
class ModSourceChip : public juce::Component, public juce::SettableTooltipClient
{
public:
    ModSourceChip (int source, const juce::String& text);
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    int source;
    juce::String text;
};

class ParamToggle : public juce::ToggleButton
{
public:
    ParamToggle (BassGodsProcessor& p, const juce::String& paramID, const juce::String& text);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

class ParamChoice : public juce::ComboBox
{
public:
    ParamChoice (BassGodsProcessor& p, const juce::String& paramID);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

/** Lays out a row of components with equal width. */
void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> comps, int gap = 4);
} // namespace bgdj
