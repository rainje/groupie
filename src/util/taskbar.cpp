#include "util/taskbar.h"
#include "util/log.h"

Taskbar& Taskbar::Instance() {
    static Taskbar instance;
    return instance;
}

bool Taskbar::Init() {
    HRESULT hr = CoCreateInstance(
        CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
        IID_ITaskbarList, reinterpret_cast<void**>(taskbar_.ReleaseAndGetAddressOf())
    );
    if (FAILED(hr)) {
        LOG_INFO(L"CoCreateInstance TaskbarList failed: 0x%08X", hr);
        return false;
    }

    hr = taskbar_->HrInit();
    if (FAILED(hr)) {
        LOG_INFO(L"ITaskbarList::HrInit failed: 0x%08X", hr);
        taskbar_.Release();
        return false;
    }

    LOG_INFO(L"Taskbar manager initialized");
    return true;
}

void Taskbar::Shutdown() {
    taskbar_.Release();
}

void Taskbar::HideButton(HWND hwnd) {
    if (taskbar_ && hwnd) {
        taskbar_->DeleteTab(hwnd);
    }
}

void Taskbar::ShowButton(HWND hwnd) {
    if (taskbar_ && hwnd) {
        taskbar_->AddTab(hwnd);
    }
}
