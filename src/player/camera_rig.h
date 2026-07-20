// Dual camera rig: first person (primary) and third person (secondary),
// both first-class. Look input is applied per-frame for latency; the rig
// blends smoothly between modes and the TP boom shortens against world
// geometry. Feel layers: head bob, landing dip spring, sprint FOV kick.
#pragma once
#include "raylib.h"

class World;

enum class CameraView { FirstPerson, ThirdPerson };

class CameraRig {
public:
    void Init(float startYaw);
    void ToggleMode();
    void SetMode(CameraView v) { mode = v; }
    CameraView Mode() const { return mode; }
    // 0 = fully FP, 1 = fully TP; use for fading the player body in/out.
    float TpVisibility() const { return blend; }

    void AddLook(Vector2 pixelDelta, float sensitivityDeg, bool invertY);
    void OnLand(float landSpeed);

    // feetPos is the interpolated player position for this render frame.
    Camera3D Update(float dt, Vector3 feetPos, Vector3 vel, bool grounded,
                    bool sprinting, const World& world, float baseFovDeg);

    Vector3 Forward() const;
    float yaw = 0.0f;      // radians; 0 faces -Z
    float pitch = 0.0f;    // radians; positive looks up

private:
    CameraView mode = CameraView::FirstPerson;
    float blend = 0.0f;          // eased FP<->TP transition
    float bobPhase = 0.0f;
    float bobAmp = 0.0f;
    float dip = 0.0f, dipVel = 0.0f;   // landing spring (offset, velocity)
    float fovKick = 0.0f;
    float armLength = 4.6f;
};
