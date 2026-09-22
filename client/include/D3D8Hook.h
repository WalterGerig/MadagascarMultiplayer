#pragma once
#include <windows.h>
#include <atomic>
#include "PlayerTransform.h"
#include "Logger.h"
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

        // F2: Maus-Modus (UI-Bedienung vs. Gameplay-Fesselung)
        bool IsMouseInputMode() const { return m_mouseInputMode.load(); }
        void ToggleMouseMode();
        void SetMouseMode(bool uiMouseMode);
        void UpdateMouseCapture();

        // Flanken-gesteuertes Hotkey-Polling (wird in OnEndScene aufgerufen)
        void ProcessHotkeys();

        // RenderWare Frustum Culling Anpassung
        void UpdateRenderWareCameraFrustum();

        HWND GetGameWindow() const { return m_hGameWindow; }

        void SetTransformStore(ThreadSafeTransform* store) { m_pTransformStore = store; }
        bool HookDeviceVTable(void** vtable);

    private:
        D3D8Hook() = default;
        ~D3D8Hook();

        static DWORD WINAPI InitThreadProc(LPVOID lpParam);
        void RenderOverlayUI();
        void UnlockMouseCursor();

        using PFN_EndScene        = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*);
        using PFN_Reset           = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, D3DPRESENT_PARAMETERS*);
        using PFN_SetTransform    = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, DWORD, CONST D3DMATRIX*);
        using PFN_SetViewport     = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, CONST D3DVIEWPORT8*);
        using PFN_DrawPrimitive   = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, D3DPRIMITIVETYPE, UINT, UINT);
        using PFN_DrawPrimitiveUP = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, D3DPRIMITIVETYPE, UINT, CONST void*, UINT);
        using PFN_SetVertexShader = HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice8*, DWORD);

        PFN_EndScene        m_pOriginalEndScene{ nullptr };
        PFN_Reset           m_pOriginalReset{ nullptr };
        PFN_SetTransform    m_pOriginalSetTransform{ nullptr };
        PFN_SetViewport     m_pOriginalSetViewport{ nullptr };
        PFN_DrawPrimitive   m_pOriginalDrawPrimitive{ nullptr };
        PFN_DrawPrimitiveUP m_pOriginalDrawPrimitiveUP{ nullptr };
        PFN_SetVertexShader m_pOriginalSetVertexShader{ nullptr };

        IDirect3DDevice8* m_pDevice{ nullptr };
        HWND              m_hGameWindow{ nullptr };
        HWND              m_hConsoleWindow{ nullptr };
        WNDPROC           m_pOriginalGameWndProc{ nullptr };
        WNDPROC           m_pOriginalConsoleWndProc{ nullptr };

        std::atomic<bool>     m_initialized{ false };
        std::atomic<bool>     m_imguiInitialized{ false };
        std::atomic<uint64_t> m_frameCount{ 0 };

        // 1. Getrennte Hotkey-Zustände (F2 = Maus-Modus, F3 = Menü-Sichtbarkeit)
        std::atomic<bool>     m_showOverlay{ true };       // Standardmäßig beim Start sichtbar
        std::atomic<bool>     m_mouseInputMode{ false };   // Standardmäßig Gameplay-Modus (Cursor gefangen)

        // 2. Non-Reset Widescreen Zustände (Taste 9)
        bool                  m_isBorderless{ false };
        RECT                  m_prevWindowRect{ 0, 0, 0, 0 };
        LONG                  m_prevWindowStyle{ 0 };
        int                   m_screenWidth{ 800 };
        int                   m_screenHeight{ 600 };
        DWORD                 m_currentFVF{ 0 };

        ThreadSafeTransform*  m_pTransformStore{ nullptr };

        // UI Eingabefelder
        char m_serverIp[64]{ "127.0.0.1" };
        int  m_serverPort{ 27015 };
        char m_playerName[32]{ "Player 1" };
        bool m_isConnected{ false };

        // Statische Hooks für VTable & WndProc
        static HRESULT STDMETHODCALLTYPE Hooked_EndScene(IDirect3DDevice8* pDevice);
        static HRESULT STDMETHODCALLTYPE Hooked_Reset(IDirect3DDevice8* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
        static HRESULT STDMETHODCALLTYPE Hooked_SetTransform(IDirect3DDevice8* pDevice, DWORD State, CONST D3DMATRIX* pMatrix);
        static HRESULT STDMETHODCALLTYPE Hooked_SetViewport(IDirect3DDevice8* pDevice, CONST D3DVIEWPORT8* pViewport);
        static HRESULT STDMETHODCALLTYPE Hooked_DrawPrimitive(IDirect3DDevice8* pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount);
        static HRESULT STDMETHODCALLTYPE Hooked_DrawPrimitiveUP(IDirect3DDevice8* pDevice, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
        static HRESULT STDMETHODCALLTYPE Hooked_SetVertexShader(IDirect3DDevice8* pDevice, DWORD Handle);

        static LRESULT CALLBACK Hooked_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK Hooked_ConsoleWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    };

} // namespace MadMultiplayer
