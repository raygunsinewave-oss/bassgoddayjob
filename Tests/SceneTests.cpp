#include "TestHelpers.h"
#include "Scene/Reactions.h"

namespace bgdj::test
{
static AudioFeatures makeFeatures (float rmsDb, float sub, float mud, float harsh, float flux, int notes, float clip = 0.0f, float subCorr = 1.0f)
{
    AudioFeatures f;
    f.blockSec = 1.0f / 60.0f;
    f.rms = std::pow (10.0f, rmsDb / 20.0f);
    f.sub = sub; f.mud = mud; f.mid = 0.05f; f.harsh = harsh; f.air = 0.01f;
    f.flux = flux; f.noteOns = notes; f.activeVoices = 1; f.hostPlaying = true; f.bpm = 120.0;
    f.clipFraction = clip; f.subCorrelation = subCorr;
    f.secondsSinceNoteOn = notes > 0 ? 0.0 : 0.3;
    return f;
}

struct SceneTests : public juce::UnitTest
{
    SceneTests() : juce::UnitTest ("Crowd mood and reactions", "Scene") {}

    void run (CrowdMood& m, ReactionDirector& d, double& t, double seconds, const AudioFeatures& f, std::set<int>* seen = nullptr, int noteEvery = 15)
    {
        int frame = 0;
        int last = d.getLastId();
        for (double s = 0; s < seconds; s += 1.0 / 60.0, ++frame)
        {
            auto ff = f;
            ff.noteOns = (noteEvery > 0 && frame % noteEvery == 0) ? f.noteOns : 0;
            m.addFeatures (ff);
            m.tick (1.0f / 60.0f);
            t += 1.0 / 60.0;
            d.update (t, 1.0f / 60.0f, m);
            for (int id : d.startedThisFrame)
            {
                expect (id != last, "reaction repeated back-to-back: " + juce::String (id));
                last = id;
                if (seen) seen->insert (id);
            }
        }
    }

    void runTest() override
    {
        beginTest ("60 reactions, ids 1..60, all named");
        expectEquals ((int) reactionDefs().size(), 60);
        std::set<int> ids;
        for (auto& r : reactionDefs()) { ids.insert (r.id); expect (juce::String (r.name).isNotEmpty()); }
        expectEquals ((int) ids.size(), 60);
        expectEquals (*ids.begin(), 1);
        expectEquals (*ids.rbegin(), 60);

        beginTest ("a loud, grooving, sub-heavy set builds hype and positive reactions");
        {
            CrowdMood m; ReactionDirector d (42); double t = 0;
            std::set<int> seen;
            run (m, d, t, 60.0, makeFeatures (-12.0f, 0.5f, 0.05f, 0.03f, 0.8f, 1), &seen);
            expectGreaterThan (m.get().hype, 70.0f);
            int positive = 0;
            for (int id : seen) if (auto* r = findReaction (id); r && (r->category == ReactionCategory::Positive || r->category == ReactionCategory::Dj)) ++positive;
            expectGreaterThan (positive, 3);
        }

        beginTest ("muddy low-mids fold arms; clipping covers ears / brings the engineer; phase issues get pointed at");
        {
            CrowdMood m; ReactionDirector d (7); double t = 0;
            std::set<int> seen;
            run (m, d, t, 20.0, makeFeatures (-12.0f, 0.05f, 0.6f, 0.02f, 0.3f, 1), &seen);
            expect (seen.count (rx::ArmsFolded) > 0, "mud should trigger reaction 43");
            seen.clear();
            run (m, d, t, 20.0, makeFeatures (-3.0f, 0.3f, 0.1f, 0.1f, 0.3f, 1, 0.02f), &seen);
            expect (seen.count (rx::EarsCovered) > 0 || seen.count (rx::EngineerGainDown) > 0, "clipping should trigger 44 or 51");
            seen.clear();
            run (m, d, t, 20.0, makeFeatures (-12.0f, 0.5f, 0.05f, 0.02f, 0.3f, 1, 0.0f, -0.6f), &seen);
            expect (seen.count (rx::PointAtSpeaker) > 0, "negative sub correlation should trigger 49");
        }

        beginTest ("a held, boring note bores the crowd; long low hype ends in a walkout, then recovers");
        {
            CrowdMood m; ReactionDirector d (9); double t = 0;
            auto boring = makeFeatures (-40.0f, 0.1f, 0.3f, 0.0f, 0.0f, 0);
            boring.sameNoteSeconds = 30.0;
            boring.secondsSinceNoteOn = 30.0;
            std::set<int> seen;
            run (m, d, t, 100.0, boring, &seen, 0);
            expectGreaterThan (m.get().boredom, 50.0f);
            expect (seen.count (rx::CrowdThins) > 0, "hype < 10 for 32 bars should thin the crowd (53)");
            expect (seen.count (rx::FullWalkout) > 0 && m.get().walkout, "then a full walkout (54)");
            run (m, d, t, 40.0, makeFeatures (-10.0f, 0.5f, 0.05f, 0.02f, 0.8f, 1));
            expect (! m.get().walkout, "playing something good recovers from the walkout");
        }

        beginTest ("idle crowd does idle things");
        {
            CrowdMood m; ReactionDirector d (11); double t = 0;
            AudioFeatures idle;
            idle.blockSec = 1.0f / 60.0f; idle.rms = 0.0f; idle.secondsSinceNoteOn = 100.0; idle.activeVoices = 0; idle.hostPlaying = false;
            std::set<int> seen;
            run (m, d, t, 60.0, idle, &seen, 0);
            int ambient = 0;
            for (int id : seen) if (id >= rx::IdleChat && id <= rx::CatSleepsOnSpeaker) ++ambient;
            expectGreaterThan (ambient, 1);
        }
    }
};

static SceneTests sceneTests;
} // namespace bgdj::test
