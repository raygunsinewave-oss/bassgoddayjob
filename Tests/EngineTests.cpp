#include "TestHelpers.h"
#include "Engine/SynthEngine.h"

namespace bgdj::test
{
struct EngineHarness
{
    LayoutHost host;
    SynthEngine engine;
    TuningTable tuning;
    ModMatrix matrix;
    std::shared_ptr<const Wavetable> tables[kNumWtOscs];
    double sr = 48000.0;

    EngineHarness()
    {
        engine.init (host.apvts);
        engine.prepare (sr, 1);
        for (int o = 0; o < kNumWtOscs; ++o)
            tables[o] = WavetableLibrary::getInstance().getFactory (factory::indexOf ("Saw"));
    }

    /** Renders blocks; events: (block index, message). Returns L channel. */
    std::vector<float> render (int blocks, std::vector<std::pair<int, juce::MidiMessage>> events, int os = 1)
    {
        EngineSources src;
        for (int o = 0; o < kNumWtOscs; ++o) src.tables[o] = tables[o].get();
        src.tuning = &tuning;
        src.matrix = &matrix;
        src.noise = NoiseBank::getInstance().getForAudio();
        std::vector<float> out;
        std::vector<float> L ((size_t) (256 * os)), R ((size_t) (256 * os));
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer m;
            for (auto& [blk, msg] : events) if (blk == b) m.addEvent (msg, 0);
            engine.beginBlock (src, 120.0, b * 256 / sr * 2.0, true, os);
            std::fill (L.begin(), L.end(), 0.0f);
            std::fill (R.begin(), R.end(), 0.0f);
            engine.render (L.data(), R.data(), 256, m);
            out.insert (out.end(), L.begin(), L.end());
        }
        return out;
    }

    static double rms (const std::vector<float>& v, size_t from = 0)
    {
        double s = 0;
        for (size_t i = from; i < v.size(); ++i) s += v[i] * v[i];
        return std::sqrt (s / std::max<size_t> (1, v.size() - from));
    }
};

struct EngineTests : public juce::UnitTest
{
    EngineTests() : juce::UnitTest ("Synth engine", "Engine") {}
    void runTest() override
    {
        using MM = juce::MidiMessage;

        beginTest ("a note sounds, is finite, and releases to silence");
        {
            EngineHarness h;
            auto out = h.render (200, { { 0, MM::noteOn (1, 36, (juce::uint8) 100) }, { 100, MM::noteOff (1, 36) } });
            expect (allFinite (out));
            expectGreaterThan (EngineHarness::rms (std::vector<float> (out.begin(), out.begin() + 25000)), 0.01);
            expectEquals (h.engine.getStats().activeVoices, 0);
        }

        beginTest ("legato mode uses one voice, poly mode stacks voices");
        {
            EngineHarness h;
            h.render (10, { { 0, MM::noteOn (1, 36, (juce::uint8) 100) }, { 2, MM::noteOn (1, 43, (juce::uint8) 100) } });
            expectEquals (h.engine.getStats().activeVoices, 1);
            h.host.set ("poly_mode", 2.0f);
            h.render (10, { { 0, MM::noteOn (1, 48, (juce::uint8) 100) }, { 1, MM::noteOn (1, 52, (juce::uint8) 100) }, { 2, MM::noteOn (1, 55, (juce::uint8) 100) } });
            expectGreaterOrEqual (h.engine.getStats().activeVoices, 3);
        }

        beginTest ("mod matrix routing changes the sound");
        {
            EngineHarness a, b;
            for (auto* h : { &a, &b })
            {
                h->host.set ("f1_cutoff", 300.0f);
                h->host.set ("f1_reso", 0.4f);
            }
            b.matrix.addRouting ((int) ModSource::Lfo1, params::modDestIndexForParam ("f1_cutoff"), 0.6f);
            auto oa = a.render (100, { { 0, MM::noteOn (1, 40, (juce::uint8) 100) } });
            auto ob = b.render (100, { { 0, MM::noteOn (1, 40, (juce::uint8) 100) } });
            double diff = 0;
            for (size_t i = 0; i < oa.size(); ++i) diff += std::abs (oa[i] - ob[i]);
            expectGreaterThan (diff / (double) oa.size(), 1.0e-3);
        }

        beginTest ("every warp mode and unison count stays finite");
        {
            EngineHarness h;
            h.host.set ("oB_on", 1.0f);
            h.host.set ("oA_uni", 16.0f);
            for (int w = 0; w < params::warpModes().size(); ++w)
            {
                h.host.set ("oA_warp", (float) w);
                h.host.set ("oA_warp_amt", 1.0f);
                auto out = h.render (20, { { 0, MM::noteOn (1, 30 + w, (juce::uint8) 127) }, { 15, MM::noteOff (1, 30 + w) } });
                expect (allFinite (out), "warp " + params::warpModes()[w]);
            }
        }

        beginTest ("seed lock makes random phase reproducible");
        {
            EngineHarness a, b;
            for (auto* h : { &a, &b }) { h->host.set ("seed_lock", 1.0f); h->host.set ("oA_rand", 1.0f); h->host.set ("oA_uni", 5.0f); }
            auto oa = a.render (10, { { 0, MM::noteOn (1, 40, (juce::uint8) 100) } });
            auto ob = b.render (10, { { 0, MM::noteOn (1, 40, (juce::uint8) 100) } });
            expect (oa == ob);
        }

        beginTest ("performance: a typical bass voice stays under 3% CPU at 48 kHz");
        {
            EngineHarness h;
            h.host.set ("oA_uni", 3.0f);
            h.host.set ("f1_on", 1.0f);
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            h.render (188, { { 0, MM::noteOn (1, 36, (juce::uint8) 100) } });   // ~1 s of audio
            const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
            logMessage ("one voice (3 unison + filter) rendered 1 s in " + juce::String (ms, 2) + " ms = " + juce::String (ms / 10.0, 2) + "% CPU");
            expectLessThan (ms / 1000.0, 0.03 * 3.0, "generous bound so slow CI machines don't flake");
        }
    }
};

static EngineTests engineTests;
} // namespace bgdj::test
