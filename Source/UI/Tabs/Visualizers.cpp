#include "Visualizers.h"
#include "../../PluginProcessor.h"

namespace bgdj
{
VizPanel::VizPanel (const juce::String& t, BassGodsProcessor& p, AnalysisData& a) : proc (p), analysis (a), title (t)
{
    addAndMakeVisible (freezeBtn);
    addAndMakeVisible (detachBtn);
    freezeBtn.setClickingTogglesState (true);
    freezeBtn.onClick = [this] { frozen = freezeBtn.getToggleState(); };
    detachBtn.onClick = [this] { if (onDetach) onDetach(); };
    detachBtn.setTooltip ("Open this panel in its own resizable window");
    startTimerHz (30);
}

void VizPanel::timerCallback()
{
    if (! isShowing()) return;
    if (! frozen) updateData();
    repaint();
}

void VizPanel::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto head = r.removeFromTop (20);
    detachBtn.setBounds (head.removeFromRight (54));
    head.removeFromRight (4);
    freezeBtn.setBounds (head.removeFromRight (54));
    head.removeFromLeft ((int) juce::GlyphArrangement::getStringWidthInt (theme::font (12.0f, true), title) + 30);
    controlArea = head;
    layoutControls (head);
}

void VizPanel::paint (juce::Graphics& g)
{
    drawPanel (g, getLocalBounds().toFloat(), title, theme::get().cyan);
    auto area = getLocalBounds().reduced (6).withTrimmedTop (24).toFloat();
    g.setColour (theme::get().bg);
    g.fillRoundedRectangle (area, 4.0f);
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (area.toNearestInt());
    draw (g, area.reduced (3.0f));
    if (frozen)
    {
        g.setColour (theme::get().yellow);
        g.setFont (theme::font (10.0f, true));
        g.drawText ("FROZEN", area.reduced (6.0f), juce::Justification::topRight);
    }
}

namespace
{
const auto& P() { return theme::get(); }

inline float xForHz (float hz, juce::Rectangle<float> a) { return a.getX() + a.getWidth() * std::log (std::max (hz, 20.0f) / 20.0f) / std::log (1000.0f); }

// ============================================================================
struct SpectrumViz : public VizPanel
{
    SpectrumViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("SPECTRUM", p, a)
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &size, &slope, &peak, &prePost }) addAndMakeVisible (c);
        size.addItemList ({ "1024", "2048", "4096", "8192", "16384", "32768" }, 1);
        size.setSelectedItemIndex (3);
        slope.addItemList ({ "0 dB/oct", "3 dB/oct", "4.5 dB/oct" }, 1);
        slope.setSelectedItemIndex (2);
        peak.setButtonText ("Peak hold");
        peak.setToggleState (true, juce::dontSendNotification);
        prePost.setButtonText ("Pre-FX overlay");
        prePost.setToggleState (true, juce::dontSendNotification);
    }
    void layoutControls (juce::Rectangle<int> r) override
    {
        size.setBounds (r.removeFromLeft (70));
        r.removeFromLeft (3);
        slope.setBounds (r.removeFromLeft (86));
        r.removeFromLeft (3);
        peak.setBounds (r.removeFromLeft (84));
        prePost.setBounds (r.removeFromLeft (110));
    }
    void updateData() override
    {
        const int order = 10 + size.getSelectedItemIndex();
        const float sl = slope.getSelectedItemIndex() == 0 ? 0.0f : (slope.getSelectedItemIndex() == 1 ? 3.0f : 4.5f);
        analysis.computeSpectrum (0, order, sl, post);
        if (prePost.getToggleState()) analysis.computeSpectrum (1, order, sl, pre);
        if (peaks.size() != post.size() || ! peak.getToggleState()) peaks = post;
        for (size_t i = 0; i < post.size(); ++i) peaks[i] = std::max (post[i], peaks[i] - 0.3f);
        // peak note readout
        float best = -200.0f; size_t bi = 0;
        for (size_t i = 1; i < post.size(); ++i) if (post[i] > best) { best = post[i]; bi = i; }
        peakHz = best > -80.0f ? (float) bi * (float) analysis.getSampleRate() / (float) (post.size() * 2) : 0.0f;
    }
    void drawCurve (juce::Graphics& g, juce::Rectangle<float> a, const std::vector<float>& s, juce::Colour c, bool fill)
    {
        if (s.empty()) return;
        const double sr = analysis.getSampleRate();
        juce::Path p;
        bool started = false;
        for (float x = a.getX(); x < a.getRight(); x += 1.5f)
        {
            const float hz = 20.0f * std::pow (1000.0f, (x - a.getX()) / a.getWidth());
            const float binF = hz / (float) sr * (float) s.size() * 2.0f;
            const int b = juce::jlimit (0, (int) s.size() - 2, (int) binF);
            const float db = juce::jmax (s[(size_t) b], s[(size_t) b + 1]);
            const float y = juce::jmap (juce::jlimit (-100.0f, 6.0f, db), -100.0f, 6.0f, a.getBottom(), a.getY());
            if (! started) { p.startNewSubPath (x, y); started = true; } else p.lineTo (x, y);
        }
        if (fill)
        {
            juce::Path f (p);
            f.lineTo (a.getRight(), a.getBottom()); f.lineTo (a.getX(), a.getBottom()); f.closeSubPath();
            g.setColour (c.withAlpha (0.18f));
            g.fillPath (f);
        }
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (1.4f));
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        g.setColour (P().outline.withAlpha (0.4f));
        g.setFont (theme::font (9.0f));
        for (float hz : { 30.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        {
            const float x = xForHz (hz, a);
            g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
            g.drawText (hz >= 1000 ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz), juce::Rectangle<float> (x + 2, a.getBottom() - 12, 30, 12), juce::Justification::left);
        }
        if (prePost.getToggleState()) drawCurve (g, a, pre, P().teal.withAlpha (0.7f), false);
        drawCurve (g, a, post, P().magenta, true);
        if (peak.getToggleState()) drawCurve (g, a, peaks, P().yellow.withAlpha (0.5f), false);
        float cents;
        const auto note = AnalysisData::noteName (peakHz, cents);
        g.setColour (P().text);
        g.setFont (theme::mono (12.0f));
        g.drawText (peakHz > 0.0f ? "Peak " + juce::String (peakHz, 1) + " Hz  " + note + " " + (cents >= 0 ? "+" : "") + juce::String (juce::roundToInt (cents)) + "c"
                                  : juce::String ("No signal"), a.reduced (4.0f), juce::Justification::topLeft);
    }
    juce::ComboBox size, slope;
    juce::ToggleButton peak, prePost;
    std::vector<float> post, pre, peaks;
    float peakHz = 0.0f;
};

// ============================================================================
struct SpectrogramViz : public VizPanel
{
    SpectrogramViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("SPECTROGRAM", p, a) {}
    void updateData() override
    {
        analysis.computeSpectrum (0, 11, 3.0f, spec);
        if (img.isNull() || img.getHeight() != 256) img = juce::Image (juce::Image::RGB, 512, 256, true);
        img.moveImageSection (0, 0, 1, 0, img.getWidth() - 1, img.getHeight());
        const double sr = analysis.getSampleRate();
        for (int y = 0; y < img.getHeight(); ++y)
        {
            const float hz = 20.0f * std::pow (1000.0f, 1.0f - (float) y / (float) img.getHeight());
            const int b = juce::jlimit (0, (int) spec.size() - 1, (int) (hz / sr * (double) spec.size() * 2.0));
            const float v = juce::jlimit (0.0f, 1.0f, (spec[(size_t) b] + 100.0f) / 100.0f);
            const auto c = v < 0.5f ? juce::Colours::black.interpolatedWith (P().magenta, v * 2.0f) : P().magenta.interpolatedWith (P().yellow, (v - 0.5f) * 2.0f);
            img.setPixelAt (img.getWidth() - 1, y, c);
        }
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        if (! img.isNull()) g.drawImage (img, a, juce::RectanglePlacement::stretchToFit);
    }
    std::vector<float> spec;
    juce::Image img;
};

// ============================================================================
struct ScopeViz : public VizPanel
{
    ScopeViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("OSCILLOSCOPE", p, a)
    {
        addAndMakeVisible (mode);
        mode.addItemList ({ "Single cycle", "1 beat", "1 bar", "1/4 beat" }, 1);
        mode.setSelectedItemIndex (0);
    }
    void layoutControls (juce::Rectangle<int> r) override { mode.setBounds (r.removeFromLeft (110)); }
    void updateData() override
    {
        const double sr = analysis.getSampleRate();
        const double beat = 60.0 / std::max (40.0, proc.uiBpm.load()) * sr;
        int n = 2048;
        switch (mode.getSelectedItemIndex())
        {
            case 1: n = (int) beat; break;
            case 2: n = (int) (beat * 4.0); break;
            case 3: n = (int) (beat * 0.25); break;
            default:
            {
                float conf;
                const float f0 = analysis.detectPitch (conf);
                n = (conf > 0.4f && f0 > 0.0f) ? (int) (sr / f0 * 2.0) : 2048;
            }
        }
        n = juce::jlimit (64, AnalysisData::kHistory / 2, n);
        std::vector<float> l ((size_t) n * 2), r ((size_t) n * 2);
        analysis.getLatest (0, l.data(), r.data(), n * 2);
        int start = 0;
        if (mode.getSelectedItemIndex() == 0)   // trigger on rising zero crossing
        {
            for (int i = 1; i < n; ++i)
                if (l[(size_t) i - 1] <= 0.0f && l[(size_t) i] > 0.0f) { start = i; break; }
        }
        else if (proc.uiPlaying.load())          // tempo-synced: align to the beat grid
        {
            const double ppq = proc.uiPpq.load();
            const double unit = mode.getSelectedItemIndex() == 2 ? 4.0 : (mode.getSelectedItemIndex() == 3 ? 0.25 : 1.0);
            const double frac = std::fmod (ppq, unit) / unit;
            start = juce::jlimit (0, n, n - (int) (frac * n));
        }
        data.assign (l.begin() + start, l.begin() + start + n);
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        g.setColour (P().outline.withAlpha (0.4f));
        g.drawHorizontalLine ((int) a.getCentreY(), a.getX(), a.getRight());
        if (data.empty()) return;
        juce::Path p;
        for (size_t i = 0; i < data.size(); ++i)
        {
            const float x = a.getX() + a.getWidth() * (float) i / (float) data.size();
            const float y = a.getCentreY() - juce::jlimit (-1.2f, 1.2f, data[i]) * a.getHeight() * 0.45f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (P().cyan);
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }
    juce::ComboBox mode;
    std::vector<float> data;
};

// ============================================================================
struct GonioViz : public VizPanel
{
    GonioViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("GONIOMETER + CORRELATION", p, a) {}
    void updateData() override
    {
        l.resize (2048); r.resize (2048);
        analysis.getLatest (0, l.data(), r.data(), 2048);
        corr = corr * 0.8f + 0.2f * analysis.correlation (4096);
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        auto meter = a.removeFromBottom (22.0f);
        const float s = std::min (a.getWidth(), a.getHeight()) * 0.5f;
        const auto c = a.getCentre();
        g.setColour (P().outline.withAlpha (0.5f));
        g.drawLine (c.x - s, c.y, c.x + s, c.y);
        g.drawLine (c.x, c.y - s, c.x, c.y + s);
        g.drawLine (c.x - s * 0.7f, c.y - s * 0.7f, c.x + s * 0.7f, c.y + s * 0.7f);
        g.drawLine (c.x - s * 0.7f, c.y + s * 0.7f, c.x + s * 0.7f, c.y - s * 0.7f);
        g.setFont (theme::font (9.0f));
        g.drawText ("M", juce::Rectangle<float> (c.x - 6, c.y - s, 12, 10), juce::Justification::centred);
        g.setColour (P().teal.withAlpha (0.6f));
        for (size_t i = 0; i < l.size(); i += 2)
        {
            const float m = (l[i] + r[i]) * 0.7071f, sd = (l[i] - r[i]) * 0.7071f;
            g.fillRect (c.x + juce::jlimit (-1.0f, 1.0f, sd) * s, c.y - juce::jlimit (-1.0f, 1.0f, m) * s, 1.5f, 1.5f);
        }
        // correlation meter
        auto bar = meter.reduced (20.0f, 6.0f);
        g.setColour (P().track);
        g.fillRoundedRectangle (bar, 3.0f);
        const float x = bar.getCentreX() + corr * bar.getWidth() * 0.5f;
        g.setColour (corr < 0.0f ? P().warn : (corr < 0.5f ? P().yellow : P().good));
        g.fillRect (juce::Rectangle<float> (std::min (x, bar.getCentreX()), bar.getY(), std::abs (x - bar.getCentreX()), bar.getHeight()));
        g.setColour (P().text);
        g.setFont (theme::font (9.0f, true));
        g.drawText ("-1", meter.removeFromLeft (18.0f), juce::Justification::centred);
        g.drawText ("+1", meter.removeFromRight (18.0f), juce::Justification::centred);
        g.drawText (juce::String (corr, 2), bar, juce::Justification::centred);
    }
    std::vector<float> l, r;
    float corr = 1.0f;
};

// ============================================================================
struct LoudnessViz : public VizPanel
{
    LoudnessViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("LOUDNESS", p, a)
    {
        addAndMakeVisible (reset);
        reset.onClick = [this] { proc.loudness.requestReset(); };
    }
    void layoutControls (juce::Rectangle<int> r) override { reset.setBounds (r.removeFromLeft (60)); }
    void updateData() override
    {
        auto& L = proc.loudness;
        vals = { L.momentary.load(), L.shortTerm.load(), L.integrated.load(), L.lra.load(), L.truePeakMaxDb.load(),
                 std::max (L.rmsDbL.load(), L.rmsDbR.load()), L.crestDb.load() };
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        const char* names[] = { "Momentary", "Short-term", "Integrated", "LRA", "True Peak (max)", "RMS", "Crest Factor" };
        const char* units[] = { "LUFS", "LUFS", "LUFS", "LU", "dBTP", "dBFS", "dB" };
        const float rowH = a.getHeight() / 7.0f;
        for (int i = 0; i < 7 && i < (int) vals.size(); ++i)
        {
            auto row = a.removeFromTop (rowH);
            g.setColour (P().textDim);
            g.setFont (theme::font (std::min (13.0f, rowH * 0.5f)));
            g.drawText (names[i], row.removeFromLeft (row.getWidth() * 0.5f), juce::Justification::centredLeft);
            const float v = vals[(size_t) i];
            g.setColour (i == 4 && v > -1.0f ? P().warn : (i == 2 ? P().cyan : P().text));
            g.setFont (theme::mono (std::min (16.0f, rowH * 0.6f)));
            g.drawText ((v <= -99.0f ? juce::String ("-inf") : juce::String (v, 1)) + " " + units[i], row, juce::Justification::centredRight);
        }
    }
    juce::TextButton reset { "Reset" };
    std::vector<float> vals;
};

// ============================================================================
struct LevelViz : public VizPanel
{
    LevelViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("LEVELS", p, a)
    {
        addAndMakeVisible (reset);
        reset.onClick = [this] { maxL = maxR = -100.0f; clipL = clipR = false; };
    }
    void layoutControls (juce::Rectangle<int> r) override { reset.setBounds (r.removeFromLeft (70)); }
    void updateData() override
    {
        auto& L = proc.loudness;
        pkL = L.peakDbL.load(); pkR = L.peakDbR.load(); rmsL = L.rmsDbL.load(); rmsR = L.rmsDbR.load();
        maxL = std::max (maxL, pkL); maxR = std::max (maxR, pkR);
        clipL = clipL || pkL >= -0.01f; clipR = clipR || pkR >= -0.01f;
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        auto meter = [&] (juce::Rectangle<float> r, float pk, float rms, float mx, bool clip, const char* name)
        {
            auto label = r.removeFromBottom (14.0f);
            auto clipLed = r.removeFromTop (10.0f);
            g.setColour (clip ? P().warn : P().track);
            g.fillRoundedRectangle (clipLed.reduced (2.0f, 1.0f), 2.0f);
            auto toY = [&] (float db) { return juce::jmap (juce::jlimit (-60.0f, 3.0f, db), -60.0f, 3.0f, r.getBottom(), r.getY()); };
            g.setColour (P().track);
            g.fillRect (r);
            juce::ColourGradient gr (P().good, 0, r.getBottom(), P().warn, 0, r.getY(), false);
            gr.addColour (0.8, P().yellow);
            g.setGradientFill (gr);
            g.fillRect (r.withTop (toY (pk)).reduced (r.getWidth() * 0.3f, 0.0f));
            g.setColour (P().cyan.withAlpha (0.8f));
            g.fillRect (r.withTop (toY (rms)));
            g.setColour (P().text);
            g.drawHorizontalLine ((int) toY (mx), r.getX(), r.getRight());
            g.setFont (theme::font (9.0f, true));
            g.drawText (juce::String (name) + " " + juce::String (mx, 1), label, juce::Justification::centred);
        };
        auto scale = a.removeFromLeft (26.0f);
        g.setColour (P().textDim);
        g.setFont (theme::font (8.5f));
        for (int db : { 0, -6, -12, -24, -48 })
            g.drawText (juce::String (db), juce::Rectangle<float> (scale.getX(), juce::jmap ((float) db, -60.0f, 3.0f, a.getBottom() - 14.0f, a.getY() + 10.0f) - 5.0f, 24.0f, 10.0f), juce::Justification::centredRight);
        const float w = a.getWidth() / 2.0f;
        meter (a.removeFromLeft (w).reduced (6.0f, 0.0f), pkL, rmsL, maxL, clipL, "L");
        meter (a.reduced (6.0f, 0.0f), pkR, rmsR, maxR, clipR, "R");
    }
    juce::TextButton reset { "Reset max" };
    float pkL = -100, pkR = -100, rmsL = -100, rmsR = -100, maxL = -100, maxR = -100;
    bool clipL = false, clipR = false;
};

// ============================================================================
struct SubCheckViz : public VizPanel
{
    SubCheckViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("SUB CHECK 20-120 Hz", p, a) {}
    void updateData() override { db = analysis.subRmsDb(); corr = analysis.subCorrelation(); }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        const float monoBelow = proc.apvts.getRawParameterValue ("bassmono_freq")->load();
        const bool monoOn = proc.apvts.getRawParameterValue ("bassmono_on")->load() >= 0.5f;
        auto bar = a.removeFromLeft (a.getWidth() * 0.35f).reduced (10.0f, 4.0f);
        g.setColour (P().track);
        g.fillRoundedRectangle (bar, 4.0f);
        const float h = juce::jmap (juce::jlimit (-60.0f, 0.0f, db), -60.0f, 0.0f, 0.0f, bar.getHeight());
        g.setGradientFill (juce::ColourGradient (P().teal, 0, bar.getBottom(), P().magenta, 0, bar.getY(), false));
        g.fillRoundedRectangle (bar.withTop (bar.getBottom() - h), 4.0f);
        g.setColour (P().text);
        g.setFont (theme::mono (12.0f));
        g.drawText (juce::String (db, 1) + " dB", bar.withTrimmedBottom (4.0f), juce::Justification::centredBottom);

        const bool silent = db < -60.0f;
        const bool warn = corr < 0.9f && ! silent;
        auto led = a.removeFromTop (a.getHeight() * 0.5f).withSizeKeepingCentre (36.0f, 36.0f);
        g.setColour ((warn ? P().warn : P().good).withAlpha (0.3f));
        g.fillEllipse (led.expanded (6.0f));
        g.setColour (warn ? P().warn : P().good);
        g.fillEllipse (led);
        g.setColour (P().text);
        g.setFont (theme::font (12.0f, true));
        g.drawFittedText (silent ? juce::String ("No sub signal") : (warn ? "Sub is NOT mono!\ncorrelation " + juce::String (corr, 2)
                                                                    : "Sub mono OK\ncorrelation " + juce::String (corr, 2)), a.removeFromTop (40.0f).toNearestInt(), juce::Justification::centred, 2);
        g.setColour (P().textDim);
        g.setFont (theme::font (11.0f));
        g.drawFittedText (monoOn ? "Bass Mono Below: " + juce::String (juce::roundToInt (monoBelow)) + " Hz (on)" : juce::String ("Bass Mono Below is OFF"),
                          a.toNearestInt(), juce::Justification::centred, 2);
    }
    float db = -100.0f, corr = 1.0f;
};

// ============================================================================
struct TunerViz : public VizPanel
{
    TunerViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("TUNER", p, a) {}
    void updateData() override
    {
        float conf;
        const float f = analysis.detectPitch (conf);
        if (conf > 0.5f && f > 0.0f) { hz = hz * 0.6f + f * 0.4f; confidence = conf; }
        else confidence *= 0.9f;
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        float cents = 0.0f;
        const auto note = AnalysisData::noteName (confidence > 0.2f ? hz : 0.0f, cents);
        g.setColour (std::abs (cents) < 5.0f && confidence > 0.2f ? P().good : P().text);
        g.setFont (theme::font (std::min (48.0f, a.getHeight() * 0.4f), true));
        g.drawText (note, a.removeFromTop (a.getHeight() * 0.5f), juce::Justification::centred);
        auto scale = a.removeFromTop (30.0f).reduced (20.0f, 8.0f);
        g.setColour (P().track);
        g.fillRect (scale);
        g.setColour (P().text);
        g.drawVerticalLine ((int) scale.getCentreX(), scale.getY() - 4.0f, scale.getBottom() + 4.0f);
        const float x = scale.getCentreX() + juce::jlimit (-50.0f, 50.0f, cents) / 50.0f * scale.getWidth() * 0.5f;
        g.setColour (P().magenta);
        g.fillRect (x - 2.0f, scale.getY() - 4.0f, 4.0f, scale.getHeight() + 8.0f);
        g.setColour (P().textDim);
        g.setFont (theme::mono (12.0f));
        g.drawText (confidence > 0.2f ? juce::String (hz, 2) + " Hz   " + (cents >= 0 ? "+" : "") + juce::String (cents, 1) + " cents" : juce::String ("no stable pitch"), a, juce::Justification::centred);
    }
    float hz = 0.0f, confidence = 0.0f;
};

// ============================================================================
struct CreativeViz : public VizPanel
{
    CreativeViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("CREATIVE", p, a)
    {
        addAndMakeVisible (mode);
        mode.addItemList ({ "Circular spectrum", "Neon 3D terrain", "Sub particle field", "Retro VU needles", "CRT scope" }, 1);
        mode.setSelectedItemIndex (0);
        for (int i = 0; i < 300; ++i) particles.push_back ({ rng.nextFloat(), rng.nextFloat(), 0.0f, 0.0f });
    }
    void layoutControls (juce::Rectangle<int> r) override { mode.setBounds (r.removeFromLeft (150)); }
    void updateData() override
    {
        analysis.computeSpectrum (0, 11, 4.5f, spec);
        terrain.insert (terrain.begin(), std::vector<float> (48));
        for (int b = 0; b < 48; ++b)
        {
            const float hz = 20.0f * std::pow (1000.0f, (float) b / 48.0f);
            const int bin = juce::jlimit (0, (int) spec.size() - 1, (int) (hz / analysis.getSampleRate() * (double) spec.size() * 2.0));
            terrain.front()[(size_t) b] = juce::jlimit (0.0f, 1.0f, (spec[(size_t) bin] + 90.0f) / 90.0f);
        }
        if (terrain.size() > 40) terrain.pop_back();
        scope.resize (1024);
        std::vector<float> rr (1024);
        analysis.getLatest (0, scope.data(), rr.data(), 1024);
        const float subE = juce::jlimit (0.0f, 1.0f, (analysis.subRmsDb() + 50.0f) / 50.0f);
        for (auto& p : particles)
        {
            p.vx += (rng.nextFloat() - 0.5f) * 0.004f * (1.0f + subE * 6.0f);
            p.vy += (rng.nextFloat() - 0.5f) * 0.004f * (1.0f + subE * 6.0f) - 0.0005f;
            p.vx *= 0.95f; p.vy *= 0.95f;
            p.x = std::fmod (p.x + p.vx + 1.0f, 1.0f); p.y = std::fmod (p.y + p.vy + 1.0f, 1.0f);
        }
        sub = subE;
        auto& L = proc.loudness;
        needleL += (juce::jlimit (0.0f, 1.0f, (L.rmsDbL.load() + 30.0f) / 33.0f) - needleL) * 0.25f;
        needleR += (juce::jlimit (0.0f, 1.0f, (L.rmsDbR.load() + 30.0f) / 33.0f) - needleR) * 0.25f;
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        switch (mode.getSelectedItemIndex())
        {
            case 0:
            {
                const auto c = a.getCentre();
                const float r0 = std::min (a.getWidth(), a.getHeight()) * 0.2f;
                for (int b = 0; b < 96 && ! spec.empty(); ++b)
                {
                    const float hz = 20.0f * std::pow (1000.0f, (float) b / 96.0f);
                    const int bin = juce::jlimit (0, (int) spec.size() - 1, (int) (hz / analysis.getSampleRate() * (double) spec.size() * 2.0));
                    const float v = juce::jlimit (0.0f, 1.0f, (spec[(size_t) bin] + 90.0f) / 90.0f);
                    const float ang = (float) b / 96.0f * juce::MathConstants<float>::twoPi;
                    const float len = r0 * (1.0f + v * 1.3f);
                    g.setColour (P().magenta.interpolatedWith (P().cyan, (float) b / 96.0f));
                    g.drawLine (c.x + std::cos (ang) * r0, c.y + std::sin (ang) * r0, c.x + std::cos (ang) * len, c.y + std::sin (ang) * len, 3.0f);
                }
                break;
            }
            case 1:
                for (int row = (int) terrain.size() - 1; row >= 0; --row)
                {
                    const float depth = (float) row / 40.0f;
                    const float y0 = a.getY() + a.getHeight() * (0.35f + 0.6f * (1.0f - depth));
                    const float w = a.getWidth() * (1.0f - depth * 0.6f);
                    juce::Path p;
                    for (int b = 0; b < 48; ++b)
                    {
                        const float x = a.getCentreX() - w * 0.5f + w * (float) b / 47.0f;
                        const float y = y0 - terrain[(size_t) row][(size_t) b] * a.getHeight() * 0.3f * (1.0f - depth * 0.5f);
                        if (b == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                    }
                    g.setColour (P().magenta.interpolatedWith (P().cyan, depth).withAlpha (1.0f - depth * 0.8f));
                    g.strokePath (p, juce::PathStrokeType (1.3f));
                }
                break;
            case 2:
                for (auto& p : particles)
                {
                    const float s = 2.0f + sub * 6.0f;
                    g.setColour (P().cyan.interpolatedWith (P().pink, p.y).withAlpha (0.3f + sub * 0.7f));
                    g.fillEllipse (a.getX() + p.x * a.getWidth(), a.getY() + p.y * a.getHeight(), s, s);
                }
                break;
            case 3:
            {
                auto vu = [&] (juce::Rectangle<float> r, float v, const char* name)
                {
                    g.setColour (juce::Colour (0xfff0e2b0));
                    g.fillRoundedRectangle (r, 6.0f);
                    const auto pivot = juce::Point<float> (r.getCentreX(), r.getBottom() - 8.0f);
                    const float len = r.getHeight() * 0.8f;
                    g.setColour (juce::Colours::black.withAlpha (0.6f));
                    for (int t = 0; t <= 10; ++t)
                    {
                        const float ang = juce::jmap ((float) t / 10.0f, -0.8f, 0.8f);
                        g.drawLine (pivot.x + std::sin (ang) * len * 0.85f, pivot.y - std::cos (ang) * len * 0.85f, pivot.x + std::sin (ang) * len * 0.95f, pivot.y - std::cos (ang) * len * 0.95f, t > 7 ? 2.5f : 1.2f);
                    }
                    const float ang = juce::jmap (v, -0.8f, 0.8f);
                    g.setColour (juce::Colour (0xffc02020));
                    g.drawLine (pivot.x, pivot.y, pivot.x + std::sin (ang) * len, pivot.y - std::cos (ang) * len, 2.0f);
                    g.setColour (juce::Colours::black);
                    g.setFont (theme::font (12.0f, true));
                    g.drawText (juce::String ("VU ") + name, r.reduced (6.0f), juce::Justification::bottomLeft);
                };
                vu (a.removeFromLeft (a.getWidth() * 0.5f).reduced (6.0f), needleL, "L");
                vu (a.reduced (6.0f), needleR, "R");
                break;
            }
            default:
            {
                g.setColour (juce::Colour (0xff031008));
                g.fillRect (a);
                juce::Path p;
                for (size_t i = 0; i < scope.size(); ++i)
                {
                    const float x = a.getX() + a.getWidth() * (float) i / (float) scope.size();
                    const float y = a.getCentreY() - juce::jlimit (-1.0f, 1.0f, scope[i]) * a.getHeight() * 0.4f;
                    if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                }
                g.setColour (juce::Colour (0xff40ff80).withAlpha (0.25f));
                g.strokePath (p, juce::PathStrokeType (5.0f));
                g.setColour (juce::Colour (0xffa0ffc0));
                g.strokePath (p, juce::PathStrokeType (1.5f));
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                for (float y = a.getY(); y < a.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, a.getX(), a.getRight());
                break;
            }
        }
    }
    struct Pt { float x, y, vx, vy; };
    juce::ComboBox mode;
    std::vector<float> spec, scope;
    std::vector<std::vector<float>> terrain;
    std::vector<Pt> particles;
    juce::Random rng;
    float sub = 0.0f, needleL = 0.0f, needleR = 0.0f;
};

// ============================================================================
struct PhaseDcViz : public VizPanel
{
    PhaseDcViz (BassGodsProcessor& p, AnalysisData& a) : VizPanel ("PHASE / POLARITY + DC", p, a) {}
    void updateData() override
    {
        std::vector<float> l (4096), r (4096);
        analysis.getLatest (0, l.data(), r.data(), 4096);
        // polarity: compare L/R correlation and the sign of the dominant low-frequency swing
        corr = analysis.correlation (4096);
        double sum = 0;
        for (size_t i = 0; i < l.size(); ++i) sum += l[i] + r[i];
        asym = (float) (sum / 8192.0);
        dcL = analysis.dcOffset (0);
        dcR = analysis.dcOffset (1);
    }
    void draw (juce::Graphics& g, juce::Rectangle<float> a) override
    {
        const float rowH = a.getHeight() / 4.0f;
        auto row = [&] (const juce::String& name, const juce::String& value, bool bad)
        {
            auto r = a.removeFromTop (rowH);
            g.setColour (bad ? P().warn : P().good);
            g.fillEllipse (r.getX() + 4.0f, r.getCentreY() - 6.0f, 12.0f, 12.0f);
            g.setColour (P().textDim);
            g.setFont (theme::font (12.0f));
            g.drawText (name, r.withTrimmedLeft (24.0f), juce::Justification::centredLeft);
            g.setColour (P().text);
            g.setFont (theme::mono (12.0f));
            g.drawText (value, r, juce::Justification::centredRight);
        };
        row ("L/R phase", corr < 0.0f ? "OUT OF PHASE (" + juce::String (corr, 2) + ")" : "OK (" + juce::String (corr, 2) + ")", corr < 0.0f);
        row ("Polarity / asymmetry", juce::String (asym * 100.0f, 2) + " %", std::abs (asym) > 0.05f);
        row ("DC offset L", juce::String (dcL * 100.0f, 3) + " %", std::abs (dcL) > 0.005f);
        row ("DC offset R", juce::String (dcR * 100.0f, 3) + " %", std::abs (dcR) > 0.005f);
    }
    float corr = 1.0f, asym = 0.0f, dcL = 0.0f, dcR = 0.0f;
};
} // namespace

namespace viz
{
const juce::StringArray& names()
{
    static const juce::StringArray n { "Spectrum", "Spectrogram", "Oscilloscope", "Goniometer", "Loudness", "Levels", "Sub Check", "Tuner", "Creative", "Phase / DC" };
    return n;
}

std::unique_ptr<VizPanel> create (int i, BassGodsProcessor& p, AnalysisData& a)
{
    switch (i)
    {
        case 0: return std::make_unique<SpectrumViz> (p, a);
        case 1: return std::make_unique<SpectrogramViz> (p, a);
        case 2: return std::make_unique<ScopeViz> (p, a);
        case 3: return std::make_unique<GonioViz> (p, a);
        case 4: return std::make_unique<LoudnessViz> (p, a);
        case 5: return std::make_unique<LevelViz> (p, a);
        case 6: return std::make_unique<SubCheckViz> (p, a);
        case 7: return std::make_unique<TunerViz> (p, a);
        case 8: return std::make_unique<CreativeViz> (p, a);
        default: return std::make_unique<PhaseDcViz> (p, a);
    }
}
} // namespace viz
} // namespace bgdj
