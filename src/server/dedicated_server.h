#pragma once

#include "shared/config.h"
#include "shared/world.h"

#include <string>

// Loaded from config/server.cfg (key = value, '#' comments).
struct ServerConfig {
    std::string name = "TacMove Server";
    int maxPlayers = 16;
    float tickRate = 128.0f;

    static ServerConfig load(const std::string& path);
};

// Headless dedicated server. Runs the same fixed-tick cadence as the client
// and builds the same shared World, so server-side player simulation can
// drop straight in once networking lands. Links no windowing, graphics, or
// audio code - that is enforced by the build (tacmove_server -> tac_shared
// only).
class DedicatedServer {
public:
    explicit DedicatedServer(ServerConfig cfg) : cfg_(std::move(cfg)) {}

    // maxTicks < 0 runs until stop is requested (Ctrl+C). Returns exit code.
    int run(long long maxTicks);

    // Async-signal-safe; called from the SIGINT handler.
    static void requestStop();

private:
    ServerConfig cfg_;
    World world_;
    MovementConfig movement_; // shared movement constants, used by the future player sim
};
