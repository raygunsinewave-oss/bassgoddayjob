#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace bgdj
{
/** 128-note tuning table. Equal temperament by default; can load Scala (.scl, optionally
    with a .kbm keyboard map) or AnaMark (.tun) files. Frequencies are stored relative to
    A4 = 440 Hz; the engine scales them by the global tuning parameter. */
struct TuningTable
{
    juce::String name { "12-TET" };
    std::array<double, 128> hz {};
    juce::String sourceText;      // original file text (stored with the plugin state)
    juce::String sourceType;      // "scl", "tun" or empty

    TuningTable() { setEqual(); }

    void setEqual()
    {
        name = "12-TET";
        sourceText = {};
        sourceType = {};
        for (int n = 0; n < 128; ++n)
            hz[(size_t) n] = 440.0 * std::pow (2.0, (n - 69) / 12.0);
    }

    /** Parses Scala .scl text. Degree 0 is mapped to MIDI note `rootNote`, whose
        frequency is kept at its 12-TET value. Returns false with an error message on failure. */
    bool loadScl (const juce::String& text, juce::String& error, int rootNote = 60);

    /** Parses AnaMark .tun text ([Exact Tuning] note N = Hz, or [Tuning] note N = cents). */
    bool loadTun (const juce::String& text, juce::String& error);

    bool loadFile (const juce::File& f, juce::String& error);

    /** Restores from stored source text (used by setStateInformation). */
    bool restore (const juce::String& type, const juce::String& text);
};
} // namespace bgdj
