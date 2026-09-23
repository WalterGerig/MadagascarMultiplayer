#include <windows.h>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <thread>

#include "../include/MemoryManager.h"
#include "../include/PlayerTransform.h"
#include "../include/D3D8Hook.h"
#include "../include/Logger.h"
#include "../include/Config.h"

namespace {

    HMODULE g_hModule = nullptr;
    HANDLE  g_hWorkerThread = nullptr;
    std::atomic<bool> g_running{ false };

    // Globaler thread-sicherer Transform-Speicher für Netzwerk & Rendering
    MadMultiplayer::ThreadSafeTransform g_localTransformStore;

    void SetupDebugConsole() {
        MadMultiplayer::Logger::InitConsole();
        printf("==================================================================\n");
        printf(" MADAGASCAR MULTIPLAYER CLIENT DLL INITIALIZED\n");
        printf(" Target Process: Game.exe (German Retail)\n");
        printf(" DirectX 8 Hook: EndScene & Reset VTable Hook Active\n");
        printf(" UI Backend:     Dear ImGui\n");
        printf(" Config File:    patches/multiplayer_config.ini\n");
        printf(" Log File:       multiplayer_debug.log\n");
        printf("==================================================================\n\n");
    }

    void CloseDebugConsole() {
        printf("[MadMultiplayer] Schließe Debug-Konsole...\n");
        FreeConsole();
    }

    DWORD WINAPI MultiplayerWorkerThread(LPVOID lpParam) {
        MAD_LOG("[WorkerThread] Worker-Thread gestartet (TID: %lu)", GetCurrentThreadId());

        // 1. D3D8 Hook & ImGui verknüpfen (Sicheres asynchrones Warten auf Spielfenster)
        auto& d3dHook = MadMultiplayer::D3D8Hook::Instance();
        d3dHook.SetTransformStore(&g_localTransformStore);
        d3dHook.InitializeAsync();

        // 2. Memory Subsystem initialisieren
        auto& mem = MadMultiplayer::MemoryManager::Instance();
        if (!mem.Initialize()) {
            MAD_LOG("[WorkerThread] FEHLER: MemoryManager konnte nicht initialisiert werden!");
            return 1;
        }

        MAD_LOG("[WorkerThread] Starte 60 Hz Polling-Loop (~16.6ms Intervall)...");

        MadMultiplayer::PlayerTransform localState{};
        uint32_t tickCounter = 0;

        while (g_running.load()) {
            auto startTime = std::chrono::steady_clock::now();

            // -----------------------------------------------------------------
            // 60 HZ LOCAL PLAYER MEMORY READING
            // -----------------------------------------------------------------
            if (mem.ReadLocalPlayer(localState)) {
                g_localTransformStore.Set(localState);

                if (tickCounter % 30 == 0) {
                    printf("\r[60Hz Ticks: %6u] Pos: X=%8.2f Y=%8.2f Z=%8.2f | Yaw: %5.2f | Status: %s   ",
                           tickCounter,
                           localState.x, localState.y, localState.z,
                           localState.yaw,
                           localState.isPaused ? "PAUSE" : "INGAME");
                    fflush(stdout);
                }
            } else {
                if (tickCounter % 60 == 0) {
                    printf("\r[WorkerThread] Warte auf Spieler-Entity (Hauptmenü/Ladebildschirm)...             ");
                    fflush(stdout);
                }
            }

            tickCounter++;

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime
            );
            if (elapsed.count() < 16) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed.count()));
            }
        }

        MAD_LOG("[WorkerThread] Worker-Thread beendet.");
        mem.Shutdown();
        return 0;
    }

} // namespace

extern "C" __declspec(dllexport) void __cdecl MadPatchInit(void) {
    MAD_LOG("[MadMultiplayer] MadPatchInit() von MadLoader aufgerufen.");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        // Dedicated Win32 Debug-Konsole bedingungslos ganz am Anfang initialisieren
        MadMultiplayer::Logger::InitConsole();
        MadMultiplayer::Logger::Instance().Init();

        // 1. Config System initialisieren
        MadMultiplayer::Config::Instance().Initialize(hModule);

        // 2. File Logger initialisieren (falls konfiguriert)
        if (MadMultiplayer::Config::Instance().Get().enableFileLogging) {
            MadMultiplayer::Logger::Instance().Initialize();
        }

        MAD_LOG("[DllMain] DLL_PROCESS_ATTACH -> MadMultiplayer.dll [0.6.5-MERGED-ALL-FEATURES] geladen.");
        MAD_LOG("[DllMain] Config initialisiert: %s", MadMultiplayer::Config::Instance().GetIniPath().c_str());

        SetupDebugConsole();

        g_running.store(true);
        g_hWorkerThread = CreateThread(nullptr, 0, MultiplayerWorkerThread, nullptr, 0, nullptr);
        if (!g_hWorkerThread) {
            MAD_LOG("[MadMultiplayer] FEHLER: CreateThread fehlgeschlagen (Error: %lu)", GetLastError());
            return FALSE;
        }
        break;
    }

    case DLL_PROCESS_DETACH: {
        MAD_LOG("[DllMain] DLL_PROCESS_DETACH signalisiert...");
        g_running.store(false);

        if (g_hWorkerThread) {
            WaitForSingleObject(g_hWorkerThread, 2000);
            CloseHandle(g_hWorkerThread);
            g_hWorkerThread = nullptr;
        }

        MadMultiplayer::D3D8Hook::Instance().Shutdown();
        CloseDebugConsole();
        MadMultiplayer::Logger::Instance().Shutdown();
        break;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void __cdecl MadMultiplayer_ForceHookDevice(IDirect3DDevice8* pDevice, HWND hWnd) {
    MAD_LOG("[MadMultiplayer] SOFORTIGER CALLBACK MadMultiplayer_ForceHookDevice: Device=%p, HWND=%p", pDevice, (void*)hWnd);
    MadMultiplayer::D3D8Hook::Instance().OnDeviceCreated(pDevice, hWnd);
}

extern "C" __declspec(dllexport) void __cdecl MadMultiplayer_OnDeviceCreated(void* pDevice, HWND hWnd) {
    MAD_LOG("[MadMultiplayer] Direkter Callback erhalten von proxy_d3d8: Device=%p, HWND=%p", pDevice, (void*)hWnd);
    MadMultiplayer::D3D8Hook::Instance().OnDeviceCreated(reinterpret_cast<IDirect3DDevice8*>(pDevice), hWnd);
}

