#include "Filter.h"

namespace bgdj
{
namespace
{
    struct VowelF { float f[5]; float gain[5]; };
    // Formant frequencies for A E I O U (male-ish, suited to bass talkers)
    const VowelF kVowels[5] = {
        { { 730, 1090, 2440, 3400, 4500 }, { 1.0f, 0.50f, 0.25f, 0.12f, 0.06f } },
        { { 530, 1840, 2480, 3500, 4500 }, { 1.0f, 0.40f, 0.30f, 0.12f, 0.06f } },
        { { 270, 2290, 3010, 3600, 4500 }, { 1.0f, 0.25f, 0.20f, 0.10f, 0.05f } },
        { { 570, 840, 2410, 3300, 4400 },  { 1.0f, 0.60f, 0.15f, 0.10f, 0.05f } },
        { { 300, 870, 2240, 3300, 4400 },  { 1.0f, 0.35f, 0.10f, 0.08f, 0.04f } },
    };
}

void VoiceFilter::prepare (double sampleRate, int maxCombSamples)
{
    fs = sampleRate;
    combSize = std::min (1 << 16, juce::nextPowerOfTwo (std::max (64, maxCombSamples)));
    for (auto& c : chans)
        c.comb.assign ((size_t) combSize, 0.0f);
    reset();
}

void VoiceFilter::reset() noexcept
{
    for (auto& c : chans)
    {
        for (auto& s : c.svf) s.reset();
        for (auto& s : c.stage) s.reset();
        c.fbHp.reset();
        c.fatLp.reset();
        c.lastOut = 0.0f;
        std::fill (c.comb.begin(), c.comb.end(), 0.0f);
        c.combPos = 0;
    }
}

void VoiceFilter::setParams (FilterType t, float cutoffHz, float r, float d, float f, SatMode s) noexcept
{
    type = t;
    sat = s;
    reso = juce::jlimit (0.0f, 1.0f, r);
    drive = juce::jlimit (0.0f, 1.0f, d);
    fat = juce::jlimit (0.0f, 1.0f, f);
    driveGain = 1.0f + drive * drive * 24.0f;

    const float nyq = (float) fs * 0.5f;
    const float fc = juce::jlimit (16.0f, nyq * 0.92f, cutoffHz);
    g = std::tan (juce::MathConstants<float>::pi * fc / (float) fs);
    k = 2.0f - 1.97f * reso;

    for (auto& c : chans)
    {
        c.fbHp.set (std::tan (juce::MathConstants<float>::pi * 60.0f / (float) fs));
        c.fatLp.set (std::tan (juce::MathConstants<float>::pi * 140.0f / (float) fs));
    }

    switch (type)
    {
        case FilterType::LP12: case FilterType::HP12: case FilterType::BP12: case FilterType::Notch12:
        case FilterType::Peak: case FilterType::AllPass: case FilterType::DirtyLP12: case FilterType::DirtyBP:
        case FilterType::LPHPMorph:
            for (auto& c : chans) c.svf[0].set (g, k);
            morph = fat;
            break;

        case FilterType::LP24: case FilterType::HP24: case FilterType::BP24: case FilterType::Notch24:
        case FilterType::DirtyLP24:
            for (auto& c : chans) { c.svf[0].set (g, 1.414f); c.svf[1].set (g, k); }
            break;

        case FilterType::LP36:
            for (auto& c : chans) { c.svf[0].set (g, 1.85f); c.svf[1].set (g, 0.77f + 0.6f * (1.0f - reso)); c.svf[2].set (g, k); }
            break;

        case FilterType::LowShelf: case FilterType::HighShelf:
            shelfGain = std::pow (10.0f, (-18.0f + 36.0f * reso) / 20.0f);
            for (auto& c : chans) c.svf[0].set (g, 1.414f);
            break;

        case FilterType::LadderLP24: case FilterType::LadderHP24: case FilterType::DiodeLP:
            for (auto& c : chans) for (int i = 0; i < 4; ++i) c.stage[(size_t) i].set (g);
            ladderFb = reso * (type == FilterType::DiodeLP ? 4.6f : 4.1f);
            break;

        case FilterType::Acid303:
            for (auto& c : chans) for (int i = 0; i < 4; ++i) c.stage[(size_t) i].set (g * (i == 0 ? 1.0f : 1.07f));
            ladderFb = reso * 5.2f;
            break;

        case FilterType::MS20LP: case FilterType::MS20HP:
            for (auto& c : chans) { c.stage[0].set (g); c.stage[1].set (g); }
            ladderFb = reso * 2.3f;
            break;

        case FilterType::CombPlus: case FilterType::CombMinus: case FilterType::FlangeComb:
        {
            const float combHz = juce::jlimit ((float) fs / (float) (combSize - 4), nyq * 0.9f, cutoffHz);
            combDelay = (float) fs / combHz;
            combFb = reso * 0.97f;
            break;
        }

        case FilterType::FormantVowel: case FilterType::FormantTalker:
        {
            // cutoff (log) morphs A -> E -> I -> O -> U; Fat shifts formants (talker)
            const float pos = juce::jlimit (0.0f, 3.999f, std::log2 (fc / 20.0f) / std::log2 (1000.0f) * 4.0f);
            const int vi = (int) pos;
            const float frac = pos - (float) vi;
            const float shift = type == FilterType::FormantTalker ? std::pow (2.0f, (fat - 0.5f) * 1.5f) : 1.0f;
            const int n = type == FilterType::FormantTalker ? 5 : 3;
            formantK = 0.35f - 0.3f * reso;
            for (int i = 0; i < 5; ++i)
            {
                const float hz = juce::jlimit (30.0f, nyq * 0.9f, (kVowels[vi].f[i] * (1.0f - frac) + kVowels[vi + 1].f[i] * frac) * shift);
                formantG[(size_t) i] = std::tan (juce::MathConstants<float>::pi * hz / (float) fs);
                formantGain[(size_t) i] = i < n ? kVowels[vi].gain[i] * (1.0f - frac) + kVowels[vi + 1].gain[i] * frac : 0.0f;
                for (auto& c : chans) c.svf[(size_t) i].set (formantG[(size_t) i], formantK);
            }
            break;
        }

        case FilterType::Phaser4: case FilterType::Phaser8: case FilterType::Phaser12:
        {
            numStages = type == FilterType::Phaser4 ? 4 : (type == FilterType::Phaser8 ? 8 : 12);
            for (auto& c : chans)
                for (int i = 0; i < numStages; ++i)
                {
                    const float spread = std::pow (1.0f + fat * 1.5f + 0.25f, (float) i / (float) numStages * 2.0f - 1.0f);
                    c.stage[(size_t) i].set (std::tan (juce::MathConstants<float>::pi * juce::jlimit (20.0f, nyq * 0.9f, fc * spread) / (float) fs));
                }
            ladderFb = reso * 0.9f;
            break;
        }

        case FilterType::DualBP:
        {
            g2 = std::tan (juce::MathConstants<float>::pi * juce::jlimit (20.0f, nyq * 0.9f, fc * (1.5f + 3.0f * fat)) / (float) fs);
            for (auto& c : chans) { c.svf[0].set (g, k); c.svf[1].set (g2, k); }
            break;
        }

        case FilterType::Count:
        default: break;
    }
}

float VoiceFilter::readComb (Channel& c, float delay) const noexcept
{
    const float rp = (float) c.combPos - delay;
    const int mask = combSize - 1;
    const int i0 = (int) std::floor (rp);
    const float fr = rp - (float) i0;
    const float a = c.comb[(size_t) (i0 & mask)];
    const float b = c.comb[(size_t) ((i0 + 1) & mask)];
    return a + (b - a) * fr;
}

float VoiceFilter::processChannel (int ch, float x) noexcept
{
    auto& c = chans[(size_t) ch];

    if (drive > 0.0f)
        x = saturate (x * driveGain, sat) / std::sqrt (driveGain) * 1.4f;

    const float dryLow = fat > 0.0f ? c.fatLp.lp (x) : 0.0f;
    float y = 0.0f;

    switch (type)
    {
        case FilterType::LP12:    y = c.svf[0].process (x).lp; break;
        case FilterType::HP12:    y = c.svf[0].process (x).hp; break;
        case FilterType::BP12:    y = c.svf[0].process (x).bp * k; break;
        case FilterType::Notch12: { auto o = c.svf[0].process (x); y = x - k * o.bp; break; }
        case FilterType::Peak:    { auto o = c.svf[0].process (x); y = x + o.bp * k * (1.0f + reso * 6.0f); break; }
        case FilterType::AllPass: { auto o = c.svf[0].process (x); y = x - 2.0f * k * o.bp; break; }
        case FilterType::LP24:    y = c.svf[1].process (c.svf[0].process (x).lp).lp; break;
        case FilterType::HP24:    y = c.svf[1].process (c.svf[0].process (x).hp).hp; break;
        case FilterType::BP24:    y = c.svf[1].process (c.svf[0].process (x).bp * 1.414f).bp * k; break;
        case FilterType::Notch24: { auto o1 = c.svf[0].process (x); const float n1 = x - 1.414f * o1.bp; auto o2 = c.svf[1].process (n1); y = n1 - k * o2.bp; break; }
        case FilterType::LP36:    y = c.svf[2].process (c.svf[1].process (c.svf[0].process (x).lp).lp).lp; break;
        case FilterType::LowShelf:  y = x + (shelfGain - 1.0f) * c.svf[0].process (x).lp; break;
        case FilterType::HighShelf: y = x + (shelfGain - 1.0f) * c.svf[0].process (x).hp; break;

        case FilterType::LadderLP24:
        case FilterType::LadderHP24:
        {
            const float u = saturate ((x - ladderFb * c.lastOut) * (1.0f + ladderFb * 0.25f), sat);
            const float y1 = c.stage[0].lp (u);
            const float y2 = c.stage[1].lp (y1);
            const float y3 = c.stage[2].lp (y2);
            const float y4 = c.stage[3].lp (y3);
            c.lastOut = y4;
            y = type == FilterType::LadderLP24 ? y4 : (u - 4.0f * y1 + 6.0f * y2 - 4.0f * y3 + y4);
            break;
        }

        case FilterType::DiodeLP:
        {
            const float u = saturate (x - ladderFb * c.lastOut, sat);
            const float y1 = c.stage[0].lp (std::tanh (u));
            const float y2 = c.stage[1].lp (std::tanh (y1 * 1.1f));
            const float y3 = c.stage[2].lp (std::tanh (y2 * 1.1f));
            const float y4 = c.stage[3].lp (y3);
            c.lastOut = y4;
            y = y4 * (1.0f + ladderFb * 0.3f);
            break;
        }

        case FilterType::Acid303:
        {
            const float fb = c.fbHp.hp (c.lastOut);
            const float u = std::tanh (x * 1.2f - ladderFb * fb);
            const float y1 = c.stage[0].lp (u);
            const float y2 = c.stage[1].lp (std::tanh (y1));
            const float y3 = c.stage[2].lp (y2);
            const float y4 = c.stage[3].lp (y3);
            c.lastOut = y4;
            y = (0.7f * y4 + 0.3f * y3) * (1.0f + ladderFb * 0.25f);
            break;
        }

        case FilterType::MS20LP:
        case FilterType::MS20HP:
        {
            const float fbSig = saturate (c.lastOut * 1.6f, SatMode::SoftClip) / 1.6f;
            const float u = x + ladderFb * fbSig;
            const float s1 = type == FilterType::MS20LP ? c.stage[0].lp (u) : c.stage[0].hp (u);
            const float s2 = type == FilterType::MS20LP ? c.stage[1].lp (s1 - c.lastOut * ladderFb * 0.5f)
                                                        : c.stage[1].hp (s1 - c.lastOut * ladderFb * 0.5f);
            c.lastOut = juce::jlimit (-4.0f, 4.0f, s2);
            y = s2 * (1.0f + ladderFb * 0.2f);
            break;
        }

        case FilterType::DirtyLP12: y = c.svf[0].processDirty (x, sat).lp; break;
        case FilterType::DirtyBP:   y = c.svf[0].processDirty (x, sat).bp * k * 1.5f; break;
        case FilterType::DirtyLP24: y = c.svf[1].processDirty (c.svf[0].processDirty (x, sat).lp, sat).lp; break;

        case FilterType::CombPlus:
        case FilterType::CombMinus:
        {
            const float d = readComb (c, combDelay);
            const float sign = type == FilterType::CombPlus ? 1.0f : -1.0f;
            y = x + sign * combFb * d;
            c.comb[(size_t) c.combPos] = saturate (y, SatMode::SoftClip);
            c.combPos = (c.combPos + 1) & (combSize - 1);
            y *= (1.0f - combFb * 0.5f);
            break;
        }

        case FilterType::FlangeComb:
        {
            const float d = readComb (c, combDelay);
            c.comb[(size_t) c.combPos] = saturate (x + combFb * 0.8f * d, SatMode::SoftClip);
            c.combPos = (c.combPos + 1) & (combSize - 1);
            y = 0.5f * (x + d);
            break;
        }

        case FilterType::FormantVowel:
        case FilterType::FormantTalker:
        {
            const int n = type == FilterType::FormantTalker ? 5 : 3;
            for (int i = 0; i < n; ++i)
                y += c.svf[(size_t) i].process (x).bp * formantGain[(size_t) i];
            y *= (type == FilterType::FormantTalker ? 2.2f : 1.6f) * (0.6f + formantK);
            break;
        }

        case FilterType::Phaser4: case FilterType::Phaser8: case FilterType::Phaser12:
        {
            float a = x + ladderFb * c.lastOut;
            for (int i = 0; i < numStages; ++i)
                a = c.stage[(size_t) i].ap (a);
            c.lastOut = juce::jlimit (-3.0f, 3.0f, a);
            y = 0.5f * (x + a);
            break;
        }

        case FilterType::LPHPMorph:
        {
            auto o = c.svf[0].process (x);
            y = morph < 0.5f ? o.lp + (o.bp * k - o.lp) * (morph * 2.0f)
                             : o.bp * k + (o.hp - o.bp * k) * ((morph - 0.5f) * 2.0f);
            break;
        }

        case FilterType::DualBP:
            y = (c.svf[0].process (x).bp + c.svf[1].process (x).bp) * k * 0.8f;
            break;

        case FilterType::Count:
        default: y = x; break;
    }

    if (fat > 0.0f)
    {
        y += dryLow * fat * reso * 1.2f;
        y = juce::jmap (fat, y, saturate (y * (1.0f + fat), SatMode::Tanh));
    }

    // Safety net against blow-ups at extreme settings
    if (! std::isfinite (y))
    {
        reset();
        return 0.0f;
    }
    return juce::jlimit (-8.0f, 8.0f, y);
}
} // namespace bgdj
