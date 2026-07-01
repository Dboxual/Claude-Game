#pragma once

#include "platform/input.h"
#include "platform/sdl/sdl_window.h"

class SdlInput final : public IInput {
public:
    explicit SdlInput(SdlWindow& window) : window_(&window) {}

    const InputState& pump() override;

private:
    SdlWindow* window_;
    InputState state_;
};
