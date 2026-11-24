#include "PluginEditor.h"
#include "Settings/Diagnostics.h"

namespace bgdj
{
BassGodsEditor::BassGodsEditor (BassGodsProcessor& p)
    : AudioProcessorEditor (&p), proc (p), analysis (p), header (p)
{
    theme::set (proc.settings->theme());
    lnf.refreshColours();
    setLookAndFeel (&lnf);
    proc.editorOpen = true;

    addAndMakeVisible (content);
    content.addAndMakeVisible (header);
    content.addAndMakeVisible (tabs);

    synthTab = std::make_unique<SynthTab> (proc);
    fxTab = std::make_unique<FxTab> (proc, analysis);
    vizTab = std::make_unique<VisualizerTab> (proc, analysis);
    tweaksTab = std::make_unique<TweaksTab> (proc);
    settingsTab = std::make_unique<SettingsTab> (proc);
    creditsTab = std::make_unique<CreditsTab> (proc);

    const auto tabBg = theme::get().bg;
    tabs.addTab ("SYNTH", tabBg, synthTab.get(), false);
    tabs.addTab ("FX", tabBg, fxTab.get(), false);
    tabs.addTab ("VISUALIZER / METERING", tabBg, vizTab.get(), false);
    tabs.addTab ("TWEAKS", tabBg, tweaksTab.get(), false);
    tabs.addTab ("SETTINGS", tabBg, settingsTab.get(), false);
    tabs.addTab ("CREDITS", tabBg, creditsTab.get(), false);
    tabs.setTabBarDepth (32);
    tabs.setOutline (0);

    synthTab->onEditWavetable = [this] (int osc) { openWavetableEditor (osc); };
    synthTab->getScene().onExpandToggle = [this] (bool e) { setSceneExpanded (e); };
    creditsTab->onWakeUp = [this] { tabs.setCurrentTabIndex (0); };
    settingsTab->onResetLayout = [this] { setSize (kLogicalW, kLogicalH); };

    browser = std::make_unique<PresetBrowser> (proc);
    content.addChildComponent (*browser);
    browser->onClose = [this] { showBrowser (false); };
    header.onBrowse = [this] { showBrowser (! browser->isVisible()); };
    header.getBrowseList = [this] { return browser->currentList(); };

    proc.settings->addChangeListener (this);
    applySettings (true);

    setResizable (true, true);
    setResizeLimits (kLogicalW / 2, kLogicalH / 2, kLogicalW * 3, kLogicalH * 3);
    const int savedW = proc.settings->getInt ("windowW", 0), savedH = proc.settings->getInt ("windowH", 0);
    if (savedW >= kLogicalW / 2 && savedH >= kLogicalH / 2)
        setSize (savedW, savedH);
    else
        setSize ((int) (kLogicalW * lastScaleSetting), (int) (kLogicalH * lastScaleSetting));

    startTimerHz (60);
}

BassGodsEditor::~BassGodsEditor()
{
    stopTimer();
    proc.editorOpen = false;
    proc.settings->removeChangeListener (this);
    if (glAttached)
        glContext.detach();
    tabs.clearTabs();
    setLookAndFeel (nullptr);
}

void BassGodsEditor::newOpenGLContextCreated()
{
    using namespace juce::gl;
    const auto* renderer = reinterpret_cast<const char*> (glGetString (GL_RENDERER));
    const auto* vendor = reinterpret_cast<const char*> (glGetString (GL_VENDOR));
    const auto* version = reinterpret_cast<const char*> (glGetString (GL_VERSION));
    const juce::String desc = juce::String (vendor != nullptr ? vendor : "?") + " / " + (renderer != nullptr ? renderer : "?")
                            + " / OpenGL " + (version != nullptr ? version : "?");
    juce::MessageManager::callAsync ([desc] { diagnostics::setGpuDescription (desc); });
}

void BassGodsEditor::applySettings (bool initial)
{
    auto& S = *proc.settings;
    if (initial || S.theme() != lastTheme)
    {
        lastTheme = S.theme();
        theme::set (lastTheme);
        lnf.refreshColours();
        sendLookAndFeelChange();
        repaint();
    }
    if (S.tooltips() && tooltips == nullptr)       tooltips = std::make_unique<juce::TooltipWindow> (this, 600);
    else if (! S.tooltips())                         tooltips.reset();

    const bool wantGl = ! S.forceSoftwareRendering();
    if (wantGl && ! glAttached)
    {
        glContext.setRenderer (this);
        glContext.setComponentPaintingEnabled (true);
        glContext.setContinuousRepainting (false);
        glContext.attachTo (*this);
        glAttached = true;
    }
    else if (! wantGl && glAttached)
    {
        glContext.detach();
        glAttached = false;
        diagnostics::setGpuDescription ("Software rendering (forced in Settings)");
    }

    const float sc = juce::jlimit (0.5f, 2.0f, S.uiScale());
    if (! initial && std::abs (sc - lastScaleSetting) > 0.001f)
        setSize ((int) (kLogicalW * sc), (int) (kLogicalH * sc));
    lastScaleSetting = sc;
    crt = S.getBool ("crtOverlay", false);
    proc.analyzersEnabled = S.analyzersEnabled();
}

void BassGodsEditor::changeListenerCallback (juce::ChangeBroadcaster*) { applySettings (false); }

void BassGodsEditor::timerCallback()
{
    analysis.update();
    static int learnTick = 0;
    if (proc.isLearning() || (++learnTick % 15) == 0)
        repaint();
}

void BassGodsEditor::showBrowser (bool show)
{
    browser->setVisible (show);
    if (show) browser->toFront (true);
}

void BassGodsEditor::openWavetableEditor (int osc)
{
    wtEditor = std::make_unique<WavetableEditor> (proc, osc);
    wtEditor->onClose = [this] { juce::MessageManager::callAsync ([this] { wtEditor.reset(); }); };
    content.addAndMakeVisible (*wtEditor);
    wtEditor->setBounds (content.getLocalBounds().reduced (60, 50));
    wtEditor->toFront (true);
}

void BassGodsEditor::setSceneExpanded (bool e)
{
    auto& scene = synthTab->getScene();
    if (e)
    {
        content.addAndMakeVisible (scene);
        scene.setBounds (content.getLocalBounds().reduced (8));
        scene.toFront (false);
    }
    else
    {
        synthTab->addAndMakeVisible (scene);
        synthTab->resized();
    }
}

void BassGodsEditor::resized()
{
    content.setBounds (0, 0, kLogicalW, kLogicalH);
    const float scale = std::min ((float) getWidth() / kLogicalW, (float) getHeight() / kLogicalH);
    const float ox = ((float) getWidth() - kLogicalW * scale) * 0.5f;
    const float oy = ((float) getHeight() - kLogicalH * scale) * 0.5f;
    content.setTransform (juce::AffineTransform::scale (scale).translated (ox, oy));

    auto r = content.getLocalBounds();
    header.setBounds (r.removeFromTop (56));
    tabs.setBounds (r);
    browser->setBounds (content.getLocalBounds());
    if (wtEditor) wtEditor->setBounds (content.getLocalBounds().reduced (60, 50));
    if (synthTab->getScene().isExpanded())
        synthTab->getScene().setBounds (content.getLocalBounds().reduced (8));

    proc.settings->set ("windowW", getWidth());
    proc.settings->set ("windowH", getHeight());
}

void BassGodsEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void BassGodsEditor::paintOverChildren (juce::Graphics& g)
{
    if (proc.isLearning())
    {
        auto r = getLocalBounds().removeFromBottom (40).toFloat().reduced (getWidth() * 0.3f, 6.0f);
        g.setColour (theme::get().yellow.withAlpha (0.92f));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colours::black);
        g.setFont (theme::font (15.0f, true));
        g.drawText ("MIDI LEARN: move a controller on your MIDI device", r, juce::Justification::centred);
    }
    if (crt)
    {
        // subtle grain + scanlines
        if (grain.isNull() || grain.getWidth() != 256)
        {
            grain = juce::Image (juce::Image::ARGB, 256, 256, true);
            juce::Random r (7);
            for (int y = 0; y < 256; ++y)
                for (int x = 0; x < 256; ++x)
                    grain.setPixelAt (x, y, juce::Colours::white.withAlpha ((float) r.nextInt (18) / 255.0f));
        }
        g.setTiledImageFill (grain, juce::Random::getSystemRandom().nextInt (64), juce::Random::getSystemRandom().nextInt (64), 1.0f);
        g.fillAll();
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        for (int y = 0; y < getHeight(); y += 3)
            g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    }
}
} // namespace bgdj
