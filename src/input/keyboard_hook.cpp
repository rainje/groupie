#include "input/keyboard_hook.h"
#include "core/group_manager.h"
#include "util/log.h"

KeyboardHook& KeyboardHook::Instance() {
    static KeyboardHook instance;
    return instance;
}

bool KeyboardHook::Init(HWND msgWnd) {
    // Ctrl+Tab -> next tab, Ctrl+Shift+Tab -> prev tab, Ctrl+W -> close tab
    BOOL r1 = RegisterHotKey(msgWnd, HOTKEY_NEXT_TAB, MOD_CONTROL | MOD_ALT, VK_TAB);
    BOOL r2 = RegisterHotKey(msgWnd, HOTKEY_PREV_TAB, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_TAB);
    BOOL r3 = RegisterHotKey(msgWnd, HOTKEY_CLOSE_TAB, MOD_CONTROL | MOD_ALT, 'W');

    if (!r1 || !r2 || !r3) {
        LOG_INFO(L"Some hotkeys failed to register (may be in use by another app)");
    }

    LOG_INFO(L"Keyboard hotkeys registered");
    return true;
}

void KeyboardHook::Shutdown(HWND msgWnd) {
    UnregisterHotKey(msgWnd, HOTKEY_NEXT_TAB);
    UnregisterHotKey(msgWnd, HOTKEY_PREV_TAB);
    UnregisterHotKey(msgWnd, HOTKEY_CLOSE_TAB);
}

void KeyboardHook::OnHotkey(int id) {
    HWND fg = GetForegroundWindow();
    if (!fg) return;

    auto& gm = GroupManager::Instance();
    TabGroup* group = gm.FindGroup(fg);
    if (!group) return;

    LOG_INFO(L"Hotkey %d triggered (fg=%p)", id, fg);

    switch (id) {
    case HOTKEY_NEXT_TAB: {
        uint32_t next = (group->activeIndex + 1) % group->tabCount;
        group->SwitchTo(next);
        if (group->tabBarHwnd) InvalidateRect(group->tabBarHwnd, nullptr, FALSE);
        break;
    }
    case HOTKEY_PREV_TAB: {
        uint32_t prev = (group->activeIndex == 0) ? group->tabCount - 1 : group->activeIndex - 1;
        group->SwitchTo(prev);
        if (group->tabBarHwnd) InvalidateRect(group->tabBarHwnd, nullptr, FALSE);
        break;
    }
    case HOTKEY_CLOSE_TAB: {
        HWND tabHwnd = group->tabs[group->activeIndex].hwnd;
        gm.RemoveFromGroup(tabHwnd);
        ShowWindow(tabHwnd, SW_SHOW);
        break;
    }
    }
}
