#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include "../Common/LockFree.h"

namespace bgdj
{
/** A looping mono sample used by OSC D in noise mode. */
struct NoiseSample
{
    juce::String name;
    std::vector<float> data;   // length is a power of two, with 2 guard samples
    int length = 0;            // loop length (power of two)
    double nativeRate = 48000.0;
};

struct NoiseSet
{
    std::vector<std::shared_ptr<const NoiseSample>> samples;
};

/** Factory noise textures are synthesised once; user samples come from the noise folder.
    The audio thread reads the current NoiseSet through an RtSharedObject. */
class NoiseBank
{
public:
    static NoiseBank& getInstance();

    static const juce::StringArray& factoryNames();

    /** Message thread: rebuilds the set from factory + every audio file in the folders. */
    void rescan (const juce::Array<juce::File>& userFolders);

    const NoiseSet* getForAudio() const noexcept { return current.get(); }
    void audioAcknowledge() noexcept             { current.audioThreadAcknowledge(); }
    void collectGarbage()                        { current.collectGarbage(); }
    juce::StringArray currentNames() const;

    static std::shared_ptr<NoiseSample> makeFactory (int index);

private:
    NoiseBank();
    std::vector<std::shared_ptr<const NoiseSample>> factory;
    RtSharedObject<NoiseSet> current;
};
} // namespace bgdj
