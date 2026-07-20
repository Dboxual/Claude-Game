#include "ui/hud.h"
#include "core/mathx.h"
#include "ui/ui.h"
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

void Hud::Draw(Ui& ui, const DrawParams& p) {
    float w = ui.ScreenW(), h = ui.ScreenH();

    if (!p.menuOpen) {
        // Crosshair — FP only; TP aims by silhouette.
        if (p.firstPerson) {
            DrawCircleV({ w / 2, h / 2 }, ui.S(2.0f), Color{ 255, 255, 255, 190 });
            DrawCircleLinesV({ w / 2, h / 2 }, ui.S(5.5f), Color{ 255, 255, 255, 55 });
        }

        // Interact prompt: key chip + verb, gently pulsing.
        if (p.promptVerb) {
            float pulse = 0.85f + 0.15f * sinf(promptPhase * 4.0f);
            float chipS = ui.S(34);
            float textW = ui.TextWidth(p.promptVerb, ui.S(24));
            float total = chipS + ui.S(12) + textW;
            float x = (w - total) / 2, y = h * 0.72f;
            Rectangle chip = { x, y, chipS, chipS };
            DrawRectangleRounded(chip, 0.3f, 6, Color{ 20, 28, 44, 210 });
            DrawRectangleRoundedLinesEx(chip, 0.3f, 6, ui.S(1.5f),
                                        ColorMul(ui.theme.accent, pulse));
            ui.LabelCentered({ x + chipS / 2, y + chipS / 2 }, p.promptKey, 20,
                             ColorMul(ui.theme.accentGlow, pulse));
            ui.DrawTextC(p.promptVerb, { x + chipS + ui.S(12), y + chipS / 2 - ui.S(12) },
                         ui.S(24), Color{ 232, 238, 248, 235 });
        }

        // Anima counter, top right: diamond gem + count; pops on gain.
        {
            float pop = 1.0f + EaseOutCubic(animaPop) * 0.35f;
            float cx = w - ui.S(96), cy = ui.S(44);
            float g = ui.S(11) * pop;
            Color gem = ColorLerpF(Color{ 140, 245, 215, 255 }, WHITE, animaPop * 0.7f);
            DrawPoly({ cx, cy }, 4, g, 45.0f, gem);
            DrawPolyLinesEx({ cx, cy }, 4, g + ui.S(2.5f), 45.0f, ui.S(1.5f),
                            Fade(gem, 0.45f));
            ui.DrawTextC(TextFormat("%d", p.anima), { cx + ui.S(24), cy - ui.S(13) },
                         ui.S(26) * (1.0f + animaPop * 0.12f), ui.theme.text);
        }

        // Blessing tally under it, small.
        if (p.blessings > 0)
            ui.DrawTextC(TextFormat("blessings %d", p.blessings),
                         { w - ui.S(120), ui.S(72) }, ui.S(16), ui.theme.textDim);

        // Center banner.
        if (bannerTimer > 0.0f) {
            float t = bannerTimer;
            float alpha = Saturate(t / 0.5f) * Saturate((3.2f - t) / 0.35f);
            float rise = EaseOutCubic(Saturate((3.2f - t) / 0.6f)) * ui.S(18);
            ui.LabelCentered({ w / 2, h * 0.30f - rise }, bannerText, 34,
                             Fade(ui.theme.accentGlow, alpha));
        }
    }

    // Screen fade-in from black (world reveal).
    if (fadeTimer > 0.0f && fadeLen > 0.0f) {
        float a = SmoothStep01(fadeTimer / fadeLen);
        DrawRectangle(0, 0, (int)w, (int)h, Fade(BLACK, a));
    }
}
