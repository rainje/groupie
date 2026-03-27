#pragma once

#include <windows.h>

// Hotkey IDs
#define HOTKEY_NEXT_TAB     1
#define HOTKEY_PREV_TAB     2
#define HOTKEY_CLOSE_TAB    3

class KeyboardHook {
public:
    static KeyboardHook& Instance();

    bool Init(HWND msgWnd);
    void Shutdown(HWND msgWnd);
    void OnHotkey(int id);

private:
    KeyboardHook() = default;
};
