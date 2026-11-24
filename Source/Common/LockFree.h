#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace bgdj
{
/** Single-producer / single-consumer FIFO of trivially copyable items.
    Storage is allocated once in the constructor; push/pop never allocate or lock. */
template <typename T, int Capacity>
class SpscFifo
{
public:
    static_assert (std::is_trivially_copyable_v<T>);

    bool push (const T& item) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);
        if (n1 + n2 < 1)
            return false;
        buffer[(size_t) (n1 > 0 ? s1 : s2)] = item;
        fifo.finishedWrite (1);
        return true;
    }

    bool pop (T& item) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (1, s1, n1, s2, n2);
        if (n1 + n2 < 1)
            return false;
        item = buffer[(size_t) (n1 > 0 ? s1 : s2)];
        fifo.finishedRead (1);
        return true;
    }

    int getNumReady() const noexcept { return fifo.getNumReady(); }
    void reset() noexcept            { fifo.reset(); }

private:
    juce::AbstractFifo fifo { Capacity };
    std::array<T, Capacity> buffer {};
};

/** Lock-free ring of stereo audio samples for audio -> UI analysis.
    The audio thread writes, the UI thread reads. When the reader falls behind,
    the writer drops new samples instead of blocking. */
class AudioSampleFifo
{
public:
    explicit AudioSampleFifo (int capacitySamples = 1 << 17)
        : fifo (capacitySamples), left ((size_t) capacitySamples), right ((size_t) capacitySamples) {}

    void push (const float* l, const float* r, int num) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (num, s1, n1, s2, n2);
        if (n1 > 0) { std::copy (l, l + n1, left.data() + s1); std::copy (r, r + n1, right.data() + s1); }
        if (n2 > 0) { std::copy (l + n1, l + n1 + n2, left.data() + s2); std::copy (r + n1, r + n1 + n2, right.data() + s2); }
        fifo.finishedWrite (n1 + n2);
    }

    /** Pops up to maxNum samples; returns the number copied. */
    int pop (float* l, float* r, int maxNum) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (maxNum, s1, n1, s2, n2);
        if (n1 > 0) { std::copy (left.data() + s1, left.data() + s1 + n1, l); std::copy (right.data() + s1, right.data() + s1 + n1, r); }
        if (n2 > 0) { std::copy (left.data() + s2, left.data() + s2 + n2, l + n1); std::copy (right.data() + s2, right.data() + s2 + n2, r + n1); }
        fifo.finishedRead (n1 + n2);
        return n1 + n2;
    }

    int getNumReady() const noexcept { return fifo.getNumReady(); }

private:
    juce::AbstractFifo fifo;
    std::vector<float> left, right;
};

/** Hands immutable objects from a non-realtime thread to the audio thread.
    The audio thread only performs an atomic load. Replaced objects are kept alive
    in a retire list and released later on the message thread (collectGarbage),
    once the audio thread has acknowledged a newer generation. */
template <typename T>
class RtSharedObject
{
public:
    RtSharedObject() = default;

    /** Non-realtime thread. */
    void publish (std::shared_ptr<const T> obj)
    {
        const juce::SpinLock::ScopedLockType sl (writerLock);
        if (current != nullptr)
            retired.push_back ({ current, generation.load() });
        current = std::move (obj);
        ptr.store (current.get(), std::memory_order_release);
        generation.fetch_add (1);
    }

    /** Audio thread: returns the current object (may be nullptr). */
    const T* get() const noexcept { return ptr.load (std::memory_order_acquire); }

    /** Audio thread: call once per block after it has stopped using older pointers. */
    void audioThreadAcknowledge() noexcept { acknowledged.store (generation.load()); }

    /** Non-realtime thread: deletes objects the audio thread can no longer see. */
    void collectGarbage()
    {
        const juce::SpinLock::ScopedLockType sl (writerLock);
        const auto ack = acknowledged.load();
        retired.erase (std::remove_if (retired.begin(), retired.end(),
                                       [ack] (const Retired& r) { return r.gen < ack; }),
                       retired.end());
    }

    /** Non-realtime thread, when the audio thread is known to be stopped. */
    void forceAcknowledge() { acknowledged.store (generation.load()); collectGarbage(); }

    std::shared_ptr<const T> getShared() const
    {
        const juce::SpinLock::ScopedLockType sl (writerLock);
        return current;
    }

private:
    struct Retired { std::shared_ptr<const T> obj; uint64_t gen; };
    std::atomic<const T*> ptr { nullptr };
    std::shared_ptr<const T> current;
    std::vector<Retired> retired;
    std::atomic<uint64_t> generation { 0 }, acknowledged { 0 };
    mutable juce::SpinLock writerLock;
};

/** Small deterministic PRNG usable on the audio thread. */
struct FastRandom
{
    uint32_t state = 0x12345678u;
    void seed (uint32_t s) noexcept { state = s ? s : 0x9E3779B9u; }
    uint32_t next() noexcept
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return state;
    }
    float nextFloat() noexcept   { return (float) (next() >> 8) * (1.0f / 16777216.0f); }       // [0, 1)
    float nextBipolar() noexcept { return nextFloat() * 2.0f - 1.0f; }                           // [-1, 1)
};
} // namespace bgdj
