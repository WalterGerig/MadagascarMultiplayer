#pragma once
#include <windows.h>
#include <atomic>
#include "PlayerTransform.h"
#include "Logger.h"
#include "CheatManager.h"
#include "../vendor/d3d8/d3d8_minimal.h"

namespace MadMultiplayer {

    class D3D8Hook {
    public:
        static D3D8Hook& Instance();

        void InitializeAsync();
        void Shutdown();

        // Versucht das Device aus RenderWare global (0x0062CE78) oder aus d3d8.dll zu lesen
        bool ForceHookD3D8Device();

        // Borderless Widescreen Modus (Taste 9) - Non-Reset Upscaling
        void ToggleBorderlessWindowed();
        bool IsBorderless() const { return m_isBorderless; }

        // Wird aufgerufen, sobald das echte IDirect3DDevice8 erzeugt oder ermittelt wird
        void OnDeviceCreated(IDirect3DDevice8* pDevice, HWND hGameWindow);

        // VTable Callback Handlers
        void OnPresent(IDirect3DDevice8* pDevice);
        void OnEndScene(IDirect3DDevice8* pDevice);
        void OnPreReset(IDirect3DDevice8* pDevice);
        void OnPostReset(IDirect3DDevice8* pDevice);

        // WndProc Handlers für sauberes Maus-Handling & Overlay
        LRESULT HandleGameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleConsoleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // F3: Overlay Sichtbarkeit
        bool IsOverlayVisible() const { return m_showOverlay.load(); }
        void ToggleOverlayVisibility();
        void ToggleOverlay() { ToggleOverlayVisibility(); } // Alias
        void SetOverlayVisibility(bool visible);
        void EnsureImGuiInitialized(IDirect3DDevice8* pDevice);

        // F2: UI / Maus-Modus (UI-Bedienung vs. Gameplay-Fesselung)
        bool IsMouseInputMode() const { return m_mouseInputMode.load(); }
        void ToggleMouseMode();
        void SetMouseMode(bool uiMouseMode);
        void ToggleUIMode() { ToggleMouseMode(); }
        void SetUIMode(bool active) { SetMouseMode(active); }
        bool IsUIModeActive() const { return IsMouseInputMode(); }
        void UpdateMouseCapture();

        // Flanken-gesteuertes Hotkey-Polling (wird in OnPresent/OnEndScene aufgerufen)
        void ProcessHotkeys();

        // RenderWare Frustum Culling Anpassung
        void UpdateRenderWareCameraFrustum();

        HWND GetGameWindow() const { return m_hGameWindow; }

        void SetTransformStore(ThreadSafeTransform* store) { m_pTransformStore = store; }
        bool HookDeviceVTable(void** vtable);

        // D3D8 Device State Preservation fuer sauberes ImGui-Rendern ohne Nebenwirkungen
        struct D3D8StateBackup {
            DWORD zEnable{ 0 };
            DWORD fillMode{ 0 };
            DWORD alphaBlend{ 0 };
            DWORD srcBlend{ 0 };
            DWORD destBlend{ 0 };
            DWORD cullMode{ 0 };
            DWORD lighting{ 0 };
            DWORD fogEnable{ 0 };
            DWORD alphaTest{ 0 };

            DWORD colorOp0{ 0 };
            DWORD alphaOp0{ 0 };
            DWORD colorArg1_0{ 0 };
            DWORD colorArg2_0{ 0 };
            DWORD alphaArg1_0{ 0 };
            DWORD alphaArg2_0{ 0 };
            DWORD minFilter0{ 0 };
            DWORD magFilter0{ 0 };

            DWORD colorOp1{ 0 };
            DWORD alphaOp1{ 0 };
            DWORD minFilter1{ 0 };
            DWORD magFilter1{ 0 };

            DWORD vertexShader{ 0 };
            IDirect3DVertexBuffer8* streamSource0{ nullptr };
            UINT streamStride0{ 0 };
            IDirect3DIndexBuffer8* indexBuffer{ nullptr };
            UINT baseVertexIndex{ 0 };

            IDirect3DBaseTexture8* texture0{ nullptr };
            IDirect3DBaseTexture8* texture1{ nullptr };

            D3DVIEWPORT8 viewport{};
        };

        static void SaveD3D8State(IDirect3DDevice8* pDevice, D3D8StateBackup& backup);
        static void RestoreD3D8State(IDirect3DDevice8* pDevice, const D3D8StateBackup& backup);

    private:
        D3D8Hook() = default;
        ~D3D8Hook();

        static DWORD WINAPI InitThreadProc(LPVOID lpParam);
        void RenderOverlayUI();
        void UnlockMouseCursor();

        using PFN_Reset   = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, D3DPRESENT_PARAMETERS*);
        using PFN_Present = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
        using PFN_EndScene = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*);

        PFN_Reset   m_pOriginalReset{ nullptr };
        PFN_Present m_pOriginalPresent{ nullptr };
        PFN_EndScene m_pOriginalEndScene{ nullptr };

        IDirect3DDevice8* m_pDevice{ nullptr };
        HWND              m_hGameWindow{ nullptr };
        HWND              m_hConsoleWindow{ nullptr };
        WNDPROC           m_pOriginalGameWndProc{ nullptr };
        WNDPROC           m_pOriginalConsoleWndProc{ nullptr };

        std::atomic<bool>     m_initialized{ false };
        std::atomic<bool>     m_imguiInitialized{ false };
        std::atomic<uint64_t> m_frameCount{ 0 };
        std::atomic<bool>     m_frameRendered{ false };

        // Frame Delta Time Berechnung
        LARGE_INTEGER         m_lastFrameTime{ 0 };
        LARGE_INTEGER         m_perfFreq{ 0 };
        float                 m_lastDeltaTime{ 0.0166f };

        // 1. Getrennte Hotkey-Zustände (F2 = Maus-Modus, F3 = Menü-Sichtbarkeit)
        std::atomic<bool>     m_showOverlay{ true };       // Standardmäßig beim Start sichtbar
        std::atomic<bool>     m_mouseInputMode{ false };   // Standardmäßig Gameplay-Modus (Cursor gefangen)

        // 2. Non-Reset Widescreen Zustände (Taste 9)
        bool                  m_isBorderless{ false };
        RECT                  m_prevWindowRect{ 0, 0, 0, 0 };
        LONG                  m_prevWindowStyle{ 0 };
        int                   m_screenWidth{ 800 };
        int                   m_screenHeight{ 600 };

        ThreadSafeTransform*  m_pTransformStore{ nullptr };

        // UI Eingabefelder
        char m_serverIp[64]{ "127.0.0.1" };
        int  m_serverPort{ 27015 };
        char m_playerName[32]{ "Player 1" };
        bool m_isConnected{ false };

        // Statische Hooks für Minimal-VTable (Reset, Present, EndScene) & WndProc
        static HRESULT STDMETHODCALLTYPE Hooked_Reset(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
        static HRESULT STDMETHODCALLTYPE Hooked_Present(IDirect3DDevice8* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
        static HRESULT STDMETHODCALLTYPE Hooked_EndScene(IDirect3DDevice8* pDevice);

        static LRESULT CALLBACK Hooked_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK Hooked_ConsoleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        using PFN_GetCursorPos = BOOL(WINAPI*)(LPPOINT);
        static PFN_GetCursorPos s_pOriginalGetCursorPos;
        static BOOL WINAPI Hooked_GetCursorPos(LPPOINT lpPoint);
    };

    // Globale Zustandsvariablen fuer strikte modale Eingabe-Isolation (F2) & Overlay-Sichtbarkeit (F3)
    extern bool g_bUIModeActive;
    void ToggleUIMode();
    void SetUIMode(bool active);
    bool IsUIModeActive();

    extern bool g_bOverlayVisible;
    void ToggleOverlay();
    void SetOverlayVisible(bool visible);
    bool IsOverlayVisible();

} // namespace MadMultiplayer

using MadMultiplayer::g_bUIModeActive;
using MadMultiplayer::ToggleUIMode;
using MadMultiplayer::SetUIMode;
using MadMultiplayer::IsUIModeActive;

using MadMultiplayer::g_bOverlayVisible;
using MadMultiplayer::ToggleOverlay;
using MadMultiplayer::SetOverlayVisible;
using MadMultiplayer::IsOverlayVisible;
