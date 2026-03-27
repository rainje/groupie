#include "config/settings.h"
#include <windows.h>

static const wchar_t* kRegKey = L"Software\\Groupie";

Settings& Settings::Instance() {
    static Settings instance;
    return instance;
}

void Settings::Load() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRegKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return; // Use defaults
    }

    DWORD val, size;

    size = sizeof(val);
    if (RegQueryValueExW(hKey, L"TabBarHeight", nullptr, nullptr, reinterpret_cast<BYTE*>(&val), &size) == ERROR_SUCCESS) {
        tabBarHeight = static_cast<int>(val);
    }

    size = sizeof(val);
    if (RegQueryValueExW(hKey, L"SnapDistance", nullptr, nullptr, reinterpret_cast<BYTE*>(&val), &size) == ERROR_SUCCESS) {
        snapDistance = static_cast<int>(val);
    }

    size = sizeof(val);
    if (RegQueryValueExW(hKey, L"EnableAnimations", nullptr, nullptr, reinterpret_cast<BYTE*>(&val), &size) == ERROR_SUCCESS) {
        enableAnimations = val != 0;
    }

    RegCloseKey(hKey);
}

void Settings::Save() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegKey, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return;
    }

    DWORD val;

    val = static_cast<DWORD>(tabBarHeight);
    RegSetValueExW(hKey, L"TabBarHeight", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));

    val = static_cast<DWORD>(snapDistance);
    RegSetValueExW(hKey, L"SnapDistance", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));

    val = enableAnimations ? 1 : 0;
    RegSetValueExW(hKey, L"EnableAnimations", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));

    RegCloseKey(hKey);
}
