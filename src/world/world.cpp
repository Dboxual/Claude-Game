#include "world/world.h"
#include "core/mathx.h"
#include "core/rng.h"
#include "render/renderer.h"

// Shared structural palette — biome palettes live in the ZoneDef.
static const Color STONE    = { 205, 192, 168, 255 };
static const Color STONE_HI = { 226, 214, 190, 255 };
static const Color ROCK_C   = { 138, 130, 120, 255 };
static const Color CRYSTAL  = { 92, 240, 205, 255 };   // emissive teal
static const Color RUNE     = { 255, 214, 126, 255 };  // emissive gold
static const Color PORTAL   = { 130, 215, 255, 255 };  // emissive gate glow
static const float PLAY_MARGIN = 6.0f;                 // hard border inset

void World::Place(const Terrain& t, Prop& p) {
    p.pos.y = t.HeightAt(p.pos.x, p.pos.z);
    propGrid.Insert((int)props.size(), { p.pos.x, p.pos.z }, p.boundRadius);
    props.push_back(p);
}

void World::AddCollider(const CylinderCollider& c) {
    colliderGrid.Insert((int)colliders.size(), c.xz, c.radius);
    colliders.push_back(c);
}

void World::Generate(unsigned int worldSeed, const Terrain& t, const ZoneDef& zdef) {
    terrain = &t;
    def = &zdef;
    props.clear(); colliders.clear(); platforms.clear();
    lights.clear(); shrines.clear(); gates.clear();
    propGrid.Init(ZONE_HALF, 16.0f);
    colliderGrid.Init(ZONE_HALF, 16.0f);
    props.reserve(5000);
    colliders.reserve(5000);
    scratch.reserve(2048);

    Rng rng(zdef.SeedFor(worldSeed) * 977u + 13u);
    const BiomeStyle& biome = zdef.biome;
    float area1k = (ZONE_SIZE * ZONE_SIZE) / 1000.0f;   // zone area in 1000 m^2

    // Spacing helper through the prop grid (placement-time only).
    auto nearProp = [&](Vector3 p, float minDist) {
        bool hit = false;
        propGrid.Query({ p.x, p.z }, minDist, [&](int i) {
            float dx = p.x - props[i].pos.x, dz = p.z - props[i].pos.z;
            if (dx * dx + dz * dz < minDist * minDist) hit = true;
        });
        return hit;
    };

    // --- Temple of the First Dawn (Elder Vale only) -----------------------
    if (zdef.temple) {
        float plateau = t.plateauHeight;
        platforms.push_back({ { 0, 0 }, 7.5f, plateau + 0.45f });
        { Prop a{}; a.type = PropType::Altar; a.pos = { 0, 0, 0 }; a.boundRadius = 16.0f; Place(t, a); }
        AddCollider({ { 0, 0 }, 1.2f, plateau + 0.45f, plateau + 1.55f });
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
            float colliderHeight = c.type == PropType::BrokenColumn ? 2.2f : 6.0f;
            AddCollider({ { c.pos.x, c.pos.z }, 0.55f,
                          c.pos.y, c.pos.y + colliderHeight });
        }
    }

    // --- Gates to neighboring zones ---------------------------------------
    for (int gi = 0; gi < (int)zdef.gates.size(); gi++) {
        Vector2 gxz = GateXZ(zdef.gates[gi]);
        Prop g{};
        g.type = PropType::Gate;
        g.pos = { gxz.x, 0, gxz.y };
        // Arch faces along the inward direction.
        Vector2 in = GateInwardDir(zdef.gates[gi].edge);
        g.yaw = atan2f(in.x, -in.y);
        g.boundRadius = 8.0f;
        Place(t, g);
        float gy = t.HeightAt(gxz.x, gxz.y);
        // Pillar colliders either side of the portal (local X after yaw).
        Vector3 side = Vector3RotateByAxisAngle({ 1, 0, 0 }, { 0, 1, 0 }, g.yaw);
        for (float s : { -3.0f, 3.0f })
            AddCollider({ { gxz.x + side.x * s, gxz.y + side.z * s }, 0.8f, gy, gy + 7.0f });
        gates.push_back({ { gxz.x, gy + 1.8f, gxz.y }, gi });
        lights.push_back({ { gxz.x, gy + 3.2f, gxz.y }, { 0.55f, 1.0f, 1.5f }, 12.0f });
    }

    // --- Wayshrines --------------------------------------------------------
    for (int i = 0; i < zdef.wayshrines; i++) {
        for (int attempt = 0; attempt < 40; attempt++) {
            float ang = rng.Range(0.0f, PI * 2.0f);
            float dist = rng.Range(90.0f, ZONE_HALF - 140.0f);
            Vector3 p = { sinf(ang) * dist, 0, cosf(ang) * dist };
            if (t.NormalAt(p.x, p.z).y < 0.88f || nearProp(p, 60.0f)) continue;
            Prop s{};
            s.type = PropType::Shrine;
            s.pos = p;
            s.yaw = rng.Range(0.0f, PI * 2.0f);
            s.boundRadius = 3.5f;
            Place(t, s);
            AddCollider({ { s.pos.x, s.pos.z }, 0.6f, s.pos.y, s.pos.y + 2.0f });
            lights.push_back({ { s.pos.x, s.pos.y + 2.1f, s.pos.z }, { 1.5f, 1.15f, 0.55f }, 10.0f });
            shrines.push_back({ { s.pos.x, s.pos.y + 2.15f, s.pos.z },
                                (int)lights.size() - 1, false });
            break;
        }
    }

    // --- Monoliths on high ground -----------------------------------------
    for (int i = 0; i < zdef.monoliths; i++) {
        float ang = rng.Range(0.0f, PI * 2.0f);
        float dist = rng.Range(120.0f, ZONE_HALF - 110.0f);
        Prop m{};
        m.type = PropType::Monolith;
        m.pos = { sinf(ang) * dist, 0, cosf(ang) * dist };
        m.yaw = rng.Range(0.0f, PI * 2.0f);
        m.scale = rng.Range(0.85f, 1.25f);
        m.tint = rng.Range(0.85f, 1.0f);
        m.boundRadius = 6.0f;
        if (nearProp(m.pos, 30.0f)) continue;
        Place(t, m);
        AddCollider({ { m.pos.x, m.pos.z }, 1.1f * m.scale,
                      m.pos.y, m.pos.y + 8.0f * m.scale });
    }

    // --- Standing-stone ring ----------------------------------------------
    if (zdef.stoneCircle) {
        float ang0 = rng.Range(0.0f, PI * 2.0f);
        float cd = rng.Range(180.0f, 420.0f);
        Vector2 c = { sinf(ang0) * cd, cosf(ang0) * cd };
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
            AddCollider({ { s.pos.x, s.pos.z }, 0.7f * s.scale,
                          s.pos.y, s.pos.y + 2.5f * s.scale });
        }
    }

    // --- Trees and rocks by biome density ---------------------------------
    int treeTarget = (int)(biome.treeDensity * area1k);
    int placedTrees = 0;
    for (int i = 0; i < treeTarget * 2 && placedTrees < treeTarget; i++) {
        Vector3 p = { rng.Range(-ZONE_HALF + 40.0f, ZONE_HALF - 40.0f), 0,
                      rng.Range(-ZONE_HALF + 40.0f, ZONE_HALF - 40.0f) };
        float centerDist = sqrtf(p.x * p.x + p.z * p.z);
        if (zdef.temple && centerDist < 36.0f) continue;
        if (t.NormalAt(p.x, p.z).y < 0.86f) continue;
        if (nearProp(p, 6.0f)) continue;
        Prop tr{};
        tr.type = PropType::Tree;
        tr.pos = p;
        tr.yaw = rng.Range(0.0f, PI * 2.0f);
        tr.scale = rng.Range(0.8f, 1.45f);
        tr.tint = rng.Range(0.8f, 1.1f);
        tr.boundRadius = 4.5f * tr.scale;
        Place(t, tr);
        AddCollider({ { p.x, p.z }, 0.34f * tr.scale,
                      tr.pos.y, tr.pos.y + 3.0f * tr.scale });
        placedTrees++;
    }

    int rockTarget = (int)(biome.rockDensity * area1k);
    int placedRocks = 0;
    for (int i = 0; i < rockTarget * 2 && placedRocks < rockTarget; i++) {
        Vector3 p = { rng.Range(-ZONE_HALF + 20.0f, ZONE_HALF - 20.0f), 0,
                      rng.Range(-ZONE_HALF + 20.0f, ZONE_HALF - 20.0f) };
        if (zdef.temple && sqrtf(p.x * p.x + p.z * p.z) < 20.0f) continue;
        if (nearProp(p, 4.0f)) continue;
        Prop rk{};
        rk.type = PropType::Rock;
        rk.pos = p;
        rk.yaw = rng.Range(0.0f, PI * 2.0f);
        rk.scale = rng.Range(0.5f, 1.6f);
        rk.tint = rng.Range(0.85f, 1.1f);
        rk.boundRadius = 2.2f * rk.scale;
        Place(t, rk);
        if (rk.scale > 1.1f)
            AddCollider({ { p.x, p.z }, 0.9f * rk.scale,
                          rk.pos.y, rk.pos.y + 1.5f * rk.scale });
        placedRocks++;
    }

    // Spawn: temple approach in Elder Vale, else a clearing near center.
    spawnPos = zdef.temple ? Vector3{ 0, 0, 44.0f } : Vector3{ 0, 0, 24.0f };
    spawnPos.y = t.HeightAt(spawnPos.x, spawnPos.z);
    spawnYaw = 0.0f;
}

// ---- queries --------------------------------------------------------------

float World::GroundHeightAt(float x, float z, float fromY) const {
    float ground = terrain->HeightAt(x, z);
    for (const Platform& pf : platforms) {
        float dx = x - pf.xz.x, dz = z - pf.xz.y;
        if (dx * dx + dz * dz > pf.radius * pf.radius) continue;
        if (fromY + 0.55f >= pf.topY && pf.topY > ground) ground = pf.topY;
    }
    return ground;
}

Vector3 World::GroundNormalAt(float x, float z, float fromY) const {
    float terrainHeight = terrain->HeightAt(x, z);
    for (const Platform& pf : platforms) {
        float dx = x - pf.xz.x, dz = z - pf.xz.y;
        if (dx * dx + dz * dz > pf.radius * pf.radius) continue;
        if (fromY + 0.55f >= pf.topY && pf.topY > terrainHeight)
            return { 0, 1, 0 };
    }
    return terrain->NormalAt(x, z);
}

bool World::PointBlocked(Vector3 p, float pad) const {
    if (p.y < terrain->HeightAt(p.x, p.z) + pad) return true;
    bool blocked = false;
    colliderGrid.Query({ p.x, p.z }, pad + 2.0f, [&](int i) {
        const CylinderCollider& c = colliders[i];
        if (p.y - pad > c.y1 || p.y + pad < c.y0) return;
        float dx = p.x - c.xz.x, dz = p.z - c.xz.y;
        float r = c.radius + pad;
        if (dx * dx + dz * dz < r * r) blocked = true;
    });
    return blocked;
}

Vector3 World::ResolveCollisions(Vector3 pos, float radius, float height,
                                 Vector3* velocity) const {
    float limit = ZONE_HALF - PLAY_MARGIN;
    Vector3 unclamped = pos;
    pos.x = Clamp(pos.x, -limit, limit);
    pos.z = Clamp(pos.z, -limit, limit);
    if (velocity) {
        if (pos.x != unclamped.x && velocity->x * unclamped.x > 0.0f) velocity->x = 0.0f;
        if (pos.z != unclamped.z && velocity->z * unclamped.z > 0.0f) velocity->z = 0.0f;
    }

    // Two passes settle corner cases where one pushout slides into another.
    for (int pass = 0; pass < 2; pass++) {
        colliderGrid.Query({ pos.x, pos.z }, radius + 2.5f, [&](int i) {
            const CylinderCollider& c = colliders[i];
            if (pos.y > c.y1 || pos.y + height < c.y0) return;
            float dx = pos.x - c.xz.x, dz = pos.z - c.xz.y;
            float d2 = dx * dx + dz * dz;
            float minD = radius + c.radius;
            if (d2 >= minD * minD) return;

            Vector2 normal;
            if (d2 > 1e-8f) {
                float invD = 1.0f / sqrtf(d2);
                normal = { dx * invD, dz * invD };
            } else if (velocity && (velocity->x * velocity->x + velocity->z * velocity->z) > 1e-8f) {
                float invSpeed = 1.0f / sqrtf(velocity->x * velocity->x + velocity->z * velocity->z);
                normal = { -velocity->x * invSpeed, -velocity->z * invSpeed };
            } else {
                normal = { 1, 0 };
            }
            pos.x = c.xz.x + normal.x * minD;
            pos.z = c.xz.y + normal.y * minD;

            if (velocity) {
                float inward = velocity->x * normal.x + velocity->z * normal.y;
                if (inward < 0.0f) {
                    velocity->x -= normal.x * inward;
                    velocity->z -= normal.y * inward;
                }
            }
        });
    }
    return pos;
}

// ---- drawing --------------------------------------------------------------

static Matrix PartXf(const Prop& p, Vector3 off, Vector3 scl, float extraYaw = 0.0f) {
    Matrix m = MatrixScale(scl.x * p.scale, scl.y * p.scale, scl.z * p.scale);
    m = MatrixMultiply(m, MatrixRotateY(p.yaw + extraYaw));
    Vector3 o = Vector3RotateByAxisAngle(Vector3Scale(off, p.scale), { 0, 1, 0 }, p.yaw);
    return MatrixMultiply(m, MatrixTranslate(p.pos.x + o.x, p.pos.y + o.y, p.pos.z + o.z));
}

void World::DrawProp(Renderer& r, const Prop& p, float time) const {
    const BiomeStyle& b = def->biome;
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
            r.DrawLit(r.sphere, PartXf(p, { 1.6f, 0.35f, 0.6f }, { 1.0f, 0.7f, 1.0f }), ColorMul(STONE, p.tint * 0.92f));
            break;
        }
        case PropType::Monolith: {
            r.DrawLit(r.cube, PartXf(p, { 0, 3.5f, 0 }, { 1.7f, 7.0f, 1.0f }), stone);
            r.DrawLit(r.cube, PartXf(p, { 0, 0.25f, 0 }, { 2.4f, 0.5f, 1.7f }), ColorMul(STONE, p.tint * 0.9f));
            float pulse = 0.75f + 0.25f * sinf(time * 1.3f + p.pos.x * 0.37f);
            r.DrawLit(r.cube, PartXf(p, { 0, 4.2f, 0.48f }, { 0.22f, 3.6f, 0.1f }), ColorMul(RUNE, pulse), 1.0f);
            break;
        }
        case PropType::StandingStone: {
            r.DrawLit(r.cube, PartXf(p, { 0, 1.1f, 0 }, { 1.1f, 2.3f, 0.7f }), stone);
            break;
        }
        case PropType::Tree: {
            Color canopy = ColorMul(ColorLerpF(b.canopyA, b.canopyB, p.tint - 0.8f), p.tint);
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0, 0 }, { 0.55f, 2.7f, 0.55f }), ColorMul(b.trunk, p.tint));
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
            float bob = sinf(time * 1.6f + p.pos.z * 0.5f) * 0.12f;
            float spin = time * 0.9f;
            r.DrawLit(r.cone, PartXf(p, { 0, 2.15f + bob, 0 }, { 0.55f, 0.7f, 0.55f }, spin), CRYSTAL, 1.0f);
            Matrix m = MatrixMultiply(MatrixScale(0.55f * p.scale, 0.7f * p.scale, 0.55f * p.scale),
                                      MatrixRotateX(PI));
            m = MatrixMultiply(m, MatrixRotateY(p.yaw + spin));
            m = MatrixMultiply(m, MatrixTranslate(p.pos.x, p.pos.y + 2.15f + bob, p.pos.z));
            r.DrawLit(r.cone, m, ColorMul(CRYSTAL, 0.85f), 1.0f);
            break;
        }
        case PropType::Altar: {
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0, 0 }, { 17.0f, 0.25f, 17.0f }), ColorMul(STONE, 0.78f));
            r.DrawLit(r.cylinder, PartXf(p, { 0, 0.25f, 0 }, { 15.0f, 0.2f, 15.0f }), ColorMul(STONE, 0.86f));
            r.DrawLit(r.cube, PartXf(p, { 0, 1.0f, 0 }, { 2.2f, 1.1f, 2.2f }), ColorMul(STONE_HI, p.tint));
            float bob = sinf(time * 1.2f) * 0.18f;
            float spin = time * 0.6f;
            r.DrawLit(r.cone, PartXf(p, { 0, 3.1f + bob, 0 }, { 0.9f, 1.2f, 0.9f }, spin), CRYSTAL, 1.0f);
            Matrix m = MatrixMultiply(MatrixScale(0.9f, 1.2f, 0.9f), MatrixRotateX(PI));
            m = MatrixMultiply(m, MatrixRotateY(spin));
            m = MatrixMultiply(m, MatrixTranslate(p.pos.x, p.pos.y + 3.1f + bob, p.pos.z));
            r.DrawLit(r.cone, m, ColorMul(CRYSTAL, 0.85f), 1.0f);
            break;
        }
        case PropType::Gate: {
            // Two pillars, a lintel, and a softly pulsing portal plane.
            r.DrawLit(r.cube, PartXf(p, { -3.0f, 3.0f, 0 }, { 1.4f, 6.0f, 1.4f }), stone);
            r.DrawLit(r.cube, PartXf(p, { 3.0f, 3.0f, 0 }, { 1.4f, 6.0f, 1.4f }), stone);
            r.DrawLit(r.cube, PartXf(p, { 0, 6.3f, 0 }, { 8.4f, 1.0f, 1.6f }), ColorMul(STONE_HI, p.tint));
            float pulse = 0.7f + 0.3f * sinf(time * 2.2f + p.pos.x * 0.1f);
            r.DrawLit(r.cube, PartXf(p, { 0, 3.0f, 0 }, { 5.6f, 5.6f, 0.18f }), ColorMul(PORTAL, pulse), 1.0f);
            break;
        }
    }
}

void World::Draw(Renderer& r, Vector3 camPos, float viewDistance, float time) const {
    scratch.clear();
    propGrid.Query({ camPos.x, camPos.z }, viewDistance, [&](int i) { scratch.push_back(i); });

    float vd2 = viewDistance * viewDistance;
    for (int i : scratch) {
        const Prop& p = props[i];
        float dx = p.pos.x - camPos.x, dz = p.pos.z - camPos.z;
        if (dx * dx + dz * dz > vd2) { r.stats.culledProps++; continue; }
        Vector3 center = { p.pos.x, p.pos.y + p.boundRadius * 0.5f, p.pos.z };
        if (!r.Visible(center, p.boundRadius)) { r.stats.culledProps++; continue; }
        DrawProp(r, p, time);
    }
}
