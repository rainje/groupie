#pragma once

#include <windows.h>
#include <d2d1.h>
#include <cstdint>

class IconCache {
public:
    static IconCache& Instance();

    // Get or create a D2D bitmap for the window's icon
    ID2D1Bitmap* Get(ID2D1RenderTarget* rt, HWND hwnd);
    void Invalidate(HWND hwnd);
    void Clear();

private:
    IconCache() = default;

    static uint64_t HashHwnd(HWND hwnd);
    ID2D1Bitmap* CreateBitmapFromIcon(ID2D1RenderTarget* rt, HICON icon);
    HICON ExtractWindowIcon(HWND hwnd);

    static constexpr uint32_t kMaxEntries = 128;

    struct Entry {
        HWND hwnd = nullptr;
        ID2D1Bitmap* bitmap = nullptr;
    };

    Entry entries_[kMaxEntries]{};
    uint32_t count_ = 0;
};
