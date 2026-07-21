#include "ui/ui.h"
#include "core/log.h"
#include "core/mathx.h"
#include <cstring>

// FNV-1a — stable widget ids from string labels.
static unsigned long long HashId(const char* s) {
    unsigned long long h = 1469598103934665603ULL;
    while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211ULL; }
    return h;
}

void Ui::Init() {
    // A real TTF beats raylib's pixel font for menu text; fall back cleanly.
    const char* candidates[] = {
#if defined(_WIN32)
        "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/SFNS.ttf", "/System/Library/Fonts/Helvetica.ttc",
#endif
    };
    for (const char* path : candidates) {
        if (FileExists(path)) {
            font = LoadFontEx(path, 64, nullptr, 0);
            if (font.texture.id != 0) {
                customFont = true;
                SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
                LOG_INFO("UI font: %s", path);
                break;
            }
        }
    }
    if (!customFont) font = GetFontDefault();
}

void Ui::Shutdown() {
    if (customFont) UnloadFont(font);
}

void Ui::Begin(float frameDt) {
    dt = frameDt;
    scale = GetScreenHeight() / 900.0f;
    mouse = GetMousePosition();
    mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    mouseReleased = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    if (!mouseDown) activeId = 0;
    prevHot = curHot;
    curHot = 0;
}

float Ui::Anim(unsigned long long id, float target, float rate) {
    float& v = anims[id];               // default-inserts 0
    v = ExpDecay(v, target, rate, dt);
    return v;
}

void Ui::DrawTextC(const char* text, Vector2 pos, float size, Color c) {
    DrawTextEx(font, text, pos, size, size * 0.03f, c);
}

float Ui::TextWidth(const char* text, float size) const {
    return MeasureTextEx(font, text, size, size * 0.03f).x;
}

void Ui::Label(Vector2 pos, const char* text, float size, Color c) {
    DrawTextC(text, pos, S(size), c);
}

void Ui::LabelCentered(Vector2 center, const char* text, float size, Color c) {
    float w = TextWidth(text, S(size));
    DrawTextC(text, { center.x - w / 2, center.y - S(size) / 2 }, S(size), c);
}

void Ui::Panel(Rectangle r, const char* title) {
    // Layered ink-glass with small archive-like corner seals. The broad dark
    // field keeps world color visible around it while the warm/cool edge pair
    // gives all menus one recognizable Zion material.
    Rectangle shadow = { r.x + S(7), r.y + S(9), r.width, r.height };
    DrawRectangleRounded(shadow, 0.045f, 8, Color{ 3, 7, 13, 150 });
    DrawRectangleRounded(r, 0.045f, 8, theme.panel);
    DrawRectangleRoundedLinesEx(r, 0.045f, 8, S(2.0f), Fade(theme.accent, 0.34f));
    Rectangle inner = { r.x + S(5), r.y + S(5), r.width - S(10), r.height - S(10) };
    DrawRectangleRoundedLinesEx(inner, 0.04f, 8, S(1.0f), Fade(theme.panelBorder, 0.55f));

    float d = S(4.0f);
    Vector2 corners[4] = {
        { r.x + S(12), r.y + S(12) }, { r.x + r.width - S(12), r.y + S(12) },
        { r.x + S(12), r.y + r.height - S(12) },
        { r.x + r.width - S(12), r.y + r.height - S(12) }
    };
    for (Vector2 p : corners) DrawPoly(p, 4, d, 45.0f, Fade(theme.accent, 0.58f));

    if (title && title[0]) {
        LabelCentered({ r.x + r.width / 2, r.y + S(34) }, title, 30, theme.accent);
        float lineW = r.width * 0.56f;
        DrawRectangle((int)(r.x + (r.width - lineW) / 2), (int)(r.y + S(58)),
                      (int)lineW, (int)S(1), Fade(theme.accent, 0.55f));
        DrawCircleV({ r.x + r.width / 2, r.y + S(58.5f) }, S(3), theme.accent);
    }
}

bool Ui::Button(const char* id, Rectangle r, const char* label) {
    unsigned long long h = HashId(id);
    bool hover = CheckCollisionPointRec(mouse, r);
    float a = Anim(h, hover ? 1.0f : 0.0f, 14.0f);
    if (hover) { curHot = h; if (prevHot != h) hoverEvents++; }

    bool clicked = hover && mousePressed;
    if (clicked) clickEvents++;

    Rectangle shadow = { r.x + S(3), r.y + S(4), r.width, r.height };
    DrawRectangleRounded(shadow, 0.22f, 8, Color{ 2, 7, 12, 120 });
    Color bg = ColorLerpF(theme.buttonBase, theme.buttonHover, a);
    DrawRectangleRounded(r, 0.28f, 8, bg);
    DrawRectangleRoundedLinesEx(r, 0.28f, 8, S(1.5f),
                                ColorLerpF(theme.panelBorder, theme.accent, a));
    // Hover: label lifts toward gold and the button grows a soft underline.
    Color txt = ColorLerpF(theme.text, theme.accentGlow, a * 0.8f);
    LabelCentered({ r.x + r.width / 2, r.y + r.height / 2 }, label, 24, txt);
    if (a > 0.02f) {
        float lineW = r.width * 0.72f * EaseOutCubic(a);
        DrawRectangle((int)(r.x + (r.width - lineW) / 2), (int)(r.y + r.height - S(6)),
                      (int)lineW, (int)S(2), Fade(theme.accent, 0.5f * a));
        DrawPoly({ r.x + S(15), r.y + r.height / 2 }, 4, S(3.5f) * a,
                 45.0f, Fade(theme.magic, a));
        DrawPoly({ r.x + r.width - S(15), r.y + r.height / 2 }, 4,
                 S(3.5f) * a, 45.0f, Fade(theme.magic, a));
    }
    return clicked;
}

bool Ui::Slider(const char* id, Rectangle r, float& value, float mn, float mx,
                const char* label, const char* valueFmt) {
    unsigned long long h = HashId(id);
    Rectangle track = { r.x + r.width * 0.42f, r.y + r.height / 2 - S(3),
                        r.width * 0.44f, S(6) };
    Rectangle grab = { track.x - S(8), r.y, track.width + S(16), r.height };
    bool hover = CheckCollisionPointRec(mouse, grab);
    if (hover && mousePressed) { activeId = h; clickEvents++; }
    bool dragging = (activeId == h);
    float a = Anim(h, (hover || dragging) ? 1.0f : 0.0f, 14.0f);

    bool changed = false;
    if (dragging) {
        float t = Saturate((mouse.x - track.x) / track.width);
        float nv = mn + (mx - mn) * t;
        if (nv != value) { value = nv; changed = true; }
    }

    Label({ r.x, r.y + r.height / 2 - S(11) }, label, 22, theme.textDim);
    float t = Saturate((value - mn) / (mx - mn));
    DrawRectangleRounded(track, 1.0f, 4, theme.track);
    Rectangle fill = { track.x, track.y, track.width * t, track.height };
    DrawRectangleRounded(fill, 1.0f, 4, ColorLerpF(theme.accent, theme.accentGlow, a));
    DrawCircleV({ track.x + track.width * t, track.y + S(3) }, S(9 + 2 * a),
                ColorLerpF(theme.text, theme.accentGlow, a));

    char buf[48];
    snprintf(buf, sizeof(buf), valueFmt, value);
    Label({ track.x + track.width + S(18), r.y + r.height / 2 - S(11) }, buf, 22, theme.text);
    return changed;
}

bool Ui::Toggle(const char* id, Rectangle r, const char* label, bool& value) {
    unsigned long long h = HashId(id);
    bool hover = CheckCollisionPointRec(mouse, r);
    float a = Anim(h, hover ? 1.0f : 0.0f, 14.0f);
    bool clicked = hover && mousePressed;
    if (clicked) { value = !value; clickEvents++; }

    Label({ r.x, r.y + r.height / 2 - S(11) }, label, 22, theme.textDim);
    // Pill switch on the right.
    Rectangle pill = { r.x + r.width - S(64), r.y + r.height / 2 - S(13), S(52), S(26) };
    float on = Anim(h ^ 0x9E3779B97F4A7C15ULL, value ? 1.0f : 0.0f, 16.0f);
    DrawRectangleRounded(pill, 1.0f, 8,
                         ColorLerpF(theme.track, ColorMul(theme.accent, 0.8f), on));
    DrawRectangleRoundedLinesEx(pill, 1.0f, 8, S(1.5f),
                                ColorLerpF(theme.panelBorder, theme.accent, fmaxf(a, on * 0.6f)));
    DrawCircleV({ pill.x + S(13) + on * S(26), pill.y + S(13) }, S(9),
                ColorLerpF(theme.textDim, WHITE, on));
    return clicked;
}

bool Ui::Selector(const char* id, Rectangle r, const char* label, int& index,
                  const char* const* options, int count) {
    Label({ r.x, r.y + r.height / 2 - S(11) }, label, 22, theme.textDim);

    Rectangle box = { r.x + r.width * 0.42f, r.y, r.width * 0.58f, r.height };
    float bw = S(34);
    Rectangle left = { box.x, box.y, bw, box.height };
    Rectangle right = { box.x + box.width - bw, box.y, bw, box.height };

    char lid[128], rid[128];
    snprintf(lid, sizeof(lid), "%s<", id);
    snprintf(rid, sizeof(rid), "%s>", id);
    bool changed = false;

    unsigned long long hl = HashId(lid), hr = HashId(rid);
    bool hovL = CheckCollisionPointRec(mouse, left);
    bool hovR = CheckCollisionPointRec(mouse, right);
    float al = Anim(hl, hovL ? 1.0f : 0.0f, 14.0f);
    float ar = Anim(hr, hovR ? 1.0f : 0.0f, 14.0f);
    if (hovL && mousePressed) { index = (index + count - 1) % count; changed = true; clickEvents++; }
    if (hovR && mousePressed) { index = (index + 1) % count; changed = true; clickEvents++; }

    LabelCentered({ left.x + bw / 2, box.y + box.height / 2 }, "<", 24,
                  ColorLerpF(theme.textDim, theme.accentGlow, al));
    LabelCentered({ right.x + bw / 2, box.y + box.height / 2 }, ">", 24,
                  ColorLerpF(theme.textDim, theme.accentGlow, ar));
    LabelCentered({ box.x + box.width / 2, box.y + box.height / 2 },
                  options[index], 22, theme.text);
    return changed;
}

bool Ui::BindButton(const char* id, Rectangle r, const char* bindingText, bool capturing) {
    unsigned long long h = HashId(id);
    bool hover = CheckCollisionPointRec(mouse, r);
    float a = Anim(h, (hover || capturing) ? 1.0f : 0.0f, 14.0f);
    bool clicked = hover && mousePressed && !capturing;
    if (clicked) clickEvents++;

    Color bg = ColorLerpF(theme.buttonBase, theme.buttonHover, a);
    if (capturing) bg = ColorLerpF(bg, ColorMul(theme.accent, 0.5f), 0.5f);
    DrawRectangleRounded(r, 0.3f, 6, bg);
    DrawRectangleRoundedLinesEx(r, 0.3f, 6, S(1.2f),
                                ColorLerpF(theme.panelBorder, theme.accent, a));
    LabelCentered({ r.x + r.width / 2, r.y + r.height / 2 },
                  capturing ? "press key..." : bindingText, 19,
                  capturing ? theme.accentGlow : theme.text);
    return clicked;
}
