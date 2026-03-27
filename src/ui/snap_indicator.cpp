#include "ui/snap_indicator.h"
#include "util/win32_helpers.h"
#include "util/log.h"

// Store armed state for WndProc to read
static bool s_armed = false;

SnapIndicator& SnapIndicator::Instance() {
    static SnapIndicator instance;
    return instance;
}

bool SnapIndicator::Init(HINSTANCE hInstance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kClassName;

    if (!RegisterClassExW(&wc)) return false;

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT
                  | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;

    hwnd_ = CreateWindowExW(
        exStyle, kClassName, nullptr, WS_POPUP,
        0, 0, 100, kIndicatorHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd_) return false;

    SetLayeredWindowAttributes(hwnd_, 0, 220, LWA_ALPHA);

    LOG_INFO(L"Snap indicator initialized");
    return true;
}

void SnapIndicator::Shutdown() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void SnapIndicator::Show(HWND targetWindow) {
    if (!hwnd_ || !targetWindow) return;

    UpdatePosition(targetWindow);

    if (!visible_) {
        ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
        visible_ = true;
    }
}

void SnapIndicator::SetArmed(bool armed) {
    if (armed_ == armed) return;
    armed_ = armed;
    s_armed = armed;
    if (hwnd_) {
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void SnapIndicator::Hide() {
    if (!hwnd_ || !visible_) return;

    ShowWindow(hwnd_, SW_HIDE);
    visible_ = false;
    armed_ = false;
    s_armed = false;
}

void SnapIndicator::UpdatePosition(HWND targetWindow) {
    RECT rc = GetExtendedFrameBounds(targetWindow);

    SetWindowPos(hwnd_, HWND_TOPMOST,
        rc.left, rc.top - kIndicatorHeight,
        rc.right - rc.left, kIndicatorHeight,
        SWP_NOACTIVATE);
}

LRESULT CALLBACK SnapIndicator::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        // Blue = waiting, Green = armed (ready to group)
        COLORREF color = s_armed ? RGB(0, 180, 80) : RGB(0, 120, 215);
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST:
        return HTTRANSPARENT;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
