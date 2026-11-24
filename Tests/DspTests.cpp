#include "TestHelpers.h"
#include "DSP/Wavetable.h"
#include "DSP/Filter.h"
#include "DSP/NoiseBank.h"
#include "DSP/Tuning.h"
#include "DSP/FX/FxRack.h"
#include "Analysis/Loudness.h"
#include "Engine/Voice.h"
#include "Common/LockFree.h"

namespace bgdj::test
{
struct WavetableTests : public juce::UnitTest
{
    WavetableTests() : juce::UnitTest ("Wavetables", "DSP") {}
    void runTest() override
    {
        beginTest ("factory library size and sanity");
        expectGreaterOrEqual (factory::numTables(), 72, "spec asks for 12 named + 60 more bass tables");
        for (auto n : { "Sine", "Saw", "Square", "Reese Stack 1", "Growl 1", "Vowel AEIOU", "Garage Organ 1", "Acid Saw", "FM Metal 1", "Pluck Bright", "Rubber 1", "Hoover 1" })
            expect (factory::indexOf (n) >= 0, juce::String ("missing table ") + n);

        for (int i = 0; i < factory::numTables(); ++i)
        {
            auto wt = factory::build (i);
            bool finite = true;
            float peak = 0.0f;
            for (int f = 0; f < wt->numFrames; ++f)
                for (int s = 0; s < Wavetable::kSize; ++s)
                {
                    const float v = wt->cycle (f, 0)[s];
                    finite = finite && std::isfinite (v);
                    peak = std::max (peak, std::abs (v));
                }
            expect (finite, "non-finite sample in " + wt->name);
            expectLessOrEqual (peak, 1.001f, "table not normalised: " + wt->name);
        }

        beginTest ("mip levels are band-limited");
        auto saw = factory::build (factory::indexOf ("Saw"));
        juce::dsp::FFT fft (11);
        for (int level : { 3, 6, 9 })
        {
            std::vector<float> buf (Wavetable::kSize * 2, 0.0f);
            std::copy (saw->cycle (0, level), saw->cycle (0, level) + Wavetable::kSize, buf.begin());
            fft.performFrequencyOnlyForwardTransform (buf.data());
            const int maxH = Wavetable::maxHarmonicForLevel (level);
            double above = 0.0, total = 0.0;
            for (int b = 1; b < Wavetable::kSize / 2; ++b)
            {
                total += buf[(size_t) b] * buf[(size_t) b];
                if (b > maxH + 1) above += buf[(size_t) b] * buf[(size_t) b];
            }
            expectLessThan (above / total, 1.0e-6, "energy above harmonic limit at level " + juce::String (level));
        }

        beginTest ("base64 round trip");
        auto restored = Wavetable::fromBase64 ("rt", saw->toBase64());
        expect (restored != nullptr);
        expectWithinAbsoluteError (restored->sourceFrames[0][100], saw->sourceFrames[0][100], 1.0e-3f);
    }
};

struct FilterTests : public juce::UnitTest
{
    FilterTests() : juce::UnitTest ("Filters", "DSP") {}
    void runTest() override
    {
        beginTest ("all 32 models stay finite and bounded at maximum resonance and drive");
        expectEquals ((int) FilterType::Count, 32);
        expectEquals (params::filterTypes().size(), 32);
        juce::Random r (1);
        for (double fs : { 48000.0, 192000.0 })
            for (int t = 0; t < (int) FilterType::Count; ++t)
                for (float cutoff : { 20.0f, 200.0f, 2000.0f, 18000.0f })
                    for (SatMode sat : { SatMode::Tanh, SatMode::Tube })
                    {
                        VoiceFilter f;
                        f.prepare (fs, (int) (fs / 20.0) + 4);
                        f.setParams ((FilterType) t, cutoff, 1.0f, 1.0f, 1.0f, sat);
                        bool ok = true;
                        float peak = 0.0f;
                        for (int i = 0; i < 20000; ++i)
                        {
                            float l = (i == 0 ? 1.0f : 0.0f) + 0.5f * (r.nextFloat() * 2.0f - 1.0f), rr = l;
                            if (i % 32 == 0) f.setParams ((FilterType) t, cutoff * (1.0f + 0.5f * std::sin ((float) i * 0.001f)), 1.0f, 1.0f, 1.0f, sat);
                            f.process (l, rr);
                            ok = ok && std::isfinite (l) && std::isfinite (rr);
                            peak = std::max (peak, std::abs (l));
                        }
                        expect (ok, "NaN in " + params::filterTypes()[t]);
                        expectLessOrEqual (peak, 8.0f, "unbounded " + params::filterTypes()[t]);
                    }

        beginTest ("LP24 attenuates far above cutoff");
        VoiceFilter f;
        f.prepare (48000.0, 4800);
        f.setParams (FilterType::LP24, 200.0f, 0.0f, 0.0f, 0.0f, SatMode::Tanh);
        double eIn = 0, eOut = 0;
        for (int i = 0; i < 48000; ++i)
        {
            const float x = std::sin (juce::MathConstants<float>::twoPi * 5000.0f * (float) i / 48000.0f);
            float l = x, rr = x;
            f.process (l, rr);
            if (i > 4800) { eIn += x * x; eOut += l * l; }
        }
        expectLessThan (10.0 * std::log10 (eOut / eIn), -60.0, "LP24 at 200 Hz should cut 5 kHz by > 60 dB");
    }
};

struct ModulationTests : public juce::UnitTest
{
    ModulationTests() : juce::UnitTest ("Envelopes and LFOs", "DSP") {}
    void runTest() override
    {
        beginTest ("envelope attack time");
        Envelope e;
        e.noteOn (false);
        const float dt = 0.001f;
        int steps = 0;
        while (e.getStage() == Envelope::Stage::Attack && steps < 10000)
        {
            e.advance (dt, 100.0f, 200.0f, 0.5f, 100.0f, 0.0f, 0.0f, 0.0f);
            ++steps;
        }
        expectWithinAbsoluteError (steps, 100, 2);

        beginTest ("decay settles at sustain and release reaches zero");
        for (int i = 0; i < 400; ++i) e.advance (dt, 100.0f, 200.0f, 0.5f, 100.0f, 0.0f, 0.5f, 0.5f);
        expectWithinAbsoluteError (e.getLevel(), 0.5f, 1.0e-4f);
        e.noteOff();
        for (int i = 0; i < 120; ++i) e.advance (dt, 100.0f, 200.0f, 0.5f, 100.0f, 0.0f, 0.5f, 0.5f);
        expect (e.isIdle());

        beginTest ("tempo-synced LFO rate (1/4 at 120 BPM = 2 Hz)");
        expectWithinAbsoluteError (params::syncRateBeats (params::syncRates().indexOf ("1/4")), 1.0, 1e-9);
        expectWithinAbsoluteError (params::syncRateBeats (params::syncRates().indexOf ("1/8 T")), 1.0 / 3.0, 1e-9);
        Lfo lfo;
        LfoStatic ls;
        auto table = renderCurve (shapes::lfoPreset (2));   // saw up
        ls.table = &table;
        FastRandom rng;
        lfo.noteOn (ls, rng, true);
        int wraps = 0;
        float last = 0.0f;
        for (int i = 0; i < 1000; ++i)   // 1 second at 1 ms steps
        {
            lfo.advance (0.001f, 2.0f, 0.0f, ls, 0.0, false, rng);
            if (lfo.getPhase() < last) ++wraps;
            last = lfo.getPhase();
        }
        expectEquals (wraps, 2);

        beginTest ("drawn curve rendering hits its breakpoints");
        auto t = renderCurve ({ { 0, 0, 0 }, { 0.5f, 1, 0 }, { 1, 0, 0 } });
        expectWithinAbsoluteError (t.lookup (0.5f), 1.0f, 1.0e-3f);
        expectWithinAbsoluteError (t.lookup (0.25f), 0.5f, 1.0e-2f);
    }
};

struct TuningTests : public juce::UnitTest
{
    TuningTests() : juce::UnitTest ("Microtuning", "DSP") {}
    void runTest() override
    {
        beginTest ("12-TET via Scala equals equal temperament");
        juce::String scl = "! test\n12 tone equal\n12\n";
        for (int i = 1; i <= 12; ++i) scl << juce::String (i * 100.0, 1) << "\n";
        TuningTable t;
        juce::String err;
        expect (t.loadScl (scl, err), err);
        TuningTable eq;
        for (int n : { 0, 33, 60, 69, 127 })
            expectWithinAbsoluteError (t.hz[(size_t) n], eq.hz[(size_t) n], 1.0e-6 * eq.hz[(size_t) n]);

        beginTest ("ratio degrees and AnaMark .tun");
        TuningTable j;
        expect (j.loadScl ("just fifths\n2\n3/2\n2/1\n", err), err);
        expectWithinAbsoluteError (j.hz[61] / j.hz[60], 1.5, 1.0e-9);
        TuningTable tun;
        expect (tun.loadTun ("[Exact Tuning]\nnote 69 = 432.0\n", err), err);
        expectWithinAbsoluteError (tun.hz[69], 432.0, 1.0e-9);
        expect (! tun.loadTun ("nothing here", err));
    }
};

struct FifoTests : public juce::UnitTest
{
    FifoTests() : juce::UnitTest ("Lock-free FIFOs", "DSP") {}
    void runTest() override
    {
        beginTest ("SPSC order and capacity");
        SpscFifo<int, 8> f;
        for (int i = 0; i < 7; ++i) expect (f.push (i));
        int v = -1;
        for (int i = 0; i < 7; ++i) { expect (f.pop (v)); expectEquals (v, i); }
        expect (! f.pop (v));

        beginTest ("audio sample FIFO across threads");
        AudioSampleFifo af (4096);
        std::atomic<bool> done { false };
        std::thread producer ([&]
        {
            float l[64], r[64];
            for (int b = 0; b < 2000; ++b)
            {
                for (int i = 0; i < 64; ++i) l[i] = r[i] = (float) (b * 64 + i);
                while (af.getNumReady() > 4096 - 128) std::this_thread::yield();
                af.push (l, r, 64);
            }
            done = true;
        });
        float l[256], r[256];
        float expected = 0.0f;
        bool inOrder = true;
        while (! done || af.getNumReady() > 0)
        {
            const int n = af.pop (l, r, 256);
            for (int i = 0; i < n; ++i) { inOrder = inOrder && l[i] == expected; expected += 1.0f; }
        }
        producer.join();
        expect (inOrder);
        expectEquals ((int) expected, 2000 * 64);
    }
};

struct LoudnessTests : public juce::UnitTest
{
    LoudnessTests() : juce::UnitTest ("Loudness meter", "DSP") {}
    void runTest() override
    {
        beginTest ("997 Hz stereo sine at -20 dBFS reads -20 LUFS (BS.1770 calibration)");
        LoudnessMeter m;
        m.prepare (48000.0);
        std::vector<float> buf (480);
        for (int b = 0; b < 500; ++b)
        {
            for (int i = 0; i < 480; ++i)
                buf[(size_t) i] = 0.1f * std::sin (juce::MathConstants<float>::twoPi * 997.0f * (float) (b * 480 + i) / 48000.0f);
            m.process (buf.data(), buf.data(), 480);
        }
        expectWithinAbsoluteError (m.momentary.load(), -20.0f, 0.3f);
        expectWithinAbsoluteError (m.integrated.load(), -20.0f, 0.3f);
        expectWithinAbsoluteError (m.truePeakMaxDb.load(), -20.0f, 0.3f);
    }
};

struct FxTests : public juce::UnitTest
{
    FxTests() : juce::UnitTest ("FX rack", "DSP") {}
    void runTest() override
    {
        LayoutHost host;
        FxRack rack;
        rack.init (host.apvts);
        rack.prepare (48000.0, 512);
        for (int t = 0; t < kNumFxModules; ++t)
            host.set (params::fxPrefix ((FxType) t) + "on", 1.0f);
        host.set ("fx_lim_ceiling", -1.0f);
        host.set ("fx_dist_drive", 1.0f);

        for (int osIdx : { 0, 2 })
        {
            beginTest ("all 13 modules on, FX oversampling index " + juce::String (osIdx));
            juce::AudioBuffer<float> buf (2, 512);
            juce::Random r (3);
            bool finite = true;
            float peak = 0.0f;
            for (int b = 0; b < 300; ++b)
            {
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < 512; ++i)
                        buf.setSample (c, i, 1.5f * (r.nextFloat() * 2.0f - 1.0f));
                FxContext ctx;
                ctx.bpm = 128.0; ctx.ppq = b * 512 / 48000.0 * 128.0 / 60.0; ctx.playing = true;
                rack.process (buf, 512, ctx, osIdx);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < 512; ++i)
                    {
                        finite = finite && std::isfinite (buf.getSample (c, i));
                        peak = std::max (peak, std::abs (buf.getSample (c, i)));
                    }
            }
            expect (finite);
            expectLessOrEqual (peak, juce::Decibels::decibelsToGain (-1.0f) + 1.0e-4f, "limiter ceiling exceeded");
        }

        beginTest ("rack order round trip");
        rack.setOrder ({ 10, 3, 1 });
        auto o = rack.getOrder();
        expectEquals (o[0], 10); expectEquals (o[1], 3); expectEquals (o[2], 1); expectEquals (o[3], -1);
        rack.orderFromVar (rack.orderToVar());
        expectEquals (rack.getOrder()[0], 10);
        rack.setDefaultOrder();
        expectEquals (rack.getOrder()[11], 11);
    }
};

static WavetableTests wavetableTests;
static FilterTests filterTests;
static ModulationTests modulationTests;
static TuningTests tuningTests;
static FifoTests fifoTests;
static LoudnessTests loudnessTests;
static FxTests fxTests;
} // namespace bgdj::test
