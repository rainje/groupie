#include "ui/tab_bar_window.h"
#include "core/tab_group.h"
#include "ui/tab_renderer.h"
#include "ui/renderer.h"
#include "core/group_manager.h"
#include "util/win32_helpers.h"
#include "util/icon_cache.h"
#include "util/log.h"

enum class DragMode {
    None,
    MoveGroup,   // dragging empty area -> move entire group
    ReorderTab   // dragging a tab -> reorder within group
};

struct TabBarData {
    TabGroup* group = nullptr;
    ComPtr<ID2D1HwndRenderTarget> renderTarget;
    int hoverIndex = -1;

    // Drag state
    DragMode dragMode = DragMode::None;
    POINT dragStart{};
    int dragTabIndex = -1;
    RECT dragOrigTabBar{};
    RECT dragOrigGroup{};

    // Close button protection: ignore close clicks shortly after tab changes
    DWORD lastTabChangeTime = 0;
    static constexpr DWORD kCloseButtonCooldownMs = 400;
};

bool TabBarWindow::RegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbWndExtra = sizeof(void*);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;

    return RegisterClassExW(&wc) != 0;
}

HWND TabBarWindow::Create(HINSTANCE hInstance, TabGroup* group) {
    RECT rc = group->groupRect;
    DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    DWORD style = WS_POPUP | WS_VISIBLE;

    HWND hwnd = CreateWindowExW(
        exStyle, kClassName, nullptr, style,
        rc.left, rc.top - kTabBarHeight,
        rc.right - rc.left, kTabBarHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (hwnd) {
        auto* data = new TabBarData();
        data->group = group;
        SetWindowLongPtrW(hwnd, 0, reinterpret_cast<LONG_PTR>(data));
        GroupManager::Instance().RegisterOwnWindow(hwnd);
    }

    return hwnd;
}

void TabBarWindow::Destroy(HWND hwnd) {
    DestroyWindow(hwnd);
}

static TabBarData* GetData(HWND hwnd) {
    return reinterpret_cast<TabBarData*>(GetWindowLongPtrW(hwnd, 0));
}

static void EnsureRenderTarget(HWND hwnd, TabBarData* data) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT32 w = static_cast<UINT32>(rc.right - rc.left);
    UINT32 h = static_cast<UINT32>(rc.bottom - rc.top);
    if (w == 0 || h == 0) return;

    if (data->renderTarget) {
        // Resize if needed
        D2D1_SIZE_U rtSize = data->renderTarget->GetPixelSize();
        if (rtSize.width != w || rtSize.height != h) {
            data->renderTarget->Resize(D2D1::SizeU(w, h));
        }
        return;
    }

    auto* factory = Renderer::Instance().GetFactory();
    if (!factory) return;

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
        hwnd,
        D2D1::SizeU(w, h),
        D2D1_PRESENT_OPTIONS_IMMEDIATELY
    );

    // Clear icon cache — bitmaps are bound to a specific render target
    IconCache::Instance().Clear();

    HRESULT hr = factory->CreateHwndRenderTarget(rtProps, hwndProps,
        data->renderTarget.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        LOG_INFO(L"CreateHwndRenderTarget failed: 0x%08X for hwnd=%p (%ux%u)", hr, hwnd, w, h);
    } else {
        LOG_INFO(L"RenderTarget created for hwnd=%p (%ux%u)", hwnd, w, h);
    }
}

static constexpr int kDragThreshold = 5;  // px before a click becomes a drag

LRESULT CALLBACK TabBarWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* data = GetData(hwnd);

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (data && data->group) {
            EnsureRenderTarget(hwnd, data);
            if (data->renderTarget) {
                if (!TabRenderer::Paint(data->renderTarget.Get(), data->group, data->hoverIndex)) {
                    LOG_INFO(L"Paint: render target lost, recreating");
                    data->renderTarget.Release();
                    // Recreate immediately
                    EnsureRenderTarget(hwnd, data);
                    if (data->renderTarget) {
                        TabRenderer::Paint(data->renderTarget.Get(), data->group, data->hoverIndex);
                    }
                }
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!data || !data->group) return 0;

        int x = LOWORD(lParam);

        if (data->dragMode != DragMode::None) {
            // We're dragging
            POINT pt;
            GetCursorPos(&pt);
            int dx = pt.x - data->dragStart.x;
            int dy = pt.y - data->dragStart.y;

            if (data->dragMode == DragMode::MoveGroup) {
                // Move tab bar + active window
                HWND active = data->group->tabs[data->group->activeIndex].hwnd;

                RECT newTabBar = data->dragOrigTabBar;
                OffsetRect(&newTabBar, dx, dy);
                SetWindowPos(hwnd, nullptr,
                    newTabBar.left, newTabBar.top,
                    0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

                RECT newGroup = data->dragOrigGroup;
                OffsetRect(&newGroup, dx, dy);
                RECT adj = AdjustRectForDwmBorders(active, newGroup);
                SetWindowPos(active, nullptr,
                    adj.left, adj.top,
                    adj.right - adj.left, adj.bottom - adj.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);

                data->group->groupRect = newGroup;
                data->group->wasMaximized = false;
            } else if (data->dragMode == DragMode::ReorderTab) {
                // Reorder: check if mouse crossed into another tab's zone
                int targetIdx = TabRenderer::HitTest(data->group, x);
                if (targetIdx >= 0 && targetIdx != data->dragTabIndex) {
                    // Swap tabs
                    TabInfo temp = data->group->tabs[data->dragTabIndex];
                    data->group->tabs[data->dragTabIndex] = data->group->tabs[targetIdx];
                    data->group->tabs[targetIdx] = temp;

                    // Update active index if needed
                    if (data->group->activeIndex == static_cast<uint32_t>(data->dragTabIndex)) {
                        data->group->activeIndex = static_cast<uint32_t>(targetIdx);
                    } else if (data->group->activeIndex == static_cast<uint32_t>(targetIdx)) {
                        data->group->activeIndex = static_cast<uint32_t>(data->dragTabIndex);
                    }

                    data->dragTabIndex = targetIdx;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
        } else {
            // Normal hover tracking
            int newHover = TabRenderer::HitTest(data->group, x);
            if (newHover != data->hoverIndex) {
                data->hoverIndex = newHover;
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            TRACKMOUSEEVENT tme{};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        if (data) {
            data->hoverIndex = -1;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        if (!data || !data->group) return 0;

        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        auto hit = TabRenderer::HitTestEx(data->group, x, y, data->hoverIndex);

        bool closeAllowed = (GetTickCount() - data->lastTabChangeTime) > data->kCloseButtonCooldownMs;

        if (hit.tabIndex >= 0 && hit.zone == TabHitZone::CloseButton && closeAllowed) {
            // Close button: immediate action, no drag
            HWND tabHwnd = data->group->tabs[hit.tabIndex].hwnd;
            LOG_INFO(L"TabBar: close button clicked for tab %d (hwnd %p)", hit.tabIndex, tabHwnd);
            GroupManager::Instance().RemoveFromGroup(tabHwnd);
            ShowWindow(tabHwnd, SW_SHOW);

            // Reset hover to prevent cascade close
            data->hoverIndex = -1;
            data->lastTabChangeTime = GetTickCount();
            InvalidateRect(hwnd, nullptr, FALSE);
        } else if (hit.zone != TabHitZone::CloseButton) {
            // Start potential drag (either tab reorder or group move)
            GetCursorPos(&data->dragStart);

            // Store original positions
            GetWindowRect(hwnd, &data->dragOrigTabBar);
            data->dragOrigGroup = data->group->groupRect;

            if (hit.tabIndex >= 0) {
                // Clicked on a tab — switch to it and prepare for reorder drag
                data->group->SwitchTo(static_cast<uint32_t>(hit.tabIndex));
                data->lastTabChangeTime = GetTickCount();
                data->dragMode = DragMode::ReorderTab;
                data->dragTabIndex = hit.tabIndex;
            } else {
                // Clicked on empty area — prepare for group move drag
                data->dragMode = DragMode::MoveGroup;
            }

            SetCapture(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (data && data->dragMode != DragMode::None) {
            data->dragMode = DragMode::None;
            data->dragTabIndex = -1;
            data->lastTabChangeTime = GetTickCount();
            ReleaseCapture();

            if (data->group) {
                data->group->UpdatePosition();
            }
        }
        return 0;
    }

    case WM_MBUTTONDOWN: {
        if (data && data->group) {
            int x = LOWORD(lParam);
            int tabIndex = TabRenderer::HitTest(data->group, x);
            if (tabIndex >= 0) {
                HWND tabHwnd = data->group->tabs[tabIndex].hwnd;
                LOG_INFO(L"TabBar: middle-click close for tab %d (hwnd %p)", tabIndex, tabHwnd);
                GroupManager::Instance().RemoveFromGroup(tabHwnd);
                ShowWindow(tabHwnd, SW_SHOW);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_CAPTURECHANGED:
        if (data) {
            data->dragMode = DragMode::None;
            data->dragTabIndex = -1;
        }
        return 0;

    case WM_SIZE:
        if (data && data->renderTarget) {
            data->renderTarget->Resize(D2D1::SizeU(LOWORD(lParam), HIWORD(lParam)));
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;  // D2D handles all painting

    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_DESTROY: {
        auto* d = GetData(hwnd);
        if (d) {
            SetWindowLongPtrW(hwnd, 0, 0);
            delete d;
        }
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
