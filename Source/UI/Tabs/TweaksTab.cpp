#include "TweaksTab.h"
#include "../../PluginProcessor.h"

namespace bgdj
{
juce::Component* TweaksTab::addItem (Group& g, juce::Component* c, const juce::String& label)
{
    owned.add (c);
    addAndMakeVisible (c);
    g.items.push_back (c);
    g.labels.push_back (label);
    return c;
}

TweaksTab::TweaksTab (BassGodsProcessor& p) : proc (p)
{
    auto choice = [this] (Group& g, const char* id, const char* label) { addItem (g, new ParamChoice (proc, id), label); };
    auto toggle = [this] (Group& g, const juce::String& id, const juce::String& label) { addItem (g, new ParamToggle (proc, id, label), {}); };
    auto knob = [this] (Group& g, const char* id, const char* label) { addItem (g, new Knob (proc, id, label, theme::get().teal), {}); };

    groups.resize (6);
    groups[0].title = "OVERSAMPLING & QUALITY";
    choice (groups[0], "os_synth", "Synth (oscillators + filters)");
    choice (groups[0], "os_fx", "FX (distortion / saturator)");
    choice (groups[0], "os_offline", "Offline render override");
    choice (groups[0], "wt_interp", "Wavetable interpolation");
    choice (groups[0], "aa_quality", "Anti-aliasing quality");
    toggle (groups[0], "denormal_ftz", "Flush denormals to zero");
    toggle (groups[0], "dc_blocker", "DC blocker on synth output");

    groups[1].title = "UNISON & VOICES";
    choice (groups[1], "uni_curve", "Unison detune curve");
    choice (groups[1], "uni_phase", "Unison phase mode");
    choice (groups[1], "voice_steal", "Voice stealing");
    choice (groups[1], "env_retrig", "Envelope retrigger");
    toggle (groups[1], "legato_env_reset", "Legato resets envelopes");
    for (int o = 0; o < kNumWtOscs; ++o)
        toggle (groups[1], params::oscPrefix (o) + "phase_reset", "Phase reset on note: Osc " + juce::String::charToString ((juce::juce_wchar) ('A' + o)));
    toggle (groups[1], "oD_phase_reset", "Phase reset on note: Osc D");

    groups[2].title = "PITCH & TUNING";
    knob (groups[2], "tune_a4", "A4 Hz");
    knob (groups[2], "drift", "Drift");
    knob (groups[2], "pb_range", "PB Range");
    addItem (groups[2], &tuningName, "Microtuning");
    owned.removeObject (&tuningName, false);
    addItem (groups[2], &loadTuning, {});
    owned.removeObject (&loadTuning, false);
    addItem (groups[2], &resetTuning, {});
    owned.removeObject (&resetTuning, false);
    loadTuning.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load microtuning", juce::File(), "*.scl;*.tun");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            if (! fc.getResult().existsAsFile()) return;
            juce::String err;
            if (! proc.loadTuningFile (fc.getResult(), err))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Tuning", err);
        });
    };
    resetTuning.onClick = [this] { proc.resetTuning(); };

    groups[3].title = "VELOCITY & MPE";
    velView = std::make_unique<VelocityCurveView> (proc);
    addAndMakeVisible (*velView);
    knob (groups[3], "amp_vel", "Vel>Amp");
    toggle (groups[3], "mpe_on", "MPE enabled");
    choice (groups[3], "mpe_zone", "MPE zone");
    knob (groups[3], "mpe_members", "Members");
    knob (groups[3], "mpe_pb_range", "MPE PB");

    groups[4].title = "FILTERS";
    knob (groups[4], "filt_key_ref", "Key Ref");
    choice (groups[4], "filt_sat", "Filter saturation algorithm");

    groups[5].title = "RANDOMNESS & FUN";
    toggle (groups[5], "seed_lock", "Random seed lock (reproducible random phase)");
    knob (groups[5], "seed_value", "Seed");
    toggle (groups[5], "cat_chaos", "Cat Chaos Mode (Grey really bumps your macros)");
    toggle (groups[5], "crowd_sfx_out", "Crowd SFX to output (off = never printed)");
    knob (groups[5], "crowd_sfx_level", "SFX Lvl");

    tuningName.setFont (theme::font (12.0f));
    startTimerHz (4);
}

void TweaksTab::timerCallback()
{
    tuningName.setText ("Tuning: " + proc.getTuningName(), juce::dontSendNotification);
}

void TweaksTab::resized()
{
    auto area = getLocalBounds().reduced (10);
    const int colW = area.getWidth() / 3;
    juce::Rectangle<int> cols[3] = { area.removeFromLeft (colW), area.removeFromLeft (colW), area };
    const int placement[6] = { 0, 0, 1, 1, 2, 2 };
    const int heights[6] = { 330, 350, 260, 360, 150, 430 };
    for (int gi = 0; gi < 6; ++gi)
    {
        auto& g = groups[(size_t) gi];
        auto& col = cols[placement[gi]];
        g.bounds = col.removeFromTop (std::min (heights[gi], col.getHeight())).reduced (5);
        col.removeFromTop (6);
        auto r = g.bounds.reduced (10).withTrimmedTop (18);
        if (gi == 3)
            velView->setBounds (r.removeFromTop (110));

        int knobX = r.getX();
        int y = r.getY();
        for (size_t i = 0; i < g.items.size(); ++i)
        {
            auto* c = g.items[i];
            if (dynamic_cast<Knob*> (c) != nullptr)
            {
                if (knobX + 70 > r.getRight()) { knobX = r.getX(); y += 76; }
                c->setBounds (knobX, y, 70, 72);
                knobX += 74;
                if (i + 1 == g.items.size() || dynamic_cast<Knob*> (g.items[i + 1]) == nullptr) { y += 78; knobX = r.getX(); }
            }
            else if (dynamic_cast<juce::ComboBox*> (c) != nullptr)
            {
                c->setBounds (r.getX() + r.getWidth() / 2, y, r.getWidth() / 2, 24);
                y += 30;
            }
            else
            {
                c->setBounds (r.getX(), y, r.getWidth(), 24);
                y += 28;
            }
        }
    }
}

void TweaksTab::paint (juce::Graphics& g)
{
    g.fillAll (theme::get().bg);
    for (auto& grp : groups)
    {
        drawPanel (g, grp.bounds.toFloat(), grp.title, theme::get().teal);
        g.setColour (theme::get().textDim);
        g.setFont (theme::font (12.0f));
        for (size_t i = 0; i < grp.items.size(); ++i)
            if (dynamic_cast<juce::ComboBox*> (grp.items[i]) != nullptr)
                g.drawText (grp.labels[i], grp.items[i]->getBounds().withX (grp.bounds.getX() + 10).withWidth (grp.items[i]->getX() - grp.bounds.getX() - 14), juce::Justification::centredLeft);
    }
}
} // namespace bgdj
