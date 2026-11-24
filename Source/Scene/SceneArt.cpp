#include "SceneArt.h"

namespace bgdj
{
namespace
{
    using juce::Colour;
    const float kPi = juce::MathConstants<float>::pi;

    struct Look { Colour skyTop, skyBottom, wall, trim, floorA, floorB, haze, beamA, beamB, rim; };

    Look lookFor (CrowdPersonality p)
    {
        switch (p)
        {
            case CrowdPersonality::Festival:            return { Colour (0xff1b0a3a), Colour (0xff5a1360), Colour (0xff2a1440), Colour (0xffb08a3a), Colour (0xff2a1a3c), Colour (0xff3f2a55), Colour (0xffff3cc8), Colour (0xffff2bd6), Colour (0xff2ef2ff), Colour (0xff2ef2ff) };
            case CrowdPersonality::UndergroundBasement: return { Colour (0xff0a0406), Colour (0xff2a070c), Colour (0xff1c0b0e), Colour (0xff6a3020), Colour (0xff140a0c), Colour (0xff24121a), Colour (0xffff2050), Colour (0xffff2050), Colour (0xffff7a2b), Colour (0xffff4060) };
            case CrowdPersonality::BeachBar:            return { Colour (0xff10324a), Colour (0xff5a3a5a), Colour (0xff3a2a3a), Colour (0xffd0a050), Colour (0xff3a2c30), Colour (0xff5a4040), Colour (0xffffa060), Colour (0xff19d3b0), Colour (0xffffb84a), Colour (0xff19d3b0) };
            case CrowdPersonality::Afterhours:          return { Colour (0xff050616), Colour (0xff1a0d3a), Colour (0xff110f26), Colour (0xff5a5a9a), Colour (0xff0e0c1e), Colour (0xff1c1838), Colour (0xff6a3cff), Colour (0xff6a3cff), Colour (0xff19d3b0), Colour (0xff8a6cff) };
            case CrowdPersonality::Warehouse:
            default:                                    return { Colour (0xff0f0716), Colour (0xff3a0f3a), Colour (0xff201428), Colour (0xff9a7a40), Colour (0xff1a1222), Colour (0xff2d2038), Colour (0xffff2bd6), Colour (0xffff2bd6), Colour (0xff2ef2ff), Colour (0xff2ef2ff) };
        }
    }

    juce::Path starPath (juce::Point<float> c, float r)
    {
        juce::Path p;
        p.addStar (c, 5, r * 0.45f, r, -kPi / 2.0f + kPi);
        return p;
    }

    const Colour hairColours[] = { Colour (0xffff3cc8), Colour (0xff2ef2ff), Colour (0xff7cff4d), Colour (0xffffd84a), Colour (0xff1a1a1a),
                                   Colour (0xff6b3a1a), Colour (0xffe8e0d0), Colour (0xffff6a2b), Colour (0xff9a5cff), Colour (0xff3a2a1a) };
    const Colour shirtColours[] = { Colour (0xff2a2a3a), Colour (0xffd02a6a), Colour (0xff2a8ad0), Colour (0xfff0f0f0), Colour (0xff1a1a1a),
                                    Colour (0xff3ad09a), Colour (0xffe0c030), Colour (0xff8a3ad0), Colour (0xffd06a2a), Colour (0xff404a5a) };
    const Colour skinColours[] = { Colour (0xfff1c9a5), Colour (0xffd9a77c), Colour (0xffb5835a), Colour (0xff8d5b3a), Colour (0xff5e3a22), Colour (0xffe8b896) };
}

// ============================================================================
void ClubRenderer::init (int quality, uint32_t seed)
{
    qualityLevel = juce::jlimit (0, 2, quality);
    rng.setSeed ((juce::int64) seed);
    const int count = qualityLevel == 0 ? 40 : (qualityLevel == 1 ? 90 : 150);
    dancers.clear();
    for (int i = 0; i < count; ++i)
    {
        Dancer d;
        d.homeX = 0.06f + rng.nextFloat() * 0.88f;
        d.homeZ = std::pow (rng.nextFloat(), 0.8f);
        d.x = d.homeX; d.z = d.homeZ;
        d.phase = rng.nextFloat();
        d.speed = 0.85f + rng.nextFloat() * 0.3f;
        d.height = 0.9f + rng.nextFloat() * 0.2f;
        d.shirt = shirtColours[rng.nextInt (10)];
        d.pants = Colour (0xff1a1a24).interpolatedWith (d.shirt, 0.15f);
        d.skin = skinColours[rng.nextInt (6)];
        d.hair = hairColours[rng.nextInt (10)];
        d.hairStyle = rng.nextInt (5);
        d.rand = rng.nextFloat();
        dancers.push_back (d);
    }
    // characters with a story
    auto assign = [this] (int idx, Role r, float x, float z)
    {
        if (idx < (int) dancers.size()) { dancers[(size_t) idx].role = r; dancers[(size_t) idx].homeX = x; dancers[(size_t) idx].homeZ = z; dancers[(size_t) idx].x = x; dancers[(size_t) idx].z = z; }
    };
    assign (0, Role::LoyalFan, 0.5f, 0.82f);
    assign (1, Role::BucketHat, 0.3f, 0.62f);
    assign (2, Role::Suit, 0.7f, 0.58f);
    assign (3, Role::CoupleA, 0.2f, 0.35f);
    assign (4, Role::CoupleB, 0.23f, 0.36f);
    assign (5, Role::Flipper, 0.55f, 0.5f);
    assign (6, Role::Rider, 0.42f, 0.45f);
    assign (7, Role::Slipper, 0.62f, 0.7f);
    assign (8, Role::Requester, 0.47f, 0.92f);
    for (int i = 9; i < 15 && i < (int) dancers.size(); ++i) dancers[(size_t) i].role = Role::Shuffler;
    dancers[1].hair = Colour (0xffe0d080);
    dancers[2].shirt = Colour (0xff2a2e3a);
    particles.clear();
}

float ClubRenderer::activeProgress (const SceneFrame& f, int id) const
{
    if (f.director == nullptr) return -1.0f;
    if (auto* a = f.director->find (id))
        return a->progress (f.time);
    return -1.0f;
}

juce::Point<float> ClubRenderer::floorToScreen (juce::Rectangle<float> r, float x, float z, float parallax) const
{
    const float y = r.getY() + r.getHeight() * (0.47f + 0.30f * std::pow (juce::jlimit (0.0f, 1.0f, z), 1.15f));
    const float spread = 0.55f + 0.6f * z;
    const float px = r.getCentreX() + (x - 0.5f) * r.getWidth() * spread + parallax * r.getWidth() * 0.012f * (0.3f + z);
    return { px, y };
}

float ClubRenderer::personHeight (juce::Rectangle<float> r, float z) const
{
    return r.getHeight() * (0.12f + 0.17f * z);
}

// ============================================================================
void ClubRenderer::update (const SceneFrame& f)
{
    if (f.mood == nullptr) return;
    const auto& m = *f.mood;
    const float dt = f.dt;

    beamAngle += dt * (0.3f + f.energy * 1.4f);
    fogPhase += dt * 0.05f;
    strobe = std::max (0.0f, strobe - dt * 9.0f);
    const bool strobeGuy = activeProgress (f, rx::StrobeGuy) >= 0.0f;
    if ((f.newBeat && m.hype > 80.0f && m.groove > 0.45f) || (strobeGuy && std::fmod (f.beatPhase * 4.0f, 1.0f) < 0.15f))
        strobe = 1.0f;

    const float dp = activeProgress (f, rx::DiscoBallDrop);
    discoDrop = dp >= 0.0f ? std::min (1.0f, discoDrop + dt * 20.0f) : std::max (0.0f, discoDrop - dt * 0.5f);
    const float ring = activeProgress (f, rx::Ringtone);
    crowdTurn += ((ring >= 0.0f && ring < 0.8f ? 1.0f : 0.0f) - crowdTurn) * std::min (1.0f, dt * 4.0f);
    lightsCut += ((activeProgress (f, rx::LightsCut) >= 0.0f ? 1.0f : 0.0f) - lightsCut) * std::min (1.0f, dt * 12.0f);
    houseLights += ((m.walkout ? 1.0f : 0.0f) - houseLights) * std::min (1.0f, dt * 1.5f);

    // ---- crowd movement ----------------------------------------------------------------
    const bool drift = activeProgress (f, rx::DriftToBar) >= 0.0f || activeProgress (f, rx::BarQueue) >= 0.0f;
    const bool thin = activeProgress (f, rx::CrowdThins) >= 0.0f;
    const float conga = activeProgress (f, rx::CongaLine);
    const float queueIn = activeProgress (f, rx::BouncerLetsQueueIn);
    const float mosh = activeProgress (f, rx::MoshPit);
    const int n = (int) dancers.size();

    for (int i = 0; i < n; ++i)
    {
        auto& d = dancers[(size_t) i];
        const bool loyal = d.role == Role::LoyalFan;
        float presTarget = loyal ? 1.0f : ((float) i / (float) n < m.crowdSize ? 1.0f : 0.0f);
        if (m.walkout && ! loyal) presTarget = 0.0f;
        if (thin && d.rand < 0.35f && ! loyal) presTarget = 0.0f;
        const bool peeking = ! m.walkout && houseLights > 0.05f;   // recovering: a few peek back in first
        if (peeking && d.rand > 0.15f && ! loyal) presTarget = 0.0f;

        d.leaving = presTarget < d.presence - 0.01f;
        float tx = d.homeX, tz = d.homeZ;
        if (d.leaving || d.presence < 0.98f)
        {
            tx = 0.92f; tz = 0.12f;   // exit door
        }
        if (presTarget > 0.5f && d.presence < 0.99f && queueIn >= 0.0f)
            d.presence = std::min (d.presence, 0.3f);
        if (drift && d.rand < 0.3f && ! loyal)
        {
            tx = 0.08f + d.rand * 0.15f;
            tz = 0.15f + d.rand * 1.4f;
            d.atBar = true;
        }
        else
            d.atBar = false;
        if (conga >= 0.0f && i % 5 == 0 && ! d.leaving)
        {
            const float k = (float) i / (float) n;
            tx = 0.5f + 0.33f * std::sin ((float) f.time * 0.6f - k * 6.0f);
            tz = 0.5f + 0.25f * std::cos ((float) f.time * 0.6f - k * 6.0f);
        }
        if (mosh >= 0.0f && std::abs (d.homeX - 0.5f) < 0.18f && std::abs (d.homeZ - 0.55f) < 0.2f)
        {
            const float ang = d.rand * 6.28f + (float) f.time * 2.0f;
            const float rad = 0.05f + 0.12f * std::abs (std::sin ((float) f.time * 3.0f + d.rand * 9.0f));
            tx = 0.5f + rad * std::cos (ang);
            tz = 0.55f + rad * std::sin (ang);
        }

        const float moveSpeed = d.leaving ? 0.25f : 0.45f;
        d.x += juce::jlimit (-moveSpeed * dt, moveSpeed * dt, tx - d.x);
        d.z += juce::jlimit (-moveSpeed * dt, moveSpeed * dt, tz - d.z);
        const float presRate = presTarget > d.presence ? 0.5f : (std::abs (d.x - 0.92f) < 0.05f ? 1.5f : 0.15f);
        d.presence += juce::jlimit (-presRate * dt, presRate * dt, presTarget - d.presence);
        computePose (d, i, f);
    }

    // ---- particles ----------------------------------------------------------------------
    if (f.director != nullptr)
        for (int id : f.director->startedThisFrame)
        {
            if (id == rx::Confetti)
                for (int k = 0; k < (qualityLevel + 1) * 80; ++k)
                    particles.push_back ({ rng.nextFloat(), -0.05f, (rng.nextFloat() - 0.5f) * 0.2f, rng.nextFloat() * 0.2f, 3.0f + rng.nextFloat() * 2.0f,
                                           3.0f + rng.nextFloat() * 3.0f, hairColours[rng.nextInt (10)], 0 });
            if (id == rx::SmokePuff || id == rx::Confetti)
                for (int k = 0; k < 12; ++k)
                    particles.push_back ({ k < 6 ? 0.05f : 0.95f, 0.75f, (k < 6 ? 0.08f : -0.08f) * rng.nextFloat(), -0.15f - rng.nextFloat() * 0.1f,
                                           2.5f, 40.0f + rng.nextFloat() * 40.0f, Colour (0xffe8e0ff), 1 });
        }
    for (auto& p : particles)
    {
        p.x += p.vx * dt; p.y += p.vy * dt;
        if (p.kind == 0) { p.vy += 0.15f * dt; p.vx += std::sin (p.y * 20.0f) * 0.02f * dt; }
        else { p.size += dt * 30.0f; }
        p.life -= dt;
    }
    particles.erase (std::remove_if (particles.begin(), particles.end(), [] (const Particle& p) { return p.life <= 0.0f || p.y > 1.2f; }), particles.end());
}

void ClubRenderer::computePose (Dancer& d, int index, const SceneFrame& f)
{
    const auto& m = *f.mood;
    Pose p;
    const float beat = f.beatPhase;
    const float energy = m.idle ? 0.15f : juce::jlimit (0.0f, 1.0f, m.hype / 100.0f * 0.8f + f.energy * 0.4f);
    const float ph = beat + d.phase * (m.hype > 60.0f ? 0.15f : 0.6f);
    const float bounce = std::abs (std::sin (kPi * ph)) * energy;
    p.bob = bounce;
    p.lean = std::sin (kPi * 2.0f * (beat * 0.5f + d.phase)) * 0.08f * energy;
    p.armL0 = 0.3f + 0.3f * energy * std::sin (kPi * 2.0f * ph);
    p.armR0 = -0.3f - 0.3f * energy * std::sin (kPi * 2.0f * ph + 1.0f);
    p.armL1 = 0.4f; p.armR1 = -0.4f;
    p.headTurn = crowdTurn * (d.rand > 0.5f ? 1.0f : -1.0f);

    auto prog = [&] (int id) { return activeProgress (f, id); };

    if (m.idle)
    {
        p.bob = 0.1f * std::abs (std::sin ((float) f.time * 1.5f + d.phase * 6.0f));
        p.armL0 = 0.15f; p.armR0 = -0.15f;
        p.headTurn += std::sin ((float) f.time * 0.7f + d.rand * 10.0f) * 0.5f;
    }
    if (float w = prog (rx::HandsUpWave); w >= 0.0f)
    {
        const float wave = std::sin (kPi * juce::jlimit (0.0f, 1.0f, w * 2.5f - d.x * 1.2f));
        p.armL0 = 0.3f + (kPi * 0.95f - 0.3f) * wave; p.armR0 = -p.armL0; p.armL1 = p.armR1 = 0.0f;
    }
    if (float j = prog (rx::SyncJump); j >= 0.0f) p.jump = std::abs (std::sin (kPi * 2.0f * beat)) * 0.25f;
    if (float c = prog (rx::CrouchExplode); c >= 0.0f)
    {
        if (c < 0.4f) { p.crouch = 0.5f; p.bob = 0.0f; }
        else { p.jump = std::abs (std::sin (kPi * 2.0f * beat)) * 0.3f; p.armL0 = 2.8f; p.armR0 = -2.8f; }
    }
    if (prog (rx::MoshPit) >= 0.0f && std::abs (d.homeX - 0.5f) < 0.18f) { p.lean = std::sin ((float) f.time * 8.0f + d.rand * 10.0f) * 0.35f; p.jump = bounce * 0.2f; }
    if ((prog (rx::RaveShuffle) >= 0.0f && d.rand < 0.6f) || (d.role == Role::Shuffler && m.hype > 40.0f))
    {
        p.kick = std::sin (kPi * 4.0f * ph);
        p.armL0 = 0.9f + 0.6f * std::sin (kPi * 4.0f * ph); p.armR0 = -0.9f - 0.6f * std::sin (kPi * 4.0f * ph + 1.0f);
    }
    if (prog (rx::TwoStep) >= 0.0f && d.rand < 0.7f) { p.lean = std::sin (kPi * ph) * 0.2f; p.kick = std::sin (kPi * 2.0f * ph) * 0.5f; }
    if (float fl = prog (rx::Flashlights); fl >= 0.0f && d.rand < 0.75f) { p.flashlight = true; p.armR0 = -2.7f; p.armR1 = 0.0f; p.bob *= 0.3f; }
    if (prog (rx::PhonesOut) >= 0.0f && d.rand < 0.6f) { p.phone = true; p.armR0 = -1.2f; p.armR1 = -1.3f; p.bob = 0.0f; }
    if (prog (rx::ArmsFolded) >= 0.0f && d.rand < 0.85f) { p.armsFolded = true; p.bob = 0.0f; }
    if (prog (rx::EarsCovered) >= 0.0f && d.rand < 0.8f) { p.earsCovered = true; p.bob = 0.0f; }
    if (prog (rx::PointAtSpeaker) >= 0.0f && d.rand < 0.25f) { p.pointing = true; p.armL0 = 1.6f; p.armL1 = 0.0f; p.headTurn = std::sin ((float) f.time * 10.0f) * 0.3f; }
    if (prog (rx::Booo) >= 0.0f && d.rand < 0.5f) { p.armL0 = 1.2f; p.armR0 = -1.2f; p.bob = 0.0f; }
    if (prog (rx::IdleChat) >= 0.0f) p.headTurn = std::sin ((float) f.time * 1.2f + d.rand * 6.0f) * 0.6f;
    if (prog (rx::WaveToStart) >= 0.0f && index == 10) { p.armL0 = 2.6f + std::sin ((float) f.time * 12.0f) * 0.4f; p.armL1 = 0.0f; }
    if (prog (rx::OneMore) >= 0.0f) { p.armR0 = -2.6f; p.armR1 = 0.0f; p.jump = std::max (p.jump, (beat < 0.3f ? 0.08f : 0.0f)); }

    switch (d.role)
    {
        case Role::BucketHat:
            p.lean = std::sin ((float) f.time * 3.1f) * 0.25f; p.kick = std::sin ((float) f.time * 5.0f);
            p.armL0 = 1.5f + std::sin ((float) f.time * 4.0f); p.armR0 = -0.2f - std::cos ((float) f.time * 3.0f);
            break;
        case Role::Suit:
            if (prog (rx::SuitFistPump) >= 0.0f || m.hype > 70.0f) { p.armR0 = beat < 0.5f ? -2.9f : -2.2f; p.armR1 = 0.0f; p.jump = beat < 0.25f ? 0.06f : 0.0f; }
            break;
        case Role::Flipper:
            if (float b = prog (rx::Backflip); b >= 0.0f && b < 0.5f) { p.jump = std::sin (kPi * b * 2.0f) * 0.45f; p.crouch = 0.2f; }
            break;
        case Role::Slipper:
            if (float s = prog (rx::PuddleSlip); s >= 0.0f) { p.lean = s < 0.4f ? -0.9f * (s / 0.4f) : 0.0f; if (s > 0.5f) { p.armL0 = 2.4f; p.armR0 = -1.0f; p.kick = 0.8f; } }
            break;
        case Role::CoupleA: case Role::CoupleB:
            if (prog (rx::StrangersHug) >= 0.0f || prog (rx::CoupleArgue) > 0.65f) { p.lean = d.role == Role::CoupleA ? 0.25f : -0.25f; p.armL0 = 1.2f; p.armR0 = -1.2f; }
            else if (prog (rx::CoupleArgue) >= 0.0f) { p.armL0 = 1.0f + std::sin ((float) f.time * 9.0f) * 0.5f; p.bob = 0.0f; }
            break;
        case Role::Requester:
            if (prog (rx::SongRequest) >= 0.0f) { p.armR0 = -2.4f; p.armR1 = 0.2f; p.bob = 0.0f; }
            break;
        default: break;
    }
    if (d.leaving) { p.bob = 0.05f * std::abs (std::sin ((float) f.time * 6.0f)); p.armL0 = 0.1f; p.armR0 = -0.1f; }
    p.alpha = juce::jlimit (0.0f, 1.0f, d.presence);
    d.pose = p;
}

// ============================================================================
void ClubRenderer::draw (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    if (f.mood == nullptr) return;
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (r.toNearestInt());

    // camera: crew pans, "drop it" leans in, beat sway
    float pan = 0.0f, zoom = 1.0f;
    if (float c = activeProgress (f, rx::CameraCrew); c >= 0.0f) pan = std::sin (c * kPi * 2.0f) * 0.04f;
    if (float l = activeProgress (f, rx::DropItLean); l >= 0.0f) zoom = 1.0f + 0.06f * std::sin (kPi * l);
    const float sway = f.mood->hype > 50.0f ? std::sin (kPi * 2.0f * f.beatPhase) * 0.004f * f.mood->hype / 100.0f : 0.0f;
    g.addTransform (juce::AffineTransform::scale (zoom, zoom, r.getCentreX(), r.getBottom())
                        .translated (r.getWidth() * (pan + sway), 0.0f));

    drawVenue (g, r, f);
    drawCrowd (g, r, f);
    drawBeams (g, r, f);
    drawActors (g, r, f);
    drawBooth (g, r, f);
    drawHands (g, r, f);
    drawOverlays (g, r, f);
}

void ClubRenderer::drawVenue (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    const auto L = lookFor (f.personality);
    const float W = r.getWidth(), H = r.getHeight(), X = r.getX(), Y = r.getY();
    const float px = f.parallax * W * 0.006f;
    const float pulse = 0.5f + 0.5f * std::sin (kPi * 2.0f * f.beatPhase);

    g.setGradientFill (juce::ColourGradient (L.skyTop, X, Y, L.skyBottom, X, Y + H * 0.5f, false));
    g.fillRect (r);

    // ---- back wall with three grand arches -----------------------------------------------
    const float wallTop = Y + H * 0.08f, wallBot = Y + H * 0.48f;
    for (int a = 0; a < 3; ++a)
    {
        const float cx = X + W * (0.25f + 0.25f * (float) a) + px;
        const float aw = W * 0.19f, top = wallTop + H * 0.04f;
        juce::Path arch;
        arch.startNewSubPath (cx - aw * 0.5f, wallBot);
        arch.lineTo (cx - aw * 0.5f, top + aw * 0.5f);
        arch.addCentredArc (cx, top + aw * 0.5f, aw * 0.5f, aw * 0.5f, 0.0f, -kPi * 0.5f, kPi * 0.5f);
        arch.lineTo (cx + aw * 0.5f, wallBot);
        arch.closeSubPath();
        g.setGradientFill (juce::ColourGradient (L.wall.darker (0.6f), cx, top, L.haze.withAlpha (0.35f), cx, wallBot, false));
        g.fillPath (arch);
        g.setColour (L.trim.withAlpha (0.55f));
        g.strokePath (arch, juce::PathStrokeType (2.0f));
        // ornate keystone + columns
        g.fillRect (cx - 4.0f, top - 2.0f, 8.0f, 9.0f);
        g.setColour (L.trim.withAlpha (0.25f));
        for (float s : { -0.5f, 0.5f })
            g.fillRect (cx + s * aw - 4.0f, top + aw * 0.5f, 8.0f, wallBot - top - aw * 0.5f);
        // peeling paint (run-down)
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (cx - aw * 0.3f, top + aw * 0.7f, aw * 0.12f, aw * 0.07f);
    }

    // ---- balcony mezzanine ------------------------------------------------------------------------
    const float balY = Y + H * 0.2f;
    g.setColour (L.wall.brighter (0.1f));
    g.fillRect (X, balY, W, H * 0.035f);
    g.setColour (L.trim.withAlpha (0.6f));
    g.drawHorizontalLine ((int) balY, X, X + W);
    g.drawHorizontalLine ((int) (balY - H * 0.045f), X, X + W);
    for (float x = X + px; x < X + W; x += W * 0.022f)
        g.drawVerticalLine ((int) x, balY - H * 0.045f, balY);
    // balcony silhouettes leaning on the rail
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    for (int i = 0; i < 9; ++i)
    {
        const float bx = X + W * (0.08f + 0.1f * (float) i) + px * 1.5f;
        const float bob = std::sin (kPi * 2.0f * (f.beatPhase + (float) i * 0.13f)) * 1.5f * f.energy;
        g.fillEllipse (bx, balY - H * 0.08f + bob, H * 0.035f, H * 0.035f);
        g.fillRoundedRectangle (bx - H * 0.008f, balY - H * 0.05f + bob, H * 0.05f, H * 0.05f, 3.0f);
    }

    // ---- ceiling orb lights ------------------------------------------------------------------------
    for (int i = 0; i < 9; ++i)
    {
        const float ox = X + W * (0.06f + 0.11f * (float) i) + px * 0.5f, oy = Y + H * 0.045f;
        const float rr = H * 0.022f * (1.0f + 0.25f * pulse * f.energy);
        const auto c = (i % 2 == 0 ? L.beamA : L.beamB);
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.5f), ox, oy, c.withAlpha (0.0f), ox + rr * 3.0f, oy, true));
        g.fillEllipse (ox - rr * 3.0f, oy - rr * 3.0f, rr * 6.0f, rr * 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillEllipse (ox - rr * 0.5f, oy - rr * 0.5f, rr, rr);
    }

    // ---- neon star signs ---------------------------------------------------------------------------
    for (int s = 0; s < 2; ++s)
    {
        const float sx = X + W * (s == 0 ? 0.13f : 0.87f) + px, sy = Y + H * 0.3f;
        const bool flicker = std::fmod ((float) f.time * (3.7f + (float) s), 7.0f) < 0.08f;
        const auto c = (s == 0 ? L.beamB : L.beamA).withAlpha (flicker ? 0.2f : 0.95f);
        auto star = starPath ({ sx, sy }, H * 0.055f);
        g.setColour (c.withAlpha (0.25f));
        g.strokePath (star, juce::PathStrokeType (7.0f));
        g.setColour (c);
        g.strokePath (star, juce::PathStrokeType (2.0f));
    }

    // ---- bar along the left side with glowing bottles -------------------------------------------------
    {
        juce::Path bar;
        bar.addQuadrilateral (X, Y + H * 0.36f, X + W * 0.15f + px, Y + H * 0.43f, X + W * 0.17f + px, Y + H * 0.5f, X, Y + H * 0.6f);
        g.setColour (Colour (0xff2a1810));
        g.fillPath (bar);
        g.setColour (L.beamA.withAlpha (0.7f));
        g.drawLine (X, Y + H * 0.36f, X + W * 0.15f + px, Y + H * 0.43f, 2.0f);   // neon strip along the counter
        for (int row = 0; row < 3; ++row)
            for (int b = 0; b < 8; ++b)
            {
                const float bx = X + W * 0.01f + (float) b * W * 0.016f + px * 0.8f;
                const float by = Y + H * (0.2f + 0.045f * (float) row) + (float) b * H * 0.006f;
                const auto bc = hairColours[(b + row * 3) % 10].withAlpha (0.85f);
                g.setColour (bc.withAlpha (0.25f));
                g.fillEllipse (bx - 2.0f, by - 2.0f, W * 0.012f, H * 0.06f);
                g.setColour (bc);
                g.fillRoundedRectangle (bx, by, W * 0.007f, H * 0.035f, 1.5f);
            }
        // bartender
        Pose bp;
        const float bartenderDance = activeProgress (f, rx::BartenderDances);
        if (bartenderDance >= 0.0f) { bp.bob = std::abs (std::sin (kPi * 2.0f * f.beatPhase)); bp.armL0 = 2.5f; bp.armR0 = -2.5f + std::sin ((float) f.time * 8.0f); }
        else { bp.armR0 = -1.3f; bp.armR1 = -1.2f; }
        drawPerson (g, X + W * 0.08f + px, Y + H * 0.42f, H * 0.16f, bp, Colour (0xff111111), Colour (0xff111111), skinColours[2], Colour (0xff1a1a1a), 1, false, 0);
    }

    // ---- exit door -------------------------------------------------------------------------------------
    {
        const auto door = juce::Rectangle<float> (X + W * 0.86f + px, Y + H * 0.3f, W * 0.06f, H * 0.17f);
        const bool open = houseLights > 0.1f || activeProgress (f, rx::CrowdThins) >= 0.0f || activeProgress (f, rx::BouncerLetsQueueIn) >= 0.0f;
        g.setColour (open ? Colour (0xfffff2c0).withAlpha (0.7f) : Colour (0xff120a14));
        g.fillRect (door);
        g.setColour (Colour (0xff3aff7a));
        g.fillRect (door.getX(), door.getY() - H * 0.035f, door.getWidth(), H * 0.03f);
        g.setColour (juce::Colours::black);
        g.setFont (juce::Font (juce::FontOptions (H * 0.026f, juce::Font::bold)));
        g.drawText ("EXIT", juce::Rectangle<float> (door.getX(), door.getY() - H * 0.035f, door.getWidth(), H * 0.03f), juce::Justification::centred);
        // security guard by the door
        Pose sp;
        if (activeProgress (f, rx::SecurityNods) >= 0.0f) sp.bob = std::abs (std::sin (kPi * 2.0f * f.beatPhase)) * 0.6f;
        sp.armsFolded = true;
        drawPerson (g, door.getX() - W * 0.02f, Y + H * 0.5f, H * 0.2f, sp, Colour (0xff141414), Colour (0xff141414), skinColours[4], Colour (0xff101010), 3, false, 1);
    }

    // ---- speaker stacks --------------------------------------------------------------------------------
    for (int s = 0; s < 2; ++s)
    {
        const float sx = s == 0 ? X + W * 0.005f : X + W * 0.925f;
        const auto box = juce::Rectangle<float> (sx, Y + H * 0.4f, W * 0.07f, H * 0.38f);
        g.setColour (Colour (0xff121016));
        g.fillRect (box);
        g.setColour (Colour (0xff2a2430));
        g.drawRect (box, 1.5f);
        for (int k = 0; k < 2; ++k)
        {
            const float cone = box.getWidth() * (0.3f + 0.03f * f.sub * (k == 1 ? 3.0f : 1.0f));
            const auto cc = juce::Point<float> (box.getCentreX(), box.getY() + box.getHeight() * (0.28f + 0.45f * (float) k));
            g.setColour (Colour (0xff050407));
            g.fillEllipse (cc.x - cone, cc.y - cone, cone * 2.0f, cone * 2.0f);
            g.setColour (L.rim.withAlpha (0.3f + 0.4f * f.sub));
            g.drawEllipse (cc.x - cone, cc.y - cone, cone * 2.0f, cone * 2.0f, 1.2f);
        }
    }

    // ---- perspective checkered dance floor -----------------------------------------------------------------
    const int rows = qualityLevel == 0 ? 8 : 12, cols = qualityLevel == 0 ? 10 : 16;
    for (int zi = 0; zi < rows; ++zi)
        for (int xi = -2; xi < cols + 2; ++xi)
        {
            const float z0 = (float) zi / (float) rows, z1 = (float) (zi + 1) / (float) rows;
            const float x0 = (float) xi / (float) cols, x1 = (float) (xi + 1) / (float) cols;
            auto a = floorToScreen (r, x0, z0, f.parallax), b = floorToScreen (r, x1, z0, f.parallax);
            auto c = floorToScreen (r, x1, z1, f.parallax), d = floorToScreen (r, x0, z1, f.parallax);
            juce::Path q;
            q.addQuadrilateral (a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y);
            const bool odd = ((zi + xi) & 1) != 0;
            auto col = odd ? L.floorA : L.floorB;
            const float glow = std::max (0.0f, std::sin ((float) xi * 0.7f + beamAngle * 2.0f)) * f.energy * 0.25f;
            g.setColour (col.interpolatedWith (L.beamA, glow).withAlpha (1.0f));
            g.fillPath (q);
        }
    // puddles reflecting the lights + debris
    for (int pdl = 0; pdl < 4; ++pdl)
    {
        const auto c = floorToScreen (r, 0.2f + 0.2f * (float) pdl, 0.35f + 0.15f * (float) (pdl % 3), f.parallax);
        const float pw = W * 0.05f, ph = H * 0.015f;
        g.setColour ((pdl % 2 == 0 ? L.beamA : L.beamB).withAlpha (0.25f + 0.2f * pulse * f.energy));
        g.fillEllipse (c.x - pw, c.y - ph, pw * 2.0f, ph * 2.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    for (int k = 0; k < 10; ++k)
    {
        const auto c = floorToScreen (r, std::fmod ((float) k * 0.137f + 0.05f, 1.0f), std::fmod ((float) k * 0.31f, 1.0f), f.parallax);
        g.fillRect (c.x, c.y, 3.0f, 2.0f);
    }

    // ---- disco ball ---------------------------------------------------------------------------------------------------
    {
        const float bx = X + W * 0.5f + px * 0.4f, by = Y + H * 0.14f + discoDrop * H * 0.02f;
        const float br = H * 0.045f;
        g.setColour (juce::Colours::grey);
        g.drawVerticalLine ((int) bx, Y, by - br);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, bx - br * 0.4f, by - br * 0.4f, Colour (0xff6a6a7a), bx + br, by + br, true));
        g.fillEllipse (bx - br, by - br, br * 2.0f, br * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        for (int k = -2; k <= 2; ++k)
        {
            g.drawHorizontalLine ((int) (by + (float) k * br * 0.4f), bx - br * 0.9f, bx + br * 0.9f);
            g.drawVerticalLine ((int) (bx + (float) k * br * 0.4f + std::fmod (beamAngle * 6.0f, br * 0.4f)), by - br * 0.9f, by + br * 0.9f);
        }
        // sparkles thrown around the room
        for (int k = 0; k < (qualityLevel + 1) * 8; ++k)
        {
            const float ang = beamAngle * 0.7f + (float) k * 2.39996f;
            const float dist = W * (0.15f + 0.3f * std::fmod ((float) k * 0.618f, 1.0f));
            g.setColour (juce::Colours::white.withAlpha (0.4f * (0.3f + f.energy)));
            g.fillEllipse (bx + std::cos (ang) * dist, by + std::abs (std::sin (ang)) * H * 0.4f, 2.5f, 2.5f);
        }
    }
}

void ClubRenderer::drawBeams (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    const auto L = lookFor (f.personality);
    const float W = r.getWidth(), H = r.getHeight();
    const float intensity = (f.mood->idle ? 0.15f : 0.25f + 0.6f * f.energy) * (1.0f - lightsCut) * (1.0f - houseLights);
    const int nBeams = qualityLevel == 0 ? 3 : (qualityLevel == 1 ? 5 : 8);
    for (int i = 0; i < nBeams; ++i)
    {
        const float ox = r.getX() + W * (0.1f + 0.8f * (float) i / (float) std::max (1, nBeams - 1));
        const float oy = r.getY() + H * 0.02f;
        const float ang = std::sin (beamAngle * (0.6f + 0.1f * (float) i) + (float) i) * 0.7f;
        const float len = H * 0.85f, spread = 0.07f;
        juce::Path beam;
        beam.startNewSubPath (ox, oy);
        beam.lineTo (ox + std::sin (ang - spread) * len, oy + std::cos (ang - spread) * len);
        beam.lineTo (ox + std::sin (ang + spread) * len, oy + std::cos (ang + spread) * len);
        beam.closeSubPath();
        const auto c = (i % 2 == 0 ? L.beamA : L.beamB);
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.45f * intensity), ox, oy, c.withAlpha (0.0f), ox + std::sin (ang) * len, oy + std::cos (ang) * len, false));
        g.fillPath (beam);
    }

    // fog cards
    const int fogs = qualityLevel == 0 ? 2 : (qualityLevel == 1 ? 4 : 6);
    for (int k = 0; k < fogs; ++k)
    {
        const float fx = r.getX() + W * std::fmod (fogPhase * (0.5f + 0.2f * (float) k) + (float) k * 0.27f, 1.3f) - W * 0.15f;
        const float fy = r.getY() + H * (0.45f + 0.08f * (float) (k % 3));
        const float rad = W * (0.18f + 0.04f * (float) k);
        g.setGradientFill (juce::ColourGradient (L.haze.withAlpha (0.09f + 0.05f * f.energy), fx, fy, L.haze.withAlpha (0.0f), fx + rad, fy, true));
        g.fillEllipse (fx - rad, fy - rad * 0.35f, rad * 2.0f, rad * 0.7f);
    }
    // magenta haze over everything
    g.setGradientFill (juce::ColourGradient (L.haze.withAlpha (0.0f), r.getX(), r.getY() + H * 0.2f, L.haze.withAlpha (0.12f), r.getX(), r.getY() + H * 0.6f, false));
    g.fillRect (r);
}

// ============================================================================
void ClubRenderer::drawPerson (juce::Graphics& g, float fx, float fy, float h, const Pose& p, Colour shirt, Colour pants,
                               Colour skin, Colour hair, int hairStyle, bool silhouette, int extra)
{
    if (p.alpha <= 0.01f) return;
    const float a = p.alpha;
    auto col = [&] (Colour c) { return silhouette ? Colour (0xff3a0610).interpolatedWith (c, 0.08f).withAlpha (a * 0.92f) : c.withAlpha (a); };

    const float crouch = p.crouch * h * 0.25f;
    const float baseY = fy - p.jump * h - crouch * 0.2f;
    const float hipY = baseY - h * 0.45f + crouch - p.bob * h * 0.05f;
    const float shoulderY = hipY - h * 0.33f;
    const float headR = h * 0.09f;
    const float lean = p.lean * h * 0.3f;
    const float shoulderW = h * 0.13f;

    // legs (low-poly quads)
    juce::Path legs;
    const float spread = h * (0.05f + p.legSpread * 0.2f);
    const float kick = p.kick * h * 0.06f;
    legs.addQuadrilateral (fx - spread - kick, baseY, fx - spread + h * 0.05f - kick, baseY, fx + h * 0.01f, hipY, fx - h * 0.07f, hipY);
    legs.addQuadrilateral (fx + spread + kick, baseY, fx + spread - h * 0.05f + kick, baseY, fx - h * 0.01f, hipY, fx + h * 0.07f, hipY);
    g.setColour (col (pants));
    g.fillPath (legs);

    // torso (trapezoid with flat shading)
    juce::Path torso;
    torso.addQuadrilateral (fx - h * 0.08f, hipY, fx + h * 0.08f, hipY, fx + shoulderW + lean, shoulderY, fx - shoulderW + lean, shoulderY);
    g.setColour (col (shirt));
    g.fillPath (torso);
    if (! silhouette)
    {
        juce::Path shade;
        shade.addTriangle (fx + h * 0.08f, hipY, fx + shoulderW + lean, shoulderY, fx + lean * 0.5f, shoulderY);
        g.setColour (juce::Colours::black.withAlpha (0.18f * a));
        g.fillPath (shade);
    }
    if (extra == 1 && ! silhouette)   // SECURITY shirt
    {
        g.setColour (juce::Colours::yellow.withAlpha (0.8f * a));
        g.setFont (juce::Font (juce::FontOptions (h * 0.07f, juce::Font::bold)));
        g.drawText ("SECURITY", juce::Rectangle<float> (fx - h * 0.12f + lean, shoulderY + h * 0.06f, h * 0.24f, h * 0.08f), juce::Justification::centred);
    }

    // arms
    auto drawArm = [&] (float side, float a0, float a1)
    {
        const float sx = fx + side * shoulderW * 0.9f + lean, sy = shoulderY + h * 0.02f;
        const float upper = h * 0.17f, lower = h * 0.16f;
        const float ex = sx + std::sin (a0) * upper, ey = sy + std::cos (a0) * upper;
        const float hx = ex + std::sin (a0 + a1) * lower, hy = ey + std::cos (a0 + a1) * lower;
        g.setColour (col (shirt.darker (0.15f)));
        g.drawLine (sx, sy, ex, ey, h * 0.055f);
        g.setColour (col (skin));
        g.drawLine (ex, ey, hx, hy, h * 0.045f);
        return juce::Point<float> (hx, hy);
    };

    juce::Point<float> handL, handR;
    if (p.armsFolded)
    {
        g.setColour (col (shirt.darker (0.25f)));
        g.fillRoundedRectangle (fx - shoulderW + lean, shoulderY + h * 0.1f, shoulderW * 2.0f, h * 0.07f, h * 0.03f);
    }
    else if (p.earsCovered)
    {
        handL = drawArm (-1.0f, 2.6f, 1.2f);
        handR = drawArm (1.0f, -2.6f, -1.2f);
    }
    else
    {
        handL = drawArm (-1.0f, p.armL0, p.armL1);
        handR = drawArm (1.0f, p.armR0, p.armR1);
    }

    // head (hexagon = low-poly)
    const float hx = fx + lean * 1.15f + p.headTurn * headR * 0.3f, hy = shoulderY - headR * 1.1f;
    juce::Path head;
    for (int k = 0; k < 6; ++k)
    {
        const float ang = (float) k / 6.0f * kPi * 2.0f + kPi / 6.0f;
        const auto pt = juce::Point<float> (hx + std::cos (ang) * headR, hy + std::sin (ang) * headR * 1.1f);
        if (k == 0) head.startNewSubPath (pt); else head.lineTo (pt);
    }
    head.closeSubPath();
    g.setColour (col (skin));
    g.fillPath (head);

    // bold hair
    g.setColour (col (hair));
    switch (hairStyle)
    {
        case 0: g.fillEllipse (hx - headR, hy - headR * 1.25f, headR * 2.0f, headR * 1.1f); break;                                     // crop
        case 1: { juce::Path sp; for (int k = 0; k < 5; ++k) sp.addTriangle (hx - headR + (float) k * headR * 0.5f, hy - headR * 0.6f, hx - headR * 0.8f + (float) k * headR * 0.5f, hy - headR * 2.0f, hx - headR * 0.5f + (float) k * headR * 0.5f, hy - headR * 0.6f); g.fillPath (sp); break; }  // spikes
        case 2: g.fillRoundedRectangle (hx - headR * 1.1f, hy - headR * 1.2f, headR * 2.2f, headR * 2.6f, headR); break;           // long
        case 3: g.fillEllipse (hx - headR * 0.9f, hy - headR * 1.15f, headR * 1.8f, headR * 0.7f); break;                          // buzz
        default: g.fillEllipse (hx - headR * 1.3f, hy - headR * 1.6f, headR * 2.6f, headR * 1.9f); break;                          // afro / puff
    }
    if (extra == 2)   // bucket hat
    {
        g.setColour (col (Colour (0xffe0d080)));
        g.fillRect (hx - headR * 1.5f, hy - headR * 0.9f, headR * 3.0f, headR * 0.35f);
        g.fillRoundedRectangle (hx - headR * 0.9f, hy - headR * 1.6f, headR * 1.8f, headR * 0.9f, headR * 0.3f);
    }
    if (extra == 3 && ! silhouette)   // suit tie
    {
        g.setColour (Colour (0xffd02a3a).withAlpha (a));
        g.fillRect (fx - h * 0.01f + lean, shoulderY, h * 0.02f, h * 0.14f);
    }

    if ((p.phone || p.flashlight) && ! p.armsFolded)
    {
        const auto c = p.flashlight ? Colour (0xfffff8e0) : Colour (0xff4aa8ff);
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.6f * a), handR.x, handR.y, c.withAlpha (0.0f), handR.x + h * 0.25f, handR.y, true));
        g.fillEllipse (handR.x - h * 0.25f, handR.y - h * 0.25f, h * 0.5f, h * 0.5f);
        g.setColour (c.withAlpha (a));
        g.fillRect (handR.x - h * 0.02f, handR.y - h * 0.05f, h * 0.04f, h * 0.06f);
    }
    if (p.pointing)
    {
        g.setColour (col (skin));
        g.drawLine (handL.x, handL.y, handL.x - h * 0.06f, handL.y, h * 0.02f);
    }
}

void ClubRenderer::drawCrowd (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    std::vector<int> order ((size_t) dancers.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = (int) i;
    std::sort (order.begin(), order.end(), [this] (int a, int b) { return dancers[(size_t) a].z < dancers[(size_t) b].z; });

    const float flip = activeProgress (f, rx::Backflip);
    const float rider = activeProgress (f, rx::ShoulderRider);

    for (int idx : order)
    {
        auto& d = dancers[(size_t) idx];
        if (d.presence <= 0.01f) continue;
        const auto pos = floorToScreen (r, d.x, d.z, f.parallax);
        const float h = personHeight (r, d.z) * d.height;
        const bool silhouette = d.z > 0.88f && d.role != Role::Requester && d.role != Role::LoyalFan;
        int extra = 0;
        if (d.role == Role::BucketHat) extra = 2;
        if (d.role == Role::Suit) extra = 3;

        if (d.role == Role::Flipper && flip >= 0.0f && flip < 0.5f)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (-flip * 2.0f * kPi * 2.0f, pos.x, pos.y - h * 0.5f - d.pose.jump * h));
            drawPerson (g, pos.x, pos.y, h, d.pose, d.shirt, d.pants, d.skin, d.hair, d.hairStyle, false, extra);
            continue;
        }
        drawPerson (g, pos.x, pos.y, h, d.pose, d.shirt, d.pants, d.skin, d.hair, d.hairStyle, silhouette, extra);

        if (d.role == Role::Rider && rider >= 0.0f)
        {
            Pose rp;
            rp.armL0 = 2.8f; rp.armR0 = -2.8f + std::sin ((float) f.time * 6.0f) * 0.4f; rp.armL1 = rp.armR1 = 0.0f;
            rp.alpha = d.presence;
            drawPerson (g, pos.x, pos.y - h * 0.62f, h * 0.9f, rp, Colour (0xffff3cc8), Colour (0xff20203a), d.skin, Colour (0xff2ef2ff), 4, false, 0);
        }
        if ((d.role == Role::LoyalFan && f.mood->hype < 25.0f) || (d.role == Role::BucketHat && activeProgress (f, rx::BucketHat) >= 0.0f)
            || (d.role == Role::Suit && activeProgress (f, rx::SuitFistPump) >= 0.0f))
        {
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), pos.x, pos.y - h * 2.0f, juce::Colours::white.withAlpha (0.0f), pos.x, pos.y, false));
            juce::Path cone;
            cone.addTriangle (pos.x, pos.y - h * 2.5f, pos.x - h * 0.4f, pos.y, pos.x + h * 0.4f, pos.y);
            g.fillPath (cone);
        }
    }
}

// ============================================================================
void ClubRenderer::drawBubble (juce::Graphics& g, juce::Point<float> at, const juce::String& text, Colour c, float scale)
{
    const auto font = juce::Font (juce::FontOptions (11.0f * scale, juce::Font::bold));
    const float tw = (float) juce::GlyphArrangement::getStringWidthInt (font, text) + 12.0f * scale;
    auto box = juce::Rectangle<float> (at.x - tw * 0.5f, at.y - 20.0f * scale, tw, 17.0f * scale);
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.fillRoundedRectangle (box, 6.0f * scale);
    juce::Path tail;
    tail.addTriangle (at.x - 4.0f * scale, box.getBottom(), at.x + 4.0f * scale, box.getBottom(), at.x, box.getBottom() + 6.0f * scale);
    g.fillPath (tail);
    g.setColour (c);
    g.setFont (font);
    g.drawText (text, box, juce::Justification::centred);
}

void ClubRenderer::drawCat (juce::Graphics& g, juce::Point<float> at, float s, int pose, double t)
{
    // Grey the cat: original low-poly grey cat. pose 0 = sitting, 1 = walking, 2 = sleeping, 3 = staring
    const Colour grey (0xff8a8a92), dark (0xff5a5a62);
    const float bob = pose == 1 ? std::abs (std::sin ((float) t * 8.0f)) * s * 0.05f : 0.0f;
    if (pose == 2)
    {
        g.setColour (grey);
        g.fillEllipse (at.x - s * 0.6f, at.y - s * 0.35f, s * 1.2f, s * 0.4f);
        g.fillEllipse (at.x + s * 0.3f, at.y - s * 0.45f, s * 0.38f, s * 0.32f);
        g.setColour (dark);
        g.drawLine (at.x - s * 0.6f, at.y - s * 0.15f, at.x - s * 0.9f, at.y - s * 0.05f, s * 0.08f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (juce::Font (juce::FontOptions (s * 0.3f, juce::Font::bold)));
        const float z = std::fmod ((float) t, 2.0f);
        g.drawText ("z", juce::Rectangle<float> (at.x + s * 0.4f + z * s * 0.2f, at.y - s * (0.7f + z * 0.3f), s * 0.3f, s * 0.3f), juce::Justification::centred);
        return;
    }
    juce::Path body;
    body.addEllipse (at.x - s * 0.45f, at.y - s * 0.55f - bob, s * 0.9f, s * 0.55f);
    g.setColour (grey);
    g.fillPath (body);
    // head with ears
    const float hx = at.x + s * 0.35f, hy = at.y - s * 0.75f - bob;
    g.fillEllipse (hx - s * 0.22f, hy - s * 0.2f, s * 0.44f, s * 0.38f);
    juce::Path ears;
    ears.addTriangle (hx - s * 0.2f, hy - s * 0.1f, hx - s * 0.14f, hy - s * 0.35f, hx - s * 0.04f, hy - s * 0.15f);
    ears.addTriangle (hx + s * 0.2f, hy - s * 0.1f, hx + s * 0.14f, hy - s * 0.35f, hx + s * 0.04f, hy - s * 0.15f);
    g.fillPath (ears);
    // eyes (staring = big)
    g.setColour (Colour (0xffd8f060));
    const float er = pose == 3 ? s * 0.07f : s * 0.045f;
    g.fillEllipse (hx - s * 0.11f - er, hy - er, er * 2.0f, er * 2.0f);
    g.fillEllipse (hx + s * 0.11f - er, hy - er, er * 2.0f, er * 2.0f);
    g.setColour (juce::Colours::black);
    g.fillEllipse (hx - s * 0.11f - er * 0.3f, hy - er * 0.8f, er * 0.6f, er * 1.6f);
    g.fillEllipse (hx + s * 0.11f - er * 0.3f, hy - er * 0.8f, er * 0.6f, er * 1.6f);
    // tail
    g.setColour (dark);
    juce::Path tail;
    tail.startNewSubPath (at.x - s * 0.4f, at.y - s * 0.4f - bob);
    tail.quadraticTo (at.x - s * 0.85f, at.y - s * 0.9f, at.x - s * 0.6f + std::sin ((float) t * 3.0f) * s * 0.15f, at.y - s * 1.1f);
    g.strokePath (tail, juce::PathStrokeType (s * 0.09f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    // legs
    for (int k = 0; k < 2; ++k)
        g.fillRect (at.x - s * 0.25f + (float) k * s * 0.4f, at.y - s * 0.12f - bob, s * 0.1f, s * 0.12f + bob);
}

void ClubRenderer::drawActors (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    const float W = r.getWidth(), H = r.getHeight(), X = r.getX(), Y = r.getY();
    const auto pink = Colour (0xffd0207a);

    auto speakerPoint = [&] (int idx) { const auto& d = dancers[(size_t) idx % dancers.size()]; auto p = floorToScreen (r, d.x, d.z, f.parallax); return p.translated (0.0f, -personHeight (r, d.z) * d.height * 1.05f); };

    // crowd speech
    if (float p = activeProgress (f, rx::Booo); p >= 0.0f)
        for (int k = 0; k < 4; ++k) drawBubble (g, speakerPoint (11 + k * 7), "BOOO", Colour (0xff602020), 0.9f);
    if (float p = activeProgress (f, rx::Oooh); p >= 0.0f)
        for (int k = 0; k < 3; ++k) drawBubble (g, speakerPoint (20 + k * 9), "OOOOH", pink, 1.0f);
    if (float p = activeProgress (f, rx::OneMore); p >= 0.0f)
    {
        drawBubble (g, speakerPoint (16), "ONE MORE!", pink, 1.1f);
        if (p > 0.3f) for (int k = 0; k < 3; ++k) if (std::fmod (f.beatPhase + (float) k * 0.3f, 1.0f) < 0.5f) drawBubble (g, speakerPoint (30 + k * 11), "ONE MORE!", pink, 0.8f);
    }
    if (activeProgress (f, rx::PlaySomethingElse) >= 0.0f) drawBubble (g, speakerPoint (19), "PLAY SOMETHING ELSE!", Colour (0xff602020), 1.0f);
    if (activeProgress (f, rx::Rewind) >= 0.0f) for (int k = 0; k < 4; ++k) drawBubble (g, speakerPoint (12 + k * 6), "REWIND!", pink, 1.0f);
    if (activeProgress (f, rx::WaveToStart) >= 0.0f) drawBubble (g, speakerPoint (10), "GO ON THEN!", Colour (0xff205060), 1.0f);
    if (activeProgress (f, rx::DiscoBallDrop) >= 0.0f) for (int k = 0; k < 3; ++k) drawBubble (g, speakerPoint (24 + k * 5), "!!", Colour (0xff602020), 0.8f);
    if (float p = activeProgress (f, rx::CoupleArgue); p >= 0.0f)
        drawBubble (g, speakerPoint (3), p < 0.65f ? "!#@?" : "<3", p < 0.65f ? Colour (0xffc02020) : pink, 0.9f);
    if (activeProgress (f, rx::StrangersHug) >= 0.0f) drawBubble (g, speakerPoint (4), "<3", pink, 0.9f);
    if (activeProgress (f, rx::SongRequest) >= 0.0f)
    {
        auto pt = speakerPoint (8);
        g.setColour (juce::Colours::white);
        g.fillRect (pt.x + 6.0f, pt.y - 10.0f, 46.0f, 24.0f);
        g.setColour (juce::Colours::black);
        g.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
        g.drawFittedText ("PLAY THAT\nONE SONG??", juce::Rectangle<int> ((int) pt.x + 7, (int) pt.y - 9, 44, 22), juce::Justification::centred, 2);
    }
    if (activeProgress (f, rx::Ringtone) >= 0.0f)
    {
        auto pt = speakerPoint (27);
        drawBubble (g, pt, "RING RING", Colour (0xff2050a0), 0.9f);
    }
    if (activeProgress (f, rx::BalconySleeper) >= 0.0f)
    {
        const float bx = X + W * 0.62f, by = Y + H * 0.17f;
        g.setColour (Colour (0xff2a2a3a));
        g.fillEllipse (bx, by, H * 0.04f, H * 0.04f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions (H * 0.05f, juce::Font::bold)));
        g.drawText ("Zzz", juce::Rectangle<float> (bx + H * 0.03f, by - H * 0.06f - std::fmod ((float) f.time * 5.0f, 6.0f), H * 0.1f, H * 0.06f), juce::Justification::left);
    }

    // flying objects
    if (float p = activeProgress (f, rx::ShoeFlies); p >= 0.0f)
    {
        const float x = X + W * (0.2f + 0.6f * p), y = Y + H * (0.5f - 0.35f * std::sin (kPi * p));
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (p * 12.0f, x, y));
        g.setColour (Colour (0xffe8e8e8));
        g.fillRoundedRectangle (x - 8.0f, y - 3.0f, 16.0f, 7.0f, 3.0f);
        g.setColour (Colour (0xffff3c6a));
        g.fillRect (x - 8.0f, y + 2.0f, 16.0f, 2.0f);
    }
    if (float p = activeProgress (f, rx::CupThrown); p >= 0.0f)
    {
        const float x = X + W * (0.35f + 0.15f * p), y = Y + H * (0.55f + 0.2f * p - 0.25f * std::sin (kPi * p));
        g.setColour (Colour (0xffe02a3a));
        juce::Path cup;
        cup.addQuadrilateral (x - 5.0f, y - 7.0f, x + 5.0f, y - 7.0f, x + 3.5f, y + 6.0f, x - 3.5f, y + 6.0f);
        g.fillPath (cup, juce::AffineTransform::rotation (p * 8.0f, x, y));
    }
    if (float p = activeProgress (f, rx::Pigeon); p >= 0.0f)
    {
        const bool flying = p < 0.5f;
        const float x = flying ? X + W * (1.05f - 0.9f * p * 2.0f) : X + W * 0.42f + std::sin ((float) f.time * 4.0f) * 6.0f;
        const float y = flying ? Y + H * (0.25f + 0.15f * std::sin (p * 20.0f)) : Y + H * 0.84f;
        g.setColour (Colour (0xff8a8aa0));
        g.fillEllipse (x - 8.0f, y - 5.0f, 16.0f, 10.0f);
        g.fillEllipse (x - 12.0f, y - 8.0f, 8.0f, 7.0f);
        g.setColour (Colour (0xff4ad08a));
        g.fillEllipse (x - 10.0f, y - 4.0f, 4.0f, 3.0f);
        if (flying)
        {
            g.setColour (Colour (0xff6a6a80));
            const float flap = std::sin ((float) f.time * 25.0f) * 8.0f;
            g.drawLine (x - 2.0f, y - 3.0f, x + 4.0f, y - 10.0f - flap, 3.0f);
        }
    }
    if (float p = activeProgress (f, rx::BouncerLetsQueueIn); p >= 0.0f)
    {
        for (int k = 0; k < 6; ++k)
        {
            const float t = juce::jlimit (0.0f, 1.0f, p * 2.0f - (float) k * 0.1f);
            if (t <= 0.0f) continue;
            const auto pos = floorToScreen (r, 0.9f - 0.3f * t, 0.15f + 0.4f * t * (float) (k % 3) / 2.0f, f.parallax);
            Pose qp; qp.bob = std::abs (std::sin ((float) f.time * 7.0f + (float) k)) * 0.5f; qp.armL0 = 2.6f; qp.armR0 = -2.6f;
            drawPerson (g, pos.x, pos.y, personHeight (r, 0.3f), qp, shirtColours[(k + 3) % 10], Colour (0xff202030), skinColours[k % 6], hairColours[k % 10], k % 5, false, 0);
        }
    }

    // confetti / smoke particles + CO2 jets
    for (auto& p : particles)
    {
        if (p.kind == 0)
        {
            g.setColour (p.c.withAlpha (juce::jlimit (0.0f, 1.0f, p.life)));
            g.fillRect (X + p.x * W, Y + p.y * H, p.size, p.size * 0.6f);
        }
        else
        {
            g.setGradientFill (juce::ColourGradient (p.c.withAlpha (0.25f * juce::jlimit (0.0f, 1.0f, p.life / 2.5f)), X + p.x * W, Y + p.y * H,
                                                     p.c.withAlpha (0.0f), X + p.x * W + p.size, Y + p.y * H, true));
            g.fillEllipse (X + p.x * W - p.size, Y + p.y * H - p.size, p.size * 2.0f, p.size * 2.0f);
        }
    }
    if (float p = activeProgress (f, rx::Confetti); p >= 0.0f && p < 0.4f)
        for (float jx : { 0.12f, 0.88f })
        {
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), X + W * jx, Y + H * 0.78f, juce::Colours::white.withAlpha (0.0f), X + W * jx, Y + H * 0.1f, false));
            g.fillRect (X + W * jx - W * 0.012f, Y + H * 0.1f, W * 0.024f, H * 0.68f);
        }
}

// ============================================================================
void ClubRenderer::drawBooth (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    const float W = r.getWidth(), H = r.getHeight(), X = r.getX(), Y = r.getY();
    const float top = Y + H * 0.76f;
    const auto L = lookFor (f.personality);

    // people in the booth (behind the desk edge)
    if (float p = activeProgress (f, rx::B2BHighFive); p >= 0.0f)
    {
        Pose bp; bp.armL0 = p > 0.4f && p < 0.6f ? 2.8f : 1.2f; bp.armL1 = 0.0f;
        drawPerson (g, X + W * (1.05f - 0.2f * std::min (1.0f, p * 3.0f)), Y + H * 1.05f, H * 0.62f, bp, Colour (0xff2ef2ff), Colour (0xff202030), skinColours[1], Colour (0xffff3cc8), 1, false, 0);
    }
    if (float p = activeProgress (f, rx::ExtendedSet); p >= 0.0f)
    {
        Pose pp; pp.armL0 = 2.2f; pp.armL1 = 0.3f;
        const float px = X + W * 0.9f, py = Y + H * 1.05f;
        drawPerson (g, px, py, H * 0.6f, pp, Colour (0xff1a1a1a), Colour (0xff101010), skinColours[0], Colour (0xff6b3a1a), 0, false, 0);
        auto phone = juce::Rectangle<float> (px - W * 0.16f, Y + H * 0.48f, W * 0.1f, H * 0.18f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (phone.expanded (3.0f), 5.0f);
        g.setColour (Colour (0xff1aff8a));
        g.fillRoundedRectangle (phone, 3.0f);
        g.setColour (juce::Colours::black);
        g.setFont (juce::Font (juce::FontOptions (H * 0.045f, juce::Font::bold)));
        g.drawFittedText ("EXTENDED\nSET", phone.toNearestInt(), juce::Justification::centred, 2);
    }
    if (float p = activeProgress (f, rx::PromoterClock); p >= 0.0f)
    {
        Pose pp; pp.armL0 = 2.0f + std::sin ((float) f.time * 6.0f) * 0.2f; pp.armL1 = 0.0f;
        drawPerson (g, X + W * 0.9f, Y + H * 1.05f, H * 0.6f, pp, Colour (0xff1a1a1a), Colour (0xff101010), skinColours[0], Colour (0xff6b3a1a), 0, false, 0);
        const auto c = juce::Point<float> (X + W * 0.78f, Y + H * 0.5f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (c.x - H * 0.06f, c.y - H * 0.06f, H * 0.12f, H * 0.12f);
        g.setColour (juce::Colours::black);
        g.drawLine (c.x, c.y, c.x, c.y - H * 0.045f, 2.0f);
        g.drawLine (c.x, c.y, c.x + H * 0.035f, c.y, 2.0f);
    }
    if (float p = activeProgress (f, rx::EngineerGainDown); p >= 0.0f)
    {
        Pose ep; ep.armL0 = 1.0f; ep.armL1 = 0.6f; ep.lean = -0.3f;
        drawPerson (g, X + W * 0.88f, Y + H * 1.05f, H * 0.62f, ep, Colour (0xff303030), Colour (0xff202020), skinColours[3], Colour (0xff3a2a1a), 3, false, 0);
        drawBubble (g, { X + W * 0.82f, Y + H * 0.42f }, "TOO LOUD!", Colour (0xff602020), 1.0f);
    }
    if (float p = activeProgress (f, rx::FanSelfie); p >= 0.0f)
    {
        Pose fp; fp.phone = true; fp.armR0 = -2.6f; fp.armR1 = 0.0f;
        const float fx = p < 0.6f ? X + W * 0.12f : X + W * (0.12f - (p - 0.6f) * 0.5f);
        drawPerson (g, fx, Y + H * 1.05f, H * 0.6f, fp, Colour (0xffffd84a), Colour (0xff202030), skinColours[5], Colour (0xff7cff4d), 4, false, 0);
        if (p > 0.55f)
        {
            Pose sp; sp.armR0 = -1.4f; sp.armR1 = -0.4f;
            drawPerson (g, fx + W * 0.08f, Y + H * 1.08f, H * 0.72f, sp, Colour (0xff141414), Colour (0xff141414), skinColours[4], Colour (0xff101010), 3, false, 1);
        }
    }
    if (float p = activeProgress (f, rx::CameraCrew); p >= 0.0f)
    {
        Pose cp; cp.armL0 = 1.6f; cp.armR0 = -1.6f;
        drawPerson (g, X + W * 0.06f, Y + H * 1.05f, H * 0.6f, cp, Colour (0xff202020), Colour (0xff101010), skinColours[2], Colour (0xff1a1a1a), 3, false, 0);
        g.setColour (Colour (0xff111111));
        g.fillRect (X + W * 0.07f, Y + H * 0.52f, W * 0.06f, H * 0.08f);
        g.setColour (juce::Colours::red.withAlpha (std::fmod ((float) f.time, 1.0f) < 0.5f ? 1.0f : 0.3f));
        g.fillEllipse (X + W * 0.12f, Y + H * 0.53f, 5.0f, 5.0f);
    }

    // desk
    juce::Path desk;
    desk.addQuadrilateral (X - W * 0.02f, Y + H, X + W * 0.06f, top, X + W * 0.94f, top, X + W * 1.02f, Y + H);
    g.setGradientFill (juce::ColourGradient (Colour (0xff1c1620), X, top, Colour (0xff0a080c), X, Y + H, false));
    g.fillPath (desk);
    g.setColour (L.rim.withAlpha (0.6f));
    g.drawLine (X + W * 0.06f, top, X + W * 0.94f, top, 2.0f);

    // stickers on the desk front (sticker-worn gear)
    auto sticker = [&] (float sx, float sy, float rot, const juce::String& text, Colour c)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (rot, sx, sy));
        g.setColour (c);
        g.fillRoundedRectangle (sx - 22.0f, sy - 7.0f, 44.0f, 14.0f, 3.0f);
        g.setColour (juce::Colours::black);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText (text, juce::Rectangle<float> (sx - 22.0f, sy - 7.0f, 44.0f, 14.0f), juce::Justification::centred);
    };
    sticker (X + W * 0.33f, Y + H * 0.93f, -0.12f, "BGDJ", L.beamA);
    sticker (X + W * 0.67f, Y + H * 0.95f, 0.1f, "SUB OK", L.beamB);

    // two decks with jog wheels
    const float rot = (float) f.time * (f.mood->idle ? 0.5f : 3.5f);
    float rewindSpin = 0.0f;
    if (float p = activeProgress (f, rx::Rewind); p >= 0.0f && p < 0.35f) rewindSpin = -p * 60.0f;
    for (int deck = 0; deck < 2; ++deck)
    {
        const float cx = X + W * (deck == 0 ? 0.25f : 0.75f), cy = top + H * 0.13f;
        const auto body = juce::Rectangle<float> (cx - W * 0.12f, top + H * 0.015f, W * 0.24f, H * 0.21f);
        g.setColour (Colour (0xff17141b));
        g.fillRoundedRectangle (body, 5.0f);
        g.setColour (Colour (0xff2e2834));
        g.drawRoundedRectangle (body, 5.0f, 1.0f);
        const float jr = H * 0.085f;
        g.setColour (Colour (0xff0c0a0e));
        g.fillEllipse (cx - jr, cy - jr * 0.55f, jr * 2.0f, jr * 1.1f);
        g.setColour (L.beamB.withAlpha (0.7f));
        g.drawEllipse (cx - jr, cy - jr * 0.55f, jr * 2.0f, jr * 1.1f, 2.0f);
        const float a = (deck == 0 ? rot + rewindSpin : rot * 1.01f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (cx + std::cos (a) * jr * 0.75f - 2.5f, cy + std::sin (a) * jr * 0.4f - 2.5f, 5.0f, 5.0f);
        // LED pads
        for (int pad = 0; pad < 4; ++pad)
        {
            const bool lit = (f.recentNotes > 0 && pad == f.beatIndex % 4) || (activeProgress (f, rx::AirHorn) >= 0.0f && deck == 1 && pad == 3);
            const auto pr = juce::Rectangle<float> (body.getX() + 6.0f + (float) pad * (body.getWidth() - 12.0f) / 4.0f, body.getBottom() - H * 0.045f, (body.getWidth() - 12.0f) / 4.0f - 3.0f, H * 0.032f);
            g.setColour ((lit ? hairColours[pad] : hairColours[pad].withAlpha (0.18f)));
            g.fillRoundedRectangle (pr, 2.0f);
        }
    }

    // mixer + level LEDs
    const auto mixer = juce::Rectangle<float> (X + W * 0.41f, top + H * 0.01f, W * 0.18f, H * 0.23f);
    g.setColour (Colour (0xff141118));
    g.fillRoundedRectangle (mixer, 4.0f);
    float level = f.energy;
    if (activeProgress (f, rx::EngineerGainDown) >= 0.0f) level *= 0.4f;
    for (int ch = 0; ch < 4; ++ch)
    {
        const float mx = mixer.getX() + mixer.getWidth() * (0.15f + 0.23f * (float) ch);
        for (int k = 0; k < 3; ++k)
        {
            const bool bumped = f.macroBumpGlow > 0.0f && ch == 1 && k == 1;
            g.setColour (bumped ? Colour (0xffffd84a) : Colour (0xff3a3442));
            g.fillEllipse (mx - 4.0f, mixer.getY() + 6.0f + (float) k * H * 0.04f, 8.0f, 8.0f);
        }
        for (int led = 0; led < 6; ++led)
        {
            const bool on = (float) led / 6.0f < level * (0.8f + 0.2f * (float) ((ch + f.beatIndex) % 2));
            g.setColour (on ? (led > 4 ? Colour (0xffff4040) : (led > 3 ? Colour (0xffffd84a) : Colour (0xff4dff9a))) : Colour (0xff201c24));
            g.fillRect (mx + 6.0f, mixer.getBottom() - 8.0f - (float) led * 5.0f, 4.0f, 3.0f);
        }
    }

    // laptop behind the mixer, screen facing you, with a scrolling waveform
    const auto screen = juce::Rectangle<float> (X + W * 0.44f, top - H * 0.12f, W * 0.12f, H * 0.11f);
    g.setColour (Colour (0xff222028));
    g.fillRoundedRectangle (screen.expanded (3.0f), 3.0f);
    g.setColour (Colour (0xff0a0814));
    g.fillRect (screen);
    juce::Path wave;
    for (int i = 0; i <= 30; ++i)
    {
        const float x = screen.getX() + screen.getWidth() * (float) i / 30.0f;
        const float amp = std::abs (std::sin ((float) i * 0.7f + (float) f.time * 4.0f)) * screen.getHeight() * 0.35f * (0.3f + f.energy);
        if (i == 0) wave.startNewSubPath (x, screen.getCentreY() - amp); else wave.lineTo (x, screen.getCentreY() - amp);
    }
    g.setColour (L.beamB);
    g.strokePath (wave, juce::PathStrokeType (1.2f));

    // your drink (unless the cat knocked it off)
    const float knock = activeProgress (f, rx::CatKnocksDrink);
    const float sip = activeProgress (f, rx::SipDrink);
    if (sip < 0.0f)
    {
        const float cx = X + W * 0.62f, cy = top + H * 0.02f;
        if (knock < 0.0f || knock < 0.4f)
        {
            g.setColour (Colour (0xffe0f0ff).withAlpha (0.7f));
            g.fillRect (cx, cy - H * 0.05f, W * 0.015f, H * 0.05f);
            g.setColour (Colour (0xffffb020).withAlpha (0.8f));
            g.fillRect (cx, cy - H * 0.03f, W * 0.015f, H * 0.03f);
        }
        else
        {
            const float t = (knock - 0.4f) / 0.6f;
            g.setColour (Colour (0xffe0f0ff).withAlpha (std::max (0.0f, 1.0f - t)));
            g.fillRect (cx + t * W * 0.05f, cy - H * 0.05f + t * H * 0.25f, W * 0.015f, H * 0.05f);
        }
    }

    // Grey the cat
    if (float p = activeProgress (f, rx::CatOnController); p >= 0.0f)
    {
        const float cx = p < 0.25f ? X + W * (0.95f - 2.0f * p) : mixer.getCentreX();
        drawCat (g, { cx, top + H * 0.08f }, H * 0.18f, p < 0.25f ? 1 : 0, f.time);
    }
    if (float p = activeProgress (f, rx::CatKnocksDrink); p >= 0.0f)
        drawCat (g, { X + W * 0.6f, top + H * 0.04f }, H * 0.17f, 3, f.time);
    if (activeProgress (f, rx::CatSleepsOnSpeaker) >= 0.0f)
        drawCat (g, { X + W * 0.04f, Y + H * 0.4f }, H * 0.14f, 2, f.time);
}

void ClubRenderer::drawHands (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    const float W = r.getWidth(), H = r.getHeight(), X = r.getX(), Y = r.getY();
    const float top = Y + H * 0.76f;
    const Colour skin (0xffd9a77c), sleeve (0xff1a1a22);

    auto hand = [&] (float x, float y, float rot, bool fist, float scale)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (rot, x, y));
        const float s = H * 0.07f * scale;
        g.setColour (sleeve);
        g.fillRect (x - s * 0.55f, y + s * 0.5f, s * 1.1f, H);   // forearm from the bottom
        juce::Path palm;
        palm.addQuadrilateral (x - s * 0.5f, y + s * 0.6f, x + s * 0.5f, y + s * 0.6f, x + s * 0.55f, y - s * 0.2f, x - s * 0.55f, y - s * 0.2f);
        g.setColour (skin);
        g.fillPath (palm);
        if (fist)
            g.fillRoundedRectangle (x - s * 0.55f, y - s * 0.7f, s * 1.1f, s * 0.6f, s * 0.2f);
        else
            for (int k = 0; k < 4; ++k)
                g.fillRoundedRectangle (x - s * 0.5f + (float) k * s * 0.27f, y - s * 0.85f, s * 0.22f, s * 0.7f, s * 0.1f);
        g.setColour (juce::Colours::black.withAlpha (0.15f));
        g.fillRect (x, y - s * 0.2f, s * 0.55f, s * 0.8f);
    };

    const float beat = f.beatPhase;
    const float nudge = std::sin (kPi * 2.0f * beat) * W * 0.004f;
    float lx = X + W * 0.27f + nudge, ly = top + H * 0.09f, lrot = 0.2f;
    float hrX = X + W * 0.47f, hrY = top + H * 0.1f - std::abs (std::sin (kPi * beat)) * H * 0.01f, hrRot = -0.1f;
    bool rFist = false;

    if (float p = activeProgress (f, rx::Rewind); p >= 0.0f && p < 0.35f)
    {
        const float a = -p * 50.0f;
        lx = X + W * 0.25f + std::cos (a) * W * 0.04f;
        ly = top + H * 0.13f + std::sin (a) * H * 0.04f;
    }
    if (float p = activeProgress (f, rx::FistPump); p >= 0.0f)
    {
        rFist = true;
        hrX = X + W * 0.55f;
        hrY = top - H * (0.15f + 0.1f * std::abs (std::sin (kPi * 4.0f * p)));
    }
    if (float p = activeProgress (f, rx::AirHorn); p >= 0.0f) { hrX = X + W * 0.8f; hrY = top + H * 0.17f - (std::fmod (p * 8.0f, 1.0f) < 0.5f ? 0.0f : H * 0.02f); }
    if (float p = activeProgress (f, rx::DropItLean); p >= 0.0f)
    {
        lx = X + W * 0.35f; ly = top - H * 0.2f * std::sin (kPi * p); lrot = -0.3f;
        hrX = X + W * 0.65f; hrY = top - H * 0.2f * std::sin (kPi * p); hrRot = 0.3f;
    }
    if (float p = activeProgress (f, rx::TowelThrow); p >= 0.0f)
    {
        hrX = X + W * 0.6f; hrY = top - H * 0.25f * std::sin (kPi * std::min (1.0f, p * 2.0f)); hrRot = 0.6f * p;
        if (p > 0.3f)
        {
            const float t = (p - 0.3f) / 0.7f;
            g.setColour (juce::Colours::white);
            const float tx = X + W * (0.6f - 0.1f * t), ty = top - H * (0.3f + 0.3f * std::sin (kPi * t)) + t * H * 0.1f;
            g.fillRoundedRectangle (tx, ty, W * 0.04f, H * 0.03f, 3.0f);
        }
    }
    if (float p = activeProgress (f, rx::SipDrink); p >= 0.0f)
    {
        hrX = X + W * 0.62f; hrY = top - H * 0.3f * std::sin (kPi * p); hrRot = -0.2f;
        g.setColour (Colour (0xffe0f0ff).withAlpha (0.8f));
        g.fillRect (hrX - W * 0.01f, hrY - H * 0.12f, W * 0.02f, H * 0.06f);
        if (p > 0.45f && p < 0.6f)   // nearly spills it on the mixer
        {
            g.setColour (Colour (0xffffb020));
            for (int k = 0; k < 3; ++k) g.fillEllipse (hrX - W * 0.03f + (float) k * 6.0f, hrY - H * 0.04f + (float) k * 5.0f, 3.0f, 4.0f);
        }
    }
    if (activeProgress (f, rx::B2BHighFive) >= 0.0f && activeProgress (f, rx::B2BHighFive) > 0.4f && activeProgress (f, rx::B2BHighFive) < 0.6f)
    { hrX = X + W * 0.8f; hrY = top - H * 0.3f; hrRot = 0.4f; }

    hand (lx, ly, lrot, false, 1.0f);
    if (activeProgress (f, rx::HeadphonePose) >= 0.0f)
    {
        // right hand holds one headphone cup to your ear (top-right of view)
        hand (X + W * 0.93f, Y + H * 0.3f, -0.5f, false, 1.3f);
        g.setColour (Colour (0xff101014));
        g.fillEllipse (X + W * 0.9f, Y + H * 0.12f, W * 0.08f, H * 0.18f);
        g.setColour (lookFor (f.personality).beamA);
        g.drawEllipse (X + W * 0.9f, Y + H * 0.12f, W * 0.08f, H * 0.18f, 2.0f);
    }
    else
        hand (hrX, hrY, hrRot, rFist, 1.0f);
}

void ClubRenderer::drawOverlays (juce::Graphics& g, juce::Rectangle<float> r, const SceneFrame& f)
{
    if (strobe > 0.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (strobe * 0.35f));
        g.fillRect (r);
    }
    if (lightsCut > 0.01f)
    {
        g.setColour (juce::Colours::black.withAlpha (lightsCut * 0.93f));
        g.fillRect (r);
    }
    if (houseLights > 0.01f)
    {
        // house lights on: flat, harsh, empty. a janitor sweeps up.
        g.setColour (Colour (0xfff0ead8).withAlpha (houseLights * 0.45f));
        g.fillRect (r);
        const float W = r.getWidth(), H = r.getHeight();
        const float jx = r.getX() + W * (0.3f + 0.25f * std::sin ((float) f.time * 0.3f));
        Pose jp; jp.armL0 = 0.8f + std::sin ((float) f.time * 3.0f) * 0.4f; jp.armR0 = 0.6f + std::sin ((float) f.time * 3.0f) * 0.4f; jp.alpha = houseLights;
        drawPerson (g, jx, r.getY() + H * 0.7f, H * 0.28f, jp, Colour (0xff3a5aa0), Colour (0xff2a3a6a), skinColours[3], Colour (0xff8a8a8a), 3, false, 0);
        g.setColour (Colour (0xff8a6a3a).withAlpha (houseLights));
        g.drawLine (jx + H * 0.08f, r.getY() + H * 0.55f, jx + H * 0.2f, r.getY() + H * 0.72f, 3.0f);
        if (f.mood->walkout)
        {
            auto card = r.withSizeKeepingCentre (W * 0.3f, H * 0.2f).translated (0.0f, -H * 0.15f);
            g.setColour (juce::Colours::black.withAlpha (0.85f * houseLights));
            g.fillRoundedRectangle (card, 8.0f);
            g.setColour (Colour (0xffff2bd6).withAlpha (houseLights));
            g.drawRoundedRectangle (card, 8.0f, 2.0f);
            g.setFont (juce::Font (juce::FontOptions (H * 0.11f, juce::Font::bold)));
            g.drawText ("SET OVER", card, juce::Justification::centred);
        }
    }
    // vignette
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(), juce::Colours::black.withAlpha (0.45f), r.getX(), r.getY(), true));
    g.fillRect (r);
}
} // namespace bgdj
