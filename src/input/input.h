// Action-based input. Gameplay code never touches raw keys — it asks for
// Actions, which user-rebindable bindings resolve. Mouse buttons are encoded
// as MOUSE_CODE_BASE + button so a binding slot holds either kind.
#pragma once
#include "raylib.h"

enum class Action : int {
    MoveForward, MoveBack, MoveLeft, MoveRight,
    Jump, Sprint, Interact, ToggleCamera,
    Pause, DebugOverlay,
    COUNT
};
constexpr int ACTION_COUNT = (int)Action::COUNT;
constexpr int MOUSE_CODE_BASE = 1000;   // binding code = MOUSE_CODE_BASE + MouseButton

struct Binding {
    int primary = KEY_NULL;
    int secondary = KEY_NULL;
};

class InputSystem {
public:
    void ResetDefaults();
    void BeginFrame();               // snapshots all action states once per frame

    bool Down(Action a) const     { return down[(int)a]; }
    bool Pressed(Action a) const  { return pressed[(int)a]; }
    bool Released(Action a) const { return released[(int)a]; }

    Vector2 MoveAxis() const;        // normalized, +y = forward
    Vector2 LookDelta() const;       // raw mouse pixels this frame (pre-sensitivity)

    // Automated recordings exercise the same action handlers as real keys
    // without requiring macOS Accessibility permission. Never used by normal
    // gameplay or dev mode.
    void InjectTestAction(Action a, bool isDown, bool isPressed = false);

    // Rebinding: StartCapture, then poll each frame; capture ends when a key
    // is taken (ESC cancels). While active, action states read as released.
    void StartCapture(Action a, int slot);
    void CancelCapture()          { capturing = false; }
    bool CaptureActive() const    { return capturing; }
    Action CaptureAction() const  { return captureAction; }
    int CaptureSlot() const       { return captureSlot; }
    void PollCapture();

    Binding bindings[ACTION_COUNT];
    static const char* ActionName(Action a);        // stable id for settings JSON
    static const char* ActionLabel(Action a);       // human-readable menu text
    static const char* CodeLabel(int code);         // "W", "Space", "Mouse L", "--"

private:
    bool CodeDown(int code) const;
    bool CodePressed(int code) const;

    bool down[ACTION_COUNT] = {};
    bool pressed[ACTION_COUNT] = {};
    bool released[ACTION_COUNT] = {};
    bool capturing = false;
    Action captureAction = Action::Jump;
    int captureSlot = 0;
    bool escLatched = false;   // ESC that cancelled a capture stays consumed
};
