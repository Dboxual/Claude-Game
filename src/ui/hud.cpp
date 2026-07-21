#include "ui/hud.h"
#include "core/mathx.h"
#include "ui/ui.h"
#include <cmath>
#include <cstring>

void Hud::Update(float frameDt) {
    dt = frameDt;
    animaPop = ExpDecay(animaPop, 0.0f, 6.0f, dt);
    bannerTimer = fmaxf(bannerTimer - dt, 0.0f);
    fadeTimer = fmaxf(fadeTimer - dt, 0.0f);
    promptPhase += dt;
}

void Hud::OnAnimaGained() { animaPop = 1.0f; }

void Hud::OnBanner(const char* text) {
    snprintf(bannerText, sizeof(bannerText), "%s", text);
    bannerTimer = 3.2f;
}

void Hud::StartFadeIn(float seconds) { fadeTimer = fadeLen = seconds; }

static const char* TierRoman(int tier) {
    static const char* names[] = { "I", "I", "II", "III", "IV", "V", "VI", "VII", "VIII" };
    return names[tier >= 1 && tier <= 8 ? tier : 0];
}

static const char* RiskName(int tier) {
    if (tier <= 1) return "SANCTUARY";
    if (tier == 2) return "GUARDED";
    if (tier == 3) return "CONTESTED";
    return "LAWLESS";
}

static Color RiskColor(const Ui& ui, int tier) {
    if (tier <= 1) return Color{ 123, 190, 108, 255 };
    if (tier == 2) return ui.theme.accentGlow;
    if (tier == 3) return ui.theme.danger;
    return Color{ 180, 120, 226, 255 };
}

static void DrawHudPlate(Ui& ui, Rectangle r) {
    Rectangle shadow = { r.x + ui.S(4), r.y + ui.S(5), r.width, r.height };
    DrawRectangleRounded(shadow, 0.12f, 7, Color{ 2, 6, 12, 125 });
    DrawRectangleRounded(r, 0.12f, 7, Color{ 8, 19, 30, 222 });
    DrawRectangleRoundedLinesEx(r, 0.12f, 7, ui.S(1.2f), Fade(ui.theme.magic, 0.42f));
    DrawRectangle((int)(r.x + ui.S(10)), (int)(r.y + r.height - ui.S(3)),
                  (int)(r.width - ui.S(20)), (int)ui.S(1), Fade(ui.theme.accent, 0.38f));
}

static void DrawCompass(Ui& ui, float yaw) {
    float w = ui.S(290), h = ui.S(47);
    Rectangle r = { (ui.ScreenW() - w) / 2, ui.S(17), w, h };
    DrawHudPlate(ui, r);

    float heading = yaw * RAD2DEG;
    while (heading < 0.0f) heading += 360.0f;
    while (heading >= 360.0f) heading -= 360.0f;
    float base = floorf(heading / 15.0f) * 15.0f;
    float spacing = ui.S(28);
    float center = r.x + r.width / 2;
    const char* cardinals[4] = { "N", "E", "S", "W" };

    for (int i = -6; i <= 6; i++) {
        float deg = base + i * 15.0f;
        float x = center + (deg - heading) / 15.0f * spacing;
        if (x < r.x + ui.S(14) || x > r.x + r.width - ui.S(14)) continue;
        int wrapped = ((int)lroundf(deg) % 360 + 360) % 360;
        bool cardinal = wrapped % 90 == 0;
        float lineH = ui.S(cardinal ? 8.0f : (wrapped % 45 == 0 ? 6.0f : 4.0f));
        DrawRectangle((int)x, (int)(r.y + r.height - ui.S(12) - lineH),
                      (int)ui.S(1), (int)lineH,
                      cardinal ? ui.theme.accent : Fade(ui.theme.textDim, 0.65f));
        if (cardinal)
            ui.LabelCentered({ x, r.y + ui.S(15) }, cardinals[wrapped / 90], 15,
                             wrapped == 0 ? ui.theme.accentGlow : ui.theme.text);
    }
    DrawPoly({ center, r.y + r.height - ui.S(5) }, 3, ui.S(5), 180.0f,
             ui.theme.magic);
}

static void DrawCombatTemplate(Ui& ui) {
    // Presentation scaffold only. Combat code will supply real vitals,
    // cooldowns, equipment-derived abilities, and input prompts later.
    float w = ui.S(448), h = ui.S(104);
    float x = (ui.ScreenW() - w) * 0.5f;
    float y = ui.ScreenH() - h - ui.S(18);
    Rectangle plate = { x, y, w, h };
    DrawHudPlate(ui, plate);

    Rectangle health = { x + ui.S(18), y + ui.S(14), ui.S(194), ui.S(9) };
    Rectangle focus = { x + w - ui.S(212), y + ui.S(14), ui.S(194), ui.S(9) };
    DrawRectangleRounded(health, 1.0f, 4, ui.theme.track);
    DrawRectangleRounded(focus, 1.0f, 4, ui.theme.track);
    DrawRectangleRounded(health, 1.0f, 4, Color{ 176, 69, 67, 255 });
    DrawRectangleRounded(focus, 1.0f, 4, Color{ 62, 126, 170, 255 });
    ui.DrawTextC("100", { health.x, health.y - ui.S(1) }, ui.S(9), ui.theme.text);
    float focusText = ui.TextWidth("100", ui.S(9));
    ui.DrawTextC("100", { focus.x + focus.width - focusText, focus.y - ui.S(1) },
                 ui.S(9), ui.theme.text);

    const char* keys[6] = { "LMB", "1", "2", "3", "4", "Q" };
    float slot = ui.S(54), gap = ui.S(10);
    float total = slot * 6 + gap * 5;
    float sx = x + (w - total) * 0.5f;
    for (int i = 0; i < 6; i++) {
        Rectangle r = { sx + i * (slot + gap), y + ui.S(36), slot, slot };
        DrawRectangleRounded(r, 0.16f, 5, Color{ 16, 30, 44, 245 });
        DrawRectangleRoundedLinesEx(r, 0.16f, 5, ui.S(1.2f),
                                    Fade(i == 0 ? ui.theme.accent : ui.theme.magic,
                                         i == 0 ? 0.68f : 0.34f));
        DrawPoly({ r.x + r.width / 2, r.y + r.height / 2 }, 4, ui.S(9), 45.0f,
                 Fade(i == 0 ? ui.theme.accent : ui.theme.magic, 0.13f));
        ui.LabelCentered({ r.x + r.width - ui.S(9), r.y + ui.S(9) }, keys[i],
                         i == 0 ? 9 : 11, ui.theme.textDim);
    }
}

static void DrawDevPanel(Ui& ui, const Hud::DrawParams& p) {
    if (!p.devMode || !p.devOverlay) return;
    Rectangle r = { ui.ScreenW() - ui.S(330), ui.S(104), ui.S(308), ui.S(154) };
    DrawHudPlate(ui, r);
    ui.Label({ r.x + ui.S(15), r.y + ui.S(11) }, "DEV CREATIVE - OPT IN", 14,
             ui.theme.accentGlow);
    ui.DrawTextC(TextFormat("F4 FLY [%s]   F5 TURBO [%s]",
                            p.devFly ? "ON" : "OFF", p.devTurbo ? "ON" : "OFF"),
                 { r.x + ui.S(15), r.y + ui.S(41) }, ui.S(12), ui.theme.text);
    ui.DrawTextC("F6 REWARD   F7 NEXT ZONE   F8 SPAWN",
                 { r.x + ui.S(15), r.y + ui.S(67) }, ui.S(11), ui.theme.textDim);
    ui.DrawTextC("F9 +3 HOURS   F1 HIDE PANEL",
                 { r.x + ui.S(15), r.y + ui.S(91) }, ui.S(11), ui.theme.textDim);
    ui.DrawTextC("FLY: SPACE UP / CTRL DOWN",
                 { r.x + ui.S(15), r.y + ui.S(118) }, ui.S(11), ui.theme.magic);
}

void Hud::Draw(Ui& ui, const DrawParams& p) {
    float w = ui.ScreenW(), h = ui.ScreenH();

    if (!p.menuOpen) {
        // FP crosshair: tiny wayfinder diamond with broken corner marks. It is
        // visible over bright sky without becoming a shooter-style reticle.
        if (p.firstPerson) {
            Vector2 c = { w / 2, h / 2 };
            DrawPoly(c, 4, ui.S(2.8f), 45.0f, Fade(ui.theme.text, 0.9f));
            DrawPolyLinesEx(c, 4, ui.S(7.0f), 45.0f, ui.S(1.0f),
                            Fade(ui.theme.magic, 0.45f));
        }

        // Stable navigation anchors.
        {
            Rectangle zone = { ui.S(22), ui.S(19), ui.S(270), ui.S(66) };
            DrawHudPlate(ui, zone);
            ui.Label({ zone.x + ui.S(17), zone.y + ui.S(10) }, p.zoneName, 20,
                     ui.theme.accentGlow);
            Color risk = RiskColor(ui, p.zoneTier);
            DrawPoly({ zone.x + ui.S(20), zone.y + ui.S(47) }, 4, ui.S(4), 45.0f, risk);
            ui.DrawTextC(TextFormat("TIER %s  |  %s", TierRoman(p.zoneTier), RiskName(p.zoneTier)),
                         { zone.x + ui.S(32), zone.y + ui.S(37) }, ui.S(13),
                         Fade(risk, 0.92f));
        }
        DrawCompass(ui, p.cameraYaw);
        ui.LabelCentered({ w / 2, ui.S(74) }, p.clockText, 11,
                         Fade(ui.theme.text, 0.72f));

        // Interact prompt: key chip + verb, gently pulsing.
        if (p.promptVerb) {
            float pulse = 0.85f + 0.15f * sinf(promptPhase * 4.0f);
            float chipS = ui.S(34);
            float textW = ui.TextWidth(p.promptVerb, ui.S(24));
            float total = chipS + ui.S(12) + textW;
            float pad = ui.S(14);
            float x = (w - total) / 2, y = h * 0.70f;
            Rectangle back = { x - pad, y - ui.S(8), total + pad * 2,
                               chipS + ui.S(16) };
            DrawHudPlate(ui, back);
            Rectangle chip = { x, y, chipS, chipS };
            DrawRectangleRounded(chip, 0.3f, 6, ui.theme.panelRaised);
            DrawRectangleRoundedLinesEx(chip, 0.3f, 6, ui.S(1.5f),
                                        ColorMul(ui.theme.magic, pulse));
            ui.LabelCentered({ x + chipS / 2, y + chipS / 2 }, p.promptKey, 19,
                             ColorMul(ui.theme.magic, pulse));
            ui.DrawTextC(p.promptVerb, { x + chipS + ui.S(12), y + chipS / 2 - ui.S(12) },
                         ui.S(22), ui.theme.text);
        }

        // Anima / mastery plate. The world-space wisps and this progress bar
        // are two ends of the same reward journey.
        {
            float pop = 1.0f + EaseOutCubic(animaPop) * 0.35f;
            Rectangle plate = { w - ui.S(242), ui.S(19), ui.S(220), ui.S(66) };
            DrawHudPlate(ui, plate);
            float cx = plate.x + ui.S(25), cy = plate.y + ui.S(26);
            float g = ui.S(10) * pop;
            Color gem = ColorLerpF(Color{ 140, 245, 215, 255 }, WHITE, animaPop * 0.7f);
            DrawPoly({ cx, cy }, 4, g + ui.S(7), 45.0f, Fade(gem, 0.09f));
            DrawPoly({ cx, cy }, 4, g, 45.0f, gem);
            DrawPolyLinesEx({ cx, cy }, 4, g + ui.S(2.5f), 45.0f, ui.S(1.5f),
                            Fade(gem, 0.45f));
            ui.DrawTextC(TextFormat("ANIMA  %d", p.anima),
                         { cx + ui.S(23), plate.y + ui.S(9) },
                         ui.S(18) * (1.0f + animaPop * 0.10f), ui.theme.text);

            int progress = p.anima % 100;
            ui.DrawTextC(TextFormat("THE LEDGER  %d / 100", progress),
                         { cx + ui.S(23), plate.y + ui.S(32) }, ui.S(11),
                         ui.theme.textDim);
            Rectangle track = { cx + ui.S(23), plate.y + ui.S(51), ui.S(150), ui.S(4) };
            DrawRectangleRounded(track, 1.0f, 4, ui.theme.track);
            Rectangle fill = track;
            fill.width *= progress / 100.0f;
            if (fill.width > 0.0f) DrawRectangleRounded(fill, 1.0f, 4, ui.theme.magic);
        }

        // Blessing tally under the reward plate.
        if (p.blessings > 0)
            ui.DrawTextC(TextFormat("blessings %d", p.blessings),
                         { w - ui.S(126), ui.S(91) }, ui.S(13), ui.theme.textDim);

        if (p.showControlHints) {
            const char* hint = "WASD MOVE   SHIFT SPRINT   SPACE JUMP   E INTERACT   V VIEW";
            float tw = ui.TextWidth(hint, ui.S(12));
            Rectangle card = { ui.S(22), h - ui.S(67), tw + ui.S(28), ui.S(43) };
            DrawHudPlate(ui, card);
            ui.DrawTextC(hint, { card.x + ui.S(14), card.y + ui.S(15) }, ui.S(12),
                         Fade(ui.theme.text, 0.86f));
        }

        DrawCombatTemplate(ui);
        DrawDevPanel(ui, p);

        // Center banner.
        if (bannerTimer > 0.0f) {
            float t = bannerTimer;
            float alpha = Saturate(t / 0.5f) * Saturate((3.2f - t) / 0.35f);
            float rise = EaseOutCubic(Saturate((3.2f - t) / 0.6f)) * ui.S(18);
            Vector2 center = { w / 2, h * 0.28f - rise };
            float textW = ui.TextWidth(bannerText, ui.S(29));
            float line = fminf(ui.S(150), (w - textW) * 0.25f);
            DrawRectangle((int)(center.x - textW / 2 - line - ui.S(18)), (int)center.y,
                          (int)line, (int)ui.S(1), Fade(ui.theme.accent, alpha * 0.65f));
            DrawRectangle((int)(center.x + textW / 2 + ui.S(18)), (int)center.y,
                          (int)line, (int)ui.S(1), Fade(ui.theme.accent, alpha * 0.65f));
            DrawPoly({ center.x - textW / 2 - ui.S(12), center.y }, 4, ui.S(4), 45.0f,
                     Fade(ui.theme.magic, alpha));
            DrawPoly({ center.x + textW / 2 + ui.S(12), center.y }, 4, ui.S(4), 45.0f,
                     Fade(ui.theme.magic, alpha));
            ui.LabelCentered(center, bannerText, 29,
                             Fade(ui.theme.accentGlow, alpha));
        }
    }

    // Screen fade-in from black (world reveal).
    if (fadeTimer > 0.0f && fadeLen > 0.0f) {
        float a = SmoothStep01(fadeTimer / fadeLen);
        DrawRectangle(0, 0, (int)w, (int)h, Fade(BLACK, a));
    }
}
