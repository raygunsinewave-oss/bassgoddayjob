#pragma once

#include "Widgets.h"
#include "../Presets/PresetManager.h"

namespace bgdj
{
/** Full-window preset browser: search, bank/character filters, favourites, ratings,
    "Similar To", randomize, import and rescan. */
class PresetBrowser : public juce::Component, private juce::ListBoxModel, private juce::ChangeListener
{
public:
    explicit PresetBrowser (BassGodsProcessor& p);
    ~PresetBrowser() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    std::function<void()> onClose;
    std::vector<int> currentList() const { return shown; }

private:
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
    void returnKeyPressed (int row) override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void refresh();

    BassGodsProcessor& proc;
    PresetManager::Filter filter;
    std::vector<int> shown;
    bool showingSimilar = false;

    juce::TextEditor search;
    juce::ListBox list { "presets", this };
    juce::ListBox bankList, charList;
    juce::ToggleButton favOnly { "Favourites only" };
    juce::ComboBox minRating;
    juce::TextButton loadBtn { "Load" }, similarBtn { "Similar To" }, randomBtn { "Randomize..." }, importBtn { "Import..." },
                     rescanBtn { "Rescan" }, closeBtn { "Close" }, clearBtn { "Clear filters" };
    juce::Label count;
    std::unique_ptr<juce::FileChooser> chooser;

    struct StringListModel : public juce::ListBoxModel
    {
        juce::StringArray items;
        int selected = 0;
        std::function<void (int)> onSelect;
        int getNumRows() override { return items.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override;
        void listBoxItemClicked (int row, const juce::MouseEvent&) override { selected = row; if (onSelect) onSelect (row); }
    };
    StringListModel bankModel, charModel;
};
} // namespace bgdj
