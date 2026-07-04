#include "server/dedicated_server.h"

#include "shared/log.h"
#include "shared/protocol.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <fstream>
#include <sstream>
#include <thread>

namespace {

volatile std::sig_atomic_t gStopRequested = 0;

} // namespace

void DedicatedServer::requestStop() {
    gStopRequested = 1;
}

ServerConfig ServerConfig::load(const std::string& path) {
    ServerConfig out;

    std::ifstream file(path);
    if (!file.is_open()) {
        eng::logInfo("No server config at '%s' - using defaults", path.c_str());
        return out;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    ConfigFile cfg;
    cfg.parseText(buffer.str());
    out.name = cfg.getString("server_name", out.name);
    out.maxPlayers = std::clamp(int(cfg.get("max_players", float(out.maxPlayers))), 1, 128);
    out.tickRate = std::clamp(cfg.get("tick_rate", out.tickRate), 10.0f, 512.0f);
    eng::logInfo("Loaded server config: %s", path.c_str());
    return out;
}

int DedicatedServer::run(long long maxTicks) {
    eng::logInfo("==============================================");
    eng::logInfo("  TacMove dedicated server");
    eng::logInfo("  Name:        %s", cfg_.name.c_str());
    eng::logInfo("  Max players: %d", cfg_.maxPlayers);
    eng::logInfo("  Tick rate:   %d Hz", int(cfg_.tickRate));
    eng::logInfo("  Protocol:    v%d (networking not implemented yet)", net::kProtocolVersion);
    eng::logInfo("==============================================");

    world_.buildTestMap();
    eng::logInfo("World ready: %d collision boxes (built-in test arena)",
                 int(world_.boxes().size()));
    eng::logInfo("Movement constants loaded (run %.1f m/s, tick sim shared with client)",
                 movement_.runSpeed);
    eng::logInfo("Server is up. Press Ctrl+C to stop.");

    using Clock = std::chrono::steady_clock;
    const double tickDt = 1.0 / cfg_.tickRate;
    const Clock::time_point start = Clock::now();
    Clock::time_point last = start;
    Clock::time_point lastStatus = start;
    double accumulator = 0.0;
    long long ticks = 0;

    while (!gStopRequested && (maxTicks < 0 || ticks < maxTicks)) {
        Clock::time_point now = Clock::now();
        accumulator += std::chrono::duration<double>(now - last).count();
        last = now;
        accumulator = std::min(accumulator, 0.5); // don't spiral after a stall

        while (accumulator >= tickDt) {
            // Placeholder for the real work: once networking lands, connected
            // players tick the shared movement sim (Player::tick against
            // world_ using movement_) right here, identically to the client.
            ++ticks;
            accumulator -= tickDt;
            if (maxTicks >= 0 && ticks >= maxTicks) break;
        }

        if (now - lastStatus >= std::chrono::seconds(10)) {
            long long uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            eng::logInfo("status | uptime %llds | tick %lld | players 0/%d",
                         uptime, ticks, cfg_.maxPlayers);
            lastStatus = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    long long uptime = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - start).count();
    eng::logInfo("Shutting down '%s' after %lld ticks (%llds uptime). Goodbye.",
                 cfg_.name.c_str(), ticks, uptime);
    return 0;
}
