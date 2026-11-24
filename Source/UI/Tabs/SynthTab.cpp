#include "SynthTab.h"
#include "../../PluginProcessor.h"

namespace bgdj
{
namespace
{
// ============================================================================
struct Panel : public juce::Component
{
    Panel (juce::String t, juce::Colour c) : title (std::move (t)), accent (c) {}
    void paint (juce::Graphics& g) override { drawPanel (g, getLocalBounds().toFloat(), title, accent); }
    juce::Rectangle<int> content() const { return getLocalBounds().reduced (6).withTrimmedTop (18); }
    juce::String title;
    juce::Colour accent;
};

template <typename T, typename... Args>
T* add (juce::Component& parent, juce::OwnedArray<juce::Component>& owner, Args&&... args)
{
    auto* c = new T (std::forward<Args> (args)...);
    owner.add (c);
    parent.addAndMakeVisible (c);
    return c;
}

// ============================================================================
struct MacroPanel : public Panel, private juce::Timer
{
    MacroPanel (BassGodsProcessor& p, int first, ClubScene& s, bool left)
        : Panel (left ? "MACRO 1-2" : "MACRO 3-4", theme::get().yellow), proc (p), scene (s), isLeft (left)
    {
        for (int k = 0; k < 2; ++k)
        {
            const int m = first + k;
            knobs[k] = add<Knob> (*this, owned, proc, params::macroId (m), "", theme::get().yellow);
            labels[k] = add<juce::Label> (*this, owned);
            labels[k]->setEditable (false, true);
            labels[k]->setJustificationType (juce::Justification::centred);
            labels[k]->setFont (theme::font (13.0f, true));
            labels[k]->setTooltip ("Double-click to rename");
            labels[k]->onTextChange = [this, k, m] { proc.macroLabels.set (m, labels[k]->getText()); };
            chips[k] = add<ModSourceChip> (*this, owned, (int) ModSource::Macro1 + m, "M" + juce::String (m + 1));
            macro[k] = m;
        }
        startTimerHz (15);
    }

    void timerCallback() override
    {
        for (int k = 0; k < 2; ++k)
            if (! labels[k]->isBeingEdited() && labels[k]->getText() != proc.macroLabels[macro[k]])
                labels[k]->setText (proc.macroLabels[macro[k]], juce::dontSendNotification);
        repaint();
    }

    void resized() override
    {
        auto r = content();
        auto meter = isLeft ? r.removeFromLeft (44) : r.removeFromRight (r.getWidth() / 2 + 6);
        meterArea = meter;
        for (int k = 0; k < 2; ++k)
        {
            auto cell = r.removeFromTop (r.getHeight() / (2 - k));
            labels[k]->setBounds (cell.removeFromTop (18));
            chips[k]->setBounds (cell.removeFromBottom (16).withSizeKeepingCentre (44, 16));
            knobs[k]->setBounds (cell);
        }
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        const auto& p = theme::get();
        const auto& m = scene.getMood();
        auto r = meterArea.toFloat().reduced (2.0f);
        if (isLeft)
        {
            // HYPE METER
            g.setColour (p.bg);
            g.fillRoundedRectangle (r, 5.0f);
            auto bar = r.reduced (6.0f).withTrimmedTop (14.0f).withTrimmedBottom (16.0f);
            const float h = bar.getHeight() * m.hype / 100.0f;
            juce::ColourGradient gr (p.cyan, 0, bar.getBottom(), p.magenta, 0, bar.getY(), false);
            gr.addColour (0.75, p.pink);
            g.setGradientFill (gr);
            g.fillRoundedRectangle (bar.withTop (bar.getBottom() - h), 3.0f);
            g.setColour (p.outline);
            g.drawRoundedRectangle (bar, 3.0f, 1.0f);
            for (int t = 1; t < 10; ++t)
                g.drawHorizontalLine ((int) (bar.getBottom() - bar.getHeight() * (float) t / 10.0f), bar.getX(), bar.getX() + 4.0f);
            g.setColour (p.text);
            g.setFont (theme::font (9.5f, true));
            g.drawText ("HYPE", r.removeFromTop (14.0f), juce::Justification::centred);
            g.setFont (theme::mono (11.0f));
            g.drawText (juce::String (juce::roundToInt (m.hype)), r.removeFromBottom (16.0f), juce::Justification::centred);
        }
        else
        {
            // CROWD MOOD readout
            g.setColour (p.bg);
            g.fillRoundedRectangle (r, 5.0f);
            auto rr = r.reduced (6.0f);
            g.setColour (p.textDim);
            g.setFont (theme::font (9.5f, true));
            g.drawText ("CROWD MOOD", rr.removeFromTop (12.0f), juce::Justification::centredLeft);
            g.setColour (m.annoyance > 40 ? p.warn : (m.hype > 70 ? p.pink : p.cyan));
            g.setFont (theme::font (14.0f, true));
            g.drawFittedText (m.moodWord(), rr.removeFromTop (34.0f).toNearestInt(), juce::Justification::centredLeft, 2);
            auto barRow = [&] (const juce::String& name, float v, juce::Colour c)
            {
                auto row = rr.removeFromTop (16.0f);
                g.setColour (p.textDim);
                g.setFont (theme::font (9.5f));
                g.drawText (name, row.removeFromLeft (44.0f), juce::Justification::centredLeft);
                auto b = row.reduced (0.0f, 4.0f);
                g.setColour (p.track);
                g.fillRoundedRectangle (b, 2.0f);
                g.setColour (c);
                g.fillRoundedRectangle (b.withWidth (b.getWidth() * juce::jlimit (0.0f, 1.0f, v)), 2.0f);
            };
            barRow ("Energy", m.energy, p.cyan);
            barRow ("Annoyed", m.annoyance / 100.0f, p.warn);
            barRow ("Bored", m.boredom / 100.0f, p.yellow);
            barRow ("Crowd", m.crowdSize, p.teal);
            g.setColour (p.textDim);
            g.setFont (theme::font (9.5f));
            g.drawFittedText (scene.lastReactionName(), rr.toNearestInt(), juce::Justification::topLeft, 3);
        }
    }

    BassGodsProcessor& proc;
    ClubScene& scene;
    bool isLeft;
    juce::OwnedArray<juce::Component> owned;
    Knob* knobs[2] {};
    juce::Label* labels[2] {};
    ModSourceChip* chips[2] {};
    int macro[2] {};
    juce::Rectangle<int> meterArea;
};

// ============================================================================
struct WtDisplay : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
    WtDisplay (BassGodsProcessor& p, int o) : proc (p), osc (o) { startTimerHz (15); setTooltip ("Click to open the wavetable editor"); }
    void timerCallback() override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { if (onClick) onClick(); }
    void paint (juce::Graphics& g) override
    {
        const auto& pal = theme::get();
        auto r = getLocalBounds().toFloat();
        g.setColour (pal.bg);
        g.fillRoundedRectangle (r, 5.0f);
        auto wt = proc.getTable (osc);
        if (wt == nullptr) return;
        const float pos = proc.apvts.getRawParameterValue (params::oscPrefix (osc) + "pos")->load();
        const int nf = wt->numFrames;
        const int cur = juce::roundToInt (pos * (float) (nf - 1));
        const int step = std::max (1, nf / 12);
        for (int f = 0; f < nf; f += step)
        {
            const float depth = (float) f / (float) std::max (1, nf - 1);
            const float ox = depth * r.getWidth() * 0.15f, oy = -depth * r.getHeight() * 0.3f;
            juce::Path path;
            const float* c = wt->cycle (f, 0);
            for (int i = 0; i < Wavetable::kSize; i += 32)
            {
                const float x = r.getX() + 4.0f + ox + (r.getWidth() * 0.82f) * (float) i / Wavetable::kSize;
                const float y = r.getCentreY() + r.getHeight() * 0.15f + oy - c[i] * r.getHeight() * 0.22f;
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (pal.magenta.withAlpha (0.18f));
            g.strokePath (path, juce::PathStrokeType (1.0f));
        }
        {
            const float depth = (float) cur / (float) std::max (1, nf - 1);
            const float ox = depth * r.getWidth() * 0.15f, oy = -depth * r.getHeight() * 0.3f;
            juce::Path path;
            const float* c = wt->cycle (cur, 0);
            for (int i = 0; i < Wavetable::kSize; i += 8)
            {
                const float x = r.getX() + 4.0f + ox + (r.getWidth() * 0.82f) * (float) i / Wavetable::kSize;
                const float y = r.getCentreY() + r.getHeight() * 0.15f + oy - c[i] * r.getHeight() * 0.22f;
                if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (pal.cyan);
            g.strokePath (path, juce::PathStrokeType (1.6f));
        }
        g.setColour (pal.textDim);
        g.setFont (theme::font (9.5f));
        g.drawText (juce::String (cur + 1) + "/" + juce::String (nf), r.reduced (4.0f), juce::Justification::bottomRight);
    }
    BassGodsProcessor& proc;
    int osc;
    std::function<void()> onClick;
};

struct OscPanel : public Panel, private juce::Timer
{
    OscPanel (BassGodsProcessor& p, int o, std::function<void (int)> onEdit)
        : Panel ({}, theme::get().cyan), proc (p), osc (o)
    {
        const auto pre = params::oscPrefix (o);
        const auto letter = juce::String::charToString ((juce::juce_wchar) ('A' + o));
        onBtn = add<ParamToggle> (*this, owned, proc, pre + "on", "OSC " + letter);
        tableBox = add<juce::ComboBox> (*this, owned);
        tableBox->addItemList (factory::tableNames(), 1);
        tableBox->addSeparator();
        tableBox->addItem ("Load user wavetable (WAV)...", 1000);
        tableBox->addItem ("Open wavetable editor...", 1001);
        tableBox->onChange = [this, onEdit]
        {
            const int id = tableBox->getSelectedId();
            if (id == 1000) { loadUser(); return; }
            if (id == 1001) { if (onEdit) onEdit (osc); syncTable(); return; }
            if (id > 0) proc.setFactoryTable (osc, id - 1);
        };
        display = add<WtDisplay> (*this, owned, proc, o);
        display->onClick = [this, onEdit] { if (onEdit) onEdit (osc); };
        warp = add<ParamChoice> (*this, owned, proc, pre + "warp");
        warpSrc = add<ParamChoice> (*this, owned, proc, pre + "warp_src");
        route = add<ParamChoice> (*this, owned, proc, pre + "route");
        const char* ids[] = { "pos", "warp_amt", "uni", "detune", "blend", "level", "pan", "width", "uni_warp", "phase", "oct", "semi", "fine", "coarse", "rand" };
        const char* names[] = { "WT Pos", "Warp", "Unison", "Detune", "Blend", "Level", "Pan", "Width", "U.Warp", "Phase", "Oct", "Semi", "Fine", "Pitch", "Rand" };
        for (int k = 0; k < 15; ++k)
            knobs.add (add<Knob> (*this, owned, proc, pre + ids[k], names[k], k < 5 ? theme::get().cyan : (k < 10 ? theme::get().pink : theme::get().teal)));
        startTimerHz (5);
        syncTable();
    }

    void loadUser()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load wavetable", proc.settings->wavetableFolders().getFirst(), "*.wav;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            juce::String err;
            if (fc.getResult().existsAsFile())
            {
                if (auto wt = Wavetable::fromAudioFile (fc.getResult(), err)) proc.setUserTable (osc, wt);
                else juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Wavetable import", err);
            }
            syncTable();
        });
    }

    void syncTable()
    {
        if (proc.isUserTable (osc))
            tableBox->setText (proc.getTableName (osc), juce::dontSendNotification);
        else
            tableBox->setSelectedId ((int) proc.apvts.getRawParameterValue (params::oscPrefix (osc) + "table")->load() + 1, juce::dontSendNotification);
    }

    void timerCallback() override
    {
        if (proc.tableVersion.load() != lastVersion) { lastVersion = proc.tableVersion.load(); syncTable(); }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (5);
        auto head = r.removeFromTop (20);
        onBtn->setBounds (head.removeFromLeft (64));
        tableBox->setBounds (head);
        r.removeFromTop (3);
        display->setBounds (r.removeFromTop (34));
        r.removeFromTop (3);
        layoutRow (r.removeFromTop (20), { warp, warpSrc, route }, 3);
        const int rowH = r.getHeight() / 3;
        for (int row = 0; row < 3; ++row)
        {
            auto line = r.removeFromTop (rowH);
            const int w = line.getWidth() / 5;
            for (int k = 0; k < 5; ++k) knobs[row * 5 + k]->setBounds (line.removeFromLeft (w));
        }
    }

    BassGodsProcessor& proc;
    int osc;
    int lastVersion = -1;
    juce::OwnedArray<juce::Component> owned;
    ParamToggle* onBtn = nullptr;
    juce::ComboBox* tableBox = nullptr;
    WtDisplay* display = nullptr;
    ParamChoice *warp = nullptr, *warpSrc = nullptr, *route = nullptr;
    juce::Array<Knob*> knobs;
    std::unique_ptr<juce::FileChooser> chooser;
};

struct OscDPanel : public Panel, private juce::Timer
{
    explicit OscDPanel (BassGodsProcessor& p) : Panel ({}, theme::get().teal), proc (p)
    {
        onBtn = add<ParamToggle> (*this, owned, proc, "oD_on", "OSC D");
        mode = add<ParamChoice> (*this, owned, proc, "oD_mode");
        shape = add<ParamChoice> (*this, owned, proc, "oD_sub_shape");
        oct = add<ParamChoice> (*this, owned, proc, "oD_sub_oct");
        lock = add<ParamToggle> (*this, owned, proc, "oD_sub_lock", "Sub Lock");
        noise = add<juce::ComboBox> (*this, owned);
        noise->onChange = [this]
        {
            if (auto* prm = proc.apvts.getParameter ("oD_noise_type"))
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (noise->getSelectedId() - 1)));
        };
        noise->setTooltip ("Noise / texture sample (factory + your noise folders)");
        key = add<ParamToggle> (*this, owned, proc, "oD_noise_key", "Key Trk");
        route = add<ParamChoice> (*this, owned, proc, "oD_route");
        level = add<Knob> (*this, owned, proc, "oD_level", "Level", theme::get().teal);
        pan = add<Knob> (*this, owned, proc, "oD_pan", "Pan", theme::get().teal);
        pitch = add<Knob> (*this, owned, proc, "oD_noise_pitch", "N.Pitch", theme::get().teal);
        startTimerHz (4);
        timerCallback();
    }
    void timerCallback() override
    {
        const auto names = NoiseBank::getInstance().currentNames();
        if (names != lastNames)
        {
            lastNames = names;
            noise->clear (juce::dontSendNotification);
            for (int i = 0; i < names.size(); ++i) noise->addItem (names[i], i + 1);
        }
        noise->setSelectedId ((int) proc.apvts.getRawParameterValue ("oD_noise_type")->load() + 1, juce::dontSendNotification);
        const bool sub = (int) proc.apvts.getRawParameterValue ("oD_mode")->load() == 0;
        shape->setEnabled (sub); oct->setEnabled (sub); lock->setEnabled (sub);
        noise->setEnabled (! sub); key->setEnabled (! sub); pitch->setEnabled (! sub);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (5);
        auto head = r.removeFromTop (20);
        onBtn->setBounds (head.removeFromLeft (64));
        mode->setBounds (head);
        r.removeFromTop (4);
        layoutRow (r.removeFromTop (20), { shape, oct }, 3);
        r.removeFromTop (3);
        lock->setBounds (r.removeFromTop (18));
        r.removeFromTop (3);
        noise->setBounds (r.removeFromTop (20));
        r.removeFromTop (3);
        layoutRow (r.removeFromTop (20), { key, route }, 3);
        layoutRow (r, { level, pan, pitch }, 2);
    }
    BassGodsProcessor& proc;
    juce::OwnedArray<juce::Component> owned;
    ParamToggle *onBtn, *lock, *key;
    ParamChoice *mode, *shape, *oct, *route;
    juce::ComboBox* noise;
    Knob *level, *pan, *pitch;
    juce::StringArray lastNames;
};

struct FilterPanel : public Panel
{
    explicit FilterPanel (BassGodsProcessor& p) : Panel ("FILTERS", theme::get().magenta), proc (p)
    {
        routing = add<ParamChoice> (*this, owned, proc, "filter_routing");
        routing->setTooltip ("Serial: F1 -> F2. Parallel: both side by side. Split: F1 on the left channel, F2 on the right.");
        for (int f = 0; f < 2; ++f)
        {
            const auto pre = params::filterPrefix (f);
            on[f] = add<ParamToggle> (*this, owned, proc, pre + "on", "F" + juce::String (f + 1));
            type[f] = add<ParamChoice> (*this, owned, proc, pre + "type");
            const char* ids[] = { "cutoff", "reso", "drive", "fat", "key", "mix", "pan" };
            const char* names[] = { "Cutoff", "Reso", "Drive", "Fat", "Key", "Mix", "Pan" };
            for (int k = 0; k < 7; ++k)
                knobs[f][k] = add<Knob> (*this, owned, proc, pre + ids[k], names[k], theme::get().magenta);
        }
    }
    void resized() override
    {
        auto r = content();
        auto top = getLocalBounds().reduced (6).removeFromTop (18);
        routing->setBounds (top.removeFromRight (100));
        for (int f = 0; f < 2; ++f)
        {
            auto block = r.removeFromTop (r.getHeight() / (2 - f));
            auto head = block.removeFromTop (20);
            on[f]->setBounds (head.removeFromLeft (44));
            type[f]->setBounds (head.removeFromLeft (150));
            const int w = block.getWidth() / 7;
            for (int k = 0; k < 7; ++k) knobs[f][k]->setBounds (block.removeFromLeft (w));
        }
    }
    BassGodsProcessor& proc;
    juce::OwnedArray<juce::Component> owned;
    ParamChoice* routing;
    ParamToggle* on[2];
    ParamChoice* type[2];
    Knob* knobs[2][7];
};

// ============================================================================
struct TabStrip : public juce::Component
{
    TabStrip (int n, const juce::String& prefix)
    {
        for (int i = 0; i < n; ++i)
        {
            auto* b = buttons.add (new juce::TextButton (prefix + juce::String (i + 1)));
            b->setRadioGroupId (0x5150 + n);
            b->setClickingTogglesState (true);
            b->onClick = [this, i] { if (onSelect) onSelect (i); };
            addAndMakeVisible (b);
        }
        buttons[0]->setToggleState (true, juce::dontSendNotification);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        const int w = r.getWidth() / buttons.size();
        for (auto* b : buttons) b->setBounds (r.removeFromLeft (w).reduced (1, 0));
    }
    juce::OwnedArray<juce::TextButton> buttons;
    std::function<void (int)> onSelect;
};

struct EnvPanel : public Panel
{
    explicit EnvPanel (BassGodsProcessor& p) : Panel ("ENV", theme::get().cyan), proc (p), tabs (4, "ENV ")
    {
        addAndMakeVisible (tabs);
        tabs.onSelect = [this] (int i) { select (i); };
        view = add<EnvelopeView> (*this, owned, proc, 0);
        for (int e = 0; e < kNumEnvs; ++e)
        {
            const auto pre = params::envPrefix (e);
            const char* ids[] = { "att", "dec", "sus", "rel", "acurve", "dcurve", "rcurve" };
            const char* names[] = { "Attack", "Decay", "Sustain", "Release", "A.Crv", "D.Crv", "R.Crv" };
            for (int k = 0; k < 7; ++k)
                knobs[e][k] = add<Knob> (*this, owned, proc, pre + ids[k], names[k], theme::get().cyan);
            chips[e] = add<ModSourceChip> (*this, owned, (int) ModSource::Env1 + e, "ENV " + juce::String (e + 1));
        }
        select (0);
    }
    void select (int e)
    {
        current = e;
        view->setEnvIndex (e);
        for (int i = 0; i < kNumEnvs; ++i)
        {
            for (auto* k : knobs[i]) k->setVisible (i == e);
            chips[i]->setVisible (i == e);
        }
        title = e == 0 ? "ENV 1 (AMP)" : "ENV " + juce::String (e + 1);
        resized();
        repaint();
    }
    void resized() override
    {
        auto r = content();
        tabs.setBounds (getLocalBounds().reduced (6).removeFromTop (18).withTrimmedLeft (100));
        auto knobRow = r.removeFromBottom (50);
        chips[current]->setBounds (r.removeFromRight (60).removeFromTop (16));
        view->setBounds (r.reduced (0, 2));
        const int w = knobRow.getWidth() / 7;
        for (int k = 0; k < 7; ++k) knobs[current][k]->setBounds (knobRow.removeFromLeft (w));
    }
    BassGodsProcessor& proc;
    juce::OwnedArray<juce::Component> owned;
    TabStrip tabs;
    EnvelopeView* view;
    Knob* knobs[kNumEnvs][7];
    ModSourceChip* chips[kNumEnvs];
    int current = 0;
};

struct LfoPanel : public Panel
{
    explicit LfoPanel (BassGodsProcessor& p) : Panel ("LFO", theme::get().magenta), proc (p), tabs (8, "")
    {
        addAndMakeVisible (tabs);
        tabs.onSelect = [this] (int i) { select (i); };
        addAndMakeVisible (editor);
        addAndMakeVisible (wobble);
        wobble.setTooltip ("Wobble Builder: classic wobble rates and shapes");
        wobble.onClick = [this]
        {
            juce::PopupMenu m;
            int id = 1;
            for (auto& w : shapes::wobblePresets())
                m.addItem (id++, w.name);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&wobble), [this] (int r) { applyWobble (r - 1); });
        };
        editor.onMenuResult = [this] (int r)
        {
            if (r >= 100 && r < 200)
            {
                proc.setLfoPoints (current, shapes::lfoPreset (r - 100));
                setParam ("shape", 11.0f);
            }
            else if (r >= 200) applyWobble (r - 200);
        };
        editor.addMenuItems = [] (juce::PopupMenu& m)
        {
            juce::PopupMenu shapesMenu, wob;
            const auto& names = params::lfoShapes();
            for (int i = 0; i < names.size() - 1; ++i) if (i != 6 && i != 7) shapesMenu.addItem (100 + i, names[i]);
            int k = 0;
            for (auto& w : shapes::wobblePresets()) wob.addItem (200 + k++, w.name);
            m.addSubMenu ("Load shape", shapesMenu);
            m.addSubMenu ("Wobble Builder", wob);
        };
        select (0);
    }

    void setParam (const juce::String& id, float real)
    {
        if (auto* prm = proc.apvts.getParameter (params::lfoPrefix (current) + id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (real));
    }

    void applyWobble (int index)
    {
        const auto& w = shapes::wobblePresets();
        if (! juce::isPositiveAndBelow (index, (int) w.size())) return;
        proc.setLfoPoints (current, w[(size_t) index].points);
        setParam ("shape", 11.0f);
        setParam ("sync", 1.0f);
        setParam ("sync_rate", (float) w[(size_t) index].syncRate);
        select (current);
    }

    void select (int l)
    {
        current = l;
        controls.clear();
        const auto pre = params::lfoPrefix (l);
        rate = new Knob (proc, pre + "rate", "Rate", theme::get().magenta);
        phase = new Knob (proc, pre + "phase", "Phase", theme::get().magenta);
        smooth = new Knob (proc, pre + "smooth", "Smooth", theme::get().magenta);
        rise = new Knob (proc, pre + "rise", "Rise", theme::get().magenta);
        sync = new ParamToggle (proc, pre + "sync", "Sync");
        bip = new ParamToggle (proc, pre + "bipolar", "Bipolar");
        syncRate = new ParamChoice (proc, pre + "sync_rate");
        mode = new ParamChoice (proc, pre + "mode");
        shape = new ParamChoice (proc, pre + "shape");
        chip = new ModSourceChip ((int) ModSource::Lfo1 + l, "LFO " + juce::String (l + 1));
        for (auto* c : std::initializer_list<juce::Component*> { rate, phase, smooth, rise, sync, bip, syncRate, mode, shape, chip })
        {
            controls.add (c);
            addAndMakeVisible (c);
        }
        editor.getPoints = [this, l]
        {
            const int sh = (int) proc.apvts.getRawParameterValue (params::lfoPrefix (l) + "shape")->load();
            return sh == 11 ? proc.getLfoPoints (l) : shapes::lfoPreset (sh);
        };
        editor.setPoints = [this, l] (const CurvePoints& pts)
        {
            proc.setLfoPoints (l, pts);
            if ((int) proc.apvts.getRawParameterValue (params::lfoPrefix (l) + "shape")->load() != 11)
                setParam ("shape", 11.0f);
        };
        editor.getPlayhead = [this, l] { return proc.engine.uiActiveVoices.load() > 0 ? proc.engine.uiLfoPhase[l].load() : -1.0f; };
        title = "LFO " + juce::String (l + 1);
        resized();
        repaint();
    }

    void resized() override
    {
        auto r = content();
        tabs.setBounds (getLocalBounds().reduced (6).removeFromTop (18).withTrimmedLeft (60));
        if (rate == nullptr) return;
        auto side = r.removeFromRight (190);
        auto row = side.removeFromTop (20);
        sync->setBounds (row.removeFromLeft (58));
        syncRate->setBounds (row);
        side.removeFromTop (3);
        row = side.removeFromTop (20);
        mode->setBounds (row.removeFromLeft (95));
        shape->setBounds (row.withTrimmedLeft (3));
        side.removeFromTop (3);
        row = side.removeFromTop (20);
        bip->setBounds (row.removeFromLeft (76));
        chip->setBounds (row.removeFromLeft (56).reduced (0, 2));
        wobble.setBounds (row.withTrimmedLeft (3));
        layoutRow (side, { rate, phase, smooth, rise }, 2);
        editor.setBounds (r.reduced (0, 2).withTrimmedRight (6));
    }

    BassGodsProcessor& proc;
    juce::OwnedArray<juce::Component> owned, controls;
    TabStrip tabs;
    CurveEditor editor;
    juce::TextButton wobble { "Wobble..." };
    Knob *rate = nullptr, *phase = nullptr, *smooth = nullptr, *rise = nullptr;
    ParamToggle *sync = nullptr, *bip = nullptr;
    ParamChoice *syncRate = nullptr, *mode = nullptr, *shape = nullptr;
    ModSourceChip* chip = nullptr;
    int current = 0;
};

// ============================================================================
/** 64-slot matrix shown as an editable table. */
struct MatrixTable : public juce::Component, private juce::Timer
{
    explicit MatrixTable (BassGodsProcessor& p) : proc (p) { startTimerHz (10); }
    static constexpr int rowH = 19;
    enum Col { Num, Src, Aux, Dst, Amt, Curve, Out, Bip, Del, NumCols };
    float colX[NumCols + 1] {};

    void timerCallback() override
    {
        const int v = proc.modMatrix.changeCounter.load();
        if (v != lastVersion) { lastVersion = v; updateSize(); repaint(); }
    }

    std::vector<int> rows() const
    {
        std::vector<int> r;
        for (int i = 0; i < kNumModSlots; ++i)
        {
            const auto s = proc.modMatrix.getSlot (i);
            if (s.src > 0 || s.dst >= 0) r.push_back (i);
        }
        r.push_back (-1);   // "add" row
        return r;
    }

    void updateSize() { setSize (getWidth(), std::max (getParentHeight(), (int) rows().size() * rowH + 2)); }

    void computeCols()
    {
        const float w = (float) getWidth();
        const float frac[NumCols] = { 0.05f, 0.17f, 0.13f, 0.27f, 0.12f, 0.08f, 0.08f, 0.05f, 0.05f };
        colX[0] = 0.0f;
        for (int c = 0; c < NumCols; ++c) colX[c + 1] = colX[c] + frac[c] * w;
    }

    void paint (juce::Graphics& g) override
    {
        computeCols();
        const auto& p = theme::get();
        const auto rs = rows();
        const auto& dests = params::modDestinations();
        g.setFont (theme::font (11.0f));
        for (size_t ri = 0; ri < rs.size(); ++ri)
        {
            const float y = (float) ri * rowH;
            const int slot = rs[ri];
            g.setColour (ri % 2 == 0 ? p.panelHi.withAlpha (0.5f) : p.panel.withAlpha (0.5f));
            g.fillRect (0.0f, y, (float) getWidth(), (float) rowH - 1.0f);
            auto cell = [&] (int c) { return juce::Rectangle<float> (colX[c] + 3.0f, y, colX[c + 1] - colX[c] - 4.0f, (float) rowH - 1.0f); };
            if (slot < 0)
            {
                g.setColour (p.textDim);
                g.drawText ("+ click to add a routing (or drag a source chip onto a knob)", cell (Src).withRight ((float) getWidth()), juce::Justification::centredLeft);
                continue;
            }
            const auto s = proc.modMatrix.getSlot (slot);
            g.setColour (p.textDim);
            g.drawText (juce::String (slot + 1), cell (Num), juce::Justification::centred);
            g.setColour (theme::modSourceColour (s.src));
            g.drawText (params::modSourceNames()[s.src], cell (Src), juce::Justification::centredLeft);
            g.setColour (s.aux > 0 ? theme::modSourceColour (s.aux) : p.textDim);
            g.drawText (s.aux > 0 ? params::modSourceNames()[s.aux] : juce::String ("-"), cell (Aux), juce::Justification::centredLeft);
            g.setColour (p.text);
            g.drawText (s.dst >= 0 ? dests[(size_t) s.dst].displayName : juce::String ("(choose)"), cell (Dst), juce::Justification::centredLeft);
            auto bar = [&] (int c, float v, bool bip)
            {
                auto b = cell (c).reduced (0.0f, 5.0f);
                g.setColour (p.track);
                g.fillRect (b);
                g.setColour (p.cyan);
                if (bip) { const float mid = b.getCentreX(); g.fillRect (juce::Rectangle<float> (std::min (mid, mid + v * b.getWidth() * 0.5f), b.getY(), std::abs (v) * b.getWidth() * 0.5f, b.getHeight())); }
                else g.fillRect (b.withWidth (b.getWidth() * v));
                g.setColour (p.text);
                g.setFont (theme::font (9.5f));
                g.drawText (juce::String (juce::roundToInt (v * 100.0f)), b.expanded (0, 5), juce::Justification::centred);
                g.setFont (theme::font (11.0f));
            };
            bar (Amt, s.amount, true);
            bar (Curve, s.curve, true);
            bar (Out, s.output, false);
            g.setColour (s.bipolar ? p.yellow : p.textDim);
            g.drawText (s.bipolar ? "BI" : "UNI", cell (Bip), juce::Justification::centred);
            g.setColour (p.warn);
            g.drawText ("X", cell (Del), juce::Justification::centred);
        }
    }

    int colAt (float x) const { for (int c = 0; c < NumCols; ++c) if (x < colX[c + 1]) return c; return NumCols - 1; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        computeCols();
        const auto rs = rows();
        const int ri = e.y / rowH;
        if (ri < 0 || ri >= (int) rs.size()) return;
        int slot = rs[(size_t) ri];
        const int c = colAt ((float) e.x);
        if (slot < 0)
        {
            slot = proc.modMatrix.findFreeSlot();
            if (slot < 0) return;
            ModSlot s; s.src = (int) ModSource::Lfo1; s.dst = -1; s.amount = 0.3f;
            proc.modMatrix.setSlot (slot, s);
            return;
        }
        dragSlot = slot; dragCol = c;
        dragStart = proc.modMatrix.getSlot (slot);
        if (c == Src || c == Aux)
        {
            juce::PopupMenu m;
            for (int i = (c == Src ? 1 : 0); i < (int) ModSource::Count; ++i) m.addItem (i + 1, params::modSourceNames()[i]);
            m.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [this, slot, c] (int r)
            {
                if (r <= 0) return;
                auto s = proc.modMatrix.getSlot (slot);
                (c == Src ? s.src : s.aux) = r - 1;
                proc.modMatrix.setSlot (slot, s);
            });
        }
        else if (c == Dst)
        {
            juce::PopupMenu m, voice, global;
            const auto& d = params::modDestinations();
            juce::PopupMenu groups[8];
            const char* groupNames[] = { "Osc A", "Osc B", "Osc C", "Osc D", "Filters", "Envelopes", "LFOs", "Voice" };
            for (int i = 0; i < (int) d.size(); ++i)
            {
                const auto& id = d[(size_t) i].paramID;
                int gi = 7;
                if (id.startsWith ("oA_")) gi = 0; else if (id.startsWith ("oB_")) gi = 1; else if (id.startsWith ("oC_")) gi = 2;
                else if (id.startsWith ("oD_")) gi = 3; else if (id.startsWith ("f1_") || id.startsWith ("f2_")) gi = 4;
                else if (id.startsWith ("e")) gi = 5; else if (id.startsWith ("l")) gi = 6;
                if (! d[(size_t) i].perVoice) global.addItem (i + 1, d[(size_t) i].displayName);
                else groups[gi].addItem (i + 1, d[(size_t) i].displayName);
            }
            for (int gi = 0; gi < 8; ++gi) m.addSubMenu (groupNames[gi], groups[gi]);
            m.addSubMenu ("Global / FX", global);
            m.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [this, slot] (int r)
            {
                if (r <= 0) return;
                auto s = proc.modMatrix.getSlot (slot);
                s.dst = r - 1;
                proc.modMatrix.setSlot (slot, s);
            });
        }
        else if (c == Bip) { auto s = proc.modMatrix.getSlot (slot); s.bipolar = ! s.bipolar; proc.modMatrix.setSlot (slot, s); }
        else if (c == Del) proc.modMatrix.clearSlot (slot);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragSlot < 0 || (dragCol != Amt && dragCol != Curve && dragCol != Out)) return;
        auto s = dragStart;
        const float d = (float) (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY()) / 150.0f;
        if (dragCol == Amt) s.amount = juce::jlimit (-1.0f, 1.0f, dragStart.amount + d);
        if (dragCol == Curve) s.curve = juce::jlimit (-1.0f, 1.0f, dragStart.curve + d);
        if (dragCol == Out) s.output = juce::jlimit (0.0f, 1.0f, dragStart.output + d);
        proc.modMatrix.setSlot (dragSlot, s);
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int c = colAt ((float) e.x);
        if (dragSlot < 0) return;
        auto s = proc.modMatrix.getSlot (dragSlot);
        if (c == Amt) s.amount = 0.0f;
        if (c == Curve) s.curve = 0.0f;
        if (c == Out) s.output = 1.0f;
        proc.modMatrix.setSlot (dragSlot, s);
    }

    void mouseUp (const juce::MouseEvent&) override { dragCol = -1; }

    BassGodsProcessor& proc;
    int lastVersion = -1, dragSlot = -1, dragCol = -1;
    ModSlot dragStart;
};

struct MatrixPanel : public Panel
{
    MatrixPanel (BassGodsProcessor& p, std::function<void()> onCollapse) : Panel ("MOD MATRIX", theme::get().yellow), proc (p), table (p)
    {
        addAndMakeVisible (viewport);
        viewport.setViewedComponent (&table, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (collapse);
        collapse.setTooltip ("Collapse / expand the matrix");
        collapse.onClick = [onCollapse] { if (onCollapse) onCollapse(); };
        const ModSource extra[] = { ModSource::Velocity, ModSource::NoteRandom, ModSource::KeyTrack, ModSource::ModWheel, ModSource::PitchBend,
                                    ModSource::Aftertouch, ModSource::MpeSlide, ModSource::MpePressure, ModSource::Noise };
        const char* names[] = { "VEL", "RND", "KEY", "MW", "PB", "AT", "SLD", "PRS", "NSE" };
        for (int i = 0; i < 9; ++i)
        {
            auto* c = chips.add (new ModSourceChip ((int) extra[i], names[i]));
            addAndMakeVisible (c);
        }
    }
    void setCollapsed (bool c)
    {
        collapsed = c;
        viewport.setVisible (! c);
        for (auto* ch : chips) ch->setVisible (! c);
        collapse.setButtonText (c ? "<" : ">");
        title = c ? "" : "MOD MATRIX";
        resized();
        repaint();
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        auto top = r.removeFromTop (18);
        collapse.setBounds (top.removeFromRight (22));
        if (collapsed) return;
        top.removeFromLeft (100);
        const int w = std::max (30, top.getWidth() / 9);
        for (auto* c : chips) c->setBounds (top.removeFromLeft (w).reduced (1, 1));
        r.removeFromTop (4);
        auto header = r.removeFromTop (14);
        headerArea = header;
        viewport.setBounds (r);
        table.setSize (r.getWidth() - viewport.getScrollBarThickness(), std::max (r.getHeight(), 20));
        table.updateSize();
    }
    void paintOverChildren (juce::Graphics& g) override
    {
        if (collapsed) return;
        table.computeCols();
        const auto& p = theme::get();
        g.setColour (p.textDim);
        g.setFont (theme::font (9.5f, true));
        const char* heads[] = { "#", "SOURCE", "AUX", "DESTINATION", "AMOUNT", "CURVE", "OUTPUT", "POL", "" };
        for (int c = 0; c < MatrixTable::NumCols; ++c)
            g.drawText (heads[c], juce::Rectangle<float> ((float) headerArea.getX() + table.colX[c] + 3.0f, (float) headerArea.getY(), table.colX[c + 1] - table.colX[c], 14.0f), juce::Justification::centredLeft);
    }
    BassGodsProcessor& proc;
    MatrixTable table;
    juce::Viewport viewport;
    juce::TextButton collapse { ">" };
    juce::OwnedArray<ModSourceChip> chips;
    juce::Rectangle<int> headerArea;
    bool collapsed = false;
};

// ============================================================================
struct PlayPanel : public Panel
{
    explicit PlayPanel (BassGodsProcessor& p) : Panel ({}, theme::get().pink), proc (p), keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        voiceMode = add<ParamChoice> (*this, owned, proc, "poly_mode");
        voices = add<Knob> (*this, owned, proc, "poly_voices", "Voices", theme::get().pink);
        porta = add<Knob> (*this, owned, proc, "porta_time", "Glide", theme::get().pink);
        portaMode = add<ParamChoice> (*this, owned, proc, "porta_mode");
        portaType = add<ParamChoice> (*this, owned, proc, "porta_type");
        pbRange = add<Knob> (*this, owned, proc, "pb_range", "PB Rng", theme::get().pink);
        monoOn = add<ParamToggle> (*this, owned, proc, "bassmono_on", "Bass Mono");
        monoFreq = add<Knob> (*this, owned, proc, "bassmono_freq", "Below", theme::get().teal);

        for (auto* s : { &wheel, &bend, &velocity })
        {
            addAndMakeVisible (s);
            s->setSliderStyle (juce::Slider::LinearVertical);
            s->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        }
        wheel.setRange (0, 127, 1);
        wheel.setTooltip ("Mod wheel (CC1)");
        wheel.onValueChange = [this] { proc.sendUiMidi (juce::MidiMessage::controllerEvent (1, 1, (int) wheel.getValue())); };
        bend.setRange (-8192, 8191, 1);
        bend.setValue (0, juce::dontSendNotification);
        bend.setTooltip ("Pitch bend (springs back)");
        bend.onValueChange = [this] { proc.sendUiMidi (juce::MidiMessage::pitchWheel (1, (int) bend.getValue() + 8192)); };
        bend.onDragEnd = [this] { bend.setValue (0); };
        velocity.setRange (1, 127, 1);
        velocity.setValue (100, juce::dontSendNotification);
        velocity.setTooltip ("On-screen keyboard velocity");
        velocity.onValueChange = [this] { keyboard.setVelocity ((float) velocity.getValue() / 127.0f, false); };
        keyboard.setVelocity (100.0f / 127.0f, false);
        addAndMakeVisible (keyboard);
        keyboard.setAvailableRange (12, 96);
        keyboard.setLowestVisibleKey (24);
        keyboard.setKeyWidth (18.0f);
        keyboard.setOctaveForMiddleC (4);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        auto left = r.removeFromLeft (330);
        auto col = left.removeFromLeft (100);
        voiceMode->setBounds (col.removeFromTop (20));
        col.removeFromTop (3);
        portaMode->setBounds (col.removeFromTop (20));
        col.removeFromTop (3);
        portaType->setBounds (col.removeFromTop (20));
        left.removeFromLeft (4);
        auto toggles = left.removeFromBottom (18);
        monoOn->setBounds (toggles);
        layoutRow (left, { voices, porta, pbRange, monoFreq }, 2);
        r.removeFromLeft (6);
        wheel.setBounds (r.removeFromLeft (22));
        bend.setBounds (r.removeFromLeft (22));
        velocity.setBounds (r.removeFromLeft (22));
        r.removeFromLeft (6);
        keyboard.setBounds (r);
    }
    void paintOverChildren (juce::Graphics& g) override
    {
        g.setColour (theme::get().textDim);
        g.setFont (theme::font (9.0f, true));
        g.drawText ("MW", wheel.getBounds().translated (0, -2).removeFromTop (10), juce::Justification::centred);
        g.drawText ("PB", bend.getBounds().translated (0, -2).removeFromTop (10), juce::Justification::centred);
        g.drawText ("VEL", velocity.getBounds().translated (0, -2).removeFromTop (10), juce::Justification::centred);
    }
    BassGodsProcessor& proc;
    juce::OwnedArray<juce::Component> owned;
    ParamChoice *voiceMode, *portaMode, *portaType;
    Knob *voices, *porta, *pbRange, *monoFreq;
    ParamToggle* monoOn;
    juce::Slider wheel, bend, velocity;
    juce::MidiKeyboardComponent keyboard;
};
} // namespace

// ============================================================================
SynthTab::SynthTab (BassGodsProcessor& p) : proc (p)
{
    scene = std::make_unique<ClubScene> (proc);
    addAndMakeVisible (*scene);

    macroLeft = add<MacroPanel> (*this, panels, proc, 0, *scene, true);
    macroRight = add<MacroPanel> (*this, panels, proc, 2, *scene, false);
    for (int o = 0; o < kNumWtOscs; ++o)
        oscRow[o] = add<OscPanel> (*this, panels, proc, o, [this] (int osc) { if (onEditWavetable) onEditWavetable (osc); });
    oscRow[3] = add<OscDPanel> (*this, panels, proc);
    filterPanel = add<FilterPanel> (*this, panels, proc);
    envPanel = add<EnvPanel> (*this, panels, proc);
    lfoPanel = add<LfoPanel> (*this, panels, proc);
    matrixPanel = add<MatrixPanel> (*this, panels, proc, [this]
    {
        matrixCollapsed = ! matrixCollapsed;
        static_cast<MatrixPanel*> (matrixPanel)->setCollapsed (matrixCollapsed);
        resized();
    });
    playPanel = add<PlayPanel> (*this, panels, proc);
}

SynthTab::~SynthTab() = default;

void SynthTab::paint (juce::Graphics& g)
{
    g.fillAll (theme::get().bg);
}

void SynthTab::layoutScene (bool expanded, juce::Rectangle<int> fullArea)
{
    if (expanded)
    {
        scene->setBounds (fullArea);
        scene->toFront (false);
    }
    else
        resized();
}

void SynthTab::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto top = r.removeFromTop (226);
    macroLeft->setBounds (top.removeFromLeft (200));
    macroRight->setBounds (top.removeFromRight (250));
    if (! scene->isExpanded())
        scene->setBounds (top.reduced (6, 0));
    r.removeFromTop (6);

    auto play = r.removeFromBottom (92);
    playPanel->setBounds (play);
    r.removeFromBottom (6);

    auto mod = r.removeFromBottom (190);
    envPanel->setBounds (mod.removeFromLeft (330));
    mod.removeFromLeft (6);
    matrixPanel->setBounds (mod.removeFromRight (matrixCollapsed ? 34 : 470));
    mod.removeFromRight (6);
    lfoPanel->setBounds (mod);
    r.removeFromBottom (6);

    auto osc = r;
    for (int o = 0; o < 3; ++o)
    {
        oscRow[o]->setBounds (osc.removeFromLeft (238));
        osc.removeFromLeft (5);
    }
    oscRow[3]->setBounds (osc.removeFromLeft (150));
    osc.removeFromLeft (5);
    filterPanel->setBounds (osc);
}
} // namespace bgdj
