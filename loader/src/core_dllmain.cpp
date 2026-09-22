// MadLoaderCore.dll -- the loader as an injectable DLL (no winmm exports).
// Injected by MadLauncher.exe; immune to DLL search-order problems.
#include <windows.h>
#include "loader.h"

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        madloader::Start("injected");
    }
    return TRUE;
}
