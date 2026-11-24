#include "FxTab.h"
#include "../../PluginProcessor.h"

namespace bgdj
{
using MP = FxTab::ModulePreset;

const std::vector<MP>& FxTab::presetsFor (int t)
{
    static const std::vector<std::vector<MP>> all = {
        { { "Warm Tube", { { "mode", "Tube" }, { "drive", 0.35 }, { "tone", 9000.0 }, { "mix", 1.0 } } },
          { "Crunchy Diode", { { "mode", "Diode" }, { "drive", 0.55 }, { "tone", 6000.0 } } },
          { "Folded Growl", { { "mode", "Foldback" }, { "drive", 0.45 }, { "tone", 8000.0 }, { "mix", 0.7 } } },
          { "Bitcrushed", { { "mode", "Bitcrush" }, { "drive", 0.6 }, { "mix", 0.6 } } },
          { "Multiband Grit (clean sub)", { { "mode", "Multiband" }, { "mb_low", 0.0 }, { "mb_mid", 0.7 }, { "mb_high", 0.4 }, { "mb_xlow", 140.0 } } } },
        { { "OTT Classic", { { "depth", 1.0 }, { "time", 0.5 }, { "mix", 0.5 } } },
          { "OTT Gentle", { { "depth", 0.5 }, { "time", 0.6 }, { "mix", 0.3 } } },
          { "Upward Only", { { "depth", 1.0 }, { "low_down", 0.0 }, { "mid_down", 0.0 }, { "high_down", 0.0 }, { "mix", 0.4 } } } },
        { { "VCA Punch", { { "type", "VCA" }, { "thresh", -20.0 }, { "ratio", 4.0 }, { "attack", 10.0 }, { "release", 80.0 } } },
          { "Opto Smooth", { { "type", "Opto" }, { "thresh", -24.0 }, { "ratio", 3.0 }, { "attack", 20.0 }, { "release", 300.0 } } },
          { "Glue Bus", { { "type", "Glue" }, { "thresh", -16.0 }, { "ratio", 2.0 }, { "attack", 30.0 }, { "release", 200.0 } } },
          { "Sidechain from Kick (host)", { { "type", "VCA" }, { "sc", true }, { "thresh", -30.0 }, { "ratio", 8.0 }, { "attack", 1.0 }, { "release", 120.0 }, { "sc_hpf", 20.0 } } } },
        { { "Kick Duck 1/4 Hard", { { "rate", "1/4" }, { "shape", "Kick Duck Hard" }, { "depth", 0.9 } } },
          { "Kick Duck 1/4 Soft", { { "rate", "1/4" }, { "shape", "Kick Duck Soft" }, { "depth", 0.7 } } },
          { "Kick Duck 1/8 Pump", { { "rate", "1/8" }, { "shape", "Kick Duck Hard" }, { "depth", 0.6 } } },
          { "Long Pump 1/2", { { "rate", "1/2" }, { "shape", "Long Pump" }, { "depth", 0.8 } } },
          { "Gate Chop 1/16", { { "rate", "1/16" }, { "shape", "Gate Chop" }, { "depth", 1.0 } } } },
        { { "Sub Clean-up", { { "b1_on", true }, { "b1_type", "Low Cut" }, { "b1_freq", 28.0 }, { "b3_on", true }, { "b3_type", "Peak" }, { "b3_freq", 280.0 }, { "b3_gain", -3.0 }, { "b3_q", 1.2 } } },
          { "Mid Bite", { { "b4_on", true }, { "b4_type", "Peak" }, { "b4_freq", 1400.0 }, { "b4_gain", 4.0 }, { "b4_q", 0.9 } } },
          { "Dark Club", { { "b6_on", true }, { "b6_type", "High Cut" }, { "b6_freq", 5000.0 } } } },
        { { "Slow LP Sweep", { { "type", "Low Pass" }, { "cutoff", 1500.0 }, { "reso", 0.4 }, { "lfo_sync", true }, { "lfo_sync_rate", "2 Bars" }, { "lfo_depth", 0.6 } } },
          { "HP Riser", { { "type", "High Pass" }, { "cutoff", 200.0 }, { "reso", 0.3 }, { "lfo_sync", true }, { "lfo_sync_rate", "4 Bars" }, { "lfo_depth", 0.8 } } } },
        { { "Wide Chorus", { { "mode", "Chorus" }, { "rate", 0.5 }, { "depth", 0.5 }, { "mix", 0.4 } } },
          { "Jet Flanger", { { "mode", "Flanger" }, { "rate", 0.2 }, { "depth", 0.8 }, { "feedback", 0.7 }, { "mix", 0.5 } } },
          { "Phaser Swirl", { { "mode", "Phaser" }, { "rate", 0.3 }, { "depth", 0.7 }, { "stages", 8 }, { "feedback", 0.5 }, { "mix", 0.6 } } } },
        { { "Dotted Ping-Pong", { { "mode", "Ping-Pong" }, { "sync", true }, { "sync_l", "1/8 D" }, { "sync_r", "1/8 D" }, { "feedback", 0.4 }, { "mix", 0.2 } } },
          { "Dub Tape", { { "mode", "Tape" }, { "sync", true }, { "sync_l", "1/4" }, { "sync_r", "1/4 D" }, { "feedback", 0.6 }, { "wow", 0.4 }, { "lp", 3500.0 } } } },
        { { "Club (matches the room)", { { "type", "Club" }, { "size", 0.55 }, { "decay", 1.4 }, { "damp", 0.5 }, { "mix", 0.18 }, { "lowcut", 200.0 } } },
          { "Plate", { { "type", "Plate" }, { "size", 0.5 }, { "decay", 2.2 }, { "mix", 0.2 } } },
          { "Small Room", { { "type", "Room" }, { "size", 0.3 }, { "decay", 0.6 }, { "mix", 0.15 } } },
          { "Big Hall", { { "type", "Hall" }, { "size", 0.9 }, { "decay", 5.0 }, { "mix", 0.25 } } } },
        { { "Hyper Wide", { { "voices", 6 }, { "detune", 0.5 }, { "rate", 0.4 }, { "dim", 0.5 }, { "mono_below", 160.0 }, { "mix", 0.6 } } } },
        { { "Warm Tape", { { "drive", 0.3 }, { "warmth", 0.6 }, { "wow", 0.15 }, { "flutter", 0.1 } } },
          { "Hot Saturation", { { "drive", 0.7 }, { "warmth", 0.3 }, { "tone", 0.2 } } } },
        { { "Mono Bass < 120", { { "mono_on", true }, { "mono_below", 120.0 }, { "width", 1.0 } } },
          { "Wide Top", { { "mono_on", true }, { "mono_below", 150.0 }, { "width", 1.5 } } } },
        { { "Safe -0.3 dB TP", { { "mode", "Limiter" }, { "ceiling", -0.3 }, { "true_peak", true }, { "release", 80.0 } } },
          { "Loud Clip", { { "mode", "Limit + Clip" }, { "input", 4.0 }, { "ceiling", -0.5 } } } },
    };
    static const std::vector<MP> none;
    return juce::isPositiveAndBelow (t, (int) all.size()) ? all[(size_t) t] : none;
}

namespace
{
juce::File modulePresetFile (int t)
{
    auto d = AppSettings::getDataFolder().getChildFile ("ModulePresets");
    d.createDirectory();
    return d.getChildFile (params::fxModuleIds()[t] + ".json");
}

// ============================================================================
struct RackList : public juce::Component, private juce::Timer
{
    RackList (BassGodsProcessor& p, std::function<void (int)> onSel) : proc (p), onSelect (std::move (onSel))
    {
        addAndMakeVisible (addBtn);
        addBtn.onClick = [this]
        {
            juce::PopupMenu m;
            const auto order = proc.fxRack.getOrder();
            for (int t = 0; t < (int) FxType::Limiter; ++t)
                m.addItem (t + 1, params::fxModuleNames()[t], ! order.contains (t));
            m.addSeparator();
            m.addItem (100, "Reset to default order");
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addBtn), [this] (int r)
            {
                if (r == 100) { proc.fxRack.setDefaultOrder(); return; }
                if (r <= 0) return;
                auto o = proc.fxRack.getOrder();
                o.removeAllInstancesOf (-1);
                if (o.size() < kNumFxSlots) { o.add (r - 1); proc.fxRack.setOrder (o); }
            });
        };
        startTimerHz (10);
    }
    void timerCallback() override { repaint(); }

    juce::Array<int> rowsList() const
    {
        auto o = proc.fxRack.getOrder();
        o.removeAllInstancesOf (-1);
        o.add ((int) FxType::Limiter);
        return o;
    }
    juce::Rectangle<int> rowRect (int i) const { return { 0, 26 + i * 34, getWidth(), 31 }; }

    void resized() override { addBtn.setBounds (getLocalBounds().removeFromTop (22)); }

    void paint (juce::Graphics& g) override
    {
        const auto& p = theme::get();
        const auto rows = rowsList();
        for (int i = 0; i < rows.size(); ++i)
        {
            if (i == dragFrom && dragTo >= 0) continue;
            drawRow (g, rowRect (i).toFloat(), rows[i], i);
        }
        if (dragFrom >= 0 && dragTo >= 0)
        {
            const float y = (float) rowRect (dragTo).getY() - 2.0f;
            g.setColour (p.yellow);
            g.fillRect (0.0f, y, (float) getWidth(), 3.0f);
            drawRow (g, rowRect (dragFrom).toFloat().withY ((float) dragY - 15.0f), rows[dragFrom], dragFrom);
        }
    }

    void drawRow (juce::Graphics& g, juce::Rectangle<float> r, int t, int index)
    {
        const auto& p = theme::get();
        auto* m = proc.fxRack.getModule ((FxType) t);
        const bool on = m->on.on();
        const bool sel = t == selectedType;
        g.setColour (sel ? p.panelHi.brighter (0.1f) : p.panelHi);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (sel ? p.magenta : p.outline);
        g.drawRoundedRectangle (r, 5.0f, sel ? 1.5f : 1.0f);
        auto rr = r.reduced (6.0f, 4.0f);
        // power dot
        auto dot = rr.removeFromLeft (16.0f).withSizeKeepingCentre (12.0f, 12.0f);
        g.setColour (on ? p.good : p.track);
        g.fillEllipse (dot);
        rr.removeFromLeft (6.0f);
        g.setColour (on ? p.text : p.textDim);
        g.setFont (theme::font (12.5f, true));
        const bool fixed = t == (int) FxType::Limiter;
        g.drawText ((fixed ? juce::String ("FINAL  ") : juce::String (index + 1) + ".  ") + params::fxModuleNames()[t], rr, juce::Justification::centredLeft);
        if (! fixed)
        {
            g.setColour (p.textDim);
            g.drawText (juce::String (juce::CharPointer_UTF8 ("\xe2\x89\xa1  \xc3\x97")), rr, juce::Justification::centredRight);
        }
        const float gr = m->meterGainReduction.load();
        if (gr < -0.1f)
        {
            g.setColour (p.yellow);
            g.fillRect (r.getRight() - 52.0f, r.getBottom() - 5.0f, juce::jlimit (0.0f, 40.0f, -gr * 2.0f), 2.0f);
        }
    }

    int rowAt (int y) const
    {
        const auto rows = rowsList();
        for (int i = 0; i < rows.size(); ++i) if (rowRect (i).contains (4, y)) return i;
        return -1;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = rowAt (e.y);
        if (i < 0) return;
        const auto rows = rowsList();
        const int t = rows[i];
        auto rr = rowRect (i);
        if (e.x < 28)
        {
            auto* m = proc.fxRack.getModule ((FxType) t);
            if (auto* prm = proc.apvts.getParameter (params::fxPrefix ((FxType) t) + "on"))
                prm->setValueNotifyingHost (m->on.on() ? 0.0f : 1.0f);
            return;
        }
        if (t != (int) FxType::Limiter && e.x > rr.getRight() - 22)
        {
            auto o = proc.fxRack.getOrder();
            o.removeFirstMatchingValue (t);
            proc.fxRack.setOrder (o);
            return;
        }
        selectedType = t;
        if (onSelect) onSelect (t);
        if (t != (int) FxType::Limiter) dragFrom = i;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragFrom < 0 || e.getDistanceFromDragStart() < 4) return;
        dragY = e.y;
        const int n = rowsList().size() - 1;   // limiter cannot move
        dragTo = juce::jlimit (0, n - 1, (e.y - 26 + 17) / 34);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragFrom >= 0 && dragTo >= 0 && dragTo != dragFrom)
        {
            auto o = proc.fxRack.getOrder();
            o.removeAllInstancesOf (-1);
            const int t = o[dragFrom];
            o.remove (dragFrom);
            o.insert (dragTo, t);
            proc.fxRack.setOrder (o);
        }
        dragFrom = dragTo = -1;
        repaint();
    }

    BassGodsProcessor& proc;
    std::function<void (int)> onSelect;
    juce::TextButton addBtn { "+ Add module" };
    int selectedType = 0, dragFrom = -1, dragTo = -1, dragY = 0;
};

// ============================================================================
struct EqView : public juce::Component, private juce::Timer
{
    EqView (BassGodsProcessor& p, AnalysisData& a) : proc (p), analysis (a) { startTimerHz (30); }
    void timerCallback() override
    {
        analysis.computeSpectrum (0, 12, 4.5f, spectrum);
        for (size_t i = 0; i < spectrum.size(); ++i)
            smooth.size() == spectrum.size() ? (void) (smooth[i] = std::max (spectrum[i], smooth[i] - 1.5f)) : (void) 0;
        if (smooth.size() != spectrum.size()) smooth = spectrum;
        repaint();
    }
    float xForHz (float hz) const { return (float) getWidth() * std::log (hz / 20.0f) / std::log (1000.0f); }
    float hzForX (float x) const { return 20.0f * std::pow (1000.0f, x / (float) getWidth()); }
    float yForDb (float db) const { return (float) getHeight() * (0.5f - db / 48.0f); }
    void paint (juce::Graphics& g) override
    {
        const auto& p = theme::get();
        g.setColour (p.bg);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
        g.setColour (p.outline.withAlpha (0.4f));
        for (float hz : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
            g.drawVerticalLine ((int) xForHz (hz), 0.0f, (float) getHeight());
        g.drawHorizontalLine ((int) yForDb (0.0f), 0.0f, (float) getWidth());
        // spectrum overlay
        if (! smooth.empty())
        {
            const double sr = analysis.getSampleRate();
            juce::Path sp;
            sp.startNewSubPath (0.0f, (float) getHeight());
            for (int x = 0; x < getWidth(); x += 2)
            {
                const float hz = hzForX ((float) x);
                const int bin = juce::jlimit (0, (int) smooth.size() - 1, (int) (hz / sr * 4096.0));
                const float db = smooth[(size_t) bin];
                sp.lineTo ((float) x, juce::jlimit (0.0f, (float) getHeight(), (float) getHeight() * (1.0f - (db + 90.0f) / 90.0f)));
            }
            sp.lineTo ((float) getWidth(), (float) getHeight());
            sp.closeSubPath();
            g.setColour (p.magenta.withAlpha (0.2f));
            g.fillPath (sp);
        }
        // EQ curve
        juce::Path curve;
        auto& eq = proc.fxRack.eq();
        for (int x = 0; x < getWidth(); x += 2)
        {
            const float y = yForDb (eq.responseDb (hzForX ((float) x)));
            if (x == 0) curve.startNewSubPath (0.0f, y); else curve.lineTo ((float) x, y);
        }
        g.setColour (p.cyan);
        g.strokePath (curve, juce::PathStrokeType (2.0f));
        for (int b = 0; b < 6; ++b)
        {
            const auto pre = params::fxPrefix (FxType::Eq) + "b" + juce::String (b + 1) + "_";
            const bool on = proc.apvts.getRawParameterValue (pre + "on")->load() >= 0.5f;
            const float x = xForHz (proc.apvts.getRawParameterValue (pre + "freq")->load());
            const float y = yForDb (proc.apvts.getRawParameterValue (pre + "gain")->load());
            g.setColour (on ? p.yellow : p.textDim);
            g.fillEllipse (x - 6.0f, y - 6.0f, 12.0f, 12.0f);
            g.setColour (juce::Colours::black);
            g.setFont (theme::font (9.0f, true));
            g.drawText (juce::String (b + 1), juce::Rectangle<float> (x - 6.0f, y - 6.0f, 12.0f, 12.0f), juce::Justification::centred);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        drag = -1;
        float best = 14.0f;
        for (int b = 0; b < 6; ++b)
        {
            const auto pre = params::fxPrefix (FxType::Eq) + "b" + juce::String (b + 1) + "_";
            const float x = xForHz (proc.apvts.getRawParameterValue (pre + "freq")->load());
            const float y = yForDb (proc.apvts.getRawParameterValue (pre + "gain")->load());
            const float d = e.position.getDistanceFrom ({ x, y });
            if (d < best) { best = d; drag = b; }
        }
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (drag < 0) return;
        const auto pre = params::fxPrefix (FxType::Eq) + "b" + juce::String (drag + 1) + "_";
        auto set = [this] (const juce::String& id, float v) { if (auto* prm = proc.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v)); };
        set (pre + "on", 1.0f);
        set (pre + "freq", juce::jlimit (20.0f, 20000.0f, hzForX (e.position.x)));
        set (pre + "gain", juce::jlimit (-24.0f, 24.0f, (0.5f - e.position.y / (float) getHeight()) * 48.0f));
    }
    BassGodsProcessor& proc;
    AnalysisData& analysis;
    std::vector<float> spectrum, smooth;
    int drag = -1;
};

// ============================================================================
struct ModuleDetail : public juce::Component
{
    ModuleDetail (BassGodsProcessor& p, AnalysisData& a, int type) : proc (p), t (type)
    {
        const auto prefix = params::fxPrefix ((FxType) t);
        addAndMakeVisible (presetBtn);
        presetBtn.onClick = [this] { showPresetMenu(); };
        onToggle = std::make_unique<ParamToggle> (proc, prefix + "on", "ON");
        addAndMakeVisible (*onToggle);
        mix = std::make_unique<Knob> (proc, prefix + "mix", "Mix", theme::get().yellow);
        addAndMakeVisible (*mix);

        for (auto* prm : proc.getParameters())
        {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm);
            if (rp == nullptr || ! rp->paramID.startsWith (prefix)) continue;
            const auto suffix = rp->paramID.fromFirstOccurrenceOf (prefix, false, false);
            if (suffix == "on" || suffix == "mix") continue;
            juce::Component* c;
            const auto label = rp->getName (40).fromFirstOccurrenceOf (params::fxModuleNames()[t].upToFirstOccurrenceOf (" ", false, false) + " ", false, false);
            if (dynamic_cast<juce::AudioParameterChoice*> (rp) != nullptr) c = new ParamChoice (proc, rp->paramID);
            else if (dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr) c = new ParamToggle (proc, rp->paramID, label.isEmpty() ? rp->getName (30) : label);
            else c = new Knob (proc, rp->paramID, label.isEmpty() ? rp->getName (12) : label.substring (0, 14), theme::get().cyan);
            controls.add (c);
            addAndMakeVisible (c);
            names.add (label.isEmpty() ? rp->getName (30) : label);
        }

        if (t == (int) FxType::Eq)
        {
            eqView = std::make_unique<EqView> (proc, a);
            addAndMakeVisible (*eqView);
        }
        if (t == (int) FxType::Ducker)
        {
            curve = std::make_unique<CurveEditor>();
            curve->colour = theme::get().teal;
            curve->getPoints = [this]
            {
                const int sh = (int) proc.apvts.getRawParameterValue ("fx_duck_shape")->load();
                return sh == 4 ? proc.getDuckPoints() : shapes::duckPreset (sh);
            };
            curve->setPoints = [this] (const CurvePoints& pts)
            {
                proc.setDuckPoints (pts);
                if (auto* prm = proc.apvts.getParameter ("fx_duck_shape")) prm->setValueNotifyingHost (prm->convertTo0to1 (4.0f));
            };
            curve->getPlayhead = [this] { return proc.fxRack.ducker().uiPhase.load(); };
            addAndMakeVisible (*curve);
        }
    }

    void showPresetMenu()
    {
        juce::PopupMenu m;
        const auto& ps = FxTab::presetsFor (t);
        for (int i = 0; i < (int) ps.size(); ++i) m.addItem (i + 1, ps[(size_t) i].name);
        const auto user = juce::JSON::parse (modulePresetFile (t));
        juce::PopupMenu userMenu;
        if (auto* o = user.getDynamicObject())
        {
            int k = 0;
            for (auto& prop : o->getProperties()) userMenu.addItem (500 + k++, prop.name.toString());
        }
        m.addSeparator();
        m.addSubMenu ("My module presets", userMenu, userMenu.getNumItems() > 0);
        m.addItem (900, "Save current settings as module preset...");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [this, user] (int r)
        {
            const auto prefix = params::fxPrefix ((FxType) t);
            auto setVal = [this, &prefix] (const juce::String& suffix, const juce::var& v)
            {
                if (auto* rp = proc.apvts.getParameter (prefix + suffix))
                {
                    float real;
                    if (presetformat::resolveValue (*rp, v, real)) rp->setValueNotifyingHost (rp->convertTo0to1 (real));
                }
            };
            if (r > 0 && r < 500)
            {
                setVal ("on", true);
                for (auto& [k, v] : FxTab::presetsFor (t)[(size_t) r - 1].values) setVal (k, v);
            }
            else if (r >= 500 && r < 900)
            {
                if (auto* o = user.getDynamicObject())
                {
                    auto props = o->getProperties();
                    if (auto* vals = props.getValueAt (r - 500).getDynamicObject())
                        for (auto& pv : vals->getProperties()) setVal (pv.name.toString(), pv.value);
                }
            }
            else if (r == 900)
            {
                auto* w = new juce::AlertWindow ("Save module preset", params::fxModuleNames()[t], juce::MessageBoxIconType::NoIcon);
                w->addTextEditor ("name", "My " + params::fxModuleNames()[t]);
                w->addButton ("Save", 1);
                w->addButton ("Cancel", 0);
                w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int res)
                {
                    if (res != 1) return;
                    auto root = juce::JSON::parse (modulePresetFile (t));
                    if (! root.isObject()) root = juce::var (new juce::DynamicObject());
                    auto* vals = new juce::DynamicObject();
                    const auto prefix = params::fxPrefix ((FxType) t);
                    for (auto* prm : proc.getParameters())
                        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm); rp != nullptr && rp->paramID.startsWith (prefix))
                            vals->setProperty (rp->paramID.fromFirstOccurrenceOf (prefix, false, false), rp->convertFrom0to1 (rp->getValue()));
                    root.getDynamicObject()->setProperty (w->getTextEditorContents ("name"), juce::var (vals));
                    modulePresetFile (t).replaceWithText (juce::JSON::toString (root));
                }), true);
            }
        });
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        r.removeFromTop (22);
        auto top = r.removeFromTop (60);
        onToggle->setBounds (top.removeFromLeft (60).removeFromTop (22));
        presetBtn.setBounds (top.removeFromRight (160).removeFromTop (24));
        mix->setBounds (top.removeFromLeft (60));
        r.removeFromTop (6);
        if (eqView) { eqView->setBounds (r.removeFromTop (r.getHeight() / 2)); r.removeFromTop (8); }
        if (curve)  { curve->setBounds (r.removeFromTop (r.getHeight() / 2)); r.removeFromTop (8); }

        // flow layout: choices/toggles get a wide cell, knobs a square one
        int x = r.getX(), y = r.getY();
        const int knobW = 74, knobH = 78, wide = 170, rowH = knobH + 6;
        for (auto* c : controls)
        {
            const bool isKnob = dynamic_cast<Knob*> (c) != nullptr;
            const int w = isKnob ? knobW : wide;
            if (x + w > r.getRight()) { x = r.getX(); y += rowH; }
            if (isKnob) c->setBounds (x, y, knobW, knobH);
            else c->setBounds (x, y + 24, w, 24);
            x += w + 8;
        }
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat(), params::fxModuleNames()[t].toUpperCase(), t == (int) FxType::Limiter ? theme::get().warn : theme::get().magenta);
        auto* m = proc.fxRack.getModule ((FxType) t);
        const float gr = m->meterGainReduction.load();
        if (t == (int) FxType::Compressor || t == (int) FxType::Ott || t == (int) FxType::Limiter || t == (int) FxType::Ducker)
        {
            g.setColour (theme::get().textDim);
            g.setFont (theme::mono (12.0f));
            g.drawText ("GR " + juce::String (gr, 1) + " dB", getLocalBounds().reduced (12).removeFromTop (20), juce::Justification::centredRight);
        }
        for (int i = 0; i < controls.size(); ++i)
            if (dynamic_cast<ParamChoice*> (controls[i]) != nullptr)
            {
                g.setColour (theme::get().textDim);
                g.setFont (theme::font (10.5f, true));
                g.drawText (names[i].toUpperCase(), controls[i]->getBounds().translated (0, -16).withHeight (14), juce::Justification::centredLeft);
            }
    }

    BassGodsProcessor& proc;
    int t;
    juce::TextButton presetBtn { "Module presets..." };
    std::unique_ptr<ParamToggle> onToggle;
    std::unique_ptr<Knob> mix;
    juce::OwnedArray<juce::Component> controls;
    juce::StringArray names;
    std::unique_ptr<EqView> eqView;
    std::unique_ptr<CurveEditor> curve;
};

struct Repainter : public juce::Timer
{
    juce::Component& c;
    explicit Repainter (juce::Component& comp) : c (comp) { startTimerHz (10); }
    void timerCallback() override { c.repaint(); }
};
} // namespace

// ============================================================================
FxTab::FxTab (BassGodsProcessor& p, AnalysisData& a) : proc (p), analysis (a)
{
    rackList = std::make_unique<RackList> (proc, [this] (int t) { selectModule (t); });
    addAndMakeVisible (*rackList);
    selectModule (0);
}

FxTab::~FxTab() = default;

void FxTab::selectModule (int t)
{
    selected = t;
    detail = std::make_unique<ModuleDetail> (proc, analysis, t);
    addAndMakeVisible (*detail);
    static_cast<RackList*> (rackList.get())->selectedType = t;
    resized();
}

void FxTab::paint (juce::Graphics& g)
{
    g.fillAll (theme::get().bg);
    auto r = getLocalBounds().reduced (10);
    drawPanel (g, r.removeFromLeft (300).toFloat(), "FX RACK (drag to reorder)", theme::get().cyan);
}

void FxTab::resized()
{
    auto r = getLocalBounds().reduced (10);
    auto left = r.removeFromLeft (300);
    rackList->setBounds (left.reduced (10).withTrimmedTop (22));
    r.removeFromLeft (10);
    if (detail) detail->setBounds (r);
}
} // namespace bgdj
