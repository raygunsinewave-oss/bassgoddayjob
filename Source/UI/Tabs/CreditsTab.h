#pragma once

#include "../Widgets.h"

namespace bgdj
{
/** CREDITS tab: starfield + neon pulse, scrolling credits, Grey the cat wandering over the
    text, auto-generated third-party licences, then the End Poem as a two-voice dialogue. */
class CreditsTab : public juce::Component, private juce::Timer
{
public:
    explicit CreditsTab (BassGodsProcessor& p);
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    std::function<void()> onWakeUp;
    void restart();

    /** Splits the poem into paragraphs and assigns speaker styles (exposed for tests). */
    enum class Style { Title, Heading, Body, Small, SpeakerA, SpeakerB, Narration, Attribution, WakeUp, Spacer };
    struct Item { juce::String text; Style style; bool scrambled = false; };
    static std::vector<Item> buildPoem (const juce::String& poemText, const juce::String& djName);

private:
    void timerCallback() override;
    void buildItems();
    float layoutHeight (const Item& it, float width) const;
    juce::Font fontFor (Style s) const;
    juce::Colour colourFor (Style s) const;
    static juce::String glitch (const juce::String& text, juce::Random& r);

    BassGodsProcessor& proc;
    std::vector<Item> items;
    std::vector<float> itemY, itemH;
    float totalHeight = 0.0f, scroll = 0.0f, speedMul = 1.0f, pulse = 0.0f;
    bool paused = false;
    double lastT = 0.0;
    juce::Slider speed;
    juce::TextButton pauseBtn { "Pause" }, skipBtn { "Skip >>" }, restartBtn { "Restart" };
    juce::Rectangle<float> wakeRect;
    struct Star { float x, y, z; };
    std::vector<Star> stars;
    juce::Random rng;
    // Grey the cat
    float catX = -100.0f, catTargetY = 0.0f, catSit = 0.0f;
    int catState = 0;   // 0 off, 1 walking in, 2 sitting, 3 walking out
    double nextCat = 6.0, time = 0.0;
    juce::String lastName;
};
} // namespace bgdj
