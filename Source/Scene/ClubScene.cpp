#include "ClubScene.h"
#include "../PluginProcessor.h"
#include "../UI/GifEncoder.h"

namespace bgdj
{
ClubScene::ClubScene (BassGodsProcessor& p) : proc (p)
{
    for (auto* b : { &pauseBtn, &expandBtn, &shotBtn, &clipBtn, &sfxBtn })
    {
        addAndMakeVisible (b);
        b->setAlpha (0.85f);
    }
    pauseBtn.setTooltip ("Pause the scene");
    expandBtn.setTooltip ("Expand the scene to the full window");
    shotBtn.setTooltip ("Save a screenshot (PNG) to Pictures/BASSGODSDAYJOB");
    clipBtn.setTooltip ("Record a 10-second clip (animated GIF) for socials");
    sfxBtn.setTooltip ("Crowd sound effects on/off (only audible if 'Crowd SFX to output' is enabled - never printed by default)");
    sfxBtn.setClickingTogglesState (true);

    pauseBtn.onClick = [this] { paused = ! paused; pauseBtn.setButtonText (paused ? ">" : "||"); };
    expandBtn.onClick = [this] { setExpanded (! expanded); if (onExpandToggle) onExpandToggle (expanded); };
    shotBtn.onClick = [this] { takeScreenshot(); };
    clipBtn.onClick = [this] { startClip(); };
    sfxBtn.onClick = [this] { proc.settings->set ("crowdSfx", sfxBtn.getToggleState()); };

    addAndMakeVisible (personality);
    personality.addItemList (crowdPersonalityNames(), 1);
    personality.setTooltip ("Crowd personality");
    personality.onChange = [this]
    {
        mood.setPersonality ((CrowdPersonality) std::max (0, personality.getSelectedItemIndex()));
        proc.settings->set ("crowdPersonality", personality.getSelectedItemIndex());
    };

    proc.settings->addChangeListener (this);
    applySettings();
    personality.setSelectedItemIndex (juce::jlimit (0, 4, proc.settings->getInt ("crowdPersonality", 0)), juce::sendNotificationSync);
    lastTime = juce::Time::getMillisecondCounterHiRes() * 0.001;
}

ClubScene::~ClubScene()
{
    proc.settings->removeChangeListener (this);
}

void ClubScene::changeListenerCallback (juce::ChangeBroadcaster*) { applySettings(); }

void ClubScene::applySettings()
{
    enabled = proc.settings->sceneEnabled();
    const int q = juce::jlimit (0, 2, proc.settings->sceneQuality());
    if (q != quality || renderer.numDancers() == 0)
    {
        quality = q;
        renderer.init (quality, 0xC1B5EEDu);
    }
    fps = quality == 0 ? 30 : (quality == 1 ? 45 : 60);
    sfxBtn.setToggleState (proc.settings->crowdSfxEnabled(), juce::dontSendNotification);
    // keep ticking the crowd even when the scene is hidden or off, just slower and without drawing
    startTimerHz (enabled && isShowing() ? fps : 15);
    repaint();
}

void ClubScene::visibilityChanged() { applySettings(); }

void ClubScene::setExpanded (bool e)
{
    expanded = e;
    expandBtn.setButtonText (e ? "><" : "[ ]");
}

juce::String ClubScene::lastReactionName() const
{
    if (auto* d = findReaction (director.getLastId()))
        return d->name;
    return {};
}

void ClubScene::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto top = r.removeFromTop (20);
    const int bw = 34;
    clipBtn.setBounds (top.removeFromRight (bw + 22));
    top.removeFromRight (3);
    shotBtn.setBounds (top.removeFromRight (bw));
    top.removeFromRight (3);
    sfxBtn.setBounds (top.removeFromRight (bw));
    top.removeFromRight (3);
    expandBtn.setBounds (top.removeFromRight (bw - 6));
    top.removeFromRight (3);
    pauseBtn.setBounds (top.removeFromRight (bw - 8));
    personality.setBounds (top.removeFromLeft (150));
}

void ClubScene::mouseMove (const juce::MouseEvent& e)
{
    parallax = juce::jlimit (-1.0f, 1.0f, (float) e.x / (float) std::max (1, getWidth()) * 2.0f - 1.0f);
}

SceneFrame ClubScene::makeFrame (float dt)
{
    SceneFrame f;
    f.time = sceneTime;
    f.dt = dt;
    const double bpm = std::max (40.0, proc.uiBpm.load());
    double beats;
    if (proc.uiPlaying.load())
        beats = proc.uiPpq.load();
    else
    {
        internalBeats += dt * bpm / 60.0;
        beats = internalBeats;
    }
    f.beatPhase = (float) (beats - std::floor (beats));
    f.beatIndex = (int) std::floor (beats);
    f.newBeat = f.beatIndex != lastBeatIndex;
    lastBeatIndex = f.beatIndex;
    f.bpm = (float) bpm;
    f.mood = &mood.get();
    f.director = &director;
    f.energy = energy;
    f.sub = sub;
    f.flux = flux;
    f.recentNotes = recentNotes;
    f.parallax = parallax;
    f.quality = quality;
    f.personality = mood.getPersonality();
    f.macroBumpGlow = macroGlow;
    return f;
}

void ClubScene::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = (float) juce::jlimit (0.001, 0.25, now - lastTime);
    lastTime = now;
    sceneTime += dt;

    // ---- drain audio features (lock-free) ---------------------------------------------------------
    AudioFeatures af;
    int notes = 0;
    float subAcc = 0.0f, fluxAcc = 0.0f, rmsAcc = 0.0f;
    int blocks = 0;
    while (proc.featureFifo.pop (af))
    {
        mood.addFeatures (af);
        notes += af.noteOns;
        subAcc += af.sub; fluxAcc += af.flux; rmsAcc += af.rms;
        ++blocks;
    }
    if (blocks > 0)
    {
        energy += (juce::jlimit (0.0f, 1.0f, (20.0f * std::log10 (rmsAcc / (float) blocks + 1.0e-6f) + 40.0f) / 32.0f) - energy) * 0.3f;
        sub += (juce::jlimit (0.0f, 1.0f, std::sqrt (subAcc / (float) blocks) * 3.0f) - sub) * 0.4f;
        flux = fluxAcc / (float) blocks;
    }
    else
        energy *= 0.95f;
    recentNotes = notes > 0 ? 6 : std::max (0, recentNotes - 1);

    mood.setCharacter (proc.currentPresetCharacter);
    mood.tick (dt);
    director.update (sceneTime, dt, mood);

    // ---- reaction side effects -----------------------------------------------------------------------
    for (int id : director.startedThisFrame)
    {
        if (auto* d = findReaction (id))
        {
            if (proc.settings->crowdSfxEnabled() && d->sfx != nullptr && *d->sfx != 0)
                proc.crowdSfx.trigger (CrowdSfx::cueFromName (d->sfx));
            AppSettings::log ("Crowd reaction: " + juce::String (d->name));
        }
        if (id == rx::CatOnController)
        {
            macroGlow = 1.0f;
            // visual only, unless Cat Chaos Mode is on: then Grey really bumps a macro
            if (proc.apvts.getRawParameterValue ("cat_chaos")->load() >= 0.5f)
            {
                juce::Random r;
                if (auto* m = proc.apvts.getParameter (params::macroId (r.nextInt (kNumMacros))))
                    m->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, m->getValue() + (r.nextBool() ? 0.12f : -0.12f)));
            }
        }
    }
    macroGlow = std::max (0.0f, macroGlow - dt * 0.4f);

    const bool draw = enabled && isShowing();
    if (! paused)
    {
        lastFrame = makeFrame (dt);
        renderer.update (lastFrame);
    }

    // ---- clip recording (12 fps, 10 s) --------------------------------------------------------------------
    if (clipStart >= 0.0 && now >= nextClipFrame)
    {
        juce::Image img (juce::Image::RGB, 480, std::max (120, (int) (480.0f * (float) getHeight() / (float) std::max (1, getWidth()))), true);
        juce::Graphics g (img);
        renderer.draw (g, img.getBounds().toFloat(), lastFrame);
        clipFrames.add (img);
        nextClipFrame = now + 1.0 / 12.0;
        clipBtn.setButtonText ("REC " + juce::String (10 - (int) (now - clipStart)));
        if (now - clipStart >= 10.0)
        {
            clipStart = -1.0;
            clipBtn.setButtonText ("GIF 10s");
            auto frames = clipFrames;
            clipFrames.clear();
            auto dir = juce::File::getSpecialLocation (juce::File::userPicturesDirectory).getChildFile ("BASSGODSDAYJOB");
            dir.createDirectory();
            const auto file = dir.getChildFile ("club_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S") + ".gif");
            juce::Thread::launch ([frames, file]
            {
                const bool ok = GifEncoder::write (file, frames, 8);
                juce::MessageManager::callAsync ([ok, file]
                {
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Clip export",
                        ok ? "Saved " + file.getFullPathName() : juce::String ("Could not write the clip."));
                });
            });
        }
    }

    if (draw && ! paused)
        repaint();
}

void ClubScene::paint (juce::Graphics& g)
{
    const auto& p = theme::get();
    auto r = getLocalBounds().toFloat();
    if (! enabled)
    {
        g.setColour (p.panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (p.textDim);
        g.setFont (theme::font (14.0f));
        g.drawText (proc.settings->safeMode() ? "Club scene off (Safe Mode)" : "Club scene off - enable it in Settings", r, juce::Justification::centred);
        return;
    }
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, 8.0f);
        g.reduceClipRegion (clip);
        renderer.draw (g, r, lastFrame);
    }
    g.setColour (p.magenta.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.5f);
    if (paused)
    {
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (theme::font (13.0f, true));
        g.drawText ("PAUSED", r.reduced (10.0f), juce::Justification::bottomLeft);
    }
    if (clipStart >= 0.0)
    {
        g.setColour (juce::Colours::red);
        g.fillEllipse (r.getRight() - 18.0f, r.getBottom() - 18.0f, 10.0f, 10.0f);
    }
}

void ClubScene::takeScreenshot()
{
    auto img = createComponentSnapshot (getLocalBounds(), true, 2.0f);
    auto dir = juce::File::getSpecialLocation (juce::File::userPicturesDirectory).getChildFile ("BASSGODSDAYJOB");
    dir.createDirectory();
    const auto file = dir.getChildFile ("club_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S") + ".png");
    juce::PNGImageFormat png;
    bool ok = false;
    if (juce::FileOutputStream out (file); out.openedOk())
        ok = png.writeImageToStream (img, out);
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Screenshot",
                                            ok ? "Saved " + file.getFullPathName() : juce::String ("Could not save the screenshot."));
}

void ClubScene::startClip()
{
    if (clipStart >= 0.0 || ! enabled)
        return;
    clipFrames.clear();
    clipStart = juce::Time::getMillisecondCounterHiRes() * 0.001;
    nextClipFrame = clipStart;
}
} // namespace bgdj
