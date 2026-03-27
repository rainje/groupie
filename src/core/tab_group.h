#pragma once

#include <windows.h>
#include <cstdint>

static constexpr uint32_t kMaxTabsPerGroup = 16;
static constexpr int kTabBarHeight = 32;

struct TabInfo {
    HWND hwnd = nullptr;
    wchar_t title[128]{};
    HICON icon = nullptr;
    bool isActive = false;
};

struct TabGroup {
    TabInfo tabs[kMaxTabsPerGroup]{};
    uint32_t tabCount = 0;
    uint32_t activeIndex = 0;
    HWND tabBarHwnd = nullptr;
    RECT groupRect{};
    bool isMinimized = false;
    bool needsRedraw = true;
    bool adjustingPosition = false;  // prevent re-entry during position adjustments
    bool wasMaximized = false;       // track maximize state for restore on ungroup

    int FindTab(HWND hwnd) const;
    bool AddTab(HWND hwnd);
    bool RemoveTab(HWND hwnd);
    void SwitchTo(uint32_t index);
    void UpdateTitle(HWND hwnd);
    void UpdatePosition();
    void EnsureZOrder();
};
