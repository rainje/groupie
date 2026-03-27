#pragma once

#include <windows.h>

class SnapIndicator {
public:
    static SnapIndicator& Instance();

    bool Init(HINSTANCE hInstance);
    void Shutdown();

    void Show(HWND targetWindow);
    void SetArmed(bool armed);
    void Hide();
    bool IsVisible() const { return visible_; }

private:
    SnapIndicator() = default;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void UpdatePosition(HWND targetWindow);

    HWND hwnd_ = nullptr;
    bool visible_ = false;
    bool armed_ = false;

    static constexpr int kIndicatorHeight = 4;
    static constexpr const wchar_t* kClassName = L"GroupieSnapIndicator";
};
