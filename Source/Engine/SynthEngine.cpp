#include "SynthEngine.h"

namespace bgdj
{
void SynthParamRefs::init (juce::AudioProcessorValueTreeState& s)
{
    auto r = [&s] (const juce::String& id)
    {
        auto* p = s.getRawParameterValue (id);
        jassert (p != nullptr);
        return p;
    };

    for (int o = 0; o < kNumWtOscs; ++o)
    {
        const auto p = params::oscPrefix (o);
        osc[o] = { r (p + "on"), r (p + "table"), r (p + "warp"), r (p + "warp_src"), r (p + "uni"), r (p + "oct"),
                   r (p + "semi"), r (p + "rand"), r (p + "route"), r (p + "phase_reset") };
    }
    dOn = r ("oD_on"); dMode = r ("oD_mode"); dSubShape = r ("oD_sub_shape"); dSubOct = r ("oD_sub_oct");
    dSubLock = r ("oD_sub_lock"); dNoiseType = r ("oD_noise_type"); dNoiseKey = r ("oD_noise_key");
    dRoute = r ("oD_route"); dPhaseReset = r ("oD_phase_reset");
    for (int f = 0; f < kNumFilters; ++f)
    {
        const auto p = params::filterPrefix (f);
        fOn[f] = r (p + "on"); fType[f] = r (p + "type"); fKey[f] = r (p + "key");
    }
    filterRouting = r ("filter_routing");
    for (int l = 0; l < kNumLfos; ++l)
    {
        const auto p = params::lfoPrefix (l);
        lfo[l] = { r (p + "sync"), r (p + "sync_rate"), r (p + "mode"), r (p + "shape"), r (p + "phase"), r (p + "rise"), r (p + "bipolar") };
    }
    for (int e = 0; e < kNumEnvs; ++e)
    {
        const auto p = params::envPrefix (e);
        envCurve[e][0] = r (p + "acurve"); envCurve[e][1] = r (p + "dcurve"); envCurve[e][2] = r (p + "rcurve");
    }
    polyMode = r ("poly_mode"); polyVoices = r ("poly_voices"); portaMode = r ("porta_mode"); portaType = r ("porta_type");
    pbRange = r ("pb_range"); voiceSteal = r ("voice_steal"); envRetrig = r ("env_retrig"); legatoEnvReset = r ("legato_env_reset");
    interp = r ("wt_interp"); aaQuality = r ("aa_quality"); uniCurve = r ("uni_curve"); uniPhase = r ("uni_phase");
    drift = r ("drift"); velCurve = r ("vel_curve"); filtKeyRef = r ("filt_key_ref"); filtSat = r ("filt_sat");
    for (int m = 0; m < kNumMacros; ++m)
        macros[m] = r (params::macroId (m));
    tuneA4 = r ("tune_a4"); seedLock = r ("seed_lock"); seedValue = r ("seed_value");
    mpeOn = r ("mpe_on"); mpeZone = r ("mpe_zone"); mpeMembers = r ("mpe_members"); mpePbRange = r ("mpe_pb_range");

    destRaw.clear();
    destParam.clear();
    for (auto& d : params::modDestinations())
    {
        destRaw.push_back (r (d.paramID));
        destParam.push_back (s.getParameter (d.paramID));
    }
}

// ============================================================================
SynthEngine::SynthEngine()
{
    for (int i = 0; i < 12; ++i)
        builtinLfo[i] = renderCurve (shapes::lfoPreset (i));
}

void SynthEngine::init (juce::AudioProcessorValueTreeState& apvts)
{
    P.init (apvts);
    snap.dest.build();
    snap.numDests = std::min ((int) P.destRaw.size(), kNumModDestsMax);
    jassert ((int) P.destRaw.size() <= kNumModDestsMax);
    for (int i = 0; i < snap.numDests; ++i)
        snap.destParam[i] = P.destParam[(size_t) i];
    initialised = true;
}

void SynthEngine::prepare (double baseSampleRate, int newOsFactor)
{
    baseRate = baseSampleRate;
    osFactor = std::max (1, newOsFactor);
    for (auto& v : voices)
        v.prepare (baseRate * osFactor, baseRate * 16.0);
    reset();
}

void SynthEngine::reset()
{
    for (auto& v : voices)
        v.kill();
    stackSize = 0;
    sustainDown = false;
    snap.pitchBend = 0.0f;
    snap.aftertouch = 0.0f;
    globalOffsets.fill (0.0f);
}

void SynthEngine::allNotesOff()
{
    for (auto& v : voices)
        if (v.isActive())
            v.noteOff();
    stackSize = 0;
}

uint32_t SynthEngine::seedFor (int note)
{
    if (snap.seedLock)
        return (uint32_t) (snap.seedValue * 2654435761u) ^ (uint32_t) (note * 40503u + 17u);
    return rng.next();
}

bool SynthEngine::isMpeMember (int ch) const noexcept
{
    if (P.mpeOn->load() < 0.5f)
        return false;
    const int members = (int) P.mpeMembers->load();
    if ((int) P.mpeZone->load() == 0)  return ch >= 2 && ch <= 1 + members;
    return ch <= 15 && ch >= 15 - members + 1;
}

// ============================================================================
void SynthEngine::beginBlock (const EngineSources& src, double bpm, double ppq, bool playing, int newOs)
{
    auto b = [] (std::atomic<float>* p) { return p->load (std::memory_order_relaxed) >= 0.5f; };
    auto i = [] (std::atomic<float>* p) { return (int) std::lround (p->load (std::memory_order_relaxed)); };
    auto f = [] (std::atomic<float>* p) { return p->load (std::memory_order_relaxed); };

    if (newOs != osFactor)   // no allocation: buffers were sized for 16x in prepare()
    {
        osFactor = std::max (1, newOs);
        for (auto& v : voices)
            v.setSampleRate (baseRate * osFactor);
        stackSize = 0;
    }

    for (int o = 0; o < kNumWtOscs; ++o)
    {
        auto& d = snap.osc[o];
        d.on = b (P.osc[o].on);
        d.warpMode = i (P.osc[o].warp);
        d.warpSrc = i (P.osc[o].warpSrc);
        d.unison = juce::jlimit (1, kMaxUnison, i (P.osc[o].uni));
        d.octave = i (P.osc[o].oct);
        d.semi = i (P.osc[o].semi);
        d.randPhase = f (P.osc[o].rand);
        d.route = i (P.osc[o].route);
        d.phaseReset = b (P.osc[o].phaseReset);
        d.table = src.tables[o];
    }
    auto& od = snap.oscD;
    od.on = b (P.dOn); od.mode = i (P.dMode); od.subShape = i (P.dSubShape); od.subOct = i (P.dSubOct);
    od.subLock = b (P.dSubLock); od.noiseKey = b (P.dNoiseKey); od.route = i (P.dRoute); od.phaseReset = b (P.dPhaseReset);
    od.noiseType = i (P.dNoiseType);
    od.noise = nullptr;
    if (src.noise != nullptr && ! src.noise->samples.empty())
        od.noise = src.noise->samples[(size_t) juce::jlimit (0, (int) src.noise->samples.size() - 1, od.noiseType)].get();

    for (int k = 0; k < kNumFilters; ++k)
    {
        snap.filterOn[k] = b (P.fOn[k]);
        snap.filterType[k] = juce::jlimit (0, (int) FilterType::Count - 1, i (P.fType[k]));
        snap.filterKey[k] = f (P.fKey[k]);
    }
    snap.filterRouting = i (P.filterRouting);

    for (int l = 0; l < kNumLfos; ++l)
    {
        auto& d = snap.lfo[l];
        d.sync = b (P.lfo[l].sync); d.syncRate = i (P.lfo[l].syncRate); d.mode = i (P.lfo[l].mode);
        d.shape = i (P.lfo[l].shape); d.phase = f (P.lfo[l].phase); d.rise = f (P.lfo[l].rise); d.bipolar = b (P.lfo[l].bipolar);
        d.table = d.shape == 11 ? src.lfoTables[l] : &builtinLfo[juce::jlimit (0, 11, d.shape)];
        if (d.table == nullptr) d.table = &builtinLfo[0];
    }
    for (int e = 0; e < kNumEnvs; ++e)
        for (int c = 0; c < 3; ++c)
            snap.envCurve[e][c] = f (P.envCurve[e][c]);

    for (int d = 0; d < snap.numDests; ++d)
    {
        const float real = f (P.destRaw[(size_t) d]);
        snap.baseReal[d] = real;
        snap.baseNorm[d] = snap.destParam[d]->convertTo0to1 (real);
    }

    snap.polyMode = i (P.polyMode);
    snap.polyVoices = juce::jlimit (1, kMaxVoices, i (P.polyVoices));
    snap.portaMode = i (P.portaMode);
    snap.portaType = i (P.portaType);
    snap.voiceSteal = i (P.voiceSteal);
    snap.envRetrig = i (P.envRetrig);
    snap.legatoEnvReset = b (P.legatoEnvReset);
    snap.pbRange = f (P.pbRange);
    snap.mpePbRange = b (P.mpeOn) ? f (P.mpePbRange) : 0.0f;
    snap.interp = i (P.interp);
    snap.aaQuality = i (P.aaQuality);
    snap.uniCurve = i (P.uniCurve);
    snap.uniPhase = i (P.uniPhase);
    snap.drift = f (P.drift);
    snap.velCurve = f (P.velCurve);
    snap.filtKeyRef = i (P.filtKeyRef);
    snap.filtSat = (SatMode) juce::jlimit (0, 3, i (P.filtSat));
    for (int m = 0; m < kNumMacros; ++m)
        snap.macros[m] = f (P.macros[m]);
    snap.bpm = bpm > 1.0 ? bpm : 120.0;
    snap.playing = playing;
    blockPpq = ppq;
    snap.ppqAtBlockStart = ppq;
    snap.osFactor = osFactor;
    snap.sampleRate = baseRate * osFactor;
    snap.seedLock = b (P.seedLock);
    snap.seedValue = i (P.seedValue);
    snap.tuneRatio = f (P.tuneA4) / 440.0f;
    snap.tuning = src.tuning;
    if (src.matrix != nullptr)
        src.matrix->takeSnapshot (snap.matrix);
    else
        snap.matrix.numActive = 0;

    // keep only valid destinations
    int w = 0;
    for (int k = 0; k < snap.matrix.numActive; ++k)
        if (snap.matrix.slots[(size_t) k].dst < snap.numDests)
            snap.matrix.slots[(size_t) w++] = snap.matrix.slots[(size_t) k];
    snap.matrix.numActive = w;
}

// ============================================================================
Voice* SynthEngine::newestActiveVoice()
{
    Voice* best = nullptr;
    for (auto& v : voices)
        if (v.isActive() && (best == nullptr || v.age > best->age))
            best = &v;
    return best;
}

int SynthEngine::findVoiceToSteal()
{
    int best = 0;
    for (int k = 1; k < snap.polyVoices; ++k)
    {
        const auto& a = voices[(size_t) k];
        const auto& c = voices[(size_t) best];
        bool better = false;
        switch (snap.voiceSteal)
        {
            case 1:  better = a.ampLevel() < c.ampLevel(); break;
            case 2:  better = a.note < c.note; break;     // "Lowest Note": steal the lowest
            case 3:  better = a.note > c.note; break;
            default: better = a.age < c.age; break;
        }
        // always prefer releasing voices
        if (! a.keyHeld && c.keyHeld) better = true;
        if (a.keyHeld && ! c.keyHeld) better = false;
        if (better)
            best = k;
    }
    return best;
}

void SynthEngine::noteOn (int ch, int note, float vel)
{
    ++stats.noteOns;
    stats.secondsSinceNoteOn = 0.0;
    const float portaMs = snap.baseReal[snap.dest.porta];
    const bool anyHeld = stackSize > 0;
    const bool mpe = isMpeMember (ch);

    if (snap.polyMode == 2 || mpe)   // ---- Poly (MPE always behaves polyphonically)
    {
        const float glideFrom = (portaMs > 0.5f && lastPlayedNote >= 0 && (snap.portaMode == 0 || anyHeld)) ? (float) lastPlayedNote : -1.0f;
        int idx = -1;
        for (int k = 0; k < snap.polyVoices; ++k)
            if (voices[(size_t) k].isActive() && voices[(size_t) k].note == note && voices[(size_t) k].channel == ch && ! voices[(size_t) k].keyHeld)
                { idx = k; break; }
        if (idx < 0)
            for (int k = 0; k < snap.polyVoices; ++k)
                if (! voices[(size_t) k].isActive()) { idx = k; break; }
        bool stolen = false;
        if (idx < 0) { idx = findVoiceToSteal(); stolen = true; }

        voices[(size_t) idx].noteOn (note, vel, ch, false, glideFrom, snap, seedFor (note),
                                     stolen || snap.envRetrig == 1, ++ageCounter);
    }
    else                              // ---- Mono / Legato
    {
        auto& v = voices[0];
        const bool overlap = v.isActive() && v.keyHeld && anyHeld;
        if (overlap)
        {
            const bool legato = snap.polyMode == 1;
            v.retarget (note, vel, snap, legato ? snap.legatoEnvReset : true, portaMs > 0.5f, (float) lastPlayedNote);
            v.channel = ch;
            v.age = ++ageCounter;
        }
        else
        {
            const float glideFrom = (portaMs > 0.5f && snap.portaMode == 0 && lastPlayedNote >= 0) ? (float) lastPlayedNote : -1.0f;
            v.noteOn (note, vel, ch, false, glideFrom, snap, seedFor (note), true, ++ageCounter);
        }
        for (int k = 1; k < kMaxVoices; ++k)
            if (voices[(size_t) k].isActive()) voices[(size_t) k].noteOff();
    }

    // note stack (for mono fallback and stats)
    for (int k = 0; k < stackSize; ++k)
        if (noteStack[(size_t) k] == note)
        {
            std::move (noteStack.begin() + k + 1, noteStack.begin() + stackSize, noteStack.begin() + k);
            --stackSize;
            break;
        }
    if (stackSize < 128)
        noteStack[(size_t) stackSize++] = note;
    lastPlayedNote = note;
}

void SynthEngine::noteOff (int ch, int note)
{
    for (int k = 0; k < stackSize; ++k)
        if (noteStack[(size_t) k] == note)
        {
            std::move (noteStack.begin() + k + 1, noteStack.begin() + stackSize, noteStack.begin() + k);
            --stackSize;
            break;
        }

    if (snap.polyMode == 2 || isMpeMember (ch))
    {
        for (auto& v : voices)
            if (v.isActive() && v.keyHeld && v.note == note && v.channel == ch)
            {
                if (sustainDown) { v.keyHeld = false; v.sustained = true; }
                else v.noteOff();
            }
        return;
    }

    auto& v = voices[0];
    if (! v.isActive() || v.note != note)
        return;
    if (stackSize > 0)
    {
        const int back = noteStack[(size_t) stackSize - 1];
        const float portaMs = snap.baseReal[snap.dest.porta];
        v.retarget (back, v.velocity, snap, snap.polyMode == 0, portaMs > 0.5f, -1.0f);
        lastPlayedNote = back;
    }
    else if (sustainDown)
    {
        v.keyHeld = false;
        v.sustained = true;
    }
    else
        v.noteOff();
}

void SynthEngine::setSustain (bool down)
{
    sustainDown = down;
    if (! down)
        for (auto& v : voices)
            if (v.isActive() && v.sustained)
            {
                v.sustained = false;
                v.noteOff();
            }
}

void SynthEngine::handleMidi (const juce::MidiMessage& m)
{
    const int ch = m.getChannel();
    if (m.isNoteOn())
        noteOn (ch, m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        noteOff (ch, m.getNoteNumber());
    else if (m.isPitchWheel())
    {
        const float v = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        if (isMpeMember (ch))
        {
            for (auto& vo : voices)
                if (vo.isActive() && vo.channel == ch) vo.mpeBend = v;
        }
        else
            snap.pitchBend = v;
    }
    else if (m.isChannelPressure())
    {
        const float v = (float) m.getChannelPressureValue() / 127.0f;
        if (isMpeMember (ch))
        {
            for (auto& vo : voices)
                if (vo.isActive() && vo.channel == ch) vo.mpePressure = v;
        }
        else
            snap.aftertouch = v;
    }
    else if (m.isAftertouch())
    {
        for (auto& vo : voices)
            if (vo.isActive() && vo.note == m.getNoteNumber()) vo.polyAftertouch = (float) m.getAfterTouchValue() / 127.0f;
    }
    else if (m.isController())
    {
        const int cc = m.getControllerNumber();
        const float v = (float) m.getControllerValue() / 127.0f;
        if (cc == 1) snap.modWheel = v;
        else if (cc == 64) setSustain (v >= 0.5f);
        else if (cc == 74 && isMpeMember (ch))
        {
            for (auto& vo : voices)
                if (vo.isActive() && vo.channel == ch) vo.mpeSlide = v;
        }
        else if (cc == 120)
        {
            for (auto& vo : voices) vo.kill();
            stackSize = 0;
        }
        else if (cc == 123)
            allNotesOff();
    }
    else if (m.isAllNotesOff())
        allNotesOff();
}

void SynthEngine::injectNoteOn (int note, float vel)
{
    noteOn (1, note, vel);
}

void SynthEngine::renderSegment (float* L, float* R, int startBase, int endBase)
{
    if (endBase <= startBase)
        return;
    snap.ppqAtBlockStart = blockPpq + (double) startBase / baseRate * snap.bpm / 60.0;
    const int s0 = startBase * osFactor;
    const int n = (endBase - startBase) * osFactor;
    for (auto& v : voices)
        if (v.isActive())
            v.render (L + s0, R + s0, n, snap);
}

void SynthEngine::render (float* osL, float* osR, int numBase, const juce::MidiBuffer& midi)
{
    stats.noteOns = 0;
    const float bendBefore = snap.pitchBend;

    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (0, numBase, meta.samplePosition);
        renderSegment (osL, osR, pos, evPos);
        handleMidi (meta.getMessage());
        pos = evPos;
    }
    renderSegment (osL, osR, pos, numBase);

    // ---- stats -------------------------------------------------------------------
    const double blockSec = numBase / baseRate;
    stats.secondsSinceNoteOn += blockSec;
    stats.activeVoices = 0;
    stats.glideActive = false;
    for (auto& v : voices)
        if (v.isActive())
        {
            ++stats.activeVoices;
            stats.glideActive = stats.glideActive || v.glideActive;
        }
    stats.heldCount = stackSize;
    stats.lowestHeld = stats.highestHeld = -1;
    for (int k = 0; k < stackSize; ++k)
    {
        const int nn = noteStack[(size_t) k];
        if (stats.lowestHeld < 0 || nn < stats.lowestHeld) stats.lowestHeld = nn;
        if (nn > stats.highestHeld) stats.highestHeld = nn;
    }
    stats.bendMovement = std::abs (snap.pitchBend - bendBefore) + std::abs (snap.pitchBend - lastBend) * 0.0f;
    lastBend = snap.pitchBend;
    const int curNote = stackSize > 0 ? noteStack[(size_t) stackSize - 1] : -1;
    if (curNote >= 0 && curNote == stats.lastNote && stats.noteOns == 0)
        stats.sameNoteSeconds += blockSec;
    else if (stats.noteOns > 0 || curNote < 0)
        stats.sameNoteSeconds = 0.0;
    stats.lastNote = curNote;

    // ---- global modulation from the newest voice -------------------------------------
    if (auto* v = newestActiveVoice())
    {
        std::copy (v->destOffset.begin(), v->destOffset.begin() + snap.numDests, globalOffsets.begin());
        for (int l = 0; l < kNumLfos; ++l)
        {
            uiLfoPhase[l].store (v->lfoPhase (l), std::memory_order_relaxed);
            uiLfoValue[l].store (v->sources[(size_t) ModSource::Lfo1 + (size_t) l], std::memory_order_relaxed);
        }
        for (int e = 0; e < kNumEnvs; ++e)
            uiEnvLevel[e].store (v->envLevel (e), std::memory_order_relaxed);
    }
    else
    {
        // no voice: only performance sources (macros, wheel, bend, aftertouch) modulate globals
        std::array<float, (size_t) ModSource::Count> src {};
        for (int m = 0; m < kNumMacros; ++m) src[(size_t) ModSource::Macro1 + (size_t) m] = snap.macros[m];
        src[(size_t) ModSource::ModWheel] = snap.modWheel;
        src[(size_t) ModSource::PitchBend] = snap.pitchBend;
        src[(size_t) ModSource::Aftertouch] = snap.aftertouch;
        std::fill (globalOffsets.begin(), globalOffsets.begin() + snap.numDests, 0.0f);
        for (int k = 0; k < snap.matrix.numActive; ++k)
        {
            const auto& slot = snap.matrix.slots[(size_t) k];
            globalOffsets[(size_t) slot.dst] += evalModSlot (slot, src.data());
        }
        for (int e = 0; e < kNumEnvs; ++e)
            uiEnvLevel[e].store (0.0f, std::memory_order_relaxed);
    }
    uiActiveVoices.store (stats.activeVoices, std::memory_order_relaxed);
}
} // namespace bgdj
