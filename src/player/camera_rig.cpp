#include "player/camera_rig.h"
#include "core/mathx.h"
#include "player/controller.h"
#include "world/world.h"

// Camera feel tuning.
static constexpr float PITCH_LIMIT = 88.0f * DEG2RAD;
static constexpr float BLEND_RATE = 11.0f;         // FP<->TP transition speed
static constexpr float BOB_FREQ = 1.55f;           // cycles per meter walked
static constexpr float BOB_AMP_WALK = 0.040f;
static constexpr float BOB_AMP_SPRINT = 0.062f;
static constexpr float DIP_STIFFNESS = 90.0f;      // landing spring
static constexpr float DIP_DAMPING = 14.0f;
static constexpr float SPRINT_FOV_KICK = 7.0f;     // degrees
static constexpr float TP_ARM = 4.6f;              // desired boom length (m)
static constexpr float TP_SHOULDER = 0.55f;        // right offset (m)
static constexpr float TP_PIVOT_HEIGHT = 1.55f;

void CameraRig::Init(float startYaw) {
    yaw = startYaw;
    pitch = -0.06f;
    blend = (mode == CameraView::ThirdPerson) ? 1.0f : 0.0f;
}

void CameraRig::ToggleMode() {
    mode = (mode == CameraView::FirstPerson) ? CameraView::ThirdPerson : CameraView::FirstPerson;
}

void CameraRig::AddLook(Vector2 d, float sensDeg, bool invertY) {
    yaw += d.x * sensDeg * DEG2RAD;
    pitch += (invertY ? d.y : -d.y) * sensDeg * DEG2RAD;
    yaw = WrapAngle(yaw);
    pitch = Clamp(pitch, -PITCH_LIMIT, PITCH_LIMIT);
}

void CameraRig::OnLand(float landSpeed) {
    // Impacts kick the spring downward; capped so cliffs don't nauseate.
    dipVel -= Clamp(landSpeed * 0.055f, 0.06f, 0.5f);
}

Vector3 CameraRig::Forward() const {
    float cp = cosf(pitch);
    return { sinf(yaw) * cp, sinf(pitch), -cosf(yaw) * cp };
}

Camera3D CameraRig::Update(float dt, Vector3 feet, Vector3 vel, bool grounded,
                           bool sprinting, const World& world, float baseFovDeg) {
    float targetBlend = (mode == CameraView::ThirdPerson) ? 1.0f : 0.0f;
    blend = ExpDecay(blend, targetBlend, BLEND_RATE, dt);
    float b = SmoothStep01(blend);

    // Head bob: phase advances with distance walked; amplitude eases in/out.
    float speed = sqrtf(vel.x * vel.x + vel.z * vel.z);
    bool striding = grounded && speed > 0.8f;
    if (striding) bobPhase += speed * BOB_FREQ * dt * 2.0f * PI / 2.15f;
    float ampTarget = striding ? (sprinting ? BOB_AMP_SPRINT : BOB_AMP_WALK) : 0.0f;
    bobAmp = ExpDecay(bobAmp, ampTarget, 8.0f, dt);

    // Landing dip: critically-damped-ish spring back to zero.
    dipVel += (-dip * DIP_STIFFNESS - dipVel * DIP_DAMPING) * dt;
    dip += dipVel * dt;

    fovKick = ExpDecay(fovKick, (sprinting && speed > 5.0f) ? SPRINT_FOV_KICK : 0.0f, 5.0f, dt);

    Vector3 fw = Forward();
    Vector3 rt = { cosf(yaw), 0, sinf(yaw) };

    // First-person eye.
    float bobY = sinf(bobPhase * 2.0f) * bobAmp;
    float bobX = sinf(bobPhase) * bobAmp * 0.55f;
    Vector3 fpEye = {
        feet.x + rt.x * bobX,
        feet.y + PlayerController::EYE + bobY + dip,
        feet.z + rt.z * bobX,
    };

    // Third-person boom: pull back from the pivot, shortening where blocked.
    Vector3 pivot = { feet.x, feet.y + TP_PIVOT_HEIGHT + dip * 0.5f, feet.z };
    Vector3 boomDir = Vector3Negate(fw);
    Vector3 shoulder = Vector3Scale(rt, TP_SHOULDER);
    float clearArm = TP_ARM;
    for (int i = 1; i <= 14; i++) {
        float t = TP_ARM * i / 14.0f;
        Vector3 p = Vector3Add(Vector3Add(pivot, Vector3Scale(boomDir, t)), shoulder);
        if (world.PointBlocked(p, 0.30f)) { clearArm = fmaxf(t - TP_ARM / 14.0f, 0.4f); break; }
    }
    // Snap in instantly (never clip a wall), ease back out.
    armLength = (clearArm < armLength) ? clearArm : ExpDecay(armLength, clearArm, 3.5f, dt);
    Vector3 tpPos = Vector3Add(Vector3Add(pivot, Vector3Scale(boomDir, armLength)), shoulder);
    Vector3 tpTarget = Vector3Add(Vector3Add(pivot, Vector3Scale(fw, 4.0f)), shoulder);

    Camera3D cam = {};
    cam.position = Vector3Lerp(fpEye, tpPos, b);
    cam.target = Vector3Lerp(Vector3Add(fpEye, fw), tpTarget, b);
    cam.up = { 0, 1, 0 };
    cam.fovy = baseFovDeg + fovKick + b * 5.0f;   // TP reads better slightly wide
    cam.projection = CAMERA_PERSPECTIVE;
    return cam;
}
