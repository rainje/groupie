#pragma once

#include <d2d1.h>

struct Theme {
    D2D1_COLOR_F background     = {0.15f, 0.15f, 0.15f, 1.0f};  // dark gray
    D2D1_COLOR_F activeTab      = {0.25f, 0.25f, 0.28f, 1.0f};  // lighter gray
    D2D1_COLOR_F inactiveTab    = {0.18f, 0.18f, 0.20f, 1.0f};  // slightly lighter than bg
    D2D1_COLOR_F hoverTab       = {0.22f, 0.22f, 0.25f, 1.0f};
    D2D1_COLOR_F text           = {0.90f, 0.90f, 0.90f, 1.0f};  // light text
    D2D1_COLOR_F inactiveText   = {0.60f, 0.60f, 0.60f, 1.0f};
    D2D1_COLOR_F separator      = {0.30f, 0.30f, 0.32f, 1.0f};
    D2D1_COLOR_F closeHover     = {0.80f, 0.20f, 0.20f, 1.0f};  // red

    float tabMinWidth = 100.0f;
    float tabMaxWidth = 250.0f;
    float tabPadding = 10.0f;
    float iconSize = 16.0f;
    float closeButtonSize = 14.0f;
    float cornerRadius = 4.0f;

    static const Theme& Default();
};
