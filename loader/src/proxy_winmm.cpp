// winmm.dll proxy entry point. Forwards the four imports Game.exe needs and
// hands off to the shared loader core.
//
// NOTE: this route only works if the application directory wins the DLL search
// order. If winmm.dll is already resident when the game's import is resolved
// (AppCompat shim, overlay injector, earlier dependency), the loader matches by
// base name and this file is never mapped. Use MadLauncher.exe in that case.
#include <windows.h>
#include <mmsystem.h>
#include "loader.h"

namespace {

HMODULE g_real = nullptr;
using PFN_get    = DWORD    (WINAPI*)();
using PFN_period = MMRESULT (WINAPI*)(UINT);
using PFN_caps   = MMRESULT (WINAPI*)(LPTIMECAPS, UINT);

PFN_get    p_timeGetTime     = nullptr;
PFN_period p_timeBeginPeriod = nullptr;
PFN_period p_timeEndPeriod   = nullptr;
PFN_caps   p_timeGetDevCaps  = nullptr;

void LoadReal() {
    char path[MAX_PATH];
    if (!GetSystemDirectoryA(path, MAX_PATH)) return;
    lstrcatA(path, "\\winmm.dll");
    g_real = LoadLibraryA(path);
    if (!g_real) return;
    p_timeGetTime     = (PFN_get)    GetProcAddress(g_real, "timeGetTime");
    p_timeBeginPeriod = (PFN_period) GetProcAddress(g_real, "timeBeginPeriod");
    p_timeEndPeriod   = (PFN_period) GetProcAddress(g_real, "timeEndPeriod");
    p_timeGetDevCaps  = (PFN_caps)   GetProcAddress(g_real, "timeGetDevCaps");
}

// Lazy: never LoadLibrary from DllMain (loader lock).
void EnsureReal() {
    static LONG once = 0;
    if (InterlockedCompareExchange(&once, 1, 0) == 0) LoadReal();
}

} // namespace

extern "C" {
DWORD WINAPI timeGetTime() {
    EnsureReal();
    return p_timeGetTime ? p_timeGetTime() : GetTickCount();
}
MMRESULT WINAPI timeBeginPeriod(UINT p) {
    EnsureReal();
    return p_timeBeginPeriod ? p_timeBeginPeriod(p) : TIMERR_NOCANDO;
}
MMRESULT WINAPI timeEndPeriod(UINT p) {
    EnsureReal();
    return p_timeEndPeriod ? p_timeEndPeriod(p) : TIMERR_NOCANDO;
}
MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS c, UINT s) {
    EnsureReal();
    return p_timeGetDevCaps ? p_timeGetDevCaps(c, s) : TIMERR_NOCANDO;
}
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        madloader::Start("winmm-proxy");
    }
    return TRUE;
}
