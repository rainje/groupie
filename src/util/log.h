#pragma once

#include <windows.h>
#include <cstdio>
#include <cstdlib>

// File-based logging for debugging without a debugger
namespace Log {
    inline FILE* GetFile() {
        static FILE* f = nullptr;
        if (!f) {
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(nullptr, path, MAX_PATH);
            // Replace .exe with .log
            wchar_t* dot = wcsrchr(path, L'.');
            if (dot) wcscpy_s(dot, 5, L".log");
            f = _wfopen(path, L"w");
            if (f) setbuf(f, nullptr); // unbuffered
        }
        return f;
    }

    inline void Write(const wchar_t* msg) {
        OutputDebugStringW(msg);
        FILE* f = GetFile();
        if (f) {
            fwprintf(f, L"%s", msg);
        }
    }
}

#define LOG_INFO(fmt, ...) do { \
    wchar_t buf_[512]; \
    _snwprintf_s(buf_, _countof(buf_), _TRUNCATE, L"[Groupie] " fmt L"\n", ##__VA_ARGS__); \
    Log::Write(buf_); \
} while(0)

#ifdef NDEBUG
#define LOG_DEBUG(fmt, ...)
#else
#define LOG_DEBUG(fmt, ...) LOG_INFO(fmt, ##__VA_ARGS__)
#endif
