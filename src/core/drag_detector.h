#pragma once

#include <windows.h>

enum class DragState {
    Idle,
    Tracking,
    Snapping,   // in zone but not yet armed
    Armed       // held long enough, will group on release
};

class DragDetector {
public:
    static DragDetector& Instance();

    void OnMoveStart(HWND hwnd);
    void OnMoveEnd(HWND hwnd);
    void OnMouseMove(POINT pt);
    void OnSnapTimer();

    DragState GetState() const { return state_; }
    HWND GetSourceWindow() const { return sourceHwnd_; }
    HWND GetTargetWindow() const { return targetHwnd_; }

    void StartSnapTimer();
    void StopSnapTimer();

private:
    DragDetector() = default;

    bool IsInSnapZone(POINT cursor, HWND target, int distance) const;
    HWND FindSnapTarget(POINT cursor) const;

    DragState state_ = DragState::Idle;
    HWND sourceHwnd_ = nullptr;
    HWND targetHwnd_ = nullptr;
    HWND lastSnapTarget_ = nullptr;
    HWND armedTarget_ = nullptr;
    DWORD snapEnterTime_ = 0;
    UINT_PTR timerId_ = 0;

    static constexpr int kSnapEnterDistance = 40;
    static constexpr int kSnapLeaveDistance = 80;
    static constexpr DWORD kSnapDelayMs = 400;
    static constexpr UINT_PTR kSnapTimerId = 42;
};
