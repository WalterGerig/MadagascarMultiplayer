#include "../include/D3D8Hook.h"
#include "../include/Logger.h"
#include "../include/Config.h"
#include "../include/CheatManager.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_win32.h"
#include "../vendor/imgui/imgui_impl_dx8.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL ((HRESULT)0x8876086CL)
#endif

namespace MadMultiplayer {

    bool g_bUIModeActive = false;

    void ToggleUIMode() {
        D3D8Hook::Instance().ToggleUIMode();
    }

    void SetUIMode(bool active) {
        D3D8Hook::Instance().SetUIMode(active);
    }

    bool IsUIModeActive() {
        return g_bUIModeActive;
    }

    bool g_bOverlayVisible = true;

    void ToggleOverlay() {
        D3D8Hook::Instance().ToggleOverlayVisibility();
    }

    void SetOverlayVisible(bool visible) {
        g_bOverlayVisible = visible;
        D3D8Hook::Instance().SetOverlayVisibility(visible);
    }

    bool IsOverlayVisible() {
        return g_bOverlayVisible;
    }

    static bool g_bRenderedThisFrame = false;

    void D3D8Hook::SaveD3D8State(IDirect3DDevice8* pDevice, D3D8StateBackup& b) {
        if (!pDevice) return;

        // 1. Render States
        pDevice->GetRenderState(D3DRS_ZENABLE, &b.zEnable);
        pDevice->GetRenderState(D3DRS_FILLMODE, &b.fillMode);
        pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &b.alphaBlend);
        pDevice->GetRenderState(D3DRS_SRCBLEND, &b.srcBlend);
        pDevice->GetRenderState(D3DRS_DESTBLEND, &b.destBlend);
        pDevice->GetRenderState(D3DRS_CULLMODE, &b.cullMode);
        pDevice->GetRenderState(D3DRS_LIGHTING, &b.lighting);
        pDevice->GetRenderState(D3DRS_FOGENABLE, &b.fogEnable);
        pDevice->GetRenderState(D3DRS_ALPHATESTENABLE, &b.alphaTest);

        // 2. Texture Stage States Stage 0 & 1
        pDevice->GetTextureStageState(0, D3DTSS_COLOROP, &b.colorOp0);
        pDevice->GetTextureStageState(0, D3DTSS_ALPHAOP, &b.alphaOp0);
        pDevice->GetTextureStageState(0, D3DTSS_COLORARG1, &b.colorArg1_0);
        pDevice->GetTextureStageState(0, D3DTSS_COLORARG2, &b.colorArg2_0);
        pDevice->GetTextureStageState(0, D3DTSS_ALPHAARG1, &b.alphaArg1_0);
        pDevice->GetTextureStageState(0, D3DTSS_ALPHAARG2, &b.alphaArg2_0);
        pDevice->GetTextureStageState(0, D3DTSS_MINFILTER, &b.minFilter0);
        pDevice->GetTextureStageState(0, D3DTSS_MAGFILTER, &b.magFilter0);

        pDevice->GetTextureStageState(1, D3DTSS_COLOROP, &b.colorOp1);
        pDevice->GetTextureStageState(1, D3DTSS_ALPHAOP, &b.alphaOp1);
        pDevice->GetTextureStageState(1, D3DTSS_MINFILTER, &b.minFilter1);
        pDevice->GetTextureStageState(1, D3DTSS_MAGFILTER, &b.magFilter1);

        // 3. Shaders, Vertex Buffers & Indices
        pDevice->GetVertexShader(&b.vertexShader);
        b.streamSource0 = nullptr;
        b.streamStride0 = 0;
        pDevice->GetStreamSource(0, &b.streamSource0, &b.streamStride0);
        b.indexBuffer = nullptr;
        b.baseVertexIndex = 0;
        pDevice->GetIndices(&b.indexBuffer, &b.baseVertexIndex);

        // 4. Textures
        b.texture0 = nullptr;
        b.texture1 = nullptr;
        pDevice->GetTexture(0, &b.texture0);
        pDevice->GetTexture(1, &b.texture1);

        // 5. Viewport
        pDevice->GetViewport(&b.viewport);
    }

    void D3D8Hook::RestoreD3D8State(IDirect3DDevice8* pDevice, const D3D8StateBackup& b) {
        if (!pDevice) return;

        // 1. Render States
        pDevice->SetRenderState(D3DRS_ZENABLE, b.zEnable);
        pDevice->SetRenderState(D3DRS_FILLMODE, b.fillMode);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, b.alphaBlend);
        pDevice->SetRenderState(D3DRS_SRCBLEND, b.srcBlend);
        pDevice->SetRenderState(D3DRS_DESTBLEND, b.destBlend);
        pDevice->SetRenderState(D3DRS_CULLMODE, b.cullMode);
        pDevice->SetRenderState(D3DRS_LIGHTING, b.lighting);
        pDevice->SetRenderState(D3DRS_FOGENABLE, b.fogEnable);
        pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, b.alphaTest);

        // 2. Texture Stage States Stage 0 & 1
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, b.colorOp0);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, b.alphaOp0);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, b.colorArg1_0);
        pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, b.colorArg2_0);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, b.alphaArg1_0);
        pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, b.alphaArg2_0);
        pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, b.minFilter0);
        pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, b.magFilter0);

        pDevice->SetTextureStageState(1, D3DTSS_COLOROP, b.colorOp1);
        pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, b.alphaOp1);
        pDevice->SetTextureStageState(1, D3DTSS_MINFILTER, b.minFilter1);
        pDevice->SetTextureStageState(1, D3DTSS_MAGFILTER, b.magFilter1);

        // 3. Shaders, Vertex Buffers & Indices
        pDevice->SetVertexShader(b.vertexShader);
        pDevice->SetStreamSource(0, b.streamSource0, b.streamStride0);
        if (b.streamSource0) b.streamSource0->Release();

        pDevice->SetIndices(b.indexBuffer, b.baseVertexIndex);
        if (b.indexBuffer) b.indexBuffer->Release();

        // 4. Textures
        pDevice->SetTexture(0, b.texture0);
        if (b.texture0) b.texture0->Release();

        pDevice->SetTexture(1, b.texture1);
        if (b.texture1) b.texture1->Release();

        // 5. Viewport
        pDevice->SetViewport(&b.viewport);
    }

    D3D8Hook::PFN_GetCursorPos D3D8Hook::s_pOriginalGetCursorPos = nullptr;

    D3D8Hook& D3D8Hook::Instance() {
        static D3D8Hook instance;
        return instance;
    }

    D3D8Hook::~D3D8Hook() {
        Shutdown();
    }

    void D3D8Hook::UnlockMouseCursor() {
        ::ClipCursor(nullptr);
        ::SetCursor(::LoadCursorA(nullptr, MAKEINTRESOURCEA(32512))); // IDC_ARROW
    }

    // F3: Overlay-Sichtbarkeit ein-/ausblenden
    void D3D8Hook::ToggleOverlayVisibility() {
        static DWORD s_lastF3Tick = 0;
        DWORD now = GetTickCount();
        if (now - s_lastF3Tick < 150) return;
        s_lastF3Tick = now;

        g_bOverlayVisible = !g_bOverlayVisible;
        m_showOverlay.store(g_bOverlayVisible);
        MAD_LOG("[D3D8Hook] Overlay Sichtbarkeit geaendert -> %s (Taste F3)", g_bOverlayVisible ? "SICHTBAR" : "VERSTECKT");
    }

    void D3D8Hook::SetOverlayVisibility(bool visible) {
        g_bOverlayVisible = visible;
        m_showOverlay.store(visible);
    }

    // F2: Maus-Modus umschalten (UI-Klicks vs. Gameplay-Kamerasteuerung)
    void D3D8Hook::ToggleMouseMode() {
        static DWORD s_lastF2Tick = 0;
        DWORD now = GetTickCount();
        if (now - s_lastF2Tick < 150) return;
        s_lastF2Tick = now;

        SetMouseMode(!m_mouseInputMode.load());
    }

    void D3D8Hook::SetMouseMode(bool uiMouseMode) {
        g_bUIModeActive = uiMouseMode;
        m_mouseInputMode.store(uiMouseMode);
        MAD_LOG("[D3D8Hook] Maus-Modus geaendert -> %s (Taste F2)", 
                uiMouseMode ? "UI-BEDIENUNG (Cursor frei, Game-Input blockiert)" : "GAMEPLAY-MODUS (Cursor im Spiel gefangen)");
        UpdateMouseCapture();
    }

    void D3D8Hook::UpdateMouseCapture() {
        HWND hWnd = m_hGameWindow;
        if (!hWnd) hWnd = FindWindowA("RWSConsoleD3D8", nullptr);

        bool isMenuOrPaused = m_mouseInputMode.load();
        if (m_pTransformStore) {
            PlayerTransform t = m_pTransformStore->Get();
            if (!t.isValid || t.isPaused) {
                isMenuOrPaused = true;
            }
        }

        if (isMenuOrPaused) {
            // Zustand A: UI-Bedienmodus oder Pause/Hauptmenü (Cursor frei, KEIN ClipCursor!)
            ::ClipCursor(nullptr);
            ::SetCursor(::LoadCursorA(nullptr, MAKEINTRESOURCEA(32512))); // IDC_ARROW
            while (::ShowCursor(TRUE) < 0);

            if (m_imguiInitialized.load()) {
                ImGui::GetIO().MouseDrawCursor = false;
            }
        } else {
            // Zustand B: Aktiver 3D-Gameplay-Modus (F2 inaktiv, nicht pausiert)
            // Cursor ausblenden und im Client-Bereich des Spielfensters fesseln
            if (m_imguiInitialized.load()) {
                ImGui::GetIO().MouseDrawCursor = false;
            }
            while (::ShowCursor(FALSE) >= 0);

            if (hWnd && GetForegroundWindow() == hWnd) {
                RECT rc = {};
                if (m_isBorderless && m_screenWidth > 0 && m_screenHeight > 0) {
                    rc.left = 0;
                    rc.top = 0;
                    rc.right = m_screenWidth;
                    rc.bottom = m_screenHeight;
                } else {
                    ::GetClientRect(hWnd, &rc);
                    ::MapWindowPoints(hWnd, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);
                }
                ::ClipCursor(&rc);
            }
        }
    }

    // 5. STRIKTES FLANKEN-TRIGGERING FUER HOTKEYS (KEIN DOPPEL-AUSLOESEN ODER SPAMMEN)
    void D3D8Hook::ProcessHotkeys() {
        const auto& cfg = Config::Instance().Get();

        // F2: Maus-Modus Toggle
        static bool prevF2 = false;
        bool curF2 = ((GetAsyncKeyState(cfg.keyToggleMouseMode) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_F2) & 0x8000) != 0);
        if (curF2 && !prevF2) {
            ToggleMouseMode();
            MAD_LOG("[Hotkey] F2 betätigt -> Neuer Maus-Modus: %s", 
                    m_mouseInputMode.load() ? "UI-BEDIENUNG (Frei)" : "GAMEPLAY (Gefangen)");
        }
        prevF2 = curF2;

        // F3 / INSERT: Overlay Sichtbarkeit Toggle
        static bool prevF3 = false;
        bool curF3 = ((GetAsyncKeyState(cfg.keyToggleOverlay) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_F3) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_INSERT) & 0x8000) != 0);
        if (curF3 && !prevF3) {
            ToggleOverlayVisibility();
            MAD_LOG("[Hotkey] F3 betätigt -> Overlay-Sichtbarkeit: %s", 
                    m_showOverlay.load() ? "SICHTBAR" : "AUSGEBLENDET");
        }
        prevF3 = curF3;

        // Taste 9 / NUMPAD9: Widescreen Toggle
        static bool prev9 = false;
        bool cur9 = ((GetAsyncKeyState(cfg.keyToggleWidescreen) & 0x8000) != 0) ||
                    (cfg.keyToggleWidescreen == 57 && ((GetAsyncKeyState(VK_NUMPAD9) & 0x8000) != 0));
        if (cur9 && !prev9) {
            ToggleBorderlessWindowed();
            MAD_LOG("[Hotkey] Taste 9 betätigt -> Widescreen-Modus: %s (%dx%d)", 
                    m_isBorderless ? "BORDERLESS 16:9" : "FENSTER 4:3", m_screenWidth, m_screenHeight);
        }
        prev9 = cur9;
    }

    // 4. RENDERWARE CPU-SIDE FRUSTUM CULLING FIX (OBJECT POP-IN AT HORIZONTAL EDGES)
    void D3D8Hook::UpdateRenderWareCameraFrustum() {
        if (!m_isBorderless || m_screenHeight <= 0) return;

        uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
        if (!gameBase) return;

        // 1. Game.exe interne Aufloesungs-Globals (0x0062a5c8 & 0x0062a5cc) synchronisieren
        int* pGameWidth  = reinterpret_cast<int*>(gameBase + 0x0022A5C8);
        int* pGameHeight = reinterpret_cast<int*>(gameBase + 0x0022A5CC);
        if (!IsBadWritePtr(pGameWidth, sizeof(int)) && !IsBadWritePtr(pGameHeight, sizeof(int))) {
            if (*pGameWidth != m_screenWidth || *pGameHeight != m_screenHeight) {
                *pGameWidth  = m_screenWidth;
                *pGameHeight = m_screenHeight;
                MAD_LOG("[RW:GLOBALS] Synchronized Game.exe resolution globals -> %dx%d (0x%p, 0x%p)",
                        m_screenWidth, m_screenHeight, (void*)pGameWidth, (void*)pGameHeight);
            }
        }

        // 0x0022AC18 enthaelt den globalen RwGlobals-Zeiger
        uintptr_t* ppRwGlobals = reinterpret_cast<uintptr_t*>(gameBase + 0x0022AC18);
        if (!ppRwGlobals || IsBadReadPtr(ppRwGlobals, sizeof(void*)) || !*ppRwGlobals) return;

        uintptr_t pRwGlobals = *ppRwGlobals;
        if (IsBadReadPtr(reinterpret_cast<void*>(pRwGlobals), sizeof(void*))) return;

        // Offset 0 von RwGlobals ist curCamera (aktive RwCamera)
        void* curCamera = *reinterpret_cast<void**>(pRwGlobals);
        if (!curCamera || IsBadReadPtr(curCamera, 0x88)) return;

        // 2. RwRaster FrameBuffer (+0x60) & ZBuffer (+0x64) Dimensionen synchronisieren
        void** ppFrameBuffer = reinterpret_cast<void**>((char*)curCamera + 0x60);
        void** ppZBuffer     = reinterpret_cast<void**>((char*)curCamera + 0x64);

        if (ppFrameBuffer && *ppFrameBuffer && !IsBadWritePtr(*ppFrameBuffer, 0x34)) {
            int* pFBWidth  = reinterpret_cast<int*>((char*)*ppFrameBuffer + 0x0C);
            int* pFBHeight = reinterpret_cast<int*>((char*)*ppFrameBuffer + 0x10);
            int* pOrigW    = reinterpret_cast<int*>((char*)*ppFrameBuffer + 0x28);
            int* pOrigH    = reinterpret_cast<int*>((char*)*ppFrameBuffer + 0x2C);
            if (*pFBWidth != m_screenWidth || *pFBHeight != m_screenHeight) {
                *pFBWidth  = m_screenWidth;
                *pFBHeight = m_screenHeight;
                *pOrigW    = m_screenWidth;
                *pOrigH    = m_screenHeight;
            }
        }

        if (ppZBuffer && *ppZBuffer && !IsBadWritePtr(*ppZBuffer, 0x34)) {
            int* pZBWidth  = reinterpret_cast<int*>((char*)*ppZBuffer + 0x0C);
            int* pZBHeight = reinterpret_cast<int*>((char*)*ppZBuffer + 0x10);
            int* pOrigW    = reinterpret_cast<int*>((char*)*ppZBuffer + 0x28);
            int* pOrigH    = reinterpret_cast<int*>((char*)*ppZBuffer + 0x2C);
            if (*pZBWidth != m_screenWidth || *pZBHeight != m_screenHeight) {
                *pZBWidth  = m_screenWidth;
                *pZBHeight = m_screenHeight;
                *pOrigW    = m_screenWidth;
                *pOrigH    = m_screenHeight;
            }
        }

        float targetAspect = (float)m_screenWidth / (float)m_screenHeight;
        float baseAspect = 4.0f / 3.0f; // 1.333333f

        if (targetAspect > baseAspect) {
            float* pViewWindowX = reinterpret_cast<float*>((char*)curCamera + 0x68);
            float* pViewWindowY = reinterpret_cast<float*>((char*)curCamera + 0x6C);
            float* pRecipX      = reinterpret_cast<float*>((char*)curCamera + 0x70);

            if (*pViewWindowY > 0.001f) {
                // Frustum-Formel:
                // aspectMultiplier = targetAspect / (4.0f / 3.0f);
                // camera->viewWindow.x = originalViewWindowX * aspectMultiplier;
                float originalViewWindowX = (*pViewWindowY) * baseAspect;
                float aspectMultiplier = targetAspect / baseAspect;
                float targetViewWindowX = originalViewWindowX * aspectMultiplier;

                if (fabs(*pViewWindowX - targetViewWindowX) > 0.0001f) {
                    *pViewWindowX = targetViewWindowX;
                    *pRecipX      = 1.0f / targetViewWindowX;

                    // SetFrustum (+0x10) aufrufen, um die 6 CPU-Culling Planes neu zu berechnen
                    using PFN_SetFrustum = void(__cdecl*)(void*);
                    PFN_SetFrustum setFrustum = *reinterpret_cast<PFN_SetFrustum*>((char*)curCamera + 0x10);
                    if (setFrustum) {
                        setFrustum(curCamera);
                    }

                    static uint32_t s_lastFrustumLog = 0;
                    if (m_frameCount.load() - s_lastFrustumLog > 300) {
                        s_lastFrustumLog = (uint32_t)m_frameCount.load();
                        MAD_LOG("[RW:CAMERA] Frustum culling plane expanded: viewWindow.x=%.4f (Aspect: %.2f)",
                                targetViewWindowX, targetAspect);
                    }
                }
            }
        }
    }

    void D3D8Hook::InitializeAsync() {
        if (m_initialized.load()) return;
        MAD_LOG("[D3D8Hook] Initialisiere D3D8 Hooking Subsystem...");
        HANDLE hThread = CreateThread(nullptr, 0, InitThreadProc, nullptr, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        }
    }

    bool D3D8Hook::ForceHookD3D8Device() {
        if (m_initialized.load()) return true;

        uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);

        // 1. Primäre Methode: Direkter Zugriff auf den statischen RenderWare 3.7 Device Pointer in Game.exe
        if (gameBase) {
            IDirect3DDevice8** ppRwDevice = reinterpret_cast<IDirect3DDevice8**>(gameBase + 0x0022CE78);
            if (ppRwDevice && !IsBadReadPtr(ppRwDevice, sizeof(void*))) {
                IDirect3DDevice8* pDevice = *ppRwDevice;
                if (pDevice && !IsBadReadPtr(pDevice, sizeof(void*))) {
                    void** vtable = *reinterpret_cast<void***>(pDevice);
                    if (vtable && !IsBadReadPtr(vtable, sizeof(void*) * 80)) {
                        HWND hGame = m_hGameWindow;
                        if (!hGame) {
                            hGame = FindWindowA("RWSConsoleD3D8", nullptr);
                            if (!hGame) {
                                HWND* pHwnd = reinterpret_cast<HWND*>(gameBase + 0x0022CE74);
                                if (pHwnd && !IsBadReadPtr(pHwnd, sizeof(HWND))) {
                                    hGame = *pHwnd;
                                }
                            }
                            if (hGame) m_hGameWindow = hGame;
                        }
                        MAD_LOG("[D3D8Hook] SUCCESS: Existing IDirect3DDevice8 captured from RenderWare global (0x%p, HWND: 0x%p)!", 
                                (void*)pDevice, (void*)hGame);
                        OnDeviceCreated(pDevice, hGame);
                        return true;
                    }
                }
            }
        }

        // 2. Sekundäre Methode: Export aus d3d8.dll Proxy (falls aktiv)
        HMODULE hD3D8 = GetModuleHandleA("d3d8.dll");
        if (hD3D8) {
            using PFN_GetDevice = IDirect3DDevice8* (__cdecl*)(void);
            auto pfnGet = reinterpret_cast<PFN_GetDevice>(GetProcAddress(hD3D8, "GetForcedD3D8Device"));
            if (!pfnGet) pfnGet = reinterpret_cast<PFN_GetDevice>(GetProcAddress(hD3D8, "MadLoader_GetD3D8Device"));

            if (pfnGet) {
                IDirect3DDevice8* pDevice = pfnGet();
                if (pDevice) {
                    HWND hGame = m_hGameWindow;
                    if (!hGame) {
                        hGame = FindWindowA("RWSConsoleD3D8", nullptr);
                        if (hGame) m_hGameWindow = hGame;
                    }
                    MAD_LOG("[D3D8Hook] SUCCESS: Existing IDirect3DDevice8 captured from d3d8 proxy export (0x%p)!", (void*)pDevice);
                    OnDeviceCreated(pDevice, hGame);
                    return true;
                }
            }
        }

        return false;
    }

    DWORD WINAPI D3D8Hook::InitThreadProc(LPVOID lpParam) {
        auto& hook = D3D8Hook::Instance();
        MAD_LOG("[D3D8Hook] Sichere verzoegerte Initialisierung gestartet (Warten auf Device & Fenster)...");

        uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);

        HWND hGame = nullptr;
        for (int i = 0; i < 400; ++i) { // bis zu 20 Sekunden warten
            hGame = FindWindowA("RWSConsoleD3D8", nullptr);
            if (!hGame && gameBase) {
                HWND* pHwnd = reinterpret_cast<HWND*>(gameBase + 0x0022CE74);
                if (pHwnd && !IsBadReadPtr(pHwnd, sizeof(HWND))) {
                    hGame = *pHwnd;
                }
            }
            if (hGame && IsWindowVisible(hGame)) {
                break;
            }
            Sleep(50);
        }

        if (hGame) {
            hook.m_hGameWindow = hGame;
            MAD_LOG("[D3D8Hook] Hauptfenster gefunden (HWND: 0x%p)", (void*)hGame);
        }

        for (int i = 0; i < 400; ++i) {
            if (hook.m_initialized.load()) break;

            if (hook.ForceHookD3D8Device()) {
                break;
            }
            Sleep(50);
        }

        if (hook.m_initialized.load()) {
            MAD_LOG("[D3D8Hook] D3D8 Device erfolgreich im Render-Kreislauf gebunden!");

            if (Config::Instance().Get().autoEnableWidescreen) {
                MAD_LOG("[Display] AutoEnableWidescreen ist aktiv -> Schalte Borderless Widescreen ein.");
                hook.ToggleBorderlessWindowed();
            }
        } else {
            MAD_LOG("[D3D8Hook] WARNUNG: D3D8 Device konnte innerhalb des Zeitlimits nicht gebunden werden.");
        }

        return 0;
    }

    void D3D8Hook::OnDeviceCreated(IDirect3DDevice8* pDevice, HWND hGameWindow) {
        if (!pDevice) return;
        if (m_initialized.load()) return;
        m_pDevice = pDevice;
        if (hGameWindow) m_hGameWindow = hGameWindow;

        void** vtable = *reinterpret_cast<void***>(pDevice);
        HookDeviceVTable(vtable);
    }

    bool D3D8Hook::HookDeviceVTable(void** vtable) {
        if (!vtable) return false;

        PFN_Reset targetReset = reinterpret_cast<PFN_Reset>(vtable[14]);
        PFN_EndScene targetEndScene = reinterpret_cast<PFN_EndScene>(vtable[35]);

        if (targetEndScene == Hooked_EndScene) {
            return true;
        }

        m_pOriginalReset = targetReset;
        m_pOriginalEndScene = targetEndScene;

        DWORD oldProtect = 0;
        // Unprotect slots 14 through 35 (22 slots = 88 bytes)
        if (!VirtualProtect(&vtable[14], sizeof(void*) * 22, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            MAD_LOG("[D3D8Hook] FEHLER: VirtualProtect auf Device VTable fehlgeschlagen! Error: %lu", GetLastError());
            return false;
        }

        vtable[14] = reinterpret_cast<void*>(&Hooked_Reset);
        vtable[35] = reinterpret_cast<void*>(&Hooked_EndScene);

        VirtualProtect(&vtable[14], sizeof(void*) * 22, oldProtect, &oldProtect);

        // IAT Hook auf GetCursorPos in Game.exe installieren (fuer 1:1 Maus-Koordinaten in Menus)
        uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
        if (gameBase && !s_pOriginalGetCursorPos) {
            void** ppGetCursorPos = reinterpret_cast<void**>(gameBase + 0x001CA240);
            if (ppGetCursorPos && !IsBadReadPtr(ppGetCursorPos, sizeof(void*))) {
                DWORD iatProtect = 0;
                if (VirtualProtect(ppGetCursorPos, sizeof(void*), PAGE_EXECUTE_READWRITE, &iatProtect)) {
                    s_pOriginalGetCursorPos = reinterpret_cast<PFN_GetCursorPos>(*ppGetCursorPos);
                    *ppGetCursorPos = reinterpret_cast<void*>(&Hooked_GetCursorPos);
                    VirtualProtect(ppGetCursorPos, sizeof(void*), iatProtect, &iatProtect);
                    MAD_LOG("[D3D8Hook] IAT Hook fuer GetCursorPos (0x%p) erfolgreich aktiv!", (void*)ppGetCursorPos);
                }
            }
        }

        m_initialized.store(true);
        MAD_LOG("[D3D8Hook] DIRECT MEMORY HOOK SUCCESS: D3D8 Device at 0x%p hooked!", (void*)m_pDevice);
        MAD_LOG("[D3D8Hook] Minimal VTable Swapped: Reset(14), EndScene(35)");
        return true;
    }

    // 2. CRASH-FREIER BORDERLESS WIDESCREEN (TASTE 9 OHNE RESET-DEADLOCK)
    void D3D8Hook::ToggleBorderlessWindowed() {
        HWND hWnd = m_hGameWindow;
        if (!hWnd) {
            hWnd = FindWindowA("RWSConsoleD3D8", nullptr);
            if (!hWnd) return;
            m_hGameWindow = hWnd;
        }

        m_isBorderless = !m_isBorderless;

        if (m_isBorderless) {
            m_prevWindowStyle = GetWindowLongA(hWnd, GWL_STYLE);
            GetWindowRect(hWnd, &m_prevWindowRect);

            HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(mi) };
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            int posX = 0, posY = 0;

            if (GetMonitorInfoA(hMon, &mi)) {
                screenW = mi.rcMonitor.right - mi.rcMonitor.left;
                screenH = mi.rcMonitor.bottom - mi.rcMonitor.top;
                posX = mi.rcMonitor.left;
                posY = mi.rcMonitor.top;
            }

            m_screenWidth = screenW;
            m_screenHeight = screenH;

            LONG newStyle = m_prevWindowStyle;
            newStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
            newStyle |= WS_POPUP;

            SetWindowLongA(hWnd, GWL_STYLE, newStyle);
            SetWindowPos(hWnd, HWND_TOP, posX, posY, screenW, screenH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            // Viewport sofort auf die Monitor-Auflösung setzen
            if (m_pDevice) {
                D3DVIEWPORT8 vp = {};
                vp.X = 0;
                vp.Y = 0;
                vp.Width = (DWORD)screenW;
                vp.Height = (DWORD)screenH;
                vp.MinZ = 0.0f;
                vp.MaxZ = 1.0f;
                m_pDevice->SetViewport(&vp);
            }

            // Game.exe globale Aufloesungsvariablen (0x0062a5c8 & 0x0062a5cc) sofort aktualisieren
            uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
            if (gameBase) {
                int* pGameWidth  = reinterpret_cast<int*>(gameBase + 0x0022A5C8);
                int* pGameHeight = reinterpret_cast<int*>(gameBase + 0x0022A5CC);
                if (!IsBadWritePtr(pGameWidth, sizeof(int)) && !IsBadWritePtr(pGameHeight, sizeof(int))) {
                    *pGameWidth  = screenW;
                    *pGameHeight = screenH;
                }
            }

            UpdateRenderWareCameraFrustum();

            MAD_LOG("[Display] Crash-freier Borderless Widescreen (Non-Reset) per Taste 9 aktiviert! (%dx%d an Pos %d,%d)", 
                    screenW, screenH, posX, posY);
        } else {
            SetWindowLongA(hWnd, GWL_STYLE, m_prevWindowStyle);
            int w = m_prevWindowRect.right - m_prevWindowRect.left;
            int h = m_prevWindowRect.bottom - m_prevWindowRect.top;
            if (w <= 0 || h <= 0) { w = 800; h = 600; }
            m_screenWidth = w;
            m_screenHeight = h;
            SetWindowPos(hWnd, HWND_NOTOPMOST, m_prevWindowRect.left, m_prevWindowRect.top, w, h, 
                         SWP_FRAMECHANGED | SWP_SHOWWINDOW);

            if (m_pDevice) {
                D3DVIEWPORT8 vp = {};
                vp.X = 0;
                vp.Y = 0;
                vp.Width = (DWORD)w;
                vp.Height = (DWORD)h;
                vp.MinZ = 0.0f;
                vp.MaxZ = 1.0f;
                m_pDevice->SetViewport(&vp);
            }

            uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
            if (gameBase) {
                int* pGameWidth  = reinterpret_cast<int*>(gameBase + 0x0022A5C8);
                int* pGameHeight = reinterpret_cast<int*>(gameBase + 0x0022A5CC);
                if (!IsBadWritePtr(pGameWidth, sizeof(int)) && !IsBadWritePtr(pGameHeight, sizeof(int))) {
                    *pGameWidth  = w;
                    *pGameHeight = h;
                }
            }

            MAD_LOG("[Display] Borderless Widescreen deaktiviert, Standardfenster wiederhergestellt (%dx%d).", w, h);
        }

        UpdateMouseCapture();
    }

    void D3D8Hook::Shutdown() {
        if (!m_initialized.load()) return;

        if (m_hGameWindow && m_pOriginalGameWndProc) {
            SetWindowLongPtrA(m_hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_pOriginalGameWndProc));
            m_pOriginalGameWndProc = nullptr;
        }

        if (m_hConsoleWindow && m_pOriginalConsoleWndProc) {
            SetWindowLongPtrA(m_hConsoleWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_pOriginalConsoleWndProc));
            m_pOriginalConsoleWndProc = nullptr;
        }

        if (m_imguiInitialized.load()) {
            ImGui_ImplDX8_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            m_imguiInitialized.store(false);
        }

        uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
        if (gameBase && s_pOriginalGetCursorPos) {
            void** ppGetCursorPos = reinterpret_cast<void**>(gameBase + 0x001CA240);
            if (ppGetCursorPos && !IsBadWritePtr(ppGetCursorPos, sizeof(void*))) {
                DWORD oldProtect = 0;
                if (VirtualProtect(ppGetCursorPos, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    *ppGetCursorPos = reinterpret_cast<void*>(s_pOriginalGetCursorPos);
                    VirtualProtect(ppGetCursorPos, sizeof(void*), oldProtect, &oldProtect);
                }
            }
            s_pOriginalGetCursorPos = nullptr;
        }

        m_initialized.store(false);
        MAD_LOG("[D3D8Hook] Subsystem beendet.");
    }

    void D3D8Hook::EnsureImGuiInitialized(IDirect3DDevice8* pDevice) {
        if (m_imguiInitialized.load() || !pDevice) return;

        if (!m_hGameWindow) {
            m_hGameWindow = FindWindowA("RWSConsoleD3D8", nullptr);
            if (!m_hGameWindow) {
                D3DDEVICE_CREATION_PARAMETERS cp = {};
                if (SUCCEEDED(pDevice->GetCreationParameters(&cp))) {
                    m_hGameWindow = cp.hFocusWindow;
                }
            }
        }

        m_hConsoleWindow = GetConsoleWindow();

        MAD_LOG("[D3D8Hook] ImGui Initialisierung fuer HWND: 0x%08X...", (unsigned int)(uintptr_t)m_hGameWindow);

        if (m_hGameWindow) {
            m_pOriginalGameWndProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrA(m_hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Hooked_GameWndProc))
            );

            if (m_hConsoleWindow && m_hConsoleWindow != m_hGameWindow) {
                m_pOriginalConsoleWndProc = reinterpret_cast<WNDPROC>(
                    SetWindowLongPtrA(m_hConsoleWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Hooked_ConsoleWndProc))
                );
            }

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 6.0f;
            style.FrameRounding = 4.0f;
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);

            ImGui_ImplWin32_Init(m_hGameWindow);
            ImGui_ImplDX8_Init(pDevice);

            m_imguiInitialized.store(true);
            MAD_LOG("[D3D8Hook] ImGui DX8 Render-Loop ERFOLGREICH INITIALISIERT!");

            UpdateMouseCapture();
        }
    }

    void D3D8Hook::OnEndScene(IDirect3DDevice8* pDevice) {
        if (!pDevice) return;
        m_pDevice = pDevice;

        D3DVIEWPORT8 currentVp;
        if (FAILED(pDevice->GetViewport(&currentVp))) {
            return;
        }

        // Render ImGui ONLY on main backbuffer passes (>= 640x480), never in small offscreen shadow maps:
        if (currentVp.Width < 640 || currentVp.Height < 480) {
            return;
        }

        // 1. Initialize ImGui once on the backbuffer thread if not already done
        EnsureImGuiInitialized(pDevice);
        if (!m_imguiInitialized.load()) {
            return;
        }

        // 2. Per-frame game updates (now run on backbuffer pass since Present is not hooked)
        uint64_t frame = ++m_frameCount;

        if (m_perfFreq.QuadPart == 0) {
            QueryPerformanceFrequency(&m_perfFreq);
            QueryPerformanceCounter(&m_lastFrameTime);
        }
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - m_lastFrameTime.QuadPart) / (double)m_perfFreq.QuadPart;
        m_lastFrameTime = now;
        m_lastDeltaTime = (float)elapsed;
        if (m_lastDeltaTime <= 0.0f || m_lastDeltaTime > 0.1f) {
            m_lastDeltaTime = 0.0166f;
        }

        // Process Hotkeys (F2, F3, Taste 9)
        ProcessHotkeys();

        // Update Widescreen Frustum
        UpdateRenderWareCameraFrustum();

        // Update Cheats & Flight
        CheatManager::Instance().Update(m_lastDeltaTime);

        // Update Mouse Capture
        static bool s_lastPaused = false;
        bool isMenuOrPaused = m_mouseInputMode.load();
        if (m_pTransformStore) {
            PlayerTransform t = m_pTransformStore->Get();
            if (!t.isValid || t.isPaused) {
                isMenuOrPaused = true;
            }
        }
        if (isMenuOrPaused != s_lastPaused) {
            s_lastPaused = isMenuOrPaused;
            UpdateMouseCapture();
        }
        if (isMenuOrPaused) {
            ::ClipCursor(nullptr);
        }

        // 3. Render ImGui Overlay if visible
        if (g_bOverlayVisible)
        {
            ImGui::GetIO().DisplaySize = ImVec2((float)currentVp.Width, (float)currentVp.Height);

            // Render ImGui frame
            ImGui_ImplDX8_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            RenderOverlayUI();

            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplDX8_RenderDrawData(ImGui::GetDrawData());
            pDevice->SetViewport(&currentVp); // Restore
        }

        m_frameRendered.store(true);
    }

    void D3D8Hook::OnPreReset(IDirect3DDevice8* pDevice) {
        if (m_imguiInitialized.load()) {
            MAD_LOG("[D3D8Hook] InvalidateDeviceObjects VOR Reset...");
            ImGui_ImplDX8_InvalidateDeviceObjects();
        }
    }

    void D3D8Hook::OnPostReset(IDirect3DDevice8* pDevice) {
        if (m_imguiInitialized.load()) {
            MAD_LOG("[D3D8Hook] CreateDeviceObjects NACH Reset...");
            ImGui_ImplDX8_CreateDeviceObjects();
        }
    }

    void D3D8Hook::RenderOverlayUI() {
        if (!g_bOverlayVisible) return;

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
        if (!g_bUIModeActive) {
            windowFlags |= ImGuiWindowFlags_NoInputs;
        }

        ImGui::SetNextWindowSize(ImVec2(540, 460), ImGuiCond_FirstUseEver);
        bool open = true;
        if (ImGui::Begin("Madagascar Multiplayer Client ###MadMultiplayerUI", &open, windowFlags)) {
            if (!open) {
                ToggleOverlay();
            }
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "MADAGASCAR (2005) - MULTIPLAYER OVERLAY");
            ImGui::TextDisabled("Hotkeys: [F2] UI-Modus an/aus | [F3] Menue ein/aus | [Taste 9] 16:9 Widescreen");
            ImGui::Separator();

            if (ImGui::BeginTabBar("MainTabBar")) {
                if (ImGui::BeginTabItem("Multiplayer & State")) {
                    if (ImGui::CollapsingHeader("Steuerung & Maus-Fokus", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Text("Maus- / UI-Status: ");
                        ImGui::SameLine();
                        if (g_bUIModeActive) {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "[UI-BEDIENUNG] (Cursor frei, Game-Input isoliert)");
                        } else {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[GAMEPLAY-MODUS] (Im Spiel gefangen - Taste F2)");
                        }

                        if (ImGui::Button(g_bUIModeActive ? "Maus im Spiel fangen [F2]" : "Maus freigeben fuer UI [F2]", ImVec2(260, 28))) {
                            ToggleUIMode();
                        }
                        ImGui::SameLine();
                        ImGui::TextDisabled("[Hotkey: F2]");
                    }

                    ImGui::Spacing();

                    if (ImGui::CollapsingHeader("Lokaler Spieler (Live Coordinates)", ImGuiTreeNodeFlags_DefaultOpen)) {
                        if (m_pTransformStore) {
                            PlayerTransform t = m_pTransformStore->Get();
                            if (t.isValid) {
                                ImGui::Text("Status:     ");
                                ImGui::SameLine();
                                if (t.isPaused) {
                                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[PAUSE]");
                                } else {
                                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[INGAME]");
                                }

                                ImGui::Text("Position X: %.2f", t.x);
                                ImGui::Text("Position Y: %.2f (Hoehe)", t.y);
                                ImGui::Text("Position Z: %.2f", t.z);
                                ImGui::Text("Rotation:   Yaw = %.2f rad | Pitch = %.2f rad", t.yaw, t.pitch);
                            } else {
                                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[WARTEN] Kein aktiver Spieler (Menue / Laden)");
                            }
                        } else {
                            ImGui::TextDisabled("Transform-Store nicht verknuepft.");
                        }
                    }

                    ImGui::Spacing();

                    if (ImGui::CollapsingHeader("Netzwerk & Relay-Server", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::InputText("Server IP", m_serverIp, sizeof(m_serverIp));
                        ImGui::InputInt("Port", &m_serverPort);
                        ImGui::InputText("Spielername", m_playerName, sizeof(m_playerName));

                        ImGui::Spacing();

                        if (!m_isConnected) {
                            if (ImGui::Button("Mit Server Verbinden", ImVec2(180, 28))) {
                                m_isConnected = true;
                                MAD_LOG("[UI] Verbindungsanfrage an %s:%d", m_serverIp, m_serverPort);
                            }
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Getrennt");
                        } else {
                            if (ImGui::Button("Verbindung Trennen", ImVec2(180, 28))) {
                                m_isConnected = false;
                                MAD_LOG("[UI] Verbindung manuell getrennt.");
                            }
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Verbunden");
                        }
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Text("Anzeige & Fenster-Modus:");
                        if (ImGui::Button(m_isBorderless ? "Borderless Widescreen: [AKTIVIERT]" : "Borderless Widescreen: [DEAKTIVIERT] (Taste 9)", ImVec2(340, 26))) {
                            ToggleBorderlessWindowed();
                        }
                        ImGui::SameLine();
                        ImGui::TextDisabled("[Taste 9]");
                        if (m_isBorderless) {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "Aufloesung: %dx%d (16:9 Hor+ FOV aktiv)", m_screenWidth, m_screenHeight);
                        }
                    }
                    ImGui::EndTabItem();
                }

                // 2. CHEATS & SANDBOX TAB
                if (ImGui::BeginTabItem("Cheats & Sandbox")) {
                    CheatManager::Instance().RenderMenu();
                    ImGui::EndTabItem();
                }

                // 3. SAUBERER LOG-TAB OHNE HIEROGLYPHEN DURCH ASCII-SANITY & TextUnformatted
                if (ImGui::BeginTabItem("Debug & Engine Logs")) {
                    ImGui::Text("Render Frames Count: %llu", (unsigned long long)m_frameCount.load());
                    if (ImGui::Button("Logs leeren")) {
                        Logger::Instance().ClearLogs();
                    }
                    ImGui::Separator();

                    ImGui::BeginChild("LogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                    auto logs = Logger::Instance().GetLogs();
                    for (const auto& logMsg : logs) {
                        ImGui::TextDisabled("[%s]", logMsg.timestamp.c_str());
                        ImGui::SameLine();
                        ImGui::TextUnformatted(logMsg.text.c_str());
                    }
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                        ImGui::SetScrollHereY(1.0f);
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    // SICHERES MOUSE-UNLOCK & WNDPROC HOOKING
    LRESULT D3D8Hook::HandleGameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // 1. WM_SETCURSOR: Im UI-Bedienmodus Cursor anzeigen & freigeben
        if (uMsg == WM_SETCURSOR) {
            if (g_bUIModeActive) {
                UnlockMouseCursor();
                return TRUE;
            }
        }

        // 2. UI MODE ACTIVE: VOLLE QUARANTAENE (ZERO INGAME CLICK-THROUGH)
        if (g_bUIModeActive)
        {
            // Let ImGui process the message first:
            if (m_imguiInitialized.load()) {
                ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
            }

            // Allow F2 to toggle back to Gameplay Mode:
            if (uMsg == WM_KEYDOWN && wParam == VK_F2) {
                ToggleUIMode();
                return 0;
            }

            // SWALLOW ALL MOUSE EVENTS COMPLETELY (Do NOT call CallWindowProc):
            switch (uMsg) {
                case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
                case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
                case WM_MOUSEMOVE:
                case WM_MOUSEWHEEL:
                case WM_XBUTTONDOWN: case WM_XBUTTONUP:
                case WM_INPUT: // Blocks RawInput camera movement
                    return 0;
            }

            // Block gameplay movement keys (WASD, Space, etc.) while UI is open:
            if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR) {
                return 0;
            }
        }
        else
        {
            // GAMEPLAY MODE:
            // Allow F2 to toggle into UI Mode:
            if (uMsg == WM_KEYDOWN && wParam == VK_F2) {
                ToggleUIMode();
                return 0;
            }

            // Allow F3 / INSERT to toggle overlay visibility:
            if (uMsg == WM_KEYDOWN && (wParam == VK_F3 || wParam == VK_INSERT)) {
                ToggleOverlayVisibility();
                return 0;
            }
        }

        bool isMenuOrPaused = m_mouseInputMode.load();
        if (m_pTransformStore) {
            PlayerTransform t = m_pTransformStore->Get();
            if (!t.isValid || t.isPaused) {
                isMenuOrPaused = true;
            }
        }

        // 3. WM_SETCURSOR: Cursor im Menü-, Pause- oder UI-Zustand stets freigeben
        if (uMsg == WM_SETCURSOR) {
            if (isMenuOrPaused) {
                UnlockMouseCursor();
                return TRUE;
            }
        }

        // 4. WM_MOUSEMOVE im Menü/Pause/UI-Modus: Cursor freihalten
        if (uMsg == WM_MOUSEMOVE || uMsg == WM_NCMOUSEMOVE) {
            if (isMenuOrPaused || GetForegroundWindow() != m_hGameWindow) {
                UnlockMouseCursor();
            }
        }

        // 5. Alt-Tab / Focus Loss
        if (uMsg == WM_KILLFOCUS || uMsg == WM_ACTIVATEAPP) {
            if (wParam == FALSE) {
                UnlockMouseCursor();
                if (m_imguiInitialized.load()) {
                    ImGuiIO& io = ImGui::GetIO();
                    io.ClearEventsQueue();
                    io.AddFocusEvent(false);
                }
                MAD_LOG("[WndProc] WM_KILLFOCUS / WM_ACTIVATEAPP -> Maus freigegeben.");
            } else {
                UpdateMouseCapture();
            }
        }

        // 6. Fenster-Drift Fix beim Draggen
        if (uMsg == WM_SYSCOMMAND) {
            DWORD cmd = (wParam & 0xFFF0);
            if (cmd == SC_MOVE || cmd == SC_SIZE) {
                UnlockMouseCursor();
            }
        }
        else if (uMsg == WM_ENTERSIZEMOVE) {
            UnlockMouseCursor();
        }


        // 6. Maus-Koordinaten fuer Game.exe Menue-Hit-Testing (0x464510 / 0x463640) 1:1 anpassen
        if (m_isBorderless && m_screenWidth > 0 && m_screenHeight > 0) {
            if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
                uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP) {
                float scaleY = (float)m_screenHeight / 600.0f;
                float scaleX = scaleY;
                float virtual43Width = 800.0f * scaleX;
                float centerOffsetX = ((float)m_screenWidth - virtual43Width) * 0.5f;

                short rawX = (short)LOWORD(lParam);
                short rawY = (short)HIWORD(lParam);

                float mappedX = ((float)rawX - centerOffsetX) / scaleX;
                if (mappedX < 0.0f) mappedX = 0.0f;
                else if (mappedX > 800.0f) mappedX = 800.0f;

                float mappedY = (float)rawY / scaleY;
                if (mappedY < 0.0f) mappedY = 0.0f;
                else if (mappedY > 600.0f) mappedY = 600.0f;

                WORD finalX = (WORD)((mappedX / 800.0f) * (float)m_screenWidth);
                WORD finalY = (WORD)((mappedY / 600.0f) * (float)m_screenHeight);
                lParam = MAKELPARAM(finalX, finalY);
            }
        }

        return CallWindowProcA(m_pOriginalGameWndProc, hWnd, uMsg, wParam, lParam);
    }

    LRESULT D3D8Hook::HandleConsoleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_SETCURSOR || uMsg == WM_MOUSEMOVE || uMsg == WM_NCMOUSEMOVE ||
            uMsg == WM_ENTERSIZEMOVE || uMsg == WM_NCLBUTTONDOWN) {
            UnlockMouseCursor();
        }
        else if (uMsg == WM_SYSCOMMAND) {
            DWORD cmd = (wParam & 0xFFF0);
            if (cmd == SC_MOVE || cmd == SC_SIZE) {
                UnlockMouseCursor();
            }
        }

        return CallWindowProcA(m_pOriginalConsoleWndProc, hWnd, uMsg, wParam, lParam);
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_EndScene(IDirect3DDevice8* pDevice) {
        if (!pDevice) return D3DERR_INVALIDCALL;
        auto& hook = D3D8Hook::Instance();
        hook.OnEndScene(pDevice);
        return hook.m_pOriginalEndScene ? hook.m_pOriginalEndScene(pDevice) : D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_Reset(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
        auto& hook = D3D8Hook::Instance();
        hook.OnPreReset(pDevice);
        MAD_LOG("[D3D8Hook] Device Reset wird ausgefuehrt (BackBuffer: %ux%u)...",
                pPresentationParameters ? pPresentationParameters->BackBufferWidth : 0,
                pPresentationParameters ? pPresentationParameters->BackBufferHeight : 0);
        HRESULT hr = hook.m_pOriginalReset ? hook.m_pOriginalReset(pDevice, pPresentationParameters) : D3D_OK;
        if (SUCCEEDED(hr)) {
            hook.OnPostReset(pDevice);
            MAD_LOG("[D3D8Hook] Device Reset erfolgreich abgeschlossen.");
        } else {
            MAD_LOG("[D3D8Hook] Device Reset fehlgeschlagen! HRESULT: 0x%08X", hr);
        }
        return hr;
    }


    LRESULT CALLBACK D3D8Hook::Hooked_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        return D3D8Hook::Instance().HandleGameWndProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK D3D8Hook::Hooked_ConsoleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        return D3D8Hook::Instance().HandleConsoleWndProc(hWnd, uMsg, wParam, lParam);
    }



    // 4. IAT Hook fuer GetCursorPos: Konvertiert absolute Bildschirmkoordinaten
    // in echte Client-Koordinaten und mappt diese auf den zentrierten Menue-Bereich
    BOOL WINAPI D3D8Hook::Hooked_GetCursorPos(LPPOINT lpPoint) {
        if (!s_pOriginalGetCursorPos || !lpPoint) return FALSE;
        BOOL res = s_pOriginalGetCursorPos(lpPoint);
        if (res) {
            auto& hook = D3D8Hook::Instance();
            HWND hWnd = hook.m_hGameWindow;
            if (!hWnd) hWnd = FindWindowA("RWSConsoleD3D8", nullptr);
            if (hWnd && IsWindow(hWnd)) {
                ScreenToClient(hWnd, lpPoint);

                if (hook.m_isBorderless && hook.m_screenHeight > 0 && hook.m_screenWidth > 0) {
                    float scaleY = (float)hook.m_screenHeight / 600.0f;
                    float scaleX = scaleY;
                    float virtual43Width = 800.0f * scaleX;
                    float centerOffsetX = ((float)hook.m_screenWidth - virtual43Width) * 0.5f;

                    // Mappe die X-Position auf den 4:3-Zentrierungsbereich
                    float mappedX = ((float)lpPoint->x - centerOffsetX) / scaleX;
                    if (mappedX < 0.0f) mappedX = 0.0f;
                    else if (mappedX > 800.0f) mappedX = 800.0f;

                    float mappedY = (float)lpPoint->y / scaleY;
                    if (mappedY < 0.0f) mappedY = 0.0f;
                    else if (mappedY > 600.0f) mappedY = 600.0f;

                    lpPoint->x = (LONG)((mappedX / 800.0f) * (float)hook.m_screenWidth);
                    lpPoint->y = (LONG)((mappedY / 600.0f) * (float)hook.m_screenHeight);
                }
            }
        }
        return res;
    }

} // namespace MadMultiplayer
