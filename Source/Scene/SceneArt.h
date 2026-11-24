#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Reactions.h"

namespace bgdj
{
/** Everything the renderer needs to know for one frame. */
struct SceneFrame
{
    double time = 0.0;
    float dt = 1.0f / 60.0f;
    float beatPhase = 0.0f;    // 0..1 within the beat
    int beatIndex = 0;         // beats since start (for bar position)
    bool newBeat = false;
    float bpm = 120.0f;
    const MoodState* mood = nullptr;
    const ReactionDirector* director = nullptr;
    float energy = 0.0f, sub = 0.0f, flux = 0.0f;
    int recentNotes = 0;
    float parallax = 0.0f;     // -1..1 from the mouse
    int quality = 1;
    CrowdPersonality personality = CrowdPersonality::Warehouse;
    float macroBumpGlow = 0.0f;
};

/** Procedural 2.5D club: layered parallax venue, low-poly crowd, DJ booth seen from behind
    the decks, volumetric beams, fog cards and strobes. All art is original and drawn in code. */
class ClubRenderer
{
public:
    void init (int quality, uint32_t seed);
    void update (const SceneFrame& f);
    void draw (juce::Graphics& g, juce::Rectangle<float> area, const SceneFrame& f);
    int numDancers() const noexcept { return (int) dancers.size(); }

private:
    struct Pose
    {
        float bob = 0, lean = 0, jump = 0, crouch = 0, headTurn = 0;
        float armL0 = 0.3f, armL1 = 0.3f, armR0 = -0.3f, armR1 = -0.3f;   // shoulder/elbow angles (radians from straight down)
        float legSpread = 0.15f, kick = 0;
        bool phone = false, flashlight = false, earsCovered = false, armsFolded = false, pointing = false;
        float alpha = 1.0f;
    };

    enum class Role { Normal, BucketHat, Suit, LoyalFan, CoupleA, CoupleB, Shuffler, Flipper, Rider, Slipper, Requester };

    struct Dancer
    {
        float x = 0.5f, z = 0.5f, homeX = 0.5f, homeZ = 0.5f;
        float phase = 0.0f, speed = 1.0f, height = 1.0f, presence = 1.0f, groupAngle = 0.0f;
        juce::Colour shirt, hair, skin, pants;
        int hairStyle = 0;
        Role role = Role::Normal;
        bool leaving = false, atBar = false;
        float rand = 0.0f;
        Pose pose;
    };

    struct Particle { float x, y, vx, vy, life, size; juce::Colour c; int kind; };

    void computePose (Dancer& d, int index, const SceneFrame& f);
    void drawVenue (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f);
    void drawBeams (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f);
    void drawCrowd (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f);
    void drawPerson (juce::Graphics& g, float fx, float fy, float h, const Pose& p, juce::Colour shirt, juce::Colour pants,
                     juce::Colour skin, juce::Colour hair, int hairStyle, bool silhouette, int extra);
    void drawBooth (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f);
    void drawHands (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f);
    void drawActors (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f);
    void drawOverlays (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f);
    void drawBubble (juce::Graphics& g, juce::Point<float> at, const juce::String& text, juce::Colour c, float scale);
    void drawCat (juce::Graphics& g, juce::Point<float> at, float size, int pose, double t);
    juce::Point<float> floorToScreen (juce::Rectangle<float> r, float x, float z, float parallax) const;
    float personHeight (juce::Rectangle<float> r, float z) const;
    float activeProgress (const SceneFrame& f, int id) const;   // -1 if not active

    std::vector<Dancer> dancers;
    std::vector<Particle> particles;
    juce::Random rng;
    float beamAngle = 0.0f, fogPhase = 0.0f, strobe = 0.0f, discoDrop = 0.0f, crowdTurn = 0.0f;
    float lightsCut = 0.0f, houseLights = 0.0f;
    int qualityLevel = 1;
};
} // namespace bgdj
