#include "WavetableEditor.h"
#include "../PluginProcessor.h"

namespace bgdj
{
namespace
{
    struct FormulaScope : public juce::Expression::Scope
    {
        double x = 0, y = 0;
        juce::Expression getSymbolValue (const juce::String& s) const override
        {
            if (s == "x") return juce::Expression (x);
            if (s == "y") return juce::Expression (y);
            if (s == "pi") return juce::Expression (juce::MathConstants<double>::pi);
            if (s == "tau") return juce::Expression (juce::MathConstants<double>::twoPi);
            if (s == "e") return juce::Expression (juce::MathConstants<double>::euler);
            throw std::runtime_error (("Unknown symbol: " + s).toStdString());
        }
        double evaluateFunction (const juce::String& f, const double* p, int n) const override
        {
            auto a = [&] (int i) { return i < n ? p[i] : 0.0; };
            auto frac = [] (double v) { return v - std::floor (v); };
            if (f == "sin") return std::sin (a (0));
            if (f == "cos") return std::cos (a (0));
            if (f == "tan") return std::tan (a (0));
            if (f == "tanh") return std::tanh (a (0));
            if (f == "abs") return std::abs (a (0));
            if (f == "sqrt") return std::sqrt (std::abs (a (0)));
            if (f == "exp") return std::exp (a (0));
            if (f == "log") return std::log (std::max (1.0e-12, a (0)));
            if (f == "pow") return std::pow (a (0), a (1));
            if (f == "floor") return std::floor (a (0));
            if (f == "frac") return frac (a (0));
            if (f == "min") return std::min (a (0), a (1));
            if (f == "max") return std::max (a (0), a (1));
            if (f == "clip") return juce::jlimit (-1.0, 1.0, a (0));
            if (f == "saw") return 2.0 * frac (a (0)) - 1.0;
            if (f == "square") return frac (a (0)) < (n > 1 ? a (1) : 0.5) ? 1.0 : -1.0;
            if (f == "tri") { const double v = frac (a (0)); return v < 0.5 ? 4 * v - 1 : 3 - 4 * v; }
            if (f == "sine") return std::sin (juce::MathConstants<double>::twoPi * a (0));
            if (f == "rand") { const auto h = (uint32_t) (a (0) * 1000003.0) * 2654435761u; return (double) (h >> 8) / 8388608.0 - 1.0; }
            throw std::runtime_error (("Unknown function: " + f).toStdString());
        }
    };
}

bool WavetableEditor::evaluateFormula (const juce::String& text, int numFrames, std::vector<std::vector<float>>& out, juce::String& error)
{
    juce::String parseError;
    juce::String t = text;
    juce::Expression expr (t, parseError);
    if (parseError.isNotEmpty())
    {
        error = parseError;
        return false;
    }
    numFrames = juce::jlimit (1, Wavetable::kMaxFrames, numFrames);
    out.assign ((size_t) numFrames, std::vector<float> ((size_t) Wavetable::kSize));
    FormulaScope scope;
    try
    {
        for (int f = 0; f < numFrames; ++f)
        {
            scope.y = numFrames > 1 ? (double) f / (numFrames - 1) : 0.0;
            for (int i = 0; i < Wavetable::kSize; ++i)
            {
                scope.x = (double) i / Wavetable::kSize;
                const double v = expr.evaluate (scope);
                out[(size_t) f][(size_t) i] = std::isfinite (v) ? (float) juce::jlimit (-4.0, 4.0, v) : 0.0f;
            }
        }
    }
    catch (const std::exception& e)
    {
        error = e.what();
        return false;
    }
    return true;
}

WavetableEditor::WavetableEditor (BassGodsProcessor& p, int o) : proc (p), osc (o)
{
    for (auto* b : { &drawBtn, &harmBtn, &formulaBtn, &importBtn, &exportBtn, &applyBtn, &closeBtn, &addFrameBtn, &delFrameBtn, &normBtn, &genBtn })
        addAndMakeVisible (b);
    drawBtn.onClick = [this] { setMode (Mode::Draw); };
    harmBtn.onClick = [this] { setMode (Mode::Harmonics); };
    formulaBtn.onClick = [this] { setMode (Mode::Formula); };
    closeBtn.onClick = [this] { if (onClose) onClose(); };
    applyBtn.onClick = [this] { apply(); };
    normBtn.onClick = [this]
    {
        float peak = 0.0f;
        for (auto& f : frames) for (float v : f) peak = std::max (peak, std::abs (v));
        if (peak > 1.0e-6f) for (auto& f : frames) for (auto& v : f) v /= peak;
        repaint();
    };
    addFrameBtn.onClick = [this]
    {
        if ((int) frames.size() < Wavetable::kMaxFrames)
        {
            frames.insert (frames.begin() + currentFrame + 1, frames[(size_t) currentFrame]);
            frameSlider.setRange (0, (double) frames.size() - 1, 1);
            frameSlider.setValue (currentFrame + 1);
        }
    };
    delFrameBtn.onClick = [this]
    {
        if (frames.size() > 1)
        {
            frames.erase (frames.begin() + currentFrame);
            currentFrame = std::min (currentFrame, (int) frames.size() - 1);
            frameSlider.setRange (0, std::max (1.0, (double) frames.size() - 1), 1);
            frameSlider.setValue (currentFrame);
            repaint();
        }
    };
    importBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Import wavetable", proc.settings->wavetableFolders().getFirst(), "*.wav;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            juce::String err;
            if (auto wt = Wavetable::fromAudioFile (f, err))
            {
                frames = wt->sourceFrames;
                currentFrame = 0;
                frameSlider.setRange (0, std::max (1.0, (double) frames.size() - 1), 1);
                frameSlider.setValue (0);
                info.setText ("Imported " + f.getFileName() + " (" + juce::String (frames.size()) + " frames)", juce::dontSendNotification);
                repaint();
            }
            else
                info.setText (err, juce::dontSendNotification);
        });
    };
    exportBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Export wavetable", proc.settings->wavetableFolders().getFirst().getChildFile ("My Table.wav"), "*.wav");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f == juce::File()) return;
            auto wt = Wavetable::fromFrames (f.getFileNameWithoutExtension(), frames, false);
            info.setText (wt->exportWav (f.withFileExtension ("wav")) ? "Exported " + f.getFileName() : juce::String ("Export failed"), juce::dontSendNotification);
        });
    };
    genBtn.onClick = [this]
    {
        std::vector<std::vector<float>> out;
        juce::String err;
        if (evaluateFormula (formula.getText(), (int) formulaFrames.getValue(), out, err))
        {
            frames = out;
            currentFrame = 0;
            frameSlider.setRange (0, std::max (1.0, (double) frames.size() - 1), 1);
            frameSlider.setValue (0);
            info.setText ("Generated " + juce::String (frames.size()) + " frames", juce::dontSendNotification);
            harmonicsFromFrame();
        }
        else
            info.setText ("Formula error: " + err, juce::dontSendNotification);
        repaint();
    };

    addAndMakeVisible (frameSlider);
    frameSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    frameSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    frameSlider.onValueChange = [this] { currentFrame = juce::jlimit (0, (int) frames.size() - 1, (int) frameSlider.getValue()); harmonicsFromFrame(); repaint(); };
    addAndMakeVisible (formulaFrames);
    formulaFrames.setSliderStyle (juce::Slider::LinearHorizontal);
    formulaFrames.setRange (1, 256, 1);
    formulaFrames.setValue (32);
    formulaFrames.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    addAndMakeVisible (formula);
    formula.setText ("sin(tau*x + y*4*sin(tau*2*x))");
    formula.setTooltip ("Variables: x (phase 0..1), y (frame 0..1), pi, tau. Functions: sin cos tan tanh abs sqrt exp log pow floor frac min max clip saw(x) square(x,pw) tri(x) sine(x) rand(x)");
    addAndMakeVisible (info);
    info.setFont (theme::font (12.0f));
    loadFromOsc();
    setMode (Mode::Draw);
}

void WavetableEditor::loadFromOsc()
{
    if (auto wt = proc.getTable (osc))
        frames = wt->sourceFrames;
    if (frames.empty())
        frames.assign (1, std::vector<float> ((size_t) Wavetable::kSize, 0.0f));
    frameSlider.setRange (0, std::max (1.0, (double) frames.size() - 1), 1);
    frameSlider.setValue (0, juce::dontSendNotification);
    currentFrame = 0;
    harmonicsFromFrame();
    info.setText ("Editing Osc " + juce::String::charToString ((juce::juce_wchar) ('A' + osc)) + ": " + proc.getTableName (osc), juce::dontSendNotification);
}

void WavetableEditor::setMode (Mode m)
{
    mode = m;
    drawBtn.setToggleState (m == Mode::Draw, juce::dontSendNotification);
    harmBtn.setToggleState (m == Mode::Harmonics, juce::dontSendNotification);
    formulaBtn.setToggleState (m == Mode::Formula, juce::dontSendNotification);
    formula.setVisible (m == Mode::Formula);
    formulaFrames.setVisible (m == Mode::Formula);
    genBtn.setVisible (m == Mode::Formula);
    if (m == Mode::Harmonics) harmonicsFromFrame();
    resized();
    repaint();
}

void WavetableEditor::harmonicsFromFrame()
{
    if (frames.empty()) return;
    const auto& f = frames[(size_t) currentFrame];
    for (int h = 1; h <= 64; ++h)
    {
        double re = 0, im = 0;
        for (int i = 0; i < Wavetable::kSize; i += 2)
        {
            const double ph = juce::MathConstants<double>::twoPi * h * i / Wavetable::kSize;
            re += f[(size_t) i] * std::cos (ph);
            im += f[(size_t) i] * std::sin (ph);
        }
        harmonics[(size_t) h - 1] = (float) (std::sqrt (re * re + im * im) * 4.0 / Wavetable::kSize);
    }
}

void WavetableEditor::frameFromHarmonics()
{
    auto& f = frames[(size_t) currentFrame];
    for (int i = 0; i < Wavetable::kSize; ++i)
    {
        double s = 0;
        for (int h = 1; h <= 64; ++h)
            if (harmonics[(size_t) h - 1] > 0.0f)
                s += harmonics[(size_t) h - 1] * std::sin (juce::MathConstants<double>::twoPi * h * i / Wavetable::kSize);
        f[(size_t) i] = (float) s;
    }
}

void WavetableEditor::apply()
{
    auto wt = Wavetable::fromFrames ("Edited " + proc.getTableName (osc).upToFirstOccurrenceOf (" (user)", false, false), frames, true);
    proc.setUserTable (osc, wt);
    info.setText ("Applied to Osc " + juce::String::charToString ((juce::juce_wchar) ('A' + osc)), juce::dontSendNotification);
}

juce::Rectangle<int> WavetableEditor::editArea() const
{
    auto r = getLocalBounds().reduced (16);
    r.removeFromTop (70);
    r.removeFromBottom (mode == Mode::Formula ? 110 : 70);
    return r;
}

void WavetableEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    auto top = r.removeFromTop (30);
    layoutRow (top, { &drawBtn, &harmBtn, &formulaBtn, &importBtn, &exportBtn, &normBtn, &applyBtn, &closeBtn }, 6);
    r.removeFromTop (8);
    auto fr = r.removeFromTop (26);
    addFrameBtn.setBounds (fr.removeFromRight (70));
    fr.removeFromRight (6);
    delFrameBtn.setBounds (fr.removeFromRight (70));
    fr.removeFromRight (6);
    frameSlider.setBounds (fr);
    auto bottom = r.removeFromBottom (mode == Mode::Formula ? 104 : 60);
    info.setBounds (bottom.removeFromBottom (24));
    if (mode == Mode::Formula)
    {
        bottom.removeFromTop (6);
        formula.setBounds (bottom.removeFromTop (30));
        bottom.removeFromTop (6);
        auto row = bottom.removeFromTop (28);
        genBtn.setBounds (row.removeFromRight (110));
        formulaFrames.setBounds (row.reduced (0, 2));
    }
}

void WavetableEditor::paint (juce::Graphics& g)
{
    const auto& p = theme::get();
    g.fillAll (p.bg.withAlpha (0.97f));
    drawPanel (g, getLocalBounds().toFloat().reduced (4.0f), "WAVETABLE EDITOR", p.cyan);
    auto r = editArea().toFloat();
    g.setColour (p.panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (p.outline);
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());

    if (frames.empty()) return;

    // ghost of neighbouring frames (3D-ish stack)
    for (int k = std::max (0, currentFrame - 6); k <= std::min ((int) frames.size() - 1, currentFrame + 6); k += 2)
    {
        if (k == currentFrame) continue;
        juce::Path gp;
        const auto& f = frames[(size_t) k];
        const float off = (float) (k - currentFrame) * 4.0f;
        for (int i = 0; i < Wavetable::kSize; i += 8)
        {
            const float x = r.getX() + r.getWidth() * (float) i / Wavetable::kSize + off;
            const float y = r.getCentreY() - f[(size_t) i] * r.getHeight() * 0.45f - off;
            if (i == 0) gp.startNewSubPath (x, y); else gp.lineTo (x, y);
        }
        g.setColour (p.magenta.withAlpha (0.15f));
        g.strokePath (gp, juce::PathStrokeType (1.0f));
    }

    if (mode == Mode::Harmonics)
    {
        const float bw = r.getWidth() / 64.0f;
        for (int h = 0; h < 64; ++h)
        {
            const float v = juce::jlimit (0.0f, 1.0f, harmonics[(size_t) h]);
            g.setColour (h == 0 ? p.yellow : p.cyan);
            g.fillRect (r.getX() + h * bw + 1.0f, r.getBottom() - v * r.getHeight(), bw - 2.0f, v * r.getHeight());
        }
        g.setColour (p.textDim);
        g.setFont (theme::font (11.0f));
        g.drawText ("Drag bars to set harmonic levels 1-64", r.reduced (6.0f), juce::Justification::topLeft);
        return;
    }

    juce::Path path;
    const auto& f = frames[(size_t) currentFrame];
    for (int i = 0; i < Wavetable::kSize; i += 2)
    {
        const float x = r.getX() + r.getWidth() * (float) i / Wavetable::kSize;
        const float y = r.getCentreY() - juce::jlimit (-1.1f, 1.1f, f[(size_t) i]) * r.getHeight() * 0.45f;
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    g.setColour (p.cyan);
    g.strokePath (path, juce::PathStrokeType (2.0f));
}

void WavetableEditor::editAt (juce::Point<int> pos, bool first)
{
    auto r = editArea().toFloat();
    if (! r.contains (pos.toFloat()) && first) return;
    if (mode == Mode::Harmonics)
    {
        const int h = juce::jlimit (0, 63, (int) ((pos.x - r.getX()) / r.getWidth() * 64.0f));
        harmonics[(size_t) h] = juce::jlimit (0.0f, 1.0f, (r.getBottom() - (float) pos.y) / r.getHeight());
        frameFromHarmonics();
    }
    else if (mode == Mode::Draw)
    {
        auto& f = frames[(size_t) currentFrame];
        auto toIdx = [&] (juce::Point<int> pt) { return juce::jlimit (0, Wavetable::kSize - 1, (int) ((pt.x - r.getX()) / r.getWidth() * Wavetable::kSize)); };
        auto toVal = [&] (juce::Point<int> pt) { return juce::jlimit (-1.0f, 1.0f, (r.getCentreY() - (float) pt.y) / (r.getHeight() * 0.45f)); };
        const auto from = first ? pos : lastDraw;
        int i0 = toIdx (from), i1 = toIdx (pos);
        float v0 = toVal (from), v1 = toVal (pos);
        if (i0 > i1) { std::swap (i0, i1); std::swap (v0, v1); }
        for (int i = i0; i <= i1; ++i)
            f[(size_t) i] = i1 == i0 ? v1 : v0 + (v1 - v0) * (float) (i - i0) / (float) (i1 - i0);
        lastDraw = pos;
    }
    repaint();
}

void WavetableEditor::mouseDown (const juce::MouseEvent& e) { editAt (e.getPosition(), true); }
void WavetableEditor::mouseDrag (const juce::MouseEvent& e) { editAt (e.getPosition(), false); }
} // namespace bgdj
