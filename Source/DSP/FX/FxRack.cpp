#include "FxRack.h"

namespace bgdj
{
FxRack::FxRack()
{
    modules[(size_t) FxType::Distortion] = std::make_unique<DistortionFx>();
    modules[(size_t) FxType::Ott]        = std::make_unique<OttFx>();
    modules[(size_t) FxType::Compressor] = std::make_unique<CompressorFx>();
    modules[(size_t) FxType::Ducker]     = std::make_unique<DuckerFx>();
    modules[(size_t) FxType::Eq]         = std::make_unique<EqFx>();
    modules[(size_t) FxType::FilterFx]   = std::make_unique<FilterFx>();
    modules[(size_t) FxType::Modulation] = std::make_unique<ModulationFx>();
    modules[(size_t) FxType::Delay]      = std::make_unique<DelayFx>();
    modules[(size_t) FxType::Reverb]     = std::make_unique<ReverbFx>();
    modules[(size_t) FxType::Hyper]      = std::make_unique<HyperFx>();
    modules[(size_t) FxType::Saturator]  = std::make_unique<SaturatorFx>();
    modules[(size_t) FxType::Utility]    = std::make_unique<UtilityFx>();
    modules[(size_t) FxType::Limiter]    = std::make_unique<LimiterFx>();
    setDefaultOrder();
}

void FxRack::init (juce::AudioProcessorValueTreeState& s)
{
    for (int i = 0; i < kNumFxModules; ++i)
        modules[(size_t) i]->init (s, params::fxPrefix ((FxType) i));
}

void FxRack::prepare (double sampleRate, int maxBlock)
{
    fs = sampleRate;
    maxBlockSize = std::max (1, maxBlock);
    dry.setSize (2, maxBlockSize, false, true, false);
    for (int e = 0; e < kMaxOversampleExp; ++e)
    {
        oversamplers[(size_t) e] = std::make_unique<juce::dsp::Oversampling<float>> (
            2, e + 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversamplers[(size_t) e]->initProcessing ((size_t) maxBlockSize);
    }
    for (auto& m : modules)
        m->prepare (sampleRate, maxBlockSize);
}

void FxRack::reset()
{
    for (auto& m : modules)
        m->reset();
    for (auto& o : oversamplers)
        if (o) o->reset();
}

void FxRack::setDefaultOrder()
{
    // Distortion, OTT, Compressor, Ducker, EQ, Filter FX, Mod FX, Delay, Reverb, Hyper, Saturator, Utility
    for (int i = 0; i < kNumFxSlots; ++i)
        order[(size_t) i].store (i);
    ++orderVersion;
}

juce::Array<int> FxRack::getOrder() const
{
    juce::Array<int> r;
    for (auto& o : order)
        r.add (o.load());
    return r;
}

void FxRack::setOrder (const juce::Array<int>& newOrder)
{
    juce::Array<int> used;
    int w = 0;
    for (int t : newOrder)
    {
        if (w >= kNumFxSlots) break;
        if (t < 0 || t >= (int) FxType::Limiter || used.contains (t))
            continue;
        used.add (t);
        order[(size_t) w++].store (t);
    }
    while (w < kNumFxSlots)
        order[(size_t) w++].store (-1);
    ++orderVersion;
}

juce::var FxRack::orderToVar() const
{
    juce::Array<juce::var> arr;
    for (auto& o : order)
        if (const int t = o.load(); t >= 0)
            arr.add (params::fxModuleIds()[t]);
    return arr;
}

void FxRack::orderFromVar (const juce::var& v)
{
    juce::Array<int> o;
    if (auto* arr = v.getArray())
        for (auto& e : *arr)
        {
            const int idx = e.isString() ? params::fxModuleIds().indexOf (e.toString()) : (int) e;
            if (idx >= 0) o.add (idx);
        }
    if (o.isEmpty())
        setDefaultOrder();
    else
        setOrder (o);
}

void FxRack::runModule (FxModule& m, float* L, float* R, int n, const FxContext& ctx, int osIdx) noexcept
{
    if (! m.isNonLinear() || osIdx <= 0)
    {
        m.process (L, R, n, ctx);
        return;
    }
    auto& os = *oversamplers[(size_t) juce::jlimit (0, kMaxOversampleExp - 1, osIdx - 1)];
    float* chans[2] = { L, R };
    juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
    auto up = os.processSamplesUp (block);
    FxContext c2 = ctx;
    c2.sampleRate = ctx.sampleRate * (double) (1 << osIdx);
    c2.scL = c2.scR = nullptr;
    m.process (up.getChannelPointer (0), up.getChannelPointer (1), (int) up.getNumSamples(), c2);
    os.processSamplesDown (block);
}

void FxRack::process (juce::AudioBuffer<float>& buffer, int numSamples, FxContext ctx, int fxOsIndex) noexcept
{
    ctx.sampleRate = fs;
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // chunk to the prepared size, in case the host sends a bigger block than announced
    for (int start = 0; start < numSamples; start += maxBlockSize)
    {
        const int n = std::min (maxBlockSize, numSamples - start);
        FxContext c = ctx;
        c.ppq = ctx.ppq + (double) start / fs * ctx.bpm / 60.0;
        if (ctx.scL) c.scL = ctx.scL + start;
        if (ctx.scR) c.scR = ctx.scR + start;

        auto runWithMix = [&] (FxModule& m)
        {
            if (! m.on.on())
                return;
            const float mix = m.mix.get (c.modOffsets);
            if (mix <= 0.0f)
                return;
            float* dl = dry.getWritePointer (0);
            float* dr = dry.getWritePointer (1);
            std::copy (L + start, L + start + n, dl);
            std::copy (R + start, R + start + n, dr);
            runModule (m, L + start, R + start, n, c, fxOsIndex);
            if (mix < 1.0f)
                for (int i = 0; i < n; ++i)
                {
                    L[start + i] = dl[i] + (L[start + i] - dl[i]) * mix;
                    R[start + i] = dr[i] + (R[start + i] - dr[i]) * mix;
                }
        };

        for (int s = 0; s < kNumFxSlots; ++s)
        {
            const int t = order[(size_t) s].load (std::memory_order_relaxed);
            if (t >= 0 && t < (int) FxType::Limiter)
                runWithMix (*modules[(size_t) t]);
        }
        runWithMix (*modules[(size_t) FxType::Limiter]);
    }
}
} // namespace bgdj
