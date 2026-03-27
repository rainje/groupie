#include "util/icon_cache.h"
#include "util/log.h"
#include "util/com_ptr.h"
#include <new>

IconCache& IconCache::Instance() {
    static IconCache instance;
    return instance;
}

HICON IconCache::ExtractWindowIcon(HWND hwnd) {
    // Try window icon first (fast, no IPC timeout)
    HICON icon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICONSM));
    if (icon) return icon;

    icon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICON));
    if (icon) return icon;

    // Try SendMessageTimeout to avoid hanging on unresponsive windows
    DWORD_PTR result = 0;
    SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL2, 0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &result);
    if (result) return reinterpret_cast<HICON>(result);

    SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &result);
    if (result) return reinterpret_cast<HICON>(result);

    SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &result);
    if (result) return reinterpret_cast<HICON>(result);

    return nullptr;
}

ID2D1Bitmap* IconCache::CreateBitmapFromIcon(ID2D1RenderTarget* rt, HICON icon) {
    if (!icon || !rt) return nullptr;

    ICONINFO ii{};
    if (!GetIconInfo(icon, &ii)) return nullptr;

    BITMAP bm{};
    GetObject(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm);

    int width = bm.bmWidth;
    int height = bm.bmHeight;
    if (width <= 0 || height <= 0) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return nullptr;
    }

    // Get pixel data via GetDIBits
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    auto* pixels = new (std::nothrow) uint8_t[width * height * 4];
    if (!pixels) {
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask) DeleteObject(ii.hbmMask);
        return nullptr;
    }

    HDC hdc = GetDC(nullptr);
    GetDIBits(hdc, ii.hbmColor ? ii.hbmColor : ii.hbmMask, 0, height, pixels, &bmi, DIB_RGB_COLORS);
    ReleaseDC(nullptr, hdc);

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);

    // Convert BGRA to premultiplied BGRA (D2D expects premultiplied alpha)
    for (int i = 0; i < width * height; i++) {
        uint8_t* p = &pixels[i * 4];
        uint8_t a = p[3];
        if (a == 0) {
            // Fully opaque if no alpha channel data
            p[3] = 255;
        } else if (a < 255) {
            p[0] = static_cast<uint8_t>((p[0] * a) / 255);
            p[1] = static_cast<uint8_t>((p[1] * a) / 255);
            p[2] = static_cast<uint8_t>((p[2] * a) / 255);
        }
    }

    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    ID2D1Bitmap* bitmap = nullptr;
    HRESULT hr = rt->CreateBitmap(
        D2D1::SizeU(width, height),
        pixels, width * 4,
        props, &bitmap
    );

    delete[] pixels;

    if (FAILED(hr)) return nullptr;
    return bitmap;
}

ID2D1Bitmap* IconCache::Get(ID2D1RenderTarget* rt, HWND hwnd) {
    // Check cache
    for (uint32_t i = 0; i < count_; i++) {
        if (entries_[i].hwnd == hwnd) {
            return entries_[i].bitmap;
        }
    }

    // Extract and convert
    HICON icon = ExtractWindowIcon(hwnd);
    if (!icon) return nullptr;

    ID2D1Bitmap* bitmap = CreateBitmapFromIcon(rt, icon);
    if (!bitmap) return nullptr;

    // Store in cache
    if (count_ < kMaxEntries) {
        entries_[count_++] = {hwnd, bitmap};
    }

    return bitmap;
}

void IconCache::Invalidate(HWND hwnd) {
    for (uint32_t i = 0; i < count_; i++) {
        if (entries_[i].hwnd == hwnd) {
            if (entries_[i].bitmap) entries_[i].bitmap->Release();
            entries_[i] = entries_[--count_];
            return;
        }
    }
}

void IconCache::Clear() {
    for (uint32_t i = 0; i < count_; i++) {
        if (entries_[i].bitmap) {
            entries_[i].bitmap->Release();
            entries_[i].bitmap = nullptr;
        }
    }
    count_ = 0;
}
