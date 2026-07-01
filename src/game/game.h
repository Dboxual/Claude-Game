#pragma once

#include "engine/config.h"
#include "game/player.h"
#include "game/world.h"
#include "platform/filesystem.h"
#include "platform/input.h"
#include "renderer/render_types.h"

#include <glm/glm.hpp>

#include <string>

// The whole simulation behind the platform seams: consumes InputState,
// steps the fixed-tick movement sim, and emits a RenderFrame. Contains no
// windowing, graphics-API, or OS calls - that is what keeps the game
// portable to Android/iOS/console targets.
class Game {
public:
    bool init(IFileSystem& fs);
    void update(const InputState& in, double frameDt);
    RenderFrame buildRenderFrame(int viewportW, int viewportH) const;

    // Purely informational (shown on the debug HUD).
    void setRendererName(std::string name) { rendererName_ = std::move(name); }

private:
    void loadConfig();

    IFileSystem* fs_ = nullptr;
    std::string cfgPath_;
    ConfigFile cfgFile_;
    MovementConfig cfg_;

    World world_;
    Player player_;
    glm::vec3 prevPos_{0.0f}; // position before the latest tick, for interpolation

    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float eyeHeight_ = 1.65f;
    double tickDt_ = 1.0 / 128.0;
    double accumulator_ = 0.0;
    double jumpBufferTimer_ = -1.0;
    double fpsSmoothed_ = 0.0;
    bool showHud_ = true;
    bool walkHeldHud_ = false;
    std::string rendererName_ = "unknown";
};
