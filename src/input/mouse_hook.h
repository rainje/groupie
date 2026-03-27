#pragma once

#include <windows.h>

class MouseHook {
public:
    static MouseHook& Instance();

    bool Init();
    void Shutdown();

private:
    MouseHook() = default;

    static LRESULT CALLBACK LowLevelProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK hHook_ = nullptr;
};
