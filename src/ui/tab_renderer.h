#pragma once

#include <d2d1_1.h>

struct TabGroup;

enum class TabHitZone {
    None,
    Tab,
    CloseButton
};

struct TabHitResult {
    int tabIndex = -1;
    TabHitZone zone = TabHitZone::None;
};

class TabRenderer {
public:
    // Returns false if render target needs recreation
    static bool Paint(ID2D1HwndRenderTarget* rt, TabGroup* group, int hoverIndex);
    static int HitTest(TabGroup* group, int mouseX);
    static TabHitResult HitTestEx(TabGroup* group, int mouseX, int mouseY, int hoverIndex);

private:
    static float CalculateTabWidth(TabGroup* group, float totalWidth);
};
