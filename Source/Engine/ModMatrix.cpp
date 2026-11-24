#include "ModMatrix.h"

namespace bgdj
{
void ModMatrix::clear()
{
    for (int i = 0; i < kNumModSlots; ++i)
        setSlot (i, {});
}

ModSlot ModMatrix::getSlot (int i) const
{
    const auto& a = slots[(size_t) i];
    ModSlot s;
    s.src = a.src.load(); s.aux = a.aux.load(); s.dst = a.dst.load();
    s.amount = a.amount.load(); s.curve = a.curve.load(); s.output = a.output.load(); s.bipolar = a.bipolar.load();
    return s;
}

void ModMatrix::setSlot (int i, const ModSlot& s)
{
    if (! juce::isPositiveAndBelow (i, kNumModSlots))
        return;
    auto& a = slots[(size_t) i];
    const int nd = (int) params::modDestinations().size();
    a.src = juce::jlimit (0, (int) ModSource::Count - 1, s.src);
    a.aux = juce::jlimit (0, (int) ModSource::Count - 1, s.aux);
    a.dst = s.dst >= 0 && s.dst < nd ? s.dst : -1;
    a.amount = juce::jlimit (-1.0f, 1.0f, s.amount);
    a.curve = juce::jlimit (-1.0f, 1.0f, s.curve);
    a.output = juce::jlimit (0.0f, 1.0f, s.output);
    a.bipolar = s.bipolar;
    ++changeCounter;
}

int ModMatrix::findFreeSlot() const
{
    for (int i = 0; i < kNumModSlots; ++i)
        if (slots[(size_t) i].src.load() == 0 || slots[(size_t) i].dst.load() < 0)
            return i;
    return -1;
}

int ModMatrix::addRouting (int src, int dst, float amount)
{
    for (int i = 0; i < kNumModSlots; ++i)
    {
        auto s = getSlot (i);
        if (s.src == src && s.dst == dst)
        {
            s.amount = amount;
            setSlot (i, s);
            return i;
        }
    }
    const int i = findFreeSlot();
    if (i < 0)
        return -1;
    ModSlot s;
    s.src = src; s.dst = dst; s.amount = amount;
    setSlot (i, s);
    return i;
}

juce::Array<int> ModMatrix::slotsForDest (int dst) const
{
    juce::Array<int> r;
    for (int i = 0; i < kNumModSlots; ++i)
        if (slots[(size_t) i].dst.load() == dst && slots[(size_t) i].src.load() > 0)
            r.add (i);
    return r;
}

void ModMatrix::takeSnapshot (ModMatrixSnapshot& s) const noexcept
{
    s.numActive = 0;
    for (int i = 0; i < kNumModSlots; ++i)
    {
        const auto& a = slots[(size_t) i];
        const int src = a.src.load (std::memory_order_relaxed);
        const int dst = a.dst.load (std::memory_order_relaxed);
        const float amt = a.amount.load (std::memory_order_relaxed);
        if (src <= 0 || dst < 0 || amt == 0.0f)
            continue;
        auto& o = s.slots[(size_t) s.numActive++];
        o.src = src; o.dst = dst; o.amount = amt;
        o.aux = a.aux.load (std::memory_order_relaxed);
        o.curve = a.curve.load (std::memory_order_relaxed);
        o.output = a.output.load (std::memory_order_relaxed);
        o.bipolar = a.bipolar.load (std::memory_order_relaxed);
    }
}

juce::ValueTree ModMatrix::toValueTree() const
{
    juce::ValueTree v ("MODMATRIX");
    const auto& dests = params::modDestinations();
    for (int i = 0; i < kNumModSlots; ++i)
    {
        const auto s = getSlot (i);
        if (s.src <= 0 || s.dst < 0)
            continue;
        juce::ValueTree sv ("SLOT");
        sv.setProperty ("index", i, nullptr);
        sv.setProperty ("src", params::modSourceNames()[s.src], nullptr);
        sv.setProperty ("aux", params::modSourceNames()[s.aux], nullptr);
        sv.setProperty ("dst", dests[(size_t) s.dst].paramID, nullptr);
        sv.setProperty ("amount", s.amount, nullptr);
        sv.setProperty ("curve", s.curve, nullptr);
        sv.setProperty ("output", s.output, nullptr);
        sv.setProperty ("bipolar", s.bipolar, nullptr);
        v.appendChild (sv, nullptr);
    }
    return v;
}

static ModSlot slotFromProps (const juce::var& src, const juce::var& aux, const juce::var& dst,
                              const juce::var& amount, const juce::var& curve, const juce::var& output, const juce::var& bip)
{
    ModSlot s;
    s.src = std::max (0, params::modSourceNames().indexOf (src.toString()));
    s.aux = aux.isVoid() ? 0 : std::max (0, params::modSourceNames().indexOf (aux.toString()));
    s.dst = params::modDestIndexForParam (dst.toString());
    s.amount = amount.isVoid() ? 0.0f : (float) amount;
    s.curve = curve.isVoid() ? 0.0f : (float) curve;
    s.output = output.isVoid() ? 1.0f : (float) output;
    s.bipolar = bip.isVoid() ? false : (bool) bip;
    return s;
}

void ModMatrix::fromValueTree (const juce::ValueTree& v)
{
    clear();
    int next = 0;
    for (const auto& sv : v)
    {
        const int idx = sv.hasProperty ("index") ? (int) sv["index"] : next;
        setSlot (idx, slotFromProps (sv["src"], sv["aux"], sv["dst"], sv["amount"], sv["curve"], sv["output"], sv["bipolar"]));
        next = idx + 1;
    }
}

juce::var ModMatrix::toVar() const
{
    juce::Array<juce::var> arr;
    const auto& dests = params::modDestinations();
    for (int i = 0; i < kNumModSlots; ++i)
    {
        const auto s = getSlot (i);
        if (s.src <= 0 || s.dst < 0)
            continue;
        auto* o = new juce::DynamicObject();
        o->setProperty ("src", params::modSourceNames()[s.src]);
        if (s.aux > 0) o->setProperty ("aux", params::modSourceNames()[s.aux]);
        o->setProperty ("dst", dests[(size_t) s.dst].paramID);
        o->setProperty ("amount", s.amount);
        if (s.curve != 0.0f) o->setProperty ("curve", s.curve);
        if (s.output != 1.0f) o->setProperty ("output", s.output);
        if (s.bipolar) o->setProperty ("bipolar", true);
        arr.add (juce::var (o));
    }
    return arr;
}

void ModMatrix::fromVar (const juce::var& v)
{
    clear();
    if (auto* arr = v.getArray())
    {
        int i = 0;
        for (auto& e : *arr)
        {
            if (i >= kNumModSlots)
                break;
            setSlot (i++, slotFromProps (e["src"], e["aux"], e["dst"], e["amount"], e["curve"], e["output"], e["bipolar"]));
        }
    }
}
} // namespace bgdj
