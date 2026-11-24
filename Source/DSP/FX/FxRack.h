#pragma once

#include "FxModules.h"

namespace bgdj
{
/** Reorderable rack of 12 slots plus the fixed final limiter. The order lives in atomics
    so the UI can drag modules around while audio runs. */
class FxRack
{
public:
    FxRack();

    void init (juce::AudioProcessorValueTreeState& s);
    void prepare (double sampleRate, int maxBlock);
    void reset();

    /** Processes a stereo buffer in place. fxOsIndex: 0=1x .. 4=16x for non-linear modules. */
    void process (juce::AudioBuffer<float>& buffer, int numSamples, FxContext ctx, int fxOsIndex) noexcept;

    FxModule* getModule (FxType t) const noexcept { return modules[(size_t) t].get(); }
    DuckerFx& ducker() const noexcept              { return *static_cast<DuckerFx*> (modules[(size_t) FxType::Ducker].get()); }
    EqFx& eq() const noexcept                      { return *static_cast<EqFx*> (modules[(size_t) FxType::Eq].get()); }

    /** Rack order: module type per slot (FxType int), -1 = empty. */
    std::array<std::atomic<int>, kNumFxSlots> order;
    std::atomic<int> orderVersion { 0 };

    juce::Array<int> getOrder() const;
    void setOrder (const juce::Array<int>& newOrder);
    void setDefaultOrder();
    juce::var orderToVar() const;
    void orderFromVar (const juce::var& v);

    int getLatencySamples() const noexcept { return 0; }

private:
    std::array<std::unique_ptr<FxModule>, kNumFxModules> modules;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, kMaxOversampleExp> oversamplers;
    juce::AudioBuffer<float> dry;
    double fs = 48000.0;
    int maxBlockSize = 512;

    void runModule (FxModule& m, float* L, float* R, int n, const FxContext& ctx, int fxOsIndex) noexcept;
};
} // namespace bgdj
