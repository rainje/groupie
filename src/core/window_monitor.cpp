#include "core/window_monitor.h"
#include "core/group_manager.h"
#include "core/drag_detector.h"
#include "util/log.h"

bool WindowMonitor::Init() {
    hHook_ = SetWinEventHook(
        EVENT_MIN, EVENT_MAX,
        nullptr,
        WinEventCallback,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    if (!hHook_) {
        LOG_INFO(L"Failed to install WinEvent hook");
        return false;
    }

    LOG_INFO(L"Window monitor initialized");
    return true;
}

void WindowMonitor::Shutdown() {
    if (hHook_) {
        UnhookWinEvent(hHook_);
        hHook_ = nullptr;
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
