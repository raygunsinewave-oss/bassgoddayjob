#pragma once

#include "Widgets.h"
#include "../DSP/Wavetable.h"

namespace bgdj
{
/** Basic wavetable editor for one oscillator: draw a frame, edit harmonics (FFT), generate
    from a formula, import/export WAV. "Apply" installs the result as a user table. */
class WavetableEditor : public juce::Component
{
public:
    WavetableEditor (BassGodsProcessor& p, int osc);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    std::function<void()> onClose;

    /** Evaluates a formula over x (phase 0..1) and y (frame position 0..1). */
    static bool evaluateFormula (const juce::String& formula, int frames, std::vector<std::vector<float>>& out, juce::String& error);

private:
    enum class Mode { Draw, Harmonics, Formula };
    void setMode (Mode m);
    void loadFromOsc();
    void harmonicsFromFrame();
    void frameFromHarmonics();
    void apply();
    juce::Rectangle<int> editArea() const;
    void editAt (juce::Point<int> pos, bool first);

    BassGodsProcessor& proc;
    int osc;
    Mode mode = Mode::Draw;
    std::vector<std::vector<float>> frames;
    std::array<float, 64> harmonics {};
    int currentFrame = 0;
    juce::Point<int> lastDraw;

    juce::TextButton drawBtn { "Draw" }, harmBtn { "Harmonics (FFT)" }, formulaBtn { "Formula" },
                     importBtn { "Import WAV..." }, exportBtn { "Export WAV..." }, applyBtn { "Apply to Osc" },
                     closeBtn { "Close" }, addFrameBtn { "+ Frame" }, delFrameBtn { "- Frame" }, normBtn { "Normalise" },
                     genBtn { "Generate" };
    juce::Slider frameSlider, formulaFrames;
    juce::TextEditor formula;
    juce::Label info;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace bgdj
