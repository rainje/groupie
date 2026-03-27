# Groupie

A native Windows application that groups any window into tabs, similar to Stardock's Groupy. Written in C++20 with raw Win32 API for maximum performance.

![Windows](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![C++20](https://img.shields.io/badge/language-C%2B%2B20-orange)
![License](https://img.shields.io/badge/license-MIT-green)

## Features

- **Drag-to-tab**: Drag a window near another window's top edge to group them into tabs
- **Tab bar**: Dark-themed tab bar with window icons, titles, hover effects, and close buttons
- **Tab switching**: Click tabs to switch between grouped windows
- **Tab reordering**: Drag tabs to reorder them within the group
- **Group dragging**: Drag the tab bar's empty area to move the entire group
- **Middle-click close**: Middle-click a tab to ungroup it
- **Taskbar integration**: Only the active tab appears in the Windows taskbar
- **Multi-group**: Create multiple independent groups simultaneously
- **Snap indicator**: Visual blue/green indicator shows snap zones during drag
- **System tray**: Tray icon with "Ungroup All" and "Exit" options
- **DPI aware**: Per-monitor DPI v2 support
- **Safe shutdown**: All windows are restored when Groupie exits

## How It Works

Groupie uses an external window management approach with zero DLL injection:

1. `SetWinEventHook(WINEVENT_OUTOFCONTEXT)` monitors all windows system-wide without affecting target processes
2. A low-level mouse hook (`WH_MOUSE_LL`) detects drag-to-tab gestures
3. Tab bars are lightweight Direct2D overlay windows (`WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`)
4. Window ownership (`GWLP_HWNDPARENT`) ensures tab bars stay above their group

## Building

Requires:
- Windows 10/11
- CMake 3.20+
- Visual Studio 2022 (Build Tools or full IDE)

```bash
# Configure
cmake -B build -G "Visual Studio 17 2022"

# Build Debug
cmake --build build --config Debug

# Build Release (optimized: /O2 /GL /LTCG)
cmake --build build --config Release

# Run
./build/Release/groupie.exe
```

## Usage

1. Launch `groupie.exe` (appears in system tray)
2. Drag a window by its title bar toward the top edge of another window
3. A blue indicator appears; hold for ~400ms until it turns green
4. Release to create a tab group
5. Click tabs to switch, middle-click to ungroup, drag tabs to reorder
6. Right-click tray icon to "Ungroup All" or "Exit"

## Architecture

```
src/
  core/       Window monitoring, group management, drag detection
  ui/         Tab bar window, Direct2D rendering, theme, snap indicator
  input/      Mouse hook, keyboard shortcuts
  util/       COM helpers, DWM helpers, icon cache, taskbar, logging
  config/     Settings (registry-backed)
```

## Performance

- **345 KB** Release binary (no runtime dependencies beyond Windows)
- Zero heap allocation in event callbacks and mouse hook
- Event-driven architecture (no polling)
- Direct2D hardware-accelerated rendering with `D2D1_PRESENT_OPTIONS_IMMEDIATELY`
- Dirty-flag rendering (repaint only when needed)
- `MsgWaitForMultipleObjectsEx` message loop (blocks instead of spinning)

## License

MIT License. See [LICENSE](LICENSE).
