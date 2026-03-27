#pragma once

#include <cstdint>

struct Settings {
    int tabBarHeight = 32;
    int snapDistance = 30;
    bool enableAnimations = false;  // off by default for perf

    static Settings& Instance();
    void Load();
    void Save();
};
