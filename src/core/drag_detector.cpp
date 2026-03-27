#include "core/drag_detector.h"
#include "core/group_manager.h"
#include "core/app.h"
#include "ui/snap_indicator.h"
#include "util/win32_helpers.h"
#include "util/log.h"

DragDetector& DragDetector::Instance() {
    static DragDetector instance;
    return instance;
}

void DragDetector::StartSnapTimer() {
    if (!timerId_) {
        // Use the App's message window for the timer
        timerId_ = SetTimer(App::Instance().GetMessageWindow(), kSnapTimerId, kSnapDelayMs, nullptr);
    }
}

void DragDetector::StopSnapTimer() {
    if (timerId_) {
        KillTimer(App::Instance().GetMessageWindow(), kSnapTimerId);
        timerId_ = 0;
    }
}

void DragDetector::OnSnapTimer() {
    if (state_ != DragState::Snapping || !targetHwnd_) return;

    DWORD elapsed = GetTickCount() - snapEnterTime_;
    if (elapsed >= kSnapDelayMs) {
        state_ = DragState::Armed;
        armedTarget_ = targetHwnd_;
        SnapIndicator::Instance().SetArmed(true);
        StopSnapTimer();
        LOG_INFO(L"Snap armed after %dms (timer)", elapsed);
    }
}

void DragDetector::OnMoveStart(HWND hwnd) {
    if (GroupManager::Instance().IsOwnWindow(hwnd)) return;
    if (!IsTopLevelAppWindow(hwnd)) return;

    state_ = DragState::Tracking;
    sourceHwnd_ = hwnd;
    targetHwnd_ = nullptr;
    lastSnapTarget_ = nullptr;
    armedTarget_ = nullptr;
    snapEnterTime_ = 0;
    StopSnapTimer();
    LOG_INFO(L"Drag started: HWND %p", hwnd);
}

void DragDetector::OnMoveEnd(HWND hwnd) {
    if (state_ == DragState::Idle) return;

    SnapIndicator::Instance().Hide();
    StopSnapTimer();

    LOG_INFO(L"Drag ended: state=%d source=%p target=%p armedTarget=%p",
        static_cast<int>(state_), sourceHwnd_, targetHwnd_, armedTarget_);

    HWND finalTarget = nullptr;
    if (state_ == DragState::Armed) {
        finalTarget = targetHwnd_;
    }
    if (!finalTarget) {
        finalTarget = armedTarget_;
    }

    if (finalTarget && sourceHwnd_ && finalTarget != sourceHwnd_) {
        if (IsWindow(finalTarget)) {
            LOG_INFO(L"Creating group: %p -> %p", sourceHwnd_, finalTarget);
            GroupManager::Instance().CreateGroup(sourceHwnd_, finalTarget);
        }
    }

    state_ = DragState::Idle;
    sourceHwnd_ = nullptr;
    targetHwnd_ = nullptr;
    lastSnapTarget_ = nullptr;
    armedTarget_ = nullptr;
    snapEnterTime_ = 0;
}

void DragDetector::OnMouseMove(POINT pt) {
    if (state_ == DragState::Idle) return;

    if ((state_ == DragState::Snapping || state_ == DragState::Armed) && targetHwnd_) {
        if (IsInSnapZone(pt, targetHwnd_, kSnapLeaveDistance)) {
            if (state_ == DragState::Snapping) {
                DWORD elapsed = GetTickCount() - snapEnterTime_;
                if (elapsed >= kSnapDelayMs) {
                    state_ = DragState::Armed;
                    armedTarget_ = targetHwnd_;
                    SnapIndicator::Instance().SetArmed(true);
                    StopSnapTimer();
                    LOG_INFO(L"Snap armed after %dms", elapsed);
                }
            }
            SnapIndicator::Instance().Show(targetHwnd_);
            return;
        }
        // Left the extended snap zone — cancel snap entirely
        LOG_INFO(L"Snap zone left");
        SnapIndicator::Instance().Hide();
        StopSnapTimer();
        state_ = DragState::Tracking;
        targetHwnd_ = nullptr;
        armedTarget_ = nullptr;
        snapEnterTime_ = 0;
    }

    HWND target = FindSnapTarget(pt);
    if (target && target != sourceHwnd_) {
        if (targetHwnd_ != target) {
            LOG_INFO(L"Snap zone entered: target %p", target);
            snapEnterTime_ = GetTickCount();
            StartSnapTimer();
        }
        state_ = DragState::Snapping;
        targetHwnd_ = target;
        lastSnapTarget_ = target;
        SnapIndicator::Instance().Show(target);
    }
}

bool DragDetector::IsInSnapZone(POINT cursor, HWND target, int distance) const {
    RECT rc = GetExtendedFrameBounds(target);
    return cursor.x >= rc.left - distance && cursor.x <= rc.right + distance
        && cursor.y >= rc.top - distance && cursor.y <= rc.top + distance;
}

HWND DragDetector::FindSnapTarget(POINT cursor) const {
    HWND firstUnderCursor = nullptr;
    HWND candidate = sourceHwnd_;

    while ((candidate = GetWindow(candidate, GW_HWNDNEXT)) != nullptr) {
        if (candidate == sourceHwnd_) continue;
        if (GroupManager::Instance().IsOwnWindow(candidate)) continue;
        if (!IsWindowVisible(candidate)) continue;

        RECT rc;
        GetWindowRect(candidate, &rc);

        if (cursor.x < rc.left || cursor.x > rc.right) continue;
        if (cursor.y < rc.top - kSnapEnterDistance || cursor.y > rc.bottom) continue;

        if (!IsTopLevelAppWindow(candidate)) continue;

        if (!firstUnderCursor) {
            firstUnderCursor = candidate;
        }

        if (candidate != firstUnderCursor) return nullptr;

        if (IsInSnapZone(cursor, candidate, kSnapEnterDistance)) return candidate;

        return nullptr;
    }
    return nullptr;
}
