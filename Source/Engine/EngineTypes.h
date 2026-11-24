#pragma once

#include "../Common/Parameters.h"
#include "../DSP/Wavetable.h"
#include "../DSP/NoiseBank.h"
#include "../DSP/Filter.h"
#include "../DSP/Tuning.h"
#include "ModMatrix.h"
#include "Shapes.h"

namespace bgdj
{
constexpr int kNumModDestsMax = 192;
constexpr int kControlBlock = 32;   // samples (at the oversampled rate) per modulation update

/** Indices of modulation destinations used directly by the voice code. */
struct DestIndex
{
    int oscPos[kNumWtOscs], oscWarp[kNumWtOscs], oscDetune[kNumWtOscs], oscBlend[kNumWtOscs], oscWidth[kNumWtOscs],
        oscCoarse[kNumWtOscs], oscFine[kNumWtOscs], oscPhase[kNumWtOscs], oscLevel[kNumWtOscs], oscPan[kNumWtOscs],
        oscUniWarp[kNumWtOscs];
    int dLevel, dPan, dNoisePitch;
    int fCut[kNumFilters], fRes[kNumFilters], fDrive[kNumFilters], fFat[kNumFilters], fMix[kNumFilters], fPan[kNumFilters];
    int envA[kNumEnvs], envD[kNumEnvs], envS[kNumEnvs], envR[kNumEnvs];
    int lfoRate[kNumLfos], lfoSmooth[kNumLfos];
    int porta, ampVel;

    void build();
};

struct OscStatic
{
    bool on = false;
    int warpMode = 0, warpSrc = 0, unison = 1, octave = 0, semi = 0, route = 0;
    bool phaseReset = true;
    float randPhase = 0.0f;
    const Wavetable* table = nullptr;
};

struct OscDStatic
{
    bool on = false, subLock = true, noiseKey = false, phaseReset = true;
    int mode = 0, subShape = 0, subOct = -1, noiseType = 0, route = 3;
    const NoiseSample* noise = nullptr;
};

struct LfoStatic
{
    bool sync = true, bipolar = false;
    int syncRate = 10, mode = 0, shape = 0;
    float phase = 0.0f, rise = 0.0f;
    const ShapeTable* table = nullptr;
};

/** Everything a voice needs for one processBlock, gathered once per block. */
struct EngineSnapshot
{
    OscStatic osc[kNumWtOscs];
    OscDStatic oscD;
    bool filterOn[kNumFilters] {};
    int filterType[kNumFilters] {};
    float filterKey[kNumFilters] {};
    int filterRouting = 0;
    LfoStatic lfo[kNumLfos];
    float envCurve[kNumEnvs][3] {};

    int numDests = 0;
    float baseNorm[kNumModDestsMax] {};
    float baseReal[kNumModDestsMax] {};
    const juce::RangedAudioParameter* destParam[kNumModDestsMax] {};

    int polyMode = 1, polyVoices = 8, portaMode = 1, portaType = 0, voiceSteal = 0, envRetrig = 0;
    bool legatoEnvReset = false;
    float pbRange = 2.0f, mpePbRange = 48.0f;
    int interp = 1, aaQuality = 2, uniCurve = 0, uniPhase = 0;
    float drift = 0.0f, velCurve = 0.0f;
    int filtKeyRef = 60;
    SatMode filtSat = SatMode::Tanh;
    float macros[kNumMacros] {};
    float modWheel = 0.0f, pitchBend = 0.0f, aftertouch = 0.0f;
    double bpm = 120.0, ppqAtBlockStart = 0.0;
    bool playing = false;
    double sampleRate = 48000.0;   // oversampled rate
    int osFactor = 1;
    bool seedLock = false;
    int seedValue = 0;
    float tuneRatio = 1.0f;
    const TuningTable* tuning = nullptr;
    ModMatrixSnapshot matrix;
    DestIndex dest;
};
} // namespace bgdj
