#pragma once

#include <windows.h>
#include <dwmapi.h>

// RAII wrapper for Windows handles
class ScopedHandle {
public:
    explicit ScopedHandle(HANDLE h = nullptr) noexcept : handle_(h) {}
    ~ScopedHandle() { if (handle_ && handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); }
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
    ScopedHandle(ScopedHandle&& o) noexcept : handle_(o.handle_) { o.handle_ = nullptr; }
    ScopedHandle& operator=(ScopedHandle&& o) noexcept {
        if (this != &o) { if (handle_ && handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_); handle_ = o.handle_; o.handle_ = nullptr; }
        return *this;
    }
    HANDLE Get() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }
private:
    HANDLE handle_;
};

// Get accurate window bounds (excluding DWM invisible borders)
inline RECT GetExtendedFrameBounds(HWND hwnd) {
    RECT rc{};
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rc, sizeof(rc)))) {
        GetWindowRect(hwnd, &rc);
    }
    return rc;
}

// Get the invisible DWM border margins (difference between GetWindowRect and visible bounds)
// These margins must be added back when using SetWindowPos to fill a target rect exactly.
struct FrameMargins {
    int left, top, right, bottom;
};

inline FrameMargins GetInvisibleBorders(HWND hwnd) {
    RECT full{}, visible{};
    GetWindowRect(hwnd, &full);
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &visible, sizeof(visible)))) {
        return {0, 0, 0, 0};
    }
    return {
        visible.left - full.left,     // left invisible border
        visible.top - full.top,       // top invisible border (usually 0)
        full.right - visible.right,   // right invisible border
        full.bottom - visible.bottom  // bottom invisible border
    };
}

// Compute SetWindowPos rect so that the VISIBLE area fills targetRect exactly
inline RECT AdjustRectForDwmBorders(HWND hwnd, const RECT& targetRect) {
    FrameMargins m = GetInvisibleBorders(hwnd);
    return {
        targetRect.left - m.left,
        targetRect.top - m.top,
        targetRect.right + m.right,
        targetRect.bottom + m.bottom
    };
}

// Check if a window is a valid top-level application window
inline bool IsTopLevelAppWindow(HWND hwnd) {
    if (!IsWindowVisible(hwnd)) return false;
    if (GetParent(hwnd) != nullptr) return false;

    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return false;

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (!(style & WS_CAPTION)) return false;

    // Skip cloaked windows (hidden UWP apps)
    BOOL cloaked = FALSE;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (cloaked) return false;

    // Must have nonzero size
    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (rc.right - rc.left <= 0 || rc.bottom - rc.top <= 0) return false;

    return true;
}
