#include "Voice.h"

namespace bgdj
{
// ============================================================================
void DestIndex::build()
{
    auto idx = [] (const juce::String& id) { return params::modDestIndexForParam (id); };
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        const auto p = params::oscPrefix (o);
        oscPos[o] = idx (p + "pos");       oscWarp[o] = idx (p + "warp_amt");
        oscDetune[o] = idx (p + "detune"); oscBlend[o] = idx (p + "blend");
        oscWidth[o] = idx (p + "width");   oscCoarse[o] = idx (p + "coarse");
        oscFine[o] = idx (p + "fine");     oscPhase[o] = idx (p + "phase");
        oscLevel[o] = idx (p + "level");   oscPan[o] = idx (p + "pan");
        oscUniWarp[o] = idx (p + "uni_warp");
    }
    dLevel = idx ("oD_level"); dPan = idx ("oD_pan"); dNoisePitch = idx ("oD_noise_pitch");
    for (int f = 0; f < kNumFilters; ++f)
    {
        const auto p = params::filterPrefix (f);
        fCut[f] = idx (p + "cutoff"); fRes[f] = idx (p + "reso"); fDrive[f] = idx (p + "drive");
        fFat[f] = idx (p + "fat");    fMix[f] = idx (p + "mix");  fPan[f] = idx (p + "pan");
    }
    for (int e = 0; e < kNumEnvs; ++e)
    {
        const auto p = params::envPrefix (e);
        envA[e] = idx (p + "att"); envD[e] = idx (p + "dec"); envS[e] = idx (p + "sus"); envR[e] = idx (p + "rel");
    }
    for (int l = 0; l < kNumLfos; ++l)
    {
        lfoRate[l] = idx (params::lfoPrefix (l) + "rate");
        lfoSmooth[l] = idx (params::lfoPrefix (l) + "smooth");
    }
    porta = idx ("porta_time");
    ampVel = idx ("amp_vel");
}

// ============================================================================
float Envelope::advance (float dt, float attMs, float decMs, float sus, float relMs, float ac, float dc, float rc) noexcept
{
    const float dtMs = dt * 1000.0f;
    switch (stage)
    {
        case Stage::Idle:
            level = 0.0f;
            break;

        case Stage::Attack:
            if (attMs <= 0.05f)
            {
                level = 1.0f; stage = Stage::Decay; t = 0.0f;
                break;
            }
            t += dtMs / attMs;
            if (t >= 1.0f) { level = 1.0f; stage = Stage::Decay; t = 0.0f; }
            else level = startLevel + (1.0f - startLevel) * bendSegment (t, ac);
            break;

        case Stage::Decay:
            t += dtMs / std::max (0.1f, decMs);
            if (t >= 1.0f) { level = sus; stage = Stage::Sustain; t = 0.0f; }
            else level = sus + (1.0f - sus) * (1.0f - bendSegment (t, -dc));
            break;

        case Stage::Sustain:
            level = sus;
            break;

        case Stage::Release:
            t += dtMs / std::max (0.1f, relMs);
            if (t >= 1.0f || relStart <= 1.0e-5f) { level = 0.0f; stage = Stage::Idle; t = 0.0f; }
            else level = relStart * (1.0f - bendSegment (t, -rc));
            break;
    }
    return level;
}

// ============================================================================
void Lfo::noteOn (const LfoStatic& s, FastRandom& rng, bool retrigger) noexcept
{
    if (s.mode == 2 && ! first)   // Free running: keep phase
        return;
    if (retrigger || first)
    {
        phase = s.phase;
        finished = false;
        riseT = 0.0f;
        shPrev = rng.nextFloat();
        shNext = rng.nextFloat();
        first = false;
    }
}

float Lfo::advance (float dt, float rateHz, float smooth, const LfoStatic& s, double ppqNow, bool playing, FastRandom& rng) noexcept
{
    const double prev = phase;
    if (s.mode == 2 && s.sync && playing)
    {
        const double beats = params::syncRateBeats (s.syncRate);
        phase = ppqNow / beats + s.phase;
        phase -= std::floor (phase);
    }
    else if (! (s.mode == 1 && finished))
    {
        phase += (double) rateHz * dt;
        if (phase >= 1.0)
        {
            if (s.mode == 1) { phase = 1.0; finished = true; }
            else phase -= std::floor (phase);
        }
    }

    const bool wrapped = phase < prev;
    if (wrapped)
    {
        shPrev = shNext;
        shNext = rng.nextFloat();
    }

    float v;
    if (s.shape == 6)        v = shNext;
    else if (s.shape == 7)   v = shPrev + (shNext - shPrev) * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * (float) phase));
    else if (s.table)        v = s.table->lookup ((float) phase);
    else                     v = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * (float) phase);

    if (smooth > 0.0f)
    {
        const float coef = std::exp (-dt / (smooth * smooth * 0.25f + 1.0e-4f));
        smoothed = v + coef * (smoothed - v);
        v = smoothed;
    }
    else
        smoothed = v;

    riseT += dt * 1000.0f;
    const float riseGain = s.rise > 0.0f ? std::min (1.0f, riseT / s.rise) : 1.0f;

    out = s.bipolar ? (v * 2.0f - 1.0f) * riseGain : v * riseGain;
    return out;
}

// ============================================================================
void Voice::prepare (double osSampleRate, double maxOsSampleRate)
{
    for (auto& f : filter)
        f.prepare (osSampleRate, (int) std::ceil (maxOsSampleRate / 20.0) + 4);
    setSampleRate (osSampleRate);
}

void Voice::setSampleRate (double osSampleRate) noexcept
{
    sr = osSampleRate;
    for (auto& f : filter)
        f.setSampleRate (sr);
    kill();
}

void Voice::kill() noexcept
{
    active = false;
    keyHeld = sustained = false;
    for (auto& e : env) e.kill();
    for (auto& f : filter) f.reset();
    note = -1;
}

float Voice::effective (const EngineSnapshot& s, int dest) const noexcept
{
    if (dest < 0)
        return 0.0f;
    const float off = destOffset[(size_t) dest];
    if (off == 0.0f)
        return s.baseReal[dest];
    return s.destParam[dest]->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, s.baseNorm[dest] + off));
}

float Voice::noteToHz (const EngineSnapshot& s, float n) const noexcept
{
    const auto& hz = s.tuning->hz;
    if (n <= 0.0f)  return (float) (hz[0] * std::pow (2.0, n / 12.0));
    if (n >= 127.0f) return (float) (hz[127] * std::pow (2.0, (n - 127.0f) / 12.0));
    const int i = (int) n;
    const float f = n - (float) i;
    return (float) (hz[(size_t) i] * std::pow (hz[(size_t) i + 1] / hz[(size_t) i], (double) f));
}

void Voice::resetOscPhases (const EngineSnapshot& s) noexcept
{
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        if (active && ! s.osc[o].phaseReset)
            continue;
        auto& w = osc[(size_t) o];
        const int n = s.osc[o].unison;
        for (int u = 0; u < kMaxUnison; ++u)
        {
            float p = 0.0f;
            switch (s.uniPhase)
            {
                case 0:  p = rng.nextFloat() * s.osc[o].randPhase; break;
                case 1:  p = (n > 1 ? (float) u / (float) n : 0.0f) * s.osc[o].randPhase; break;
                default: p = 0.0f; break;
            }
            if (s.uniPhase == 0 && n == 1)
                p = rng.nextFloat() * s.osc[o].randPhase;
            w.phase[(size_t) u] = p;
        }
    }
    if (! active || s.oscD.phaseReset)
    {
        subPhase = 0.0;
        noisePos = s.oscD.noise != nullptr ? (s.oscD.phaseReset ? 0.0 : rng.nextFloat() * s.oscD.noise->length) : 0.0;
    }
}

void Voice::noteOn (int n, float vel, int ch, bool legatoTransition, float glideFromNote,
                    const EngineSnapshot& s, uint32_t seed, bool retrigFromCurrent, uint64_t newAge)
{
    juce::ignoreUnused (legatoTransition);
    note = n; velocity = vel; channel = ch; age = newAge;
    keyHeld = true; sustained = false;
    mpeBend = mpePressure = mpeSlide = polyAftertouch = 0.0f;

    rng.seed (seed);
    noteRandom = rng.nextFloat();
    for (auto& w : osc)
        w.driftCents = rng.nextBipolar() * s.drift * 6.0f;

    targetNote = (float) n;
    const float portaMs = effective (s, s.dest.porta);
    if (glideFromNote >= 0.0f && portaMs > 0.5f && std::abs (glideFromNote - targetNote) > 0.001f)
    {
        currentNote = active ? currentNote : glideFromNote;
        if (! active) currentNote = glideFromNote;
        const float interval = std::abs (targetNote - currentNote);
        float dur = portaMs;
        if (s.portaType == 1) dur = portaMs * interval / 12.0f;
        else if (s.portaType == 2) dur = portaMs * std::sqrt (interval / 12.0f);
        glideRate = interval / std::max (0.001f, dur / 1000.0f);
        glideActive = true;
    }
    else
    {
        currentNote = targetNote;
        glideActive = false;
    }

    const bool fromCurrent = active && retrigFromCurrent;
    for (auto& e : env)
        e.noteOn (fromCurrent);
    for (int l = 0; l < kNumLfos; ++l)
        lfo[(size_t) l].noteOn (s.lfo[l], rng, s.lfo[l].mode != 2);

    if (! active)
        for (auto& f : filter) f.reset();
    resetOscPhases (s);

    ampStart = fromCurrent ? env[0].getLevel() : 0.0f;
    ampEnd = ampStart;
    active = true;
    ctrlCountdown = 0;
    samplesSinceStart = 0.0;
}

void Voice::retarget (int n, float vel, const EngineSnapshot& s, bool retriggerEnvs, bool glide, float glideFromNote)
{
    note = n;
    velocity = vel;
    keyHeld = true;
    sustained = false;
    targetNote = (float) n;

    const float portaMs = effective (s, s.dest.porta);
    if (glide && portaMs > 0.5f)
    {
        if (glideFromNote >= 0.0f && ! glideActive)
            currentNote = glideFromNote;
        const float interval = std::abs (targetNote - currentNote);
        float dur = portaMs;
        if (s.portaType == 1) dur = portaMs * interval / 12.0f;
        else if (s.portaType == 2) dur = portaMs * std::sqrt (interval / 12.0f);
        glideRate = interval / std::max (0.001f, dur / 1000.0f);
        glideActive = interval > 0.0001f;
    }
    else
    {
        currentNote = targetNote;
        glideActive = false;
    }

    if (retriggerEnvs)
    {
        for (auto& e : env)
            e.noteOn (s.envRetrig == 1 || s.polyMode == 1);
        for (int l = 0; l < kNumLfos; ++l)
            lfo[(size_t) l].noteOn (s.lfo[l], rng, s.lfo[l].mode != 2);
        resetOscPhases (s);
    }
    active = true;
}

void Voice::noteOff() noexcept
{
    keyHeld = false;
    for (auto& e : env)
        e.noteOff();
}

// ============================================================================
void Voice::updateControl (const EngineSnapshot& s, int offsetInSegment) noexcept
{
    const float dt = (float) kControlBlock / (float) sr;
    const auto& d = s.dest;

    // ---- envelopes ---------------------------------------------------------------
    ampStart = ampEnd;
    for (int e = 0; e < kNumEnvs; ++e)
        env[(size_t) e].advance (dt, effective (s, d.envA[e]), effective (s, d.envD[e]), effective (s, d.envS[e]),
                                 effective (s, d.envR[e]), s.envCurve[e][0], s.envCurve[e][1], s.envCurve[e][2]);
    ampEnd = env[0].getLevel();

    // ---- LFOs ----------------------------------------------------------------------
    const double ppqNow = s.ppqAtBlockStart + (double) offsetInSegment / sr * s.bpm / 60.0;   // engine sets ppq per segment
    for (int l = 0; l < kNumLfos; ++l)
    {
        const auto& ls = s.lfo[l];
        float hz;
        if (ls.sync)
        {
            const double beats = params::syncRateBeats (ls.syncRate);
            const float off = destOffset[(size_t) d.lfoRate[l]];
            hz = (float) (s.bpm / 60.0 / beats) * std::pow (2.0f, off * 8.0f);
        }
        else
            hz = effective (s, d.lfoRate[l]);
        sources[(size_t) ModSource::Lfo1 + (size_t) l] =
            lfo[(size_t) l].advance (dt, hz, effective (s, d.lfoSmooth[l]), ls, ppqNow, s.playing, rng);
    }

    // ---- mod sources ------------------------------------------------------------------
    for (int e = 0; e < kNumEnvs; ++e)
        sources[(size_t) ModSource::Env1 + (size_t) e] = env[(size_t) e].getLevel();
    for (int m = 0; m < kNumMacros; ++m)
        sources[(size_t) ModSource::Macro1 + (size_t) m] = s.macros[m];
    sources[(size_t) ModSource::Velocity]   = velocity;
    sources[(size_t) ModSource::NoteRandom] = noteRandom;
    sources[(size_t) ModSource::KeyTrack]   = (float) note / 127.0f;
    sources[(size_t) ModSource::ModWheel]   = s.modWheel;
    sources[(size_t) ModSource::PitchBend]  = s.pitchBend + mpeBend;
    sources[(size_t) ModSource::Aftertouch] = std::max ({ s.aftertouch, polyAftertouch, mpePressure });
    sources[(size_t) ModSource::MpeSlide]   = mpeSlide;
    sources[(size_t) ModSource::MpePressure]= mpePressure;
    sources[(size_t) ModSource::Noise]      = rng.nextBipolar();
    sources[0] = 0.0f;

    // ---- matrix ------------------------------------------------------------------------------
    std::fill (destOffset.begin(), destOffset.begin() + s.numDests, 0.0f);
    for (int i = 0; i < s.matrix.numActive; ++i)
    {
        const auto& slot = s.matrix.slots[(size_t) i];
        destOffset[(size_t) slot.dst] += evalModSlot (slot, sources.data());
    }

    // ---- glide & pitch -------------------------------------------------------------------------
    if (glideActive)
    {
        const float step = glideRate * dt;
        if (std::abs (targetNote - currentNote) <= step) { currentNote = targetNote; glideActive = false; }
        else currentNote += currentNote < targetNote ? step : -step;
    }
    driftState = juce::jlimit (-1.0f, 1.0f, 0.998f * driftState + 0.03f * rng.nextBipolar());
    const float pitch = currentNote + s.pitchBend * s.pbRange + mpeBend * s.mpePbRange + s.drift * driftState * 0.15f;
    const float baseHz = noteToHz (s, pitch) * s.tuneRatio;
    const float nyquistInc = 0.5f;
    static const float aaFactor[] = { 2.0f, 1.4f, 1.0f, 0.7f };
    const float aa = aaFactor[juce::jlimit (0, 3, s.aaQuality)];

    // ---- wavetable oscillators -------------------------------------------------------------------
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        const auto& os = s.osc[o];
        auto& w = osc[(size_t) o];
        if (! os.on || os.table == nullptr)
            continue;

        const float semis = (float) (os.octave * 12 + os.semi) + effective (s, d.oscCoarse[o])
                          + (effective (s, d.oscFine[o]) + w.driftCents) / 100.0f;
        const float hz = baseHz * std::exp2 (semis / 12.0f);
        const int N = juce::jlimit (1, kMaxUnison, os.unison);
        const float detune = effective (s, d.oscDetune[o]);
        const float blend = effective (s, d.oscBlend[o]);
        const float width = effective (s, d.oscWidth[o]);
        const float uniWarp = effective (s, d.oscUniWarp[o]);
        const float warpAmt = effective (s, d.oscWarp[o]);
        const float maxCents = std::pow (detune, 1.5f) * 100.0f;

        w.numUni = N;
        float sumSq = 0.0f, maxInc = 0.0f;
        for (int u = 0; u < N; ++u)
        {
            const float sp = N == 1 ? 0.0f : (float) u / (float) (N - 1) * 2.0f - 1.0f;
            float c;
            switch (s.uniCurve)
            {
                case 1:  c = (sp < 0 ? -1.0f : 1.0f) * sp * sp * maxCents; break;
                case 2:  c = (sp < 0 ? -1.0f : 1.0f) * (0.25f * std::abs (sp) + 0.75f * std::abs (sp * sp * sp)) * maxCents * 1.2f; break;
                default: c = sp * maxCents; break;
            }
            w.inc[(size_t) u] = (double) hz * std::exp2 (c / 1200.0f) / sr;
            maxInc = std::max (maxInc, (float) w.inc[(size_t) u]);

            const bool centre = N == 1 || (N % 2 == 1 ? u == N / 2 : (u == N / 2 || u == N / 2 - 1));
            const float g = N == 1 ? 1.0f : (centre ? 1.0f - blend * 0.5f : blend);
            const float pan = sp * width;
            const float ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            w.gainL[(size_t) u] = g * std::cos (ang) * juce::MathConstants<float>::sqrt2;
            w.gainR[(size_t) u] = g * std::sin (ang) * juce::MathConstants<float>::sqrt2;
            w.warp[(size_t) u] = juce::jlimit (0.0f, 1.0f, warpAmt + sp * uniWarp * 0.5f);
            sumSq += g * g;
        }
        const float norm = sumSq > 0.0f ? 1.0f / std::sqrt (sumSq) : 1.0f;
        for (int u = 0; u < N; ++u) { w.gainL[(size_t) u] *= norm; w.gainR[(size_t) u] *= norm; }

        // Sync / FM warps add harmonics above the table's band limit: pick a safer mip
        float warpBoost = 1.0f;
        if (os.warpMode == 1) warpBoost = 1.0f + warpAmt * 15.0f;
        if (os.warpMode == 10) warpBoost = 1.0f + warpAmt * 4.0f;
        const float allowed = nyquistInc / std::max (1.0e-7f, maxInc * warpBoost) * aa;
        w.mip = juce::jlimit (0, Wavetable::kLevels - 1, (int) std::ceil (std::log2 (1024.0f / std::max (1.0f, allowed))));

        const float pos = juce::jlimit (0.0f, 1.0f, effective (s, d.oscPos[o])) * (float) (os.table->numFrames - 1);
        w.frameA = juce::jlimit (0, os.table->numFrames - 1, (int) pos);
        w.frameB = std::min (w.frameA + 1, os.table->numFrames - 1);
        w.frameFrac = pos - (float) w.frameA;
        w.phaseOffset = effective (s, d.oscPhase[o]);

        const float lvl = effective (s, d.oscLevel[o]);
        const float pan = effective (s, d.oscPan[o]);
        const float ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        w.levelL = lvl * std::cos (ang) * juce::MathConstants<float>::sqrt2;
        w.levelR = lvl * std::sin (ang) * juce::MathConstants<float>::sqrt2;
    }

    // ---- OSC D ------------------------------------------------------------------------------
    if (s.oscD.on)
    {
        const float lvl = effective (s, d.dLevel);
        if (s.oscD.mode == 0)
        {
            subInc = (double) baseHz * std::exp2 ((float) s.oscD.subOct) / sr;
            if (s.oscD.subLock) { subGainL = subGainR = lvl; }
            else
            {
                const float ang = (effective (s, d.dPan) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
                subGainL = lvl * std::cos (ang) * juce::MathConstants<float>::sqrt2;
                subGainR = lvl * std::sin (ang) * juce::MathConstants<float>::sqrt2;
            }
        }
        else if (s.oscD.noise != nullptr)
        {
            const float key = s.oscD.noiseKey ? std::exp2 ((pitch - 60.0f) / 12.0f) : 1.0f;
            noiseInc = s.oscD.noise->nativeRate / sr * key * std::exp2 (effective (s, d.dNoisePitch) / 12.0f);
            const float ang = (effective (s, d.dPan) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            subGainL = lvl * std::cos (ang) * juce::MathConstants<float>::sqrt2;
            subGainR = lvl * std::sin (ang) * juce::MathConstants<float>::sqrt2;
        }
    }

    // ---- filters -------------------------------------------------------------------------------
    for (int f = 0; f < kNumFilters; ++f)
    {
        if (! s.filterOn[f])
            continue;
        float cutoff = effective (s, d.fCut[f]);
        if (s.filterKey[f] > 0.0f)
            cutoff *= std::exp2 ((pitch - (float) s.filtKeyRef) / 12.0f * s.filterKey[f]);
        filter[(size_t) f].setParams ((FilterType) s.filterType[f], cutoff, effective (s, d.fRes[f]),
                                      effective (s, d.fDrive[f]), effective (s, d.fFat[f]), s.filtSat);
        filterMix[f] = effective (s, d.fMix[f]);
        const float ang = (effective (s, d.fPan[f]) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        filterPanL[f] = std::cos (ang) * juce::MathConstants<float>::sqrt2;
        filterPanR[f] = std::sin (ang) * juce::MathConstants<float>::sqrt2;
    }

    // ---- velocity -----------------------------------------------------------------------------
    const float curved = std::pow (juce::jlimit (0.0f, 1.0f, velocity), std::pow (3.0f, -s.velCurve));
    const float amt = effective (s, d.ampVel);
    velGain = 1.0f - amt + amt * curved;
}

// ============================================================================
namespace
{
    inline float readCycle (const float* c, double phase, int interp) noexcept
    {
        const float idx = (float) (phase * Wavetable::kSize);
        const int i = std::min ((int) idx, Wavetable::kSize - 1);
        const float f = idx - (float) i;
        switch (interp)
        {
            case 0: return c[i];
            case 1: return c[i] + (c[i + 1] - c[i]) * f;
            default:
            {
                const float xm1 = c[i - 1], x0 = c[i], x1 = c[i + 1], x2 = c[i + 2];
                const float c0 = x0, c1 = 0.5f * (x1 - xm1);
                const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
                const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
                return ((c3 * f + c2) * f + c1) * f + c0;
            }
        }
    }

    inline double wrap01 (double p) noexcept { return p - std::floor (p); }

    inline float polyBlep (double t, double dt) noexcept
    {
        if (t < dt)       { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }
}

void Voice::render (float* L, float* R, int numSamples, const EngineSnapshot& s) noexcept
{
    if (! active)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        if (ctrlCountdown <= 0)
        {
            updateControl (s, i);
            ctrlCountdown = kControlBlock;
            if (env[0].isIdle() && ampStart <= 1.0e-5f)
            {
                active = false;
                note = -1;
                return;
            }
        }
        const float tpos = (float) (kControlBlock - ctrlCountdown + 1) / (float) kControlBlock;
        --ctrlCountdown;
        const float amp = (ampStart + (ampEnd - ampStart) * tpos) * velGain;

        float f1L = 0, f1R = 0, f2L = 0, f2R = 0, dL = 0, dR = 0;
        auto routeAdd = [&] (int route, float l, float r)
        {
            switch (route)
            {
                case 0: f1L += l; f1R += r; break;
                case 1: f2L += l; f2R += r; break;
                case 2:
                    if (s.filterRouting == 1) { f1L += 0.5f * l; f1R += 0.5f * r; f2L += 0.5f * l; f2R += 0.5f * r; }
                    else if (s.filterRouting == 2) { f1L += l; f1R += r; f2L += l; f2R += r; }
                    else { f1L += l; f1R += r; }
                    break;
                default: dL += l; dR += r; break;
            }
        };

        // ---- OSC D first so it can be an FM source this sample ----------------------------
        if (s.oscD.on)
        {
            if (s.oscD.mode == 0)
            {
                const double p = subPhase;
                float v;
                switch (s.oscD.subShape)
                {
                    case 1:  v = (float) (p < 0.5 ? 4.0 * p - 1.0 : 3.0 - 4.0 * p); break;
                    case 2:  v = (p < 0.5 ? 1.0f : -1.0f) + polyBlep (p, subInc) - polyBlep (wrap01 (p + 0.5), subInc); break;
                    case 3:
                    {
                        const float x = (float) p * juce::MathConstants<float>::twoPi;
                        v = 0.62f * (std::sin (x) + 0.5f * std::sin (2.0f * x) + 0.2f * std::sin (3.0f * x) + 0.075f * std::sin (4.0f * x));
                        break;
                    }
                    default: v = std::sin ((float) p * juce::MathConstants<float>::twoPi); break;
                }
                subPhase = wrap01 (subPhase + subInc);
                subLast = v;
                routeAdd (s.oscD.route, v * subGainL, v * subGainR);
            }
            else if (const auto* ns = s.oscD.noise)
            {
                const int len = ns->length;
                const int i0 = (int) noisePos;
                const float fr = (float) (noisePos - i0);
                const float* dd = ns->data.data();
                const float l = dd[i0] + (dd[i0 + 1] - dd[i0]) * fr;
                const int j0 = (i0 + len / 2) & (len - 1);
                const float r = dd[j0] + (dd[j0 + 1] - dd[j0]) * fr;
                noisePos += noiseInc;
                while (noisePos >= len) noisePos -= len;
                noiseLast = l;
                routeAdd (s.oscD.route, l * subGainL, r * subGainR);
            }
        }

        // ---- wavetable oscillators -------------------------------------------------------------
        for (int o = 0; o < kNumWtOscs; ++o)
        {
            const auto& os = s.osc[o];
            if (! os.on || os.table == nullptr)
                continue;
            auto& w = osc[(size_t) o];
            const float* ca = os.table->cycle (w.frameA, w.mip);
            const float* cb = os.table->cycle (w.frameB, w.mip);
            const bool twoFrames = w.frameFrac > 1.0e-4f && w.frameB != w.frameA;
            float srcVal = 0.0f;
            if (os.warpMode >= 10)
            {
                switch (os.warpSrc)
                {
                    case 0: case 1: case 2: srcVal = osc[(size_t) os.warpSrc].last; break;
                    case 3: srcVal = subLast; break;
                    default: srcVal = noiseLast; break;
                }
            }

            float accL = 0.0f, accR = 0.0f, mono = 0.0f;
            for (int u = 0; u < w.numUni; ++u)
            {
                double p = wrap01 (w.phase[(size_t) u] + w.phaseOffset);
                const float wa = w.warp[(size_t) u];
                bool silent = false, mirror = false;
                switch (os.warpMode)
                {
                    case 1: p = wrap01 (p * (1.0 + wa * 15.0)); break;
                    case 2: p = std::pow (p, 1.0 / (1.0 + wa * 3.0)); break;
                    case 3: p = std::pow (p, 1.0 + wa * 3.0); break;
                    case 4: p = p < 0.5 ? 0.5 * std::pow (2.0 * p, 1.0 + wa * 3.0) : 1.0 - 0.5 * std::pow (2.0 * (1.0 - p), 1.0 + wa * 3.0); break;
                    case 5: { const double lim = 1.0 - wa * 0.95; if (p < lim) p /= lim; else silent = true; break; }
                    case 6: case 7:
                    {
                        const double b = os.warpMode == 6 ? 0.5 + wa * 0.45 : 0.5 - wa * 0.45;
                        p = p < b ? p * 0.5 / b : 0.5 + (p - b) * 0.5 / (1.0 - b);
                        break;
                    }
                    case 8: mirror = true; break;
                    case 9: { const double steps = std::exp2 (1.0 + (1.0 - wa) * 9.0); p = std::floor (p * steps) / steps; break; }
                    case 10: p = wrap01 (p + wa * 2.0 * srcVal); break;
                    default: break;
                }

                float v = 0.0f;
                if (! silent)
                {
                    v = readCycle (ca, p, s.interp);
                    if (twoFrames) v += (readCycle (cb, p, s.interp) - v) * w.frameFrac;
                    if (mirror)
                    {
                        const double pm = p < 0.5 ? p * 2.0 : 2.0 - p * 2.0;
                        float vm = readCycle (ca, wrap01 (pm), s.interp);
                        if (twoFrames) vm += (readCycle (cb, wrap01 (pm), s.interp) - vm) * w.frameFrac;
                        v += (vm - v) * wa;
                    }
                }
                else
                    v = ca[0];

                if (os.warpMode == 11)      v *= 1.0f - wa + wa * (0.5f + 0.5f * srcVal);
                else if (os.warpMode == 12) v += (v * srcVal - v) * wa;

                accL += v * w.gainL[(size_t) u];
                accR += v * w.gainR[(size_t) u];
                mono += v;
                w.phase[(size_t) u] = wrap01 (w.phase[(size_t) u] + w.inc[(size_t) u]);
            }
            w.last = mono / (float) w.numUni;
            routeAdd (os.route, accL * w.levelL, accR * w.levelR);
        }

        // ---- filters ----------------------------------------------------------------------
        auto runFilter = [&] (int f, float& l, float& r)
        {
            if (! s.filterOn[f])
                return;
            const float dl = l, dr = r;
            filter[(size_t) f].process (l, r);
            l = (dl + (l - dl) * filterMix[f]) * filterPanL[f];
            r = (dr + (r - dr) * filterMix[f]) * filterPanR[f];
        };

        float outL, outR;
        switch (s.filterRouting)
        {
            case 1:   // parallel
                runFilter (0, f1L, f1R);
                runFilter (1, f2L, f2R);
                outL = f1L + f2L; outR = f1R + f2R;
                break;
            case 2:   // split: filter 1 on the left channel, filter 2 on the right
            {
                float l = f1L + f2L, r = f1R + f2R;
                if (s.filterRouting == 2)
                {
                    l *= 0.5f; r *= 0.5f;
                    float dummyR = 0.0f, dummyL = 0.0f;
                    float l2 = l, r2 = r;
                    if (s.filterOn[0]) { filter[0].process (l2, dummyR); l = l + (l2 - l) * filterMix[0]; }
                    if (s.filterOn[1]) { filter[1].process (dummyL, r2); r = r + (r2 - r) * filterMix[1]; }
                }
                outL = l; outR = r;
                break;
            }
            default:  // serial
                runFilter (0, f1L, f1R);
                f1L += f2L; f1R += f2R;
                runFilter (1, f1L, f1R);
                outL = f1L; outR = f1R;
                break;
        }

        L[i] += (outL + dL) * amp * 0.5f;
        R[i] += (outR + dR) * amp * 0.5f;
    }
    samplesSinceStart += numSamples;
}
} // namespace bgdj
