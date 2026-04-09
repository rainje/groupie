#include "core/group_manager.h"
#include "core/app.h"
#include "ui/tab_bar_window.h"
#include "util/win32_helpers.h"
#include "util/taskbar.h"
#include "util/log.h"

// RAII guard for modifying_ flag
struct ModifyGuard {
    bool& flag;
    ModifyGuard(bool& f) : flag(f) { flag = true; }
    ~ModifyGuard() { flag = false; }
};

GroupManager& GroupManager::Instance() {
    static GroupManager instance;
    return instance;
}

TabGroup* GroupManager::FindGroup(HWND hwnd) {
    for (uint32_t i = 0; i < lookupCount_; i++) {
        if (lookup_[i].hwnd == hwnd) return lookup_[i].group;
    }
    return nullptr;
}

void GroupManager::AddLookup(HWND hwnd, TabGroup* group) {
    for (uint32_t i = 0; i < lookupCount_; i++) {
        if (lookup_[i].hwnd == hwnd) {
            lookup_[i].group = group;
            return;
        }
    }
    if (lookupCount_ < kMaxLookup) {
        lookup_[lookupCount_++] = {hwnd, group};
    }
}

void GroupManager::RemoveLookup(HWND hwnd) {
    for (uint32_t i = 0; i < lookupCount_; i++) {
        if (lookup_[i].hwnd == hwnd) {
            lookup_[i] = lookup_[--lookupCount_];
            return;
        }
    }
}

bool GroupManager::IsOwnWindow(HWND hwnd) const {
    for (uint32_t i = 0; i < ownWindowCount_; i++) {
        if (ownWindows_[i] == hwnd) return true;
    }
    return false;
}

void GroupManager::RegisterOwnWindow(HWND hwnd) {
    if (ownWindowCount_ < kMaxOwnWindows) {
        ownWindows_[ownWindowCount_++] = hwnd;
    }
}

void GroupManager::UnregisterOwnWindow(HWND hwnd) {
    for (uint32_t i = 0; i < ownWindowCount_; i++) {
        if (ownWindows_[i] == hwnd) {
            ownWindows_[i] = ownWindows_[--ownWindowCount_];
            return;
        }
    }
}

TabGroup* GroupManager::CreateGroup(HWND a, HWND b) {
    ModifyGuard guard(modifying_);

    TabGroup* groupA = FindGroup(a);
    TabGroup* groupB = FindGroup(b);

    if (groupA && groupB) {
        if (groupA != groupB) {
            MergeGroups(groupB, groupA);
        }
        return groupB;
    }

    if (groupB) {
        AddToGroup(groupB, a);
        return groupB;
    }

    if (groupA) {
        AddToGroup(groupA, b);
        return groupA;
    }

    // Neither is grouped - create new group
    TabGroup* group = nullptr;
    for (uint32_t i = 0; i < kMaxGroups; i++) {
        if (!groupInUse_[i]) {
            group = &groupPool_[i];
            *group = TabGroup{};
            groupInUse_[i] = true;
            groupCount_++;
            break;
        }
    }
    if (!group) {
        LOG_INFO(L"Max groups reached");
        return nullptr;
    }

    group->groupRect = GetExtendedFrameBounds(b);

    group->tabBarHwnd = TabBarWindow::Create(App::Instance().GetHInstance(), group);
    if (!group->tabBarHwnd) {
        LOG_INFO(L"Failed to create tab bar window");
        for (uint32_t i = 0; i < kMaxGroups; i++) {
            if (&groupPool_[i] == group) { groupInUse_[i] = false; groupCount_--; break; }
        }
        return nullptr;
    }

    // B (target) is the active tab — it stays in place
    group->AddTab(b);
    group->tabs[0].isActive = true;
    group->activeIndex = 0;
    AddLookup(b, group);

    // A (source/dragged) moves to the group position and becomes inactive
    group->AddTab(a);
    AddLookup(a, group);

    // Activate B (target) BEFORE hiding A — prevents focus loss
    SetForegroundWindow(b);
    BringWindowToTop(b);

    RECT adj = AdjustRectForDwmBorders(a, group->groupRect);
    SetWindowPos(a, nullptr,
        adj.left, adj.top,
        adj.right - adj.left, adj.bottom - adj.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(a, SW_HIDE);

    // Hide inactive tab from taskbar
    Taskbar::Instance().HideButton(a);

    group->UpdatePosition();
    group->ForceActivate();

    if (group->tabBarHwnd) {
        InvalidateRect(group->tabBarHwnd, nullptr, FALSE);
        UpdateWindow(group->tabBarHwnd);
    }

    LOG_INFO(L"Group created with %p and %p (tabs:%d active=%d tabBar=%p visible=%d)",
        a, b, group->tabCount, group->activeIndex,
        group->tabBarHwnd, group->tabBarHwnd ? IsWindowVisible(group->tabBarHwnd) : -1);
    return group;
}

void GroupManager::AddToGroup(TabGroup* group, HWND hwnd) {
    if (!group) return;
    if (group->FindTab(hwnd) >= 0) return;

    // If in another group, extract it first
    TabGroup* existing = FindGroup(hwnd);
    if (existing && existing != group) {
        existing->RemoveTab(hwnd);
        RemoveLookup(hwnd);

        if (existing->tabCount <= 1) {
            if (existing->tabCount == 1) {
                HWND remaining = existing->tabs[0].hwnd;
                RemoveLookup(remaining);
                ShowWindow(remaining, SW_SHOW);
            }
            DestroyGroup(existing);
        } else {
            ShowWindow(existing->tabs[existing->activeIndex].hwnd, SW_SHOW);
            existing->UpdatePosition();
            if (existing->tabBarHwnd) {
                InvalidateRect(existing->tabBarHwnd, nullptr, FALSE);
            }
        }
    }

    group->AddTab(hwnd);
    AddLookup(hwnd, group);

    // Activate the current active tab BEFORE hiding the new one
    HWND active = group->tabs[group->activeIndex].hwnd;
    SetForegroundWindow(active);
    BringWindowToTop(active);

    // Move the new window to the group's position and hide it (inactive tab)
    RECT adj = AdjustRectForDwmBorders(hwnd, group->groupRect);
    SetWindowPos(hwnd, nullptr,
        adj.left, adj.top,
        adj.right - adj.left, adj.bottom - adj.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(hwnd, SW_HIDE);

    // Hide from taskbar
    Taskbar::Instance().HideButton(hwnd);

    // Keep current active tab visible, ensure tab bar is shown
    if (group->tabBarHwnd) {
        ShowWindow(group->tabBarHwnd, SW_SHOWNOACTIVATE);
        InvalidateRect(group->tabBarHwnd, nullptr, FALSE);
        UpdateWindow(group->tabBarHwnd);
    }
    group->ForceActivate();

    LOG_INFO(L"Added window %p to group (now %d tabs, active=%d, tabBar=%p visible=%d)",
        hwnd, group->tabCount, group->activeIndex,
        group->tabBarHwnd, group->tabBarHwnd ? IsWindowVisible(group->tabBarHwnd) : -1);
}

void GroupManager::MergeGroups(TabGroup* dest, TabGroup* src) {
    if (!dest || !src || dest == src) return;
    ModifyGuard guard(modifying_);

    LOG_INFO(L"Merging groups: %d tabs into %d tabs", src->tabCount, dest->tabCount);

    for (uint32_t i = 0; i < src->tabCount; i++) {
        HWND hwnd = src->tabs[i].hwnd;
        if (dest->tabCount >= kMaxTabsPerGroup) break;

        dest->AddTab(hwnd);
        AddLookup(hwnd, dest);

        if (hwnd != dest->tabs[dest->activeIndex].hwnd) {
            ShowWindow(hwnd, SW_HIDE);
        }
    }

    src->tabCount = 0;
    DestroyGroup(src);

    dest->UpdatePosition();
    dest->ForceActivate();
    if (dest->tabBarHwnd) {
        InvalidateRect(dest->tabBarHwnd, nullptr, FALSE);
        UpdateWindow(dest->tabBarHwnd);
    }

    LOG_INFO(L"Merge complete: dest now has %d tabs", dest->tabCount);
}

void GroupManager::RemoveFromGroup(HWND hwnd) {
    if (modifying_) {
        LOG_INFO(L"RemoveFromGroup BLOCKED (reentrant): %p", hwnd);
        return;
    }

    TabGroup* group = FindGroup(hwnd);
    if (!group) return;

    LOG_INFO(L"RemoveFromGroup: %p (group has %d tabs, active=%d)",
        hwnd, group->tabCount, group->activeIndex);

    bool wasActive = (group->FindTab(hwnd) == static_cast<int>(group->activeIndex));

    group->RemoveTab(hwnd);
    RemoveLookup(hwnd);

    ShowWindow(hwnd, SW_SHOW);
    Taskbar::Instance().ShowButton(hwnd);

    if (group->tabCount <= 1) {
        if (group->tabCount == 1) {
            HWND remaining = group->tabs[0].hwnd;
            RemoveLookup(remaining);
            ShowWindow(remaining, SW_SHOW);
            Taskbar::Instance().ShowButton(remaining);
        }
        DestroyGroup(group);
    } else {
        if (wasActive) {
            group->tabs[group->activeIndex].isActive = true;
            ShowWindow(group->tabs[group->activeIndex].hwnd, SW_SHOW);
            SetForegroundWindow(group->tabs[group->activeIndex].hwnd);
        }
        group->UpdatePosition();
        if (group->tabBarHwnd) {
            InvalidateRect(group->tabBarHwnd, nullptr, FALSE);
        }
    }
}

void GroupManager::DestroyGroup(TabGroup* group) {
    if (!group) return;

    LOG_INFO(L"Destroying group with %d tabs", group->tabCount);

    for (uint32_t i = 0; i < group->tabCount; i++) {
        HWND hwnd = group->tabs[i].hwnd;
        RemoveLookup(hwnd);
        if (IsWindow(hwnd)) {
            if (!IsWindowVisible(hwnd)) {
                ShowWindow(hwnd, SW_SHOW);
                LOG_INFO(L"  Restored hidden window %p", hwnd);
            }
            Taskbar::Instance().ShowButton(hwnd);
        }
    }

    if (group->tabBarHwnd) {
        UnregisterOwnWindow(group->tabBarHwnd);
        DestroyWindow(group->tabBarHwnd);
        group->tabBarHwnd = nullptr;
    }

    for (uint32_t i = 0; i < kMaxGroups; i++) {
        if (&groupPool_[i] == group) {
            groupInUse_[i] = false;
            groupCount_--;
            break;
        }
    }

    LOG_INFO(L"Group destroyed");
}

void GroupManager::UngroupAll() {
    LOG_INFO(L"Ungrouping all windows");

    for (uint32_t i = 0; i < kMaxGroups; i++) {
        if (!groupInUse_[i]) continue;

        TabGroup* group = &groupPool_[i];

        for (uint32_t t = 0; t < group->tabCount; t++) {
            ShowWindow(group->tabs[t].hwnd, SW_SHOW);
            Taskbar::Instance().ShowButton(group->tabs[t].hwnd);
            RemoveLookup(group->tabs[t].hwnd);
        }

        if (group->tabBarHwnd) {
            UnregisterOwnWindow(group->tabBarHwnd);
            DestroyWindow(group->tabBarHwnd);
            group->tabBarHwnd = nullptr;
        }

        groupInUse_[i] = false;
        groupCount_--;
    }
}

void GroupManager::OnWindowMoved(HWND hwnd) {
    if (modifying_) return;

    TabGroup* group = FindGroup(hwnd);
    if (!group) return;

    if (group->tabs[group->activeIndex].hwnd == hwnd) {
        group->UpdatePosition();
    }
}

void GroupManager::OnWindowActivated(HWND hwnd) {
    if (modifying_) return;

    TabGroup* group = FindGroup(hwnd);

    if (!group) return;

    int idx = group->FindTab(hwnd);
    if (idx >= 0 && static_cast<uint32_t>(idx) != group->activeIndex) {
        group->tabs[group->activeIndex].isActive = false;
        ShowWindow(group->tabs[group->activeIndex].hwnd, SW_HIDE);
        Taskbar::Instance().HideButton(group->tabs[group->activeIndex].hwnd);

        group->activeIndex = static_cast<uint32_t>(idx);
        group->tabs[idx].isActive = true;
        group->needsRedraw = true;

        // Ensure the newly activated window is visible
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        } else if (!IsWindowVisible(hwnd)) {
            ShowWindow(hwnd, SW_SHOW);
        }
        Taskbar::Instance().ShowButton(hwnd);

        group->isMinimized = false;
        group->UpdatePosition();
        group->EnsureZOrder();
        if (group->tabBarHwnd) {
            ShowWindow(group->tabBarHwnd, SW_SHOWNOACTIVATE);
            InvalidateRect(group->tabBarHwnd, nullptr, FALSE);
        }
    } else if (idx >= 0) {
        group->EnsureZOrder();
    }
}

void GroupManager::OnWindowDestroyed(HWND hwnd) {
    if (modifying_) return;

    TabGroup* group = FindGroup(hwnd);
    if (!group) return;

    LOG_INFO(L"Window destroyed: %p (group has %d tabs)", hwnd, group->tabCount);
    RemoveFromGroup(hwnd);
}

void GroupManager::OnTitleChanged(HWND hwnd) {
    if (modifying_) return;

    TabGroup* group = FindGroup(hwnd);
    if (!group) return;

    group->UpdateTitle(hwnd);
    if (group->tabBarHwnd) {
        InvalidateRect(group->tabBarHwnd, nullptr, FALSE);
    }
}

void GroupManager::OnMinimizeChanged(HWND hwnd) {
    if (modifying_) return;

    TabGroup* group = FindGroup(hwnd);
    if (!group) return;

    BOOL isMinimized = IsIconic(hwnd);
    if (isMinimized && !group->isMinimized) {
        group->isMinimized = true;
        if (group->tabBarHwnd) {
            ShowWindow(group->tabBarHwnd, SW_HIDE);
        }
    } else if (!isMinimized && group->isMinimized) {
        group->isMinimized = false;
        if (group->tabBarHwnd) {
            ShowWindow(group->tabBarHwnd, SW_SHOWNOACTIVATE);
        }
        group->UpdatePosition();
    }
}
