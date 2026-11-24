#include "Widgets.h"
#include "../PluginProcessor.h"

namespace bgdj
{
// ============================================================================
void KnobSlider::mouseDown (const juce::MouseEvent& e)
{
    intercepting = onMouseDownIntercept && onMouseDownIntercept (e);
    if (! intercepting)
        Slider::mouseDown (e);
}
void KnobSlider::mouseDrag (const juce::MouseEvent& e)
{
    if (intercepting) { if (onModDrag) onModDrag (e); }
    else Slider::mouseDrag (e);
}
void KnobSlider::mouseUp (const juce::MouseEvent& e)
{
    if (intercepting) { if (onModDragEnd) onModDragEnd (e); intercepting = false; }
    else Slider::mouseUp (e);
}

// ============================================================================
Knob::Knob (BassGodsProcessor& p, const juce::String& id, const juce::String& label, juce::Colour accent)
    : proc (p), paramID (id), labelText (label)
{
    auto* param = proc.apvts.getParameter (id);
    jassert (param != nullptr);
    dest = params::modDestIndexForParam (id);

    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr, 600);
    if (accent != juce::Colour())
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, slider);
    if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));

    slider.setTooltip (param->getName (64) + (dest >= 0 ? "\nDrag a mod source here. Alt/Option-drag to set mod depth. Right-click for MIDI learn." : "\nRight-click for MIDI learn."));

    slider.onMouseDownIntercept = [this] (const juce::MouseEvent& e)
    {
        if (e.mods.isPopupMenu()) { showMenu(); return true; }
        SharedSettings s;
        const int fine = s->fineModifier();
        const bool fineDown = (fine == 0 && e.mods.isShiftDown()) || (fine == 1 && e.mods.isCommandDown()) || (fine == 2 && e.mods.isAltDown() && dest < 0);
        slider.setMouseDragSensitivity (fineDown ? 1600 : 250);
        if (e.mods.isAltDown() && dest >= 0)
        {
            const auto slots = proc.modMatrix.slotsForDest (dest);
            if (! slots.isEmpty())
            {
                modSlotDragging = slots.getLast();
                modDragStart = proc.modMatrix.getSlot (modSlotDragging).amount;
                return true;
            }
        }
        return false;
    };
    slider.onModDrag = [this] (const juce::MouseEvent& e)
    {
        if (modSlotDragging < 0) return;
        auto s = proc.modMatrix.getSlot (modSlotDragging);
        s.amount = juce::jlimit (-1.0f, 1.0f, modDragStart - (float) e.getDistanceFromDragStartY() / 150.0f);
        proc.modMatrix.setSlot (modSlotDragging, s);
        repaint();
    };
    slider.onModDragEnd = [this] (const juce::MouseEvent&) { modSlotDragging = -1; };

    SharedSettings s;
    s->addChangeListener (this);
    applyBehaviour();
    startTimerHz (10);
}

Knob::~Knob()
{
    SharedSettings s;
    s->removeChangeListener (this);
}

void Knob::applyBehaviour()
{
    SharedSettings s;
    switch (s->knobMode())
    {
        case 0:  slider.setSliderStyle (juce::Slider::Rotary); break;
        case 2:  slider.setSliderStyle (juce::Slider::RotaryHorizontalDrag); break;
        default: slider.setSliderStyle (juce::Slider::RotaryVerticalDrag); break;
    }
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
}

void Knob::changeListenerCallback (juce::ChangeBroadcaster*) { applyBehaviour(); }

void Knob::timerCallback()
{
    const int mv = proc.modMatrix.changeCounter.load();
    const int mi = proc.midiMapVersion.load();
    if (mv != lastMatrixVersion || mi != lastMidiVersion)
    {
        lastMatrixVersion = mv;
        lastMidiVersion = mi;
        const int cc = proc.getCcForParam (paramID);
        ccText = cc >= 0 ? "CC" + juce::String (cc) : juce::String();
        repaint();
    }
}

juce::Rectangle<float> Knob::knobArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromBottom (14.0f);
    const float s = std::min (r.getWidth(), r.getHeight());
    return r.withSizeKeepingCentre (s, s);
}

void Knob::resized()
{
    slider.setBounds (knobArea().reduced (4.0f).toNearestInt());
}

void Knob::paint (juce::Graphics& g)
{
    const auto& p = theme::get();
    auto r = getLocalBounds().toFloat();
    g.setColour (p.textDim);
    g.setFont (theme::font (10.5f, true));
    g.drawFittedText (labelText.toUpperCase(), r.removeFromBottom (13.0f).toNearestInt(), juce::Justification::centred, 1);
    if (ccText.isNotEmpty())
    {
        g.setColour (p.yellow);
        g.setFont (theme::font (9.0f));
        g.drawText (ccText, getLocalBounds().removeFromTop (10), juce::Justification::topRight);
    }
}

void Knob::paintOverChildren (juce::Graphics& g)
{
    const auto& p = theme::get();
    const auto area = knobArea().reduced (1.0f);
    if (dragHover)
    {
        g.setColour (p.yellow.withAlpha (0.35f));
        g.fillEllipse (area);
    }
    if (proc.isLearning())
        return;
    if (dest < 0)
        return;

    const auto slots = proc.modMatrix.slotsForDest (dest);
    if (slots.isEmpty())
        return;
    const float start = juce::MathConstants<float>::pi * 1.25f, end = juce::MathConstants<float>::pi * 2.75f;
    auto* rp = proc.apvts.getParameter (paramID);
    const float base = rp->getValue();
    float radius = area.getWidth() * 0.5f + 1.0f;
    const auto c = area.getCentre();
    for (int idx : slots)
    {
        const auto s = proc.modMatrix.getSlot (idx);
        const float a0 = start + base * (end - start);
        const float lo = s.bipolar ? base - std::abs (s.amount) * s.output : base;
        const float tgt = juce::jlimit (0.0f, 1.0f, base + s.amount * s.output);
        const float aFrom = start + juce::jlimit (0.0f, 1.0f, std::min (lo, tgt)) * (end - start);
        const float aTo = start + std::max (tgt, s.bipolar ? juce::jlimit (0.0f, 1.0f, base + std::abs (s.amount) * s.output) : base) * (end - start);
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, radius, radius, 0.0f, std::min (aFrom, aTo), std::max (aFrom, aTo), true);
        g.setColour (theme::modSourceColour (s.src));
        g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::ignoreUnused (a0);
        radius += 3.0f;
    }
}

bool Knob::isInterestedInDragSource (const SourceDetails& d)
{
    return dest >= 0 && d.description.toString().startsWith ("modsrc:");
}

void Knob::itemDropped (const SourceDetails& d)
{
    dragHover = false;
    const int src = d.description.toString().fromFirstOccurrenceOf ("modsrc:", false, false).getIntValue();
    if (src > 0 && proc.modMatrix.addRouting (src, dest, 0.3f) < 0)
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Mod matrix full",
                                                "All 64 modulation slots are in use. Remove one in the matrix first.");
    repaint();
}

void Knob::showMenu()
{
    juce::PopupMenu m;
    const int cc = proc.getCcForParam (paramID);
    m.addSectionHeader (proc.apvts.getParameter (paramID)->getName (64));
    m.addItem (1, "MIDI Learn");
    m.addItem (2, "Clear MIDI mapping" + (cc >= 0 ? " (CC " + juce::String (cc) + ")" : juce::String()), cc >= 0);
    m.addItem (3, "Reset to default");
    if (dest >= 0)
    {
        const auto slots = proc.modMatrix.slotsForDest (dest);
        juce::PopupMenu mods;
        for (int s : slots)
        {
            const auto sl = proc.modMatrix.getSlot (s);
            mods.addItem (100 + s, "Remove " + params::modSourceNames()[sl.src] + " (" + juce::String (juce::roundToInt (sl.amount * 100.0f)) + "%)");
        }
        m.addSubMenu ("Modulation", mods, ! slots.isEmpty());
        juce::PopupMenu add;
        for (int src = 1; src < (int) ModSource::Count; ++src)
            add.addItem (300 + src, params::modSourceNames()[src]);
        m.addSubMenu ("Add modulation from", add);
    }

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
    {
        if (r == 1) proc.startMidiLearn (paramID);
        else if (r == 2) proc.clearMidiMapping (paramID);
        else if (r == 3) { auto* p = proc.apvts.getParameter (paramID); p->setValueNotifyingHost (p->getDefaultValue()); }
        else if (r >= 300) proc.modMatrix.addRouting (r - 300, dest, 0.3f);
        else if (r >= 100) proc.modMatrix.clearSlot (r - 100);
        repaint();
    });
}

// ============================================================================
ModSourceChip::ModSourceChip (int s, const juce::String& t) : source (s), text (t)
{
    setTooltip ("Drag onto any knob to modulate it with " + t);
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
}

void ModSourceChip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const auto c = theme::modSourceColour (source);
    g.setColour (c.withAlpha (0.18f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (c);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
    g.fillEllipse (r.getX() + 4.0f, r.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour (theme::get().text);
    g.setFont (theme::font (10.5f, true));
    g.drawText (text, r.withTrimmedLeft (12.0f), juce::Justification::centred);
}

void ModSourceChip::mouseDrag (const juce::MouseEvent&)
{
    if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
        if (! c->isDragAndDropActive())
            c->startDragging ("modsrc:" + juce::String (source), this);
}

// ============================================================================
ParamToggle::ParamToggle (BassGodsProcessor& p, const juce::String& id, const juce::String& text)
{
    setButtonText (text);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id, *this);
    setTooltip (p.apvts.getParameter (id)->getName (64));
}

ParamChoice::ParamChoice (BassGodsProcessor& p, const juce::String& id)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (id)))
        addItemList (c->choices, 1);
    else if (auto* i = dynamic_cast<juce::AudioParameterInt*> (p.apvts.getParameter (id)))
        for (int v = i->getRange().getStart(); v <= i->getRange().getEnd(); ++v)
            addItem (juce::String (v), v - i->getRange().getStart() + 1);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id, *this);
    setTooltip (p.apvts.getParameter (id)->getName (64));
}

void layoutRow (juce::Rectangle<int> area, std::initializer_list<juce::Component*> comps, int gap)
{
    const int n = (int) comps.size();
    if (n == 0) return;
    const int w = (area.getWidth() - gap * (n - 1)) / n;
    for (auto* c : comps)
    {
        if (c != nullptr) c->setBounds (area.removeFromLeft (w));
        area.removeFromLeft (gap);
    }
}
} // namespace bgdj
