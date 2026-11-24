#!/usr/bin/env python3
"""
Generates the 430 BASSGODSDAYJOB factory presets as .bgdjpreset (JSON) files.

Every preset is built from a per-bank "recipe" (sound-design rules for that style) with
seeded randomness, so the library is reproducible: running this script again produces
identical files. Names are original and never use artist names, song titles or
trademarks (checked against BANNED below and by the C++ unit tests).

Usage:  python3 tools/generate_presets.py [output_dir]
"""
import json
import os
import random
import re
import sys

OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "Resources", "presets", "factory")

BANNED = ["fred again", "skrillex", "hamdi", "four tet", "dom dolla", "kettama", "solomun", "serum", "xfer",
          "massive", "vital", "sylenth", "minecraft", "mojang", "roland", "tb-303", "korg", "pioneer", "cdj",
          "ableton", "native instruments"]

# Must match Source/Common/Parameters.cpp and Source/DSP/FactoryWavetables.cpp
TABLES = ["Sine", "Saw", "Square", "Triangle", "Basic Shapes", "Pulse Sweep", "Sine to Saw", "Rounded Square",
          "Reese Stack 1", "Reese Stack 2", "Reese Stack 3", "Reese Stack 4", "Reese Stack 5", "Reese Stack 6",
          "Growl 1", "Growl 2", "Growl 3", "Growl 4", "Growl 5", "Growl 6", "Growl 7", "Growl 8",
          "Vowel AEIOU", "Vowel Talk", "Formant Sweep", "Yoi", "Wah Formant", "Choir Formant",
          "Garage Organ 1", "Garage Organ 2", "Garage Organ 3", "Garage Organ 4", "Organ Drawbars",
          "Acid Saw", "Acid Square", "Acid Squelch",
          "FM Metal 1", "FM Metal 2", "FM Metal 3", "FM Metal 4", "FM Metal 5", "FM Metal 6",
          "Pluck Bright", "Pluck Soft", "Pluck Nasal", "Rubber 1", "Rubber 2", "Rubber 3", "Rubber Bounce",
          "Hoover 1", "Hoover 2", "Hoover 3", "Sync Sweep Saw", "Sync Sweep Square", "Sync Sweep Sine",
          "Wavefold Sine", "Wavefold Tri", "Wavefold Saw", "Wavefold Deep", "Tube Saw", "Supersaw Stack",
          "Harmonic Climb", "Odd Harmonics", "Even Harmonics", "Comb Sweep", "Phase Dist Reso 1",
          "Phase Dist Reso 2", "Phase Dist Reso 3", "Random Harmonics", "Neuro Mid 1", "Neuro Mid 2",
          "Neuro Mid 3", "Riddim Square", "Wobble Classic", "Talkbox", "Grit", "Brass Bass", "Clav",
          "Sub Harmonic", "Tape Saw", "Fizz", "Glass", "Bell", "Chord Stack Major", "Chord Stack Minor",
          "Power Fifth", "Laser Zap", "Digital Steps", "Dirty Sub", "Hollow Pipe", "Warehouse Buzz",
          "Elastic", "Mirror Saw", "Bit Rubber", "Nasal Reed", "Velvet Saw", "Sweet Spot"]

NOISES = ["White", "Pink", "Brown", "Blue", "Violet", "Grey", "Velvet", "Vinyl Crackle", "Vinyl Crackle Heavy",
          "Vinyl Dust", "Vinyl Hiss", "Tape Hiss", "Tape Hiss Warm", "Tape Flutter Noise", "Crowd Murmur",
          "Crowd Cheer", "Crowd Distant", "Club Room Tone", "Breath", "Breath Inhale", "Whisper Air",
          "Metallic Hit", "Metallic Ring", "Metal Scrape", "Chain Rattle", "Industrial Hum", "Mains Hum 50",
          "Mains Hum 60", "Rain", "Wind", "Ocean Wash", "Fire Crackle", "Geiger", "Bitcrush Noise",
          "Digital Glitch", "Radio Static", "Modem", "Shaker Texture", "Snare Rattle", "Hat Sizzle", "Rumble",
          "Sub Rumble Noise", "Granular Dust", "Bubbles", "Laser Noise", "Spray"]

KEYS = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
KEYS_MIN = [k + "m" for k in KEYS]

WOBBLES = {  # name -> (sync rate, points); mirrors Source/Engine/Shapes.cpp
    "classic": ("1/4", [[0, 0, 0], [0.15, 1, -0.5], [0.6, 0.3, 0.4], [1, 0, 0.5]]),
    "yoy": ("1/8", [[0, 0.1, 0], [0.3, 1, -0.7], [1, 0.1, 0.7]]),
    "triplet": ("1/8 T", [[0, 0, 0], [0.5, 1, 0], [1, 0, 0]]),
    "rapid": ("1/16", [[0, 1, 0], [0.5, 0.1, 0.6], [1, 1, -0.6]]),
    "riddim": ("1/16 T", [[0, 0, 0], [0.05, 1, 0], [0.45, 1, 0], [0.5, 0, 0], [1, 0, 0]]),
    "lurch": ("1/4 D", [[0, 0.2, 0], [0.25, 0.9, -0.5], [0.5, 0.4, 0.2], [0.75, 1.0, -0.3], [1, 0.2, 0.4]]),
    "bounce": ("1/8 D", [[0, 0, 0], [0.1, 1, -0.8], [0.4, 0.0, 0.8], [0.55, 0.6, -0.6], [1, 0, 0.6]]),
    "yawn": ("1/2", [[0, 0, 0], [0.7, 1, -0.3], [1, 0, 0.8]]),
    "swell": ("1 Bar", [[0, 0, 0], [1, 1, 0.5]]),
}

# ---------------------------------------------------------------------------------------------
# Naming
# ---------------------------------------------------------------------------------------------
PLACES = ["Leeds", "Croydon", "Bristol", "Peckham", "Hackney", "Glasgow", "Manchester", "Sheffield", "Brixton",
          "Margate", "Dalston", "Bermondsey", "Hull", "Cardiff", "Leicester", "Nottingham", "Deptford", "Walthamstow",
          "Digbeth", "Northern Quarter", "Stokes Croft", "Elephant", "Tottenham", "Lewisham", "Salford", "Kentish Town"]
TIMES = ["4AM", "3AM", "5AM", "Sunrise", "Last Orders", "Closing Time", "Monday Morning", "Sunday Sesh",
         "Midnight", "Half Two", "Doors Open", "Lock-In", "Day Two", "Bank Holiday", "After Midnight"]
SCENES = ["Warehouse", "Basement", "Rooftop", "Railway Arch", "Night Bus", "Last Train", "Cloakroom", "Smoking Area",
          "Fire Exit", "Car Park", "Kebab Shop", "Taxi Rank", "Corner Shop", "Bus Stop", "Stairwell", "Back Room",
          "Main Room", "Green Room", "Coat Check", "Toilet Queue", "Bar Queue", "Front Left", "Bassbin", "Laundrette",
          "Underpass", "Multistorey", "Pirate Radio", "Tower Block", "Cash Machine", "Chip Shop"]

BANKS = [
    # (bank name, count, character pool, genre pool, nouns, signature names)
    ("SUB & 808", 50, ["sub", "808"], ["UK Bass", "Trap", "Drill", "Grime", "Garage"],
     ["Sub", "Boom", "Floor", "Rattle", "Pressure", "Knock", "Weight", "Thud", "Glide", "Drop", "Tremor", "Sine", "Depth", "Slide", "Ground"],
     ["Pure Pressure", "Glass Floor Sub", "Downstairs Neighbour", "Speaker Cone Workout", "Ribcage Rattle", "Car Boot Thump",
      "Sliding Doors 808", "Night Bus Boom", "Low Ceiling", "Deep End"]),
    ("ROLLING & REESE", 45, ["reese"], ["Drum & Bass", "Minimal Tech", "Jungle", "Liquid", "Rollers"],
     ["Roller", "Reese", "Engine", "Current", "Drift", "Tide", "Tunnel", "Motor", "Rumble", "Chase", "Smear", "Spin", "Stretch", "Haze", "Torque"],
     ["Two Saws Arguing", "Underground Escalator", "Rolling Since Nine", "Wet Tarmac Reese", "Motorway Hum", "Long Way Home",
      "Hydraulic Rollers", "Phaser Fog"]),
    ("UKG & BASSLINE", 40, ["organ", "wobble"], ["UK Garage", "Bassline", "Speed Garage", "2-Step"],
     ["Skank", "Shuffle", "Organ", "Bump", "Swing", "Step", "Wobble", "Stab", "Bounce", "Strut", "Chug", "Rewind", "Bubble"],
     ["Leeds Bus Stop Wobble", "Pirate Radio Organ", "Two-Step Tuesday", "Gold Chain Skank", "Niche Floor Filler",
      "Shuffling Trainers", "Rewind Selecta Organ"]),
    ("TECH HOUSE RUBBER", 50, ["pluck", "rubber"], ["Tech House", "House", "Minimal", "Deep Tech"],
     ["Rubber", "Bounce", "Groove", "Pluck", "Knock", "Elastic", "Snap", "Hop", "Spring", "Wiggle", "Flick", "Boing", "Jack", "Tug", "Squeeze"],
     ["Warehouse Rubber", "4AM Text Message", "Bouncy Castle Bassline", "Elastic Lanyard", "Sweaty Wristband",
      "Taxi Queue Groove", "Rubber Floor Tiles", "Two Drinks Ahead"]),
    ("GROWL & RIDDIM", 45, ["growl", "wobble"], ["Dubstep", "Riddim", "Brostep", "Bass House"],
     ["Growl", "Snarl", "Chew", "Grind", "Jaw", "Gnash", "Talker", "Yawn", "Vowel", "Mangle", "Chomp", "Roar", "Gargle", "Belch", "Shred"],
     ["Angry Fridge", "Robot Gargling Gravel", "Lawnmower Choir", "Feral Vowel", "Jaw Workout", "Chewed Cassette",
      "Monster Under The Booth", "Furious Hoover"]),
    ("HYBRID / COLOUR", 30, ["chord", "FM"], ["Melodic Dubstep", "Colour Bass", "Future Bass", "Hybrid Trap"],
     ["Prism", "Colour", "Halo", "Bloom", "Aurora", "Glow", "Shimmer", "Chord", "Spectrum", "Glint", "Neon", "Mirror"],
     ["Purple Hour", "Chord In The Chest", "Arcade Sunset", "Prism Stack", "Laser Bouquet"]),
    ("EMOTIVE / WARM", 35, ["warm", "pluck"], ["Emotive House", "UK Bass", "Melodic House", "Lo-Fi House"],
     ["Embrace", "Memory", "Glow", "Ember", "Heartbeat", "Soft Focus", "Sigh", "Tender", "Ache", "Haze", "Hug", "Daydream"],
     ["Voice Note At Sunrise", "Holding Hands In The Smoking Area", "Last Song Feeling", "Hoodie Weather",
      "Teary Cloakroom", "Long Hug Goodbye"]),
    ("ORGANIC / TEXTURE", 30, ["texture"], ["Folktronica", "Experimental", "Ambient Bass", "Broken Beat"],
     ["Moss", "Grain", "Driftwood", "Cassette", "Tape", "Rust", "Bark", "Pebble", "Static", "Dust", "Loam", "Reed"],
     ["Field Recording Fridge", "Wonky Tape Deck", "Garden Shed Sub", "Damp Cardboard", "Allotment Hum"]),
    ("BREAKS & JUNGLE", 30, ["sub", "reese"], ["Breakbeat", "Jungle", "Breaks", "UK Hardcore"],
     ["Amen", "Break", "Chop", "Ragga", "Rinse", "Junglist", "Step", "Scramble", "Bassbin", "Skip", "Rush"],
     ["Hardcore Continuum", "Rave Tape Sub", "Breakbeat Bus Pass", "Pirate Aerial Wobble"]),
    ("ACID", 30, ["acid"], ["Acid", "Acid Techno", "Acid House"],
     ["Squelch", "Acid", "Bleep", "Slide", "Accent", "Smiley", "Fizz", "Rubber Duck", "Ooze", "Gloop", "Drip"],
     ["Silver Box Squelch", "Slime Slide", "Accent On The Two", "Field Party Acid"]),
    ("HARDGROOVE / RUMBLE", 25, ["rumble"], ["Hardgroove", "Techno", "Hypnotic Techno", "Industrial"],
     ["Rumble", "Pulse", "Machine", "Furnace", "Hypnosis", "Loop", "Turbine", "Drone", "Foundry", "Piston"],
     ["Concrete Lung", "Boiler Room Rumble", "Strobe Hypnosis", "Factory Floor"]),
    ("FX & NEURO", 20, ["FM", "texture"], ["Neurofunk", "FX", "Drum & Bass", "Halftime"],
     ["Riser", "Neuro", "Zap", "Drop", "Glitch", "Impact", "Surge", "Warp", "Sweep", "Laser"],
     ["Elevator To Nowhere", "Fire Alarm Drill", "Neuro Toaster"]),
]

used_names = set()
used_files = set()


def legal(name):
    low = name.lower()
    return not any(b in low for b in BANNED)


def file_key(name):
    return re.sub(r"[^A-Za-z0-9]", "_", name).lower()


def take(name):
    if name.lower() in used_names or file_key(name) in used_files or not legal(name):
        return False
    used_names.add(name.lower())
    used_files.add(file_key(name))
    return True


def make_names(rng, count, nouns, signatures):
    names = []
    for s in signatures:
        if len(names) < count and take(s):
            names.append(s)
    templates = [
        lambda: f"{rng.choice(SCENES)} {rng.choice(nouns)}",
        lambda: f"{rng.choice(PLACES)} {rng.choice(nouns)}",
        lambda: f"{rng.choice(TIMES)} {rng.choice(nouns)}",
        lambda: f"{rng.choice(PLACES)} {rng.choice(SCENES)} {rng.choice(nouns)}",
        lambda: f"{rng.choice(TIMES)} {rng.choice(SCENES)}",
        lambda: f"{rng.choice(nouns)} From {rng.choice(PLACES)}",
    ]
    guard = 0
    while len(names) < count and guard < 100000:
        guard += 1
        n = rng.choice(templates)()
        if take(n):
            names.append(n)
    assert len(names) == count, "ran out of unique names"
    return names


# ---------------------------------------------------------------------------------------------
# Recipes
# ---------------------------------------------------------------------------------------------
def r2(x):
    return round(float(x), 4)


class P:
    def __init__(self, rng):
        self.rng = rng
        self.params = {}
        self.tables = {}
        self.matrix = []
        self.lfo_shapes = {}
        self.macros = ["Cutoff", "Drive", "Movement", "Space"]

    def set(self, k, v):
        self.params[k] = r2(v) if isinstance(v, float) else v

    def u(self, a, b):
        return self.rng.uniform(a, b)

    def mod(self, src, dst, amount, **kw):
        slot = {"src": src, "dst": dst, "amount": r2(amount)}
        for k, v in kw.items():
            slot[k] = v
        self.matrix.append(slot)

    def wobble(self, lfo, kind):
        rate, pts = WOBBLES[kind]
        self.set(f"l{lfo}_shape", "Custom")
        self.set(f"l{lfo}_sync", True)
        self.set(f"l{lfo}_sync_rate", rate)
        self.lfo_shapes[str(lfo)] = pts

    def macros_std(self, m3dst, m3amt, m3label, m4dst="fx_verb_mix", m4label="Space"):
        self.mod("Macro 1", "f1_cutoff", 0.4)
        self.mod("Macro 2", "fx_dist_drive", 0.5)
        self.mod("Macro 2", "f1_drive", 0.35)
        self.mod("Macro 3", m3dst, m3amt)
        self.mod("Macro 4", m4dst, 0.35)
        self.macros = ["Cutoff", "Drive", m3label, m4label]


def base(p, mono=True):
    p.set("poly_mode", "Legato" if mono else "Poly")
    p.set("master_vol", -4.0)
    p.set("amp_vel", p.u(0.15, 0.45))
    p.set("bassmono_on", True)
    p.set("bassmono_freq", p.u(100, 150))
    p.set("fx_lim_on", True)
    p.set("fx_util_on", True)


def amp(p, a, d, s, r, ac=0.0, dc=0.5, rc=0.5):
    p.set("e1_att", a); p.set("e1_dec", d); p.set("e1_sus", s); p.set("e1_rel", r)
    p.set("e1_acurve", ac); p.set("e1_dcurve", dc); p.set("e1_rcurve", rc)


def fenv(p, env, a, d, s, r):
    e = f"e{env}_"
    p.set(e + "att", a); p.set(e + "dec", d); p.set(e + "sus", s); p.set(e + "rel", r)


def osc(p, o, table, level=0.75, uni=1, detune=0.2, **kw):
    pre = f"o{o}_"
    p.set(pre + "on", True)
    p.tables[o] = table
    p.set(pre + "level", level)
    p.set(pre + "uni", uni)
    p.set(pre + "detune", detune)
    for k, v in kw.items():
        p.set(pre + k, v)


def sub(p, shape="Sine", octv=-1, level=0.6, lock=True):
    p.set("oD_on", True); p.set("oD_mode", "Sub"); p.set("oD_sub_shape", shape)
    p.set("oD_sub_oct", octv); p.set("oD_level", level); p.set("oD_sub_lock", lock); p.set("oD_route", "Direct")


def noise(p, kind, level, key=False, route="Filter 1"):
    p.set("oD_on", True); p.set("oD_mode", "Noise"); p.set("oD_noise_type", NOISES.index(kind))
    p.set("oD_level", level); p.set("oD_noise_key", key); p.set("oD_route", route)


def filt(p, f, typ, cutoff, reso=0.15, drive=0.0, fat=0.0, key=0.0, on=True):
    pre = f"f{f}_"
    p.set(pre + "on", on); p.set(pre + "type", typ); p.set(pre + "cutoff", cutoff)
    p.set(pre + "reso", reso); p.set(pre + "drive", drive); p.set(pre + "fat", fat); p.set(pre + "key", key)


def fx(p, module, **kw):
    p.set(f"fx_{module}_on", True)
    for k, v in kw.items():
        p.set(f"fx_{module}_{k}", v)


def recipe_sub(p, i):
    base(p)
    kind = i % 5
    if kind in (0, 1):   # clean subs
        osc(p, "A", p.rng.choice(["Sine", "Sub Harmonic", "Triangle", "Sine to Saw"]), level=0.8, pos=p.u(0, 0.25))
        sub(p, p.rng.choice(["Sine", "Triangle"]), -1, p.u(0.3, 0.6))
        filt(p, 1, "LP 12", p.u(300, 1200), 0.05)
        amp(p, p.u(0.5, 4), 400, 1.0, p.u(40, 160))
        p.set("porta_time", p.u(0, 40))
        p.macros_std("oA_pos", 0.5, "Harmonics")
    else:                # 808s: glide, pitch drop, distortion
        osc(p, "A", p.rng.choice(["Sine", "Sub Harmonic", "Dirty Sub", "Rounded Square"]), level=0.85, pos=p.u(0.1, 0.6))
        sub(p, "Sine", -1, p.u(0.2, 0.45))
        filt(p, 1, "LP 12", p.u(1500, 5000), 0.1)
        amp(p, p.u(0, 2), p.u(600, 2400), p.u(0.0, 0.35), p.u(150, 500), 0.0, p.u(0.4, 0.8), 0.6)
        p.set("porta_time", p.u(60, 220)); p.set("porta_mode", "Always")
        fenv(p, 2, 0.5, p.u(40, 120), 0.0, 60)
        p.mod("Env 2", "oA_coarse", p.u(0.06, 0.16))
        if kind >= 3:
            fx(p, "dist", mode=p.rng.choice(["Tube", "Soft Clip", "Asym", "Multiband"]), drive=p.u(0.25, 0.6), mix=p.u(0.5, 1.0),
               mb_low=0.0, mb_mid=p.u(0.4, 0.8), mb_high=p.u(0.2, 0.5))
            fx(p, "sat", drive=p.u(0.2, 0.4), warmth=p.u(0.3, 0.6))
        p.macros_std("e1_dec", 0.3, "Decay")


def recipe_reese(p, i):
    base(p)
    t = p.rng.choice(["Reese Stack 1", "Reese Stack 2", "Reese Stack 3", "Reese Stack 4", "Reese Stack 5", "Reese Stack 6", "Saw", "Supersaw Stack"])
    osc(p, "A", t, level=0.7, uni=p.rng.choice([2, 3, 4, 5, 7]), detune=p.u(0.25, 0.45), blend=p.u(0.6, 0.9), width=p.u(0.5, 0.9), pos=p.u(0, 0.6))
    osc(p, "B", p.rng.choice(["Saw", "Reese Stack 1", "Tube Saw"]), level=0.55, fine=p.u(-12, 12), uni=2, detune=p.u(0.15, 0.3), route="Filter 1")
    sub(p, "Sine", -1, p.u(0.4, 0.6))
    filt(p, 1, p.rng.choice(["LP 24", "Ladder LP 24", "Dirty LP 24", "MS-20 LP"]), p.u(450, 1600), p.u(0.15, 0.35), p.u(0, 0.3), p.u(0.2, 0.5))
    amp(p, p.u(0.5, 10), 500, 1.0, p.u(80, 250))
    p.set("porta_time", p.u(20, 80))
    p.set("l1_sync", True); p.set("l1_sync_rate", p.rng.choice(["1 Bar", "2 Bars", "1/2"])); p.set("l1_mode", "Free")
    p.mod("LFO 1", "oA_pos", p.u(0.2, 0.5), bipolar=True)
    p.mod("LFO 1", "f1_cutoff", p.u(0.05, 0.15), bipolar=True)
    if i % 3 == 0:
        fx(p, "ott", depth=p.u(0.3, 0.7), mix=p.u(0.2, 0.4))
    if i % 2 == 0:
        fx(p, "dist", mode="Multiband", drive=p.u(0.2, 0.4), mb_low=0.0, mb_mid=p.u(0.4, 0.7), mb_high=p.u(0.2, 0.4), mix=p.u(0.4, 0.8))
    fx(p, "mod", mode="Phaser", rate=p.u(0.05, 0.3), depth=p.u(0.3, 0.6), mix=p.u(0.15, 0.35))
    p.macros_std("oA_detune", 0.4, "Detune")


def recipe_ukg(p, i):
    base(p)
    organ = i % 2 == 0
    if organ:
        osc(p, "A", p.rng.choice(["Garage Organ 1", "Garage Organ 2", "Garage Organ 3", "Garage Organ 4", "Organ Drawbars"]), level=0.8, pos=p.u(0, 0.7))
        filt(p, 1, "LP 24", p.u(1200, 4000), p.u(0.1, 0.3))
        amp(p, 0.5, p.u(150, 400), p.u(0.2, 0.6), p.u(40, 120))
        fenv(p, 2, 0.5, p.u(80, 250), 0.0, 80)
        p.mod("Env 2", "f1_cutoff", p.u(0.15, 0.3))
        p.macros_std("e2_dec", 0.4, "Stab Length")
    else:
        osc(p, "A", p.rng.choice(["Wobble Classic", "Square", "Saw", "Hollow Pipe", "Pulse Sweep"]), level=0.8, pos=p.u(0.2, 0.8))
        filt(p, 1, p.rng.choice(["LP 24", "Ladder LP 24", "Dirty LP 12"]), p.u(300, 900), p.u(0.3, 0.55), p.u(0.1, 0.3))
        amp(p, 0.5, 400, 1.0, 80)
        p.wobble(1, p.rng.choice(["classic", "yoy", "triplet", "bounce"]))
        p.set("l1_mode", "Trigger")
        p.mod("LFO 1", "f1_cutoff", p.u(0.3, 0.5))
        p.mod("LFO 1", "oA_pos", p.u(0.1, 0.3))
        p.macros_std("l1_rate", 0.3, "Wobble Speed")
    sub(p, "Sine", -1, p.u(0.35, 0.55))
    p.set("porta_time", p.u(25, 70)); p.set("porta_mode", "Legato")
    if i % 3 == 0:
        fx(p, "duck", rate="1/4", shape="Kick Duck Soft", depth=p.u(0.4, 0.7))
    fx(p, "dist", mode="Tube", drive=p.u(0.1, 0.3), mix=p.u(0.4, 0.7))


def recipe_tech(p, i):
    base(p)
    osc(p, "A", p.rng.choice(["Rubber 1", "Rubber 2", "Rubber 3", "Rubber Bounce", "Elastic", "Pluck Soft", "Pluck Bright", "Bit Rubber", "Sine to Saw"]),
        level=0.8, pos=p.u(0.1, 0.7))
    if i % 3 == 0:
        osc(p, "B", p.rng.choice(["Sine", "Triangle", "Pluck Nasal"]), level=0.4, oct=1)
    sub(p, "Sine", -1, p.u(0.35, 0.6))
    filt(p, 1, p.rng.choice(["LP 24", "LP 12", "Ladder LP 24", "Diode LP"]), p.u(250, 800), p.u(0.15, 0.4))
    amp(p, 0.3, p.u(160, 380), p.u(0.0, 0.5), p.u(40, 120), 0.0, p.u(0.5, 0.9))
    fenv(p, 2, 0.3, p.u(90, 260), 0.0, 80)
    p.mod("Env 2", "f1_cutoff", p.u(0.3, 0.5))
    p.mod("Env 2", "oA_pos", p.u(0.1, 0.3))
    p.mod("Velocity", "f1_cutoff", p.u(0.05, 0.15))
    p.set("porta_time", p.u(0, 35))
    fx(p, "duck", rate="1/4", shape=p.rng.choice(["Kick Duck Hard", "Kick Duck Soft"]), depth=p.u(0.5, 0.85))
    if i % 2 == 0:
        fx(p, "sat", drive=p.u(0.15, 0.35), warmth=p.u(0.3, 0.6))
    p.macros_std("e2_dec", 0.35, "Pluck Length", "fx_delay_mix", "Echo")
    if i % 4 == 0:
        fx(p, "delay", mode="Ping-Pong", sync=True, sync_l="1/8 D", sync_r="1/8 D", feedback=0.25, mix=0.0)


def recipe_growl(p, i):
    base(p)
    osc(p, "A", p.rng.choice(["Growl 1", "Growl 2", "Growl 3", "Growl 4", "Growl 5", "Growl 6", "Growl 7", "Growl 8", "Neuro Mid 1", "Riddim Square", "Talkbox"]),
        level=0.75, uni=p.rng.choice([1, 2, 3]), detune=p.u(0.1, 0.25), pos=p.u(0.1, 0.5),
        warp=p.rng.choice(["Off", "Bend +/-", "Mirror", "FM", "Asym +"]), warp_amt=p.u(0.0, 0.4))
    osc(p, "B", p.rng.choice(["Square", "Saw", "Vowel Talk", "FM Metal 2"]), level=0.45, semi=0, oct=p.rng.choice([0, 1]))
    sub(p, "Sine", -1, p.u(0.4, 0.6))
    filt(p, 1, p.rng.choice(["Formant Vowel", "Formant Talker", "Comb +", "LP 24", "Dirty BP", "Phaser 8"]), p.u(300, 2500), p.u(0.3, 0.6), p.u(0.1, 0.4), p.u(0.0, 0.5))
    amp(p, 0.5, 400, 1.0, p.u(40, 120))
    p.wobble(1, p.rng.choice(["rapid", "riddim", "yoy", "triplet", "lurch", "classic"]))
    p.set("l1_mode", "Trigger")
    p.mod("LFO 1", "oA_pos", p.u(0.3, 0.6))
    p.mod("LFO 1", "f1_cutoff", p.u(0.2, 0.45))
    fx(p, "dist", mode=p.rng.choice(["Foldback", "Multiband", "Diode", "Hard Clip"]), drive=p.u(0.35, 0.7), mix=p.u(0.6, 1.0),
       mb_low=0.0, mb_mid=p.u(0.5, 0.9), mb_high=p.u(0.3, 0.6))
    fx(p, "ott", depth=p.u(0.6, 1.0), mix=p.u(0.3, 0.6))
    p.set("porta_time", p.u(10, 60))
    p.macros_std("l1_rate", 0.35, "Wobble Rate")


def recipe_hybrid(p, i):
    base(p, mono=False)
    p.set("poly_voices", p.rng.choice([4, 6, 8]))
    osc(p, "A", p.rng.choice(["Chord Stack Major", "Chord Stack Minor", "Supersaw Stack", "Power Fifth", "Glass", "Bell"]),
        level=0.7, uni=p.rng.choice([3, 5, 7]), detune=p.u(0.2, 0.35), width=0.9, pos=p.u(0.2, 0.9))
    osc(p, "B", p.rng.choice(["FM Metal 1", "FM Metal 3", "Saw", "Sync Sweep Saw"]), level=0.4, oct=1)
    sub(p, "Sine", -1, p.u(0.3, 0.5))
    filt(p, 1, p.rng.choice(["LP 24", "LP/HP Morph", "Dual BP", "Phaser 4"]), p.u(1500, 6000), p.u(0.1, 0.3))
    amp(p, p.u(1, 20), 600, p.u(0.6, 1.0), p.u(150, 500))
    p.set("l2_sync", True); p.set("l2_sync_rate", p.rng.choice(["1/4", "1/8", "1/2"]))
    p.mod("LFO 2", "oA_pos", p.u(0.1, 0.3))
    fx(p, "mod", mode="Chorus", rate=p.u(0.3, 0.8), depth=p.u(0.3, 0.6), mix=p.u(0.2, 0.4))
    fx(p, "verb", type=p.rng.choice(["Plate", "Hall"]), size=p.u(0.5, 0.8), decay=p.u(1.5, 3.5), mix=p.u(0.12, 0.25))
    fx(p, "hyper", voices=5, detune=p.u(0.2, 0.4), mix=p.u(0.3, 0.5), mono_below=150.0)
    p.macros_std("oA_pos", 0.4, "Colour")


def recipe_emotive(p, i):
    base(p, mono=i % 3 != 0)
    if i % 3 == 0:
        p.set("poly_voices", 4)
    osc(p, "A", p.rng.choice(["Velvet Saw", "Tape Saw", "Sweet Spot", "Sine to Saw", "Pluck Soft", "Choir Formant"]), level=0.75,
        uni=p.rng.choice([1, 2, 3]), detune=p.u(0.1, 0.2), pos=p.u(0.1, 0.6))
    sub(p, "Sine", -1, p.u(0.4, 0.6))
    filt(p, 1, p.rng.choice(["LP 12", "LP 24", "Ladder LP 24"]), p.u(500, 2000), p.u(0.05, 0.25), 0.0, p.u(0.2, 0.5))
    amp(p, p.u(2, 40), 600, p.u(0.6, 1.0), p.u(150, 600))
    fx(p, "sat", drive=p.u(0.2, 0.4), warmth=p.u(0.5, 0.8), wow=p.u(0.05, 0.2), flutter=p.u(0.0, 0.15))
    fx(p, "duck", rate="1/4", shape=p.rng.choice(["Kick Duck Soft", "Long Pump"]), depth=p.u(0.5, 0.8))
    fx(p, "verb", type="Plate", size=p.u(0.4, 0.7), decay=p.u(1.2, 2.5), mix=p.u(0.08, 0.18))
    p.set("drift", p.u(0.1, 0.35))
    p.macros_std("e1_rel", 0.3, "Tail")


def recipe_organic(p, i):
    base(p)
    osc(p, "A", p.rng.choice(["Random Harmonics", "Glass", "Bell", "Hollow Pipe", "Nasal Reed", "Tape Saw", "Pluck Nasal", "Harmonic Climb"]),
        level=0.7, pos=p.u(0, 1), rand=1.0)
    noise(p, p.rng.choice(["Vinyl Crackle", "Tape Hiss Warm", "Breath", "Rain", "Fire Crackle", "Granular Dust", "Wind", "Shaker Texture", "Vinyl Hiss"]),
          p.u(0.15, 0.35), key=p.rng.choice([True, False]))
    filt(p, 1, p.rng.choice(["LP 12", "BP 12", "Formant Vowel", "Comb -", "Phaser 4"]), p.u(600, 3000), p.u(0.1, 0.4))
    filt(p, 2, "HP 12", p.u(30, 60), 0.0)
    amp(p, p.u(1, 30), p.u(300, 900), p.u(0.3, 0.9), p.u(100, 400))
    p.set("drift", p.u(0.4, 0.9))
    p.set("l3_sync", False); p.set("l3_rate", p.u(0.05, 0.4)); p.set("l3_shape", "Smooth Random"); p.set("l3_mode", "Free")
    p.mod("LFO 3", "oA_pos", p.u(0.2, 0.5))
    p.mod("Note Random", "f1_cutoff", p.u(0.05, 0.15))
    fx(p, "sat", drive=p.u(0.1, 0.3), warmth=0.6, wow=p.u(0.2, 0.5), flutter=p.u(0.1, 0.4))
    fx(p, "delay", mode="Tape", sync=True, sync_l="1/4", sync_r="1/4 D", feedback=p.u(0.3, 0.5), wow=0.4, mix=p.u(0.1, 0.2))
    p.macros_std("oD_level", 0.3, "Texture", "fx_delay_mix", "Echo")


def recipe_breaks(p, i):
    base(p)
    osc(p, "A", "Sine", level=0.8)
    osc(p, "B", p.rng.choice(["Reese Stack 1", "Reese Stack 3", "Hoover 1", "Hoover 2", "Square", "Warehouse Buzz"]), level=0.5,
        uni=p.rng.choice([2, 4]), detune=p.u(0.2, 0.35), route="Filter 1")
    p.set("oA_route", "Direct")
    sub(p, "Sine", -1, p.u(0.2, 0.4))
    filt(p, 1, p.rng.choice(["BP 12", "LP 24", "HP 12", "Dirty BP"]), p.u(400, 1500), p.u(0.2, 0.4), p.u(0.1, 0.4))
    amp(p, 0.5, p.u(250, 600), p.u(0.4, 1.0), p.u(60, 150))
    p.set("porta_time", p.u(30, 120)); p.set("porta_mode", "Always")
    p.set("l1_sync", True); p.set("l1_sync_rate", p.rng.choice(["1/2", "1 Bar", "1/4"]))
    p.mod("LFO 1", "oB_pos", p.u(0.2, 0.5))
    fx(p, "dist", mode=p.rng.choice(["Tube", "Multiband"]), drive=p.u(0.2, 0.45), mb_low=0.0, mb_mid=0.6, mb_high=0.3, mix=0.7)
    p.macros_std("oB_level", 0.4, "Mid Stack")


def recipe_acid(p, i):
    base(p)
    osc(p, "A", p.rng.choice(["Acid Saw", "Acid Square", "Saw", "Square", "Acid Squelch"]), level=0.85, pos=p.u(0, 0.6))
    p.set("oD_on", False)
    filt(p, 1, "Acid Ladder 303", p.u(250, 900), p.u(0.6, 0.85), p.u(0.2, 0.5), 0.2, 0.3)
    amp(p, 0.3, p.u(200, 500), p.u(0.3, 0.8), p.u(30, 90))
    fenv(p, 2, 0.3, p.u(120, 400), 0.0, 60)
    p.mod("Env 2", "f1_cutoff", p.u(0.25, 0.45))
    p.mod("Velocity", "f1_cutoff", p.u(0.1, 0.2))      # accent
    p.mod("Velocity", "f1_reso", p.u(0.05, 0.12))
    p.set("porta_time", p.u(40, 90)); p.set("porta_mode", "Legato"); p.set("porta_type", "Constant Time")
    p.set("amp_vel", p.u(0.3, 0.6))
    fx(p, "dist", mode=p.rng.choice(["Diode", "Tube", "Soft Clip"]), drive=p.u(0.3, 0.6), mix=p.u(0.6, 1.0))
    if i % 2 == 0:
        fx(p, "delay", mode="Ping-Pong", sync=True, sync_l="1/8 D", sync_r="1/16", feedback=p.u(0.3, 0.5), mix=p.u(0.1, 0.2))
    p.macros_std("f1_reso", 0.3, "Resonance", "fx_delay_mix", "Echo")
    p.macros = ["Cutoff", "Drive", "Resonance", "Echo"]


def recipe_rumble(p, i):
    base(p)
    osc(p, "A", p.rng.choice(["Sine", "Sub Harmonic", "Dirty Sub", "Triangle"]), level=0.8)
    noise(p, p.rng.choice(["Rumble", "Sub Rumble Noise", "Brown", "Club Room Tone", "Industrial Hum"]), p.u(0.3, 0.55), key=True)
    filt(p, 1, p.rng.choice(["LP 24", "LP 36", "Ladder LP 24"]), p.u(120, 400), p.u(0.1, 0.3), p.u(0.2, 0.5), 0.6)
    amp(p, p.u(1, 10), p.u(500, 1500), p.u(0.2, 0.7), p.u(200, 800))
    fx(p, "verb", type=p.rng.choice(["Club", "Hall"]), size=p.u(0.6, 0.9), decay=p.u(3.0, 8.0), damp=p.u(0.5, 0.8), mix=p.u(0.2, 0.35), lowcut=40.0)
    fx(p, "delay", mode="Stereo", sync=True, sync_l="1/8", sync_r="1/8 D", feedback=p.u(0.3, 0.6), lp=1500.0, mix=p.u(0.1, 0.25))
    fx(p, "dist", mode="Tube", drive=p.u(0.3, 0.5), mix=0.6)
    p.set("l1_sync", True); p.set("l1_sync_rate", p.rng.choice(["1/16", "1/8"]))
    p.mod("LFO 1", "f1_cutoff", p.u(0.08, 0.2))
    p.macros_std("fx_verb_size", 0.4, "Room Size")


def recipe_fx(p, i):
    base(p)
    kind = i % 3
    if kind == 0:   # risers
        osc(p, "A", p.rng.choice(["Laser Zap", "Sync Sweep Saw", "Supersaw Stack", "Fizz"]), level=0.7, uni=5, detune=0.3)
        noise(p, p.rng.choice(["White", "Spray", "Whisper Air"]), 0.3)
        amp(p, p.u(2000, 6000), 1000, 1.0, 500)
        fenv(p, 3, p.u(3000, 8000), 1000, 1.0, 300)
        p.mod("Env 3", "oA_coarse", p.u(0.15, 0.25))
        p.mod("Env 3", "f1_cutoff", 0.4)
        filt(p, 1, "HP 12", 150.0, 0.3)
        fx(p, "verb", type="Hall", size=0.8, decay=4.0, mix=0.3)
    elif kind == 1:  # neuro mid basses
        osc(p, "A", p.rng.choice(["Neuro Mid 1", "Neuro Mid 2", "Neuro Mid 3", "Phase Dist Reso 2", "Comb Sweep"]), level=0.75, uni=2,
            warp=p.rng.choice(["FM", "Sync", "Bend +"]), warp_amt=p.u(0.2, 0.6))
        osc(p, "C", "FM Metal 4", level=0.4)
        sub(p, "Sine", -1, 0.5)
        filt(p, 1, p.rng.choice(["Comb -", "Flange Comb", "Phaser 12", "Dirty BP"]), p.u(400, 2000), p.u(0.4, 0.7), 0.4)
        amp(p, 0.5, 400, 1.0, 80)
        p.set("l1_sync", True); p.set("l1_sync_rate", p.rng.choice(["1/16", "1/32", "1/16 T"]))
        p.mod("LFO 1", "oA_warp_amt", p.u(0.2, 0.5))
        p.mod("LFO 1", "f1_cutoff", p.u(0.2, 0.4))
        fx(p, "dist", mode="Multiband", drive=0.6, mb_low=0.0, mb_mid=0.8, mb_high=0.5)
        fx(p, "ott", depth=0.8, mix=0.5)
    else:            # one-shot drops / impacts
        osc(p, "A", p.rng.choice(["Dirty Sub", "Laser Zap", "Wavefold Deep", "Bell"]), level=0.85)
        sub(p, "Sine", -1, 0.6)
        amp(p, 0.1, p.u(800, 2500), 0.0, 300, 0.0, 0.7)
        fenv(p, 2, 0.1, p.u(300, 1200), 0.0, 100)
        p.mod("Env 2", "oA_coarse", p.u(0.15, 0.3))
        filt(p, 1, "LP 24", 4000.0, 0.2)
        fx(p, "dist", mode="Hard Clip", drive=0.5, mix=0.6)
        fx(p, "verb", type="Club", size=0.6, decay=2.0, mix=0.2)
    p.macros_std("oA_warp_amt", 0.4, "Chaos")


RECIPES = {
    "SUB & 808": recipe_sub, "ROLLING & REESE": recipe_reese, "UKG & BASSLINE": recipe_ukg,
    "TECH HOUSE RUBBER": recipe_tech, "GROWL & RIDDIM": recipe_growl, "HYBRID / COLOUR": recipe_hybrid,
    "EMOTIVE / WARM": recipe_emotive, "ORGANIC / TEXTURE": recipe_organic, "BREAKS & JUNGLE": recipe_breaks,
    "ACID": recipe_acid, "HARDGROOVE / RUMBLE": recipe_rumble, "FX & NEURO": recipe_fx,
}

BPM = {"UK Bass": (130, 140), "Trap": (140, 150), "Drill": (140, 145), "Grime": (138, 142), "Garage": (130, 134),
       "Drum & Bass": (170, 175), "Minimal Tech": (124, 128), "Jungle": (160, 170), "Liquid": (172, 174), "Rollers": (172, 175),
       "UK Garage": (130, 134), "Bassline": (135, 140), "Speed Garage": (130, 135), "2-Step": (130, 134),
       "Tech House": (124, 128), "House": (120, 126), "Minimal": (124, 128), "Deep Tech": (122, 126),
       "Dubstep": (140, 140), "Riddim": (140, 150), "Brostep": (140, 150), "Bass House": (125, 128),
       "Melodic Dubstep": (140, 150), "Colour Bass": (145, 150), "Future Bass": (140, 160), "Hybrid Trap": (140, 150),
       "Emotive House": (120, 128), "Melodic House": (118, 124), "Lo-Fi House": (115, 122),
       "Folktronica": (100, 125), "Experimental": (90, 130), "Ambient Bass": (80, 110), "Broken Beat": (115, 125),
       "Breakbeat": (130, 140), "Breaks": (128, 136), "UK Hardcore": (140, 150),
       "Acid": (125, 135), "Acid Techno": (135, 145), "Acid House": (120, 126),
       "Hardgroove": (138, 145), "Techno": (128, 135), "Hypnotic Techno": (128, 132), "Industrial": (130, 140),
       "Neurofunk": (172, 175), "FX": (120, 175), "Halftime": (170, 175)}


def main():
    rng = random.Random(0xBA55)
    total = 0
    for bank, count, chars, genres, nouns, sigs in BANKS:
        names = make_names(rng, count, nouns, sigs)
        folder = os.path.join(OUT, re.sub(r"[^A-Za-z0-9]+", "_", bank).strip("_"))
        os.makedirs(folder, exist_ok=True)
        for f in os.listdir(folder):
            if f.endswith(".bgdjpreset"):
                os.remove(os.path.join(folder, f))
        for i, name in enumerate(names):
            p = P(random.Random(rng.random()))
            RECIPES[bank](p, i)
            genre = genres[i % len(genres)]
            lo, hi = BPM.get(genre, (120, 130))
            character = chars[i % len(chars)]
            key = rng.choice(KEYS_MIN if bank in ("ROLLING & REESE", "GROWL & RIDDIM", "EMOTIVE / WARM", "HARDGROOVE / RUMBLE") else KEYS + KEYS_MIN)
            extra = sorted({rng.choice(["dark", "bright", "wide", "mono", "clean", "dirty", "punchy", "smooth", "aggressive", "warm",
                                        "hypnotic", "bouncy", "rolling", "gritty", "lush", "tight", "playful"]) for _ in range(2)})
            preset = {
                "format": "bgdjpreset",
                "version": 1,
                "name": name,
                "bank": bank,
                "author": "Raygun",
                "description": f"{character.capitalize()} bass for {genre}.",
                "tags": {"genre": genre, "character": character, "key": key, "bpm": rng.randint(lo, hi), "extra": extra},
                "macros": p.macros,
                "tables": p.tables,
                "params": p.params,
                "matrix": p.matrix,
            }
            if p.lfo_shapes:
                preset["lfo_shapes"] = p.lfo_shapes
            assert legal(name) and all(legal(t) for t in extra + [genre, character])
            fn = re.sub(r"[^A-Za-z0-9 \-]", "", name).strip().replace(" ", "_") + ".bgdjpreset"
            with open(os.path.join(folder, fn), "w", encoding="utf-8") as fh:
                json.dump(preset, fh, indent=1, ensure_ascii=False)
            total += 1
    print(f"wrote {total} presets to {os.path.abspath(OUT)}")
    assert total == 430, total


if __name__ == "__main__":
    main()
