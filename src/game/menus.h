// Menu screens, drawn with the immediate-mode UI kit. Pure presentation +
// intent: they mutate Settings/bindings directly and report navigation as
// return values; Game applies side effects (window mode, volumes, saves).
#pragma once

class Ui;
class InputSystem;
struct Settings;

enum class TitleAction { None, Continue, NewJourney, Settings, Quit };
enum class PauseAction { None, Resume, Settings, SaveAndTitle, QuitDesktop };
enum class SettingsAction { None, Back };

struct MenuState {
    int settingsTab = 0;      // 0 graphics, 1 audio, 2 controls
};

TitleAction DrawTitleMenu(Ui& ui, bool hasSave, float time);
PauseAction DrawPauseMenu(Ui& ui);
SettingsAction DrawSettingsMenu(Ui& ui, MenuState& ms, Settings& s, InputSystem& input);
