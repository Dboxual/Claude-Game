#include "input/input.h"
#include "raymath.h"
#include <cstdio>
#include <cstring>

void InputSystem::ResetDefaults() {
    auto set = [&](Action a, int primary, int secondary = KEY_NULL) {
        bindings[(int)a] = { primary, secondary };
    };
    set(Action::MoveForward,  KEY_W, KEY_UP);
    set(Action::MoveBack,     KEY_S, KEY_DOWN);
    set(Action::MoveLeft,     KEY_A, KEY_LEFT);
    set(Action::MoveRight,    KEY_D, KEY_RIGHT);
    set(Action::Jump,         KEY_SPACE);
    set(Action::Sprint,       KEY_LEFT_SHIFT);
    set(Action::Interact,     KEY_E);
    set(Action::ToggleCamera, KEY_V);
    set(Action::Pause,        KEY_ESCAPE);
    set(Action::DebugOverlay, KEY_F3);
}

bool InputSystem::CodeDown(int code) const {
    if (code == KEY_NULL) return false;
    if (code >= MOUSE_CODE_BASE) return IsMouseButtonDown(code - MOUSE_CODE_BASE);
    return IsKeyDown(code);
}

bool InputSystem::CodePressed(int code) const {
    if (code == KEY_NULL) return false;
    if (code >= MOUSE_CODE_BASE) return IsMouseButtonPressed(code - MOUSE_CODE_BASE);
    return IsKeyPressed(code);
}

void InputSystem::BeginFrame() {
    // An ESC that cancelled a rebind capture must not double as Pause when
    // it is still held on the following frames.
    if (escLatched && !IsKeyDown(KEY_ESCAPE)) escLatched = false;

    for (int i = 0; i < ACTION_COUNT; i++) {
        bool was = down[i];
        // While rebinding, gameplay must not react to the keys being pressed.
        bool now = !capturing && (CodeDown(bindings[i].primary) || CodeDown(bindings[i].secondary));
        // Escape is a permanent safety/navigation key even if a damaged or
        // user-edited settings file removes the Pause binding. Rebinding can
        // add another key, but it may never make the game impossible to exit.
        if (!capturing && i == (int)Action::Pause && IsKeyDown(KEY_ESCAPE)) now = true;
        if (escLatched && (bindings[i].primary == KEY_ESCAPE ||
                           bindings[i].secondary == KEY_ESCAPE)) now = false;
        down[i] = now;
        pressed[i] = now && !was;
        released[i] = !now && was;
    }
    if (capturing) PollCapture();
}

Vector2 InputSystem::MoveAxis() const {
    Vector2 v = { 0, 0 };
    if (Down(Action::MoveForward)) v.y += 1;
    if (Down(Action::MoveBack))    v.y -= 1;
    if (Down(Action::MoveRight))   v.x += 1;
    if (Down(Action::MoveLeft))    v.x -= 1;
    if (v.x != 0 || v.y != 0) v = Vector2Normalize(v);
    return v;
}

Vector2 InputSystem::LookDelta() const {
    return GetMouseDelta();
}

void InputSystem::InjectTestAction(Action a, bool isDown, bool isPressed) {
    int i = (int)a;
    down[i] = isDown;
    pressed[i] = isPressed;
    released[i] = false;
}

void InputSystem::StartCapture(Action a, int slot) {
    capturing = true;
    captureAction = a;
    captureSlot = slot;
}

void InputSystem::PollCapture() {
    if (!capturing) return;
    if (IsKeyPressed(KEY_ESCAPE)) { capturing = false; escLatched = true; return; }

    int code = KEY_NULL;
    int key = GetKeyPressed();
    if (key != 0) code = key;
    for (int b = 0; b <= MOUSE_BUTTON_BACK && code == KEY_NULL; b++)
        if (IsMouseButtonPressed(b)) code = MOUSE_CODE_BASE + b;
    if (code == KEY_NULL) return;

    Binding& bind = bindings[(int)captureAction];
    // The same code may not live on two actions: steal it from the old owner.
    for (int i = 0; i < ACTION_COUNT; i++) {
        if (bindings[i].primary == code) bindings[i].primary = KEY_NULL;
        if (bindings[i].secondary == code) bindings[i].secondary = KEY_NULL;
    }
    if (captureSlot == 0) bind.primary = code; else bind.secondary = code;
    capturing = false;
}

const char* InputSystem::ActionName(Action a) {
    switch (a) {
        case Action::MoveForward:  return "move_forward";
        case Action::MoveBack:     return "move_back";
        case Action::MoveLeft:     return "move_left";
        case Action::MoveRight:    return "move_right";
        case Action::Jump:         return "jump";
        case Action::Sprint:       return "sprint";
        case Action::Interact:     return "interact";
        case Action::ToggleCamera: return "toggle_camera";
        case Action::Pause:        return "pause";
        case Action::DebugOverlay: return "debug_overlay";
        default:                   return "unknown";
    }
}

const char* InputSystem::ActionLabel(Action a) {
    switch (a) {
        case Action::MoveForward:  return "Move Forward";
        case Action::MoveBack:     return "Move Back";
        case Action::MoveLeft:     return "Move Left";
        case Action::MoveRight:    return "Move Right";
        case Action::Jump:         return "Jump";
        case Action::Sprint:       return "Sprint";
        case Action::Interact:     return "Interact";
        case Action::ToggleCamera: return "Toggle Camera";
        case Action::Pause:        return "Pause / Menu";
        case Action::DebugOverlay: return "Debug Overlay";
        default:                   return "?";
    }
}

const char* InputSystem::CodeLabel(int code) {
    static char buf[24];
    if (code == KEY_NULL) return "--";
    if (code >= MOUSE_CODE_BASE) {
        switch (code - MOUSE_CODE_BASE) {
            case MOUSE_BUTTON_LEFT:   return "Mouse L";
            case MOUSE_BUTTON_RIGHT:  return "Mouse R";
            case MOUSE_BUTTON_MIDDLE: return "Mouse M";
            default: snprintf(buf, sizeof(buf), "Mouse %d", code - MOUSE_CODE_BASE + 1); return buf;
        }
    }
    if (code >= KEY_A && code <= KEY_Z) { snprintf(buf, sizeof(buf), "%c", 'A' + (code - KEY_A)); return buf; }
    if (code >= KEY_ZERO && code <= KEY_NINE) { snprintf(buf, sizeof(buf), "%c", '0' + (code - KEY_ZERO)); return buf; }
    if (code >= KEY_F1 && code <= KEY_F12) { snprintf(buf, sizeof(buf), "F%d", code - KEY_F1 + 1); return buf; }
    switch (code) {
        case KEY_SPACE:         return "Space";
        case KEY_LEFT_SHIFT:    return "L Shift";
        case KEY_RIGHT_SHIFT:   return "R Shift";
        case KEY_LEFT_CONTROL:  return "L Ctrl";
        case KEY_RIGHT_CONTROL: return "R Ctrl";
        case KEY_LEFT_ALT:      return "L Alt";
        case KEY_TAB:           return "Tab";
        case KEY_ENTER:         return "Enter";
        case KEY_BACKSPACE:     return "Backspace";
        case KEY_ESCAPE:        return "Esc";
        case KEY_UP:            return "Up";
        case KEY_DOWN:          return "Down";
        case KEY_LEFT:          return "Left";
        case KEY_RIGHT:         return "Right";
        case KEY_CAPS_LOCK:     return "Caps";
        default: snprintf(buf, sizeof(buf), "Key %d", code); return buf;
    }
}
