#pragma once

#include "Widgets.h"
#include "../Engine/Shapes.h"

namespace bgdj
{
/** Breakpoint editor: drag points, double-click to add/remove, Alt-drag a segment to bend it,
    Shift to snap to the grid. Used for LFO shapes and the ducker. */
class CurveEditor : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    CurveEditor();
    std::function<CurvePoints()> getPoints;
    std::function<void (const CurvePoints&)> setPoints;
    std::function<float()> getPlayhead;      // 0..1 or <0 for none
    std::function<void (juce::PopupMenu&)> addMenuItems;
    std::function<void (int)> onMenuResult;
    juce::Colour colour;
    int gridX = 8, gridY = 4;
    bool invertDisplay = false;             // ducker shows gain (1 = top)

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override { if (getPlayhead) repaint(); }
    juce::Point<float> toScreen (const CurvePoint& p) const;
    CurvePoint fromScreen (juce::Point<float> s, bool snap) const;
    int hitPoint (juce::Point<float> s) const;
    juce::Rectangle<float> plotArea() const { return getLocalBounds().toFloat().reduced (4.0f); }

    CurvePoints working;
    int dragIndex = -1, bendIndex = -1;
    float bendStart = 0.0f;
};

/** ADSR display driven by parameters, with a live level dot. */
class EnvelopeView : public juce::Component, private juce::Timer
{
public:
    EnvelopeView (BassGodsProcessor& p, int envIndex);
    void setEnvIndex (int e) { env = e; repaint(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override { repaint(); }
    BassGodsProcessor& proc;
    int env;
};

/** Velocity curve editor: drag up/down to change the vel_curve parameter. */
class VelocityCurveView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit VelocityCurveView (BassGodsProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
private:
    BassGodsProcessor& proc;
    float startValue = 0.0f;
};
} // namespace bgdj
