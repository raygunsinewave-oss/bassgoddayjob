#pragma once

#include <juce_graphics/juce_graphics.h>

namespace bgdj
{
/** Minimal animated GIF89a writer (fixed 6x7x6 colour cube + ordered dithering, LZW).
    Used for the 10-second club-scene clip export. */
class GifEncoder
{
public:
    bool begin (const juce::File& file, int width, int height, int frameDelayCentiseconds);
    void addFrame (const juce::Image& frame);
    bool end();

    /** Convenience: encodes a list of frames in one go. */
    static bool write (const juce::File& file, const juce::Array<juce::Image>& frames, int delayCs);

private:
    void writeLzw (const std::vector<uint8_t>& indices);
    std::unique_ptr<juce::FileOutputStream> out;
    int w = 0, h = 0, delay = 8;
};
} // namespace bgdj
