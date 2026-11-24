#pragma once

#include "SceneArt.h"
#include "../UI/Widgets.h"

namespace bgdj
{
class BassGodsProcessor;

/** The interactive club: drains audio features, runs the mood state machine and reaction
    director, plays SFX cues and renders the scene (capped at 60 fps, never on the audio thread). */
class ClubScene : public juce::Component, private juce::Timer, private juce::ChangeListener
{
public:
    explicit ClubScene (BassGodsProcessor& p);
    ~ClubScene() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void visibilityChanged() override;

    const MoodState& getMood() const noexcept { return mood.get(); }
    const ReactionDirector& getDirector() const noexcept { return director; }
    juce::String lastReactionName() const;

    std::function<void (bool)> onExpandToggle;
    void setExpanded (bool e);
    bool isExpanded() const noexcept { return expanded; }

    void takeScreenshot();
    void startClip();

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void applySettings();
    SceneFrame makeFrame (float dt);

    BassGodsProcessor& proc;
    ClubRenderer renderer;
    CrowdMood mood;
    ReactionDirector director;

    juce::TextButton pauseBtn { "||" }, expandBtn { "[ ]" }, shotBtn { "PNG" }, clipBtn { "GIF 10s" }, sfxBtn { "SFX" };
    juce::ComboBox personality;

    bool enabled = true, paused = false, expanded = false;
    int quality = 1, fps = 45;
    double lastTime = 0.0, sceneTime = 0.0, internalBeats = 0.0;
    int lastBeatIndex = -1;
    float parallax = 0.0f, energy = 0.0f, sub = 0.0f, flux = 0.0f, macroGlow = 0.0f;
    int recentNotes = 0;
    SceneFrame lastFrame;

    // clip capture
    juce::Array<juce::Image> clipFrames;
    double clipStart = -1.0, nextClipFrame = 0.0;
};
} // namespace bgdj
