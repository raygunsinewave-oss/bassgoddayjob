#pragma once

#include <juce_core/juce_core.h>
#include "../Common/Parameters.h"

namespace bgdj
{
/** In-memory form of a .bgdjpreset (JSON) file. */
struct PresetData
{
    juce::String name, bank, author { "Raygun" }, description;
    juce::String genre, character, key;
    int bpm = 0;
    juce::StringArray extraTags;
    juce::StringArray macroLabels { "Macro 1", "Macro 2", "Macro 3", "Macro 4" };
    juce::NamedValueSet params;            // paramID -> number, bool, or choice name
    juce::String tables[kNumWtOscs];       // factory table names ("" = keep parameter index)
    juce::String userTables[kNumWtOscs];   // base64 user wavetable data (optional)
    juce::var matrix;                      // array of slot objects
    juce::var lfoShapes;                   // object: "1".."8" -> points
    juce::var duckShape;                   // points
    juce::var fxOrder;                     // array of module ids
    juce::String tuningType, tuningText;
    bool isFactory = false;

    juce::StringArray allTags() const;     // genre, character, key, bpm and extras, for search
};

namespace presetformat
{
    constexpr const char* kExtension = ".bgdjpreset";

    bool parse (const juce::String& json, PresetData& out, juce::String& error);
    juce::String serialise (const PresetData& p);

    /** Checks every parameter id/choice against the layout. Returns problems (empty = valid). */
    juce::StringArray validate (const PresetData& p, const juce::AudioProcessorValueTreeState& state);

    /** Value conversion: a param var (number / bool / choice string) -> real parameter value. */
    bool resolveValue (const juce::RangedAudioParameter& param, const juce::var& v, float& realValue);

    /** Words that must never appear in factory preset names or tags (trademark/artist safety). */
    const juce::StringArray& bannedWords();
}
} // namespace bgdj
