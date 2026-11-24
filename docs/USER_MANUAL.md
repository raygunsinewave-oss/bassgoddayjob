# BASSGODSDAYJOB - User Manual

*Created, designed & developed by Raymond Duarte (Raygun).*

BASSGODSDAYJOB is a bass synthesizer. It has four oscillators, two filters, a big modulation
system, a 13-module FX rack, a full metering suite, 430 factory basses, and a club full of
people who tell you, honestly, how your bass sounds.

---

## 1. The window

```
 BASSGODSDAYJOB  | < Preset Name > * | BROWSE | SAVE | A B A>B | CPU | MASTER
 [SYNTH] [FX] [VISUALIZER / METERING] [TWEAKS] [SETTINGS] [CREDITS]
```

- **< >** step through presets (inside the browser's current filter). Click the name to open the browser.
- **\*** marks the preset as a favourite.
- **SAVE** stores the current sound as a user preset with name, bank, character, genre, key, BPM and tags.
- **A / B / A>B** compare two versions of a sound. Pressing the other slot stores the current state in
  the active slot and recalls the other one. **A>B** copies the active slot to the other.
- **CPU** shows the audio-thread load. **MASTER** is the output volume (applied before the FX rack, so the final limiter catches it).

The window can be dragged to any size; everything is drawn as vectors. Set a default size with
**Settings > UI scale (50-200 %)**.

### Knobs

- Drag to change (circular, vertical or horizontal drag, see Settings). Hold the **fine-adjust
  modifier** (Shift by default) for precise moves.
- Hover to see the value. **Double-click** resets to default.
- **Right-click**: MIDI learn, clear MIDI mapping, reset, remove modulation, add modulation from any source.
- **Drop a modulation chip** (ENV, LFO, M1-M4, VEL, KEY...) onto a knob to modulate it. A coloured ring
  shows the depth. **Alt/Option-drag** the knob to change the newest routing's depth.

---

## 2. SYNTH tab

### Macros, Hype Meter and Crowd Mood
Four big macro knobs (rename them by double-clicking the label). Each has a chip you can drag onto
other knobs. The **Hype Meter** (left) and **Crowd Mood** readout (right) show how the club feels.

### OSC A / B / C (wavetable)
- **Table menu**: 97 factory bass tables (Sine, Saw, Square, Reese stacks, Growls, Vowels, Garage organs,
  Acid, FM Metal, Plucks, Rubber, Hoover, and many more), **Load user wavetable (WAV)**, or **Open wavetable editor**.
- **WT Pos** scans the frames. **Warp** + **Warp mode**: Sync, Bend +/-/+-, PWM, Asym +/-, Mirror, Quantize,
  FM, AM, RM. FM/AM/RM use the **Warp source** (Osc A/B/C, Sub, Noise).
- **Unison** 1-16, **Detune**, **Blend** (centre vs. side voices), **Width**, **U.Warp** (spreads warp across voices).
- **Oct / Semi / Fine / Pitch** (continuous, modulatable), **Phase**, **Rand** (random start phase), **Level**, **Pan**.
- **Route**: Filter 1, Filter 2, Both, or Direct (no filter).

Click the wavetable display to open the **wavetable editor**: draw a frame, edit 64 harmonics,
generate tables from a formula (`x` = phase 0..1, `y` = frame 0..1, e.g. `sin(tau*x + y*4*sin(tau*2*x))`),
add/remove frames, import/export WAV, then **Apply to Osc**. Imported WAVs can be single-cycle or
multi-frame (2048-sample frames, or the frame size stored in the file).

### OSC D (Sub / Noise)
- **Sub**: Sine, Triangle, Square, Rounded Saw, octave -2..0. **Sub Lock** keeps it mono and centred.
- **Noise**: 46 textures (vinyl, tape hiss, crowd, breath, metallic, rain, room tone...) plus WAVs from
  your noise folder. **Key Trk** follows the keyboard; **N.Pitch** transposes.

### Filters
Two filters, 32 models each: LP 12/24/36, HP 12/24, BP 12/24, Notch 12/24, Peak, All Pass, Low/High Shelf,
Ladder LP/HP, Acid Ladder 303, Diode LP, MS-20 LP/HP, Dirty LP 12/24, Dirty BP, Comb +/-, Flange Comb,
Formant Vowel, Formant Talker, Phaser 4/8/12, LP/HP Morph, Dual BP.
Controls: Cutoff, Reso, Drive, **Fat** (keeps the low end when resonance thins it; on LP/HP Morph and
Formant Talker it is the morph / formant shift), Key (key tracking), Mix, Pan.
Routing: **Serial** (F1 into F2), **Parallel**, **Split** (F1 on the left channel, F2 on the right).
For shelves, Resonance sets the gain (centre = 0 dB).

### Envelopes
ENV 1 is the amp envelope. Each has Attack, Decay, Sustain, Release and a **curve per segment**.

### LFOs
Eight LFOs with a **drawable shape**: drag points, double-click to add/remove, Alt-drag a segment to bend
it, Shift snaps to the grid, right-click for shapes. **Sync** to host tempo (8 bars to 1/64, dotted and
triplet), modes **Trigger** (restarts per note), **Envelope** (one-shot) and **Free** (locked to the host
grid). **Wobble...** opens the Wobble Builder (1/4 classic, 1/8 yoy, 1/8T, 1/16, 1/16T riddim and more).

### Mod matrix
64 slots: **Source**, **Aux** (multiplies the source), **Destination**, **Amount**, **Curve**, **Output**
and **polarity** (UNI/BI). Click cells to choose, drag the bars, double-click to reset, X to delete.
Collapse it with the arrow button. Destinations include every oscillator/filter/envelope/LFO control
plus global FX targets (distortion drive, filter FX cutoff, ducker depth, reverb/delay mix and more),
which follow the most recently played voice.

Sources: Env 1-4, LFO 1-8, Macro 1-4, Velocity, Note Random, Key Track, Mod Wheel, Pitch Bend,
Aftertouch, MPE Slide, MPE Pressure, Noise.

### Performance strip
Voice mode (**Mono / Legato / Poly**), voices, **Glide** time, glide mode (Always / Legato) and type
(constant time, constant rate, scaled), pitch-bend range (up to 24), **Bass Mono** + **Below** frequency,
mod wheel, pitch bend, velocity, and the on-screen keyboard.

---

## 3. FX tab

Twelve rack slots plus the fixed final **Limiter / Clipper**. Drag modules to reorder, click the dot to
switch a module on/off, click x to remove, **+ Add module** to add. Select a module to edit it.
Every module has **On**, **Mix** and **Module presets** (built-in ones, plus save/recall your own).

1. **Distortion**: Tube, Diode, Soft/Hard clip, Foldback, Bitcrush, Downsample, Rectify, Asym, Multiband (3 bands).
2. **OTT Multiband**: depth, time, upward/downward per band.
3. **Compressor**: VCA / Opto / Glue, with **external sidechain** from the host (enable the plugin's sidechain input).
4. **Ducker**: tempo-synced pump with a drawable shape (Kick Duck presets).
5. **EQ**: 6 bands, drag the numbered nodes over the live spectrum.
6. **Filter FX**: LP/HP/BP with a (synced) LFO.
7. **Chorus / Flanger / Phaser**.
8. **Delay**: Stereo, Ping-Pong, Tape (wow), HP/LP in the feedback path.
9. **Reverb**: Plate, Room, Hall, **Club** (matches the room on screen).
10. **Hyper / Dimension**: widener with a mono-below crossover.
11. **Saturator / Tape**: drive, warmth, wow, flutter, tone.
12. **Imager / Utility**: width, mono below, gain, pan, DC filter, phase invert L/R.
13. **Limiter / Clipper** (always last): ceiling, release, true-peak detection.

---

## 4. VISUALIZER / METERING tab

Drag the dividers to resize rows. Every panel has **Freeze** and **Detach** (opens it in its own window).

- **Spectrum**: FFT 1024-32768, slope 0/3/4.5 dB/oct, peak hold, pre-FX overlay, peak note readout.
- **Spectrogram** (scrolling waterfall), **Oscilloscope** (single-cycle trigger or 1/4 beat, 1 beat, 1 bar synced).
- **Goniometer + correlation**, **Loudness** (LUFS momentary / short-term / integrated, LRA, true peak, RMS, crest; Reset),
  **Levels** (peak + RMS, clip LEDs, resettable max), **Sub Check** (20-120 Hz meter + "not mono" warning),
  **Tuner**, **Creative** (circular spectrum, neon 3D terrain, sub particle field, VU needles, CRT scope),
  **Phase / Polarity + DC offset**.

---

## 5. TWEAKS tab

Oversampling per stage (synth, FX) from 1x to 16x and an offline-render override; wavetable
interpolation; anti-aliasing quality; unison detune curve (linear / exponential / supersaw) and phase
mode; voice stealing; envelope retrigger; legato envelope reset; per-oscillator **phase reset on note**
(keeps bass transients consistent); A4 tuning; **microtuning** (.scl / .tun); analog drift; velocity
curve; MPE zone, member channels and bend range; filter key-track reference note and saturation
algorithm; denormal flushing; DC blocker; **random seed lock** (makes random-phase presets render the
same every time); **Cat Chaos Mode**; **Crowd SFX to output** and its level.

---

## 6. SETTINGS tab

**Basic:** UI scale, theme (Neon Club / Dark / Light / High Contrast), club scene on/off, scene quality,
scene SFX, tooltips, knob behaviour, fine-adjust modifier, grain/CRT overlay, **DJ Name** (used in the
Credits poem; defaults to your OS user name), MIDI CC map save/load/clear.

**Paths:** default preset folder, extra preset folders, wavetable folder, noise folder, **Rescan**.

**Expansions:** install `.bgdjpack` files (button or drag onto the tab), enable/disable/remove. A pack
is a zip with `metadata.json` (`name`, `author`, `version`, `description`) and `presets/`,
`wavetables/`, `noise/` folders.

**Troubleshooting:** CPU monitor + xrun counter, reset UI layout and window size, force software
rendering (no OpenGL), **Safe Mode** (scene off, oversampling 1x, analyzers off), clear caches /
rebuild the preset database, reset all settings, **export a diagnostic log (.zip)** (OS, host, buffer
size, sample rate, GPU, version, settings, logs), check for updates (manual), auto-save settings,
crash-report opt-in (standalone app; written to the local log, never sent anywhere).

Settings are stored per user in:
- macOS: `~/Library/Application Support/BASSGODSDAYJOB/`
- Windows: `%APPDATA%\BASSGODSDAYJOB\`

User presets default to `Documents/BASSGODSDAYJOB/Presets`.

---

## 7. Presets

430 factory presets in 12 banks: SUB & 808 (50), ROLLING & REESE (45), UKG & BASSLINE (40),
TECH HOUSE RUBBER (50), GROWL & RIDDIM (45), HYBRID / COLOUR (30), EMOTIVE / WARM (35),
ORGANIC / TEXTURE (30), BREAKS & JUNGLE (30), ACID (30), HARDGROOVE / RUMBLE (25), FX & NEURO (20).

The **browser** has search (names, tags, macro labels), bank and character filters, favourites only,
minimum rating, **Similar To**, **Randomize** (whole patch or one section: oscillators, filters,
envelopes, LFOs, FX, macros, or a gentle 20 % nudge), import and rescan. Click the star to favourite,
click the dots to rate 1-5. Double-click or Enter loads.

---

## 8. The club

The scene sits behind the decks: your hands, two decks, the mixer and your laptop in front, the crowd
on a checkered floor, a bar with glowing bottles on the left, the exit on the right, balconies, arches,
orb lights and a disco ball above.

### How the crowd listens
Every audio block the synth measures loudness, peaks and clipping, sub (20-80 Hz), low-mid mud
(150-400 Hz), harshness (2-6 kHz), air (10 kHz+), spectral centroid and flux, stereo and sub-band
correlation, note activity, glide and bend use, macro movement, host tempo and transport. These go to
the **crowd mood** (hype, energy, annoyance, boredom, crowd size), which moves with inertia: hype
builds over seconds and fades more slowly.

What they love: a loud, punchy, sub-heavy, moving bass. A sub drop after silence. Filter sweeps landing
on the downbeat. Screaming resonance.
What they hate: muddy 150-400 Hz, harsh highs, clipping, an out-of-phase sub, one note held forever,
silence while the track plays.

### Reactions (60)
Hype: hands-up waves, jumps on the downbeat, "OOOOH", "ONE MORE!" chants, phone flashlights, shoulder
riders, mosh pits (growls), shufflers (tech-house rubber), two-steppers (UKG), crouch-and-explode,
confetti and CO2, hugs, backflips, the bartender dancing, security nodding, the lighting tech going full strobe.
DJ moments: rewinds, fist pumps, the air-horn pad, the "drop it" lean, a B2B high-five, a towel throw,
headphones-on-one-ear, nearly spilling your drink, "EXTENDED SET", a selfie-seeking fan escorted out,
a camera crew, lights cut to black until your next note.
Funny: bucket-hat confidence, a flying shoe, a conga line, the business suit, a couple who make up on
the drop, a balcony sleeper, a song request on paper, the disco ball slipping, a puddle slip, a pigeon,
a ringtone everybody turns around for, **Grey the Cat** sitting on your controller (and bumping a macro
if Cat Chaos Mode is on) or knocking your drink off while staring at you, the bouncer letting the queue in.
Negative: folded arms, covered ears, boos, "PLAY SOMETHING ELSE!", drifting to the bar, phones out,
pointing at the speaker, a (gentle) cup, the sound engineer turning you down, the promoter pointing at
the clock, people leaving, and the **full walkout** (house lights, a janitor, "SET OVER") until you play
something good again. One loyal fan never leaves.
Idle: chatting, a growing bar queue, someone waving at you to start, the smoke machine, Grey asleep on a speaker.

### Scene controls
Crowd personality (Warehouse, Festival, Underground Basement, Beach Bar, Afterhours), pause, expand to
the full window, PNG screenshot and a 10-second GIF clip (saved to `Pictures/BASSGODSDAYJOB`), and SFX on/off.
**Crowd SFX are never sent to the plugin output by default.** In a plugin there is nowhere else to play
them, so you only hear them if you enable **Tweaks > Crowd SFX to output** (with its own level).

---

## 9. CREDITS tab

Scrolling credits over a starfield, the build-generated third-party licence list, then the End Poem as
a dialogue between two voices (teal and light green) and centred narration. Your DJ Name replaces
PLAYERNAME. Speed 0.5x-3x, pause, skip, restart; click **Wake up.** to return to the synth.
*End Poem by Julian Gough. Released into the public domain under CC0.*

---

## 10. Troubleshooting

| Problem | Try |
|---|---|
| Graphics glitches / black window | Settings > Force software rendering, then reopen the plugin window |
| High CPU | Lower oversampling in Tweaks, reduce unison, set scene quality Low, or Safe Mode |
| Presets missing | Settings > Rescan content; check your preset folders |
| Clicks on note changes | Enable phase reset on note, or use Envelope retrigger "From Current Level" |
| Need support | Settings > Export diagnostic log (.zip) and send it along with your report |
