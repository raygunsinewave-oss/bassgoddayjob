#pragma once

#include "CrowdMood.h"
#include <functional>
#include <vector>

namespace bgdj
{
enum class ReactionCategory { Positive, Dj, Funny, Negative, Ambient };

/** Reaction ids follow the numbering of the design spec (1..60). */
namespace rx
{
    enum Id : int
    {
        HandsUpWave = 1, SyncJump, Oooh, OneMore, Flashlights, ShoulderRider, MoshPit, RaveShuffle, TwoStep,
        CrouchExplode, Confetti, StrangersHug, Backflip, BartenderDances, SecurityNods, StrobeGuy,
        Rewind, FistPump, AirHorn, DropItLean, B2BHighFive, TowelThrow, HeadphonePose, SipDrink, ExtendedSet,
        FanSelfie, CameraCrew, LightsCut,
        BucketHat, ShoeFlies, CongaLine, SuitFistPump, CoupleArgue, BalconySleeper, SongRequest, DiscoBallDrop,
        PuddleSlip, Pigeon, Ringtone, CatOnController, CatKnocksDrink, BouncerLetsQueueIn,
        ArmsFolded, EarsCovered, Booo, PlaySomethingElse, DriftToBar, PhonesOut, PointAtSpeaker, CupThrown,
        EngineerGainDown, PromoterClock, CrowdThins, FullWalkout, LoyalFan,
        IdleChat, BarQueue, WaveToStart, SmokePuff, CatSleepsOnSpeaker,
        Count
    };
}

struct ReactionDef
{
    int id;
    const char* name;
    ReactionCategory category;
    float weight;
    float duration;     // seconds
    float cooldown;     // seconds
    const char* sfx;    // crowd SFX cue (may be empty)
    std::function<bool (const MoodState&, const juce::String& character)> eligible;
};

const std::vector<ReactionDef>& reactionDefs();
const ReactionDef* findReaction (int id);

struct ActiveReaction
{
    int id = 0;
    double start = 0.0;
    float duration = 1.0f;
    float seed = 0.0f;
    float progress (double now) const noexcept { return juce::jlimit (0.0f, 1.0f, (float) ((now - start) / duration)); }
};

/** Chooses reactions: hard-wired triggers first, then weighted-random picks among the
    eligible ones. The same reaction never fires twice in a row. */
class ReactionDirector
{
public:
    explicit ReactionDirector (juce::int64 seed = 0x5eed) : rng (seed) {}

    void update (double now, float dt, CrowdMood& mood);
    bool trigger (int id, double now, CrowdMood& mood, bool ignoreCooldown = false);

    const std::vector<ActiveReaction>& getActive() const noexcept { return active; }
    bool isActive (int id) const noexcept;
    const ActiveReaction* find (int id) const noexcept;

    /** Reactions started during the last update() (for SFX and the editor). */
    std::vector<int> startedThisFrame;
    int getLastId() const noexcept { return lastId; }
    int getTotalFired() const noexcept { return totalFired; }

private:
    bool pickFrom (std::initializer_list<int> ids, double now, CrowdMood& mood);
    std::vector<ActiveReaction> active;
    std::array<double, rx::Count> lastFired {};
    double nextRandomAt = 2.0;
    int lastId = 0, totalFired = 0;
    bool thinnedThisStreak = false, walkoutThisStreak = false, hypeStreakUsed = false, lightsAreCut = false;
    juce::Random rng;
};
} // namespace bgdj
