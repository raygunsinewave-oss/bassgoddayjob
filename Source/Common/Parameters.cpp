#include "Parameters.h"

namespace bgdj::params
{
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;
using API = juce::AudioParameterInt;
using Range = juce::NormalisableRange<float>;

#define BGDJ_LIST(fn, ...) const juce::StringArray& fn() { static const juce::StringArray s { __VA_ARGS__ }; return s; }

BGDJ_LIST (warpModes, "Off", "Sync", "Bend +", "Bend -", "Bend +/-", "PWM", "Asym +", "Asym -", "Mirror", "Quantize", "FM", "AM", "RM")
BGDJ_LIST (warpSources, "Osc A", "Osc B", "Osc C", "Sub", "Noise")
BGDJ_LIST (oscRoutes, "Filter 1", "Filter 2", "Both", "Direct")
BGDJ_LIST (subShapes, "Sine", "Triangle", "Square", "Rounded Saw")
BGDJ_LIST (oscDModes, "Sub", "Noise")
BGDJ_LIST (filterTypes,
           "LP 12", "LP 24", "LP 36", "HP 12", "HP 24", "BP 12", "BP 24", "Notch 12", "Notch 24", "Peak",
           "All Pass", "Low Shelf", "High Shelf", "Ladder LP 24", "Ladder HP 24", "Acid Ladder 303", "Diode LP",
           "MS-20 LP", "MS-20 HP", "Dirty LP 12", "Dirty LP 24", "Dirty BP", "Comb +", "Comb -", "Flange Comb",
           "Formant Vowel", "Formant Talker", "Phaser 4", "Phaser 8", "Phaser 12", "LP/HP Morph", "Dual BP")
BGDJ_LIST (filterRoutings, "Serial", "Parallel", "Split")
BGDJ_LIST (polyModes, "Mono", "Legato", "Poly")
BGDJ_LIST (portaModes, "Always", "Legato")
BGDJ_LIST (portaTypes, "Constant Time", "Constant Rate", "Scaled")
BGDJ_LIST (lfoModes, "Trigger", "Envelope", "Free")
BGDJ_LIST (lfoShapes, "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "Pulse", "Sample & Hold",
           "Smooth Random", "Wobble", "Stairs", "Bounce", "Custom")
BGDJ_LIST (syncRates, "8 Bars", "4 Bars", "2 Bars", "1 Bar", "1/2", "1/2 D", "1/2 T", "1/4", "1/4 D", "1/4 T",
           "1/8", "1/8 D", "1/8 T", "1/16", "1/16 D", "1/16 T", "1/32", "1/32 T", "1/64")
BGDJ_LIST (oversampleFactors, "1x", "2x", "4x", "8x", "16x")
BGDJ_LIST (offlineOversample, "Same as Realtime", "2x", "4x", "8x", "16x")
BGDJ_LIST (interpModes, "Nearest", "Linear", "Cubic")
BGDJ_LIST (aaQualities, "Low", "Medium", "High", "Ultra")
BGDJ_LIST (unisonCurves, "Linear", "Exponential", "Supersaw")
BGDJ_LIST (unisonPhaseModes, "Random", "Spread", "Zero")
BGDJ_LIST (voiceStealModes, "Oldest", "Quietest", "Lowest Note", "Highest Note")
BGDJ_LIST (envRetrigModes, "Reset to Zero", "From Current Level")
BGDJ_LIST (filterSatModes, "Tanh", "Soft Clip", "Asymmetric", "Tube")
BGDJ_LIST (mpeZones, "Lower", "Upper")
BGDJ_LIST (modSourceNames, "None", "Env 1", "Env 2", "Env 3", "Env 4",
           "LFO 1", "LFO 2", "LFO 3", "LFO 4", "LFO 5", "LFO 6", "LFO 7", "LFO 8",
           "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Velocity", "Note Random", "Key Track",
           "Mod Wheel", "Pitch Bend", "Aftertouch", "MPE Slide", "MPE Pressure", "Noise")
BGDJ_LIST (fxModuleIds, "dist", "ott", "comp", "duck", "eq", "ffx", "mod", "delay", "verb", "hyper", "sat", "util", "lim")
BGDJ_LIST (fxModuleNames, "Distortion", "OTT Multiband", "Compressor", "Ducker", "EQ", "Filter FX",
           "Chorus / Flanger / Phaser", "Delay", "Reverb", "Hyper / Dimension", "Saturator / Tape",
           "Imager / Utility", "Limiter / Clipper")
BGDJ_LIST (distModes, "Tube", "Diode", "Soft Clip", "Hard Clip", "Foldback", "Bitcrush", "Downsample", "Rectify", "Asym", "Multiband")
BGDJ_LIST (compTypes, "VCA", "Opto", "Glue")
BGDJ_LIST (duckRates, "1/1", "1/2", "1/4", "1/8", "1/16", "1/2 T", "1/4 T", "1/8 T")
BGDJ_LIST (duckShapes, "Kick Duck Hard", "Kick Duck Soft", "Long Pump", "Gate Chop", "Custom")
BGDJ_LIST (eqTypes, "Low Cut", "Low Shelf", "Peak", "Notch", "High Shelf", "High Cut")
BGDJ_LIST (ffxTypes, "Low Pass", "High Pass", "Band Pass")
BGDJ_LIST (modFxModes, "Chorus", "Flanger", "Phaser")
BGDJ_LIST (delayModes, "Stereo", "Ping-Pong", "Tape")
BGDJ_LIST (verbTypes, "Plate", "Room", "Hall", "Club")
BGDJ_LIST (limModes, "Limiter", "Clipper", "Limit + Clip")
#undef BGDJ_LIST

double syncRateBeats (int i)
{
    static const double b[] = { 32.0, 16.0, 8.0, 4.0, 2.0, 3.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0,
                                0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125, 1.0 / 12.0, 0.0625 };
    return b[juce::jlimit (0, (int) std::size (b) - 1, i)];
}

double duckRateBeats (int i)
{
    static const double b[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 4.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0 };
    return b[juce::jlimit (0, (int) std::size (b) - 1, i)];
}

juce::String oscPrefix (int osc)     { return juce::String ("o") + juce::String::charToString ((juce::juce_wchar) ('A' + osc)) + "_"; }
juce::String filterPrefix (int f)    { return "f" + juce::String (f + 1) + "_"; }
juce::String envPrefix (int e)       { return "e" + juce::String (e + 1) + "_"; }
juce::String lfoPrefix (int l)       { return "l" + juce::String (l + 1) + "_"; }
juce::String macroId (int m)         { return "macro" + juce::String (m + 1); }
juce::String fxPrefix (FxType t)     { return "fx_" + fxModuleIds()[(int) t] + "_"; }

// ============================================================================
// Value <-> text helpers
// ============================================================================
namespace
{
    Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::String hzText (float v, int)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (v, v < 100.0f ? 1 : 0) + " Hz";
    }
    juce::String msText (float v, int)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (v, v < 10.0f ? 2 : 1) + " ms";
    }
    juce::String pctText (float v, int)     { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    juce::String bipolarPct (float v, int)  { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
    juce::String dbText (float v, int)      { return v <= -59.9f ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; }
    juce::String centsText (float v, int)   { return juce::String (juce::roundToInt (v)) + " ct"; }
    juce::String semiText (float v, int)    { return juce::String (v, 2) + " st"; }
    juce::String panText (float v, int)
    {
        if (std::abs (v) < 0.005f) return "C";
        return juce::String (juce::roundToInt (std::abs (v) * 100.0f)) + (v < 0 ? " L" : " R");
    }

    juce::AudioParameterFloatAttributes attrs (std::function<juce::String (float, int)> fn, const juce::String& label = {})
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fn)).withLabel (label);
    }

    struct LayoutBuilder
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        void f (const juce::String& id, const juce::String& name, Range r, float def,
                std::function<juce::String (float, int)> txt = pctText, bool automatable = true)
        {
            auto a = attrs (std::move (txt)).withAutomatable (automatable);
            params.push_back (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def, a));
        }
        void i (const juce::String& id, const juce::String& name, int lo, int hi, int def)
        {
            params.push_back (std::make_unique<API> (juce::ParameterID { id, 1 }, name, lo, hi, def));
        }
        void b (const juce::String& id, const juce::String& name, bool def)
        {
            params.push_back (std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def));
        }
        void c (const juce::String& id, const juce::String& name, const juce::StringArray& list, int def)
        {
            params.push_back (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, list, def));
        }
    };
} // namespace

// ============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    LayoutBuilder L;

    // ---- Master / voicing ---------------------------------------------------
    L.f ("master_vol", "Master Volume", Range (-60.0f, 6.0f, 0.01f), -3.0f, dbText);
    L.f ("amp_vel", "Velocity -> Amp", Range (0.0f, 1.0f), 0.35f);
    L.c ("poly_mode", "Voice Mode", polyModes(), 1);
    L.i ("poly_voices", "Poly Voices", 1, kMaxVoices, 8);
    L.f ("porta_time", "Portamento", skewed (0.0f, 2000.0f, 150.0f), 0.0f, msText);
    L.c ("porta_mode", "Portamento Mode", portaModes(), 1);
    L.c ("porta_type", "Portamento Type", portaTypes(), 0);
    L.i ("pb_range", "Pitch Bend Range", 0, 24, 2);
    L.b ("bassmono_on", "Bass Mono On", true);
    L.f ("bassmono_freq", "Bass Mono Below", skewed (20.0f, 400.0f, 120.0f), 120.0f, hzText);
    L.c ("filter_routing", "Filter Routing", filterRoutings(), 0);

    for (int m = 0; m < kNumMacros; ++m)
        L.f (macroId (m), "Macro " + juce::String (m + 1), Range (0.0f, 1.0f), 0.0f);

    // ---- Wavetable oscillators --------------------------------------------
    for (int o = 0; o < kNumWtOscs; ++o)
    {
        const auto p = oscPrefix (o);
        const auto n = juce::String ("Osc ") + juce::String::charToString ((juce::juce_wchar) ('A' + o)) + " ";
        L.b (p + "on", n + "On", o == 0);
        L.i (p + "table", n + "Wavetable", 0, 127, o == 0 ? 1 : 0);
        L.f (p + "pos", n + "WT Position", Range (0.0f, 1.0f), 0.0f);
        L.c (p + "warp", n + "Warp Mode", warpModes(), 0);
        L.f (p + "warp_amt", n + "Warp Amount", Range (0.0f, 1.0f), 0.0f);
        L.c (p + "warp_src", n + "Warp Source", warpSources(), o == 0 ? 1 : 0);
        L.i (p + "uni", n + "Unison", 1, kMaxUnison, 1);
        L.f (p + "detune", n + "Unison Detune", Range (0.0f, 1.0f), 0.25f);
        L.f (p + "blend", n + "Unison Blend", Range (0.0f, 1.0f), 0.75f);
        L.f (p + "width", n + "Unison Width", Range (0.0f, 1.0f), 0.8f);
        L.f (p + "uni_warp", n + "Unison Warp Spread", Range (0.0f, 1.0f), 0.0f);
        L.i (p + "oct", n + "Octave", -4, 4, 0);
        L.i (p + "semi", n + "Semi", -12, 12, 0);
        L.f (p + "fine", n + "Fine", Range (-100.0f, 100.0f), 0.0f, centsText);
        L.f (p + "coarse", n + "Coarse Pitch", Range (-48.0f, 48.0f), 0.0f, semiText);
        L.f (p + "phase", n + "Phase", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "rand", n + "Random Phase", Range (0.0f, 1.0f), o == 0 ? 0.0f : 1.0f);
        L.f (p + "level", n + "Level", Range (0.0f, 1.0f), 0.75f);
        L.f (p + "pan", n + "Pan", Range (-1.0f, 1.0f), 0.0f, panText);
        L.c (p + "route", n + "Route", oscRoutes(), 0);
        L.b (p + "phase_reset", n + "Phase Reset on Note", true);
    }

    // ---- OSC D: sub / noise hybrid ----------------------------------------
    {
        const auto p = oscD();
        L.b (p + "on", "Osc D On", true);
        L.c (p + "mode", "Osc D Mode", oscDModes(), 0);
        L.c (p + "sub_shape", "Sub Shape", subShapes(), 0);
        L.i (p + "sub_oct", "Sub Octave", -2, 0, -1);
        L.b (p + "sub_lock", "Sub Lock (mono, centred)", true);
        L.i (p + "noise_type", "Noise Type", 0, 63, 0);
        L.b (p + "noise_key", "Noise Key Track", false);
        L.f (p + "noise_pitch", "Noise Pitch", Range (-24.0f, 24.0f), 0.0f, semiText);
        L.f (p + "level", "Osc D Level", Range (0.0f, 1.0f), 0.5f);
        L.f (p + "pan", "Osc D Pan", Range (-1.0f, 1.0f), 0.0f, panText);
        L.c (p + "route", "Osc D Route", oscRoutes(), 3);
        L.b (p + "phase_reset", "Osc D Phase Reset on Note", true);
    }

    // ---- Filters -------------------------------------------------------------
    for (int fi = 0; fi < kNumFilters; ++fi)
    {
        const auto p = filterPrefix (fi);
        const auto n = "Filter " + juce::String (fi + 1) + " ";
        L.b (p + "on", n + "On", fi == 0);
        L.c (p + "type", n + "Type", filterTypes(), fi == 0 ? 1 : 3);
        L.f (p + "cutoff", n + "Cutoff", skewed (20.0f, 20000.0f, 900.0f), fi == 0 ? 2400.0f : 60.0f, hzText);
        L.f (p + "reso", n + "Resonance", Range (0.0f, 1.0f), 0.15f);
        L.f (p + "drive", n + "Drive", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "fat", n + "Fat", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "key", n + "Key Track", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "mix", n + "Mix", Range (0.0f, 1.0f), 1.0f);
        L.f (p + "pan", n + "Pan", Range (-1.0f, 1.0f), 0.0f, panText);
    }

    // ---- Envelopes -----------------------------------------------------------
    for (int e = 0; e < kNumEnvs; ++e)
    {
        const auto p = envPrefix (e);
        const auto n = "Env " + juce::String (e + 1) + " ";
        L.f (p + "att", n + "Attack", skewed (0.0f, 10000.0f, 400.0f), e == 0 ? 1.0f : 0.5f, msText);
        L.f (p + "dec", n + "Decay", skewed (1.0f, 15000.0f, 600.0f), e == 0 ? 400.0f : 250.0f, msText);
        L.f (p + "sus", n + "Sustain", Range (0.0f, 1.0f), e == 0 ? 1.0f : 0.0f);
        L.f (p + "rel", n + "Release", skewed (1.0f, 15000.0f, 600.0f), e == 0 ? 60.0f : 150.0f, msText);
        L.f (p + "acurve", n + "Attack Curve", Range (-1.0f, 1.0f), 0.0f, bipolarPct);
        L.f (p + "dcurve", n + "Decay Curve", Range (-1.0f, 1.0f), 0.5f, bipolarPct);
        L.f (p + "rcurve", n + "Release Curve", Range (-1.0f, 1.0f), 0.5f, bipolarPct);
    }

    // ---- LFOs ------------------------------------------------------------------
    for (int l = 0; l < kNumLfos; ++l)
    {
        const auto p = lfoPrefix (l);
        const auto n = "LFO " + juce::String (l + 1) + " ";
        L.f (p + "rate", n + "Rate", skewed (0.01f, 40.0f, 2.0f), 2.0f, hzText);
        L.b (p + "sync", n + "Tempo Sync", true);
        L.c (p + "sync_rate", n + "Sync Rate", syncRates(), 10);
        L.c (p + "mode", n + "Mode", lfoModes(), 0);
        L.c (p + "shape", n + "Shape", lfoShapes(), 0);
        L.f (p + "phase", n + "Phase", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "smooth", n + "Smooth", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "rise", n + "Rise", skewed (0.0f, 5000.0f, 500.0f), 0.0f, msText);
        L.b (p + "bipolar", n + "Bipolar", false);
    }

    // ---- Tweaks -------------------------------------------------------------------
    L.c ("os_synth", "Synth Oversampling", oversampleFactors(), 1);
    L.c ("os_fx", "FX Oversampling", oversampleFactors(), 1);
    L.c ("os_offline", "Offline Render Oversampling", offlineOversample(), 2);
    L.c ("wt_interp", "Wavetable Interpolation", interpModes(), 1);
    L.c ("aa_quality", "Anti-Alias Quality", aaQualities(), 2);
    L.c ("uni_curve", "Unison Detune Curve", unisonCurves(), 0);
    L.c ("uni_phase", "Unison Phase Mode", unisonPhaseModes(), 0);
    L.c ("voice_steal", "Voice Stealing", voiceStealModes(), 0);
    L.c ("env_retrig", "Envelope Retrigger", envRetrigModes(), 0);
    L.b ("legato_env_reset", "Legato Envelope Reset", false);
    L.f ("tune_a4", "Global Tuning (A4)", Range (415.0f, 466.0f, 0.01f), 440.0f, [] (float v, int) { return juce::String (v, 2) + " Hz"; });
    L.f ("drift", "Analog Drift", Range (0.0f, 1.0f), 0.0f);
    L.f ("vel_curve", "Velocity Curve", Range (-1.0f, 1.0f), 0.0f, bipolarPct);
    L.b ("mpe_on", "MPE Enabled", false);
    L.c ("mpe_zone", "MPE Zone", mpeZones(), 0);
    L.i ("mpe_members", "MPE Member Channels", 1, 15, 15);
    L.i ("mpe_pb_range", "MPE Pitch Bend Range", 0, 96, 48);
    L.i ("filt_key_ref", "Filter Key-Track Reference Note", 0, 127, 60);
    L.c ("filt_sat", "Filter Saturation Algorithm", filterSatModes(), 0);
    L.b ("denormal_ftz", "Flush Denormals to Zero", true);
    L.b ("dc_blocker", "DC Blocker", true);
    L.b ("seed_lock", "Random Seed Lock", false);
    L.i ("seed_value", "Random Seed", 0, 9999, 1234);
    L.b ("cat_chaos", "Cat Chaos Mode", false);
    L.b ("crowd_sfx_out", "Crowd SFX to Output", false);
    L.f ("crowd_sfx_level", "Crowd SFX Level", Range (-60.0f, 0.0f, 0.1f), -18.0f, dbText);

    // ---- FX ----------------------------------------------------------------------
    auto fxOn = [&] (FxType t, bool def, float mix = 1.0f)
    {
        const auto p = fxPrefix (t);
        const auto n = fxModuleNames()[(int) t] + " ";
        L.b (p + "on", n + "On", def);
        L.f (p + "mix", n + "Mix", Range (0.0f, 1.0f), mix);
    };
    {   // Distortion
        const auto p = fxPrefix (FxType::Distortion); fxOn (FxType::Distortion, false);
        L.c (p + "mode", "Distortion Mode", distModes(), 0);
        L.f (p + "drive", "Distortion Drive", Range (0.0f, 1.0f), 0.3f);
        L.f (p + "bias", "Distortion Bias", Range (-1.0f, 1.0f), 0.0f, bipolarPct);
        L.f (p + "tone", "Distortion Tone", skewed (200.0f, 20000.0f, 4000.0f), 20000.0f, hzText);
        L.f (p + "out", "Distortion Output", Range (-24.0f, 12.0f), 0.0f, dbText);
        L.f (p + "mb_low", "Distortion Low Drive", Range (0.0f, 1.0f), 0.1f);
        L.f (p + "mb_mid", "Distortion Mid Drive", Range (0.0f, 1.0f), 0.5f);
        L.f (p + "mb_high", "Distortion High Drive", Range (0.0f, 1.0f), 0.3f);
        L.f (p + "mb_xlow", "Distortion Low Crossover", skewed (40.0f, 600.0f, 150.0f), 150.0f, hzText);
        L.f (p + "mb_xhigh", "Distortion High Crossover", skewed (800.0f, 10000.0f, 3000.0f), 3000.0f, hzText);
    }
    {   // OTT
        const auto p = fxPrefix (FxType::Ott); fxOn (FxType::Ott, false, 0.5f);
        L.f (p + "depth", "OTT Depth", Range (0.0f, 1.0f), 1.0f);
        L.f (p + "time", "OTT Time", Range (0.0f, 1.0f), 0.5f);
        L.f (p + "in", "OTT Input Gain", Range (-24.0f, 24.0f), 0.0f, dbText);
        L.f (p + "out", "OTT Output Gain", Range (-24.0f, 24.0f), 0.0f, dbText);
        for (auto band : { "low", "mid", "high" })
        {
            L.f (p + band + "_up", juce::String ("OTT ") + band + " Upward", Range (0.0f, 1.0f), 1.0f);
            L.f (p + band + "_down", juce::String ("OTT ") + band + " Downward", Range (0.0f, 1.0f), 1.0f);
        }
    }
    {   // Compressor
        const auto p = fxPrefix (FxType::Compressor); fxOn (FxType::Compressor, false);
        L.c (p + "type", "Compressor Type", compTypes(), 0);
        L.f (p + "thresh", "Compressor Threshold", Range (-60.0f, 0.0f), -18.0f, dbText);
        L.f (p + "ratio", "Compressor Ratio", skewed (1.0f, 20.0f, 4.0f), 4.0f, [] (float v, int) { return juce::String (v, 1) + ":1"; });
        L.f (p + "attack", "Compressor Attack", skewed (0.1f, 100.0f, 10.0f), 5.0f, msText);
        L.f (p + "release", "Compressor Release", skewed (10.0f, 1000.0f, 150.0f), 120.0f, msText);
        L.f (p + "knee", "Compressor Knee", Range (0.0f, 12.0f), 3.0f, dbText);
        L.f (p + "makeup", "Compressor Makeup", Range (0.0f, 24.0f), 0.0f, dbText);
        L.b (p + "sc", "Compressor External Sidechain", false);
        L.f (p + "sc_hpf", "Compressor Sidechain HPF", skewed (20.0f, 500.0f, 100.0f), 20.0f, hzText);
    }
    {   // Ducker
        const auto p = fxPrefix (FxType::Ducker); fxOn (FxType::Ducker, false);
        L.c (p + "rate", "Ducker Rate", duckRates(), 2);
        L.f (p + "depth", "Ducker Depth", Range (0.0f, 1.0f), 0.8f);
        L.c (p + "shape", "Ducker Shape", duckShapes(), 0);
        L.f (p + "smooth", "Ducker Smooth", Range (0.0f, 1.0f), 0.2f);
        L.f (p + "offset", "Ducker Offset", Range (0.0f, 1.0f), 0.0f);
    }
    {   // EQ
        const auto p = fxPrefix (FxType::Eq); fxOn (FxType::Eq, false);
        const float defF[] = { 30.0f, 90.0f, 300.0f, 1200.0f, 5000.0f, 16000.0f };
        const int defT[] = { 0, 1, 2, 2, 4, 5 };
        for (int band = 0; band < 6; ++band)
        {
            const auto bp = p + "b" + juce::String (band + 1) + "_";
            const auto bn = "EQ Band " + juce::String (band + 1) + " ";
            L.b (bp + "on", bn + "On", band == 0);
            L.c (bp + "type", bn + "Type", eqTypes(), defT[band]);
            L.f (bp + "freq", bn + "Freq", skewed (20.0f, 20000.0f, 1000.0f), defF[band], hzText);
            L.f (bp + "gain", bn + "Gain", Range (-24.0f, 24.0f), 0.0f, dbText);
            L.f (bp + "q", bn + "Q", skewed (0.1f, 18.0f, 1.0f), 0.707f, [] (float v, int) { return juce::String (v, 2); });
        }
        L.f (p + "out", "EQ Output", Range (-24.0f, 24.0f), 0.0f, dbText);
    }
    {   // Filter FX
        const auto p = fxPrefix (FxType::FilterFx); fxOn (FxType::FilterFx, false);
        L.c (p + "type", "Filter FX Type", ffxTypes(), 0);
        L.f (p + "cutoff", "Filter FX Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 8000.0f, hzText);
        L.f (p + "reso", "Filter FX Resonance", Range (0.0f, 1.0f), 0.2f);
        L.f (p + "drive", "Filter FX Drive", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "lfo_rate", "Filter FX LFO Rate", skewed (0.01f, 20.0f, 1.0f), 0.5f, hzText);
        L.b (p + "lfo_sync", "Filter FX LFO Sync", true);
        L.c (p + "lfo_sync_rate", "Filter FX LFO Sync Rate", syncRates(), 3);
        L.f (p + "lfo_depth", "Filter FX LFO Depth", Range (0.0f, 1.0f), 0.0f);
    }
    {   // Chorus / Flanger / Phaser
        const auto p = fxPrefix (FxType::Modulation); fxOn (FxType::Modulation, false, 0.5f);
        L.c (p + "mode", "Mod FX Mode", modFxModes(), 0);
        L.f (p + "rate", "Mod FX Rate", skewed (0.01f, 10.0f, 1.0f), 0.6f, hzText);
        L.f (p + "depth", "Mod FX Depth", Range (0.0f, 1.0f), 0.5f);
        L.f (p + "feedback", "Mod FX Feedback", Range (-0.95f, 0.95f), 0.2f, bipolarPct);
        L.f (p + "delay", "Mod FX Delay", Range (1.0f, 40.0f), 12.0f, msText);
        L.i (p + "stages", "Phaser Stages", 2, 12, 6);
    }
    {   // Delay
        const auto p = fxPrefix (FxType::Delay); fxOn (FxType::Delay, false, 0.25f);
        L.c (p + "mode", "Delay Mode", delayModes(), 1);
        L.b (p + "sync", "Delay Sync", true);
        L.c (p + "sync_l", "Delay Sync Left", syncRates(), 11);
        L.c (p + "sync_r", "Delay Sync Right", syncRates(), 10);
        L.f (p + "time_l", "Delay Time Left", skewed (1.0f, 2000.0f, 300.0f), 375.0f, msText);
        L.f (p + "time_r", "Delay Time Right", skewed (1.0f, 2000.0f, 300.0f), 250.0f, msText);
        L.f (p + "feedback", "Delay Feedback", Range (0.0f, 0.98f), 0.35f);
        L.f (p + "hp", "Delay Feedback HP", skewed (20.0f, 2000.0f, 200.0f), 150.0f, hzText);
        L.f (p + "lp", "Delay Feedback LP", skewed (500.0f, 20000.0f, 4000.0f), 8000.0f, hzText);
        L.f (p + "wow", "Delay Tape Wow", Range (0.0f, 1.0f), 0.2f);
    }
    {   // Reverb
        const auto p = fxPrefix (FxType::Reverb); fxOn (FxType::Reverb, false, 0.2f);
        L.c (p + "type", "Reverb Type", verbTypes(), 3);
        L.f (p + "size", "Reverb Size", Range (0.0f, 1.0f), 0.5f);
        L.f (p + "decay", "Reverb Decay", skewed (0.1f, 20.0f, 2.0f), 1.6f, [] (float v, int) { return juce::String (v, 2) + " s"; });
        L.f (p + "damp", "Reverb Damping", Range (0.0f, 1.0f), 0.5f);
        L.f (p + "predelay", "Reverb Pre-Delay", Range (0.0f, 200.0f), 10.0f, msText);
        L.f (p + "width", "Reverb Width", Range (0.0f, 1.0f), 1.0f);
        L.f (p + "lowcut", "Reverb Low Cut", skewed (20.0f, 1000.0f, 200.0f), 180.0f, hzText);
    }
    {   // Hyper / Dimension
        const auto p = fxPrefix (FxType::Hyper); fxOn (FxType::Hyper, false, 0.5f);
        L.f (p + "rate", "Hyper Rate", Range (0.0f, 1.0f), 0.4f);
        L.f (p + "detune", "Hyper Detune", Range (0.0f, 1.0f), 0.3f);
        L.i (p + "voices", "Hyper Voices", 1, 7, 4);
        L.f (p + "dim", "Dimension Size", Range (0.0f, 1.0f), 0.4f);
        L.f (p + "mono_below", "Hyper Mono Below", skewed (20.0f, 500.0f, 150.0f), 150.0f, hzText);
    }
    {   // Saturator / Tape
        const auto p = fxPrefix (FxType::Saturator); fxOn (FxType::Saturator, false);
        L.f (p + "drive", "Saturator Drive", Range (0.0f, 1.0f), 0.3f);
        L.f (p + "warmth", "Saturator Warmth", Range (0.0f, 1.0f), 0.4f);
        L.f (p + "wow", "Tape Wow", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "flutter", "Tape Flutter", Range (0.0f, 1.0f), 0.0f);
        L.f (p + "tone", "Saturator Tone", Range (-1.0f, 1.0f), 0.0f, bipolarPct);
        L.f (p + "out", "Saturator Output", Range (-24.0f, 12.0f), 0.0f, dbText);
    }
    {   // Utility
        const auto p = fxPrefix (FxType::Utility); fxOn (FxType::Utility, true);
        L.f (p + "width", "Utility Width", Range (0.0f, 2.0f), 1.0f);
        L.b (p + "mono_on", "Utility Mono Below On", false);
        L.f (p + "mono_below", "Utility Mono Below", skewed (20.0f, 500.0f, 120.0f), 100.0f, hzText);
        L.f (p + "gain", "Utility Gain", Range (-24.0f, 24.0f), 0.0f, dbText);
        L.f (p + "pan", "Utility Pan", Range (-1.0f, 1.0f), 0.0f, panText);
        L.b (p + "dc", "Utility DC Filter", true);
        L.b (p + "inv_l", "Utility Invert Left", false);
        L.b (p + "inv_r", "Utility Invert Right", false);
    }
    {   // Limiter / Clipper (always the final stage)
        const auto p = fxPrefix (FxType::Limiter); fxOn (FxType::Limiter, true);
        L.c (p + "mode", "Limiter Mode", limModes(), 0);
        L.f (p + "input", "Limiter Input Gain", Range (0.0f, 24.0f), 0.0f, dbText);
        L.f (p + "ceiling", "Limiter Ceiling", Range (-24.0f, 0.0f), -0.3f, dbText);
        L.f (p + "release", "Limiter Release", skewed (1.0f, 500.0f, 60.0f), 80.0f, msText);
        L.b (p + "true_peak", "Limiter True Peak", true);
    }

    return { std::make_move_iterator (L.params.begin()), std::make_move_iterator (L.params.end()) };
}

// ============================================================================
const std::vector<ModDest>& modDestinations()
{
    static const std::vector<ModDest> dests = []
    {
        std::vector<ModDest> d;
        auto v = [&d] (const juce::String& id, const juce::String& name) { d.push_back ({ id, name, true }); };
        auto g = [&d] (const juce::String& id, const juce::String& name) { d.push_back ({ id, name, false }); };

        for (int o = 0; o < kNumWtOscs; ++o)
        {
            const auto p = oscPrefix (o);
            const auto n = juce::String ("Osc ") + juce::String::charToString ((juce::juce_wchar) ('A' + o)) + " ";
            v (p + "pos", n + "WT Pos");      v (p + "warp_amt", n + "Warp");
            v (p + "detune", n + "Detune");   v (p + "blend", n + "Blend");
            v (p + "width", n + "Width");     v (p + "coarse", n + "Pitch");
            v (p + "fine", n + "Fine");       v (p + "phase", n + "Phase");
            v (p + "level", n + "Level");     v (p + "pan", n + "Pan");
            v (p + "uni_warp", n + "Uni Warp");
        }
        v ("oD_level", "Osc D Level");  v ("oD_pan", "Osc D Pan");  v ("oD_noise_pitch", "Noise Pitch");
        for (int f = 0; f < kNumFilters; ++f)
        {
            const auto p = filterPrefix (f);
            const auto n = "Filter " + juce::String (f + 1) + " ";
            v (p + "cutoff", n + "Cutoff"); v (p + "reso", n + "Resonance"); v (p + "drive", n + "Drive");
            v (p + "fat", n + "Fat");       v (p + "mix", n + "Mix");        v (p + "pan", n + "Pan");
        }
        for (int e = 0; e < kNumEnvs; ++e)
        {
            const auto p = envPrefix (e);
            const auto n = "Env " + juce::String (e + 1) + " ";
            v (p + "att", n + "Attack"); v (p + "dec", n + "Decay"); v (p + "sus", n + "Sustain"); v (p + "rel", n + "Release");
        }
        for (int l = 0; l < kNumLfos; ++l)
        {
            v (lfoPrefix (l) + "rate", "LFO " + juce::String (l + 1) + " Rate");
            v (lfoPrefix (l) + "smooth", "LFO " + juce::String (l + 1) + " Smooth");
        }
        v ("porta_time", "Portamento");
        v ("amp_vel", "Velocity -> Amp");

        // Global (evaluated from the most recently triggered voice)
        g ("master_vol", "Master Volume");
        g ("fx_dist_drive", "Distortion Drive");   g ("fx_dist_mix", "Distortion Mix");
        g ("fx_dist_tone", "Distortion Tone");
        g ("fx_ott_depth", "OTT Depth");
        g ("fx_ffx_cutoff", "Filter FX Cutoff");   g ("fx_ffx_reso", "Filter FX Resonance");
        g ("fx_duck_depth", "Ducker Depth");
        g ("fx_mod_mix", "Mod FX Mix");            g ("fx_mod_rate", "Mod FX Rate");
        g ("fx_delay_mix", "Delay Mix");           g ("fx_delay_feedback", "Delay Feedback");
        g ("fx_verb_mix", "Reverb Mix");           g ("fx_verb_size", "Reverb Size");
        g ("fx_hyper_mix", "Hyper Mix");           g ("fx_hyper_detune", "Hyper Detune");
        g ("fx_sat_drive", "Saturator Drive");
        g ("fx_util_width", "Utility Width");
        return d;
    }();
    return dests;
}

int modDestIndexForParam (const juce::String& paramID)
{
    const auto& d = modDestinations();
    for (size_t i = 0; i < d.size(); ++i)
        if (d[i].paramID == paramID)
            return (int) i;
    return -1;
}

juce::StringArray paramsInSection (const juce::String& section, const juce::AudioProcessorValueTreeState& state)
{
    juce::StringArray out;
    static const juce::StringArray never { "master_vol", "os_synth", "os_fx", "os_offline", "denormal_ftz", "seed_lock",
                                           "seed_value", "crowd_sfx_out", "crowd_sfx_level", "mpe_on", "mpe_zone",
                                           "mpe_members", "mpe_pb_range", "tune_a4", "cat_chaos", "fx_lim_ceiling" };
    for (auto* p : state.processor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (rp == nullptr || never.contains (rp->paramID))
            continue;
        const auto& id = rp->paramID;
        bool match = false;
        if (section == "all")         match = ! id.startsWith ("fx_lim");
        else if (section == "osc")    match = id.startsWith ("oA_") || id.startsWith ("oB_") || id.startsWith ("oC_") || id.startsWith ("oD_");
        else if (section == "filter") match = id.startsWith ("f1_") || id.startsWith ("f2_") || id == "filter_routing";
        else if (section == "env")    match = id.startsWith ("e") && id.length() > 2 && id[2] == '_' && juce::CharacterFunctions::isDigit (id[1]);
        else if (section == "lfo")    match = id.startsWith ("l") && id.length() > 2 && id[2] == '_' && juce::CharacterFunctions::isDigit (id[1]);
        else if (section == "fx")     match = id.startsWith ("fx_") && ! id.startsWith ("fx_lim");
        else if (section == "macro")  match = id.startsWith ("macro");
        if (match)
            out.add (id);
    }
    return out;
}

} // namespace bgdj::params
