#include "CreditsTab.h"
#include "../../PluginProcessor.h"
#include "BinaryData.h"

namespace bgdj
{
static juce::String resourceText (const char* name)
{
    int size = 0;
    if (const char* d = BinaryData::getNamedResource (name, size))
        return juce::String::fromUTF8 (d, size);
    return {};
}

std::vector<CreditsTab::Item> CreditsTab::buildPoem (const juce::String& poemText, const juce::String& djName)
{
    std::vector<Item> out;
    juce::StringArray lines;
    lines.addLines (poemText.replace ("\r\n", "\n"));
    bool narration = false;
    int speaker = 0;
    for (auto line : lines)
    {
        line = line.trim();
        if (line.isEmpty())
            continue;
        if (line.startsWith ("Take a breath, now."))
            narration = true;
        Item it;
        it.text = line.replace ("PLAYERNAME", djName);
        it.scrambled = it.text.contains ("[scrambled]");
        if (line == "Wake up.")
            it.style = Style::WakeUp;
        else if (narration)
            it.style = Style::Narration;
        else
            it.style = (speaker++ % 2 == 0) ? Style::SpeakerA : Style::SpeakerB;
        out.push_back (it);
    }
    return out;
}

CreditsTab::CreditsTab (BassGodsProcessor& p) : proc (p)
{
    addAndMakeVisible (speed);
    speed.setRange (0.5, 3.0, 0.05);
    speed.setValue (1.0);
    speed.setTextValueSuffix ("x");
    speed.setSliderStyle (juce::Slider::LinearHorizontal);
    speed.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    speed.onValueChange = [this] { speedMul = (float) speed.getValue(); };
    for (auto* b : { &pauseBtn, &skipBtn, &restartBtn }) addAndMakeVisible (b);
    pauseBtn.onClick = [this] { paused = ! paused; pauseBtn.setButtonText (paused ? "Play" : "Pause"); };
    skipBtn.onClick = [this]
    {
        // jump to the poem, or to the end if already in it
        for (size_t i = 0; i < items.size(); ++i)
            if (items[i].style == Style::SpeakerA && itemY[i] > scroll + 5.0f) { scroll = itemY[i] - (float) getHeight() * 0.3f; return; }
        scroll = totalHeight - (float) getHeight() * 0.6f;
    };
    restartBtn.onClick = [this] { restart(); };
    for (int i = 0; i < 220; ++i) stars.push_back ({ rng.nextFloat() * 2.0f - 1.0f, rng.nextFloat() * 2.0f - 1.0f, rng.nextFloat() });
    setWantsKeyboardFocus (false);
}

void CreditsTab::buildItems()
{
    items.clear();
    auto add = [this] (const juce::String& t, Style s) { items.push_back ({ t, s, false }); };
    add ("BASSGODSDAYJOB", Style::Title);
    add ("version " BGDJ_VERSION_STRING, Style::Small);
    add ({}, Style::Spacer);
    add ("Created, designed & developed by", Style::Heading);
    add ("Raymond Duarte (Raygun)", Style::Body);
    add ({}, Style::Spacer);
    add ("Special thanks to", Style::Heading);
    add ("Grey the Cat", Style::Body);
    add ("for annoying me during the entire process of making this plugin.", Style::Small);
    add ({}, Style::Spacer);
    add ("Third-party licences", Style::Heading);
    juce::StringArray lic;
    lic.addLines (resourceText ("ThirdPartyLicenses_txt"));
    for (auto& l : lic) add (l, Style::Small);
    add ({}, Style::Spacer);
    add ({}, Style::Spacer);
    for (auto& it : buildPoem (resourceText ("end_poem_txt"), proc.settings->djName()))
        items.push_back (it);
    add ({}, Style::Spacer);
    add ("End Poem by Julian Gough. Released into the public domain under CC0.", Style::Attribution);
    add ({}, Style::Spacer);
    lastName = proc.settings->djName();
    resized();
}

void CreditsTab::restart()
{
    scroll = -(float) getHeight() * 0.8f;
    paused = false;
    pauseBtn.setButtonText ("Pause");
}

void CreditsTab::visibilityChanged()
{
    if (isShowing())
    {
        if (items.empty() || lastName != proc.settings->djName()) buildItems();
        restart();
        lastT = juce::Time::getMillisecondCounterHiRes() * 0.001;
        startTimerHz (60);
    }
    else
        stopTimer();
}

juce::Font CreditsTab::fontFor (Style s) const
{
    switch (s)
    {
        case Style::Title:       return theme::font (52.0f, true);
        case Style::Heading:     return theme::font (16.0f, true);
        case Style::Body:        return theme::font (26.0f, true);
        case Style::Small:       return theme::font (13.0f);
        case Style::Attribution: return theme::font (13.0f, true);
        case Style::WakeUp:      return theme::font (32.0f, true);
        default:                 return theme::font (18.0f);
    }
}

juce::Colour CreditsTab::colourFor (Style s) const
{
    const auto& p = theme::get();
    switch (s)
    {
        case Style::Title:     return p.pink;
        case Style::Heading:   return p.cyan;
        case Style::SpeakerA:  return juce::Colour (0xff2ad4c8);    // teal voice
        case Style::SpeakerB:  return juce::Colour (0xff9dff8a);    // light green voice
        case Style::Narration: return juce::Colour (0xfff2e6ff);
        case Style::WakeUp:    return p.yellow;
        case Style::Small:     return p.textDim;
        default:               return p.text;
    }
}

float CreditsTab::layoutHeight (const Item& it, float width) const
{
    if (it.style == Style::Spacer) return 40.0f;
    juce::AttributedString a;
    a.append (it.text.isEmpty() ? " " : it.text, fontFor (it.style), colourFor (it.style));
    a.setJustification (juce::Justification::centred);
    juce::TextLayout tl;
    tl.createLayout (a, width);
    return tl.getHeight() + (it.style == Style::Small ? 2.0f : 14.0f);
}

void CreditsTab::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto bar = r.removeFromBottom (28);
    restartBtn.setBounds (bar.removeFromRight (80));
    bar.removeFromRight (6);
    skipBtn.setBounds (bar.removeFromRight (80));
    bar.removeFromRight (6);
    pauseBtn.setBounds (bar.removeFromRight (70));
    bar.removeFromRight (6);
    speed.setBounds (bar.removeFromRight (220));

    const float w = std::min (760.0f, (float) getWidth() - 80.0f);
    itemY.clear(); itemH.clear();
    float y = 0.0f;
    for (auto& it : items)
    {
        const float h = layoutHeight (it, w);
        itemY.push_back (y);
        itemH.push_back (h);
        y += h;
    }
    totalHeight = y;
}

juce::String CreditsTab::glitch (const juce::String& text, juce::Random& r)
{
    // every [scrambled] token becomes constantly shuffling glyphs that can never be read
    static const juce::String glyphs = juce::String (juce::CharPointer_UTF8 ("\xe2\x96\x93\xe2\x96\x92\xe2\x96\x91\xe2\x96\x88\xc2\xa7\xc2\xa4\xc3\x98\xc3\x9e\xce\xa9\xce\xa8\xe2\x88\x86\xe2\x88\x91#%&@$*=+~^"));
    juce::String out;
    int idx = 0;
    while (true)
    {
        const int pos = text.indexOf (idx, "[scrambled]");
        if (pos < 0) { out << text.substring (idx); break; }
        out << text.substring (idx, pos);
        const int len = 6 + r.nextInt (6);
        for (int k = 0; k < len; ++k) out << glyphs[r.nextInt (glyphs.length())];
        idx = pos + 11;
    }
    return out;
}

void CreditsTab::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = (float) juce::jlimit (0.0, 0.1, now - lastT);
    lastT = now;
    time += dt;
    pulse = 0.5f + 0.5f * std::sin ((float) time * 1.3f);
    if (! paused && scroll < totalHeight - (float) getHeight() * 0.45f)
        scroll += dt * 38.0f * speedMul;
    for (auto& s : stars)
    {
        s.z -= dt * 0.05f * speedMul;
        if (s.z <= 0.02f) { s.z = 1.0f; s.x = rng.nextFloat() * 2.0f - 1.0f; s.y = rng.nextFloat() * 2.0f - 1.0f; }
    }

    // Grey the cat wanders onto the credits and sits on the text
    const float W = (float) getWidth();
    if (catState == 0 && time > nextCat)
    {
        catState = 1;
        catX = -60.0f;
        catTargetY = (float) getHeight() * (0.35f + 0.3f * rng.nextFloat());
    }
    else if (catState == 1)
    {
        catX += dt * 90.0f;
        if (catX > W * (0.3f + 0.2f * (float) std::fmod (time, 1.0))) { catState = 2; catSit = 3.0f + rng.nextFloat() * 4.0f; }
    }
    else if (catState == 2)
    {
        catSit -= dt;
        if (catSit <= 0.0f) catState = 3;
    }
    else if (catState == 3)
    {
        catX += dt * 110.0f;
        if (catX > W + 60.0f) { catState = 0; nextCat = time + 10.0 + rng.nextFloat() * 15.0; }
    }
    repaint();
}

void CreditsTab::paint (juce::Graphics& g)
{
    const auto& p = theme::get();
    g.fillAll (juce::Colour (0xff05030a));
    const auto c = getLocalBounds().toFloat().getCentre();
    // starfield
    for (auto& s : stars)
    {
        const float sx = c.x + s.x / s.z * (float) getWidth() * 0.3f;
        const float sy = c.y + s.y / s.z * (float) getHeight() * 0.3f;
        const float size = (1.0f - s.z) * 3.0f;
        g.setColour (juce::Colours::white.withAlpha (juce::jlimit (0.0f, 1.0f, 1.0f - s.z)));
        g.fillEllipse (sx, sy, size, size);
    }
    // slow neon pulse
    g.setGradientFill (juce::ColourGradient (p.magenta.withAlpha (0.08f + 0.1f * pulse), c.x, c.y, juce::Colours::transparentBlack, c.x, 0.0f, true));
    g.fillAll();

    const float w = std::min (760.0f, (float) getWidth() - 80.0f);
    const float x0 = c.x - w * 0.5f;
    wakeRect = {};
    g.saveState();   // keep the scrolling text above the control bar
    g.reduceClipRegion (getLocalBounds().withTrimmedBottom (52));
    for (size_t i = 0; i < items.size(); ++i)
    {
        const float y = itemY[i] - scroll;
        if (y > (float) getHeight() || y + itemH[i] < 0.0f || items[i].style == Style::Spacer)
            continue;
        const auto& it = items[i];
        auto text = it.scrambled ? glitch (it.text, rng) : it.text;
        juce::AttributedString a;
        a.append (text, fontFor (it.style), colourFor (it.style));
        a.setJustification (it.style == Style::SpeakerA || it.style == Style::SpeakerB ? juce::Justification::centredLeft : juce::Justification::centred);
        juce::TextLayout tl;
        tl.createLayout (a, w);
        auto area = juce::Rectangle<float> (x0, y, w, itemH[i]);
        if (it.style == Style::Title)
        {
            const auto glow = area.getCentre();
            g.setGradientFill (juce::ColourGradient (p.magenta.withAlpha (0.35f + 0.25f * pulse), glow.x, glow.y,
                                                     p.magenta.withAlpha (0.0f), glow.x + w * 0.45f, glow.y, true));
            g.fillEllipse (area.withSizeKeepingCentre (w * 0.9f, itemH[i] * 1.4f));
        }
        // fade at top and bottom edges
        const float fade = juce::jlimit (0.0f, 1.0f, std::min (y / 80.0f, ((float) getHeight() - 70.0f - y) / 90.0f));
        g.setOpacity (fade);
        tl.draw (g, area);
        g.setOpacity (1.0f);
        if (it.style == Style::WakeUp)
            wakeRect = area.withSizeKeepingCentre (220.0f, itemH[i]);
    }

    g.restoreState();

    // Grey the cat
    if (catState > 0)
    {
        const float s = 46.0f;
        const float bob = catState == 2 ? 0.0f : std::abs (std::sin ((float) time * 9.0f)) * 3.0f;
        const auto at = juce::Point<float> (catX, catTargetY - bob);
        const juce::Colour grey (0xff8a8a92), dark (0xff5a5a62);
        g.setColour (grey);
        g.fillEllipse (at.x - s * 0.45f, at.y - s * 0.55f, s * 0.9f, s * 0.5f);
        const float hx = at.x + s * 0.38f, hy = at.y - s * 0.72f;
        g.fillEllipse (hx - s * 0.2f, hy - s * 0.18f, s * 0.4f, s * 0.36f);
        juce::Path ears;
        ears.addTriangle (hx - s * 0.18f, hy - s * 0.08f, hx - s * 0.12f, hy - s * 0.33f, hx - s * 0.02f, hy - s * 0.12f);
        ears.addTriangle (hx + s * 0.18f, hy - s * 0.08f, hx + s * 0.12f, hy - s * 0.33f, hx + s * 0.02f, hy - s * 0.12f);
        g.fillPath (ears);
        g.setColour (juce::Colour (0xffd8f060));
        g.fillEllipse (hx - s * 0.12f, hy - s * 0.04f, s * 0.07f, s * 0.07f);
        g.fillEllipse (hx + s * 0.05f, hy - s * 0.04f, s * 0.07f, s * 0.07f);
        g.setColour (dark);
        juce::Path tail;
        tail.startNewSubPath (at.x - s * 0.4f, at.y - s * 0.4f);
        tail.quadraticTo (at.x - s * 0.8f, at.y - s * 0.9f, at.x - s * 0.55f + std::sin ((float) time * 3.0f) * 6.0f, at.y - s * 1.05f);
        g.strokePath (tail, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (catState != 2)
            for (int k = 0; k < 2; ++k) g.fillRect (at.x - s * 0.25f + (float) k * s * 0.4f, at.y - s * 0.1f, 4.0f, s * 0.1f + bob);
    }
}

void CreditsTab::mouseUp (const juce::MouseEvent& e)
{
    if (! wakeRect.isEmpty() && wakeRect.contains (e.position) && onWakeUp)
        onWakeUp();
}

void CreditsTab::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    scroll = juce::jlimit (-(float) getHeight(), totalHeight, scroll - w.deltaY * 200.0f);
}
} // namespace bgdj
