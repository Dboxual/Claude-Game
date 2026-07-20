#include "game/menus.h"
#include "core/mathx.h"
#include "input/input.h"
#include "settings/settings.h"
#include "ui/ui.h"
#include <cmath>

// ---- title ----------------------------------------------------------------

TitleAction DrawTitleMenu(Ui& ui, bool hasSave, float time) {
    float w = ui.ScreenW(), h = ui.ScreenH();

    // Wordmark with a slow golden breath.
    float pulse = 0.9f + 0.1f * sinf(time * 1.4f);
    ui.LabelCentered({ w / 2 + ui.S(3), h * 0.24f + ui.S(3) }, "Z I O N", 96,
                     Fade(BLACK, 0.45f));
    ui.LabelCentered({ w / 2, h * 0.24f }, "Z I O N", 96,
                     ColorMul(ui.theme.accentGlow, pulse));
    ui.LabelCentered({ w / 2, h * 0.24f + ui.S(64) },
                     "the vale remembers", 24, Fade(ui.theme.text, 0.75f));

    float bw = ui.S(300), bh = ui.S(54), gap = ui.S(16);
    float x = (w - bw) / 2;
    float y = h * 0.46f;
    TitleAction action = TitleAction::None;

    if (hasSave) {
        if (ui.Button("title.continue", { x, y, bw, bh }, "Continue"))
            action = TitleAction::Continue;
        y += bh + gap;
    }
    if (ui.Button("title.new", { x, y, bw, bh },
                  hasSave ? "New Journey" : "Begin the Journey"))
        action = TitleAction::NewJourney;
    y += bh + gap;
    if (ui.Button("title.settings", { x, y, bw, bh }, "Settings"))
        action = TitleAction::Settings;
    y += bh + gap;
    if (ui.Button("title.quit", { x, y, bw, bh }, "Quit"))
        action = TitleAction::Quit;

    // ASCII only: the UI font atlas bakes the default 95-glyph set.
    ui.Label({ ui.S(16), h - ui.S(30) }, "Zion 0.1.0 - Phase 1: Foundation", 16,
             Fade(ui.theme.textDim, 0.8f));
    return action;
}

// ---- pause ----------------------------------------------------------------

PauseAction DrawPauseMenu(Ui& ui) {
    float w = ui.ScreenW(), h = ui.ScreenH();
    DrawRectangle(0, 0, (int)w, (int)h, Fade(BLACK, 0.45f));

    float pw = ui.S(380), ph = ui.S(380);
    Rectangle panel = { (w - pw) / 2, (h - ph) / 2, pw, ph };
    ui.Panel(panel, "Paused");

    float bw = pw - ui.S(80), bh = ui.S(50), gap = ui.S(14);
    float x = panel.x + ui.S(40);
    float y = panel.y + ui.S(84);
    PauseAction action = PauseAction::None;

    if (ui.Button("pause.resume", { x, y, bw, bh }, "Resume")) action = PauseAction::Resume;
    y += bh + gap;
    if (ui.Button("pause.settings", { x, y, bw, bh }, "Settings")) action = PauseAction::Settings;
    y += bh + gap;
    if (ui.Button("pause.title", { x, y, bw, bh }, "Save & Return to Title"))
        action = PauseAction::SaveAndTitle;
    y += bh + gap;
    if (ui.Button("pause.quit", { x, y, bw, bh }, "Save & Quit to Desktop"))
        action = PauseAction::QuitDesktop;
    return action;
}

// ---- settings -------------------------------------------------------------

static const int FPS_CAPS[] = { 60, 120, 144, 240, 0 };
static const char* FPS_LABELS[] = { "60", "120", "144", "240", "Uncapped" };

static int FpsCapToIndex(int cap) {
    for (int i = 0; i < 5; i++) if (FPS_CAPS[i] == cap) return i;
    return 3;
}

SettingsAction DrawSettingsMenu(Ui& ui, MenuState& ms, Settings& s, InputSystem& input) {
    float w = ui.ScreenW(), h = ui.ScreenH();
    DrawRectangle(0, 0, (int)w, (int)h, Fade(BLACK, 0.45f));

    float pw = fminf(ui.S(760), w - ui.S(40)), ph = ui.S(700);
    Rectangle panel = { (w - pw) / 2, (h - ph) / 2, pw, ph };
    ui.Panel(panel, "Settings");

    // Tab row.
    const char* tabs[3] = { "Graphics", "Audio", "Controls" };
    float tw = ui.S(150), th = ui.S(40);
    float tx = panel.x + (pw - tw * 3 - ui.S(24)) / 2;
    for (int i = 0; i < 3; i++) {
        Rectangle r = { tx + i * (tw + ui.S(12)), panel.y + ui.S(76), tw, th };
        if (ms.settingsTab == i) {
            DrawRectangleRounded(r, 0.3f, 6, Fade(ui.theme.accent, 0.22f));
            DrawRectangleRoundedLinesEx(r, 0.3f, 6, ui.S(1.5f), ui.theme.accent);
            ui.LabelCentered({ r.x + tw / 2, r.y + th / 2 }, tabs[i], 22, ui.theme.accentGlow);
        } else if (ui.Button(TextFormat("settings.tab%d", i), r, tabs[i])) {
            ms.settingsTab = i;
            input.CancelCapture();
        }
    }

    float x = panel.x + ui.S(48);
    float rw = pw - ui.S(96);
    float y = panel.y + ui.S(140);
    float rh = ui.S(44), gap = ui.S(10);

    if (ms.settingsTab == 0) {
        ui.Toggle("gfx.fullscreen", { x, y, rw, rh }, "Fullscreen (borderless)", s.gfx.fullscreen);
        y += rh + gap;
        int fpsIdx = FpsCapToIndex(s.gfx.fpsCap);
        if (ui.Selector("gfx.fps", { x, y, rw, rh }, "FPS Cap", fpsIdx, FPS_LABELS, 5))
            s.gfx.fpsCap = FPS_CAPS[fpsIdx];
        y += rh + gap;
        ui.Slider("gfx.scale", { x, y, rw, rh }, s.gfx.renderScale, 0.5f, 1.25f,
                  "Render Scale", "%.2f");
        y += rh + gap;
        ui.Slider("gfx.fov", { x, y, rw, rh }, s.gfx.fovY, 60.0f, 100.0f,
                  "Field of View", "%.0f");
        y += rh + gap;
        ui.Slider("gfx.viewdist", { x, y, rw, rh }, s.gfx.viewDistance, 120.0f, 320.0f,
                  "View Distance", "%.0f m");
        y += rh + gap;
        ui.Toggle("gfx.bloom", { x, y, rw, rh }, "Bloom", s.gfx.bloom);
        y += rh + gap;
        ui.Slider("gfx.particles", { x, y, rw, rh }, s.gfx.particleDensity, 0.25f, 1.0f,
                  "Particle Density", "%.2f");
    } else if (ms.settingsTab == 1) {
        ui.Slider("aud.master", { x, y, rw, rh }, s.audio.master, 0.0f, 1.0f, "Master", "%.2f");
        y += rh + gap;
        ui.Slider("aud.sfx", { x, y, rw, rh }, s.audio.sfx, 0.0f, 1.0f, "Effects", "%.2f");
        y += rh + gap;
        ui.Slider("aud.ambience", { x, y, rw, rh }, s.audio.ambience, 0.0f, 1.0f, "Ambience", "%.2f");
        y += rh + gap;
        ui.Slider("aud.music", { x, y, rw, rh }, s.audio.music, 0.0f, 1.0f, "Music", "%.2f");
    } else {
        ui.Slider("ctl.sens", { x, y, rw, rh }, s.controls.mouseSensitivity, 0.03f, 0.30f,
                  "Mouse Sensitivity", "%.2f");
        y += rh + gap;
        ui.Toggle("ctl.invert", { x, y, rw, rh }, "Invert Y", s.controls.invertY);
        y += rh + gap * 1.5f;

        // Bind table: action name + two binding slots.
        float bindW = ui.S(120), bindH = ui.S(32);
        float rowH = bindH + ui.S(7);
        for (int i = 0; i < ACTION_COUNT; i++) {
            Action a = (Action)i;
            ui.Label({ x, y + ui.S(5) }, InputSystem::ActionLabel(a), 20, ui.theme.textDim);
            for (int slot = 0; slot < 2; slot++) {
                bool capturing = input.CaptureActive() &&
                                 input.CaptureAction() == a && input.CaptureSlot() == slot;
                int code = slot == 0 ? input.bindings[i].primary : input.bindings[i].secondary;
                Rectangle r = { x + rw - bindW * (2 - slot) - ui.S(slot == 0 ? 12.0f : 0.0f),
                                y, bindW, bindH };
                if (ui.BindButton(TextFormat("bind.%d.%d", i, slot), r,
                                  InputSystem::CodeLabel(code), capturing))
                    input.StartCapture(a, slot);
            }
            y += rowH;
        }
    }

    SettingsAction action = SettingsAction::None;
    float bw = ui.S(200), bh = ui.S(48);
    if (ui.Button("settings.back", { panel.x + (pw - bw) / 2, panel.y + ph - bh - ui.S(24),
                                     bw, bh }, "Back"))
        action = SettingsAction::Back;
    return action;
}
