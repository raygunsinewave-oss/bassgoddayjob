#include "Header.h"
#include "../PluginProcessor.h"
#include "../Presets/PresetManager.h"

namespace bgdj
{
Header::Header (BassGodsProcessor& p) : proc (p), master (p, "master_vol", "Master", theme::get().pink)
{
    for (auto* b : { &prevBtn, &nextBtn, &favBtn, &saveBtn, &aBtn, &bBtn, &copyBtn, &browseBtn })
        addAndMakeVisible (b);
    addAndMakeVisible (master);
    prevBtn.setTooltip ("Previous preset");
    nextBtn.setTooltip ("Next preset");
    favBtn.setTooltip ("Favourite (star) this preset");
    saveBtn.setTooltip ("Save as a user preset with tags");
    aBtn.setTooltip ("A/B compare: slot A");
    bBtn.setTooltip ("A/B compare: slot B");
    copyBtn.setTooltip ("Copy the current slot to the other one");
    browseBtn.setTooltip ("Open the preset browser");

    auto list = [this] { return getBrowseList ? getBrowseList() : std::vector<int>(); };
    prevBtn.onClick = [this, list] { auto l = list(); if (l.empty()) { for (int i = 0; i < (int) proc.presets->all().size(); ++i) l.push_back (i); } proc.presets->step (-1, l); };
    nextBtn.onClick = [this, list] { auto l = list(); if (l.empty()) { for (int i = 0; i < (int) proc.presets->all().size(); ++i) l.push_back (i); } proc.presets->step (1, l); };
    favBtn.onClick = [this] { proc.presets->toggleFavourite (proc.presets->getCurrentIndex()); };
    saveBtn.onClick = [this] { showSaveDialog(); };
    aBtn.onClick = [this] { if (proc.presets->abActive() != 0) proc.presets->abToggle(); };
    bBtn.onClick = [this] { if (proc.presets->abActive() != 1) proc.presets->abToggle(); };
    copyBtn.onClick = [this] { proc.presets->abCopyAtoB(); };
    browseBtn.onClick = [this] { if (onBrowse) onBrowse(); };
    aBtn.setClickingTogglesState (false);
    proc.presets->addChangeListener (this);
    startTimerHz (4);
}

Header::~Header() { proc.presets->removeChangeListener (this); }

void Header::timerCallback()
{
    cpuText = "CPU " + juce::String (proc.loadMeasurer.getLoadAsPercentage(), 1) + "%";
    aBtn.setToggleState (proc.presets->abActive() == 0, juce::dontSendNotification);
    bBtn.setToggleState (proc.presets->abActive() == 1, juce::dontSendNotification);
    favBtn.setToggleState (proc.presets->isFavourite (proc.presets->getCurrentIndex()), juce::dontSendNotification);
    favBtn.setButtonText (favBtn.getToggleState() ? juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85")) : juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x86")));
    if (proc.presetVersion.load() != lastPresetVersion) lastPresetVersion = proc.presetVersion.load();
    repaint();
}

juce::Rectangle<int> Header::nameArea() const
{
    return { prevBtn.getRight() + 4, 8, nextBtn.getX() - prevBtn.getRight() - 8, getHeight() - 16 };
}

void Header::resized()
{
    auto r = getLocalBounds().reduced (8, 6);
    r.removeFromLeft (300);   // logo
    master.setBounds (r.removeFromRight (56));
    r.removeFromRight (6);
    auto cpu = r.removeFromRight (90);
    juce::ignoreUnused (cpu);
    r.removeFromRight (6);
    copyBtn.setBounds (r.removeFromRight (42).reduced (0, 6));
    bBtn.setBounds (r.removeFromRight (30).reduced (0, 6));
    aBtn.setBounds (r.removeFromRight (30).reduced (0, 6));
    r.removeFromRight (8);
    saveBtn.setBounds (r.removeFromRight (60).reduced (0, 6));
    browseBtn.setBounds (r.removeFromRight (76).reduced (0, 6));
    r.removeFromRight (8);
    favBtn.setBounds (r.removeFromRight (30).reduced (0, 6));
    nextBtn.setBounds (r.removeFromRight (28).reduced (0, 6));
    prevBtn.setBounds (r.removeFromLeft (28).reduced (0, 6));
}

void Header::paint (juce::Graphics& g)
{
    const auto& p = theme::get();
    auto r = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (p.panelHi, 0, 0, p.bg, 0, r.getHeight(), false));
    g.fillRect (r);
    g.setColour (p.magenta.withAlpha (0.5f));
    g.drawHorizontalLine (getHeight() - 1, 0.0f, r.getWidth());

    // neon logo
    auto logo = juce::Rectangle<float> (12.0f, 4.0f, 290.0f, r.getHeight() - 8.0f);
    g.setFont (theme::font (26.0f, true));
    for (float glow : { 6.0f, 3.0f })
    {
        g.setColour (p.magenta.withAlpha (0.18f));
        g.drawText ("BASSGODSDAYJOB", logo.translated (0, glow * 0.3f).expanded (glow * 0.3f, 0), juce::Justification::centredLeft);
    }
    juce::ColourGradient lg (p.pink, logo.getX(), 0, p.cyan, logo.getRight(), 0, false);
    g.setGradientFill (lg);
    g.drawText ("BASSGODSDAYJOB", logo, juce::Justification::centredLeft);

    // preset display
    auto na = nameArea().toFloat();
    g.setColour (p.bg);
    g.fillRoundedRectangle (na, 5.0f);
    g.setColour (p.outline);
    g.drawRoundedRectangle (na, 5.0f, 1.0f);
    g.setColour (p.text);
    g.setFont (theme::font (16.0f, true));
    g.drawText (proc.currentPresetName, na.reduced (10, 0).withTrimmedBottom (12), juce::Justification::centredLeft);
    const int idx = proc.presets->getCurrentIndex();
    juce::String sub;
    if (juce::isPositiveAndBelow (idx, (int) proc.presets->all().size()))
    {
        const auto& d = proc.presets->all()[(size_t) idx].data;
        sub = d.bank + "  |  " + d.character + "  |  " + d.genre + (d.key.isNotEmpty() ? "  |  " + d.key : juce::String()) + (d.bpm > 0 ? "  |  " + juce::String (d.bpm) + " BPM" : juce::String());
    }
    g.setColour (p.textDim);
    g.setFont (theme::font (11.0f));
    g.drawText (sub, na.reduced (10, 2).withTrimmedTop (22), juce::Justification::centredLeft);

    // CPU
    auto cpu = juce::Rectangle<float> ((float) master.getX() - 96.0f, 10.0f, 90.0f, r.getHeight() - 20.0f);
    const float load = (float) proc.loadMeasurer.getLoadAsPercentage();
    g.setColour (load > 70.0f ? p.warn : (load > 40.0f ? p.yellow : p.good));
    g.setFont (theme::mono (12.0f));
    g.drawText (cpuText, cpu, juce::Justification::centred);
}

void Header::mouseUp (const juce::MouseEvent& e)
{
    if (nameArea().contains (e.getPosition()) && onBrowse)
        onBrowse();
}

void Header::showSaveDialog()
{
    auto* w = new juce::AlertWindow ("Save Preset", "Save the current sound as a user preset.", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", proc.currentPresetName, "Name");
    w->addTextEditor ("bank", "USER", "Bank");
    juce::StringArray chars { "sub", "reese", "growl", "pluck", "wobble", "acid", "organ", "FM", "texture" };
    w->addComboBox ("character", chars, "Character");
    w->addTextEditor ("genre", "", "Genre");
    w->addTextEditor ("key", "", "Key (e.g. F#)");
    w->addTextEditor ("bpm", "", "BPM hint");
    w->addTextEditor ("tags", "", "Extra tags (comma separated)");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result)
    {
        if (result == 1)
        {
            juce::StringArray tags;
            tags.addTokens (w->getTextEditorContents ("tags"), ",", "");
            tags.trim();
            tags.removeEmptyStrings();
            const auto err = proc.presets->saveUser (w->getTextEditorContents ("name"), w->getTextEditorContents ("bank"),
                                                     w->getComboBoxComponent ("character")->getText(), w->getTextEditorContents ("genre"),
                                                     tags, w->getTextEditorContents ("key"), w->getTextEditorContents ("bpm").getIntValue());
            if (err.isNotEmpty())
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Save failed", err);
        }
    }), true);
}
} // namespace bgdj
