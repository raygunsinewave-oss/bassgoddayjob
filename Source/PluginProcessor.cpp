#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets/PresetManager.h"

namespace bgdj
{
BassGodsProcessor::BassGodsProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "BGDJ_PARAMS", params::createLayout())
{
    for (auto& c : ccMap) c.store (-1);
    paramList = getParameters();

    engine.init (apvts);
    fxRack.init (apvts);

    pMasterVol = apvts.getRawParameterValue ("master_vol");
    pBassMonoOn = apvts.getRawParameterValue ("bassmono_on");
    pBassMonoFreq = apvts.getRawParameterValue ("bassmono_freq");
    pOsSynth = apvts.getRawParameterValue ("os_synth");
    pOsFx = apvts.getRawParameterValue ("os_fx");
    pOsOffline = apvts.getRawParameterValue ("os_offline");
    pDenormal = apvts.getRawParameterValue ("denormal_ftz");
    pDc = apvts.getRawParameterValue ("dc_blocker");
    pSfxOut = apvts.getRawParameterValue ("crowd_sfx_out");
    pSfxLevel = apvts.getRawParameterValue ("crowd_sfx_level");
    for (int o = 0; o < kNumWtOscs; ++o)
        pTable[o] = apvts.getRawParameterValue (params::oscPrefix (o) + "table");
    masterDest = params::modDestIndexForParam ("master_vol");

    for (int l = 0; l < kNumLfos; ++l)
        setLfoPoints (l, shapes::lfoPreset (11));
    setDuckPoints (shapes::duckPreset (0));
    tuning.publish (std::make_shared<TuningTable>());

    NoiseBank::getInstance().rescan (settings->noiseFolders());
    ensureTables (true);

    presets = std::make_unique<PresetManager> (*this);
    midiMapFromVar (juce::JSON::parse (settings->getString ("midiMap")));
    analyzersEnabled = settings->analyzersEnabled();

    hostName = juce::PluginHostType().getHostDescription();
    AppSettings::log ("Plugin instance created in host: " + hostName + " (" + juce::String (getWrapperTypeDescription (wrapperType)) + ")");
    startTimerHz (20);
}

BassGodsProcessor::~BassGodsProcessor()
{
    stopTimer();
}

juce::String BassGodsProcessor::getHostDescription() const { return hostName; }

// ============================================================================
bool BassGodsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    if (layouts.inputBuses.size() > 0)
    {
        const auto sc = layouts.getChannelSet (true, 0);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono())
            return false;
    }
    return true;
}

void BassGodsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    maxBlock = std::max (16, samplesPerBlock);
    sampleRateForUi = (float) sampleRate;

    for (int e = 0; e < kMaxOversampleExp; ++e)
    {
        synthOversamplers[(size_t) e] = std::make_unique<juce::dsp::Oversampling<float>> (
            2, e + 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        synthOversamplers[(size_t) e]->initProcessing ((size_t) maxBlock);
    }

    engine.prepare (sampleRate, 1);
    fxRack.prepare (sampleRate, maxBlock);
    loudness.prepare (sampleRate);
    features.prepare (sampleRate);
    crowdSfx.prepare (sampleRate);
    bassMono.reset();
    dcBlock.prepare (sampleRate);
    dcBlock.reset();
    preFx.setSize (2, maxBlock, false, true, false);
    workBuffer.setSize (2, maxBlock, false, true, false);
    chunkMidi.ensureSize (8192);
    loadMeasurer.reset (sampleRate, maxBlock);
    masterGainSmooth = juce::Decibels::decibelsToGain (pMasterVol->load(), -60.0f);
    ensureTables (false);
    for (auto& t : oscTables) t.table.forceAcknowledge();
    AppSettings::log ("prepareToPlay: " + juce::String (sampleRate) + " Hz, block " + juce::String (samplesPerBlock));
}

void BassGodsProcessor::releaseResources()
{
    engine.reset();
    fxRack.reset();
}

// ============================================================================
void BassGodsProcessor::handleMidiLearn (const juce::MidiBuffer& midi)
{
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (! m.isController())
            continue;
        const int cc = m.getControllerNumber();
        const int target = learnTarget.load();
        if (target >= 0 && cc != 64 && cc != 120 && cc != 123)
        {
            for (auto& c : ccMap)
                if (c.load() == target) c.store (-1);
            ccMap[(size_t) cc].store (target);
            learnTarget.store (-1);
            ++midiMapVersion;
        }
        const int pi = ccMap[(size_t) cc].load();
        if (pi >= 0 && pi < paramList.size())
            paramList[pi]->setValueNotifyingHost ((float) m.getControllerValue() / 127.0f);
    }
}

void BassGodsProcessor::renderSynth (float* L, float* R, int n, const juce::MidiBuffer& midi, int osIdx)
{
    if (osIdx <= 0)
    {
        juce::FloatVectorOperations::clear (L, n);
        juce::FloatVectorOperations::clear (R, n);
        engine.render (L, R, n, midi);
        return;
    }
    auto& os = *synthOversamplers[(size_t) juce::jlimit (0, kMaxOversampleExp - 1, osIdx - 1)];
    float* chans[2] = { L, R };
    juce::FloatVectorOperations::clear (L, n);
    juce::FloatVectorOperations::clear (R, n);
    juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
    auto up = os.processSamplesUp (block);
    up.clear();
    engine.render (up.getChannelPointer (0), up.getChannelPointer (1), n, midi);
    os.processSamplesDown (block);
}

void BassGodsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::AudioProcessLoadMeasurer::ScopedTimer loadTimer (loadMeasurer, buffer.getNumSamples());
    juce::ScopedNoDenormals sn;   // always flush; the toggle additionally clamps tiny values below
    const bool ftz = pDenormal->load() >= 0.5f;

    const int total = buffer.getNumSamples();
    lastBlockSize = total;
    auto out = getBusBuffer (buffer, false, 0);
    const bool scOn = getBusCount (true) > 0 && getBus (true, 0) != nullptr && getBus (true, 0)->isEnabled();
    const float* scL = nullptr; const float* scR = nullptr;
    if (scOn)
    {
        auto sc = getBusBuffer (buffer, true, 0);
        if (sc.getNumChannels() > 0) { scL = sc.getReadPointer (0); scR = sc.getReadPointer (std::min (1, sc.getNumChannels() - 1)); }
    }

    keyboardState.processNextMidiBuffer (midi, 0, total, true);
    handleMidiLearn (midi);

    // ---- host transport ---------------------------------------------------------------------
    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto p = pos->getPpqPosition()) ppq = *p;
            playing = pos->getIsPlaying();
        }
    uiBpm = bpm; uiPpq = ppq; uiPlaying = playing;

    int osIdx = juce::jlimit (0, 4, (int) pOsSynth->load());
    int fxOsIdx = juce::jlimit (0, 4, (int) pOsFx->load());
    if (isNonRealtime() && (int) pOsOffline->load() > 0)
        osIdx = fxOsIdx = juce::jlimit (0, 4, (int) pOsOffline->load());

    // ---- sources for the engine ----------------------------------------------------------------
    EngineSources src;
    for (int o = 0; o < kNumWtOscs; ++o)
        src.tables[o] = oscTables[(size_t) o].table.get();
    for (int l = 0; l < kNumLfos; ++l)
        src.lfoTables[l] = lfoTables[(size_t) l].get();
    src.noise = NoiseBank::getInstance().getForAudio();
    src.tuning = tuning.get();
    src.matrix = &modMatrix;

    const bool monoOut = out.getNumChannels() < 2;
    float* L = out.getWritePointer (0);

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = std::min (maxBlock, total - start);
        const double chunkPpq = ppq + (double) start / currentSampleRate * bpm / 60.0;

        chunkMidi.clear();
        chunkMidi.addEvents (midi, start, n, -start);

        float* cl = L + start;
        float* cr = monoOut ? workBuffer.getWritePointer (1) : out.getWritePointer (1) + start;

        engine.beginBlock (src, bpm, chunkPpq, playing, 1 << osIdx);
        renderSynth (cl, cr, n, chunkMidi, osIdx);

        // ---- bass mono below + DC blocker -------------------------------------------------------
        if (pBassMonoOn->load() >= 0.5f)
        {
            bassMono.set (currentSampleRate, pBassMonoFreq->load());
            for (int i = 0; i < n; ++i)
            {
                float lo1, hi1, lo2, hi2;
                bassMono.split (cl[i], 0, lo1, hi1);
                bassMono.split (cr[i], 1, lo2, hi2);
                const float m = 0.5f * (lo1 + lo2);
                cl[i] = m + hi1; cr[i] = m + hi2;
            }
        }
        if (pDc->load() >= 0.5f)
            for (int i = 0; i < n; ++i) { cl[i] = dcBlock.process (cl[i], 0); cr[i] = dcBlock.process (cr[i], 1); }

        // ---- master volume (modulatable), before FX so the final limiter catches it ---------------
        float masterDb = pMasterVol->load();
        const auto& offs = engine.getGlobalOffsets();
        if (masterDest >= 0 && offs[(size_t) masterDest] != 0.0f)
        {
            auto* p = apvts.getParameter ("master_vol");
            masterDb = p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, p->convertTo0to1 (masterDb) + offs[(size_t) masterDest]));
        }
        const float target = masterDb <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (masterDb);
        for (int i = 0; i < n; ++i)
        {
            masterGainSmooth += (target - masterGainSmooth) * 0.002f;
            cl[i] *= masterGainSmooth; cr[i] *= masterGainSmooth;
        }

        std::copy (cl, cl + n, preFx.getWritePointer (0));
        std::copy (cr, cr + n, preFx.getWritePointer (1));

        // ---- crowd SFX (opt-in, never printed by default) -------------------------------------------
        if (pSfxOut->load() >= 0.5f)
            crowdSfx.render (cl, cr, n, juce::Decibels::decibelsToGain (pSfxLevel->load()));

        // ---- FX rack ----------------------------------------------------------------------------------
        FxContext ctx;
        ctx.bpm = bpm; ctx.ppq = chunkPpq; ctx.playing = playing;
        ctx.modOffsets = offs.data();
        ctx.scL = scL ? scL + start : nullptr;
        ctx.scR = scR ? scR + start : nullptr;
        juce::AudioBuffer<float> view (std::array<float*, 2> { cl, cr }.data(), 2, n);
        fxRack.process (view, n, ctx, fxOsIdx);

        if (ftz)
            for (int i = 0; i < n; ++i)
            {
                if (std::abs (cl[i]) < 1.0e-20f) cl[i] = 0.0f;
                if (std::abs (cr[i]) < 1.0e-20f) cr[i] = 0.0f;
            }

        // ---- analysis -------------------------------------------------------------------------------------
        if (analyzersEnabled.load())
        {
            loudness.process (cl, cr, n);
            if (editorOpen.load())
            {
                preFxFifo.push (preFx.getReadPointer (0), preFx.getReadPointer (1), n);
                postFxFifo.push (cl, cr, n);
            }
        }

        auto f = features.analyse (preFx.getReadPointer (0), preFx.getReadPointer (1), cl, cr, n);
        const auto& st = engine.getStats();
        f.noteOns = st.noteOns; f.lowestNote = st.lowestHeld; f.highestNote = st.highestHeld;
        f.heldNotes = st.heldCount; f.activeVoices = st.activeVoices; f.bendMovement = st.bendMovement;
        f.glide = st.glideActive; f.sameNoteSeconds = st.sameNoteSeconds; f.secondsSinceNoteOn = st.secondsSinceNoteOn;
        f.bpm = bpm; f.ppq = chunkPpq; f.hostPlaying = playing;
        const auto& snap = engine.getSnapshot();
        float mm = 0.0f;
        for (int m = 0; m < kNumMacros; ++m) { mm += std::abs (snap.macros[m] - lastMacros[m]); lastMacros[m] = snap.macros[m]; }
        f.macroMovement = mm;
        if (auto* lim = fxRack.getModule (FxType::Limiter); lim != nullptr && lim->on.on() && lim->meterGainReduction.load() < -8.0f)
            f.clipFraction = std::max (f.clipFraction, 0.006f);   // smashing the limiter counts as clipping
        featureFifo.push (f);

        if (monoOut)
            for (int i = 0; i < n; ++i) cl[i] = 0.5f * (cl[i] + cr[i]);
    }

    for (int c = out.getNumChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, total);

    for (auto& t : oscTables) t.table.audioThreadAcknowledge();
    for (auto& t : lfoTables) t.audioThreadAcknowledge();
    tuning.audioThreadAcknowledge();
    NoiseBank::getInstance().audioAcknowledge();
}

// ============================================================================
void BassGodsProcessor::timerCallback()
{
    ensureTables (false);
    for (auto& t : oscTables) t.table.collectGarbage();
    for (auto& t : lfoTables) t.collectGarbage();
    tuning.collectGarbage();
    fxRack.ducker().customShape.collectGarbage();
    NoiseBank::getInstance().collectGarbage();
}

void BassGodsProcessor::ensureTables (bool force)
{
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        auto& t = oscTables[(size_t) o];
        if (t.isUser)
            continue;
        const int idx = juce::jlimit (0, factory::numTables() - 1, (int) pTable[o]->load());
        if (force || idx != t.loadedFactory)
        {
            t.table.publish (WavetableLibrary::getInstance().getFactory (idx));
            t.loadedFactory = idx;
            ++tableVersion;
        }
    }
}

void BassGodsProcessor::setFactoryTable (int osc, int index)
{
    auto& t = oscTables[(size_t) osc];
    t.isUser = false;
    t.userName = {};
    t.loadedFactory = -1;
    if (auto* p = apvts.getParameter (params::oscPrefix (osc) + "table"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, factory::numTables() - 1, index)));
    ensureTables (false);
}

void BassGodsProcessor::setUserTable (int osc, std::shared_ptr<const Wavetable> table)
{
    if (table == nullptr)
        return;
    auto& t = oscTables[(size_t) osc];
    t.isUser = true;
    t.userName = table->name;
    t.table.publish (std::move (table));
    ++tableVersion;
}

juce::String BassGodsProcessor::getTableName (int osc) const
{
    const auto& t = oscTables[(size_t) osc];
    if (t.isUser) return t.userName + " (user)";
    return factory::tableNames()[juce::jlimit (0, factory::numTables() - 1, (int) pTable[osc]->load())];
}

void BassGodsProcessor::setLfoPoints (int lfo, const CurvePoints& pts)
{
    lfoPoints[(size_t) lfo] = pts;
    lfoTables[(size_t) lfo].publish (std::make_shared<ShapeTable> (renderCurve (pts)));
    ++shapeVersion;
}

void BassGodsProcessor::setDuckPoints (const CurvePoints& pts)
{
    duckPoints = pts;
    fxRack.ducker().customShape.publish (std::make_shared<ShapeTable> (renderCurve (pts)));
    ++shapeVersion;
}

bool BassGodsProcessor::loadTuningFile (const juce::File& f, juce::String& error)
{
    auto t = std::make_shared<TuningTable>();
    if (! t->loadFile (f, error))
        return false;
    tuning.publish (t);
    return true;
}

void BassGodsProcessor::resetTuning()      { tuning.publish (std::make_shared<TuningTable>()); }
juce::String BassGodsProcessor::getTuningName() const { auto t = tuning.getShared(); return t ? t->name : juce::String ("12-TET"); }

// ============================================================================
// MIDI learn
void BassGodsProcessor::startMidiLearn (const juce::String& paramID)
{
    for (int i = 0; i < paramList.size(); ++i)
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (paramList[i]); rp != nullptr && rp->paramID == paramID)
            learnTarget.store (i);
}

void BassGodsProcessor::clearMidiMapping (const juce::String& paramID)
{
    for (auto& c : ccMap)
        if (const int i = c.load(); i >= 0)
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (paramList[i]); rp != nullptr && rp->paramID == paramID)
                c.store (-1);
    ++midiMapVersion;
}

int BassGodsProcessor::getCcForParam (const juce::String& paramID) const
{
    for (int cc = 0; cc < 128; ++cc)
        if (const int i = ccMap[(size_t) cc].load(); i >= 0)
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (paramList[i]); rp != nullptr && rp->paramID == paramID)
                return cc;
    return -1;
}

juce::var BassGodsProcessor::midiMapToVar() const
{
    auto* o = new juce::DynamicObject();
    for (int cc = 0; cc < 128; ++cc)
        if (const int i = ccMap[(size_t) cc].load(); i >= 0)
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (paramList[i]))
                o->setProperty (juce::String (cc), rp->paramID);
    return juce::var (o);
}

void BassGodsProcessor::midiMapFromVar (const juce::var& v)
{
    for (auto& c : ccMap) c.store (-1);
    if (auto* o = v.getDynamicObject())
        for (auto& prop : o->getProperties())
        {
            const int cc = prop.name.toString().getIntValue();
            if (cc < 0 || cc > 127) continue;
            for (int i = 0; i < paramList.size(); ++i)
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (paramList[i]); rp != nullptr && rp->paramID == prop.value.toString())
                    ccMap[(size_t) cc].store (i);
        }
    ++midiMapVersion;
}

void BassGodsProcessor::applySafeMode (bool on)
{
    settings->set ("safeMode", on);
    analyzersEnabled = settings->analyzersEnabled();
    if (on)
        for (auto id : { "os_synth", "os_fx", "os_offline" })
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (0.0f);
}

// ============================================================================
// Presets
void BassGodsProcessor::applyPreset (const PresetData& p)
{
    // everything not mentioned by the preset returns to its default
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto& id = rp->paramID;
            if (id == "master_vol" || id.startsWith ("os_") || id == "denormal_ftz" || id.startsWith ("mpe_")
                || id == "crowd_sfx_out" || id == "crowd_sfx_level" || id == "tune_a4" || id == "cat_chaos")
                continue;
            rp->setValueNotifyingHost (rp->getDefaultValue());
        }

    for (auto& nv : p.params)
        if (auto* rp = apvts.getParameter (nv.name.toString()))
        {
            float real;
            if (presetformat::resolveValue (*rp, nv.value, real))
                rp->setValueNotifyingHost (rp->convertTo0to1 (real));
        }

    for (int o = 0; o < kNumWtOscs; ++o)
    {
        if (p.userTables[o].isNotEmpty())
        {
            if (auto wt = Wavetable::fromBase64 (p.tables[o].isNotEmpty() ? p.tables[o] : juce::String ("User"), p.userTables[o]))
                { setUserTable (o, wt); continue; }
        }
        oscTables[(size_t) o].isUser = false;
        const int idx = factory::indexOf (p.tables[o]);
        if (idx >= 0)
            setFactoryTable (o, idx);
    }
    ensureTables (false);

    modMatrix.fromVar (p.matrix);
    for (int l = 0; l < kNumLfos; ++l)
    {
        const auto v = p.lfoShapes[juce::Identifier (juce::String (l + 1))];
        setLfoPoints (l, v.isArray() ? shapes::pointsFromVar (v) : shapes::lfoPreset (11));
    }
    setDuckPoints (p.duckShape.isArray() ? shapes::pointsFromVar (p.duckShape) : shapes::duckPreset (0));
    fxRack.orderFromVar (p.fxOrder);
    macroLabels = p.macroLabels;

    if (p.tuningType.isNotEmpty())
    {
        auto t = std::make_shared<TuningTable>();
        t->restore (p.tuningType, p.tuningText);
        tuning.publish (t);
    }

    currentPresetName = p.name;
    currentPresetCharacter = p.character + " " + p.genre + " " + p.bank;
    ++presetVersion;
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

PresetData BassGodsProcessor::capturePreset (const juce::String& name) const
{
    PresetData p;
    p.name = name;
    p.macroLabels = macroLabels;
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto real = rp->convertFrom0to1 (rp->getValue());
            if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (rp))
                p.params.set (rp->paramID, ch->getCurrentChoiceName());
            else if (dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr)
                p.params.set (rp->paramID, real >= 0.5f);
            else
                p.params.set (rp->paramID, (double) real);
        }
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        const auto& t = oscTables[(size_t) o];
        if (t.isUser)
        {
            if (auto wt = t.table.getShared())
            {
                p.tables[o] = t.userName;
                p.userTables[o] = wt->toBase64();
            }
        }
        else
            p.tables[o] = factory::tableNames()[juce::jlimit (0, factory::numTables() - 1, (int) pTable[o]->load())];
    }
    p.matrix = modMatrix.toVar();
    auto* shapesObj = new juce::DynamicObject();
    for (int l = 0; l < kNumLfos; ++l)
        shapesObj->setProperty (juce::String (l + 1), shapes::pointsToVar (lfoPoints[(size_t) l]));
    p.lfoShapes = juce::var (shapesObj);
    p.duckShape = shapes::pointsToVar (duckPoints);
    p.fxOrder = fxRack.orderToVar();
    if (auto t = tuning.getShared(); t != nullptr && t->sourceType.isNotEmpty())
    {
        p.tuningType = t->sourceType;
        p.tuningText = t->sourceText;
    }
    return p;
}

// ============================================================================
// State
juce::ValueTree BassGodsProcessor::saveExtras() const
{
    juce::ValueTree x ("BGDJ_EXTRA");
    const auto preset = capturePreset (currentPresetName);
    x.setProperty ("presetJson", presetformat::serialise (preset), nullptr);
    x.setProperty ("presetCharacter", currentPresetCharacter, nullptr);
    return x;
}

void BassGodsProcessor::restoreExtras (const juce::ValueTree& x)
{
    PresetData p;
    juce::String err;
    if (! presetformat::parse (x["presetJson"].toString(), p, err))
        return;

    for (int o = 0; o < kNumWtOscs; ++o)
    {
        if (p.userTables[o].isNotEmpty())
        {
            if (auto wt = Wavetable::fromBase64 (p.tables[o], p.userTables[o]))
                setUserTable (o, wt);
        }
        else
            oscTables[(size_t) o].isUser = false;
    }
    modMatrix.fromVar (p.matrix);
    for (int l = 0; l < kNumLfos; ++l)
    {
        const auto v = p.lfoShapes[juce::Identifier (juce::String (l + 1))];
        setLfoPoints (l, v.isArray() ? shapes::pointsFromVar (v) : shapes::lfoPreset (11));
    }
    setDuckPoints (p.duckShape.isArray() ? shapes::pointsFromVar (p.duckShape) : shapes::duckPreset (0));
    fxRack.orderFromVar (p.fxOrder);
    macroLabels = p.macroLabels;
    auto t = std::make_shared<TuningTable>();
    t->restore (p.tuningType, p.tuningText);
    tuning.publish (t);
    currentPresetName = p.name;
    currentPresetCharacter = x["presetCharacter"].toString();
}

void BassGodsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    juce::ValueTree root ("BASSGODSDAYJOB_STATE");
    root.setProperty ("version", BGDJ_VERSION_STRING, nullptr);
    root.appendChild (state, nullptr);
    root.appendChild (saveExtras(), nullptr);
    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, destData);
}

void BassGodsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;
    auto root = juce::ValueTree::fromXml (*xml);
    if (! root.isValid())
        return;
    auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid())
        apvts.replaceState (params);
    restoreExtras (root.getChildWithName ("BGDJ_EXTRA"));
    for (auto& t : oscTables) t.loadedFactory = -1;
    ensureTables (false);
    ++presetVersion;
}

juce::AudioProcessorEditor* BassGodsProcessor::createEditor()
{
    return new BassGodsEditor (*this);
}
} // namespace bgdj

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new bgdj::BassGodsProcessor();
}
