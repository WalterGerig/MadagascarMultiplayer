#include "../include/D3D8Hook.h"
#include "../include/Logger.h"
#include "../include/Config.h"
#include "../include/CheatManager.h"
#include <cmath>
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_impl_win32.h"
#include "../vendor/imgui/imgui_impl_dx8.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace MadMultiplayer {

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
        bool visible = !m_showOverlay.load();
        m_showOverlay.store(visible);
        MAD_LOG("[D3D8Hook] Overlay Sichtbarkeit geaendert -> %s (Taste F3)", visible ? "SICHTBAR" : "VERSTECKT");
    }

    // F2: Maus-Modus umschalten (UI-Klicks vs. Gameplay-Kamerasteuerung)
    void D3D8Hook::ToggleMouseMode() {
        SetMouseMode(!m_mouseInputMode.load());
    }

    void D3D8Hook::SetMouseMode(bool uiMouseMode) {
        m_mouseInputMode.store(uiMouseMode);
        MAD_LOG("[D3D8Hook] Maus-Modus geaendert -> %s (Taste F2)", 
                uiMouseMode ? "UI-BEDIENUNG (Cursor frei)" : "GAMEPLAY-MODUS (Cursor im Spiel gefangen)");
        UpdateMouseCapture();
    }

    void D3D8Hook::UpdateMouseCapture() {
        HWND hWnd = m_hGameWindow;
        if (!hWnd) hWnd = FindWindowA("RWSConsoleD3D8", nullptr);

        if (m_mouseInputMode.load()) {
            // Zustand A: UI-Bedienmodus (F2 aktiv)
            // Cursor vollstaendig freigeben fuer Menue und Desktop
            ::ClipCursor(nullptr);
            ::SetCursor(::LoadCursorA(nullptr, MAKEINTRESOURCEA(32512))); // IDC_ARROW
            while (::ShowCursor(TRUE) < 0);

            if (m_imguiInitialized.load()) {
                ImGui::GetIO().MouseDrawCursor = true;
            }
        } else {
            // Zustand B: Gameplay-Modus (F2 inaktiv)
            // Cursor ausblenden und im Client-Bereich des Spielfensters fesseln,
            // damit die Maus bei Kameradrehungen NICHT mehr herausgleitet!
            if (m_imguiInitialized.load()) {
                ImGui::GetIO().MouseDrawCursor = false;
            }
            while (::ShowCursor(FALSE) >= 0);

            if (hWnd && GetForegroundWindow() == hWnd) {
                RECT rc = {};
                ::GetClientRect(hWnd, &rc);
                ::MapWindowPoints(hWnd, nullptr, reinterpret_cast<LPPOINT>(&rc), 2);
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
        }
        prevF2 = curF2;

        // F3 / INSERT: Overlay Sichtbarkeit Toggle
        static bool prevF3 = false;
        bool curF3 = ((GetAsyncKeyState(cfg.keyToggleOverlay) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_F3) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_INSERT) & 0x8000) != 0);
        if (curF3 && !prevF3) {
            ToggleOverlayVisibility();
        }
        prevF3 = curF3;

        // Taste 9 / NUMPAD9: Widescreen Toggle
        static bool prev9 = false;
        bool cur9 = ((GetAsyncKeyState(cfg.keyToggleWidescreen) & 0x8000) != 0) ||
                    (cfg.keyToggleWidescreen == 57 && ((GetAsyncKeyState(VK_NUMPAD9) & 0x8000) != 0));
        if (cur9 && !prev9) {
            ToggleBorderlessWindowed();
        }
        prev9 = cur9;
    }

    // 1. RENDERWARE FRUSTUM CULLING BEHEBEN (OBJEKTE PLOPPEN AM RAND NICHT MEHR WEG)
    void D3D8Hook::UpdateRenderWareCameraFrustum() {
        if (!m_isBorderless || m_screenHeight <= 0) return;

        uintptr_t gameBase = (uintptr_t)GetModuleHandleA(nullptr);
        if (!gameBase) return;

        // 0x0022AC18 enthaelt den globalen RwGlobals-Zeiger
        uintptr_t* ppRwGlobals = reinterpret_cast<uintptr_t*>(gameBase + 0x0022AC18);
        if (!ppRwGlobals || IsBadReadPtr(ppRwGlobals, sizeof(void*)) || !*ppRwGlobals) return;

        uintptr_t pRwGlobals = *ppRwGlobals;
        if (IsBadReadPtr(reinterpret_cast<void*>(pRwGlobals), sizeof(void*))) return;

        // Offset 0 von RwGlobals ist curCamera (aktive RwCamera)
        void* curCamera = *reinterpret_cast<void**>(pRwGlobals);
        if (!curCamera || IsBadReadPtr(curCamera, 0x88)) return;

        float aspect = (float)m_screenWidth / (float)m_screenHeight;
        float baseAspect = 4.0f / 3.0f; // 1.333333f

        if (aspect > baseAspect) {
            float* pViewWindowX = reinterpret_cast<float*>((char*)curCamera + 0x68);
            float* pViewWindowY = reinterpret_cast<float*>((char*)curCamera + 0x6C);
            float* pRecipX      = reinterpret_cast<float*>((char*)curCamera + 0x70);

            if (*pViewWindowY > 0.001f) {
                float targetViewWindowX = (*pViewWindowY) * aspect;
                if (fabs(*pViewWindowX - targetViewWindowX) > 0.001f) {
                    *pViewWindowX = targetViewWindowX;
                    *pRecipX      = 1.0f / targetViewWindowX;

                    // SetFrustum aufrufen (+0x10), um die Frustum-Planes fuer CPU-Culling neu zu berechnen
                    using PFN_SetFrustum = void(__cdecl*)(void*);
                    PFN_SetFrustum setFrustum = *reinterpret_cast<PFN_SetFrustum*>((char*)curCamera + 0x10);
                    if (setFrustum) {
                        setFrustum(curCamera);
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
        PFN_Present targetPresent = reinterpret_cast<PFN_Present>(vtable[15]);
        PFN_EndScene targetEndScene = reinterpret_cast<PFN_EndScene>(vtable[35]);
        PFN_SetTransform targetSetTransform = reinterpret_cast<PFN_SetTransform>(vtable[37]);
        PFN_SetViewport targetSetViewport = reinterpret_cast<PFN_SetViewport>(vtable[40]);
        PFN_DrawPrimitive targetDrawPrimitive = reinterpret_cast<PFN_DrawPrimitive>(vtable[70]);
        PFN_DrawPrimitiveUP targetDrawPrimitiveUP = reinterpret_cast<PFN_DrawPrimitiveUP>(vtable[72]);
        PFN_SetVertexShader targetSetVertexShader = reinterpret_cast<PFN_SetVertexShader>(vtable[76]);

        if (targetPresent == Hooked_Present) {
            return true;
        }

        m_pOriginalReset = targetReset;
        m_pOriginalPresent = targetPresent;
        m_pOriginalEndScene = targetEndScene;
        m_pOriginalSetTransform = targetSetTransform;
        m_pOriginalSetViewport = targetSetViewport;
        m_pOriginalDrawPrimitive = targetDrawPrimitive;
        m_pOriginalDrawPrimitiveUP = targetDrawPrimitiveUP;
        m_pOriginalSetVertexShader = targetSetVertexShader;

        DWORD oldProtect = 0;
        // Unprotect slots 14 through 77 (64 slots = 256 bytes)
        if (!VirtualProtect(&vtable[14], sizeof(void*) * 65, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            MAD_LOG("[D3D8Hook] FEHLER: VirtualProtect auf Device VTable fehlgeschlagen! Error: %lu", GetLastError());
            return false;
        }

        vtable[14] = reinterpret_cast<void*>(&Hooked_Reset);
        vtable[15] = reinterpret_cast<void*>(&Hooked_Present);
        vtable[35] = reinterpret_cast<void*>(&Hooked_EndScene);
        vtable[37] = reinterpret_cast<void*>(&Hooked_SetTransform);
        vtable[40] = reinterpret_cast<void*>(&Hooked_SetViewport);
        vtable[70] = reinterpret_cast<void*>(&Hooked_DrawPrimitive);
        vtable[72] = reinterpret_cast<void*>(&Hooked_DrawPrimitiveUP);
        vtable[76] = reinterpret_cast<void*>(&Hooked_SetVertexShader);

        VirtualProtect(&vtable[14], sizeof(void*) * 65, oldProtect, &oldProtect);

        m_initialized.store(true);
        MAD_LOG("[D3D8Hook] DIRECT MEMORY HOOK SUCCESS: D3D8 Device at 0x%p hooked!", (void*)m_pDevice);
        MAD_LOG("[D3D8Hook] VTable Swapped: Reset(14), Present(15), EndScene(35), SetTransform(37), SetViewport(40), DrawPrim(70), DrawPrimUP(72), SetVS(76)");
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

        m_initialized.store(false);
        MAD_LOG("[D3D8Hook] Subsystem beendet.");
    }

    void D3D8Hook::OnEndScene(IDirect3DDevice8* pDevice) {
        m_pDevice = pDevice;
        // In Madagascar (2005) ruft RenderWare EndScene mehrfach pro Frame auf (Schattenpass, Weltpass, 2D UI).
        // Das tatsaechliche Rendern von ImGui findet strikt 1x pro Frame in OnPresent() statt,
        // um Multi-Layer Stacking und Ghosting restlos zu eliminieren!
    }

    void D3D8Hook::OnPresent(IDirect3DDevice8* pDevice) {
        m_pDevice = pDevice;
        uint64_t frame = ++m_frameCount;

        // 1. DeltaTime Berechnung für sanfte Bewegung & Cheats
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

        if (frame == 1 || frame == 10 || frame == 100 || frame % 1000 == 0) {
            MAD_LOG("[D3D8Hook] IDirect3DDevice8::Present Hook ACTIVE! Frame: %llu (dt: %.2f ms)", frame, m_lastDeltaTime * 1000.0f);
        }

        // 2. Hotkeys flankengesteuert verarbeiten
        ProcessHotkeys();

        // 3. RenderWare Kamera-Frustum kontinuierlich auf Widescreen halten
        UpdateRenderWareCameraFrustum();

        // 4. In-Game Cheat Manager pro Frame aktualisieren
        CheatManager::Instance().Update(m_lastDeltaTime);

        // 5. Lazy Initialization von ImGui beim ersten echten Present-Aufruf
        if (!m_imguiInitialized.load()) {
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

            MAD_LOG("[D3D8Hook] ImGui Initialisierung beim ersten Present Aufruf fuer HWND: 0x%08X...", (unsigned int)(uintptr_t)m_hGameWindow);

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

                ImGui::StyleColorsDark();
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

        // 6. STRICT FRAME GATE & COMPLETE D3D8 STATE PRESERVATION:
        // Rendern NUR wenn m_showOverlay aktiv ist - Exakt 1x pro Present auf den Backbuffer!
        if (m_imguiInitialized.load() && m_showOverlay.load()) {
            D3D8StateBackup backup{};
            SaveD3D8State(pDevice, backup);

            bool bSceneStarted = false;
            HRESULT hrBegin = pDevice->BeginScene();
            if (SUCCEEDED(hrBegin)) {
                bSceneStarted = true;
            }

            ImGui_ImplDX8_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            RenderOverlayUI();

            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplDX8_RenderDrawData(ImGui::GetDrawData());

            if (bSceneStarted) {
                pDevice->EndScene();
            }

            RestoreD3D8State(pDevice, backup);
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
        ImGui::SetNextWindowSize(ImVec2(540, 460), ImGuiCond_FirstUseEver);
        bool open = m_showOverlay.load();
        if (ImGui::Begin("Madagascar Multiplayer Client ###MadMultiplayerUI", &open)) {
            m_showOverlay.store(open);
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "MADAGASCAR (2005) - MULTIPLAYER OVERLAY");
            ImGui::TextDisabled("Hotkeys: [F2] Maus freigeben/fangen | [F3] Menue ein/aus | [Taste 9] 16:9 Widescreen");
            ImGui::Separator();

            if (ImGui::BeginTabBar("MainTabBar")) {
                if (ImGui::BeginTabItem("Multiplayer & State")) {
                    if (ImGui::CollapsingHeader("Steuerung & Maus-Fokus", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Text("Maus-Status: ");
                        ImGui::SameLine();
                        if (m_mouseInputMode.load()) {
                            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "[UI-BEDIENUNG] (Cursor frei)");
                        } else {
                            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[GAMEPLAY-MODUS] (Im Spiel gefangen - Taste F2)");
                        }

                        if (ImGui::Button(m_mouseInputMode.load() ? "Maus im Spiel fangen [F2]" : "Maus freigeben fuer UI [F2]", ImVec2(260, 28))) {
                            ToggleMouseMode();
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
        // 1. WM_SETCURSOR: Nur im UI-Bedienmodus Cursor freigeben; im Gameplay-Modus fesselt RenderWare ihn
        if (uMsg == WM_SETCURSOR) {
            if (m_mouseInputMode.load()) {
                UnlockMouseCursor();
                return TRUE;
            }
        }

        // 2. UI-Maus-Offset im Pausenmenue beheben (Widescreen Mouse Scaling)
        // Transformiert Screen-Koordinaten auf das urspruengliche 800x600-Format
        if (m_isBorderless && !m_mouseInputMode.load() && m_screenWidth > 0 && m_screenHeight > 0) {
            if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
                uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP) {
                short origX = (short)LOWORD(lParam);
                short origY = (short)HIWORD(lParam);
                short scaledX = (short)((float)origX * (800.0f / (float)m_screenWidth));
                short scaledY = (short)((float)origY * (600.0f / (float)m_screenHeight));
                lParam = MAKELPARAM(scaledX, scaledY);
            }
        }

        // 3. WM_MOUSEMOVE im UI-Modus
        if (uMsg == WM_MOUSEMOVE || uMsg == WM_NCMOUSEMOVE) {
            if (m_mouseInputMode.load() || GetForegroundWindow() != m_hGameWindow) {
                UnlockMouseCursor();
            }
        }

        // 4. Alt-Tab / Focus Loss
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

        // 5. Fenster-Drift Fix beim Draggen
        if (uMsg == WM_SYSCOMMAND) {
            DWORD cmd = (wParam & 0xFFF0);
            if (cmd == SC_MOVE || cmd == SC_SIZE) {
                UnlockMouseCursor();
            }
        }
        else if (uMsg == WM_ENTERSIZEMOVE) {
            UnlockMouseCursor();
        }

        // 6. ImGui Message Handler & Input Dispatch
        if (m_imguiInitialized.load()) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

            // Wenn UI-Bedienmodus aktiv ist: Alle Maus-Events abfangen,
            // damit Klicks im Menue nicht ins Gameplay durchdringen!
            if (m_mouseInputMode.load()) {
                if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) {
                    return 1;
                }
            }

            // Keyboard-Events fuer ImGui-Eingabefelder abfangen
            if (m_showOverlay.load()) {
                ImGuiIO& io = ImGui::GetIO();
                if (io.WantCaptureKeyboard && (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)) {
                    return 1;
                }
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

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_Present(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
        auto& hook = D3D8Hook::Instance();
        hook.m_frameRendered.store(false);
        hook.OnPresent(pDevice);
        return hook.m_pOriginalPresent ? hook.m_pOriginalPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) : D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_EndScene(IDirect3DDevice8* pDevice) {
        auto& hook = D3D8Hook::Instance();
        hook.OnEndScene(pDevice);
        return hook.m_pOriginalEndScene ? hook.m_pOriginalEndScene(pDevice) : D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_Reset(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
        auto& hook = D3D8Hook::Instance();
        hook.OnPreReset(pDevice);
        HRESULT hr = hook.m_pOriginalReset ? hook.m_pOriginalReset(pDevice, pPresentationParameters) : D3D_OK;
        if (SUCCEEDED(hr)) {
            hook.OnPostReset(pDevice);
        }
        return hr;
    }

    // FOV & Aspect Ratio Hor+ Korrektur (Slot 37: SetTransform)
    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_SetTransform(IDirect3DDevice8* pDevice, DWORD State, CONST D3DMATRIX* pMatrix) {
        auto& hook = D3D8Hook::Instance();
        // State 3 = D3DTS_PROJECTION
        if (hook.m_isBorderless && State == 3 && pMatrix && hook.m_screenHeight > 0) {
            D3DMATRIX modified = *pMatrix;
            float currentAspect = (float)hook.m_screenWidth / (float)hook.m_screenHeight;
            float originalAspect = 4.0f / 3.0f; // 1.333333f
            if (currentAspect > originalAspect) {
                // Horizontale Skalierung anpassen (Hor+ Modus):
                // In D3D projection matrix: _11 = cot(fovX / 2) = cot(fovY / 2) / aspect
                // Skaliere _11 um (4/3) / aspect, um das Seitenverhaeltnis exakt zu entzerren
                float factor = originalAspect / currentAspect;
                modified._11 *= factor;
                return hook.m_pOriginalSetTransform ? hook.m_pOriginalSetTransform(pDevice, State, &modified) : D3D_OK;
            }
        }
        return hook.m_pOriginalSetTransform ? hook.m_pOriginalSetTransform(pDevice, State, pMatrix) : D3D_OK;
    }

    // Viewport-Korrektur (Slot 40: SetViewport)
    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_SetViewport(IDirect3DDevice8* pDevice, CONST D3DVIEWPORT8* pViewport) {
        auto& hook = D3D8Hook::Instance();
        if (hook.m_isBorderless && pViewport && hook.m_screenWidth > 0 && hook.m_screenHeight > 0) {
            D3DVIEWPORT8 vp = *pViewport;
            // Falls RenderWare den Viewport auf alte 800x600 begrenzen will:
            if (vp.Width < (DWORD)hook.m_screenWidth || vp.Height < (DWORD)hook.m_screenHeight) {
                vp.X = 0;
                vp.Y = 0;
                vp.Width = (DWORD)hook.m_screenWidth;
                vp.Height = (DWORD)hook.m_screenHeight;
                vp.MinZ = 0.0f;
                vp.MaxZ = 1.0f;
                return hook.m_pOriginalSetViewport ? hook.m_pOriginalSetViewport(pDevice, &vp) : D3D_OK;
            }
        }
        return hook.m_pOriginalSetViewport ? hook.m_pOriginalSetViewport(pDevice, pViewport) : D3D_OK;
    }

    // 3. CUTSCENE-BALKEN (2D LETTERBOX OBEN LINKS ENTFERNEN)
    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_SetVertexShader(IDirect3DDevice8* pDevice, DWORD Handle) {
        auto& hook = D3D8Hook::Instance();
        hook.m_currentFVF = Handle;
        return hook.m_pOriginalSetVertexShader ? hook.m_pOriginalSetVertexShader(pDevice, Handle) : D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_DrawPrimitive(IDirect3DDevice8* pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
        auto& hook = D3D8Hook::Instance();
        return hook.m_pOriginalDrawPrimitive ? hook.m_pOriginalDrawPrimitive(pDevice, PrimitiveType, StartVertex, PrimitiveCount) : D3D_OK;
    }

    HRESULT STDMETHODCALLTYPE D3D8Hook::Hooked_DrawPrimitiveUP(IDirect3DDevice8* pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
        auto& hook = D3D8Hook::Instance();
        if (hook.m_isBorderless && pVertexStreamZeroData && PrimitiveCount <= 4) {
            // Pruefe ob es sich um 2D Screen-Space Cutscene Letterbox-Balken handelt
            if (hook.m_currentFVF & D3DFVF_XYZRHW) {
                const char* vBytes = reinterpret_cast<const char*>(pVertexStreamZeroData);
                struct Vertex2D { float x, y, z, rhw; DWORD color; };
                if (VertexStreamZeroStride >= sizeof(Vertex2D)) {
                    const Vertex2D* v0 = reinterpret_cast<const Vertex2D*>(vBytes);
                    const Vertex2D* v1 = reinterpret_cast<const Vertex2D*>(vBytes + VertexStreamZeroStride);
                    
                    // Schwarze Farbe (RGB == 0)
                    bool isBlack = ((v0->color & 0x00FFFFFF) == 0) && ((v1->color & 0x00FFFFFF) == 0);
                    if (isBlack) {
                        // Oberer Balken (y <= 120) oder unterer Balken (y >= 450)
                        bool isTopBar = (v0->y <= 120.0f && v1->y <= 120.0f);
                        bool isBottomBar = (v0->y >= 450.0f || v1->y >= 450.0f);
                        if (isTopBar || isBottomBar) {
                            // Letterbox Balken ueberspringen fuer sauberes 16:9 Vollbild
                            return D3D_OK;
                        }
                    }
                }
            }
        }
        return hook.m_pOriginalDrawPrimitiveUP ? hook.m_pOriginalDrawPrimitiveUP(pDevice, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride) : D3D_OK;
    }

    LRESULT CALLBACK D3D8Hook::Hooked_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        return D3D8Hook::Instance().HandleGameWndProc(hWnd, uMsg, wParam, lParam);
    }

    LRESULT CALLBACK D3D8Hook::Hooked_ConsoleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        return D3D8Hook::Instance().HandleConsoleWndProc(hWnd, uMsg, wParam, lParam);
    }

} // namespace MadMultiplayer
