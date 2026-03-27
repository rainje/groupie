#include "core/app.h"
#include "core/group_manager.h"
#include "core/drag_detector.h"
#include "ui/tab_bar_window.h"
#include "config/settings.h"
#include "util/log.h"

static const wchar_t* kMsgWndClass = L"GroupieMsgWnd";

App& App::Instance() {
    static App instance;
    return instance;
}

bool App::Init(HINSTANCE hInstance) {
    hInstance_ = hInstance;

    // Load settings
    Settings::Instance().Load();

    // Init taskbar manager
    Taskbar::Instance().Init();

    // Init renderer (D2D + DWrite)
    if (!renderer_.Init()) {
        LOG_INFO(L"Failed to initialize renderer");
        return false;
    }

    // Register tab bar window class + snap indicator
    TabBarWindow::RegisterClass(hInstance);
    snapIndicator_.Init(hInstance);

    // Create message window + tray icon
    if (!CreateMessageWindow()) {
        LOG_INFO(L"Failed to create message window");
        return false;
    }
    if (!CreateTrayIcon()) {
        LOG_INFO(L"Failed to create tray icon");
        return false;
    }

    // Start window monitoring
    if (!windowMonitor_.Init()) {
        LOG_INFO(L"Failed to initialize window monitor");
        return false;
    }

    // Start mouse hook for drag detection
    if (!mouseHook_.Init()) {
        LOG_INFO(L"Failed to initialize mouse hook");
        return false;
    }

    // Register keyboard hotkeys
    keyboardHook_.Init(hMsgWnd_);

    LOG_INFO(L"App initialized");
    return true;
}

void App::Shutdown() {
    // Restore all grouped windows before shutting down
    GroupManager::Instance().UngroupAll();

    keyboardHook_.Shutdown(hMsgWnd_);
    mouseHook_.Shutdown();
    windowMonitor_.Shutdown();
    snapIndicator_.Shutdown();
    Taskbar::Instance().Shutdown();
    renderer_.Shutdown();
    RemoveTrayIcon();
    if (hMsgWnd_) {
        DestroyWindow(hMsgWnd_);
        hMsgWnd_ = nullptr;
    }
    UnregisterClassW(kMsgWndClass, hInstance_);
    LOG_INFO(L"App shutdown");
}

int App::Run() {
    running_ = true;
    MSG msg{};

    while (running_) {
        MsgWaitForMultipleObjectsEx(
            0, nullptr, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE
        );

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running_ = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

bool App::CreateMessageWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MsgWndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = kMsgWndClass;

    if (!RegisterClassExW(&wc)) return false;

    hMsgWnd_ = CreateWindowExW(
        0, kMsgWndClass, L"Groupie", 0,
        0, 0, 0, 0,
        HWND_MESSAGE, nullptr, hInstance_, nullptr
    );

    return hMsgWnd_ != nullptr;
}

bool App::CreateTrayIcon() {
    nid_.cbSize = sizeof(nid_);
    nid_.hWnd = hMsgWnd_;
    nid_.uID = 1;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;
    nid_.hIcon = LoadIconW(hInstance_, MAKEINTRESOURCEW(IDI_APPICON));
    if (!nid_.hIcon) {
        nid_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    }
    wcscpy_s(nid_.szTip, L"Groupie - Window Tab Manager");

    return Shell_NotifyIconW(NIM_ADD, &nid_) != FALSE;
}

void App::RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &nid_);
}

LRESULT CALLBACK App::MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_RBUTTONUP) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 2, L"Ungroup All");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, 1, L"Exit");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
            DestroyMenu(hMenu);

            if (cmd == 1) {
                GroupManager::Instance().UngroupAll();
                PostQuitMessage(0);
            } else if (cmd == 2) {
                GroupManager::Instance().UngroupAll();
            }
        }
        return 0;

    case WM_TIMER:
        if (wParam == 42) {  // kSnapTimerId
            DragDetector::Instance().OnSnapTimer();
        }
        return 0;

    case WM_HOTKEY:
        KeyboardHook::Instance().OnHotkey(static_cast<int>(wParam));
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
