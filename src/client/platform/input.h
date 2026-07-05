#pragma once

#include <string>

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

    // Held state (combat).
    bool attackHeld = false; // left mouse held (charges a heavy attack)
    bool blockHeld = false;  // right mouse held (guard up / parry timing)

    // Edge-triggered: true only on the pump where the press happened.
    bool jumpPressed = false;
    bool escapePressed = false;
    bool reloadConfigPressed = false;
    bool toggleHudPressed = false;
    bool spawnMenuPressed = false; // B toggles the dev spawn menu in-game
    bool interactPressed = false;  // E: pick up / use what the player looks at
    bool attackPressed = false;    // left mouse edge (gameplay attack)
    bool kickPressed = false;      // F: kick (opens raised guards)
    bool throwPressed = false;     // Q: throw the held melee weapon
    bool feintPressed = false;     // R or middle mouse: cancel the windup
    bool altAttackPressed = false; // Left Alt or mouse side button: alternate slash
    int wheelDelta = 0;            // scroll steps this pump: >0 up (overhead), <0 down (stab)
    int slotPressed = 0;           // 1..9 when a weapon-slot key was pressed, else 0
    bool mouseClicked = false;
    bool enterPressed = false;
    bool backspacePressed = false; // key-repeat counts, for held-delete in text fields

    // UTF-8 text typed this pump; only fills while the window has text input
    // active (see IWindow::setTextInputActive).
    std::string typedText;

    // Look delta for this pump, in device counts (only accumulates while the
    // mouse is captured).
    float lookDx = 0.0f;
    float lookDy = 0.0f;

    // Absolute cursor position in render pixels (top-left origin) for UI
    // hit-testing while the mouse is not captured.
    float mouseX = 0.0f;
    float mouseY = 0.0f;

    bool quitRequested = false;
};

class IInput {
public:
    virtual ~IInput() = default;

    // Processes pending OS events and returns the fresh snapshot.
    virtual const InputState& pump() = 0;
};
