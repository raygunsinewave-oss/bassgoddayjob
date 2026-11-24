#include "SettingsTab.h"
#include "../../PluginProcessor.h"
#include "../../Presets/PresetManager.h"
#include "../../Presets/ExpansionManager.h"
#include "../../Settings/Diagnostics.h"

namespace bgdj
{
SettingsTab::SettingsTab (BassGodsProcessor& p) : proc (p)
{
    addAndMakeVisible (viewport);
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (true, false);
    auto& S = *proc.settings;

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &uiScale, &themeBox, &sceneQuality, &knobMode, &fineMod, &sceneOn, &sfxOn, &tooltips, &crt, &djName, &saveCc, &loadCc, &clearCc,
             &presetFolder, &extraFolders, &wtFolder, &noiseFolder, &changePreset, &addFolder, &clearFolders, &changeWt, &changeNoise, &rescan,
             &installPackBtn, &expansionList, &resetUi, &clearCaches, &factoryReset, &exportDiag, &resetXruns, &checkUpdates,
             &softwareRender, &safeMode, &autoSave, &crashReports, &analyzers, &cpuLabel })
        content.addAndMakeVisible (c);

    // ---- basic ----------------------------------------------------------------------------------
    uiScale.setRange (50, 200, 5);
    uiScale.setTextValueSuffix (" %");
    uiScale.setSliderStyle (juce::Slider::LinearHorizontal);
    uiScale.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 22);
    uiScale.setValue (S.uiScale() * 100.0, juce::dontSendNotification);
    uiScale.onDragEnd = [this] { proc.settings->set ("uiScale", uiScale.getValue() / 100.0); };
    themeBox.addItemList (theme::names(), 1);
    themeBox.setText (S.theme(), juce::dontSendNotification);
    themeBox.onChange = [this] { proc.settings->set ("theme", themeBox.getText()); };
    sceneQuality.addItemList ({ "Low (40 dancers, 30 fps)", "Medium (90 dancers, 45 fps)", "High (150 dancers, 60 fps)" }, 1);
    sceneQuality.setSelectedItemIndex (S.sceneQuality(), juce::dontSendNotification);
    sceneQuality.onChange = [this] { proc.settings->set ("sceneQuality", sceneQuality.getSelectedItemIndex()); };
    knobMode.addItemList ({ "Circular drag", "Vertical drag", "Horizontal drag" }, 1);
    knobMode.setSelectedItemIndex (S.knobMode(), juce::dontSendNotification);
    knobMode.onChange = [this] { proc.settings->set ("knobMode", knobMode.getSelectedItemIndex()); };
    fineMod.addItemList ({ "Shift", "Ctrl / Cmd", "Alt / Option" }, 1);
    fineMod.setSelectedItemIndex (S.fineModifier(), juce::dontSendNotification);
    fineMod.onChange = [this] { proc.settings->set ("fineModifier", fineMod.getSelectedItemIndex()); };

    auto bindToggle = [this] (juce::ToggleButton& b, const juce::String& key, bool def)
    {
        b.setToggleState (proc.settings->getBool (key, def), juce::dontSendNotification);
        b.onClick = [this, &b, key] { proc.settings->set (key, b.getToggleState()); };
    };
    bindToggle (sceneOn, "sceneOn", true);
    bindToggle (sfxOn, "crowdSfx", true);
    bindToggle (tooltips, "tooltips", true);
    bindToggle (crt, "crtOverlay", false);
    bindToggle (softwareRender, "forceSoftware", false);
    bindToggle (autoSave, "autoSave", true);
    bindToggle (crashReports, "crashReports", false);
    analyzers.setToggleState (S.getBool ("analyzersOn", true), juce::dontSendNotification);
    analyzers.onClick = [this] { proc.settings->set ("analyzersOn", analyzers.getToggleState()); proc.analyzersEnabled = proc.settings->analyzersEnabled(); };
    safeMode.setToggleState (S.safeMode(), juce::dontSendNotification);
    safeMode.onClick = [this] { proc.applySafeMode (safeMode.getToggleState()); };

    djName.setText (S.getString ("djName"), juce::dontSendNotification);
    djName.setTextToShowWhenEmpty ("Default: " + juce::SystemStats::getLogonName(), theme::get().textDim);
    djName.onTextChange = [this] { proc.settings->set ("djName", djName.getText()); };

    saveCc.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Save MIDI CC map", AppSettings::getDataFolder().getChildFile ("cc_map.json"), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            if (fc.getResult() != juce::File())
                fc.getResult().withFileExtension ("json").replaceWithText (juce::JSON::toString (proc.midiMapToVar()));
        });
    };
    loadCc.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load MIDI CC map", AppSettings::getDataFolder(), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            if (fc.getResult().existsAsFile())
            {
                proc.midiMapFromVar (juce::JSON::parse (fc.getResult()));
                proc.settings->set ("midiMap", juce::JSON::toString (proc.midiMapToVar(), true));
            }
        });
    };
    clearCc.onClick = [this] { proc.midiMapFromVar ({}); proc.settings->set ("midiMap", "{}"); };

    // ---- advanced: content paths ------------------------------------------------------------------------
    changePreset.onClick = [this] { chooseFolder ("presetFolder", "Default preset folder"); };
    changeWt.onClick = [this] { chooseFolder ("wavetableFolder", "Wavetable folder"); };
    changeNoise.onClick = [this] { chooseFolder ("noiseFolder", "Noise folder"); };
    addFolder.onClick = [this] { chooseFolder ("extraPresetFolders", "Add a custom preset folder"); };
    clearFolders.onClick = [this] { proc.settings->set ("extraPresetFolders", ""); };
    rescan.onClick = [this]
    {
        proc.presets->rescan();
        NoiseBank::getInstance().rescan (proc.settings->noiseFolders());
        refreshExpansions();
    };

    installPackBtn.setTooltip ("Install an expansion pack (or drag a .bgdjpack file onto this tab)");
    installPackBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Install expansion", juce::File(), "*.bgdjpack");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            if (fc.getResult().existsAsFile()) installPack (fc.getResult());
        });
    };

    // ---- troubleshooting -----------------------------------------------------------------------------------
    resetUi.onClick = [this]
    {
        proc.settings->set ("uiScale", 1.0);
        proc.settings->set ("windowW", 0);
        proc.settings->set ("windowH", 0);
        uiScale.setValue (100, juce::dontSendNotification);
        if (onResetLayout) onResetLayout();
    };
    clearCaches.onClick = [this] { proc.presets->clearCachesAndRebuild(); };
    factoryReset.onClick = [this]
    {
        juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::WarningIcon, "Reset all settings",
            "Reset every BASSGODSDAYJOB setting to factory defaults? Presets, favourites and expansions are kept.", "Reset", "Cancel", nullptr,
            juce::ModalCallbackFunction::create ([this] (int r) { if (r == 1) { proc.settings->resetToFactory(); proc.applySafeMode (false); } }));
    };
    exportDiag.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Export diagnostics", juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                                                           .getChildFile ("BASSGODSDAYJOB_diagnostics.zip"), "*.zip");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            if (fc.getResult() == juce::File()) return;
            const auto err = diagnostics::exportZip (proc, fc.getResult().withFileExtension ("zip"));
            juce::AlertWindow::showMessageBoxAsync (err.isEmpty() ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon,
                                                    "Diagnostics", err.isEmpty() ? "Saved " + fc.getResult().withFileExtension ("zip").getFullPathName() : err);
        });
    };
    resetXruns.onClick = [this] { proc.resetXruns(); };
    checkUpdates.onClick = [this]
    {
        checkUpdates.setEnabled (false);
        diagnostics::checkForUpdates ([safe = juce::Component::SafePointer<SettingsTab> (this)] (juce::String msg)
        {
            if (safe == nullptr) return;
            safe->checkUpdates.setEnabled (true);
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Updates", msg);
        });
    };
    cpuLabel.setFont (theme::mono (13.0f));

    content.painter = [this] (juce::Graphics& g)
    {
        for (auto& sec : sections)
            drawPanel (g, sec.r.toFloat(), sec.title, theme::get().magenta);
        g.setColour (theme::get().textDim);
        g.setFont (theme::font (13.0f));
        for (auto& [r, t] : rowLabels)
            g.drawText (t, r, juce::Justification::centredLeft);
    };
    refreshExpansions();
    startTimerHz (4);
}

SettingsTab::~SettingsTab() = default;

void SettingsTab::chooseFolder (const juce::String& key, const juce::String& title)
{
    chooser = std::make_unique<juce::FileChooser> (title, juce::File::getSpecialLocation (juce::File::userDocumentsDirectory));
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories, [this, key] (const juce::FileChooser& fc)
    {
        const auto d = fc.getResult();
        if (! d.isDirectory()) return;
        if (key == "extraPresetFolders")
        {
            auto cur = proc.settings->getString (key);
            proc.settings->set (key, cur.isEmpty() ? d.getFullPathName() : cur + "\n" + d.getFullPathName());
        }
        else
            proc.settings->set (key, d.getFullPathName());
        proc.presets->rescan();
        NoiseBank::getInstance().rescan (proc.settings->noiseFolders());
    });
}

void SettingsTab::installPack (const juce::File& f)
{
    const auto err = ExpansionManager::install (f);
    if (err.isNotEmpty())
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Expansion", err);
    proc.presets->rescan();
    NoiseBank::getInstance().rescan (proc.settings->noiseFolders());
    refreshExpansions();
    resized();
}

bool SettingsTab::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (f.endsWithIgnoreCase (".bgdjpack")) return true;
    return false;
}

void SettingsTab::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files) if (f.endsWithIgnoreCase (".bgdjpack")) installPack (juce::File (f));
}

void SettingsTab::refreshExpansions()
{
    expansionRows.clear();
    for (auto& e : ExpansionManager::list())
    {
        struct Row : public juce::Component
        {
            Row (ExpansionManager::Expansion ex, std::function<void()> changed) : exp (ex)
            {
                addAndMakeVisible (enabled);
                addAndMakeVisible (remove);
                enabled.setButtonText (ex.name + (ex.version.isNotEmpty() ? " v" + ex.version : juce::String()) + "  (" + juce::String (ex.numPresets) + " presets, "
                                       + juce::String (ex.numWavetables) + " wavetables, " + juce::String (ex.numNoise) + " noises)");
                enabled.setToggleState (ex.enabled, juce::dontSendNotification);
                enabled.onClick = [this, changed] { ExpansionManager::setEnabled (exp.id, enabled.getToggleState()); changed(); };
                remove.onClick = [this, changed]
                {
                    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::QuestionIcon, "Remove expansion", "Delete \"" + exp.name + "\" from disk?", "Remove", "Cancel", nullptr,
                        juce::ModalCallbackFunction::create ([this, changed] (int r) { if (r == 1) { ExpansionManager::remove (exp.id); changed(); } }));
                };
            }
            void resized() override { auto r = getLocalBounds(); remove.setBounds (r.removeFromRight (80)); enabled.setBounds (r); }
            ExpansionManager::Expansion exp;
            juce::ToggleButton enabled;
            juce::TextButton remove { "Remove" };
        };
        auto* row = new Row (e, [this] { juce::MessageManager::callAsync ([this] { proc.presets->rescan(); NoiseBank::getInstance().rescan (proc.settings->noiseFolders()); refreshExpansions(); resized(); }); });
        expansionRows.add (row);
        expansionList.addAndMakeVisible (row);
    }
}

void SettingsTab::timerCallback()
{
    const auto folders = proc.settings->presetFolders();
    presetFolder.setText ("Default preset folder: " + folders.getFirst().getFullPathName(), juce::dontSendNotification);
    juce::StringArray extra;
    for (int i = 1; i < folders.size(); ++i) extra.add (folders[i].getFullPathName());
    extraFolders.setText ("Custom preset folders: " + (extra.isEmpty() ? juce::String ("(none)") : extra.joinIntoString ("; ")), juce::dontSendNotification);
    wtFolder.setText ("Wavetable folder: " + proc.settings->wavetableFolders().getFirst().getFullPathName(), juce::dontSendNotification);
    noiseFolder.setText ("Noise folder: " + proc.settings->noiseFolders().getFirst().getFullPathName(), juce::dontSendNotification);
    cpuLabel.setText ("Audio thread CPU: " + juce::String (proc.loadMeasurer.getLoadAsPercentage(), 1) + " %     Xruns: " + juce::String (proc.getXruns())
                      + "     Block: " + juce::String (proc.lastBlockSize.load()) + " @ " + juce::String (proc.getSampleRate(), 0) + " Hz", juce::dontSendNotification);
    safeMode.setToggleState (proc.settings->safeMode(), juce::dontSendNotification);
}

void SettingsTab::resized()
{
    viewport.setBounds (getLocalBounds().reduced (8));
    const int W = viewport.getWidth() - viewport.getScrollBarThickness() - 4;
    sections.clear();
    int y = 0;
    const int pad = 14;
    auto beginSection = [&] (const juce::String& t) { sections.push_back ({ t, { 0, y, W, 0 } }); y += 28; };
    auto endSection = [&] { sections.back().r.setHeight (y - sections.back().r.getY() + 8); y += 18; };
    auto row = [&] (int h) { auto r = juce::Rectangle<int> (pad, y, W - pad * 2, h); y += h + 6; return r; };
    rowLabels.clear();
    auto labelled = [&] (juce::Component& c, const juce::String& text, int h = 26) { auto r = row (h); c.setBounds (r.withTrimmedLeft (220)); rowLabels.push_back ({ r.withWidth (210), text }); return r; };

    beginSection ("BASIC");
    labelled (uiScale, "UI scale");
    labelled (themeBox, "Theme");
    sceneOn.setBounds (row (24));
    labelled (sceneQuality, "Scene quality");
    sfxOn.setBounds (row (24));
    tooltips.setBounds (row (24));
    labelled (knobMode, "Knob behaviour");
    labelled (fineMod, "Fine-adjust modifier");
    crt.setBounds (row (24));
    labelled (djName, "DJ Name (Credits poem)");
    { auto r = row (28); layoutRow (r.withTrimmedLeft (220), { &saveCc, &loadCc, &clearCc }, 8); rowLabels.push_back ({ r.withWidth (210), "MIDI learn (right-click a knob)" }); }
    endSection();

    beginSection ("PRESET & CONTENT PATHS");
    { auto r = row (24); changePreset.setBounds (r.removeFromRight (110)); presetFolder.setBounds (r); }
    { auto r = row (24); clearFolders.setBounds (r.removeFromRight (110)); r.removeFromRight (6); addFolder.setBounds (r.removeFromRight (110)); extraFolders.setBounds (r); }
    { auto r = row (24); changeWt.setBounds (r.removeFromRight (110)); wtFolder.setBounds (r); }
    { auto r = row (24); changeNoise.setBounds (r.removeFromRight (110)); noiseFolder.setBounds (r); }
    rescan.setBounds (row (28).removeFromLeft (200));
    endSection();

    beginSection ("EXPANSIONS (.bgdjpack) - drag a pack onto this tab to install");
    installPackBtn.setBounds (row (28).removeFromLeft (200));
    {
        const int h = std::max (26, expansionRows.size() * 28);
        auto r = row (h);
        expansionList.setBounds (r);
        int ry = 0;
        for (auto* er : expansionRows) { er->setBounds (0, ry, r.getWidth(), 26); ry += 28; }
    }
    endSection();

    beginSection ("TROUBLESHOOTING");
    cpuLabel.setBounds (row (24));
    { auto r = row (28); layoutRow (r, { &resetXruns, &resetUi, &clearCaches }, 8); }
    { auto r = row (28); layoutRow (r, { &exportDiag, &factoryReset, &checkUpdates }, 8); }
    softwareRender.setBounds (row (24));
    safeMode.setBounds (row (24));
    analyzers.setBounds (row (24));
    autoSave.setBounds (row (24));
    crashReports.setBounds (row (24));
    endSection();

    content.setSize (W, y);
    repaint();
}

void SettingsTab::paint (juce::Graphics& g)
{
    g.fillAll (theme::get().bg);
}
} // namespace bgdj
