#include "world/interact.h"
#include "core/mathx.h"
#include "fx/particles.h"
#include "render/renderer.h"

static constexpr float REARM_TIME = 30.0f;      // shrines recharge (playground)
static constexpr float WISP_MAX_SPEED = 17.0f;
static constexpr float WISP_COLLECT_DIST = 0.6f;
static const Color WISP_COLOR = { 140, 245, 215, 255 };

float InteractionSystem::Rand01() {
    rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
    return (rngState & 0xFFFFFF) / 16777215.0f;
}

void InteractionSystem::Reset() {
    items.clear();
    wisps.clear();
}

int InteractionSystem::Add(const Interactable& item) {
    items.push_back(item);
    return (int)items.size() - 1;
}

int InteractionSystem::FindTarget(Vector3 playerPos, Vector3 eyePos, Vector3 eyeFw) const {
    int best = -1;
    float bestScore = 0.72f;   // minimum gaze alignment to count as "looking at"
    for (int i = 0; i < (int)items.size(); i++) {
        const Interactable& it = items[i];
        float dx = it.pos.x - playerPos.x, dz = it.pos.z - playerPos.z;
        if (dx * dx + dz * dz > it.radius * it.radius) continue;

        Vector3 to = Vector3Normalize(Vector3Subtract(it.pos, eyePos));
        float gaze = Vector3DotProduct(to, eyeFw);
        if (gaze > bestScore) { bestScore = gaze; best = i; }
    }
    return best;
}

bool InteractionSystem::Activate(int index, int wispCount) {
    if (index < 0 || index >= (int)items.size()) return false;
    Interactable& it = items[index];
    if (it.rearm > 0.0f) return false;
    it.rearm = REARM_TIME;
    it.glow = 1.0f;

    // Wisps scatter outward first, then home in — anticipation before payout.
    for (int i = 0; i < wispCount; i++) {
        Wisp w;
        w.alive = true;
        w.pos = it.pos;
        float a = Rand01() * PI * 2.0f;
        float e = Rand01() * 1.1f - 0.15f;
        float spd = 3.0f + Rand01() * 3.5f;
        w.vel = { sinf(a) * cosf(e) * spd, sinf(e) * spd + 2.0f, cosf(a) * cosf(e) * spd };
        w.delay = 0.35f + Rand01() * 0.55f;
        wisps.push_back(w);
    }
    return true;
}

InteractEvents InteractionSystem::Update(float dt, Vector3 chest, ParticleSystem& fx) {
    InteractEvents ev;

    for (Interactable& it : items) {
        it.rearm = fmaxf(it.rearm - dt, 0.0f);
        it.glow = ExpDecay(it.glow, 0.0f, 1.8f, dt);
    }

    for (size_t i = 0; i < wisps.size();) {
        Wisp& w = wisps[i];
        w.age += dt;
        if (w.age < w.delay) {
            // Scatter phase: decelerate, leave a sparse trail.
            w.vel = Vector3Scale(w.vel, expf(-3.0f * dt));
        } else {
            // Homing phase: accelerate toward the player, capped speed that
            // rises with age so stragglers always catch up.
            Vector3 to = Vector3Subtract(chest, w.pos);
            float dist = Vector3Length(to);
            if (dist < WISP_COLLECT_DIST) {
                ev.wispsCollected++;
                ev.lastCollectPos = w.pos;
                fx.CollectBurst(w.pos, WISP_COLOR);
                wisps[i] = wisps.back();
                wisps.pop_back();
                continue;
            }
            // Velocity steering (not raw acceleration): tangential speed dies
            // off, so wisps can never orbit out of reach. Both the speed cap
            // and steering rate rise with age — stragglers always catch up.
            Vector3 dir = Vector3Scale(to, 1.0f / dist);
            float homing = w.age - w.delay;
            float cap = fminf(6.0f + homing * 8.0f, WISP_MAX_SPEED);
            float steer = 4.0f + homing * 3.5f;
            w.vel = ExpDecayV3(w.vel, Vector3Scale(dir, cap), steer, dt);
        }
        w.pos = Vector3Add(w.pos, Vector3Scale(w.vel, dt));
        if ((int)(w.age * 30.0f) != (int)((w.age - dt) * 30.0f))   // ~30 Hz trail
            fx.WispTrail(w.pos, WISP_COLOR);
        i++;
    }
    return ev;
}

float InteractionSystem::LightBoost(int lightIndex) const {
    for (const Interactable& it : items)
        if (it.lightIndex == lightIndex) return 1.0f + it.glow * 2.2f;
    return 1.0f;
}

void InteractionSystem::DrawWisps(Renderer& r, float time) const {
    for (const Wisp& w : wisps) {
        if (!w.alive) continue;
        float pulse = 0.8f + 0.2f * sinf(time * 9.0f + w.pos.x * 3.0f);
        Matrix m = MatrixMultiply(MatrixScale(0.22f, 0.22f, 0.22f),
                                  MatrixTranslate(w.pos.x, w.pos.y, w.pos.z));
        r.DrawLit(r.sphere, m, ColorMul(WISP_COLOR, pulse), 1.0f);
    }
}
