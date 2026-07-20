#include "fx/particles.h"
#include "core/mathx.h"
#include "rlgl.h"

void ParticleSystem::Init() {
    // Soft radial dot: the one texture every particle uses.
    Image img = GenImageGradientRadial(64, 64, 0.0f, WHITE, BLANK);
    softDot = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(softDot, TEXTURE_FILTER_BILINEAR);
}

void ParticleSystem::Shutdown() {
    UnloadTexture(softDot);
}

float ParticleSystem::Rand01() {
    // xorshift32 — decorative randomness, determinism not required here.
    rngState ^= rngState << 13;
    rngState ^= rngState >> 17;
    rngState ^= rngState << 5;
    return (rngState & 0xFFFFFF) / 16777215.0f;
}

void ParticleSystem::Emit(const Particle& p) {
    // Density scaling drops whole particles instead of shrinking them —
    // cheaper and keeps the look crisp on low settings.
    if (density < 1.0f && Rand01() > density) return;
    if (alive >= MAX_PARTICLES) return;
    pool[alive] = p;
    pool[alive].maxLife = p.life;
    alive++;
}

void ParticleSystem::Update(float dt) {
    for (int i = 0; i < alive;) {
        Particle& p = pool[i];
        p.life -= dt;
        if (p.life <= 0.0f) { pool[i] = pool[--alive]; continue; }  // swap-erase
        p.vel.y -= p.gravity * dt;
        if (p.drag > 0.0f) {
            float k = expf(-p.drag * dt);
            p.vel.x *= k; p.vel.y *= k; p.vel.z *= k;
        }
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.pos.z += p.vel.z * dt;
        i++;
    }
}

void ParticleSystem::Draw(const Camera3D& cam) {
    // Two passes: alpha-blended dust first, additive glow on top.
    for (int pass = 0; pass < 2; pass++) {
        bool additivePass = (pass == 1);
        BeginBlendMode(additivePass ? BLEND_ADDITIVE : BLEND_ALPHA);
        for (int i = 0; i < alive; i++) {
            const Particle& p = pool[i];
            if (p.additive != additivePass) continue;
            float t = 1.0f - p.life / p.maxLife;
            float size = Lerp(p.sizeA, p.sizeB, t);
            Color c = ColorLerpF(p.colA, p.colB, t);
            DrawBillboard(cam, softDot, p.pos, size, c);
        }
        EndBlendMode();
    }
}

// ---- feel helpers ---------------------------------------------------------

void ParticleSystem::FootstepPuff(Vector3 feet, Vector3 vel) {
    for (int i = 0; i < 3; i++) {
        Particle p;
        p.pos = { feet.x + RandRange(-0.15f, 0.15f), feet.y + 0.06f,
                  feet.z + RandRange(-0.15f, 0.15f) };
        p.vel = { -vel.x * 0.06f + RandRange(-0.25f, 0.25f), RandRange(0.3f, 0.7f),
                  -vel.z * 0.06f + RandRange(-0.25f, 0.25f) };
        p.life = RandRange(0.35f, 0.6f);
        p.sizeA = RandRange(0.12f, 0.2f); p.sizeB = 0.42f;
        p.colA = { 168, 152, 122, 130 }; p.colB = { 168, 152, 122, 0 };
        p.drag = 2.5f;
        Emit(p);
    }
}

void ParticleSystem::LandBurst(Vector3 feet, float impactSpeed) {
    int n = (int)Clamp(impactSpeed * 2.2f, 6.0f, 18.0f);
    for (int i = 0; i < n; i++) {
        float ang = RandRange(0.0f, PI * 2.0f);
        float spd = RandRange(0.8f, 2.2f);
        Particle p;
        p.pos = { feet.x, feet.y + 0.08f, feet.z };
        p.vel = { sinf(ang) * spd, RandRange(0.5f, 1.4f), cosf(ang) * spd };
        p.life = RandRange(0.4f, 0.75f);
        p.sizeA = 0.16f; p.sizeB = 0.5f;
        p.colA = { 172, 156, 126, 150 }; p.colB = { 172, 156, 126, 0 };
        p.gravity = 2.0f; p.drag = 2.2f;
        Emit(p);
    }
}

void ParticleSystem::ShrineBurst(Vector3 center, Color color) {
    for (int i = 0; i < 42; i++) {
        // Sphere-shell burst with upward bias — reads as released energy.
        float a = RandRange(0.0f, PI * 2.0f);
        float e = RandRange(-0.4f, 1.0f);
        float spd = RandRange(2.0f, 5.5f);
        Particle p;
        p.pos = center;
        p.vel = { sinf(a) * cosf(e) * spd, sinf(e) * spd + 1.5f, cosf(a) * cosf(e) * spd };
        p.life = RandRange(0.6f, 1.3f);
        p.sizeA = RandRange(0.1f, 0.22f); p.sizeB = 0.02f;
        p.colA = color; p.colB = { color.r, color.g, color.b, 0 };
        p.gravity = -0.8f;   // drifts upward
        p.drag = 1.6f;
        p.additive = true;
        Emit(p);
    }
}

void ParticleSystem::WispTrail(Vector3 pos, Color color) {
    Particle p;
    p.pos = { pos.x + RandRange(-0.06f, 0.06f), pos.y + RandRange(-0.06f, 0.06f),
              pos.z + RandRange(-0.06f, 0.06f) };
    p.vel = { RandRange(-0.15f, 0.15f), RandRange(-0.05f, 0.25f), RandRange(-0.15f, 0.15f) };
    p.life = RandRange(0.3f, 0.55f);
    p.sizeA = 0.16f; p.sizeB = 0.02f;
    p.colA = color; p.colB = { color.r, color.g, color.b, 0 };
    p.additive = true;
    Emit(p);
}

void ParticleSystem::CollectBurst(Vector3 pos, Color color) {
    for (int i = 0; i < 10; i++) {
        float a = RandRange(0.0f, PI * 2.0f);
        Particle p;
        p.pos = pos;
        p.vel = { sinf(a) * RandRange(0.6f, 1.6f), RandRange(0.4f, 1.8f),
                  cosf(a) * RandRange(0.6f, 1.6f) };
        p.life = RandRange(0.25f, 0.5f);
        p.sizeA = 0.14f; p.sizeB = 0.02f;
        p.colA = color; p.colB = { color.r, color.g, color.b, 0 };
        p.drag = 3.0f;
        p.additive = true;
        Emit(p);
    }
}

void ParticleSystem::AmbientMotes(Vector3 around, float dt) {
    // Gentle golden motes drifting near the player — the air itself feels
    // enchanted. Spawn rate is small and density-scaled via Emit.
    moteAccum += dt * 5.0f;
    while (moteAccum >= 1.0f) {
        moteAccum -= 1.0f;
        float a = RandRange(0.0f, PI * 2.0f);
        float r = RandRange(3.0f, 14.0f);
        Particle p;
        p.pos = { around.x + sinf(a) * r, around.y + RandRange(0.2f, 3.5f),
                  around.z + cosf(a) * r };
        p.vel = { RandRange(-0.12f, 0.12f), RandRange(0.04f, 0.16f), RandRange(-0.12f, 0.12f) };
        p.life = RandRange(2.5f, 4.5f);
        p.sizeA = 0.05f; p.sizeB = 0.09f;
        p.colA = { 255, 226, 150, 85 };     // soft glint fading away
        p.colB = { 255, 226, 150, 0 };
        p.additive = true;
        Emit(p);
    }
}
