#include "game/menus.h"
#include "core/mathx.h"
#include "input/input.h"
#include "settings/settings.h"
#include "ui/ui.h"
#include <cmath>

// ---- title ----------------------------------------------------------------

TitleAction DrawTitleMenu(Ui& ui, bool hasSave, const char* zoneName, float time) {
    float w = ui.ScreenW(), h = ui.ScreenH();

    // A dark wayfinder's-archive slab anchors the controls while the living
    // world remains the hero on the right. The gradient is deliberately broad
    // enough to preserve text contrast over every biome/title-camera angle.
    float pw = fmaxf(ui.S(420), fminf(w * 0.42f, ui.S(540)));
    DrawRectangleGradientH(0, 0, (int)(pw + ui.S(170)), (int)h,
                           Color{ 6, 12, 23, 250 }, Color{ 6, 12, 23, 0 });
    DrawRectangle(0, 0, (int)pw, (int)h, Color{ 7, 15, 27, 228 });
    DrawRectangle((int)(pw - ui.S(2)), 0, (int)ui.S(2), (int)h,
                  Fade(ui.theme.magic, 0.42f));

    float x = ui.S(58);
    float contentW = pw - ui.S(110);
    float top = h * 0.105f;

    // Small orbiting rune behind the chapter label gives the otherwise flat
    // menu a quiet magical pulse without competing with the world scene.
    float pulse = 0.9f + 0.1f * sinf(time * 1.4f);
    Vector2 seal = { x + ui.S(7), top + ui.S(4) };
    DrawCircleLinesV(seal, ui.S(12 + pulse * 2), Fade(ui.theme.magic, 0.28f));
    DrawPolyLinesEx(seal, 4, ui.S(7), 45.0f + time * 8.0f, ui.S(1),
                    Fade(ui.theme.accent, 0.55f));
    ui.Label({ x + ui.S(28), top - ui.S(7) }, "THE FIRST HORIZON", 14,
             ui.theme.magic);

    ui.DrawTextC("ZION", { x, top + ui.S(30) }, ui.S(76),
                 ColorMul(ui.theme.accentGlow, pulse));
    ui.Label({ x, top + ui.S(111) }, "THE VEILBOUND AGE", 18, ui.theme.text);
    DrawRectangle((int)x, (int)(top + ui.S(142)), (int)ui.S(76), (int)ui.S(2),
                  ui.theme.accent);
    ui.Label({ x, top + ui.S(160) }, "Beyond the old gods, the land remembers.",
             17, ui.theme.textDim);
    ui.Label({ x, top + ui.S(183) }, "Walk softly. Build boldly. Leave a legend.",
             17, ui.theme.textDim);

    float bw = contentW, bh = ui.S(50), gap = ui.S(13);
    float y = fmaxf(top + ui.S(230), h * 0.46f);
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
    ui.Label({ x, h - ui.S(54) }, "FOUNDATION BUILD 0.1", 13,
             Fade(ui.theme.magic, 0.72f));
    ui.Label({ x, h - ui.S(34) }, "NATIVE DESKTOP  |  FP + TP", 12,
             Fade(ui.theme.textDim, 0.72f));

    const char* view = TextFormat("LIVE VIEW  |  %s", zoneName);
    float vw = ui.TextWidth(view, ui.S(12));
    ui.DrawTextC(view, { w - vw - ui.S(20), h - ui.S(30) }, ui.S(12),
                 Fade(ui.theme.text, 0.68f));
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
static const int RES_W[] = { 960, 1024, 1152, 1200, 1280, 1600, 1920, 1920, 2560 };
static const int RES_H[] = { 540, 576, 648, 700, 720, 900, 1080, 1200, 1440 };
static const char* RES_LABELS[] = {
    "960 x 540", "1024 x 576", "1152 x 648", "1200 x 700", "1280 x 720",
    "1600 x 900", "1920 x 1080", "1920 x 1200", "2560 x 1440"
};
static constexpr int RES_COUNT = sizeof(RES_W) / sizeof(RES_W[0]);

static int FpsCapToIndex(int cap) {
    for (int i = 0; i < 5; i++) if (FPS_CAPS[i] == cap) return i;
    return 3;
}

static int ResolutionToIndex(int width, int height) {
    for (int i = 0; i < RES_COUNT; i++)
        if (RES_W[i] == width && RES_H[i] == height) return i;
    return 0;
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
        ui.Toggle("gfx.fullscreen", { x, y, rw, rh }, "Borderless Fullscreen", s.gfx.fullscreen);
        y += rh + gap;
        int resIdx = ResolutionToIndex(s.gfx.width, s.gfx.height);
        const char* resolutionLabel =
#if defined(__APPLE__)
            "Window Resolution (restart required)";
#else
            "Window Resolution";
#endif
        if (ui.Selector("gfx.resolution", { x, y, rw, rh }, resolutionLabel,
                        resIdx, RES_LABELS, RES_COUNT)) {
            s.gfx.width = RES_W[resIdx];
            s.gfx.height = RES_H[resIdx];
        }
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
