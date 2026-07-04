#pragma once

#include "client/platform/input.h"
#include "client/platform/sdl/sdl_window.h"

class SdlInput final : public IInput {
public:
    explicit SdlInput(SdlWindow& window) : window_(&window) {}

    const InputState& pump() override;

private:
    SdlWindow* window_;
    InputState state_;
};
