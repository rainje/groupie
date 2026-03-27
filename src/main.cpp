#include <windows.h>
#include <objbase.h>
#include "core/app.h"
#include "core/group_manager.h"
#include "util/log.h"

static HANDLE g_mutex = nullptr;

static bool EnsureSingleInstance() {
    g_mutex = CreateMutexW(nullptr, TRUE, L"GroupieWindowTabManager_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        LOG_INFO(L"Another instance is already running");
        return false;
    }
    return g_mutex != nullptr;
}

// Called when the process is being terminated (Ctrl+C, taskkill, logoff, etc.)
static BOOL WINAPI ConsoleCtrlHandler(DWORD) {
    GroupManager::Instance().UngroupAll();
    return FALSE;  // let default handler run
}

static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS*) {
    GroupManager::Instance().UngroupAll();
    return EXCEPTION_CONTINUE_SEARCH;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    if (!EnsureSingleInstance()) return 1;

    // Install cleanup handlers for unexpected termination
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    SetUnhandledExceptionFilter(UnhandledExceptionHandler);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LOG_INFO(L"COM initialization failed: 0x%08X", hr);
        return 1;
    }

    int exitCode = 1;
    {
        App& app = App::Instance();
        if (app.Init(hInstance)) {
            exitCode = app.Run();
            app.Shutdown();
        }
    }

    CoUninitialize();

    if (g_mutex) {
        ReleaseMutex(g_mutex);
        CloseHandle(g_mutex);
    }

    return exitCode;
}
