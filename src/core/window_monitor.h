#pragma once

#include <windows.h>

class WindowMonitor {
public:
    bool Init();
    void Shutdown();

private:
    static void CALLBACK WinEventCallback(
        HWINEVENTHOOK hook, DWORD event, HWND hwnd,
        LONG idObject, LONG idChild, DWORD eventThread, DWORD eventTime
    );

    static constexpr int kHookCount = 4;
    HWINEVENTHOOK hooks_[kHookCount]{};
};
