// Shared patch-loading core. Used by both winmm.dll (proxy) and
// MadLoaderCore.dll (injected by MadLauncher.exe).

#include "loader.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>

namespace madloader {
namespace {

char g_exeDir[MAX_PATH]  = {};
char g_logPath[MAX_PATH] = {};
char g_how[64]           = {};

// Optional entry point a patch may export; called after its DllMain returns.
typedef void (__cdecl *PFN_MadPatchInit)(void);

void ComputePaths() {
    GetModuleFileNameA(nullptr, g_exeDir, MAX_PATH);
    char* slash = nullptr;
    for (char* p = g_exeDir; *p; ++p) if (*p == '\\') slash = p;
    if (slash) slash[1] = '\0';
    wsprintfA(g_logPath, "%sMadLoader.log", g_exeDir);
}

int LoadPatches() {
    char dir[MAX_PATH];
    wsprintfA(dir, "%spatches", g_exeDir);

    const DWORD attr = GetFileAttributesA(dir);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        Log("[MadLoader] no patches folder at %s -- nothing to load", dir);
        return 0;
    }

    char pattern[MAX_PATH];
    wsprintfA(pattern, "%s\\*.dll", dir);

    WIN32_FIND_DATAA fd;
    HANDLE find = FindFirstFileA(pattern, &fd);
    if (find == INVALID_HANDLE_VALUE) {
        Log("[MadLoader] patches folder is empty");
        return 0;
    }

    int loaded = 0, failed = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (fd.cFileName[0] == '_') {   // leading underscore disables a patch
            Log("[MadLoader] skip     %s (disabled by leading underscore)", fd.cFileName);
            continue;
        }

        char full[MAX_PATH];
        wsprintfA(full, "%s\\%s", dir, fd.cFileName);

        // LOAD_WITH_ALTERED_SEARCH_PATH lets a patch ship its own dependencies
        // alongside it inside patches\.
        HMODULE h = LoadLibraryExA(full, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!h) {
            Log("[MadLoader] FAILED   %s (err=%lu)", fd.cFileName, GetLastError());
            ++failed;
            continue;
        }
        Log("[MadLoader] loaded   %s -> %p", fd.cFileName, (void*)h);
        ++loaded;

        if (auto init = (PFN_MadPatchInit)GetProcAddress(h, "MadPatchInit")) {
            Log("[MadLoader]            calling MadPatchInit()");
            init();
        }
    } while (FindNextFileA(find, &fd));

    FindClose(find);
    Log("[MadLoader] done: %d loaded, %d failed", loaded, failed);
    return loaded;
}

void RunLoad() {
    Log("[MadLoader] ---- entry=%s exe=%s pid=%lu ----",
        g_how, g_exeDir, GetCurrentProcessId());

    // Report whether a System32 winmm was already resident. When the proxy
    // route silently fails, this is usually why: something loaded winmm.dll
    // before the game's own import was resolved, and the loader then matches
    // by base name and never looks in the application directory.
    if (HMODULE w = GetModuleHandleA("winmm.dll")) {
        char p[MAX_PATH] = {};
        GetModuleFileNameA(w, p, MAX_PATH);
        Log("[MadLoader] resident winmm.dll = %s", p);
    } else {
        Log("[MadLoader] winmm.dll not resident at this point");
    }

    LoadPatches();
}

} // namespace

void Log(const char* fmt, ...) {
    char body[1024];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf_s(body, sizeof body, _TRUNCATE, fmt, ap);
    va_end(ap);

    char line[1200];
    wsprintfA(line, "%s\r\n", body);
    OutputDebugStringA(line);

    if (!g_logPath[0]) return;
    HANDLE h = CreateFileA(g_logPath, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD w = 0;
    WriteFile(h, line, (DWORD)lstrlenA(line), &w, nullptr);
    CloseHandle(h);
}

void Start(const char* how) {
    lstrcpynA(g_how, how ? how : "?", sizeof g_how);
    ComputePaths();
    DeleteFileA(g_logPath);

    // SYNCHRONOUS, on purpose.
    //
    // This previously ran on a worker thread to avoid calling LoadLibrary
    // under the loader lock. But that races the main thread: a patch that has
    // to rewrite an instruction immediate inside WinMain must land before
    // WinMain executes it, and the worker frequently lost that race. The
    // observed symptom was a half-applied patch -- MadWindowed's viewportFlags
    // write (late in WinMain) landing while its g_bWindowedMode write (early in
    // WinMain) missed, leaving a fullscreen window around a windowed device,
    // which crashes on the first frame.
    //
    // Loading inline here means patches are in place before the CRT calls
    // WinMain. This is what ASI-style loaders have always done. The cost is the
    // loader-lock caveat: a patch DLL must not call LoadLibrary or do
    // reentrant work in its own DllMain.
    RunLoad();
}

} // namespace madloader
