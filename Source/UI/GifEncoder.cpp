#include "GifEncoder.h"

namespace bgdj
{
static void writeShort (juce::OutputStream& o, int v) { o.writeByte ((char) (v & 0xff)); o.writeByte ((char) ((v >> 8) & 0xff)); }

bool GifEncoder::begin (const juce::File& file, int width, int height, int delayCs)
{
    file.deleteFile();
    out = std::make_unique<juce::FileOutputStream> (file);
    if (! out->openedOk())
        return false;
    w = width; h = height; delay = delayCs;
    out->write ("GIF89a", 6);
    writeShort (*out, w);
    writeShort (*out, h);
    out->writeByte ((char) 0xF7);      // global colour table, 8 bits, 256 entries
    out->writeByte (0);
    out->writeByte (0);
    for (int i = 0; i < 256; ++i)
    {
        if (i < 252)
        {
            const int r = i / 42, g = (i / 6) % 7, b = i % 6;
            out->writeByte ((char) (r * 51));
            out->writeByte ((char) (g * 255 / 6));
            out->writeByte ((char) (b * 51));
        }
        else { out->writeByte (0); out->writeByte (0); out->writeByte (0); }
    }
    // loop forever
    const uint8_t netscape[] = { 0x21, 0xFF, 0x0B, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 0x03, 0x01, 0x00, 0x00, 0x00 };
    out->write (netscape, sizeof (netscape));
    return true;
}

void GifEncoder::addFrame (const juce::Image& frameIn)
{
    if (out == nullptr)
        return;
    auto frame = frameIn.rescaled (w, h, juce::Graphics::mediumResamplingQuality).convertedToFormat (juce::Image::RGB);
    static const float bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    std::vector<uint8_t> idx ((size_t) w * (size_t) h);
    const juce::Image::BitmapData bd (frame, juce::Image::BitmapData::readOnly);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const auto c = bd.getPixelColour (x, y);
            const float d = (bayer[y & 3][x & 3] / 16.0f - 0.5f);
            auto q = [d] (int v, int levels) { return juce::jlimit (0, levels - 1, (int) std::round (v / 255.0f * (float) (levels - 1) + d)); };
            const int r = q (c.getRed(), 6), g = q (c.getGreen(), 7), b = q (c.getBlue(), 6);
            idx[(size_t) (y * w + x)] = (uint8_t) (r * 42 + g * 6 + b);
        }

    const uint8_t gce[] = { 0x21, 0xF9, 0x04, 0x00, (uint8_t) (delay & 0xff), (uint8_t) (delay >> 8), 0x00, 0x00 };
    out->write (gce, sizeof (gce));
    out->writeByte (0x2C);
    writeShort (*out, 0); writeShort (*out, 0);
    writeShort (*out, w); writeShort (*out, h);
    out->writeByte (0);
    writeLzw (idx);
}

void GifEncoder::writeLzw (const std::vector<uint8_t>& indices)
{
    constexpr int minCode = 8, clearCode = 256, eoi = 257;
    out->writeByte ((char) minCode);

    std::vector<uint8_t> block;
    uint32_t bitBuf = 0;
    int bitCount = 0, codeSize = minCode + 1, nextCode = eoi + 1;
    auto flushBlock = [&]
    {
        if (block.empty()) return;
        out->writeByte ((char) block.size());
        out->write (block.data(), block.size());
        block.clear();
    };
    auto emit = [&] (int code)
    {
        bitBuf |= (uint32_t) code << bitCount;
        bitCount += codeSize;
        while (bitCount >= 8)
        {
            block.push_back ((uint8_t) (bitBuf & 0xff));
            bitBuf >>= 8;
            bitCount -= 8;
            if (block.size() == 255) flushBlock();
        }
    };

    std::vector<int16_t> dict ((size_t) 4096 * 256, -1);
    emit (clearCode);
    int prefix = indices.empty() ? 0 : indices[0];
    for (size_t i = 1; i < indices.size(); ++i)
    {
        const int k = indices[i];
        const int16_t existing = dict[(size_t) prefix * 256 + (size_t) k];
        if (existing >= 0) { prefix = existing; continue; }
        emit (prefix);
        if (nextCode < 4096)
        {
            dict[(size_t) prefix * 256 + (size_t) k] = (int16_t) nextCode;
            if (nextCode == (1 << codeSize) && codeSize < 12) ++codeSize;
            ++nextCode;
        }
        else
        {
            emit (clearCode);
            std::fill (dict.begin(), dict.end(), (int16_t) -1);
            codeSize = minCode + 1;
            nextCode = eoi + 1;
        }
        prefix = k;
    }
    emit (prefix);
    emit (eoi);
    if (bitCount > 0) block.push_back ((uint8_t) (bitBuf & 0xff));
    flushBlock();
    out->writeByte (0);
}

bool GifEncoder::end()
{
    if (out == nullptr) return false;
    out->writeByte (0x3B);
    out->flush();
    const bool ok = out->getStatus().wasOk();
    out.reset();
    return ok;
}

bool GifEncoder::write (const juce::File& file, const juce::Array<juce::Image>& frames, int delayCs)
{
    if (frames.isEmpty()) return false;
    GifEncoder e;
    if (! e.begin (file, frames[0].getWidth(), frames[0].getHeight(), delayCs)) return false;
    for (auto& f : frames) e.addFrame (f);
    return e.end();
}
} // namespace bgdj
