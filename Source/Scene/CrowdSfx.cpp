#include "CrowdSfx.h"

namespace bgdj
{
CrowdSfx::Cue CrowdSfx::cueFromName (const juce::String& n)
{
    static const std::pair<const char*, Cue> map[] = {
        { "cheer", Cheer }, { "oooh", Oooh }, { "one_more", OneMore }, { "rewind", Rewind }, { "airhorn", AirHorn },
        { "boo", Boo }, { "play_else", PlayElse }, { "ringtone", Ringtone }, { "gasp", Gasp }, { "pigeon", Pigeon },
        { "meow", Meow }, { "crash", Crash }, { "cup", Cup }, { "chatter", Chatter }, { "snore", Snore }, { "co2", Co2 }, { "smoke", Smoke } };
    for (auto& [k, c] : map)
        if (n == k) return c;
    return None;
}

void CrowdSfx::prepare (double sampleRate)
{
    fs = sampleRate;
    for (auto& v : voices) v.cue = None;
    rng.seed (77);
}

static float env (double t, double dur, double a, double r) noexcept
{
    if (t < a) return (float) (t / a);
    if (t > dur - r) return (float) std::max (0.0, (dur - t) / r);
    return 1.0f;
}

float CrowdSfx::renderVoice (VoiceState& v) noexcept
{
    const double dt = 1.0 / fs;
    const double t = v.t;
    v.t += dt;
    auto tanG = [this] (float hz) { return std::tan (juce::MathConstants<float>::pi * juce::jlimit (20.0f, (float) fs * 0.45f, hz) / (float) fs); };
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float noise = v.rng.nextBipolar();
    float s = 0.0f;

    switch (v.cue)
    {
        case Cheer: case Chatter: case OneMore:
        {
            const float rate = v.cue == OneMore ? 2.2f : 4.0f;
            const float syl = v.cue == Chatter ? 0.5f + 0.5f * std::sin (twoPi * 3.1f * (float) t) * std::sin (twoPi * 1.3f * (float) t)
                                               : 0.6f + 0.4f * std::abs (std::sin (twoPi * rate * (float) t));
            v.f1.set (tanG (v.cue == Chatter ? 500.0f : 900.0f + 300.0f * std::sin ((float) t * 3.0f)), 1.2f);
            v.f2.set (tanG (2400.0f), 1.5f);
            s = (v.f1.process (noise).bp * 1.5f + v.f2.process (noise).bp * 0.6f) * syl * env (t, v.dur, 0.3, 0.8) * (v.cue == Chatter ? 0.4f : 0.9f);
            break;
        }
        case Oooh: case Boo:
        {
            const float base = v.cue == Oooh ? 220.0f + 140.0f * (float) std::min (1.0, t / v.dur) : 140.0f - 30.0f * (float) (t / v.dur);
            v.ph1 += (base * (1.0f + 0.01f * std::sin (twoPi * 5.0f * (float) t))) * dt;
            v.ph2 += base * 1.007f * dt;
            v.ph3 += base * 0.994f * dt;
            const float src = (float) ((v.ph1 - std::floor (v.ph1)) + (v.ph2 - std::floor (v.ph2)) + (v.ph3 - std::floor (v.ph3))) - 1.5f;
            v.f1.set (tanG (v.cue == Oooh ? 320.0f : 450.0f), 4.0f);
            v.f2.set (tanG (v.cue == Oooh ? 800.0f : 1000.0f), 5.0f);
            s = (v.f1.process (src + 0.2f * noise).bp + 0.5f * v.f2.process (src).bp) * env (t, v.dur, 0.25, 0.6) * 1.4f;
            break;
        }
        case Rewind:
        {
            const float hz = 900.0f * std::pow (0.12f, (float) (t / v.dur)) * (1.0f + 0.5f * std::sin (twoPi * 9.0f * (float) t));
            v.ph1 += hz * dt;
            s = ((float) (v.ph1 - std::floor (v.ph1)) * 2.0f - 1.0f) * 0.5f * env (t, v.dur, 0.01, 0.2);
            break;
        }
        case AirHorn:
        {
            const float trem = 0.75f + 0.25f * std::sin (twoPi * 18.0f * (float) t);
            v.ph1 += 466.0f * dt; v.ph2 += 587.0f * dt; v.ph3 += 698.0f * dt;
            auto sq = [] (double p) { return (p - std::floor (p)) < 0.5 ? 1.0f : -1.0f; };
            v.f1.set (tanG (2500.0f), 0.7f);
            s = v.f1.process ((sq (v.ph1) + sq (v.ph2) + sq (v.ph3)) * 0.22f).lp * trem * env (t, v.dur, 0.01, 0.1);
            break;
        }
        case PlayElse: case Gasp:
        {
            v.f1.set (tanG (v.cue == Gasp ? 3000.0f : 1100.0f), v.cue == Gasp ? 0.8f : 2.0f);
            s = v.f1.process (noise).bp * env (t, v.dur, 0.02, 0.3) * (v.cue == Gasp ? 0.6f : 1.0f) * (0.6f + 0.4f * std::sin (twoPi * 6.0f * (float) t));
            break;
        }
        case Ringtone:
        {
            static const float notes[] = { 1318.5f, 1174.7f, 740.0f, 830.6f, 1108.7f, 987.8f, 587.3f, 659.3f };
            const int idx = std::min (7, (int) (t / 0.14));
            v.ph1 += notes[idx] * dt;
            s = std::sin (twoPi * (float) v.ph1) * 0.35f * env (t - idx * 0.14, 0.14, 0.005, 0.03);
            break;
        }
        case Pigeon:
        {
            const float hz = 380.0f + 80.0f * std::sin (twoPi * 2.5f * (float) t);
            v.ph1 += hz * dt;
            s = std::sin (twoPi * (float) v.ph1 + 2.0f * std::sin (twoPi * 25.0f * (float) t)) * 0.4f * std::abs (std::sin (twoPi * 1.2f * (float) t)) * env (t, v.dur, 0.05, 0.3);
            break;
        }
        case Meow:
        {
            const float p = (float) (t / v.dur);
            const float hz = 550.0f + 400.0f * std::sin (juce::MathConstants<float>::pi * p);
            v.ph1 += hz * dt;
            const float src = (float) (v.ph1 - std::floor (v.ph1)) * 2.0f - 1.0f;
            v.f1.set (tanG (700.0f + 1500.0f * p), 3.0f);
            s = v.f1.process (src).bp * env (t, v.dur, 0.03, 0.2) * 1.2f;
            break;
        }
        case Crash: case Cup:
        {
            v.f1.set (tanG (v.cue == Cup ? 2800.0f : 5000.0f), 1.0f);
            const float decay = std::exp (-(float) t * (v.cue == Cup ? 25.0f : 6.0f));
            v.ph1 += 3150.0 * dt;
            s = (v.f1.process (noise).bp + 0.3f * std::sin (twoPi * (float) v.ph1)) * decay;
            break;
        }
        case Snore:
        {
            const float cycle = (float) std::fmod (t, 2.0) / 2.0f;
            v.f1.set (tanG (300.0f), 3.0f);
            s = v.f1.process (noise).bp * std::max (0.0f, std::sin (juce::MathConstants<float>::pi * cycle * 1.6f)) * 0.9f;
            break;
        }
        case Co2: case Smoke:
        {
            v.f1.set (tanG (v.cue == Co2 ? 4500.0f : 2000.0f), 0.6f);
            s = v.f1.process (noise).hp * env (t, v.dur, 0.02, v.dur * 0.7) * (v.cue == Co2 ? 0.7f : 0.35f);
            break;
        }
        default: break;
    }

    if (v.t >= v.dur)
        v.cue = None;
    return s;
}

void CrowdSfx::render (float* L, float* R, int n, float gain) noexcept
{
    int c;
    while (queue.pop (c))
    {
        for (auto& v : voices)
        {
            if (v.cue != None)
                continue;
            v = VoiceState();
            v.cue = (Cue) c;
            v.rng.seed (rng.next());
            v.pan = rng.nextBipolar() * 0.6f;
            static const double durs[NumCues] = { 0, 2.5, 2.2, 2.8, 1.2, 1.4, 2.0, 1.2, 1.15, 0.5, 1.8, 0.9, 0.8, 0.25, 4.0, 4.0, 1.5, 2.5 };
            v.dur = durs[c];
            break;
        }
    }

    for (auto& v : voices)
    {
        if (v.cue == None)
            continue;
        const float gl = gain * (1.0f - v.pan) * 0.7f, gr = gain * (1.0f + v.pan) * 0.7f;
        for (int i = 0; i < n && v.cue != None; ++i)
        {
            const float s = renderVoice (v);
            L[i] += s * gl;
            R[i] += s * gr;
        }
    }
}
} // namespace bgdj
