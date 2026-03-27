#include "input/mouse_hook.h"
#include "core/drag_detector.h"
#include "util/log.h"

MouseHook& MouseHook::Instance() {
    static MouseHook instance;
    return instance;
}

bool MouseHook::Init() {
    hHook_ = SetWindowsHookExW(WH_MOUSE_LL, LowLevelProc, nullptr, 0);
    if (!hHook_) {
        LOG_INFO(L"Failed to install mouse hook");
        return false;
    }
    LOG_INFO(L"Mouse hook installed");
    return true;
}

void MouseHook::Shutdown() {
    if (hHook_) {
        UnhookWindowsHookEx(hHook_);
        hHook_ = nullptr;
    }
}

LRESULT CALLBACK MouseHook::LowLevelProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE) {
        auto* ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        // Must be fast - only a point update
        DragDetector::Instance().OnMouseMove(ms->pt);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
