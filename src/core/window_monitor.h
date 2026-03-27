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

    HWINEVENTHOOK hHook_ = nullptr;
};
