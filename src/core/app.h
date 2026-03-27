#pragma once

#include <windows.h>
#include <shellapi.h>
#include "core/window_monitor.h"
#include "input/mouse_hook.h"
#include "input/keyboard_hook.h"
#include "ui/snap_indicator.h"
#include "ui/renderer.h"
#include "util/taskbar.h"

#define WM_TRAYICON (WM_USER + 1)
#define IDI_APPICON 101

class App {
public:
    static App& Instance();

    bool Init(HINSTANCE hInstance);
    void Shutdown();
    int Run();

    HINSTANCE GetHInstance() const { return hInstance_; }
    HWND GetMessageWindow() const { return hMsgWnd_; }

private:
    App() = default;
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool CreateMessageWindow();
    bool CreateTrayIcon();
    void RemoveTrayIcon();

    HINSTANCE hInstance_ = nullptr;
    HWND hMsgWnd_ = nullptr;
    NOTIFYICONDATAW nid_{};
    bool running_ = false;

    WindowMonitor windowMonitor_;
    MouseHook& mouseHook_ = MouseHook::Instance();
    Renderer& renderer_ = Renderer::Instance();
    KeyboardHook& keyboardHook_ = KeyboardHook::Instance();
    SnapIndicator& snapIndicator_ = SnapIndicator::Instance();
};
