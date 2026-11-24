#include "Wavetable.h"

namespace bgdj
{
std::shared_ptr<Wavetable> Wavetable::fromFrames (const juce::String& name, std::vector<std::vector<float>> frames, bool normalise)
{
    if (frames.empty())
        frames.push_back (std::vector<float> ((size_t) kSize, 0.0f));
    if ((int) frames.size() > kMaxFrames)
        frames.resize ((size_t) kMaxFrames);

    for (auto& f : frames)
        f.resize ((size_t) kSize, 0.0f);

    auto wt = std::make_shared<Wavetable>();
    wt->name = name;
    wt->numFrames = (int) frames.size();
    wt->data.assign ((size_t) wt->numFrames * kLevels * kStride, 0.0f);

    juce::dsp::FFT fft (11);
    std::vector<juce::dsp::Complex<float>> spec ((size_t) kSize), work ((size_t) kSize), out ((size_t) kSize);

    float globalPeak = 0.0f;

    for (int fr = 0; fr < wt->numFrames; ++fr)
    {
        for (int i = 0; i < kSize; ++i)
            work[(size_t) i] = { frames[(size_t) fr][(size_t) i], 0.0f };
        fft.perform (work.data(), spec.data(), false);
        spec[0] = { 0.0f, 0.0f };   // remove DC

        for (int level = 0; level < kLevels; ++level)
        {
            const int maxH = maxHarmonicForLevel (level);
            for (int k = 0; k < kSize; ++k)
            {
                const int h = k <= kSize / 2 ? k : kSize - k;
                work[(size_t) k] = (h >= 1 && h < maxH) || (h == maxH && maxH < kSize / 2) ? spec[(size_t) k] : juce::dsp::Complex<float> { 0.0f, 0.0f };
            }
            fft.perform (work.data(), out.data(), true);

            float* dst = wt->data.data() + ((size_t) fr * kLevels + (size_t) level) * kStride + 1;
            for (int i = 0; i < kSize; ++i)
                dst[i] = out[(size_t) i].real();
            dst[-1] = dst[kSize - 1];
            dst[kSize] = dst[0];
            dst[kSize + 1] = dst[1];

            if (level == 0)
                for (int i = 0; i < kSize; ++i)
                    globalPeak = std::max (globalPeak, std::abs (dst[i]));
        }
    }

    if (normalise && globalPeak > 1.0e-6f)
    {
        const float g = 1.0f / globalPeak;
        for (auto& s : wt->data)
            s *= g;
        for (auto& f : frames)
            for (auto& s : f)
                s *= g;
    }

    wt->sourceFrames = std::move (frames);
    return wt;
}

// ----------------------------------------------------------------------------
static int findSerumFrameSize (const juce::File& file)
{
    juce::MemoryBlock mb;
    if (! file.loadFileAsData (mb) || mb.getSize() < 12)
        return 0;
    const auto* d = static_cast<const char*> (mb.getData());
    const size_t n = mb.getSize();
    for (size_t i = 12; i + 8 < n; ++i)
    {
        if (d[i] == 'c' && d[i + 1] == 'l' && d[i + 2] == 'm' && d[i + 3] == ' ')
        {
            const auto len = (size_t) juce::ByteOrder::littleEndianInt (d + i + 4);
            if (i + 8 + len > n || len > 256)
                return 0;
            juce::String s (d + i + 8, len);
            if (s.startsWith ("<!>"))
                return s.substring (3).getIntValue();
            return 0;
        }
    }
    return 0;
}

static std::vector<float> resampleCycle (const float* src, int len, int outLen)
{
    std::vector<float> out ((size_t) outLen);
    for (int i = 0; i < outLen; ++i)
    {
        const double pos = (double) i * len / outLen;
        const int i0 = (int) pos;
        const float frac = (float) (pos - i0);
        const float a = src[i0 % len], b = src[(i0 + 1) % len];
        out[(size_t) i] = a + (b - a) * frac;
    }
    return out;
}

std::shared_ptr<Wavetable> Wavetable::fromAudioFile (const juce::File& file, juce::String& error, int frameSizeHint)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "Could not read audio file: " + file.getFileName();
        return nullptr;
    }
    const auto total = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) kSize * kMaxFrames * 4);
    if (total < 8)
    {
        error = "Audio file is too short to be a wavetable.";
        return nullptr;
    }

    juce::AudioBuffer<float> buf (1, total);
    reader->read (&buf, 0, total, 0, true, false);
    const float* s = buf.getReadPointer (0);

    int frameSize = frameSizeHint > 0 ? frameSizeHint : findSerumFrameSize (file);
    std::vector<std::vector<float>> frames;

    if (frameSize <= 0)
    {
        if (total <= 4096)
            frames.push_back (resampleCycle (s, total, kSize));
        else
            frameSize = kSize;
    }

    if (frameSize > 0)
    {
        const int count = std::min (kMaxFrames, std::max (1, total / frameSize));
        for (int f = 0; f < count; ++f)
            frames.push_back (resampleCycle (s + (size_t) f * (size_t) frameSize, std::min (frameSize, total - f * frameSize), kSize));
    }

    return fromFrames (file.getFileNameWithoutExtension(), std::move (frames));
}

bool Wavetable::exportWav (const juce::File& file) const
{
    juce::AudioBuffer<float> buf (1, numFrames * kSize);
    for (int f = 0; f < numFrames; ++f)
        std::copy (sourceFrames[(size_t) f].begin(), sourceFrames[(size_t) f].end(), buf.getWritePointer (0) + f * kSize);

    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> os (file.createOutputStream());
    if (os == nullptr)
        return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), 48000.0, 1, 32, {}, 0));
    if (w == nullptr)
        return false;
    os.release();
    return w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
}

juce::String Wavetable::toBase64() const
{
    juce::MemoryOutputStream mo;
    mo.writeInt (numFrames);
    for (const auto& f : sourceFrames)
        for (float v : f)
            mo.writeShort ((short) juce::jlimit (-32767, 32767, juce::roundToInt (v * 32767.0f)));
    juce::MemoryOutputStream z;
    {
        juce::GZIPCompressorOutputStream gz (z, 6);
        gz.write (mo.getData(), mo.getDataSize());
    }
    return juce::Base64::toBase64 (z.getData(), z.getDataSize());
}

std::shared_ptr<Wavetable> Wavetable::fromBase64 (const juce::String& name, const juce::String& b64)
{
    juce::MemoryOutputStream raw;
    if (! juce::Base64::convertFromBase64 (raw, b64))
        return nullptr;
    juce::MemoryInputStream mis (raw.getData(), raw.getDataSize(), false);
    juce::GZIPDecompressorInputStream gz (mis);
    juce::MemoryBlock mb;
    gz.readIntoMemoryBlock (mb);
    juce::MemoryInputStream in (mb, false);
    const int nf = in.readInt();
    if (nf <= 0 || nf > kMaxFrames || (size_t) mb.getSize() < 4 + (size_t) nf * kSize * 2)
        return nullptr;
    std::vector<std::vector<float>> frames ((size_t) nf, std::vector<float> ((size_t) kSize));
    for (auto& f : frames)
        for (auto& v : f)
            v = (float) in.readShort() / 32767.0f;
    return fromFrames (name, std::move (frames), false);
}

// ============================================================================
WavetableLibrary& WavetableLibrary::getInstance()
{
    static WavetableLibrary lib;
    return lib;
}

std::shared_ptr<const Wavetable> WavetableLibrary::getFactory (int index)
{
    index = juce::jlimit (0, factory::numTables() - 1, index);
    {
        std::lock_guard<std::mutex> g (lock);
        auto it = cache.find (index);
        if (it != cache.end())
        {
            lru.erase (std::remove (lru.begin(), lru.end(), index), lru.end());
            lru.push_back (index);
            return it->second;
        }
    }

    std::shared_ptr<const Wavetable> built = factory::build (index);

    std::lock_guard<std::mutex> g (lock);
    cache[index] = built;
    lru.erase (std::remove (lru.begin(), lru.end(), index), lru.end());
    lru.push_back (index);
    while (lru.size() > 12)   // evicted tables stay alive while an oscillator still holds them
    {
        cache.erase (lru.front());
        lru.erase (lru.begin());
    }
    return built;
}

void WavetableLibrary::clear()
{
    std::lock_guard<std::mutex> g (lock);
    cache.clear();
    lru.clear();
}
} // namespace bgdj
