#include "settings/settings.h"
#include "core/log.h"
#include "core/paths.h"
#include "input/input.h"
#include <fstream>
#include <nlohmann/json.hpp>

using nlohmann::json;

bool LoadStartupGraphics(GraphicsSettings& graphics) {
    std::ifstream f(SettingsFilePath());
    if (!f.is_open()) return false;
    try {
        json j = json::parse(f);
        const json& g = j.value("graphics", json::object());
        graphics.fullscreen = g.value("fullscreen", graphics.fullscreen);
        graphics.width = g.value("width", graphics.width);
        graphics.height = g.value("height", graphics.height);
        return true;
    } catch (const std::exception& e) {
        LOG_WARN("Startup graphics unreadable (%s); using window defaults", e.what());
        return false;
    }
}

// value<T> with defaults keeps old settings files forward-compatible: new
// fields simply fall back instead of failing the whole load.
void Settings::Load(InputSystem& input) {
    input.ResetDefaults();

    std::ifstream f(SettingsFilePath());
    if (!f.is_open()) { LOG_INFO("No settings file; using defaults"); return; }

    try {
        json j = json::parse(f);
        const json& g = j.value("graphics", json::object());
        gfx.fullscreen      = g.value("fullscreen", gfx.fullscreen);
        gfx.width           = g.value("width", gfx.width);
        gfx.height          = g.value("height", gfx.height);
        gfx.fpsCap          = g.value("fps_cap", gfx.fpsCap);
        gfx.renderScale     = g.value("render_scale", gfx.renderScale);
        gfx.fovY            = g.value("fov", gfx.fovY);
        gfx.viewDistance    = g.value("view_distance", gfx.viewDistance);
        gfx.bloom           = g.value("bloom", gfx.bloom);
        gfx.particleDensity = g.value("particle_density", gfx.particleDensity);

        const json& a = j.value("audio", json::object());
        audio.master   = a.value("master", audio.master);
        audio.sfx      = a.value("sfx", audio.sfx);
        audio.music    = a.value("music", audio.music);
        audio.ambience = a.value("ambience", audio.ambience);

        const json& c = j.value("controls", json::object());
        controls.mouseSensitivity = c.value("mouse_sensitivity", controls.mouseSensitivity);
        controls.invertY          = c.value("invert_y", controls.invertY);

        const json& b = j.value("bindings", json::object());
        for (int i = 0; i < ACTION_COUNT; i++) {
            const char* name = InputSystem::ActionName((Action)i);
            if (!b.contains(name)) continue;
            const json& e = b[name];
            input.bindings[i].primary = e.value("primary", input.bindings[i].primary);
            input.bindings[i].secondary = e.value("secondary", input.bindings[i].secondary);
        }
        LOG_INFO("Settings loaded from %s", SettingsFilePath().c_str());
    } catch (const std::exception& e) {
        LOG_WARN("Settings file unreadable (%s); using defaults", e.what());
    }
}

void Settings::Save(const InputSystem& input) const {
    json j;
    j["graphics"] = {
        { "fullscreen", gfx.fullscreen },
        { "width", gfx.width },
        { "height", gfx.height },
        { "fps_cap", gfx.fpsCap },
        { "render_scale", gfx.renderScale },
        { "fov", gfx.fovY },
        { "view_distance", gfx.viewDistance },
        { "bloom", gfx.bloom },
        { "particle_density", gfx.particleDensity },
    };
    j["audio"] = {
        { "master", audio.master },
        { "sfx", audio.sfx },
        { "music", audio.music },
        { "ambience", audio.ambience },
    };
    j["controls"] = {
        { "mouse_sensitivity", controls.mouseSensitivity },
        { "invert_y", controls.invertY },
    };
    json b;
    for (int i = 0; i < ACTION_COUNT; i++) {
        b[InputSystem::ActionName((Action)i)] = {
            { "primary", input.bindings[i].primary },
            { "secondary", input.bindings[i].secondary },
        };
    }
    j["bindings"] = b;

    std::ofstream f(SettingsFilePath());
    if (!f.is_open()) { LOG_ERROR("Cannot write %s", SettingsFilePath().c_str()); return; }
    f << j.dump(2);
}
