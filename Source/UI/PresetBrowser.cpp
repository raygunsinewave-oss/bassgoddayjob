#include "PresetBrowser.h"
#include "../PluginProcessor.h"

namespace bgdj
{
void PresetBrowser::StringListModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel)
{
    const auto& p = theme::get();
    if (sel || row == selected)
    {
        g.setColour (p.magenta.withAlpha (0.35f));
        g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
    }
    g.setColour (p.text);
    g.setFont (theme::font (13.0f, row == 0));
    g.drawText (items[row], 8, 0, w - 10, h, juce::Justification::centredLeft);
}

PresetBrowser::PresetBrowser (BassGodsProcessor& p) : proc (p)
{
    addAndMakeVisible (search);
    search.setTextToShowWhenEmpty ("Search names, tags, macros...", theme::get().textDim);
    search.onTextChange = [this] { filter.search = search.getText(); showingSimilar = false; refresh(); };
    search.onEscapeKey = [this] { if (onClose) onClose(); };

    addAndMakeVisible (list);
    list.setRowHeight (26);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);

    bankList.setModel (&bankModel);
    charList.setModel (&charModel);
    for (auto* l : { &bankList, &charList })
    {
        addAndMakeVisible (l);
        l->setRowHeight (22);
        l->setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    }
    bankModel.onSelect = [this] (int row) { filter.bank = row == 0 ? juce::String() : bankModel.items[row]; showingSimilar = false; bankList.repaint(); refresh(); };
    charModel.onSelect = [this] (int row) { filter.character = row == 0 ? juce::String() : charModel.items[row]; showingSimilar = false; charList.repaint(); refresh(); };

    addAndMakeVisible (favOnly);
    favOnly.onClick = [this] { filter.favouritesOnly = favOnly.getToggleState(); refresh(); };
    addAndMakeVisible (minRating);
    minRating.addItemList ({ "Any rating", "1+ stars", "2+ stars", "3+ stars", "4+ stars", "5 stars" }, 1);
    minRating.setSelectedItemIndex (0, juce::dontSendNotification);
    minRating.onChange = [this] { filter.minRating = minRating.getSelectedItemIndex(); refresh(); };

    for (auto* b : { &loadBtn, &similarBtn, &randomBtn, &importBtn, &rescanBtn, &closeBtn, &clearBtn })
        addAndMakeVisible (b);
    loadBtn.onClick = [this] { const int r = list.getSelectedRow(); if (juce::isPositiveAndBelow (r, (int) shown.size())) proc.presets->load (shown[(size_t) r]); };
    similarBtn.setTooltip ("Show presets similar to the selected (or current) one");
    similarBtn.onClick = [this]
    {
        int idx = list.getSelectedRow() >= 0 && list.getSelectedRow() < (int) shown.size() ? shown[(size_t) list.getSelectedRow()] : proc.presets->getCurrentIndex();
        if (idx < 0) return;
        shown = proc.presets->similarTo (idx, 20);
        shown.insert (shown.begin(), idx);
        showingSimilar = true;
        count.setText ("Similar to \"" + proc.presets->all()[(size_t) idx].data.name + "\"", juce::dontSendNotification);
        list.updateContent();
        list.repaint();
    };
    randomBtn.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Whole patch");
        m.addItem (2, "Oscillators");
        m.addItem (3, "Filters");
        m.addItem (4, "Envelopes");
        m.addItem (5, "LFOs (+ a couple of mod routings)");
        m.addItem (6, "FX rack");
        m.addItem (7, "Macros");
        m.addSeparator();
        m.addItem (8, "Gentle nudge (whole patch, 20%)");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&randomBtn), [this] (int r)
        {
            static const char* sections[] = { "", "all", "osc", "filter", "env", "lfo", "fx", "macro", "all" };
            if (r > 0) proc.presets->randomize (sections[r], r == 8 ? 0.2f : 1.0f);
        });
    };
    importBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Import preset", juce::File(), "*.bgdjpreset");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            if (fc.getResult().existsAsFile())
                if (auto err = proc.presets->importFile (fc.getResult()); err.isNotEmpty())
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Import failed", err);
        });
    };
    rescanBtn.onClick = [this] { proc.presets->rescan(); };
    closeBtn.onClick = [this] { if (onClose) onClose(); };
    clearBtn.onClick = [this]
    {
        filter = {};
        search.clear();
        favOnly.setToggleState (false, juce::dontSendNotification);
        minRating.setSelectedItemIndex (0, juce::dontSendNotification);
        bankModel.selected = charModel.selected = 0;
        showingSimilar = false;
        refresh();
    };
    addAndMakeVisible (count);
    count.setFont (theme::font (12.0f));

    proc.presets->addChangeListener (this);
    refresh();
}

PresetBrowser::~PresetBrowser()
{
    proc.presets->removeChangeListener (this);
    list.setModel (nullptr);
    bankList.setModel (nullptr);
    charList.setModel (nullptr);
}

void PresetBrowser::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (! showingSimilar) refresh();
    list.repaint();
}

void PresetBrowser::visibilityChanged()
{
    if (isVisible())
    {
        search.grabKeyboardFocus();
        refresh();
        const auto it = std::find (shown.begin(), shown.end(), proc.presets->getCurrentIndex());
        if (it != shown.end())
        {
            const int row = (int) (it - shown.begin());
            list.selectRow (row);
            list.scrollToEnsureRowIsOnscreen (row);
        }
    }
}

void PresetBrowser::refresh()
{
    bankModel.items = proc.presets->banks();
    bankModel.items.insert (0, "All banks");
    charModel.items = proc.presets->characters();
    charModel.items.insert (0, "All characters");
    bankList.updateContent();
    charList.updateContent();
    shown = proc.presets->filtered (filter);
    count.setText (juce::String (shown.size()) + " of " + juce::String (proc.presets->all().size()) + " presets", juce::dontSendNotification);
    list.updateContent();
    list.repaint();
}

void PresetBrowser::resized()
{
    auto r = getLocalBounds().reduced (24);
    r.removeFromTop (28);
    auto top = r.removeFromTop (32);
    closeBtn.setBounds (top.removeFromRight (80));
    top.removeFromRight (8);
    clearBtn.setBounds (top.removeFromRight (110));
    top.removeFromRight (8);
    search.setBounds (top);
    r.removeFromTop (10);

    auto left = r.removeFromLeft (230);
    favOnly.setBounds (left.removeFromTop (24));
    minRating.setBounds (left.removeFromTop (26).reduced (0, 2));
    left.removeFromTop (8);
    bankList.setBounds (left.removeFromTop (left.getHeight() / 2 - 4));
    left.removeFromTop (8);
    charList.setBounds (left);
    r.removeFromLeft (14);

    auto bottom = r.removeFromBottom (34);
    layoutRow (bottom.removeFromRight (560), { &loadBtn, &similarBtn, &randomBtn, &importBtn, &rescanBtn }, 8);
    count.setBounds (bottom);
    r.removeFromBottom (8);
    list.setBounds (r);
}

void PresetBrowser::paint (juce::Graphics& g)
{
    const auto& p = theme::get();
    g.fillAll (p.bg.withAlpha (0.97f));
    drawPanel (g, getLocalBounds().toFloat().reduced (8.0f), "PRESET BROWSER", p.magenta);
    auto lr = list.getBounds().toFloat();
    g.setColour (p.panel);
    g.fillRoundedRectangle (lr.expanded (4.0f), 6.0f);
    g.setColour (p.textDim);
    g.setFont (theme::font (11.0f, true));
    g.drawText ("NAME", lr.getX() + 30, lr.getY() - 18, 200, 16, juce::Justification::left);
    g.drawText ("BANK / CHARACTER / GENRE", lr.getX() + lr.getWidth() * 0.4f, lr.getY() - 18, 300, 16, juce::Justification::left);
    g.drawText ("RATING", lr.getRight() - 110, lr.getY() - 18, 100, 16, juce::Justification::left);
}

void PresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) shown.size())) return;
    const auto& p = theme::get();
    const int idx = shown[(size_t) row];
    const auto& e = proc.presets->all()[(size_t) idx];
    if (selected || idx == proc.presets->getCurrentIndex())
    {
        g.setColour ((idx == proc.presets->getCurrentIndex() ? p.cyan : p.magenta).withAlpha (selected ? 0.35f : 0.18f));
        g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
    }
    const bool fav = proc.presets->isFavourite (idx);
    g.setColour (fav ? p.yellow : p.textDim.withAlpha (0.5f));
    g.setFont (theme::font (15.0f));
    g.drawText (fav ? juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x85")) : juce::String (juce::CharPointer_UTF8 ("\xe2\x98\x86")), 6, 0, 20, h, juce::Justification::centred);
    g.setColour (p.text);
    g.setFont (theme::font (13.5f, true));
    g.drawText (e.data.name, 30, 0, (int) (w * 0.4f) - 34, h, juce::Justification::centredLeft);
    g.setColour (p.textDim);
    g.setFont (theme::font (12.0f));
    g.drawText (e.data.bank + "  /  " + e.data.character + "  /  " + e.data.genre + (e.source != "Factory" ? "  [" + e.source + "]" : juce::String()),
                (int) (w * 0.4f), 0, (int) (w * 0.6f) - 120, h, juce::Justification::centredLeft);
    const int rating = proc.presets->getRating (idx);
    for (int s = 0; s < 5; ++s)
    {
        g.setColour (s < rating ? p.pink : p.track);
        g.fillEllipse ((float) (w - 108 + s * 20), (float) h * 0.5f - 6.0f, 12.0f, 12.0f);
    }
}

void PresetBrowser::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if (! juce::isPositiveAndBelow (row, (int) shown.size())) return;
    const int idx = shown[(size_t) row];
    const int w = list.getWidth();
    if (e.x < 28) { proc.presets->toggleFavourite (idx); return; }
    if (e.x > w - 112)
    {
        const int star = juce::jlimit (1, 5, (e.x - (w - 112)) / 20 + 1);
        proc.presets->setRating (idx, proc.presets->getRating (idx) == star ? 0 : star);
    }
}

void PresetBrowser::listBoxItemDoubleClicked (int row, const juce::MouseEvent&) { returnKeyPressed (row); }

void PresetBrowser::returnKeyPressed (int row)
{
    if (juce::isPositiveAndBelow (row, (int) shown.size()))
        proc.presets->load (shown[(size_t) row]);
}
} // namespace bgdj
