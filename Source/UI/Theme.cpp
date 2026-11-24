#include "Theme.h"
#include "../Common/Parameters.h"

namespace bgdj
{
namespace theme
{
static Palette makeNeon()
{
    Palette p;
    p.bg = juce::Colour (0xff0c0812); p.panel = juce::Colour (0xff1a1222); p.panelHi = juce::Colour (0xff261a33);
    p.outline = juce::Colour (0xff3b2a4d); p.text = juce::Colour (0xfff4ecff); p.textDim = juce::Colour (0xff9c8bb0);
    p.magenta = juce::Colour (0xffff2bd6); p.pink = juce::Colour (0xffff5fa2); p.cyan = juce::Colour (0xff2ef2ff);
    p.teal = juce::Colour (0xff19d3b0); p.yellow = juce::Colour (0xffffd84a); p.warn = juce::Colour (0xffff4d4d);
    p.good = juce::Colour (0xff4dff9a); p.knobBody = juce::Colour (0xff2a1f36); p.track = juce::Colour (0xff3a2e48);
    return p;
}
static Palette makeDark()
{
    Palette p = makeNeon();
    p.bg = juce::Colour (0xff111214); p.panel = juce::Colour (0xff1b1d21); p.panelHi = juce::Colour (0xff25282e);
    p.outline = juce::Colour (0xff33363d); p.textDim = juce::Colour (0xff8c9099); p.knobBody = juce::Colour (0xff24272c);
    p.track = juce::Colour (0xff353840); p.magenta = juce::Colour (0xffd64cc0); p.cyan = juce::Colour (0xff4cc9d6);
    return p;
}
static Palette makeLight()
{
    Palette p;
    p.bg = juce::Colour (0xffe9e4ee); p.panel = juce::Colour (0xfff7f3fa); p.panelHi = juce::Colour (0xffffffff);
    p.outline = juce::Colour (0xffc9bfd4); p.text = juce::Colour (0xff1d1426); p.textDim = juce::Colour (0xff6a5d78);
    p.magenta = juce::Colour (0xffc4139f); p.pink = juce::Colour (0xffd62e74); p.cyan = juce::Colour (0xff0a8fa0);
    p.teal = juce::Colour (0xff0b8f77); p.yellow = juce::Colour (0xffb88a00); p.warn = juce::Colour (0xffd02020);
    p.good = juce::Colour (0xff109050); p.knobBody = juce::Colour (0xffe2dbe9); p.track = juce::Colour (0xffcfc5da);
    return p;
}
static Palette makeHighContrast()
{
    Palette p;
    p.bg = juce::Colours::black; p.panel = juce::Colour (0xff0a0a0a); p.panelHi = juce::Colour (0xff1a1a1a);
    p.outline = juce::Colours::white; p.text = juce::Colours::white; p.textDim = juce::Colour (0xffdddddd);
    p.magenta = juce::Colour (0xffff00ff); p.pink = juce::Colour (0xffff66cc); p.cyan = juce::Colour (0xff00ffff);
    p.teal = juce::Colour (0xff00ff99); p.yellow = juce::Colour (0xffffff00); p.warn = juce::Colour (0xffff3030);
    p.good = juce::Colour (0xff00ff00); p.knobBody = juce::Colour (0xff202020); p.track = juce::Colour (0xff606060);
    return p;
}

static Palette current = makeNeon();

const juce::StringArray& names()
{
    static const juce::StringArray n { "Neon Club", "Dark", "Light", "High Contrast" };
    return n;
}

void set (const juce::String& name)
{
    if (name == "Dark") current = makeDark();
    else if (name == "Light") current = makeLight();
    else if (name == "High Contrast") current = makeHighContrast();
    else current = makeNeon();
}

const Palette& get() { return current; }

juce::Colour modSourceColour (int s)
{
    const auto& p = current;
    if (s >= (int) ModSource::Env1 && s <= (int) ModSource::Env4)     return p.cyan;
    if (s >= (int) ModSource::Lfo1 && s <= (int) ModSource::Lfo8)     return p.magenta;
    if (s >= (int) ModSource::Macro1 && s <= (int) ModSource::Macro4) return p.yellow;
    if (s == (int) ModSource::Velocity || s == (int) ModSource::NoteRandom) return p.teal;
    return p.pink;
}

juce::Font font (float h, bool bold)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

juce::Font mono (float h)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain));
}
} // namespace theme

// ============================================================================
BgdjLookAndFeel::BgdjLookAndFeel() { refreshColours(); }

void BgdjLookAndFeel::refreshColours()
{
    const auto& p = theme::get();
    setColour (juce::ResizableWindow::backgroundColourId, p.bg);
    setColour (juce::Slider::textBoxTextColourId, p.text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, p.text);
    setColour (juce::TextButton::buttonColourId, p.panelHi);
    setColour (juce::TextButton::buttonOnColourId, p.magenta.withAlpha (0.8f));
    setColour (juce::TextButton::textColourOffId, p.text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, p.panelHi);
    setColour (juce::ComboBox::textColourId, p.text);
    setColour (juce::ComboBox::outlineColourId, p.outline);
    setColour (juce::ComboBox::arrowColourId, p.cyan);
    setColour (juce::PopupMenu::backgroundColourId, p.panel);
    setColour (juce::PopupMenu::textColourId, p.text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, p.magenta.withAlpha (0.5f));
    setColour (juce::PopupMenu::headerTextColourId, p.cyan);
    setColour (juce::TextEditor::backgroundColourId, p.bg);
    setColour (juce::TextEditor::textColourId, p.text);
    setColour (juce::TextEditor::outlineColourId, p.outline);
    setColour (juce::TextEditor::focusedOutlineColourId, p.cyan);
    setColour (juce::TextEditor::highlightColourId, p.magenta.withAlpha (0.4f));
    setColour (juce::CaretComponent::caretColourId, p.cyan);
    setColour (juce::ListBox::backgroundColourId, p.panel);
    setColour (juce::ScrollBar::thumbColourId, p.magenta.withAlpha (0.6f));
    setColour (juce::TooltipWindow::backgroundColourId, p.panelHi);
    setColour (juce::TooltipWindow::textColourId, p.text);
    setColour (juce::TooltipWindow::outlineColourId, p.cyan);
    setColour (juce::ToggleButton::textColourId, p.text);
    setColour (juce::ToggleButton::tickColourId, p.cyan);
    setColour (juce::AlertWindow::backgroundColourId, p.panel);
    setColour (juce::AlertWindow::textColourId, p.text);
    setColour (juce::AlertWindow::outlineColourId, p.magenta);
    setColour (juce::BubbleComponent::backgroundColourId, p.panelHi);
    setColour (juce::BubbleComponent::outlineColourId, p.cyan);
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffeee8f5));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1b1424));
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, p.magenta.withAlpha (0.7f));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, p.cyan.withAlpha (0.35f));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff2a2233));
    setColour (juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
}

void BgdjLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto& p = theme::get();
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId) == juce::Colour()
                            ? p.cyan : s.findColour (juce::Slider::rotarySliderFillColourId);

    // track
    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, start, end, true);
    g.setColour (p.track);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc (bipolar params start from the centre)
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (start + end) * 0.5f : start;
    juce::Path val;
    val.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, std::min (from, angle), std::max (from, angle), true);
    g.setColour (accent.withAlpha (0.35f));
    g.strokePath (val, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // body
    const float br = r * 0.68f;
    juce::ColourGradient body (p.knobBody.brighter (0.25f), c.x - br, c.y - br, p.knobBody.darker (0.4f), c.x + br, c.y + br, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (p.outline);
    g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.0f);

    // pointer
    juce::Path ptr;
    ptr.addRoundedRectangle (-1.5f, -br * 0.95f, 3.0f, br * 0.55f, 1.5f);
    g.setColour (s.isEnabled() ? p.text : p.textDim);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c));
}

void BgdjLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    const auto& p = theme::get();
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const bool vert = style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical;
    g.setColour (p.track);
    if (vert)
    {
        auto t = r.withSizeKeepingCentre (6.0f, r.getHeight());
        g.fillRoundedRectangle (t, 3.0f);
        const bool bip = s.getMinimum() < 0.0;
        const float mid = bip ? r.getCentreY() : r.getBottom();
        g.setColour (p.magenta);
        g.fillRoundedRectangle (juce::Rectangle<float> (t.getX(), std::min (pos, mid), 6.0f, std::abs (mid - pos)), 3.0f);
        g.setColour (p.text);
        g.fillRoundedRectangle (r.getCentreX() - 9.0f, pos - 3.0f, 18.0f, 6.0f, 2.0f);
    }
    else
    {
        auto t = r.withSizeKeepingCentre (r.getWidth(), 4.0f);
        g.fillRoundedRectangle (t, 2.0f);
        g.setColour (p.cyan);
        g.fillRoundedRectangle (t.withRight (pos), 2.0f);
        g.setColour (p.text);
        g.fillEllipse (pos - 6.0f, r.getCentreY() - 6.0f, 12.0f, 12.0f);
    }
}

void BgdjLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto& p = theme::get();
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto base = on ? b.findColour (juce::TextButton::buttonOnColourId) : b.findColour (juce::TextButton::buttonColourId);
    if (over) base = base.brighter (0.15f);
    if (down) base = base.darker (0.2f);
    g.setColour (base);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? p.magenta : p.outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void BgdjLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const auto& p = theme::get();
    auto r = b.getLocalBounds().toFloat();
    auto box = r.removeFromLeft (std::min (r.getHeight(), 18.0f)).reduced (2.0f);
    g.setColour (b.getToggleState() ? p.cyan : p.track);
    g.fillRoundedRectangle (box, 3.0f);
    if (b.getToggleState())
    {
        g.setColour (p.cyan.withAlpha (0.35f));
        g.drawRoundedRectangle (box.expanded (1.5f), 4.0f, 2.0f);
    }
    g.setColour (over ? p.text : p.text.withAlpha (0.9f));
    g.setFont (theme::font (std::min (13.0f, r.getHeight() * 0.8f)));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().withTrimmedLeft (4), juce::Justification::centredLeft, 1);
}

void BgdjLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto& p = theme::get();
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? p.cyan : p.outline);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (p.cyan);
    g.fillPath (arrow);
}

juce::Font BgdjLookAndFeel::getComboBoxFont (juce::ComboBox& b)  { return theme::font (std::min (14.0f, (float) b.getHeight() * 0.62f)); }
juce::Font BgdjLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return theme::font (std::min (14.0f, (float) h * 0.55f), true); }
juce::Font BgdjLookAndFeel::getLabelFont (juce::Label& l)        { return theme::font (std::min (14.0f, (float) l.getHeight() * 0.8f)); }
juce::Font BgdjLookAndFeel::getPopupMenuFont()                   { return theme::font (14.0f); }

void BgdjLookAndFeel::drawTabButton (juce::TabBarButton& b, juce::Graphics& g, bool over, bool)
{
    const auto& p = theme::get();
    auto r = b.getLocalBounds().toFloat().reduced (2.0f, 3.0f);
    const bool front = b.isFrontTab();
    if (front)
    {
        juce::ColourGradient gr (p.magenta.withAlpha (0.55f), r.getX(), r.getY(), p.cyan.withAlpha (0.35f), r.getRight(), r.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (p.magenta);
        g.drawRoundedRectangle (r, 5.0f, 1.2f);
    }
    else if (over)
    {
        g.setColour (p.panelHi);
        g.fillRoundedRectangle (r, 5.0f);
    }
    g.setColour (front ? juce::Colours::white : p.textDim);
    g.setFont (theme::font (13.0f, true));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

int BgdjLookAndFeel::getTabButtonBestWidth (juce::TabBarButton& b, int)
{
    return juce::GlyphArrangement::getStringWidthInt (theme::font (13.0f, true), b.getButtonText()) + 36;
}

void BgdjLookAndFeel::drawTabAreaBehindFrontButton (juce::TabbedButtonBar&, juce::Graphics&, int, int) {}

juce::Label* BgdjLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setFont (theme::font (11.0f));
    return l;
}

void BgdjLookAndFeel::drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&, const juce::Rectangle<float>& body)
{
    const auto& p = theme::get();
    g.setColour (p.panelHi.withAlpha (0.95f));
    g.fillRoundedRectangle (body, 4.0f);
    g.setColour (p.cyan);
    g.drawRoundedRectangle (body, 4.0f, 1.0f);
}

void BgdjLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                     int thumbStart, int thumbSize, bool over, bool)
{
    const auto& p = theme::get();
    auto thumb = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize) : juce::Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour (p.magenta.withAlpha (over ? 0.8f : 0.5f));
    g.fillRoundedRectangle (thumb.toFloat().reduced (2.0f), 3.0f);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, juce::Colour accent)
{
    const auto& p = theme::get();
    juce::ColourGradient gr (p.panelHi, r.getX(), r.getY(), p.panel, r.getX(), r.getBottom(), false);
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (p.outline);
    g.drawRoundedRectangle (r, 7.0f, 1.0f);
    // worn sticker strip behind the title
    if (title.isNotEmpty())
    {
        auto strip = r.withHeight (20.0f).reduced (6.0f, 3.0f);
        const float tw = (float) juce::GlyphArrangement::getStringWidthInt (theme::font (12.0f, true), title) + 16.0f;
        auto sticker = strip.withWidth (tw);
        g.setColour (accent.withAlpha (0.85f));
        g.fillRoundedRectangle (sticker, 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        for (int i = 0; i < 4; ++i)   // scuffs
            g.fillEllipse (sticker.getX() + (float) ((i * 37) % (int) std::max (1.0f, tw)), sticker.getY() + (float) ((i * 5) % 9), 3.0f, 2.0f);
        g.setColour (juce::Colours::black);
        g.setFont (theme::font (12.0f, true));
        g.drawText (title, sticker, juce::Justification::centred);
    }
}
} // namespace bgdj
