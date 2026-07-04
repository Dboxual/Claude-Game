// TacMove dedicated server - entry point.
//
// Headless by construction: this binary links only tac_shared (simulation,
// config, logging). Usage:
//   tacmove_server [--config path/to/server.cfg] [--ticks N]
// --ticks N exits after N simulation ticks (smoke testing); the default is
// to run until Ctrl+C.

#include "server/dedicated_server.h"
#include "shared/log.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
    std::string configPath = "config/server.cfg";
    long long maxTicks = -1;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            configPath = argv[++i];
        } else if (std::strcmp(argv[i], "--ticks") == 0 && i + 1 < argc) {
            maxTicks = std::atoll(argv[++i]);
        } else {
            eng::logError("Unknown argument '%s' (expected --config <path> or --ticks <n>)", argv[i]);
            return 2;
        }
    }

    std::signal(SIGINT, [](int) { DedicatedServer::requestStop(); });
#if defined(SIGTERM)
    std::signal(SIGTERM, [](int) { DedicatedServer::requestStop(); });
#endif

    DedicatedServer server(ServerConfig::load(configPath));
    return server.run(maxTicks);
}
