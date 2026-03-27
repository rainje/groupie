cmake_minimum_required(VERSION 3.20)
project(Groupie LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimize for speed in Release
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /GL /DNDEBUG /GS-")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /OPT:REF /OPT:ICF")
endif()

add_executable(groupie WIN32
    src/main.cpp
    src/core/app.cpp
    src/core/window_monitor.cpp
    src/core/group_manager.cpp
    src/core/tab_group.cpp
    src/core/drag_detector.cpp
    src/ui/tab_bar_window.cpp
    src/ui/renderer.cpp
    src/ui/tab_renderer.cpp
    src/ui/snap_indicator.cpp
    src/ui/theme.cpp
    src/input/mouse_hook.cpp
    src/input/keyboard_hook.cpp
    src/util/icon_cache.cpp
    src/util/taskbar.cpp
    src/config/settings.cpp
    res/groupie.rc
)

target_include_directories(groupie PRIVATE src)

target_link_libraries(groupie PRIVATE
    d2d1
    dwrite
    dwmapi
    user32
    gdi32
    shell32
    ole32
    shlwapi
    windowscodecs
)

target_compile_definitions(groupie PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# Embed manifest for DPI awareness
set_target_properties(groupie PROPERTIES
    LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/res/groupie.manifest"
)
