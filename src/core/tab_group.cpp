#include "core/tab_group.h"
#include "util/win32_helpers.h"
#include "util/taskbar.h"
#include "util/log.h"

int TabGroup::FindTab(HWND hwnd) const {
    for (uint32_t i = 0; i < tabCount; i++) {
        if (tabs[i].hwnd == hwnd) return static_cast<int>(i);
    }
    return -1;
}

bool TabGroup::AddTab(HWND hwnd) {
    if (tabCount >= kMaxTabsPerGroup) return false;
    if (FindTab(hwnd) >= 0) return false;

    TabInfo& tab = tabs[tabCount];
    tab.hwnd = hwnd;
    tab.isActive = false;
    GetWindowTextW(hwnd, tab.title, _countof(tab.title));
    tab.icon = nullptr;

    tabCount++;
    needsRedraw = true;
    return true;
}

bool TabGroup::RemoveTab(HWND hwnd) {
    int idx = FindTab(hwnd);
    if (idx < 0) return false;

    for (uint32_t i = static_cast<uint32_t>(idx); i < tabCount - 1; i++) {
        tabs[i] = tabs[i + 1];
    }
    tabCount--;
    tabs[tabCount] = TabInfo{};

    if (activeIndex >= tabCount && tabCount > 0) {
        activeIndex = tabCount - 1;
    }
    if (tabCount > 0) {
        tabs[activeIndex].isActive = true;
    }

    needsRedraw = true;
    return true;
}

static RECT GetWorkArea(HWND hwnd) {
    HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMon, &mi);
    return mi.rcWork;
}

static void SetWindowToFillRect(HWND hwnd, const RECT& targetRect, UINT flags) {
    RECT adj = AdjustRectForDwmBorders(hwnd, targetRect);
    SetWindowPos(hwnd, nullptr,
        adj.left, adj.top,
        adj.right - adj.left,
        adj.bottom - adj.top,
        flags);
}

// Force a window to the foreground even from a background process
static void ForceSetForeground(HWND hwnd) {
    DWORD ourThread = GetCurrentThreadId();
    DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    DWORD targetThread = GetWindowThreadProcessId(hwnd, nullptr);

    if (ourThread != fgThread) {
        AttachThreadInput(ourThread, fgThread, TRUE);
    }
    if (ourThread != targetThread && fgThread != targetThread) {
        AttachThreadInput(ourThread, targetThread, TRUE);
    }

    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(hwnd);

    if (ourThread != fgThread) {
        AttachThreadInput(ourThread, fgThread, FALSE);
    }
    if (ourThread != targetThread && fgThread != targetThread) {
        AttachThreadInput(ourThread, targetThread, FALSE);
    }
}

void TabGroup::EnsureZOrder() {
    if (!tabBarHwnd || tabCount == 0) return;
    HWND active = tabs[activeIndex].hwnd;

    // Make tab bar owned by active window — owned windows are always above their owner
    HWND currentOwner = GetWindow(tabBarHwnd, GW_OWNER);
    if (currentOwner != active) {
        SetWindowLongPtrW(tabBarHwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(active));
    }

    ForceSetForeground(active);
}

void TabGroup::SwitchTo(uint32_t index) {
    if (index >= tabCount || index == activeIndex) return;

    HWND oldActive = tabs[activeIndex].hwnd;
    tabs[activeIndex].isActive = false;
    ShowWindow(oldActive, SW_HIDE);
    Taskbar::Instance().HideButton(oldActive);

    activeIndex = index;
    tabs[activeIndex].isActive = true;

    HWND active = tabs[activeIndex].hwnd;
    Taskbar::Instance().ShowButton(active);

    adjustingPosition = true;

    if (IsZoomed(active)) {
        ShowWindow(active, SW_RESTORE);
    }

    SetWindowToFillRect(active, groupRect, SWP_NOZORDER | SWP_SHOWWINDOW);

    adjustingPosition = false;
    needsRedraw = true;

    EnsureZOrder();
}

void TabGroup::UpdateTitle(HWND hwnd) {
    int idx = FindTab(hwnd);
    if (idx >= 0) {
        GetWindowTextW(hwnd, tabs[idx].title, _countof(tabs[idx].title));
        needsRedraw = true;
    }
}

void TabGroup::UpdatePosition() {
    if (tabCount == 0 || !tabBarHwnd) return;
    if (adjustingPosition) return;

    HWND active = tabs[activeIndex].hwnd;
    bool maximized = IsZoomed(active);

    if (maximized) {
        wasMaximized = true;
        adjustingPosition = true;

        ShowWindow(active, SW_RESTORE);

        RECT wa = GetWorkArea(active);
        groupRect = {wa.left, wa.top + kTabBarHeight, wa.right, wa.bottom};
        SetWindowToFillRect(active, groupRect, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        SetWindowPos(tabBarHwnd, nullptr,
            wa.left, wa.top,
            wa.right - wa.left, kTabBarHeight,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

        adjustingPosition = false;
        EnsureZOrder();
    } else {
        RECT currentRect = GetExtendedFrameBounds(active);

        RECT wa = GetWorkArea(active);
        int tabBarTop = currentRect.top - kTabBarHeight;

        if (tabBarTop < wa.top) {
            adjustingPosition = true;

            RECT target = {currentRect.left, wa.top + kTabBarHeight, currentRect.right, currentRect.bottom};
            SetWindowToFillRect(active, target, SWP_NOZORDER | SWP_NOACTIVATE);

            groupRect = target;
            tabBarTop = wa.top;
            wasMaximized = true;
            adjustingPosition = false;
        } else {
            groupRect = currentRect;
            wasMaximized = false;
        }

        // Position tab bar (Z-order handled by ownership)
        SetWindowPos(tabBarHwnd, nullptr,
            groupRect.left, tabBarTop,
            groupRect.right - groupRect.left, kTabBarHeight,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}
