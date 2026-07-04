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

    // Edge-triggered: true only on the pump where the press happened.
    bool jumpPressed = false;
    bool escapePressed = false;
    bool reloadConfigPressed = false;
    bool toggleHudPressed = false;
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
