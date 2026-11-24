#pragma once

#include <juce_opengl/juce_opengl.h>
#include "PluginProcessor.h"
#include "UI/Theme.h"
#include "UI/Header.h"
#include "UI/PresetBrowser.h"
#include "UI/WavetableEditor.h"
#include "UI/Tabs/SynthTab.h"
#include "UI/Tabs/FxTab.h"
#include "UI/Tabs/VisualizerTab.h"
#include "UI/Tabs/TweaksTab.h"
#include "UI/Tabs/SettingsTab.h"
#include "UI/Tabs/CreditsTab.h"
#include "Analysis/AnalysisData.h"

namespace bgdj
{
/** The editor draws a fixed 1280x860 logical layout and scales it (vector, crisp at any size)
    to whatever size the window is dragged to. */
class BassGodsEditor : public juce::AudioProcessorEditor,
                       public juce::DragAndDropContainer,
                       private juce::Timer,
                       private juce::ChangeListener,
                       private juce::OpenGLRenderer
{
public:
    static constexpr int kLogicalW = 1280, kLogicalH = 860;

    explicit BassGodsEditor (BassGodsProcessor&);
    ~BassGodsEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void applySettings (bool initial);
    void showBrowser (bool show);
    void openWavetableEditor (int osc);
    void setSceneExpanded (bool e);

    // OpenGLRenderer (only used to read the GPU name; painting is done by JUCE components)
    void newOpenGLContextCreated() override;
    void renderOpenGL() override {}
    void openGLContextClosing() override {}

    BassGodsProcessor& proc;
    BgdjLookAndFeel lnf;
    juce::OpenGLContext glContext;
    bool glAttached = false;
    std::unique_ptr<juce::TooltipWindow> tooltips;
    AnalysisData analysis;

    struct Content : public juce::Component
    {
        void paint (juce::Graphics& g) override { g.fillAll (theme::get().bg); }
    } content;

    Header header;
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    std::unique_ptr<SynthTab> synthTab;
    std::unique_ptr<FxTab> fxTab;
    std::unique_ptr<VisualizerTab> vizTab;
    std::unique_ptr<TweaksTab> tweaksTab;
    std::unique_ptr<SettingsTab> settingsTab;
    std::unique_ptr<CreditsTab> creditsTab;
    std::unique_ptr<PresetBrowser> browser;
    std::unique_ptr<WavetableEditor> wtEditor;

    float lastScaleSetting = 1.0f;
    juce::String lastTheme;
    bool crt = false;
    juce::Image grain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassGodsEditor)
};
} // namespace bgdj
