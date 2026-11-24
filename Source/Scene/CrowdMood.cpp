#include "CrowdMood.h"

namespace bgdj
{
const juce::StringArray& crowdPersonalityNames()
{
    static const juce::StringArray s { "Warehouse", "Festival", "Underground Basement", "Beach Bar", "Afterhours" };
    return s;
}

juce::String MoodState::moodWord() const
{
    if (walkout)            return "SET OVER";
    if (idle)               return "Waiting for you";
    if (annoyance > 60.0f)  return "Furious";
    if (annoyance > 35.0f)  return "Annoyed";
    if (boredom > 60.0f)    return "Bored";
    if (hype > 90.0f)       return "ABSOLUTE SCENES";
    if (hype > 75.0f)       return "Going off";
    if (hype > 55.0f)       return "Locked in";
    if (hype > 35.0f)       return "Vibing";
    if (hype > 15.0f)       return "Warming up";
    return "Unimpressed";
}

void CrowdMood::addFeatures (const AudioFeatures& f)
{
    const double w = f.blockSec;
    accSec += w;
    accRms += (double) f.rms * f.rms * w;
    accSub += f.sub * w; accMud += f.mud * w; accMid += f.mid * w; accHarsh += f.harsh * w; accAir += f.air * w;
    accFlux += f.flux; accCentroid += f.centroidHz * w; accSubCorr += f.subCorrelation * w;
    accClip += f.clipFraction * w;
    accNotes += f.noteOns;
    ++accBlocks;
    last = f;
}

void CrowdMood::tick (float dt)
{
    auto& s = state;
    dt = juce::jlimit (0.001f, 0.25f, dt);

    // personality: hype gain, patience, how much they hate mud/harshness
    float hypeGain = 1.0f, patience = 1.0f, pickiness = 1.0f;
    switch (personality)
    {
        case CrowdPersonality::Festival:            hypeGain = 1.25f; patience = 0.8f; pickiness = 0.8f; break;
        case CrowdPersonality::UndergroundBasement: hypeGain = 0.9f;  patience = 1.5f; pickiness = 1.2f; break;
        case CrowdPersonality::BeachBar:            hypeGain = 0.8f;  patience = 1.2f; pickiness = 1.4f; break;
        case CrowdPersonality::Afterhours:          hypeGain = 1.0f;  patience = 2.0f; pickiness = 0.7f; break;
        default: break;
    }

    // ---- consume accumulated features ------------------------------------------------
    s.noteOnsThisFrame = accNotes;
    s.totalNoteOns += accNotes;
    if (accSec > 0.0)
    {
        const double rms = std::sqrt (accRms / accSec);
        const double sub = accSub / accSec, mud = accMud / accSec, mid = accMid / accSec, harsh = accHarsh / accSec, air = accAir / accSec;
        const double tot = sub + mud + mid + harsh + air + 1.0e-12;
        const float k = 1.0f - std::exp (-dt / 0.35f);
        const float db = (float) (20.0 * std::log10 (rms + 1.0e-9));
        s.loudness += (juce::jlimit (0.0f, 1.0f, (db + 42.0f) / 30.0f) - s.loudness) * k;
        s.subShare += ((float) (sub / tot) - s.subShare) * k;
        s.mudShare += ((float) (mud / tot) - s.mudShare) * k;
        s.harshShare += ((float) (harsh / tot) - s.harshShare) * k;
        s.airShare += ((float) (air / tot) - s.airShare) * k;
        s.centroid += ((float) (accCentroid / accSec) - s.centroid) * k;
        s.subCorrelation += ((float) (accSubCorr / accSec) - s.subCorrelation) * k;
        s.clipFraction += ((float) (accClip / accSec) - s.clipFraction) * (1.0f - std::exp (-dt / 0.15f));
        s.groove += (juce::jlimit (0.0f, 1.0f, (float) (accFlux / std::max (1, accBlocks)) * 2.0f) - s.groove) * (1.0f - std::exp (-dt / 1.5f));
        s.silent = db < -60.0f;

        const float subNow = (float) sub;
        s.subSpike = subNow > prevSub * 6.0f && subNow > 0.01f;
        prevSub = prevSub * 0.9f + subNow * 0.1f;
    }
    s.noteRate += ((float) accNotes / dt - s.noteRate) * (1.0f - std::exp (-dt / 2.0f));

    s.bpm = last.bpm > 1.0 ? last.bpm : 120.0;
    s.hostPlaying = last.hostPlaying;
    s.sameNoteSeconds = last.sameNoteSeconds;
    s.secondsSinceNoteOn = last.secondsSinceNoteOn;
    const double beatSec = 60.0 / s.bpm;
    const bool notesActive = last.activeVoices > 0;
    s.idle = ! s.hostPlaying && ! notesActive && s.secondsSinceNoteOn > 3.0;

    // ---- edge signals ------------------------------------------------------------------
    s.subDropAfterSilence = false;
    s.dropAfterQuiet = false;
    s.sweepThenDownbeat = false;
    if (accNotes > 0 && s.silenceSeconds >= 2.0 * beatSec && s.subShare > 0.15f)
        s.subDropAfterSilence = true;
    if (accNotes > 0 && s.quietSeconds >= 4.0 * beatSec)
        s.dropAfterQuiet = true;

    centroidHistory[centroidIdx] = s.centroid;
    centroidIdx = (centroidIdx + 1) % 16;
    float cMin = 1.0e9f, cMax = 0.0f;
    for (float c : centroidHistory) { cMin = std::min (cMin, c); cMax = std::max (cMax, c); }
    if (cMax > 200.0f && cMax > cMin * 3.0f)
        s.sweepAge = 0.0;
    else
        s.sweepAge += dt;
    if (accNotes > 0 && s.sweepAge < 2.0 && s.hostPlaying)
    {
        const double beat = last.ppq - std::floor (last.ppq / 4.0) * 4.0;
        s.sweepThenDownbeat = beat < 0.5 || beat > 3.75;
    }

    s.resonanceScream = s.harshShare > 0.25f && s.groove < 0.35f && s.loudness > 0.4f;
    s.clipping = s.clipFraction > 0.005f;
    s.mud = s.mudShare > 0.42f * pickiness && s.loudness > 0.2f;
    s.phaseProblem = s.subCorrelation < 0.0f && s.subShare > 0.1f && s.loudness > 0.2f;

    // ---- durations ------------------------------------------------------------------------
    s.silenceSeconds = s.silent ? s.silenceSeconds + dt : 0.0;
    s.quietSeconds = (s.loudness < 0.25f) ? s.quietSeconds + dt : 0.0;

    // ---- annoyance & boredom -----------------------------------------------------------------
    float annoyTarget = 0.0f;
    if (s.mud)           annoyTarget += 45.0f * pickiness;
    if (s.harshShare > 0.35f && s.loudness > 0.3f) annoyTarget += 40.0f * pickiness;
    if (s.clipping)      annoyTarget += 60.0f;
    if (s.phaseProblem)  annoyTarget += 35.0f;
    annoyTarget = std::min (100.0f, annoyTarget);
    const float annoyTau = annoyTarget > s.annoyance ? 2.5f : 6.0f;
    s.annoyance += (annoyTarget - s.annoyance) * (1.0f - std::exp (-dt / annoyTau));

    float boredTarget = 0.0f;
    if (s.sameNoteSeconds > 8.0 * patience)                 boredTarget += 70.0f;
    if (s.hostPlaying && s.secondsSinceNoteOn > 10.0 * patience) boredTarget += 80.0f;
    if (! s.idle && s.groove < 0.05f && s.noteRate < 0.2f)  boredTarget += 30.0f;
    boredTarget = std::min (100.0f, boredTarget);
    const float boredTau = boredTarget > s.boredom ? 5.0f * patience : 2.0f;
    s.boredom += (boredTarget - s.boredom) * (1.0f - std::exp (-dt / boredTau));
    s.bored = s.boredom > 55.0f;

    // ---- energy & hype ------------------------------------------------------------------------
    const float activity = juce::jlimit (0.0f, 1.0f, s.noteRate / 3.0f + (notesActive ? 0.35f : 0.0f));
    const float subGood = juce::jlimit (0.0f, 1.0f, s.subShare * 2.5f);
    const float energyTarget = s.silent ? 0.0f : s.loudness * (0.45f + 0.35f * activity + 0.2f * s.groove);
    s.energy += (energyTarget - s.energy) * (1.0f - std::exp (-dt / 0.8f));

    float hypeTarget = 100.0f * hypeGain * (0.55f * s.energy + 0.25f * subGood * s.loudness + 0.2f * s.groove)
                     - 0.6f * s.annoyance - 0.5f * s.boredom;
    if (s.idle) hypeTarget = std::min (hypeTarget, 20.0f);
    if (s.subDropAfterSilence) s.hype = std::min (100.0f, s.hype + 12.0f);
    hypeTarget = juce::jlimit (0.0f, 100.0f, hypeTarget);
    const float hypeTau = hypeTarget > s.hype ? 4.0f : 8.0f;
    s.hype += (hypeTarget - s.hype) * (1.0f - std::exp (-dt / hypeTau));

    s.hypeHighSeconds = s.hype > 85.0f ? s.hypeHighSeconds + dt : 0.0;
    s.hypeLowSeconds = (s.hype < 10.0f && ! s.idle) ? s.hypeLowSeconds + dt : 0.0;

    // ---- crowd size ---------------------------------------------------------------------------
    float sizeTarget = 0.55f + 0.45f * (s.hype / 100.0f);
    if (s.barsHypeLow() > 16.0) sizeTarget = 0.2f;
    if (s.walkout) sizeTarget = 0.0f;
    if (personality == CrowdPersonality::UndergroundBasement) sizeTarget = std::min (1.0f, sizeTarget * 1.1f);
    s.crowdSize += (sizeTarget - s.crowdSize) * (1.0f - std::exp (-dt / 10.0f));
    if (s.walkout && s.hype > 30.0f)
        s.walkout = false;

    accSec = accRms = accSub = accMud = accMid = accHarsh = accAir = accFlux = accCentroid = accSubCorr = accClip = 0.0;
    accBlocks = accNotes = 0;
}
} // namespace bgdj
