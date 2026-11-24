#include "NoiseBank.h"

namespace bgdj
{
namespace
{
    constexpr int kLen = 1 << 16;
    constexpr double kRate = 48000.0;

    struct OnePole
    {
        float z = 0.0f, a = 0.0f;
        void setLP (float hz) { a = std::exp (-juce::MathConstants<float>::twoPi * hz / (float) kRate); }
        float lp (float x) { z = x + a * (z - x); return z; }
        float hp (float x) { return x - lp (x); }
    };

    struct BandPass   // RBJ biquad band-pass, constant 0 dB peak gain
    {
        float b0 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        void set (float hz, float q)
        {
            const float w = juce::MathConstants<float>::twoPi * hz / (float) kRate;
            const float alpha = std::sin (w) / (2.0f * q);
            const float a0 = 1.0f + alpha;
            b0 = alpha / a0; b2 = -alpha / a0; a1 = -2.0f * std::cos (w) / a0; a2 = (1.0f - alpha) / a0;
        }
        float process (float x)
        {
            const float y = b0 * x + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            return y;
        }
    };

    struct Pink   // Paul Kellet's economy pink filter
    {
        float b0 = 0, b1 = 0, b2 = 0;
        float process (float w)
        {
            b0 = 0.99765f * b0 + w * 0.0990460f;
            b1 = 0.96300f * b1 + w * 0.2965164f;
            b2 = 0.57000f * b2 + w * 1.0526913f;
            return (b0 + b1 + b2 + w * 0.1848f) * 0.25f;
        }
    };

    using Gen = std::function<void (std::vector<float>&, FastRandom&)>;
    struct Def { const char* name; Gen gen; };

    void crackle (std::vector<float>& d, FastRandom& r, float density, float size, float hissLevel)
    {
        OnePole hp; hp.setLP (300.0f);
        float env = 0.0f, sign = 1.0f;
        for (auto& s : d)
        {
            if (r.nextFloat() < density) { env = size * (0.3f + r.nextFloat()); sign = r.nextFloat() < 0.5f ? -1.0f : 1.0f; }
            env *= 0.82f;
            s = hp.hp (env * sign * (0.6f + 0.4f * r.nextBipolar())) + hissLevel * r.nextBipolar();
        }
    }

    void filteredNoise (std::vector<float>& d, FastRandom& r, float centre, float q, float lfoHz = 0.0f, float lfoDepth = 0.0f)
    {
        BandPass bp;
        for (size_t i = 0; i < d.size(); ++i)
        {
            if (i % 64 == 0)
            {
                const float m = lfoHz > 0.0f ? 1.0f + lfoDepth * std::sin (juce::MathConstants<float>::twoPi * lfoHz * (float) i / (float) kRate) : 1.0f;
                bp.set (juce::jlimit (20.0f, 20000.0f, centre * m), q);
            }
            d[i] = bp.process (r.nextBipolar());
        }
    }

    void crowd (std::vector<float>& d, FastRandom& r, int talkers, float cheer)
    {
        std::fill (d.begin(), d.end(), 0.0f);
        for (int t = 0; t < talkers; ++t)
        {
            BandPass f1, f2;
            const float base = 300.0f + r.nextFloat() * 500.0f;
            float vowelPhase = r.nextFloat() * 10.0f, syll = 0.0f;
            const float rate = 2.5f + r.nextFloat() * 3.0f;
            for (size_t i = 0; i < d.size(); ++i)
            {
                if (i % 128 == 0)
                {
                    vowelPhase += rate * 128.0f / (float) kRate;
                    f1.set (base * (1.0f + 0.4f * std::sin (vowelPhase * 2.1f)), 6.0f);
                    f2.set (base * 2.6f * (1.0f + 0.3f * std::sin (vowelPhase * 1.3f)), 8.0f);
                    syll = std::max (0.0f, std::sin (vowelPhase * juce::MathConstants<float>::pi));
                }
                const float n = r.nextBipolar();
                d[i] += (f1.process (n) + 0.6f * f2.process (n)) * syll * (1.0f + cheer * 2.0f);
            }
        }
        if (cheer > 0.0f)
        {
            Pink p;
            for (size_t i = 0; i < d.size(); ++i)
                d[i] += cheer * p.process (r.nextBipolar()) * 2.0f;
        }
    }

    void metallic (std::vector<float>& d, FastRandom& r, int partials, float decay, float hitsPerSec)
    {
        std::vector<float> freqs, phases;
        for (int p = 0; p < partials; ++p) { freqs.push_back (300.0f + r.nextFloat() * 6000.0f); phases.push_back (0.0f); }
        float env = 0.0f;
        const float hitProb = hitsPerSec / (float) kRate;
        for (auto& s : d)
        {
            if (r.nextFloat() < hitProb) env = 1.0f;
            env *= decay;
            float v = 0.0f;
            for (int p = 0; p < partials; ++p)
            {
                phases[(size_t) p] += freqs[(size_t) p] / (float) kRate;
                v += std::sin (juce::MathConstants<float>::twoPi * phases[(size_t) p]);
            }
            s = v * env / (float) partials + 0.02f * r.nextBipolar() * env;
        }
    }

    const std::vector<Def>& defs()
    {
        static const std::vector<Def> d = {
            { "White", [] (auto& v, auto& r) { for (auto& s : v) s = r.nextBipolar(); } },
            { "Pink", [] (auto& v, auto& r) { Pink p; for (auto& s : v) s = p.process (r.nextBipolar()); } },
            { "Brown", [] (auto& v, auto& r) { float z = 0; for (auto& s : v) { z = 0.996f * z + 0.04f * r.nextBipolar(); s = z; } } },
            { "Blue", [] (auto& v, auto& r) { float last = 0; for (auto& s : v) { const float w = r.nextBipolar(); s = w - 0.7f * last; last = w; } } },
            { "Violet", [] (auto& v, auto& r) { float last = 0; for (auto& s : v) { const float w = r.nextBipolar(); s = w - last; last = w; } } },
            { "Grey", [] (auto& v, auto& r) { Pink p; OnePole hp; hp.setLP (120.0f); for (auto& s : v) s = hp.hp (p.process (r.nextBipolar())) + 0.3f * r.nextBipolar(); } },
            { "Velvet", [] (auto& v, auto& r) { for (auto& s : v) { const float x = r.nextFloat(); s = x < 0.01f ? 1.0f : (x > 0.99f ? -1.0f : 0.0f); } } },
            { "Vinyl Crackle", [] (auto& v, auto& r) { crackle (v, r, 0.0015f, 1.0f, 0.01f); } },
            { "Vinyl Crackle Heavy", [] (auto& v, auto& r) { crackle (v, r, 0.006f, 1.0f, 0.03f); } },
            { "Vinyl Dust", [] (auto& v, auto& r) { crackle (v, r, 0.0004f, 0.6f, 0.0f); } },
            { "Vinyl Hiss", [] (auto& v, auto& r) { OnePole lp; lp.setLP (6000.0f); for (auto& s : v) s = lp.lp (r.nextBipolar()) * 0.5f; } },
            { "Tape Hiss", [] (auto& v, auto& r) { OnePole hp; hp.setLP (2500.0f); for (auto& s : v) s = hp.hp (r.nextBipolar()); } },
            { "Tape Hiss Warm", [] (auto& v, auto& r) { OnePole hp, lp; hp.setLP (800.0f); lp.setLP (7000.0f); for (auto& s : v) s = lp.lp (hp.hp (r.nextBipolar())); } },
            { "Tape Flutter Noise", [] (auto& v, auto& r) { filteredNoise (v, r, 3000.0f, 0.7f, 6.0f, 0.4f); } },
            { "Crowd Murmur", [] (auto& v, auto& r) { crowd (v, r, 10, 0.0f); } },
            { "Crowd Cheer", [] (auto& v, auto& r) { crowd (v, r, 14, 0.6f); } },
            { "Crowd Distant", [] (auto& v, auto& r) { crowd (v, r, 16, 0.1f); OnePole lp; lp.setLP (900.0f); for (auto& s : v) s = lp.lp (s); } },
            { "Club Room Tone", [] (auto& v, auto& r) { Pink p; OnePole lp; lp.setLP (400.0f); float ph = 0; for (auto& s : v) { ph += 50.0f / (float) kRate; s = lp.lp (p.process (r.nextBipolar())) * 2.0f + 0.1f * std::sin (juce::MathConstants<float>::twoPi * ph); } } },
            { "Breath", [] (auto& v, auto& r) { filteredNoise (v, r, 1500.0f, 0.8f, 0.35f, 0.5f); for (size_t i = 0; i < v.size(); ++i) v[i] *= 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * 0.7f * (float) i / (float) kRate); } },
            { "Breath Inhale", [] (auto& v, auto& r) { filteredNoise (v, r, 2200.0f, 1.2f, 0.7f, 0.6f); } },
            { "Whisper Air", [] (auto& v, auto& r) { filteredNoise (v, r, 5000.0f, 0.6f, 1.3f, 0.3f); } },
            { "Metallic Hit", [] (auto& v, auto& r) { metallic (v, r, 7, 0.9995f, 2.0f); } },
            { "Metallic Ring", [] (auto& v, auto& r) { metallic (v, r, 5, 0.99995f, 0.7f); } },
            { "Metal Scrape", [] (auto& v, auto& r) { filteredNoise (v, r, 3500.0f, 12.0f, 2.0f, 0.5f); } },
            { "Chain Rattle", [] (auto& v, auto& r) { metallic (v, r, 9, 0.998f, 25.0f); } },
            { "Industrial Hum", [] (auto& v, auto& r) { float ph = 0; for (auto& s : v) { ph += 55.0f / (float) kRate; s = 0.5f * std::tanh (3.0f * std::sin (juce::MathConstants<float>::twoPi * ph)) + 0.15f * r.nextBipolar(); } } },
            { "Mains Hum 50", [] (auto& v, auto&) { float ph = 0; for (auto& s : v) { ph += 50.0f / (float) kRate; s = std::sin (juce::MathConstants<float>::twoPi * ph) + 0.3f * std::sin (juce::MathConstants<float>::twoPi * ph * 3.0f); } } },
            { "Mains Hum 60", [] (auto& v, auto&) { float ph = 0; for (auto& s : v) { ph += 60.0f / (float) kRate; s = std::sin (juce::MathConstants<float>::twoPi * ph) + 0.3f * std::sin (juce::MathConstants<float>::twoPi * ph * 3.0f); } } },
            { "Rain", [] (auto& v, auto& r) { crackle (v, r, 0.02f, 0.3f, 0.05f); } },
            { "Wind", [] (auto& v, auto& r) { filteredNoise (v, r, 700.0f, 2.0f, 0.2f, 0.7f); } },
            { "Ocean Wash", [] (auto& v, auto& r) { Pink p; for (size_t i = 0; i < v.size(); ++i) v[i] = p.process (r.nextBipolar()) * (0.3f + 0.7f * std::pow (std::sin (juce::MathConstants<float>::pi * 0.73f * (float) i / (float) kRate), 2.0f)); } },
            { "Fire Crackle", [] (auto& v, auto& r) { crackle (v, r, 0.003f, 0.8f, 0.0f); Pink p; OnePole lp; lp.setLP (300.0f); for (auto& s : v) s += 1.5f * lp.lp (p.process (r.nextBipolar())); } },
            { "Geiger", [] (auto& v, auto& r) { for (auto& s : v) s = r.nextFloat() < 0.0008f ? 1.0f : 0.0f; } },
            { "Bitcrush Noise", [] (auto& v, auto& r) { float held = 0; for (size_t i = 0; i < v.size(); ++i) { if (i % 12 == 0) held = std::round (r.nextBipolar() * 4.0f) / 4.0f; v[i] = held; } } },
            { "Digital Glitch", [] (auto& v, auto& r) { float held = 0; int len = 1; for (size_t i = 0; i < v.size(); ++i) { if (--len <= 0) { len = 1 + (int) (r.nextFloat() * 400.0f); held = r.nextBipolar(); } v[i] = (i % 7 < 3 ? held : -held) * 0.7f; } } },
            { "Radio Static", [] (auto& v, auto& r) { filteredNoise (v, r, 2000.0f, 0.5f, 9.0f, 0.8f); for (auto& s : v) s *= r.nextFloat() < 0.98f ? 1.0f : 3.0f; } },
            { "Modem", [] (auto& v, auto& r) { float ph = 0, f = 1200; for (size_t i = 0; i < v.size(); ++i) { if (i % 480 == 0) f = r.nextFloat() < 0.5f ? 1200.0f : 2200.0f; ph += f / (float) kRate; v[i] = std::sin (juce::MathConstants<float>::twoPi * ph) * 0.6f + 0.1f * r.nextBipolar(); } } },
            { "Shaker Texture", [] (auto& v, auto& r) { OnePole hp; hp.setLP (5000.0f); for (size_t i = 0; i < v.size(); ++i) { const float e = std::exp (-(float) (i % 6000) / 900.0f); v[i] = hp.hp (r.nextBipolar()) * e; } } },
            { "Snare Rattle", [] (auto& v, auto& r) { filteredNoise (v, r, 4500.0f, 1.5f); for (size_t i = 0; i < v.size(); ++i) v[i] *= std::exp (-(float) (i % 12000) / 2500.0f); } },
            { "Hat Sizzle", [] (auto& v, auto& r) { OnePole hp; hp.setLP (8000.0f); for (size_t i = 0; i < v.size(); ++i) v[i] = hp.hp (r.nextBipolar()) * (0.3f + 0.7f * std::exp (-(float) (i % 3000) / 600.0f)); } },
            { "Rumble", [] (auto& v, auto& r) { OnePole lp, lp2; lp.setLP (90.0f); lp2.setLP (90.0f); for (auto& s : v) s = 6.0f * lp2.lp (lp.lp (r.nextBipolar())); } },
            { "Sub Rumble Noise", [] (auto& v, auto& r) { filteredNoise (v, r, 45.0f, 3.0f, 0.3f, 0.3f); for (auto& s : v) s *= 3.0f; } },
            { "Granular Dust", [] (auto& v, auto& r) { std::fill (v.begin(), v.end(), 0.0f); for (int g = 0; g < 300; ++g) { const int st = (int) (r.nextFloat() * (float) (v.size() - 2000)); const float f = 200.0f + r.nextFloat() * 4000.0f; for (int i = 0; i < 1500; ++i) v[(size_t) (st + i)] += std::sin (juce::MathConstants<float>::twoPi * f * (float) i / (float) kRate) * std::sin (juce::MathConstants<float>::pi * (float) i / 1500.0f) * 0.3f; } } },
            { "Bubbles", [] (auto& v, auto& r) { std::fill (v.begin(), v.end(), 0.0f); for (int g = 0; g < 120; ++g) { const int st = (int) (r.nextFloat() * (float) (v.size() - 4000)); float f = 400.0f + r.nextFloat() * 900.0f, ph = 0.0f; for (int i = 0; i < 3500; ++i) { f *= 1.0002f; ph += f / (float) kRate; v[(size_t) (st + i)] += std::sin (juce::MathConstants<float>::twoPi * ph) * std::exp (-(float) i / 700.0f) * 0.5f; } } } },
            { "Laser Noise", [] (auto& v, auto& r) { float ph = 0; for (size_t i = 0; i < v.size(); ++i) { const float t = (float) (i % 8000) / 8000.0f; ph += (3000.0f * (1.0f - t) + 100.0f) / (float) kRate; v[i] = std::sin (juce::MathConstants<float>::twoPi * ph) * (1.0f - t) + 0.05f * r.nextBipolar(); } } },
            { "Spray", [] (auto& v, auto& r) { OnePole hp; hp.setLP (3000.0f); for (size_t i = 0; i < v.size(); ++i) v[i] = hp.hp (r.nextBipolar()) * (0.6f + 0.4f * std::sin (juce::MathConstants<float>::twoPi * 3.0f * (float) i / (float) kRate)); } },
        };
        return d;
    }

    void normaliseAndLoop (std::vector<float>& d)
    {
        // crossfade the last 2048 samples into the start so the loop is seamless
        const int xf = 2048;
        const int n = (int) d.size();
        for (int i = 0; i < xf; ++i)
        {
            const float a = (float) i / (float) xf;
            d[(size_t) i] = d[(size_t) i] * a + d[(size_t) (n - xf + i)] * (1.0f - a);
        }
        float peak = 0.0f, mean = 0.0f;
        for (float s : d) mean += s;
        mean /= (float) n;
        for (auto& s : d) { s -= mean; peak = std::max (peak, std::abs (s)); }
        if (peak > 1.0e-6f)
            for (auto& s : d) s *= 0.9f / peak;
    }
} // namespace

const juce::StringArray& NoiseBank::factoryNames()
{
    static const juce::StringArray names = []
    {
        juce::StringArray s;
        for (auto& d : defs()) s.add (d.name);
        return s;
    }();
    return names;
}

std::shared_ptr<NoiseSample> NoiseBank::makeFactory (int index)
{
    const auto& def = defs()[(size_t) juce::jlimit (0, (int) defs().size() - 1, index)];
    auto ns = std::make_shared<NoiseSample>();
    ns->name = def.name;
    std::vector<float> d ((size_t) kLen + 2048, 0.0f);
    FastRandom r;
    r.seed ((uint32_t) (index * 7919 + 17));
    def.gen (d, r);
    normaliseAndLoop (d);
    d.resize ((size_t) kLen);
    d.push_back (d[0]);
    d.push_back (d[1]);
    ns->data = std::move (d);
    ns->length = kLen;
    ns->nativeRate = kRate;
    return ns;
}

NoiseBank::NoiseBank()
{
    for (int i = 0; i < (int) defs().size(); ++i)
        factory.push_back (makeFactory (i));
    auto set = std::make_shared<NoiseSet>();
    set->samples = factory;
    current.publish (set);
}

NoiseBank& NoiseBank::getInstance()
{
    static NoiseBank bank;
    return bank;
}

void NoiseBank::rescan (const juce::Array<juce::File>& userFolders)
{
    auto set = std::make_shared<NoiseSet>();
    set->samples = factory;

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    for (auto& folder : userFolders)
    {
        if (! folder.isDirectory())
            continue;
        for (const auto& entry : juce::RangedDirectoryIterator (folder, true, "*.wav;*.aif;*.aiff;*.flac"))
        {
            if (set->samples.size() >= 64)
                break;
            std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (entry.getFile()));
            if (reader == nullptr || reader->lengthInSamples < 64)
                continue;
            int len = 1;
            while (len * 2 <= (int) std::min<juce::int64> (reader->lengthInSamples, 1 << 20))
                len *= 2;
            juce::AudioBuffer<float> buf (1, len);
            reader->read (&buf, 0, len, 0, true, false);
            auto ns = std::make_shared<NoiseSample>();
            ns->name = entry.getFile().getFileNameWithoutExtension();
            ns->data.assign (buf.getReadPointer (0), buf.getReadPointer (0) + len);
            ns->data.push_back (ns->data[0]);
            ns->data.push_back (ns->data[1]);
            ns->length = len;
            ns->nativeRate = reader->sampleRate;
            set->samples.push_back (ns);
        }
    }
    current.publish (set);
}

juce::StringArray NoiseBank::currentNames() const
{
    juce::StringArray s;
    if (auto set = current.getShared())
        for (auto& n : set->samples)
            s.add (n->name);
    return s;
}
} // namespace bgdj
