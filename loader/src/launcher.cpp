// MadLauncher -- starts the game with MadLoaderCore.dll injected.
//
// The winmm.dll proxy route depends on the application directory winning the
// DLL search order. That fails whenever winmm.dll is already resident before
// the game's own import is resolved (an AppCompat shim, an overlay injector,
// or any earlier dependency), because the loader then matches by base name.
//
// Injection sidesteps all of that: create the process suspended, force our DLL
// in with a remote LoadLibraryA, then resume. Nothing depends on search order,
// and the game's winmm import resolves normally to System32.
//
// Usage:  MadLauncher.exe [path\to\Game.exe] [args...]
//         defaults to Game.exe next to the launcher.

#include <windows.h>
#include <cstdio>

namespace {

void Fail(const char* what) {
    char msg[512];
    wsprintfA(msg, "%s\r\n\r\nGetLastError = %lu", what, GetLastError());
    printf("%s\n", msg);
    MessageBoxA(nullptr, msg, "MadLauncher", MB_ICONERROR | MB_OK);
}

bool FileExists(const char* p) {
    const DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

} // namespace

int main(int argc, char** argv) {
    char dir[MAX_PATH];
    GetModuleFileNameA(nullptr, dir, MAX_PATH);
    char* slash = nullptr;
    for (char* p = dir; *p; ++p) if (*p == '\\') slash = p;
    if (slash) slash[1] = '\0';

    char game[MAX_PATH];
    if (argc > 1) {
        lstrcpynA(game, argv[1], MAX_PATH);
    } else {
        wsprintfA(game, "%sGame.exe", dir);
    }
    if (!FileExists(game)) {
        Fail("Game executable not found. Pass it as the first argument.");
        return 1;
    }

    char core[MAX_PATH];
    wsprintfA(core, "%sMadLoaderCore.dll", dir);
    if (!FileExists(core)) {
        Fail("MadLoaderCore.dll not found next to MadLauncher.exe.");
        return 1;
    }

    printf("game : %s\ncore : %s\n", game, core);

    // Working directory must be the game's own folder or it will not find its
    // data; the launcher may live elsewhere.
    char workdir[MAX_PATH];
    lstrcpynA(workdir, game, MAX_PATH);
    slash = nullptr;
    for (char* p = workdir; *p; ++p) if (*p == '\\') slash = p;
    if (slash) *slash = '\0';

    // Rebuild a command line, forwarding any extra arguments.
    char cmd[2048];
    wsprintfA(cmd, "\"%s\"", game);
    for (int i = 2; i < argc; ++i) {
        lstrcatA(cmd, " ");
        lstrcatA(cmd, argv[i]);
    }

    STARTUPINFOA si = { sizeof si };
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE,
                        CREATE_SUSPENDED, nullptr, workdir, &si, &pi)) {
        Fail("CreateProcess failed.");
        return 1;
    }

    bool ok = false;
    do {
        const SIZE_T n = (SIZE_T)lstrlenA(core) + 1;
        void* remote = VirtualAllocEx(pi.hProcess, nullptr, n,
                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remote) { Fail("VirtualAllocEx failed."); break; }

        if (!WriteProcessMemory(pi.hProcess, remote, core, n, nullptr)) {
            Fail("WriteProcessMemory failed."); break;
        }

        // kernel32 is mapped at the same address in every process of a session,
        // so our LoadLibraryA is valid in the target.
        HMODULE k32 = GetModuleHandleA("kernel32.dll");
        FARPROC loadLib = k32 ? GetProcAddress(k32, "LoadLibraryA") : nullptr;
        if (!loadLib) { Fail("Could not resolve LoadLibraryA."); break; }

        HANDLE th = CreateRemoteThread(pi.hProcess, nullptr, 0,
                                       (LPTHREAD_START_ROUTINE)loadLib,
                                       remote, 0, nullptr);
        if (!th) { Fail("CreateRemoteThread failed."); break; }

        WaitForSingleObject(th, 10000);
        DWORD remoteModule = 0;
        GetExitCodeThread(th, &remoteModule);
        CloseHandle(th);
        VirtualFreeEx(pi.hProcess, remote, 0, MEM_RELEASE);

        if (remoteModule == 0) {
            Fail("Remote LoadLibraryA returned NULL -- the core DLL failed to load.");
            break;
        }
        printf("injected MadLoaderCore.dll -> 0x%08lX\n", remoteModule);
        ok = true;
    } while (false);

    if (!ok) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 1;
    }

    ResumeThread(pi.hThread);
    printf("resumed pid %lu\n", pi.dwProcessId);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
