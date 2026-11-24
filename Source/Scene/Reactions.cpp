#include "Reactions.h"

namespace bgdj
{
using C = ReactionCategory;
using M = const MoodState&;
using S = const juce::String&;

static bool playing (M m)        { return ! m.idle && ! m.walkout; }
static bool charIs (S c, std::initializer_list<const char*> any)
{
    for (auto* a : any) if (c.contains (a)) return true;
    return false;
}

const std::vector<ReactionDef>& reactionDefs()
{
    static const std::vector<ReactionDef> defs = {
        // ---- Positive / hype ---------------------------------------------------------------------------------------
        { rx::HandsUpWave, "Hands-up wave", C::Positive, 3, 4.0f, 10, "cheer", [] (M m, S) { return playing (m) && m.hype > 60 && m.subShare > 0.25f; } },
        { rx::SyncJump, "Everyone jumps on the downbeat", C::Positive, 2, 3.0f, 12, "cheer", [] (M m, S) { return playing (m) && m.hype > 55; } },
        { rx::Oooh, "OOOOH pull-up", C::Positive, 2, 3.0f, 15, "oooh", [] (M m, S) { return playing (m) && m.resonanceScream && m.hype > 30; } },
        { rx::OneMore, "\"ONE MORE!\" chant", C::Positive, 2, 5.0f, 40, "one_more", [] (M m, S) { return playing (m) && m.hype > 75; } },
        { rx::Flashlights, "Phone flashlights up", C::Positive, 3, 7.0f, 30, "", [] (M m, S c) { return playing (m) && m.hype > 40 && (charIs (c, { "emotive", "warm", "hybrid", "colour" }) || m.hype > 80); } },
        { rx::ShoulderRider, "Shoulder rider", C::Positive, 2, 8.0f, 35, "cheer", [] (M m, S) { return playing (m) && m.hype > 70; } },
        { rx::MoshPit, "Mosh pit opens", C::Positive, 3, 7.0f, 25, "cheer", [] (M m, S c) { return playing (m) && m.hype > 55 && (charIs (c, { "growl", "riddim", "neuro", "dubstep" }) || m.harshShare > 0.2f); } },
        { rx::RaveShuffle, "Rave hands & shufflers", C::Positive, 3, 8.0f, 20, "", [] (M m, S c) { return playing (m) && m.hype > 40 && (charIs (c, { "tech", "rubber", "rolling", "reese", "hardgroove" }) || m.groove > 0.5f); } },
        { rx::TwoStep, "Two-stepping couples", C::Positive, 3, 8.0f, 20, "", [] (M m, S c) { return playing (m) && m.hype > 35 && charIs (c, { "ukg", "garage", "bassline", "organ", "breaks" }); } },
        { rx::CrouchExplode, "Crouch low... then explode", C::Positive, 1, 4.0f, 30, "cheer", [] (M m, S) { return playing (m) && m.dropAfterQuiet; } },
        { rx::Confetti, "Confetti cannon + CO2", C::Positive, 2, 5.0f, 45, "co2", [] (M m, S) { return playing (m) && m.hype > 90; } },
        { rx::StrangersHug, "Strangers hug", C::Positive, 2, 5.0f, 30, "", [] (M m, S) { return playing (m) && m.hype > 65; } },
        { rx::Backflip, "Dancer backflips", C::Positive, 1, 3.0f, 40, "cheer", [] (M m, S) { return playing (m) && m.hype > 80; } },
        { rx::BartenderDances, "Bartender stops pouring and dances", C::Positive, 2, 8.0f, 40, "", [] (M m, S) { return playing (m) && m.hype > 70; } },
        { rx::SecurityNods, "Security guard nods along", C::Positive, 2, 7.0f, 35, "", [] (M m, S) { return playing (m) && m.hype > 60; } },
        { rx::StrobeGuy, "Lighting tech goes full strobe", C::Positive, 2, 4.0f, 30, "", [] (M m, S) { return playing (m) && m.hype > 75 && m.groove > 0.3f; } },
        // ---- DJ moments ------------------------------------------------------------------------------------------------
        { rx::Rewind, "REWIND!", C::Dj, 1, 4.0f, 60, "rewind", [] (M m, S) { return playing (m) && m.hype > 80; } },
        { rx::FistPump, "Fist pump behind the decks", C::Dj, 3, 2.0f, 12, "cheer", [] (M m, S) { return playing (m) && m.hype > 55; } },
        { rx::AirHorn, "Air horn pad", C::Dj, 2, 2.5f, 25, "airhorn", [] (M m, S) { return playing (m) && m.hype > 65; } },
        { rx::DropItLean, "\"Drop it\" lean", C::Dj, 2, 3.0f, 25, "", [] (M m, S) { return playing (m) && m.hype > 50; } },
        { rx::B2BHighFive, "B2B partner high-five", C::Dj, 1, 5.0f, 90, "", [] (M m, S) { return playing (m) && m.hype > 45; } },
        { rx::TowelThrow, "Towel into the crowd", C::Dj, 1, 3.5f, 60, "cheer", [] (M m, S) { return playing (m) && m.hype > 85; } },
        { rx::HeadphonePose, "One-ear headphone focus", C::Dj, 3, 5.0f, 15, "", [] (M m, S) { return playing (m); } },
        { rx::SipDrink, "Sip... nearly spill it on the mixer", C::Dj, 2, 4.0f, 45, "", [] (M m, S) { return playing (m) && m.hype > 20 && m.hype < 80; } },
        { rx::ExtendedSet, "Promoter: EXTENDED SET", C::Dj, 1, 5.0f, 120, "cheer", [] (M m, S) { return playing (m) && m.barsHypeHigh() >= 16.0; } },
        { rx::FanSelfie, "Fan sneaks in for a selfie", C::Dj, 1, 6.0f, 90, "", [] (M m, S) { return playing (m) && m.hype > 60; } },
        { rx::CameraCrew, "Camera crew pans the booth", C::Dj, 1, 6.0f, 90, "", [] (M m, S) { return playing (m) && m.barsHypeHigh() >= 8.0; } },
        { rx::LightsCut, "Lights cut to black", C::Dj, 1, 30.0f, 40, "gasp", [] (M m, S) { return playing (m) && m.hype > 60 && m.silent; } },
        // ---- Funny ----------------------------------------------------------------------------------------------------------
        { rx::BucketHat, "Bucket-hat confidence", C::Funny, 2, 8.0f, 40, "", [] (M m, S) { return playing (m) && m.hype > 20; } },
        { rx::ShoeFlies, "A shoe flies across the room", C::Funny, 1, 3.0f, 60, "", [] (M m, S) { return playing (m) && m.hype > 70; } },
        { rx::CongaLine, "Conga line", C::Funny, 1, 10.0f, 90, "", [] (M m, S) { return playing (m) && m.hype > 35; } },
        { rx::SuitFistPump, "Business suit, last night on earth", C::Funny, 2, 7.0f, 50, "", [] (M m, S) { return playing (m) && m.hype > 60; } },
        { rx::CoupleArgue, "Couple argues... makes up on the drop", C::Funny, 1, 9.0f, 80, "", [] (M m, S) { return playing (m); } },
        { rx::BalconySleeper, "Asleep on the balcony railing", C::Funny, 2, 10.0f, 40, "snore", [] (M m, S) { return ! m.walkout && (m.bored || (playing (m) && m.hype < 25)); } },
        { rx::SongRequest, "Song request on a scrap of paper", C::Funny, 2, 6.0f, 50, "", [] (M m, S) { return playing (m) && m.hype < 60; } },
        { rx::DiscoBallDrop, "Disco ball drops an inch", C::Funny, 1, 3.0f, 90, "gasp", [] (M m, S) { return playing (m) && m.subSpike; } },
        { rx::PuddleSlip, "Puddle slip into a perfect pose", C::Funny, 1, 3.5f, 60, "cheer", [] (M m, S) { return playing (m) && m.hype > 50; } },
        { rx::Pigeon, "A pigeon enters the club", C::Funny, 0, 8.0f, 120, "pigeon", [] (M, S) { return true; } },
        { rx::Ringtone, "Ringtone - whole crowd turns around", C::Funny, 1, 4.0f, 90, "ringtone", [] (M m, S) { return ! m.walkout && m.loudness < 0.3f; } },
        { rx::CatOnController, "Grey the cat sits on the controller", C::Funny, 0, 7.0f, 180, "meow", [] (M, S) { return true; } },
        { rx::CatKnocksDrink, "Grey knocks your drink off, staring at you", C::Funny, 0, 5.0f, 180, "crash", [] (M, S) { return true; } },
        { rx::BouncerLetsQueueIn, "Bouncer vibes too hard, queue floods in", C::Funny, 1, 6.0f, 120, "cheer", [] (M m, S) { return playing (m) && m.hype > 75 && m.crowdSize < 0.92f; } },
        // ---- Negative ------------------------------------------------------------------------------------------------------
        { rx::ArmsFolded, "Arms folded (too muddy)", C::Negative, 3, 6.0f, 15, "", [] (M m, S) { return playing (m) && m.mud; } },
        { rx::EarsCovered, "Hands over ears", C::Negative, 3, 4.0f, 12, "", [] (M m, S) { return playing (m) && (m.clipping || m.harshShare > 0.35f); } },
        { rx::Booo, "Booo", C::Negative, 2, 4.0f, 20, "boo", [] (M m, S) { return playing (m) && m.annoyance > 50; } },
        { rx::PlaySomethingElse, "\"PLAY SOMETHING ELSE!\"", C::Negative, 2, 3.5f, 30, "play_else", [] (M m, S) { return ! m.walkout && (m.annoyance > 60 || m.boredom > 60); } },
        { rx::DriftToBar, "People drift to the bar", C::Negative, 3, 9.0f, 25, "chatter", [] (M m, S) { return ! m.walkout && m.bored; } },
        { rx::PhonesOut, "Phones out, screens glow blue", C::Negative, 3, 8.0f, 25, "", [] (M m, S) { return ! m.walkout && m.bored; } },
        { rx::PointAtSpeaker, "Points at the speaker, shakes head", C::Negative, 3, 5.0f, 20, "", [] (M m, S) { return playing (m) && m.phaseProblem; } },
        { rx::CupThrown, "Plastic cup thrown (gently)", C::Negative, 1, 2.5f, 40, "cup", [] (M m, S) { return playing (m) && m.annoyance > 70; } },
        { rx::EngineerGainDown, "Sound engineer turns you down", C::Negative, 2, 6.0f, 45, "", [] (M m, S) { return playing (m) && m.clipping; } },
        { rx::PromoterClock, "Promoter taps the clock", C::Negative, 2, 5.0f, 60, "", [] (M m, S) { return ! m.walkout && (m.boredom > 50 || m.annoyance > 50); } },
        { rx::CrowdThins, "Crowd thins out to the exit", C::Negative, 1, 10.0f, 60, "chatter", [] (M m, S) { return ! m.walkout && m.barsHypeLow() >= 32.0; } },
        { rx::FullWalkout, "Full walkout - SET OVER", C::Negative, 0, 25.0f, 120, "chatter", [] (M m, S) { return m.barsHypeLow() >= 40.0; } },
        { rx::LoyalFan, "One loyal fan dances alone", C::Negative, 2, 12.0f, 40, "", [] (M m, S) { return m.hype < 25 || m.walkout; } },
        // ---- Ambient / idle ----------------------------------------------------------------------------------------------
        { rx::IdleChat, "Crowd chats and sways", C::Ambient, 4, 8.0f, 8, "chatter", [] (M m, S) { return m.idle; } },
        { rx::BarQueue, "Bar queue grows", C::Ambient, 3, 10.0f, 15, "", [] (M m, S) { return m.idle; } },
        { rx::WaveToStart, "Someone waves at you to start", C::Ambient, 2, 4.0f, 20, "", [] (M m, S) { return m.idle; } },
        { rx::SmokePuff, "Smoke machine puff", C::Ambient, 2, 5.0f, 12, "smoke", [] (M m, S) { return m.idle || m.hype > 50; } },
        { rx::CatSleepsOnSpeaker, "Grey the cat sleeps on the speaker stack", C::Ambient, 2, 15.0f, 30, "", [] (M m, S) { return m.idle; } },
    };
    return defs;
}

const ReactionDef* findReaction (int id)
{
    for (auto& d : reactionDefs())
        if (d.id == id)
            return &d;
    return nullptr;
}

bool ReactionDirector::isActive (int id) const noexcept
{
    return find (id) != nullptr;
}

const ActiveReaction* ReactionDirector::find (int id) const noexcept
{
    for (auto& a : active)
        if (a.id == id)
            return &a;
    return nullptr;
}

bool ReactionDirector::trigger (int id, double now, CrowdMood& mood, bool ignoreCooldown)
{
    const auto* def = findReaction (id);
    if (def == nullptr || id == lastId || isActive (id))
        return false;
    if (! ignoreCooldown && lastFired[(size_t) id] > 0.0 && now - lastFired[(size_t) id] < def->cooldown)
        return false;

    active.push_back ({ id, now, def->duration, rng.nextFloat() });
    lastFired[(size_t) id] = now;
    lastId = id;
    ++totalFired;
    startedThisFrame.push_back (id);

    if (id == rx::BouncerLetsQueueIn) mood.bumpCrowdSize (0.25f);
    if (id == rx::FullWalkout)        mood.getMutable().walkout = true;
    if (id == rx::Confetti || id == rx::HandsUpWave) mood.getMutable().hype = std::min (100.0f, mood.get().hype + 3.0f);
    return true;
}

bool ReactionDirector::pickFrom (std::initializer_list<int> ids, double now, CrowdMood& mood)
{
    std::vector<int> pool (ids);
    // shuffle so equal triggers vary
    for (size_t i = pool.size(); i > 1; --i)
        std::swap (pool[i - 1], pool[(size_t) rng.nextInt ((int) i)]);
    for (int id : pool)
        if (trigger (id, now, mood))
            return true;
    return false;
}

void ReactionDirector::update (double now, float dt, CrowdMood& mood)
{
    juce::ignoreUnused (dt);
    startedThisFrame.clear();
    const auto& m = mood.get();
    const auto& ch = mood.getCharacter();

    // expire finished reactions; lights-cut ends on the next note-on
    active.erase (std::remove_if (active.begin(), active.end(), [&] (const ActiveReaction& a)
    {
        if (a.id == rx::LightsCut && m.noteOnsThisFrame > 0)
            return true;
        return now - a.start > a.duration;
    }), active.end());
    lightsAreCut = isActive (rx::LightsCut);

    // ---- hard-wired triggers ------------------------------------------------------------------
    if (m.subDropAfterSilence)
        pickFrom (m.hype > 70 ? std::initializer_list<int> { rx::HandsUpWave, rx::CrouchExplode, rx::Confetti }
                              : std::initializer_list<int> { rx::HandsUpWave, rx::CrouchExplode }, now, mood);
    if (m.dropAfterQuiet)      trigger (rx::CrouchExplode, now, mood);
    if (m.sweepThenDownbeat && m.hype > 40) trigger (rx::SyncJump, now, mood);
    if (m.resonanceScream && m.hype > 30)   trigger (rx::Oooh, now, mood);

    if (m.barsHypeHigh() >= 16.0 && ! hypeStreakUsed)
        hypeStreakUsed = pickFrom ({ rx::Confetti, rx::CameraCrew, rx::ExtendedSet }, now, mood);
    if (m.barsHypeHigh() < 1.0) hypeStreakUsed = false;

    if (m.clipping)     pickFrom ({ rx::EarsCovered, rx::EngineerGainDown }, now, mood);
    if (m.mud)          trigger (rx::ArmsFolded, now, mood);
    if (m.phaseProblem) trigger (rx::PointAtSpeaker, now, mood);
    if (m.bored)        pickFrom ({ rx::BalconySleeper, rx::DriftToBar, rx::PhonesOut }, now, mood);

    if (m.barsHypeLow() >= 32.0 && ! thinnedThisStreak)
        thinnedThisStreak = trigger (rx::CrowdThins, now, mood, true);
    if (m.barsHypeLow() >= 40.0 && ! walkoutThisStreak)
        walkoutThisStreak = trigger (rx::FullWalkout, now, mood, true);
    if (m.barsHypeLow() < 1.0) { thinnedThisStreak = false; walkoutThisStreak = false; }

    if (m.silent && m.hype > 60 && ! lightsAreCut && m.silenceSeconds > 60.0 / m.bpm)
        trigger (rx::LightsCut, now, mood);

    // rare spawns: 1 in 400 notes
    for (int n = 0; n < m.noteOnsThisFrame; ++n)
        if (rng.nextInt (400) == 0)
            pickFrom ({ rx::Pigeon, rx::CatOnController, rx::CatKnocksDrink }, now, mood);

    // ---- weighted random ambience --------------------------------------------------------------------
    if (now >= nextRandomAt)
    {
        double total = 0.0;
        std::vector<std::pair<int, double>> pool;
        for (auto& d : reactionDefs())
        {
            if (d.weight <= 0.0f || d.id == lastId || isActive (d.id))
                continue;
            if (lastFired[(size_t) d.id] > 0.0 && now - lastFired[(size_t) d.id] < d.cooldown)
                continue;
            if (! d.eligible (m, ch))
                continue;
            double w = d.weight;
            if (d.category == C::Positive) w *= 0.5 + m.hype / 50.0;
            if (d.category == C::Negative) w *= 0.5 + (m.annoyance + m.boredom) / 60.0;
            pool.push_back ({ d.id, w });
            total += w;
        }
        if (total > 0.0)
        {
            double r = rng.nextDouble() * total;
            for (auto& [id, w] : pool)
            {
                r -= w;
                if (r <= 0.0) { trigger (id, now, mood); break; }
            }
        }
        const double gap = m.idle ? 4.0 : juce::jmap ((double) m.hype, 0.0, 100.0, 7.0, 2.5);
        nextRandomAt = now + gap * (0.7 + 0.6 * rng.nextDouble());
    }
}
} // namespace bgdj
