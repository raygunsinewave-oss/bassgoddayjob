#include "VisualizerTab.h"
#include "../../PluginProcessor.h"

namespace bgdj
{
namespace
{
    struct VizWindow : public juce::DocumentWindow
    {
        VizWindow (const juce::String& name, std::unique_ptr<VizPanel> panel, std::function<void (VizWindow*)> onCloseFn)
            : DocumentWindow (name, theme::get().bg, DocumentWindow::closeButton), onClose (std::move (onCloseFn))
        {
            setUsingNativeTitleBar (true);
            setContentOwned (panel.release(), false);
            setResizable (true, false);
            setResizeLimits (240, 160, 3000, 2000);
            centreWithSize (640, 400);
            setAlwaysOnTop (true);
            setVisible (true);
        }
        void closeButtonPressed() override { if (onClose) onClose (this); }
        std::function<void (VizWindow*)> onClose;
    };
}

VisualizerTab::VisualizerTab (BassGodsProcessor& p, AnalysisData& a) : proc (p), analysis (a)
{
    for (int i = 0; i < viz::names().size(); ++i)
    {
        auto panel = viz::create (i, proc, analysis);
        panel->onDetach = [this, i] { detach (i); };
        addAndMakeVisible (*panel);
        panels.push_back (std::move (panel));
    }
    // three rows with draggable dividers (fully resizable)
    rowsLayout.setItemLayout (0, 80, -1.0, -0.36);
    rowsLayout.setItemLayout (1, 6, 6, 6);
    rowsLayout.setItemLayout (2, 80, -1.0, -0.32);
    rowsLayout.setItemLayout (3, 6, 6, 6);
    rowsLayout.setItemLayout (4, 80, -1.0, -0.32);
    addAndMakeVisible (rowBar1);
    addAndMakeVisible (rowBar2);
}

VisualizerTab::~VisualizerTab()
{
    windows.clear();
}

void VisualizerTab::detach (int index)
{
    auto panel = viz::create (index, proc, analysis);
    auto* w = new VizWindow ("BASSGODSDAYJOB - " + viz::names()[index], std::move (panel), [this] (VizWindow* win)
    {
        juce::MessageManager::callAsync ([this, win] { windows.removeObject (win); });
    });
    windows.add (w);
}

void VisualizerTab::paint (juce::Graphics& g)
{
    g.fillAll (theme::get().bg);
}

void VisualizerTab::resized()
{
    auto r = getLocalBounds().reduced (8);
    // dummy components so the layout manager can size the three rows
    juce::Component rowA, rowB, rowC;
    juce::Component* comps[] = { &rowA, &rowBar1, &rowB, &rowBar2, &rowC };
    rowsLayout.layOutComponents (comps, 5, r.getX(), r.getY(), r.getWidth(), r.getHeight(), true, true);

    auto layoutRow = [] (juce::Rectangle<int> area, std::initializer_list<std::pair<VizPanel*, float>> items)
    {
        const int total = area.getWidth();
        for (auto& [panel, frac] : items)
        {
            panel->setBounds (area.removeFromLeft ((int) (total * frac)).reduced (3));
        }
    };
    layoutRow (rowA.getBounds(), { { panels[0].get(), 0.45f }, { panels[1].get(), 0.3f }, { panels[3].get(), 0.25f } });
    layoutRow (rowB.getBounds(), { { panels[2].get(), 0.32f }, { panels[4].get(), 0.24f }, { panels[5].get(), 0.14f }, { panels[6].get(), 0.3f } });
    layoutRow (rowC.getBounds(), { { panels[7].get(), 0.25f }, { panels[8].get(), 0.5f }, { panels[9].get(), 0.25f } });
}
} // namespace bgdj
