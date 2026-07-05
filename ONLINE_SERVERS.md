# What the Online server list needs (not built yet)

The server browser has an **Online** tab that currently says only:
*"Online server list requires master server backend."* That is deliberate —
nothing is faked. This document is the checklist of what actually has to
exist before that tab can show real servers. None of it is implemented; the
game today has no networking at all (see `src/shared/protocol.h`, which only
reserves a protocol version).

## 1. Master server API

A small HTTPS service the game talks to. Minimum endpoints:

| Endpoint                 | Who calls it | Purpose |
| ------------------------ | ------------ | ------- |
| `POST /v1/servers`       | game server  | register/heartbeat: address, port, name, map, player count, max players, protocol version |
| `GET /v1/servers`        | client       | fetch the list (paged, filterable by region/map/not-full) |
| `DELETE /v1/servers/:id` | game server  | clean shutdown de-registration (optional; timeout covers crashes) |

Design notes:
- Plain JSON over HTTPS is enough at this scale; no need for a custom
  protocol where a load balancer and a cache will do.
- The master server must verify the heartbeat's source: at minimum, probe
  the advertised `address:port` with a query (section 3) before listing it,
  so entries can't advertise someone else's IP (reflection/spoofing guard).
- Servers that miss ~3 heartbeat intervals (e.g. 3 × 60 s) drop off the list.

## 2. Server heartbeat / register (game server side)

The dedicated server (`src/server/`) needs a background task that:

- registers on startup and re-sends every ~60 s (name, map, players, slots,
  protocol version, password-protected flag);
- de-registers on clean shutdown;
- keeps working when the master is down (the server stays joinable by
  direct connect — the master is discovery only, never a dependency).

## 3. Server query / status protocol

A lightweight UDP query (separate from the future game protocol) so clients
and the master can ask any server directly: name, map, player count/slots,
protocol version, and a ping measurement from the exchange. Rules:

- The response must be **smaller** than the request (pad the request), so
  the query can't be abused for DDoS amplification.
- Include a challenge token (server sends a nonce, client echoes it) to stop
  spoofed-source floods.
- This is also what the **LAN tab** will use, via UDP broadcast on the local
  network — same packet, no master server involved.

## 4. Client fetches the online list

The client's Online tab then becomes:

1. `GET /v1/servers` from the master (async — the UI must not block);
2. query each listed server directly (section 3) to verify it is reachable
   and to measure real ping;
3. render rows: name, map, players, ping; sort/filter; join = the same code
   path as Direct Connect;
4. unreachable-but-listed servers are shown greyed out or dropped, never
   invented. Favorites/History tabs persist addresses locally and re-query
   them the same way.

## 5. NAT / port forwarding notes

- A server behind NAT must forward its game UDP port (and query port, if
  separate) or it will register with the master and still be unjoinable —
  the master's verification probe (section 1) should catch this and warn
  the operator in the server console.
- Document the default port (`27015` per the current placeholder) in the
  server README so router setup instructions are copy-pasteable.
- Hosting from home should be treated as best effort; recommend rented
  hosts for public servers. NAT punch-through / relays are a much later
  concern and need the master server to act as a rendezvous point.

## 6. Moderation / security basics

Minimum bar before public listing exists:

- **Input validation everywhere**: server names, map names and chat are
  attacker-controlled strings shown in other people's UI — length-limit,
  strip control characters, and never interpret them.
- **Protocol version gating**: mismatched clients/servers refuse cleanly
  (protocol.h's version exists exactly for this).
- **Rate limiting** on the master (per-IP registration and query caps) so
  one host can't flood the list or use it as an amplifier.
- **Delisting mechanism**: master server operators need a way to remove
  malicious or impersonating servers (stable server IDs + a blocklist).
- **No trust in the client**: once real networking lands, the server
  simulates and validates; the client only sends inputs. This is already
  the plan for the netcode milestone and applies to the browser too (e.g.
  reported player counts come from the server's own query response, not
  the registration payload).
- **Player-side basics**: password-protected servers, and a client-side
  server blocklist, before any community listing goes live.

## Explicit non-goals right now

No accounts, no mod distribution, no scripting, no marketplace, no
matchmaking. The list above is only about making the Online tab honest.
