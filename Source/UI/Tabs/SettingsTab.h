#pragma once

#include "../Widgets.h"

namespace bgdj
{
/** SETTINGS tab: basic preferences plus the advanced / troubleshooting toolbox. */
class SettingsTab : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit SettingsTab (BassGodsProcessor& p);
    ~SettingsTab() override;
    void resized() override;
    void paint (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

    std::function<void()> onResetLayout;

private:
    void timerCallback() override;
    void refreshExpansions();
    void installPack (const juce::File& f);
    void chooseFolder (const juce::String& key, const juce::String& title);

    BassGodsProcessor& proc;
    struct Content : public juce::Component { std::function<void (juce::Graphics&)> painter; void paint (juce::Graphics& g) override { if (painter) painter (g); } };
    juce::Viewport viewport;
    Content content;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> rowLabels;
    juce::OwnedArray<juce::Component> owned;
    std::unique_ptr<juce::FileChooser> chooser;

    // basic
    juce::Slider uiScale;
    juce::ComboBox themeBox, sceneQuality, knobMode, fineMod;
    juce::ToggleButton sceneOn { "Club scene on" }, sfxOn { "Scene SFX on (only heard if 'Crowd SFX to output' is on in Tweaks)" },
                       tooltips { "Tooltips" }, crt { "Grain / CRT overlay" };
    juce::TextEditor djName;
    juce::TextButton saveCc { "Save CC map..." }, loadCc { "Load CC map..." }, clearCc { "Clear all mappings" };

    // advanced
    juce::Label presetFolder, extraFolders, wtFolder, noiseFolder;
    juce::TextButton changePreset { "Change..." }, addFolder { "Add folder..." }, clearFolders { "Clear extra" },
                     changeWt { "Change..." }, changeNoise { "Change..." }, rescan { "Rescan content" };
    juce::TextButton installPackBtn { "Install .bgdjpack..." };
    juce::Component expansionList;
    juce::OwnedArray<juce::Component> expansionRows;

    juce::TextButton resetUi { "Reset UI layout & window size" }, clearCaches { "Clear caches / rebuild preset database" },
                     factoryReset { "Reset ALL settings to factory" }, exportDiag { "Export diagnostic log (.zip)..." },
                     resetXruns { "Reset xrun counter" }, checkUpdates { "Check for updates" };
    juce::ToggleButton softwareRender { "Force software rendering (disable OpenGL / GPU) - reopen the window" },
                       safeMode { "Safe Mode (scene off, oversampling 1x, analyzers off)" },
                       autoSave { "Auto-save settings" }, crashReports { "Write crash reports to the log (standalone app, opt-in)" },
                       analyzers { "Analyzers on" };
    juce::Label cpuLabel;

    struct Section { juce::String title; juce::Rectangle<int> r; };
    std::vector<Section> sections;
};
} // namespace bgdj
