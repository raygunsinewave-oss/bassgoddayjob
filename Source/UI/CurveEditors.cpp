#include "CurveEditors.h"
#include "../PluginProcessor.h"

namespace bgdj
{
CurveEditor::CurveEditor()
{
    colour = theme::get().magenta;
    startTimerHz (30);
    setTooltip ("Drag points. Double-click to add/remove. Alt/Option-drag a segment to bend it. Shift snaps to grid. Right-click for shapes.");
}

juce::Point<float> CurveEditor::toScreen (const CurvePoint& p) const
{
    auto r = plotArea();
    return { r.getX() + p.x * r.getWidth(), r.getBottom() - p.y * r.getHeight() };
}

CurvePoint CurveEditor::fromScreen (juce::Point<float> s, bool snap) const
{
    auto r = plotArea();
    float x = juce::jlimit (0.0f, 1.0f, (s.x - r.getX()) / r.getWidth());
    float y = juce::jlimit (0.0f, 1.0f, (r.getBottom() - s.y) / r.getHeight());
    if (snap)
    {
        x = std::round (x * (float) gridX) / (float) gridX;
        y = std::round (y * (float) gridY) / (float) gridY;
    }
    return { x, y, 0.0f };
}

int CurveEditor::hitPoint (juce::Point<float> s) const
{
    for (int i = 0; i < (int) working.size(); ++i)
        if (toScreen (working[(size_t) i]).getDistanceFrom (s) < 7.0f)
            return i;
    return -1;
}

void CurveEditor::paint (juce::Graphics& g)
{
    const auto& p = theme::get();
    auto r = plotArea();
    g.setColour (p.bg.withAlpha (0.8f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
    g.setColour (p.outline.withAlpha (0.5f));
    for (int i = 1; i < gridX; ++i)
        g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / (float) gridX), r.getY(), r.getBottom());
    for (int i = 1; i < gridY; ++i)
        g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / (float) gridY), r.getX(), r.getRight());

    if (dragIndex < 0 && bendIndex < 0 && getPoints)
        working = getPoints();
    if (working.empty())
        return;

    const auto table = renderCurve (working);
    juce::Path path, fill;
    for (int i = 0; i <= kLfoTableSize; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) kLfoTableSize;
        const float y = r.getBottom() - table.v[(size_t) i] * r.getHeight();
        if (i == 0) { path.startNewSubPath (x, y); fill.startNewSubPath (x, r.getBottom()); }
        else path.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (r.getRight(), r.getBottom());
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (colour.withAlpha (0.35f), 0, r.getY(), colour.withAlpha (0.02f), 0, r.getBottom(), false));
    g.fillPath (fill);
    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    for (int i = 0; i < (int) working.size(); ++i)
    {
        const auto s = toScreen (working[(size_t) i]);
        g.setColour (i == dragIndex ? p.yellow : p.text);
        g.fillEllipse (s.x - 3.5f, s.y - 3.5f, 7.0f, 7.0f);
    }

    if (getPlayhead)
    {
        const float ph = getPlayhead();
        if (ph >= 0.0f)
        {
            const float x = r.getX() + ph * r.getWidth();
            g.setColour (p.cyan.withAlpha (0.8f));
            g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
            g.fillEllipse (x - 3.0f, r.getBottom() - table.lookup (ph) * r.getHeight() - 3.0f, 6.0f, 6.0f);
        }
    }
}

void CurveEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! getPoints) return;
    working = getPoints();
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        if (addMenuItems) addMenuItems (m);
        m.addSeparator();
        m.addItem (9001, "Flip vertical");
        m.addItem (9002, "Reverse");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int res)
        {
            if (res == 9001 || res == 9002)
            {
                auto pts = getPoints();
                for (auto& pt : pts) { if (res == 9001) pt.y = 1.0f - pt.y; else pt.x = 1.0f - pt.x; }
                std::sort (pts.begin(), pts.end(), [] (auto& a, auto& b) { return a.x < b.x; });
                setPoints (pts);
            }
            else if (res > 0 && onMenuResult)
                onMenuResult (res);
            repaint();
        });
        return;
    }
    dragIndex = hitPoint (e.position);
    if (dragIndex < 0 && e.mods.isAltDown())
    {
        const auto pos = fromScreen (e.position, false);
        for (int i = 1; i < (int) working.size(); ++i)
            if (pos.x <= working[(size_t) i].x) { bendIndex = i; bendStart = working[(size_t) i].curve; break; }
    }
}

void CurveEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragIndex >= 0)
    {
        auto np = fromScreen (e.position, e.mods.isShiftDown());
        auto& pt = working[(size_t) dragIndex];
        const float lo = dragIndex > 0 ? working[(size_t) dragIndex - 1].x : 0.0f;
        const float hi = dragIndex + 1 < (int) working.size() ? working[(size_t) dragIndex + 1].x : 1.0f;
        pt.x = dragIndex == 0 ? 0.0f : (dragIndex == (int) working.size() - 1 ? 1.0f : juce::jlimit (lo, hi, np.x));
        pt.y = np.y;
        setPoints (working);
        repaint();
    }
    else if (bendIndex > 0)
    {
        const auto& a = working[(size_t) bendIndex - 1];
        const auto& b = working[(size_t) bendIndex];
        const float dir = b.y >= a.y ? 1.0f : -1.0f;
        working[(size_t) bendIndex].curve = juce::jlimit (-1.0f, 1.0f, bendStart + dir * (float) e.getDistanceFromDragStartY() / 100.0f);
        setPoints (working);
        repaint();
    }
}

void CurveEditor::mouseUp (const juce::MouseEvent&) { dragIndex = bendIndex = -1; }

void CurveEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! getPoints) return;
    working = getPoints();
    const int hit = hitPoint (e.position);
    if (hit > 0 && hit < (int) working.size() - 1)
        working.erase (working.begin() + hit);
    else if (hit < 0)
    {
        auto np = fromScreen (e.position, e.mods.isShiftDown());
        working.push_back (np);
        std::sort (working.begin(), working.end(), [] (auto& a, auto& b) { return a.x < b.x; });
    }
    setPoints (working);
    repaint();
}

// ============================================================================
EnvelopeView::EnvelopeView (BassGodsProcessor& p, int e) : proc (p), env (e) { startTimerHz (30); }

void EnvelopeView::paint (juce::Graphics& g)
{
    const auto& pal = theme::get();
    auto r = getLocalBounds().toFloat().reduced (4.0f);
    g.setColour (pal.bg.withAlpha (0.8f));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);

    const auto pre = params::envPrefix (env);
    auto v = [this, &pre] (const char* id) { return proc.apvts.getRawParameterValue (pre + id)->load(); };
    const float a = v ("att"), d = v ("dec"), s = v ("sus"), rl = v ("rel");
    const float ac = v ("acurve"), dc = v ("dcurve"), rc = v ("rcurve");
    const float total = a + d + rl + 300.0f;
    const float holdW = 300.0f / total * r.getWidth();
    auto xOf = [&] (float ms) { return r.getX() + ms / total * r.getWidth(); };

    juce::Path path;
    path.startNewSubPath (r.getX(), r.getBottom());
    for (int i = 1; i <= 40; ++i)
    {
        const float t = (float) i / 40.0f;
        path.lineTo (xOf (a * t), r.getBottom() - bendSegment (t, ac) * r.getHeight());
    }
    for (int i = 1; i <= 40; ++i)
    {
        const float t = (float) i / 40.0f;
        const float lvl = s + (1.0f - s) * (1.0f - bendSegment (t, -dc));
        path.lineTo (xOf (a + d * t), r.getBottom() - lvl * r.getHeight());
    }
    path.lineTo (xOf (a + d) + holdW, r.getBottom() - s * r.getHeight());
    for (int i = 1; i <= 40; ++i)
    {
        const float t = (float) i / 40.0f;
        const float lvl = s * (1.0f - bendSegment (t, -rc));
        path.lineTo (xOf (a + d) + holdW + rl / total * r.getWidth() * t, r.getBottom() - lvl * r.getHeight());
    }
    g.setColour (pal.cyan.withAlpha (0.15f));
    juce::Path fill (path);
    fill.lineTo (r.getRight(), r.getBottom());
    fill.closeSubPath();
    g.fillPath (fill);
    g.setColour (pal.cyan);
    g.strokePath (path, juce::PathStrokeType (2.0f));

    const float lvl = proc.engine.uiEnvLevel[env].load();
    g.setColour (pal.yellow);
    g.fillRect (r.getRight() - 4.0f, r.getBottom() - lvl * r.getHeight(), 4.0f, lvl * r.getHeight());
}

// ============================================================================
VelocityCurveView::VelocityCurveView (BassGodsProcessor& p) : proc (p)
{
    setTooltip ("Velocity curve: drag up for a softer touch, down for a harder one. Double-click resets.");
}

void VelocityCurveView::paint (juce::Graphics& g)
{
    const auto& pal = theme::get();
    auto r = getLocalBounds().toFloat().reduced (4.0f);
    g.setColour (pal.bg);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
    const float c = proc.apvts.getRawParameterValue ("vel_curve")->load();
    const float e = std::pow (3.0f, -c);
    juce::Path p;
    for (int i = 0; i <= 50; ++i)
    {
        const float x = (float) i / 50.0f;
        const auto pt = juce::Point<float> (r.getX() + x * r.getWidth(), r.getBottom() - std::pow (x, e) * r.getHeight());
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    g.setColour (pal.outline);
    g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);
    g.setColour (pal.teal);
    g.strokePath (p, juce::PathStrokeType (2.0f));
    g.setColour (pal.textDim);
    g.setFont (theme::font (10.0f));
    g.drawText ("VELOCITY IN", r, juce::Justification::bottomRight);
}

void VelocityCurveView::mouseDown (const juce::MouseEvent&)
{
    startValue = proc.apvts.getParameter ("vel_curve")->getValue();
    proc.apvts.getParameter ("vel_curve")->beginChangeGesture();
}

void VelocityCurveView::mouseDrag (const juce::MouseEvent& e)
{
    auto* p = proc.apvts.getParameter ("vel_curve");
    p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, startValue - (float) e.getDistanceFromDragStartY() / 200.0f));
    repaint();
}

void VelocityCurveView::mouseUp (const juce::MouseEvent&)
{
    proc.apvts.getParameter ("vel_curve")->endChangeGesture();
}

void VelocityCurveView::mouseDoubleClick (const juce::MouseEvent&)
{
    auto* p = proc.apvts.getParameter ("vel_curve");
    p->setValueNotifyingHost (p->getDefaultValue());
    repaint();
}
} // namespace bgdj
