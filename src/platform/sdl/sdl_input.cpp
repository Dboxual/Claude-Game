#include "platform/sdl/sdl_input.h"

const InputState& SdlInput::pump() {
    // Edges and deltas are per-pump; held state is re-sampled below.
    state_.jumpPressed = false;
    state_.escapePressed = false;
    state_.reloadConfigPressed = false;
    state_.toggleHudPressed = false;
    state_.mouseClicked = false;
    state_.lookDx = 0.0f;
    state_.lookDy = 0.0f;

    const bool captured = window_->relativeMouseMode();

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
            state_.quitRequested = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (captured) {
                state_.lookDx += e.motion.xrel;
                state_.lookDy += e.motion.yrel;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            state_.mouseClicked = true;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (e.key.repeat) break;
            if (e.key.key == SDLK_ESCAPE) state_.escapePressed = true;
            else if (e.key.key == SDLK_F5) state_.reloadConfigPressed = true;
            else if (e.key.key == SDLK_F1) state_.toggleHudPressed = true;
            else if (e.key.scancode == SDL_SCANCODE_SPACE) state_.jumpPressed = true;
            break;
        default:
            break;
        }
    }

    const bool* keys = SDL_GetKeyboardState(nullptr);
    state_.moveForward = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);
    state_.moveRight = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);
    state_.jumpHeld = keys[SDL_SCANCODE_SPACE];
    state_.crouchHeld = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_C];
    state_.walkHeld = keys[SDL_SCANCODE_LSHIFT];

    return state_;
}
