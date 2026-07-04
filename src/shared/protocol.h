#pragma once

// Placeholder for the future client<->server protocol. No real networking
// exists yet - this is the one shared constant both sides will check when it
// does, bumped on every breaking change. Message definitions will live here
// so client and server can never drift apart.
namespace net {

inline constexpr int kProtocolVersion = 1;

} // namespace net
