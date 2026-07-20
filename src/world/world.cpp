#include "world/world.h"
#include "core/mathx.h"
#include "core/rng.h"
#include "render/renderer.h"

// Zone palette — warm ancient stone against vivid vegetation.
static const Color STONE    = { 205, 192, 168, 255 };
static const Color STONE_HI = { 226, 214, 190, 255 };
static const Color TRUNK    = { 104, 76, 50, 255 };
static const Color CANOPY_A = { 74, 142, 66, 255 };
static const Color CANOPY_B = { 118, 162, 58, 255 };
static const Color ROCK_C   = { 138, 130, 120, 255 };
static const Color CRYSTAL  = { 92, 240, 205, 255 };   // emissive teal
static const Color RUNE     = { 255, 214, 126, 255 };  // emissive gold
static const float PLAY_LIMIT = 148.0f;                // hard world border

void World::Place(const Terrain& t, Prop p) {
    p.pos.y = t.HeightAt(p.pos.x, p.pos.z);
    props.push_back(p);
}

void World::Generate(unsigned int seed, const Terrain& t) {
    terrain = &t;
    props.clear(); colliders.clear(); platforms.clear(); lights.clear(); shrines.clear();
    Rng rng(seed * 977u + 13u);

    float plateau = t.plateauHeight;

    // --- Temple of the First Dawn: dais, altar, column ring ---------------
    platforms.push_back({ { 0, 0 }, 7.5f, plateau + 0.45f });
    { Prop a{}; a.type = PropType::Altar; a.pos = { 0, 0, 0 }; a.boundRadius = 16.0f; Place(t, a); }
    lights.push_back({ { 0, plateau + 3.4f, 0 }, { 0.5f, 1.7f, 1.35f }, 15.0f });
    shrines.push_back({ { 0, plateau + 3.1f, 0 }, 0, true });

    for (int i = 0; i < 8; i++) {
        float ang = i * PI / 4.0f;
        Prop c{};
        c.type = (i == 2 || i == 5) ? PropType::BrokenColumn : PropType::Column;
        c.pos = { sinf(ang) * 11.0f, 0, cosf(ang) * 11.0f };
        c.yaw = ang;
        c.tint = rng.Range(0.9f, 1.05f);
        c.boundRadius = 4.0f;
        Place(t, c);
        colliders.push_back({ { c.pos.x, c.pos.z }, 0.55f, c.pos.y, c.pos.y + 6.0f });
    }

    // --- Wayshrines scattered through the vale ----------------------------
    const float shrineAngles[4] = { 0.6f, 2.1f, 3.6f, 5.2f };
    for (int i = 0; i < 4; i++) {
        float dist = rng.Range(52.0f, 78.0f);
        Prop s{};
        s.type = PropType::Shrine;
        s.pos = { sinf(shrineAngles[i]) * dist, 0, cosf(shrineAngles[i]) * dist };
        s.yaw = rng.Range(0.0f, PI * 2.0f);
        s.boundRadius = 3.5f;
        Place(t, s);
        colliders.push_back({ { s.pos.x, s.pos.z }, 0.6f, s.pos.y, s.pos.y + 2.0f });
        lights.push_back({ { s.pos.x, s.pos.y + 2.1f, s.pos.z }, { 1.5f, 1.15f, 0.55f }, 10.0f });
        shrines.push_back({ { s.pos.x, s.pos.y + 2.15f, s.pos.z }, (int)lights.size() - 1, false });
    }

    // --- Monoliths of the old gods on the high ground ---------------------
    for (int i = 0; i < 6; i++) {
        float ang = rng.Range(0.0f, PI * 2.0f);
        float dist = rng.Range(85.0f, 128.0f);
        Prop m{};
        m.type = PropType::Monolith;
        m.pos = { sinf(ang) * dist, 0, cosf(ang) * dist };
        m.yaw = rng.Range(0.0f, PI * 2.0f);
        m.scale = rng.Range(0.85f, 1.25f);
        m.tint = rng.Range(0.85f, 1.0f);
        m.boundRadius = 6.0f;
        Place(t, m);
        colliders.push_back({ { m.pos.x, m.pos.z }, 1.1f * m.scale, m.pos.y, m.pos.y + 8.0f });
    }

    // --- A ring of standing stones (future ritual site) -------------------
    {
        Vector2 c = { sinf(2.4f) * 92.0f, cosf(2.4f) * 92.0f };
        for (int i = 0; i < 7; i++) {
            float ang = i * (PI * 2.0f / 7.0f);
            Prop s{};
            s.type = PropType::StandingStone;
            s.pos = { c.x + sinf(ang) * 8.0f, 0, c.y + cosf(ang) * 8.0f };
            s.yaw = ang + rng.Range(-0.2f, 0.2f);
            s.scale = rng.Range(0.8f, 1.2f);
            s.tint = rng.Range(0.85f, 1.0f);
            s.boundRadius = 3.0f;
            Place(t, s);
            colliders.push_back({ { s.pos.x, s.pos.z }, 0.7f * s.scale, s.pos.y, s.pos.y + 2.5f });
        }
    }

    // --- Trees and rocks, rejection-sampled for spacing and slope ---------
    auto farFromProps = [&](Vector3 p, float minDist) {
        for (const Prop& q : props) {
            float dx = p.x - q.pos.x, dz = p.z - q.pos.z;
            if (dx * dx + dz * dz < minDist * minDist) return false;
        }
        return true;
    };

    for (int i = 0; i < 260 && (int)props.size() < 400; i++) {
        Vector3 p = { rng.Range(-140.0f, 140.0f), 0, rng.Range(-140.0f, 140.0f) };
        float dist = sqrtf(p.x * p.x + p.z * p.z);
        if (dist < 36.0f || dist > 142.0f) continue;
        if (t.NormalAt(p.x, p.z).y < 0.86f) continue;      // too steep
        if (!farFromProps(p, 7.0f)) continue;
        Prop tr{};
        tr.type = PropType::Tree;
        tr.pos = p;
        tr.yaw = rng.Range(0.0f, PI * 2.0f);
        tr.scale = rng.Range(0.8f, 1.45f);
        tr.tint = rng.Range(0.8f, 1.1f);
        tr.boundRadius = 4.5f * tr.scale;
        Place(t, tr);
        colliders.push_back({ { p.x, p.z }, 0.34f * tr.scale, tr.pos.y, tr.pos.y + 3.0f });
    }

    for (int i = 0; i < 90; i++) {
        Vector3 p = { rng.Range(-142.0f, 142.0f), 0, rng.Range(-142.0f, 142.0f) };
        float dist = sqrtf(p.x * p.x + p.z * p.z);
        if (dist < 14.0f) continue;
        if (!farFromProps(p, 4.0f)) continue;
        Prop rk{};
        rk.type = PropType::Rock;
        rk.pos = p;
        rk.yaw = rng.Range(0.0f, PI * 2.0f);
        rk.scale = rng.Range(0.5f, 1.6f);
        rk.tint = rng.Range(0.85f, 1.1f);
        rk.boundRadius = 2.2f * rk.scale;
        Place(t, rk);
        if (rk.scale > 1.1f)
            colliders.push_back({ { p.x, p.z }, 0.9f * rk.scale, rk.pos.y, rk.pos.y + 1.5f });
    }

    // Spawn on the southern approach, facing the temple.
    spawnPos = { 0, 0, 44.0f };
    spawnPos.y = t.HeightAt(spawnPos.x, spawnPos.z);
    spawnYaw = 0.0f;   // convention: yaw 0 faces -Z, toward the temple
}

float World::GroundHeightAt(float x, float z, float fromY) const {
    float ground = terrain->HeightAt(x, z);
    for (const Platform& pf : platforms) {
        float dx = x - pf.xz.x, dz = z - pf.xz.y;
        if (dx * dx + dz * dz > pf.radius * pf.radius) continue;
        // Only counts as ground if reachable: standing near its top or above.
        if (fromY + 0.55f >= pf.topY && pf.topY > ground) ground = pf.topY;
    }
    return ground;
}

bool World::PointBlocked(Vector3 p, float pad) const {
    if (p.y < terrain->HeightAt(p.x, p.z) + pad) return true;
    for (const CylinderCollider& c : colliders) {
        if (p.y > c.y1 || p.y < c.y0) continue;
        float dx = p.x - c.xz.x, dz = p.z - c.xz.y;
        float r = c.radius + pad;
        if (dx * dx + dz * dz < r * r) return true;
    }
    return false;
}

Vector3 World::ResolveCollisions(Vector3 pos, float radius, float height) const {
    pos.x = Clamp(pos.x, -PLAY_LIMIT, PLAY_LIMIT);
    pos.z = Clamp(pos.z, -PLAY_LIMIT, PLAY_LIMIT);

    // Two passes settle corner cases where one pushout slides into another.
    for (int pass = 0; pass < 2; pass++) {
        for (const CylinderCollider& c : colliders) {
            if (pos.y > c.y1 || pos.y + height < c.y0) continue;
            float dx = pos.x - c.xz.x, dz = pos.z - c.xz.y;
            float d2 = dx * dx + dz * dz;
            float minD = radius + c.radius;
            if (d2 >= minD * minD || d2 < 1e-8f) continue;
            float d = sqrtf(d2);
            pos.x = c.xz.x + dx / d * minD;
            pos.z = c.xz.y + dz / d * minD;
        }
    }
    return pos;
}

// ---- drawing --------------------------------------------------------------

// Local-space part transform: scale, yaw about the prop origin, translate.
static Matrix PartXf(const Prop& p, Vector3 off, Vector3 scl, float extraYaw = 0.0f) {
    Matrix m = MatrixScale(scl.x * p.scale, scl.y * p.scale, scl.z * p.scale);
    m = MatrixMultiply(m, MatrixRotateY(p.yaw + extraYaw));
    Vector3 o = Vector3RotateByAxisAngle(Vector3Scale(off, p.scale), { 0, 1, 0 }, p.yaw);
    return MatrixMultiply(m, MatrixTranslate(p.pos.x + o.x, p.pos.y + o.y, p.pos.z + o.z));
}

void World::DrawProp(Renderer& r, const Prop& p, float time) const {
    Color stone = ColorMul(STONE, p.tint);
    switch (p.type) {
        case PropType::Column: {
            r.DrawLit(r.cube, PartXf(p, { 0, 0.15f, 0 }, { 1.5f, 0.3f, 1.5f }), stone);
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0.3f, 0 }, { 0.9f, 4.6f, 0.9f }), stone);
            r.DrawLit(r.cube, PartXf(p, { 0, 5.05f, 0 }, { 1.6f, 0.35f, 1.6f }), ColorMul(STONE_HI, p.tint));
            break;
        }
        case PropType::BrokenColumn: {
            r.DrawLit(r.cube, PartXf(p, { 0, 0.15f, 0 }, { 1.5f, 0.3f, 1.5f }), stone);
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0.3f, 0 }, { 0.9f, 1.9f, 0.9f }), stone);
            // Fallen drum beside the base.
            r.DrawLit(r.sphere, PartXf(p, { 1.6f, 0.35f, 0.6f }, { 1.0f, 0.7f, 1.0f }), ColorMul(STONE, p.tint * 0.92f));
            break;
        }
        case PropType::Monolith: {
            r.DrawLit(r.cube, PartXf(p, { 0, 3.5f, 0 }, { 1.7f, 7.0f, 1.0f }), stone);
            r.DrawLit(r.cube, PartXf(p, { 0, 0.25f, 0 }, { 2.4f, 0.5f, 1.7f }), ColorMul(STONE, p.tint * 0.9f));
            // Glowing rune stripe — pulses slowly so the world feels alive.
            float pulse = 0.75f + 0.25f * sinf(time * 1.3f + p.pos.x * 0.37f);
            r.DrawLit(r.cube, PartXf(p, { 0, 4.2f, 0.48f }, { 0.22f, 3.6f, 0.1f }), ColorMul(RUNE, pulse), 1.0f);
            break;
        }
        case PropType::StandingStone: {
            r.DrawLit(r.cube, PartXf(p, { 0, 1.1f, 0 }, { 1.1f, 2.3f, 0.7f }), stone);
            break;
        }
        case PropType::Tree: {
            Color canopy = ColorMul(ColorLerpF(CANOPY_A, CANOPY_B, p.tint - 0.8f), p.tint);
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0, 0 }, { 0.55f, 2.7f, 0.55f }), ColorMul(TRUNK, p.tint));
            r.DrawLit(r.sphere, PartXf(p, { 0, 3.4f, 0 }, { 3.2f, 2.6f, 3.2f }), canopy);
            r.DrawLit(r.sphere, PartXf(p, { 0.9f, 2.9f, 0.5f }, { 1.9f, 1.6f, 1.9f }), ColorMul(canopy, 0.92f));
            r.DrawLit(r.sphere, PartXf(p, { -0.8f, 4.3f, -0.3f }, { 1.7f, 1.5f, 1.7f }), ColorMul(canopy, 1.08f));
            break;
        }
        case PropType::Rock: {
            r.DrawLit(r.sphere, PartXf(p, { 0, 0.35f, 0 }, { 1.9f, 1.1f, 1.5f }), ColorMul(ROCK_C, p.tint));
            r.DrawLit(r.sphere, PartXf(p, { 0.8f, 0.2f, 0.4f }, { 1.0f, 0.7f, 0.9f }), ColorMul(ROCK_C, p.tint * 0.92f));
            break;
        }
        case PropType::Shrine: {
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0, 0 }, { 1.7f, 0.4f, 1.7f }), stone);
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0.4f, 0 }, { 0.7f, 1.1f, 0.7f }), ColorMul(STONE_HI, p.tint));
            // Floating twin-cone crystal, bobbing and turning.
            float bob = sinf(time * 1.6f + p.pos.z * 0.5f) * 0.12f;
            float spin = time * 0.9f;
            r.DrawLit(r.cone, PartXf(p, { 0, 2.15f + bob, 0 }, { 0.55f, 0.7f, 0.55f }, spin), CRYSTAL, 1.0f);
            Matrix flip = MatrixMultiply(MatrixRotateX(PI), MatrixScale(1, 1, 1));
            Matrix m = MatrixMultiply(MatrixScale(0.55f * p.scale, 0.7f * p.scale, 0.55f * p.scale), flip);
            m = MatrixMultiply(m, MatrixRotateY(p.yaw + spin));
            m = MatrixMultiply(m, MatrixTranslate(p.pos.x, p.pos.y + 2.15f + bob, p.pos.z));
            r.DrawLit(r.cone, m, ColorMul(CRYSTAL, 0.85f), 1.0f);
            break;
        }
        case PropType::Altar: {
            // Dais: two stacked discs (the top one is the walkable platform).
            // Darker than columns: huge sunlit horizontals blow out otherwise.
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0, 0 }, { 17.0f, 0.25f, 17.0f }), ColorMul(STONE, 0.78f));
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0.25f, 0 }, { 15.0f, 0.2f, 15.0f }), ColorMul(STONE, 0.86f));
            // Central altar block and its hovering heart-crystal.
            r.DrawLit(r.cube, PartXf(p, { 0, 0.45f + 0.55f, 0 }, { 2.2f, 1.1f, 2.2f }), ColorMul(STONE_HI, p.tint));
            float bob = sinf(time * 1.2f) * 0.18f;
            float spin = time * 0.6f;
            r.DrawLit(r.cone, PartXf(p, { 0, 3.1f + bob, 0 }, { 0.9f, 1.2f, 0.9f }, spin), CRYSTAL, 1.0f);
            Matrix m = MatrixMultiply(MatrixScale(0.9f, 1.2f, 0.9f), MatrixRotateX(PI));
            m = MatrixMultiply(m, MatrixRotateY(spin));
            m = MatrixMultiply(m, MatrixTranslate(p.pos.x, p.pos.y + 3.1f + bob, p.pos.z));
            r.DrawLit(r.cone, m, ColorMul(CRYSTAL, 0.85f), 1.0f);
            break;
        }
    }
}

void World::Draw(Renderer& r, Vector3 camPos, float viewDistance, float time) const {
    float vd2 = viewDistance * viewDistance;
    for (const Prop& p : props) {
        float dx = p.pos.x - camPos.x, dz = p.pos.z - camPos.z;
        if (dx * dx + dz * dz > vd2) { r.stats.culledProps++; continue; }
        Vector3 center = { p.pos.x, p.pos.y + p.boundRadius * 0.5f, p.pos.z };
        if (!r.Visible(center, p.boundRadius)) { r.stats.culledProps++; continue; }
        DrawProp(r, p, time);
    }
}
