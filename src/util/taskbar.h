#pragma once

#include <windows.h>
#include <shobjidl_core.h>
#include "util/com_ptr.h"

class Taskbar {
public:
    static Taskbar& Instance();

    bool Init();
    void Shutdown();

    // Hide a window's taskbar button
    void HideButton(HWND hwnd);
    // Show/restore a window's taskbar button
    void ShowButton(HWND hwnd);

private:
    Taskbar() = default;
    ComPtr<ITaskbarList> taskbar_;
};
