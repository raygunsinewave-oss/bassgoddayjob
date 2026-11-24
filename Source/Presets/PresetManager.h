#pragma once

#include "PresetFormat.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <set>

namespace bgdj
{
class BassGodsProcessor;

struct PresetEntry
{
    PresetData data;
    juce::File file;            // empty for embedded factory presets
    juce::String source;        // "Factory", "User" or the expansion name
    juce::String uid;           // stable id for favourites / ratings
};

class PresetManager : public juce::ChangeBroadcaster
{
public:
    explicit PresetManager (BassGodsProcessor& p);

    void rescan();
    const std::vector<PresetEntry>& all() const noexcept { return entries; }
    juce::StringArray banks() const;
    juce::StringArray characters() const;

    struct Filter
    {
        juce::String search, bank, character, genre;
        bool favouritesOnly = false;
        int minRating = 0;
    };
    std::vector<int> filtered (const Filter& f) const;

    void load (int index);
    int getCurrentIndex() const noexcept { return current; }
    void step (int delta, const std::vector<int>& within);

    /** Saves the current sound as a user preset. Returns error text or empty. */
    juce::String saveUser (const juce::String& name, const juce::String& bank, const juce::String& character,
                           const juce::String& genre, const juce::StringArray& extraTags, const juce::String& key, int bpm);
    juce::String importFile (const juce::File& f);

    bool isFavourite (int index) const;
    void toggleFavourite (int index);
    int getRating (int index) const;
    void setRating (int index, int stars);

    std::vector<int> similarTo (int index, int maxResults = 12) const;

    // A/B compare
    void abStore (int slot);
    void abRecall (int slot);
    void abToggle();
    int abActive() const noexcept { return activeSlot; }
    void abCopyAtoB();

    /** Randomises a section ("all", "osc", "filter", "env", "lfo", "fx", "macro"). */
    void randomize (const juce::String& section, float amount = 1.0f);

    void clearCachesAndRebuild();

private:
    void loadDb();
    void saveDb() const;
    void addFromJson (const juce::String& json, const juce::File& file, const juce::String& source);

    BassGodsProcessor& proc;
    std::vector<PresetEntry> entries;
    int current = -1;
    std::set<juce::String> favourites;
    std::map<juce::String, int> ratings;
    std::array<std::unique_ptr<PresetData>, 2> abSlots;
    int activeSlot = 0;
    juce::Random random;
};
} // namespace bgdj
