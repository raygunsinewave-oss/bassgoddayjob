#pragma once

#include <juce_core/juce_core.h>

namespace bgdj
{
/** Installs and manages .bgdjpack expansion packs: zip files containing
    metadata.json plus presets/, wavetables/ and noise/ folders. */
class ExpansionManager
{
public:
    struct Expansion
    {
        juce::String id, name, author, version, description;
        juce::File folder;
        bool enabled = true;
        int numPresets = 0, numWavetables = 0, numNoise = 0;
    };

    static juce::Array<Expansion> list();
    /** Returns an empty string on success, otherwise an error message. */
    static juce::String install (const juce::File& packFile);
    static bool remove (const juce::String& id);
    static void setEnabled (const juce::String& id, bool enabled);

    /** Folders of enabled expansions for a content type: "presets", "wavetables" or "noise". */
    static juce::Array<juce::File> contentFolders (const juce::String& type);
};
} // namespace bgdj
