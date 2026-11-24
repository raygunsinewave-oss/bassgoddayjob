#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bgdj
{
struct Palette
{
    juce::Colour bg, panel, panelHi, outline, text, textDim, magenta, pink, cyan, teal, yellow, warn, good, knobBody, track;
};

namespace theme
{
    const juce::StringArray& names();
    void set (const juce::String& name);
    const Palette& get();
    juce::Colour modSourceColour (int source);
    juce::Font font (float height, bool bold = false);
    juce::Font mono (float height);
}

/** Neon-club look: chunky rotary knobs with glow, sticker-worn panels. */
class BgdjLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BgdjLookAndFeel();
    void refreshColours();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawTabButton (juce::TabBarButton&, juce::Graphics&, bool over, bool down) override;
    int getTabButtonBestWidth (juce::TabBarButton&, int depth) override;
    void drawTabAreaBehindFrontButton (juce::TabbedButtonBar&, juce::Graphics&, int w, int h) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip, const juce::Rectangle<float>& body) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override;
};

/** Draws a rounded "sticker-worn" panel with a title. */
void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour accent);
} // namespace bgdj
