#include "ui/tab_renderer.h"
#include "core/tab_group.h"
#include "ui/renderer.h"
#include "ui/theme.h"
#include "util/com_ptr.h"
#include "util/icon_cache.h"
#include "util/log.h"

#include <algorithm>

float TabRenderer::CalculateTabWidth(TabGroup* group, float totalWidth) {
    const auto& theme = Theme::Default();
    if (group->tabCount == 0) return 0;
    float w = totalWidth / static_cast<float>(group->tabCount);
    return std::clamp(w, theme.tabMinWidth, theme.tabMaxWidth);
}

bool TabRenderer::Paint(ID2D1HwndRenderTarget* rt, TabGroup* group, int hoverIndex) {
    const auto& theme = Theme::Default();
    auto* tabFont = Renderer::Instance().GetTabFont();

    rt->BeginDraw();
    rt->Clear(theme.background);

    D2D1_SIZE_F size = rt->GetSize();
    float tabW = CalculateTabWidth(group, size.width);

    ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(theme.activeTab, brush.ReleaseAndGetAddressOf());
    if (!brush) {
        LOG_INFO(L"CreateSolidColorBrush failed, size=%.0fx%.0f tabs=%d", size.width, size.height, group->tabCount);
        rt->EndDraw();
        return true;
    }

    float x = 0;
    for (uint32_t i = 0; i < group->tabCount; i++) {
        D2D1_RECT_F tabRect = {x, 0, x + tabW, size.height};

        // Tab background
        D2D1_COLOR_F bgColor;
        if (i == group->activeIndex) {
            bgColor = theme.activeTab;
        } else if (static_cast<int>(i) == hoverIndex) {
            bgColor = theme.hoverTab;
        } else {
            bgColor = theme.inactiveTab;
        }
        brush->SetColor(bgColor);

        D2D1_ROUNDED_RECT rr = {tabRect, theme.cornerRadius, theme.cornerRadius};
        rt->FillRoundedRectangle(rr, brush.Get());

        // Icon
        float iconX = x + theme.tabPadding;
        float iconY = (size.height - theme.iconSize) * 0.5f;
        ID2D1Bitmap* iconBmp = IconCache::Instance().Get(rt, group->tabs[i].hwnd);
        if (iconBmp) {
            D2D1_RECT_F iconRect = {iconX, iconY, iconX + theme.iconSize, iconY + theme.iconSize};
            rt->DrawBitmap(iconBmp, iconRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }

        // Tab title
        D2D1_COLOR_F textColor = (i == group->activeIndex) ? theme.text : theme.inactiveText;
        brush->SetColor(textColor);

        float textLeft = x + theme.tabPadding + theme.iconSize + 6.0f;
        float textRight = x + tabW - theme.tabPadding;

        // Reserve space for close button on hover
        if (static_cast<int>(i) == hoverIndex) {
            textRight -= theme.closeButtonSize + 4.0f;
        }

        D2D1_RECT_F textRect = {textLeft, 0, textRight, size.height};

        if (tabFont && textRight > textLeft) {
            rt->DrawTextW(
                group->tabs[i].title,
                static_cast<UINT32>(wcslen(group->tabs[i].title)),
                tabFont, textRect, brush.Get(),
                D2D1_DRAW_TEXT_OPTIONS_CLIP
            );
        }

        // Close button on hover
        if (static_cast<int>(i) == hoverIndex) {
            float cbSize = theme.closeButtonSize;
            float cbX = x + tabW - theme.tabPadding - cbSize;
            float cbY = (size.height - cbSize) * 0.5f;

            brush->SetColor(theme.closeHover);

            // Draw X
            float margin = 3.0f;
            rt->DrawLine(
                {cbX + margin, cbY + margin},
                {cbX + cbSize - margin, cbY + cbSize - margin},
                brush.Get(), 1.5f);
            rt->DrawLine(
                {cbX + cbSize - margin, cbY + margin},
                {cbX + margin, cbY + cbSize - margin},
                brush.Get(), 1.5f);
        }

        // Separator between inactive tabs
        if (i < group->tabCount - 1 && i != group->activeIndex &&
            (i + 1) != group->activeIndex) {
            brush->SetColor(theme.separator);
            D2D1_POINT_2F p1 = {x + tabW, 6.0f};
            D2D1_POINT_2F p2 = {x + tabW, size.height - 6.0f};
            rt->DrawLine(p1, p2, brush.Get(), 1.0f);
        }

        x += tabW;
    }

    HRESULT hr = rt->EndDraw();
    group->needsRedraw = false;

    if (FAILED(hr)) {
        LOG_INFO(L"EndDraw failed: 0x%08X", hr);
        return false;
    }
    return true;
}

int TabRenderer::HitTest(TabGroup* group, int mouseX) {
    auto result = HitTestEx(group, mouseX, 0, -1);
    return result.tabIndex;
}

TabHitResult TabRenderer::HitTestEx(TabGroup* group, int mouseX, int mouseY, int hoverIndex) {
    TabHitResult result;
    if (group->tabCount == 0) return result;

    float totalWidth = 0;
    RECT rc;
    if (group->tabBarHwnd) {
        GetClientRect(group->tabBarHwnd, &rc);
        totalWidth = static_cast<float>(rc.right - rc.left);
    }
    if (totalWidth <= 0) return result;

    const auto& theme = Theme::Default();
    float tabW = CalculateTabWidth(group, totalWidth);
    float barHeight = static_cast<float>(rc.bottom - rc.top);
    int index = static_cast<int>(static_cast<float>(mouseX) / tabW);

    if (index < 0 || static_cast<uint32_t>(index) >= group->tabCount) return result;

    result.tabIndex = index;
    result.zone = TabHitZone::Tab;

    // Check close button (only visible on hovered tab)
    if (index == hoverIndex) {
        float x = static_cast<float>(index) * tabW;
        float cbSize = theme.closeButtonSize;
        float cbX = x + tabW - theme.tabPadding - cbSize;
        float cbY = (barHeight - cbSize) * 0.5f;

        if (static_cast<float>(mouseX) >= cbX &&
            static_cast<float>(mouseX) <= cbX + cbSize &&
            static_cast<float>(mouseY) >= cbY &&
            static_cast<float>(mouseY) <= cbY + cbSize) {
            result.zone = TabHitZone::CloseButton;
        }
    }

    return result;
}
