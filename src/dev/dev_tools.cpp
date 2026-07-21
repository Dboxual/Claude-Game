#include "dev/dev_tools.h"
#include "raylib.h"

DevCommands DevTools::Poll() const {
    DevCommands c;
    c.toggleOverlay = IsKeyPressed(KEY_F1);
    c.toggleFly = IsKeyPressed(KEY_F4);
    c.toggleTurbo = IsKeyPressed(KEY_F5);
    c.spawnReward = IsKeyPressed(KEY_F6);
    c.nextZone = IsKeyPressed(KEY_F7);
    c.returnToSpawn = IsKeyPressed(KEY_F8);
    c.advanceTime = IsKeyPressed(KEY_F9);
    if (IsKeyDown(KEY_SPACE)) c.vertical += 1.0f;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) c.vertical -= 1.0f;
    return c;
}
