#pragma once

#include <windows.h>
#include <cstdint>
#include "core/tab_group.h"

class GroupManager {
public:
    static GroupManager& Instance();

    void OnWindowMoved(HWND hwnd);
    void OnWindowActivated(HWND hwnd);
    void OnWindowDestroyed(HWND hwnd);
    void OnTitleChanged(HWND hwnd);
    void OnMinimizeChanged(HWND hwnd);

    TabGroup* FindGroup(HWND hwnd);
    TabGroup* CreateGroup(HWND a, HWND b);
    void AddToGroup(TabGroup* group, HWND hwnd);
    void MergeGroups(TabGroup* dest, TabGroup* src);
    void RemoveFromGroup(HWND hwnd);
    void DestroyGroup(TabGroup* group);
    void UngroupAll();

    bool IsOwnWindow(HWND hwnd) const;
    void RegisterOwnWindow(HWND hwnd);
    void UnregisterOwnWindow(HWND hwnd);

private:
    GroupManager() = default;

    void AddLookup(HWND hwnd, TabGroup* group);
    void RemoveLookup(HWND hwnd);

    static constexpr uint32_t kMaxLookup = 256;
    static constexpr uint32_t kMaxGroups = 64;
    static constexpr uint32_t kMaxOwnWindows = 64;

    struct LookupEntry {
        HWND hwnd = nullptr;
        TabGroup* group = nullptr;
    };

    LookupEntry lookup_[kMaxLookup]{};
    uint32_t lookupCount_ = 0;

    TabGroup groupPool_[kMaxGroups]{};
    bool groupInUse_[kMaxGroups]{};
    uint32_t groupCount_ = 0;

    HWND ownWindows_[kMaxOwnWindows]{};
    uint32_t ownWindowCount_ = 0;

    bool modifying_ = false;  // reentrancy guard for group operations
};
