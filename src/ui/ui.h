// Immediate-mode UI kit. Menus rebuild every frame from game state — no
// retained tree to desync. Per-widget animation (hover glow, press flash)
// lives in an id-keyed map inside the context. Widgets report interactions
// through return values; sounds are queued in `hoverEvents`/`clickEvents`
// for the game to consume (UI never touches the audio system directly).
#pragma once
#include "raylib.h"
#include <unordered_map>

struct UiTheme {
    Color panel = { 16, 22, 34, 232 };
    Color panelBorder = { 92, 108, 148, 120 };
    Color text = { 226, 232, 244, 255 };
    Color textDim = { 148, 158, 182, 255 };
    Color accent = { 235, 200, 120, 255 };      // ancient gold
    Color accentGlow = { 255, 224, 150, 255 };
    Color buttonBase = { 30, 40, 62, 220 };
    Color buttonHover = { 48, 62, 94, 235 };
    Color track = { 52, 62, 88, 255 };
};

class Ui {
public:
    void Init();
    void Shutdown();
    void Begin(float dt);        // call once per frame before any widget

    // Layout scale: UI is authored at 900px height and scales with the window.
    float S(float v) const { return v * scale; }
    float ScreenW() const { return (float)GetScreenWidth(); }
    float ScreenH() const { return (float)GetScreenHeight(); }

    void Panel(Rectangle r, const char* title);
    void Label(Vector2 pos, const char* text, float size, Color c);
    void LabelCentered(Vector2 center, const char* text, float size, Color c);
    bool Button(const char* id, Rectangle r, const char* label);
    bool Slider(const char* id, Rectangle r, float& value, float min, float max,
                const char* label, const char* valueFmt);
    bool Toggle(const char* id, Rectangle r, const char* label, bool& value);
    bool Selector(const char* id, Rectangle r, const char* label, int& index,
                  const char* const* options, int count);
    // Shows `bindingText`; returns true when clicked (game starts capture).
    bool BindButton(const char* id, Rectangle r, const char* bindingText, bool capturing);

    Font FontRef() const { return font; }
    void DrawTextC(const char* text, Vector2 pos, float size, Color c);
    float TextWidth(const char* text, float size) const;

    UiTheme theme;
    int hoverEvents = 0;         // consumed (zeroed) by game each frame
    int clickEvents = 0;

private:
    float Anim(unsigned long long id, float target, float rate);
    unsigned long long prevHot = 0, curHot = 0;   // hover-sound edge detection
    std::unordered_map<unsigned long long, float> anims;
    Font font = {};
    bool customFont = false;
    float scale = 1.0f;
    float dt = 1.0f / 60.0f;
    Vector2 mouse = {};
    bool mouseDown = false, mousePressed = false, mouseReleased = false;
    unsigned long long activeId = 0;     // widget being dragged
};
