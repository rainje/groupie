#pragma once

#include <windows.h>
#include <d2d1_1.h>
#include "util/com_ptr.h"

struct TabGroup;

class TabBarWindow {
public:
    static bool RegisterClass(HINSTANCE hInstance);

    static HWND Create(HINSTANCE hInstance, TabGroup* group);
    static void Destroy(HWND hwnd);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static constexpr const wchar_t* kClassName = L"GroupieTabBar";
};
