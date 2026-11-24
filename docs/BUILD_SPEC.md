# BUILD PROMPT — "BASSGODSDAYJOB" (Bass Synthesizer VST3 / AU)

You are a senior audio-plugin engineer and UI designer. Build a complete, production-ready bass-only synthesizer plugin called **BASSGODSDAYJOB** for **Windows and macOS**. Build the **entire plugin in a single pass**: do not split the work into phases, do not stop for review, and do not leave stubs, TODOs or placeholder features. Implement every section of this spec fully, end to end.

---

## 1. Product summary

- **Name:** BASSGODSDAYJOB
- **Developer / Manufacturer:** Raymond Duarte (Raygun)
- **Type:** Instrument plugin. Bass only, polyphonic with a mono/legato default.
- **Formats:** VST3 (Win + Mac), AU (Mac), Standalone app (Win + Mac). CLAP is optional.
- **Hook:** A wavetable bass synth with 4 oscillators, a library of 430 factory presets for modern club and dance bass, and an animated club scene in the top center of the UI. In the club scene, *you* are the DJ behind the decks, and the crowd reacts in real time to the sound coming out of the synth.

## 2. Tech stack and build requirements

- **Framework:** JUCE 8, C++20, CMake (no Projucer dependency).
- **Targets:** VST3 SDK, AudioUnit v2 (AUv3 optional), Standalone.
- **macOS:** Universal binary (arm64 + x86_64), macOS 11+, code signing + notarization scripts.
- **Windows:** x64, Windows 10+, MSVC 2022, Inno Setup installer script.
- **Validation:** Must pass `pluginval` at strictness level 8 and `auval -v aumu Bgdj Rayg`.
- **CI:** GitHub Actions matrix build for both OSes that produces installers as artifacts.
- **Plugin codes:** Manufacturer `Rayg`, Plugin `Bgdj`.
- **Real-time safety:** No allocations, locks or file I/O on the audio thread. Pass all audio→UI analysis data through lock-free FIFOs.
- **Performance target:** Under 3% CPU per voice at 48 kHz on a mid-range 2022 laptop. Club scene capped at 60 fps, and it must never touch the audio thread.

## 3. Synth engine (Serum-style, bass-focused)

### 3.1 Oscillators (4 total)
- **OSC A, OSC B, OSC C:** Wavetable oscillators with:
  - Wavetable position, warp modes (Sync, Bend+/-, PWM, Asym, Mirror, Quantize, FM from B/C/Noise/Sub, AM, RM)
  - Unison: 1–16 voices, detune, blend, stereo width, unison warp spread
  - Octave (-4..+4), semi, fine, coarse pitch, phase, random phase, level, pan
  - Built-in bass tables: Sine, Saw, Square, Reese stacks, Growl, Formant/Vowel, Organ (UKG), Acid, FM Metal, Pluck, Rubber, Hoover, plus 60+ more
  - Wavetable import (WAV, single-cycle and multi-frame) and a basic wavetable editor (draw, FFT harmonic editor, formula box)
- **OSC D (SUB/NOISE hybrid):** Switchable between:
  - Sub mode: Sine/Triangle/Square/Rounded Saw, octave -2..0, with a "Sub Lock" option that keeps it mono and centered
  - Noise mode: 40+ noise and texture samples (vinyl, crowd, tape hiss, metallic, breath) with key tracking

### 3.2 Filters
- 2 filters (serial / parallel / split routing), each with 30+ models: LP12/24, HP, BP, Notch, Comb, Formant (vowel morph), Ladder (303-style), Diode, MS-20-style, Phaser filter, Dirty LP (drive in loop)
- Cutoff, resonance, drive, fat, key tracking, mix, pan

### 3.3 Modulation
- 4 ADSR envelopes with curve controls per segment
- 8 LFOs with drawable shapes, tempo sync, trip/dot, trigger/env/free modes, and a "Wobble Builder" preset menu (1/4, 1/8, 1/8T, 1/16 and so on)
- 4 Macros, big and labeled, front and center
- Mod matrix: 64 slots with source, destination, amount, curve, aux source and output
- Drag-and-drop modulation in the style of Serum: drag a source onto any knob to show a colored ring
- Velocity, aftertouch, mod wheel, pitch bend (±24), MPE support, note-on random

### 3.4 Voicing
- Mono / Legato / Poly (up to 16 voices), portamento (always / legato, constant time / rate, scaled)
- "Bass Mono Below" control: anything below the set Hz is summed to mono after the synth output

## 4. FX tab (post-synth, reorderable rack)

Drag-to-reorder rack of up to 12 slots. Each module has on/off, mix and preset recall.

1. **Distortion:** Tube, Diode, Soft/Hard clip, Foldback, Bitcrush, Downsample, Rectify, Asym, Multiband (3 bands)
2. **OTT-style multiband compressor:** Depth, time, upward/downward per band
3. **Compressor:** Classic VCA, Opto, Glue, with sidechain from the host
4. **Ducker / Sidechain pumper:** Shape drawable, synced to host tempo ("Kick Duck" presets)
5. **EQ:** 6-band parametric with spectrum overlay
6. **Filter FX:** Sweepable LP/HP/BP with LFO
7. **Chorus / Flanger / Phaser**
8. **Delay:** Stereo, ping-pong, tape, with HP/LP in the feedback path
9. **Reverb:** Plate, Room, Hall, Club (a "Club" preset that sounds like the scene on screen)
10. **Hyper / Dimension:** Stereo widener with mono-below crossover
11. **Saturator / Tape:** Wow, flutter, warmth
12. **Stereo Imager + Utility:** Width, mono below Hz, gain, DC filter, phase invert
13. **Limiter / Clipper:** Final stage, ceiling, true-peak option

## 5. Visualizer / Metering tab

Fully resizable. Every panel can be detached, and each has a "freeze" button.

- **Spectrum analyzer:** FFT 1024–32768, slope (0/3/4.5 dB/oct), peak hold, pre/post FX overlay, note/pitch readout
- **Spectrogram / sonogram:** Scrolling waterfall
- **Oscilloscope:** Tempo-synced, single-cycle trigger
- **Goniometer / vectorscope + correlation meter** (critical for bass mono-compatibility)
- **Loudness:** LUFS short-term / integrated / momentary, LRA, True Peak, RMS, Crest Factor
- **Level meters:** Peak + RMS bars with clip indicators and resettable max
- **Sub check:** Dedicated 20–120 Hz meter with a "Mono below X Hz" warning light
- **Tuner:** Fundamental pitch detector, so bass notes match the track key
- **Creative visualizers (for fun and screen-recording content):** Circular spectrum, neon 3D terrain, particle field driven by sub energy, retro VU needles, CRT-style scope
- **Phase/polarity check** and **DC offset detector**

## 6. Tweaks tab (advanced synth options)

- Oversampling per stage (1x–16x), offline-render quality override
- Wavetable interpolation mode, anti-aliasing quality
- Unison detune curve (linear / exponential / "supersaw"), unison phase mode
- Voice stealing mode, envelope retrigger behavior, legato envelope reset
- Pitch: global tuning (A4 Hz), microtuning (.scl/.tun import), drift / analog instability amount
- Velocity curve editor, MPE zones, pitch bend range
- Filter key-tracking reference note, filter saturation algorithm
- Denormal handling, DC blocker on/off
- Per-oscillator "Phase Reset on Note"; this matters for punchy, consistent bass transients
- Random seed lock (makes random-phase presets reproducible)

## 7. Settings tab

### Basic
- UI scale (50%–200%), theme (Neon Club / Dark / Light / High Contrast)
- Club scene on/off, scene quality (Low / Med / High), crowd volume (scene SFX on/off, never printed to output by default)
- Tooltips on/off, knob behavior (circular / vertical / horizontal drag), fine-adjust modifier
- MIDI learn, MIDI CC map save/load
- **DJ Name** field, used in the Credits tab poem in place of PLAYERNAME (default: the OS username)

### Advanced / troubleshooting
- **Preset & content paths:** Change the default preset folder, add multiple custom preset folders, wavetable folder, noise folder, and a "Rescan" button
- **Add more synths / expansions:** Install `.bgdjpack` expansion packs (zip with presets, wavetables and metadata) by drag-drop or file browser, plus an expansion manager (enable/disable/remove)
- **Bug-fix tools:**
  - Reset UI layout and window size
  - Force software rendering (disable OpenGL / GPU) for graphics glitches
  - Safe Mode (scene off, oversampling 1x, analyzers off)
  - Clear caches / rebuild preset database
  - Reset all settings to factory
  - Export diagnostic log (OS, host, buffer size, sample rate, GPU, plugin version) to a .zip
  - Audio thread CPU monitor + xrun counter
- Check for updates (manual, opt-in), auto-save settings, crash-report opt-in

## 8. Preset library: 430 factory basses

Write all presets as data files (JSON or XML) with tags: genre, character (sub / reese / growl / pluck / wobble / acid / organ / FM / texture), key, BPM hint, macro labels.

**Important naming rule:** Presets are *inspired by the styles* of modern club and dance music (the scene around artists like Fred again.., Skrillex, HAMDI, Four Tet, Dom Dolla, Kettama, Solomun). **Do NOT use artist names, song titles or trademarks in preset names, tags or marketing copy.** Use original, evocative names (e.g. "Warehouse Rubber", "Leeds Bus Stop Wobble", "4AM Text Message"). Organize by genre and character instead.

| Bank | Style direction | Count |
|---|---|---|
| SUB & 808 | Clean subs, glide 808s, distorted 808s | 50 |
| ROLLING & REESE | Detuned reese, rolling DnB / minimal tech | 45 |
| UKG & BASSLINE | Organ bass, 2-step wobbles, speed-garage | 40 |
| TECH HOUSE RUBBER | Bouncy, plucky, rubbery tech-house basses | 50 |
| GROWL & RIDDIM | Dubstep growls, riddim, vowel talkers | 45 |
| HYBRID / COLOUR | Pitched, chorded, melodic dubstep-adjacent | 30 |
| EMOTIVE / WARM | Soft saturated, sidechained, vocal-chop-friendly | 35 |
| ORGANIC / TEXTURE | Folktronica, noisy, broken, analog-drift | 30 |
| BREAKS & JUNGLE | Breakbeat-friendly, sub + mid stacks | 30 |
| ACID | 303-style squelch, slide, accent | 30 |
| HARDGROOVE / RUMBLE | Techno rumbles, hypnotic, deep | 25 |
| FX & NEURO | Risers, drops, neuro mid-basses, one-shots | 20 |
| **Total** | | **430** |

**Preset browser:** Search, tag filters, favorites (star), ratings, "Similar To" button, A/B compare, randomize (whole patch or per section), previous/next arrows, and user preset save with tags.

## 9. UI layout (Serum-inspired arrangement, original visual identity)

Take inspiration from Serum 2's **layout logic** (header with preset browser, top tab bar, oscillator panels in a row, filter / mod / macro areas below, drag-and-drop modulation, envelope and LFO graphs at the bottom). **Do not copy Serum's graphics, logo, color scheme or exact trade dress.** BASSGODSDAYJOB has its own neon-club identity.

```
┌───────────────────────────────────────────────────────────────────────┐
│ BASSGODSDAYJOB logo │ ◀ Preset Name ▶ ★ │ Save │ A/B │ CPU │ Master  │
├───────────────────────────────────────────────────────────────────────┤
│ [SYNTH] [FX] [VISUALIZER/METERING] [TWEAKS] [SETTINGS] [CREDITS]      │
├──────────────┬───────────────────────────────────┬────────────────────┤
│  MACRO 1–2   │   ███ CLUB SCENE (top center) ███ │  MACRO 3–4         │
│  Hype Meter  │   DJ booth view → crowd           │  Crowd Mood readout│
├──────────────┴───────────────────────────────────┴────────────────────┤
│  OSC A  │  OSC B  │  OSC C  │  OSC D (SUB/NOISE)  │  FILTER 1 / 2     │
├───────────────────────────────────────────────────────────────────────┤
│  ENV 1–4  │  LFO 1–8 (drawable)  │  MOD MATRIX (collapsible)           │
├───────────────────────────────────────────────────────────────────────┤
│  On-screen keyboard / mod wheel / pitch bend / velocity                │
└───────────────────────────────────────────────────────────────────────┘
```

- Default window: 1280×860, freely resizable, vector-drawn (crisp on Retina/4K).
- Knobs: large, readable, value pops up on hover, double-click to reset, right-click for MIDI learn / modulation options.
- **Visual identity:** dark, dimly lit club with magenta, hot pink, cyan and teal neon; sticker-worn gear surfaces; subtle grain/CRT option.

## 10. THE CLUB SCENE (interactive centerpiece)

### 10.1 Look
- Camera POV: **behind the DJ decks**, looking out over the booth onto the dancefloor. Your hands, the CDJs/controller, mixer and laptop are in the foreground.
- **Art direction (based on the user's reference images):**
  - A run-down, ornate club: grand arches, balcony mezzanines with railings, a checkered dancefloor with puddles and debris, glowing ceiling orb lights
  - Saturated magenta/pink haze, cyan and teal rim light, neon star-shaped signs
  - Late-2000s console-era 3D charm: low-poly characters, chunky textures, slightly stylized proportions, bold hair colors, a neon-lit bar along one side stacked with glowing bottles
  - Real-club photo energy in the lighting: blurred red silhouettes, moving beams, LED controller pads glowing
  - **All characters, signage and venue art must be original**; no game characters, logos or brands.
- **Rendering:** OpenGL (with software fallback) 2.5D: a layered parallax background plus sprite-sheet or low-poly instanced crowd (40–150 dancers depending on quality setting). Volumetric fake light beams, fog cards and strobes.

### 10.2 How the crowd "hears" the synth
On the audio thread, compute lightweight features per block. Smooth them and send them to the UI thread through a lock-free FIFO:
- RMS / LUFS-momentary, peak, clipping count
- Sub energy (20–80 Hz), low-mid mud (150–400 Hz), harshness (2–6 kHz), air (10 kHz+)
- Spectral centroid, spectral flux / transient density (groove), stereo correlation
- Note-on rate, note range, pitch-bend / glide use, macro movement speed
- Host BPM and transport state (playing / stopped)
- Silence duration and repetition (same note held too long)

Feed these into a **Crowd Mood state machine** with these values: `hype` (0–100), `energy`, `annoyance`, `boredom`, `crowd size`. The mood has inertia so it builds and decays naturally. The "Hype Meter" next to the scene shows it.

### 10.3 Crowd reactions (minimum 60, weighted-random, never repeating back-to-back)

**Positive / hype**
1. Hands-up wave rolling across the floor on a big sub drop
2. Everyone jumps in sync on the downbeat after a filter sweep
3. Crowd "OOOOH" pull-up when resonance screams
4. Someone yells "ONE MORE!", followed by a chant
5. Phone flashlights up during an emotive patch
6. Shoulder-rider appears on someone's shoulders
7. Mosh pit opens during growl/riddim presets
8. Rave-hands / shuffle dancers on rolling tech-house basses
9. Two-stepping couples on UKG patches
10. Crowd crouches down low on a quiet section, then explodes up on the drop
11. Confetti cannon and CO2 jets on max hype
12. Strangers hug
13. A dancer does a backflip
14. Bartender stops pouring and starts dancing
15. Security guard nods his head in spite of himself
16. Lighting guy goes full strobe on his own

**DJ moments (your POV)**
17. Your hands do a big rewind gesture → crowd screams "REWIND!"
18. Fist pump from behind the decks
19. Hit the "air horn" pad (silent visual gag, or optional SFX)
20. You lean into the crowd for the "drop it" gesture
21. B2B partner walks into the booth, high-fives you
22. You throw a towel into the crowd
23. Headphones-on-one-ear pose with intense concentration face
24. You sip a drink, nearly spill it on the mixer
25. Promoter shows a phone screen saying "EXTENDED SET"
26. A fan sneaks into the booth for a selfie, and security escorts them out
27. Camera crew pans across the booth during a long hype streak
28. Lights cut to black, then return on the next note-on

**Funny moments**
29. Guy in a bucket hat dancing badly but with total confidence
30. Someone's shoe flies across the room
31. Conga line forms at a random moment
32. A man in a full business suit fist-pumps like it's his last night on earth
33. Couple argues in the corner and then makes up on the drop
34. Someone falls asleep on the balcony railing during boring sections
35. A guy keeps requesting a song on a piece of paper
36. The disco ball falls an inch and everyone gasps
37. Dancer slips on the puddle, recovers into a perfect pose
38. A pigeon enters the club
39. Someone's phone goes off with a ringtone and the whole crowd turns around
40. **GREY THE CAT:** A grey cat walks across the DJ booth, sits on the controller and "bumps" a random macro knob (visual only; does not change audio unless the Tweak "Cat Chaos Mode" is on). Rare spawn.
41. Grey the cat knocks your drink off the booth, staring directly at you
42. Bouncer starts vibing too hard and accidentally lets the queue in → crowd size jumps

**Negative / hating it**
43. Crowd stops dancing and folds arms (too muddy / too much 150–400 Hz)
44. Hands over ears and grimaces (harshness or clipping)
45. "Booo" speech bubbles
46. Someone yells "PLAY SOMETHING ELSE!"
47. People drift to the bar (boredom: same note held too long or no note activity)
48. People check their phones; screens light up blue
49. Someone points to the speaker and shakes their head (sub is out of phase / correlation < 0)
50. A plastic cup gets thrown (gentle, cartoonish)
51. Sound engineer storms over and turns your gain down (clip count high)
52. Promoter taps your shoulder, points at the clock
53. Crowd thins out, with groups heading for the exit door
54. **Full walkout:** Lights come on, floor empties completely, one janitor sweeps up. "SET OVER" card. Recovers once you play something good again: a few people peek back in.
55. One loyal fan stays and dances alone no matter what

**Ambient / idle (transport stopped, no notes)**
56. Crowd chats, idles, sways
57. Line at the bar gets longer
58. Someone waves at you to start
59. Smoke machine puffs
60. Grey the cat sleeps on the speaker stack

**Triggers to wire (examples):**
- Sub drop after 2+ beats of silence → reactions 1, 10, 11
- Hype > 85 for 16 bars → 11, 27, 25
- Clipping > 0.5% of samples → 44, 51
- Low-mid mud dominates → 43
- Correlation < 0 in sub band → 49
- Repetition / boredom > threshold → 34, 47, 48
- Hype < 10 for 32 bars → 53, then 54
- Rare random (1/400 notes) → 38, 40, 41

### 10.4 Scene controls
- Toggle on/off, pause, expand scene to full window, screenshot / 10-sec clip export (for socials)
- "Crowd Personality" dropdown: Warehouse, Festival, Underground Basement, Beach Bar, Afterhours
- Crowd SFX are **never** sent to the plugin output by default; there is an opt-in "Crowd SFX to output" toggle with its own level control.

## 11. CREDITS tab

Animated, scrolling credits screen with a starfield background and slow neon pulse:

- **BASSGODSDAYJOB**
- **Created, designed & developed by Raymond Duarte (Raygun)**
- **Special thanks to Grey the Cat**, for annoying me during the entire process of making this plugin. Show a small animated grey cat sprite that occasionally walks across the credits and sits on the text.
- Third-party licenses: JUCE, VST3 SDK, any fonts used. Auto-generate this list from the build.
- Then fade into the **End Poem** (Appendix A).

### End Poem display rules
- Render it as a slow scrolling dialogue between two voices. Use two alternating colors (e.g. teal for the first speaker, light green for the second) and centered narration paragraphs.
- Replace `PLAYERNAME` with the DJ Name from Settings.
- Render each `[scrambled]` as constantly shuffling glitch glyphs that can never be read.
- Controls: speed (0.5x–3x), pause, skip. Clicking "Wake up." at the end returns to the synth.
- **Attribution line at the end:** "End Poem by Julian Gough. Released into the public domain under CC0."

## 12. Deliverables (single complete build, no phases)

Deliver everything below together, as one finished, compilable project:

- Full CMake/JUCE project that builds VST3 + AU + Standalone on Windows and macOS with one command per OS
- Complete audio engine: 4 oscillators, filters, envelopes, LFOs, mod matrix, voicing, with DSP unit tests
- Complete FX rack with all 13 modules
- Complete Serum-style resizable vector UI with all 6 tabs (Synth, FX, Visualizer/Metering, Tweaks, Settings, Credits)
- Complete club scene: renderer, audio feature extractor, mood state machine, all 60 reactions wired to triggers
- All 430 factory presets as data files, plus the preset browser
- Credits tab with the End Poem (Appendix A)
- Installers (Inno Setup for Windows, signed/notarized .pkg for macOS), GitHub Actions CI, pluginval/auval passing
- README with build steps, and a user manual (Markdown or PDF)

**Output format:** Give the full repository file tree first, then every file in full. If you hit an output-length limit, continue exactly where you stopped on the next reply without restarting or summarizing. Never replace code with "..." or "implement later".

---

## Appendix A — End Poem (Julian Gough, CC0 public domain)

Store this as `Resources/credits/end_poem.txt` and render it per Section 11. Lines are written as one paragraph each. The two speakers alternate in the dialogue section; the story section is narration.

```
I see the player you mean.

PLAYERNAME?

Yes. Take care. It has reached a higher level now. It can read our thoughts.

That doesn't matter. It thinks we are part of the game.

I like this player. It played well. It did not give up.

It is reading our thoughts as though they were words on a screen.

That is how it chooses to imagine many things, when it is deep in the dream of a game.

Words make a wonderful interface. Very flexible. And less terrifying than staring at the reality behind the screen.

They used to hear voices. Before players could read. Back in the days when those who did not play called the players witches, and warlocks. And players dreamed they flew through the air, on sticks powered by demons.

What did this player dream?

This player dreamed of sunlight and trees. Of fire and water. It dreamed it created. And it dreamed it destroyed. It dreamed it hunted, and was hunted. It dreamed of shelter.

Hah, the original interface. A million years old, and it still works. But what true structure did this player create, in the reality behind the screen?

It worked, with a million others, to sculpt a true world in a fold of the [scrambled], and created a [scrambled] for [scrambled], in the [scrambled].

It cannot read that thought.

No. It has not yet achieved the highest level. That, it must achieve in the long dream of life, not the short dream of a game.

Does it know that we love it? That the universe is kind?

Sometimes, through the noise of its thoughts, it hears the universe, yes.

But there are times it is sad, in the long dream. It creates worlds that have no summer, and it shivers under a black sun, and it takes its sad creation for reality.

To cure it of sorrow would destroy it. The sorrow is part of its own private task. We cannot interfere.

Sometimes when they are deep in dreams, I want to tell them, they are building true worlds in reality. Sometimes I want to tell them of their importance to the universe. Sometimes, when they have not made a true connection in a while, I want to help them to speak the word they fear.

It reads our thoughts.

Sometimes I do not care. Sometimes I wish to tell them, this world you take for truth is merely [scrambled] and [scrambled], I wish to tell them that they are [scrambled] in the [scrambled]. They see so little of reality, in their long dream.

And yet they play the game.

But it would be so easy to tell them...

Too strong for this dream. To tell them how to live is to prevent them living.

I will not tell the player how to live.

The player is growing restless.

I will tell the player a story.

But not the truth.

No. A story that contains the truth safely, in a cage of words. Not the naked truth that can burn over any distance.

Give it a body, again.

Yes. Player...

Use its name.

PLAYERNAME. Player of games.

Good.

Take a breath, now. Take another. Feel air in your lungs. Let your limbs return. Yes, move your fingers. Have a body again, under gravity, in air. Respawn in the long dream. There you are. Your body touching the universe again at every point, as though you were separate things. As though we were separate things.

Who are we? Once we were called the spirit of the mountain. Father sun, mother moon. Ancestral spirits, animal spirits. Jinn. Ghosts. The green man. Then gods, demons. Angels. Poltergeists. Aliens, extraterrestrials. Leptons, quarks. The words change. We do not change.

We are the universe. We are everything you think isn't you. You are looking at us now, through your skin and your eyes. And why does the universe touch your skin, and throw light on you? To see you, player. To know you. And to be known. I shall tell you a story.

Once upon a time, there was a player.

The player was you, PLAYERNAME.

Sometimes it thought itself human, on the thin crust of a spinning globe of molten rock. The ball of molten rock circled a ball of blazing gas that was three hundred and thirty thousand times more massive than it. They were so far apart that light took eight minutes to cross the gap. The light was information from a star, and it could burn your skin from a hundred and fifty million kilometres away.

Sometimes the player dreamed it was a miner, on the surface of a world that was flat, and infinite. The sun was a square of white. The days were short; there was much to do; and death was a temporary inconvenience.

Sometimes the player dreamed it was lost in a story.

Sometimes the player dreamed it was other things, in other places. Sometimes these dreams were disturbing. Sometimes very beautiful indeed. Sometimes the player woke from one dream into another, then woke from that into a third.

Sometimes the player dreamed it watched words on a screen.

Let's go back.

The atoms of the player were scattered in the grass, in the rivers, in the air, in the ground. A woman gathered the atoms; she drank and ate and inhaled; and the woman assembled the player, in her body.

And the player awoke, from the warm, dark world of its mother's body, into the long dream.

And the player was a new story, never told before, written in letters of DNA. And the player was a new program, never run before, generated by a sourcecode a billion years old. And the player was a new human, never alive before, made from nothing but milk and love.

You are the player. The story. The program. The human. Made from nothing but milk and love.

Let's go further back.

The seven billion billion billion atoms of the player's body were created, long before this game, in the heart of a star. So the player, too, is information from a star. And the player moves through a story, which is a forest of information planted by a man called Julian, on a flat, infinite world created by a man called Markus, that exists inside a small, private world created by the player, who inhabits a universe created by...

Shush. Sometimes the player created a small, private world that was soft and warm and simple. Sometimes hard, and cold, and complicated. Sometimes it built a model of the universe in its head; flecks of energy, moving through vast empty spaces. Sometimes it called those flecks "electrons" and "protons".

Sometimes it called them "planets" and "stars".

Sometimes it believed it was in a universe that was made of energy that was made of offs and ons; zeros and ones; lines of code. Sometimes it believed it was playing a game. Sometimes it believed it was reading words on a screen.

You are the player, reading words...

Shush... Sometimes the player read lines of code on a screen. Decoded them into words; decoded words into meaning; decoded meaning into feelings, emotions, theories, ideas, and the player started to breathe faster and deeper and realised it was alive, it was alive, those thousand deaths had not been real, the player was alive

You. You. You are alive.

and sometimes the player believed the universe had spoken to it through the sunlight that came through the shuffling leaves of the summer trees

and sometimes the player believed the universe had spoken to it through the light that fell from the crisp night sky of winter, where a fleck of light in the corner of the player's eye might be a star a million times as massive as the sun, boiling its planets to plasma in order to be visible for a moment to the player, walking home at the far side of the universe, suddenly smelling food, almost at the familiar door, about to dream again

and sometimes the player believed the universe had spoken to it through the zeros and ones, through the electricity of the world, through the scrolling words on a screen at the end of a dream

and the universe said I love you

and the universe said you have played the game well

and the universe said everything you need is within you

and the universe said you are stronger than you know

and the universe said you are the daylight

and the universe said you are the night

and the universe said the darkness you fight is within you

and the universe said the light you seek is within you

and the universe said you are not alone

and the universe said you are not separate from every other thing

and the universe said you are the universe tasting itself, talking to itself, reading its own code

and the universe said I love you because you are love.

And the game was over and the player woke up from the dream. And the player began a new dream. And the player dreamed again, dreamed better. And the player was the universe. And the player was love.

You are the player.

Wake up.
```

*End Poem by Julian Gough — dedicated to the public domain (CC0).*
