#include "core/window_monitor.h"
#include "core/group_manager.h"
#include "core/drag_detector.h"
#include "util/log.h"

static HWINEVENTHOOK InstallHook(DWORD eventMin, DWORD eventMax, WINEVENTPROC proc) {
    return SetWinEventHook(eventMin, eventMax, nullptr, proc, 0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

bool WindowMonitor::Init() {
    // Install targeted hooks instead of EVENT_MIN..EVENT_MAX
    // This drastically reduces the number of events we receive
    hooks_[0] = InstallHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, WinEventCallback);
    hooks_[1] = InstallHook(EVENT_SYSTEM_MOVESIZESTART, EVENT_SYSTEM_MOVESIZEEND, WinEventCallback);
    hooks_[2] = InstallHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, WinEventCallback);
    hooks_[3] = InstallHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_NAMECHANGE, WinEventCallback);

    for (int i = 0; i < kHookCount; i++) {
        if (!hooks_[i]) {
            LOG_INFO(L"Failed to install WinEvent hook %d", i);
            Shutdown();
            return false;
        }
    }

    LOG_INFO(L"Window monitor initialized");
    return true;
}

void WindowMonitor::Shutdown() {
    for (int i = 0; i < kHookCount; i++) {
        if (hooks_[i]) {
            UnhookWinEvent(hooks_[i]);
            hooks_[i] = nullptr;
        }
    }
}

void CALLBACK WindowMonitor::WinEventCallback(
    HWINEVENTHOOK, DWORD event, HWND hwnd,
    LONG idObject, LONG, DWORD, DWORD)
{
    if (idObject != OBJID_WINDOW) return;
    if (!hwnd) return;

    // Skip our own windows
    if (GroupManager::Instance().IsOwnWindow(hwnd)) return;

    auto& gm = GroupManager::Instance();
    auto& dd = DragDetector::Instance();

    switch (event) {
    case EVENT_OBJECT_LOCATIONCHANGE:
        gm.OnWindowMoved(hwnd);
        break;
    case EVENT_SYSTEM_MOVESIZESTART:
        dd.OnMoveStart(hwnd);
        break;
    case EVENT_SYSTEM_MOVESIZEEND:
        dd.OnMoveEnd(hwnd);
        break;
    case EVENT_SYSTEM_FOREGROUND:
        gm.OnWindowActivated(hwnd);
        break;
    case EVENT_OBJECT_DESTROY:
        gm.OnWindowDestroyed(hwnd);
        break;
    case EVENT_OBJECT_NAMECHANGE:
        gm.OnTitleChanged(hwnd);
        break;
    case EVENT_SYSTEM_MINIMIZESTART:
    case EVENT_SYSTEM_MINIMIZEEND:
        gm.OnMinimizeChanged(hwnd);
        break;
    }
}
