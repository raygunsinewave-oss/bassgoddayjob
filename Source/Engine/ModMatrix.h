#pragma once

#include "../Common/Parameters.h"
#include "../Common/LockFree.h"
#include <array>

namespace bgdj
{
struct ModSlot
{
    int src = 0;        // ModSource
    int aux = 0;        // ModSource (0 = none), multiplies the source
    int dst = -1;       // index into params::modDestinations()
    float amount = 0.0f;  // -1..1 (normalised parameter range)
    float curve = 0.0f;   // -1..1
    float output = 1.0f;  // 0..1
    bool bipolar = false;
};

/** Audio-thread copy of the active slots. */
struct ModMatrixSnapshot
{
    std::array<ModSlot, kNumModSlots> slots {};
    int numActive = 0;
};

/** 64-slot modulation matrix. Every field is an atomic so the message thread can edit
    while the audio thread takes a snapshot once per block. */
class ModMatrix
{
public:
    ModMatrix() { clear(); }

    void clear();
    ModSlot getSlot (int i) const;
    void setSlot (int i, const ModSlot& s);
    void clearSlot (int i)            { setSlot (i, {}); }

    /** Adds a routing (or updates an existing src/dst pair). Returns the slot index or -1 if full. */
    int addRouting (int src, int dst, float amount);
    /** Slots that target the given destination. */
    juce::Array<int> slotsForDest (int dst) const;
    int findFreeSlot() const;

    void takeSnapshot (ModMatrixSnapshot& s) const noexcept;

    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree& v);
    juce::var toVar() const;               // preset JSON
    void fromVar (const juce::var& v);

    std::atomic<int> changeCounter { 0 };   // UI polls this to redraw rings

private:
    struct AtomicSlot
    {
        std::atomic<int> src { 0 }, aux { 0 }, dst { -1 };
        std::atomic<float> amount { 0.0f }, curve { 0.0f }, output { 1.0f };
        std::atomic<bool> bipolar { false };
    };
    std::array<AtomicSlot, kNumModSlots> slots;
};

/** Applies curve/aux/output for a slot. */
inline float evalModSlot (const ModSlot& s, const float* sourceValues) noexcept
{
    float v = sourceValues[s.src];
    if (s.bipolar)
        v = v * 2.0f - 1.0f;
    if (s.curve != 0.0f)
    {
        const float e = std::pow (4.0f, s.curve);
        v = v < 0.0f ? -std::pow (-v, e) : std::pow (v, e);
    }
    if (s.aux > 0)
        v *= sourceValues[s.aux];
    return v * s.amount * s.output;
}
} // namespace bgdj
