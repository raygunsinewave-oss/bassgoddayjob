#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace bgdj
{
/** Band-limited, mip-mapped multi-frame wavetable.
    Level L keeps harmonics 1 .. (1024 >> L). Each stored cycle has one guard sample
    before and two after so linear and cubic interpolation can read without wrapping. */
class Wavetable
{
public:
    static constexpr int kSize = 2048;
    static constexpr int kLevels = 11;
    static constexpr int kStride = kSize + 3;
    static constexpr int kMaxFrames = 256;

    juce::String name;
    int numFrames = 0;

    /** Original (non band-limited) frames, kept for the editor and export. */
    std::vector<std::vector<float>> sourceFrames;

    const float* cycle (int frame, int level) const noexcept
    {
        return data.data() + ((size_t) frame * kLevels + (size_t) level) * kStride + 1;
    }

    static int maxHarmonicForLevel (int level) noexcept { return 1024 >> level; }

    /** Builds a table from time-domain frames (any count 1..kMaxFrames, each kSize samples).
        Removes DC and normalises the loudest frame to peak 1. */
    static std::shared_ptr<Wavetable> fromFrames (const juce::String& name, std::vector<std::vector<float>> frames,
                                                  bool normalise = true);

    /** Imports a WAV/AIFF file. Single-cycle files (<= 4096 samples, or frameSizeHint == 0 and
        no marker) are resampled to one frame; longer files are split into frames of
        frameSizeHint samples (2048 if 0, or the size stored in a Serum-style 'clm ' chunk). */
    static std::shared_ptr<Wavetable> fromAudioFile (const juce::File& file, juce::String& error, int frameSizeHint = 0);

    /** Writes the source frames as a 32-bit float WAV with 2048-sample frames. */
    bool exportWav (const juce::File& file) const;

    /** Serialisation for embedding user tables in plugin state. */
    juce::String toBase64() const;
    static std::shared_ptr<Wavetable> fromBase64 (const juce::String& name, const juce::String& b64);

private:
    std::vector<float> data;
};

// ============================================================================
/** Factory wavetable definitions (procedurally generated on demand). */
namespace factory
{
    int numTables();
    const juce::StringArray& tableNames();
    int indexOf (const juce::String& name);      // -1 if unknown
    std::shared_ptr<Wavetable> build (int index);
}

// ============================================================================
/** Cache of built factory tables. Thread-safe; build() may be slow and must not
    be called on the audio thread. */
class WavetableLibrary
{
public:
    static WavetableLibrary& getInstance();
    std::shared_ptr<const Wavetable> getFactory (int index);
    void clear();

private:
    std::mutex lock;
    std::map<int, std::shared_ptr<const Wavetable>> cache;
    std::vector<int> lru;
};
} // namespace bgdj
