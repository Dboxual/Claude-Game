#pragma once

// Device-independent input snapshot. Gameplay reads intents (move axes, jump)
// rather than keys, so touch controls or gamepads later only change the
// platform layer, never the game.
struct InputState {
    // Held state.
    float moveForward = 0.0f; // -1..1
    float moveRight = 0.0f;   // -1..1
    bool jumpHeld = false;
    bool crouchHeld = false;
    bool walkHeld = false;

    // Edge-triggered: true only on the pump where the press happened.
    bool jumpPressed = false;
    bool escapePressed = false;
    bool reloadConfigPressed = false;
    bool toggleHudPressed = false;
    bool mouseClicked = false;

    // Look delta for this pump, in device counts (only accumulates while the
    // mouse is captured).
    float lookDx = 0.0f;
    float lookDy = 0.0f;

    bool quitRequested = false;
};

class IInput {
public:
    virtual ~IInput() = default;

    // Processes pending OS events and returns the fresh snapshot.
    virtual const InputState& pump() = 0;
};
