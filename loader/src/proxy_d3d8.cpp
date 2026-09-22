// d3d8.dll proxy entry point with IDirect3D8Proxy COM interception.

#include <windows.h>
#include "loader.h"
#include "d3d8_minimal.h"

namespace {

HMODULE           g_real = nullptr;
IDirect3DDevice8* g_pCapturedDevice = nullptr;
HWND              g_capturedHwnd = nullptr;

using PFN_Direct3DCreate8      = void*    (WINAPI*)(UINT);
using PFN_ValidatePixelShader  = HRESULT  (WINAPI*)(DWORD*, DWORD*, BOOL, DWORD*);
using PFN_ValidateVertexShader = HRESULT  (WINAPI*)(DWORD*, DWORD*, DWORD*, BOOL, DWORD*);
using PFN_DebugSetMute         = void     (WINAPI*)(void);
using PFN_MaxWindowedShim      = void*    (WINAPI*)(UINT);

PFN_Direct3DCreate8      p_Direct3DCreate8      = nullptr;
PFN_ValidatePixelShader  p_ValidatePixelShader  = nullptr;
PFN_ValidateVertexShader p_ValidateVertexShader = nullptr;
PFN_DebugSetMute         p_DebugSetMute         = nullptr;
PFN_MaxWindowedShim      p_MaxWindowedShim      = nullptr;

void LoadReal() {
    char path[MAX_PATH];

    // 1. a chained wrapper next to the game, if present
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    char* slash = nullptr;
    for (char* p = path; *p; ++p) if (*p == '\\') slash = p;
    if (slash) slash[1] = '\0';
    lstrcatA(path, "d3d8_orig.dll");

    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
        g_real = LoadLibraryA(path);
        madloader::Log("[MadLoader] d3d8 chain -> %s (%p)", path, (void*)g_real);
    }

    // 2. otherwise the system copy
    if (!g_real) {
        GetSystemDirectoryA(path, MAX_PATH);
        lstrcatA(path, "\\d3d8.dll");
        g_real = LoadLibraryA(path);
        madloader::Log("[MadLoader] d3d8 real  -> %s (%p)", path, (void*)g_real);
    }

    if (!g_real) {
        madloader::Log("[MadLoader] FATAL: no d3d8 implementation found (err=%lu)", GetLastError());
        return;
    }

    p_Direct3DCreate8      = (PFN_Direct3DCreate8)      GetProcAddress(g_real, "Direct3DCreate8");
    p_ValidatePixelShader  = (PFN_ValidatePixelShader)  GetProcAddress(g_real, "ValidatePixelShader");
    p_ValidateVertexShader = (PFN_ValidateVertexShader) GetProcAddress(g_real, "ValidateVertexShader");
    p_DebugSetMute         = (PFN_DebugSetMute)         GetProcAddress(g_real, "DebugSetMute");
    p_MaxWindowedShim      = (PFN_MaxWindowedShim)      GetProcAddress(g_real, "Direct3D8EnableMaximizedWindowedModeShim");
}

void EnsureReal() {
    static LONG once = 0;
    if (InterlockedCompareExchange(&once, 1, 0) == 0) LoadReal();
}

// =============================================================================
// IDirect3D8 PROXY WRAPPER TO CAPTURE REAL DEVICE ON CreateDevice
// =============================================================================
class Direct3D8Proxy : public IDirect3D8 {
public:
    IDirect3D8* m_realD3D;
    Direct3D8Proxy(IDirect3D8* realD3D) : m_realD3D(realD3D) {}

    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj) override {
        return m_realD3D->QueryInterface(riid, ppvObj);
    }
    STDMETHOD_(ULONG, AddRef)() override {
        return m_realD3D->AddRef();
    }
    STDMETHOD_(ULONG, Release)() override {
        ULONG count = m_realD3D->Release();
        if (count == 0) delete this;
        return count;
    }

    STDMETHOD(RegisterSoftwareDevice)(void* pInitializeFunction) override {
        return m_realD3D->RegisterSoftwareDevice(pInitializeFunction);
    }
    STDMETHOD_(UINT, GetAdapterCount)() override {
        return m_realD3D->GetAdapterCount();
    }
    STDMETHOD(GetAdapterIdentifier)(UINT Adapter, DWORD Flags, void* pIdentifier) override {
        return m_realD3D->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
    }
    STDMETHOD_(UINT, GetAdapterModeCount)(UINT Adapter) override {
        return m_realD3D->GetAdapterModeCount(Adapter);
    }
    STDMETHOD(EnumAdapterModes)(UINT Adapter, UINT Mode, void* pMode) override {
        return m_realD3D->EnumAdapterModes(Adapter, Mode, pMode);
    }
    STDMETHOD(GetAdapterDisplayMode)(UINT Adapter, void* pMode) override {
        return m_realD3D->GetAdapterDisplayMode(Adapter, pMode);
    }
    STDMETHOD(CheckDeviceType)(UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed) override {
        return m_realD3D->CheckDeviceType(Adapter, CheckType, DisplayFormat, BackBufferFormat, Windowed);
    }
    STDMETHOD(CheckDeviceFormat)(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, DWORD RType, D3DFORMAT CheckFormat) override {
        return m_realD3D->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
    }
    STDMETHOD(CheckDeviceMultiSampleType)(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, DWORD MultiSampleType) override {
        return m_realD3D->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType);
    }
    STDMETHOD(CheckDepthStencilMatch)(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override {
        return m_realD3D->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);
    }
    STDMETHOD(GetDeviceCaps)(UINT Adapter, D3DDEVTYPE DeviceType, void* pCaps) override {
        return m_realD3D->GetDeviceCaps(Adapter, DeviceType, pCaps);
    }
    STDMETHOD_(HMONITOR, GetAdapterMonitor)(UINT Adapter) override {
        return m_realD3D->GetAdapterMonitor(Adapter);
    }

    STDMETHOD(CreateDevice)(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface) override {
        madloader::Log("[proxy_d3d8] IDirect3D8::CreateDevice intercepting (hFocusWindow: %p)...", (void*)hFocusWindow);
        HRESULT hr = m_realD3D->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
        
        if (SUCCEEDED(hr) && ppReturnedDeviceInterface && *ppReturnedDeviceInterface) {
            g_pCapturedDevice = *ppReturnedDeviceInterface;
            g_capturedHwnd = hFocusWindow;
            madloader::Log("[proxy_d3d8] Real RenderWare IDirect3DDevice8 captured: %p (HWND: %p)", (void*)g_pCapturedDevice, (void*)g_capturedHwnd);

            HMODULE hMultiplayer = GetModuleHandleA("MadMultiplayer.dll");
            if (hMultiplayer) {
                using PFN_ForceHook = void(__cdecl*)(IDirect3DDevice8*, HWND);
                auto forceHook = (PFN_ForceHook)GetProcAddress(hMultiplayer, "MadMultiplayer_ForceHookDevice");
                if (forceHook) {
                    madloader::Log("[proxy_d3d8] Forcing MadMultiplayer_ForceHookDevice directly!");
                    forceHook(g_pCapturedDevice, g_capturedHwnd);
                } else {
                    using PFN_Notify = void(__cdecl*)(void*, HWND);
                    auto notify = (PFN_Notify)GetProcAddress(hMultiplayer, "MadMultiplayer_OnDeviceCreated");
                    if (notify) notify(g_pCapturedDevice, g_capturedHwnd);
                }
            }
        }
        return hr;
    }
};

} // namespace

extern "C" {

__declspec(dllexport) IDirect3DDevice8* __cdecl GetForcedD3D8Device() {
    return g_pCapturedDevice;
}

IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion) {
    EnsureReal();
    if (!p_Direct3DCreate8) return nullptr;
    IDirect3D8* realD3D = reinterpret_cast<IDirect3D8*>(p_Direct3DCreate8(SDKVersion));
    if (!realD3D) return nullptr;

    madloader::Log("[proxy_d3d8] Direct3DCreate8 returning Direct3D8Proxy instance.");
    return new Direct3D8Proxy(realD3D);
}

HRESULT WINAPI ValidatePixelShader(DWORD* a, DWORD* b, BOOL c, DWORD* d) {
    EnsureReal();
    return p_ValidatePixelShader ? p_ValidatePixelShader(a, b, c, d) : E_FAIL;
}

HRESULT WINAPI ValidateVertexShader(DWORD* a, DWORD* b, DWORD* c, BOOL d, DWORD* e) {
    EnsureReal();
    return p_ValidateVertexShader ? p_ValidateVertexShader(a, b, c, d, e) : E_FAIL;
}

void WINAPI DebugSetMute(void) {
    EnsureReal();
    if (p_DebugSetMute) p_DebugSetMute();
}

void* WINAPI Direct3D8EnableMaximizedWindowedModeShim(UINT a) {
    EnsureReal();
    return p_MaxWindowedShim ? p_MaxWindowedShim(a) : nullptr;
}

__declspec(dllexport) void* __cdecl MadLoader_GetD3D8Device() {
    return g_pCapturedDevice;
}

__declspec(dllexport) HWND __cdecl MadLoader_GetD3D8FocusWindow() {
    return g_capturedHwnd;
}

} // extern "C"

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hMod);
        madloader::Start("d3d8-proxy");
    }
    return TRUE;
}
