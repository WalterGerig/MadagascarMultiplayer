#pragma once
#include <windows.h>
#include <unknwn.h>

// Direct3D 8 SDK Version
#define D3D_SDK_VERSION 220
#define D3D_OK          S_OK

// Basic D3D8 types & macros
typedef DWORD D3DCOLOR;
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#define D3DCOLOR_RGBA(r,g,b,a) D3DCOLOR_ARGB(a,r,g,b)

typedef struct _D3DMATRIX {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };
} D3DMATRIX;

typedef struct _D3DVIEWPORT8 {
    DWORD X;
    DWORD Y;
    DWORD Width;
    DWORD Height;
    float MinZ;
    float MaxZ;
} D3DVIEWPORT8;

typedef struct _D3DRECT {
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
} D3DRECT;

typedef enum _D3DFORMAT {
    D3DFMT_UNKNOWN          =  0,
    D3DFMT_R8G8B8           = 20,
    D3DFMT_A8R8G8B8         = 21,
    D3DFMT_X8R8G8B8         = 22,
    D3DFMT_R5G6B5           = 23,
    D3DFMT_X1R5G5B5         = 24,
    D3DFMT_A1R5G5B5         = 25,
    D3DFMT_A4R4G4B4         = 26,
    D3DFMT_INDEX16          = 101,
    D3DFMT_INDEX32          = 102,
    D3DFMT_FORCE_DWORD      = 0x7fffffff
} D3DFORMAT;

typedef enum _D3DPOOL {
    D3DPOOL_DEFAULT         = 0,
    D3DPOOL_MANAGED         = 1,
    D3DPOOL_SYSTEMMEM       = 2,
    D3DPOOL_SCRATCH         = 3,
    D3DPOOL_FORCE_DWORD     = 0x7fffffff
} D3DPOOL;

typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST         = 1,
    D3DPT_LINELIST          = 2,
    D3DPT_LINESTRIP         = 3,
    D3DPT_TRIANGLELIST      = 4,
    D3DPT_TRIANGLESTRIP     = 5,
    D3DPT_TRIANGLEFAN       = 6,
    D3DPT_FORCE_DWORD       = 0x7fffffff
} D3DPRIMITIVETYPE;

typedef enum _D3DRENDERSTATETYPE {
    D3DRS_ZENABLE           = 7,
    D3DRS_FILLMODE          = 8,
    D3DRS_SHADEMODE         = 9,
    D3DRS_ZWRITEENABLE      = 14,
    D3DRS_ALPHATESTENABLE   = 15,
    D3DRS_LASTPIXEL         = 16,
    D3DRS_SRCBLEND          = 19,
    D3DRS_DESTBLEND         = 20,
    D3DRS_CULLMODE          = 22,
    D3DRS_ZFUNC             = 23,
    D3DRS_ALPHAREF          = 24,
    D3DRS_ALPHAFUNC         = 25,
    D3DRS_DITHERENABLE      = 26,
    D3DRS_ALPHABLENDENABLE  = 27,
    D3DRS_FOGENABLE         = 28,
    D3DRS_SPECULARENABLE    = 29,
    D3DRS_FOGCOLOR          = 34,
    D3DRS_FOGTABLEMODE      = 35,
    D3DRS_FOGSTART          = 36,
    D3DRS_FOGEND            = 37,
    D3DRS_FOGDENSITY        = 38,
    D3DRS_RANGEFOGENABLE    = 48,
    D3DRS_STENCILENABLE     = 52,
    D3DRS_STENCILFAIL       = 53,
    D3DRS_STENCILZFAIL      = 54,
    D3DRS_STENCILPASS       = 55,
    D3DRS_STENCILFUNC       = 56,
    D3DRS_STENCILREF        = 57,
    D3DRS_STENCILMASK       = 58,
    D3DRS_STENCILWRITEMASK  = 59,
    D3DRS_TEXTUREFACTOR     = 60,
    D3DRS_WRAP0             = 128,
    D3DRS_CLIPPING          = 136,
    D3DRS_LIGHTING          = 137,
    D3DRS_COLORVERTEX       = 141,
    D3DRS_LOCALVIEWER       = 142,
    D3DRS_NORMALIZENORMALS  = 143,
    D3DRS_DIFFUSEMATERIALSOURCE = 145,
    D3DRS_SPECULARMATERIALSOURCE= 146,
    D3DRS_AMBIENTMATERIALSOURCE = 147,
    D3DRS_EMISSIVEMATERIALSOURCE= 148,
    D3DRS_VERTEXBLEND       = 151,
    D3DRS_CLIPPLANEENABLE   = 152,
    D3DRS_MULTISAMPLEANTIALIAS = 161,
    D3DRS_MULTISAMPLEMASK   = 162,
    D3DRS_PATCHEDGESTYLE    = 163,
    D3DRS_COLORWRITEENABLE  = 168,
    D3DRS_BLENDOP           = 171,
    D3DRS_SCISSORTESTENABLE = 174,
    D3DRS_FORCE_DWORD       = 0x7fffffff
} D3DRENDERSTATETYPE;

typedef enum _D3DTRANSFORMSTATETYPE {
    D3DTS_VIEW          = 2,
    D3DTS_PROJECTION    = 3,
    D3DTS_TEXTURE0      = 16,
    D3DTS_TEXTURE1      = 17,
    D3DTS_FORCE_DWORD   = 0x7fffffff
} D3DTRANSFORMSTATETYPE;

#define D3DTS_WORLD (D3DTRANSFORMSTATETYPE)256

typedef enum _D3DTEXTURESTAGESTATETYPE {
    D3DTSS_COLOROP          = 1,
    D3DTSS_COLORARG1        = 2,
    D3DTSS_COLORARG2        = 3,
    D3DTSS_ALPHAOP          = 4,
    D3DTSS_ALPHAARG1        = 5,
    D3DTSS_ALPHAARG2        = 6,
    D3DTSS_TEXCOORDINDEX    = 11,
    D3DTSS_ADDRESSU         = 13,
    D3DTSS_ADDRESSV         = 14,
    D3DTSS_MINFILTER        = 16,
    D3DTSS_MAGFILTER        = 17,
    D3DTSS_MIPFILTER        = 18,
    D3DTSS_TEXTURETRANSFORMFLAGS = 24,
    D3DTSS_FORCE_DWORD      = 0x7fffffff
} D3DTEXTURESTAGESTATETYPE;

#define D3DTTFF_DISABLE         0
#define D3DTTFF_COUNT1          1
#define D3DTTFF_COUNT2          2
#define D3DTTFF_COUNT3          3
#define D3DTTFF_COUNT4          4
#define D3DTTFF_PROJECTED       256

#define D3DFILL_POINT           1
#define D3DFILL_WIREFRAME       2
#define D3DFILL_SOLID           3

#define D3DTADDRESS_WRAP        1
#define D3DTADDRESS_MIRROR      2
#define D3DTADDRESS_CLAMP       3
#define D3DTADDRESS_BORDER      4
#define D3DTADDRESS_MIRRORONCE  5

#define D3DTEXF_NONE            0
#define D3DTEXF_POINT           1
#define D3DTEXF_LINEAR          2
#define D3DTEXF_ANISOTROPIC     3

typedef enum _D3DTEXTUREOP {
    D3DTOP_DISABLE          = 1,
    D3DTOP_SELECTARG1       = 2,
    D3DTOP_SELECTARG2       = 3,
    D3DTOP_MODULATE         = 4,
    D3DTOP_FORCE_DWORD      = 0x7fffffff
} D3DTEXTUREOP;

#define D3DTA_SELECTMASK        0x0000000f
#define D3DTA_DIFFUSE           0x00000000
#define D3DTA_CURRENT           0x00000001
#define D3DTA_TEXTURE           0x00000002

#define D3DBLEND_ZERO           1
#define D3DBLEND_ONE            2
#define D3DBLEND_SRCCOLOR       3
#define D3DBLEND_INVSRCCOLOR    4
#define D3DBLEND_SRCALPHA       5
#define D3DBLEND_INVSRCALPHA    6
#define D3DBLEND_DESTALPHA      7
#define D3DBLEND_INVDESTALPHA   8
#define D3DBLEND_DESTCOLOR      9
#define D3DBLEND_INVDESTCOLOR   10

#define D3DCULL_NONE            1
#define D3DCULL_CW              2
#define D3DCULL_CCW             3

#define D3DFVF_XYZRHW           0x004
#define D3DFVF_DIFFUSE          0x040
#define D3DFVF_TEX1             0x100

#define D3DUSAGE_RENDERTARGET   (0x00000001L)
#define D3DUSAGE_DEPTHSTENCIL   (0x00000002L)
#define D3DUSAGE_WRITEONLY      (0x00000008L)
#define D3DUSAGE_DYNAMIC        (0x00000200L)

#define D3DLOCK_READONLY        0x00000010L
#define D3DLOCK_DISCARD         0x00002000L
#define D3DLOCK_NOOVERWRITE     0x00001000L

#define D3DSWAPEFFECT_DISCARD   1
#define D3DSWAPEFFECT_FLIP      2
#define D3DSWAPEFFECT_COPY      3

typedef enum _D3DDEVTYPE {
    D3DDEVTYPE_HAL          = 1,
    D3DDEVTYPE_REF          = 2,
    D3DDEVTYPE_SW           = 3,
    D3DDEVTYPE_FORCE_DWORD  = 0x7fffffff
} D3DDEVTYPE;

#define D3DADAPTER_DEFAULT      0
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0x00000020L
#define D3DCREATE_HARDWARE_VERTEXPROCESSING 0x00000040L

typedef struct _D3DDEVICE_CREATION_PARAMETERS {
    UINT        AdapterOrdinal;
    D3DDEVTYPE  DeviceType;
    HWND        hFocusWindow;
    DWORD       BehaviorFlags;
} D3DDEVICE_CREATION_PARAMETERS;

typedef struct _D3DPRESENT_PARAMETERS_ {
    UINT                BackBufferWidth;
    UINT                BackBufferHeight;
    D3DFORMAT           BackBufferFormat;
    UINT                BackBufferCount;
    DWORD               MultiSampleType;
    DWORD               SwapEffect;
    HWND                hDeviceWindow;
    BOOL                Windowed;
    BOOL                EnableAutoDepthStencil;
    D3DFORMAT           AutoDepthStencilFormat;
    DWORD               Flags;
    UINT                FullScreen_RefreshRateInHz;
    UINT                FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS;

typedef struct _D3DLOCKED_RECT {
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;

typedef struct _D3DINDEXBUFFER_DESC {
    D3DFORMAT           Format;
    DWORD               Type;
    DWORD               Usage;
    D3DPOOL             Pool;
    UINT                Size;
} D3DINDEXBUFFER_DESC;

typedef struct _D3DVERTEXBUFFER_DESC {
    D3DFORMAT           Format;
    DWORD               Type;
    DWORD               Usage;
    D3DPOOL             Pool;
    UINT                Size;
    DWORD               FVF;
} D3DVERTEXBUFFER_DESC;

typedef enum _D3DBACKBUFFER_TYPE {
    D3DBACKBUFFER_TYPE_MONO         = 0,
    D3DBACKBUFFER_TYPE_LEFT         = 1,
    D3DBACKBUFFER_TYPE_RIGHT        = 2,
    D3DBACKBUFFER_TYPE_FORCE_DWORD  = 0x7fffffff
} D3DBACKBUFFER_TYPE;

typedef struct _D3DSURFACE_DESC {
    D3DFORMAT           Format;
    DWORD               Type;
    DWORD               Usage;
    D3DPOOL             Pool;
    DWORD               MultiSampleType;
    UINT                Width;
    UINT                Height;
} D3DSURFACE_DESC;

// Forward declarations of interfaces
struct IDirect3D8;
struct IDirect3DDevice8;
struct IDirect3DResource8;
struct IDirect3DBaseTexture8;
struct IDirect3DTexture8;
struct IDirect3DVertexBuffer8;
struct IDirect3DIndexBuffer8;
struct IDirect3DSurface8;

// Interfaces definitions using COM macro standard
#undef INTERFACE
#define INTERFACE IDirect3DResource8
DECLARE_INTERFACE_(IDirect3DResource8, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
    STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
    STDMETHOD_(void, PreLoad)(THIS) PURE;
    STDMETHOD_(DWORD, GetType)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DBaseTexture8
DECLARE_INTERFACE_(IDirect3DBaseTexture8, IDirect3DResource8)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
    STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
    STDMETHOD_(void, PreLoad)(THIS) PURE;
    STDMETHOD_(DWORD, GetType)(THIS) PURE;
    STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew) PURE;
    STDMETHOD_(DWORD, GetLOD)(THIS) PURE;
    STDMETHOD_(DWORD, GetLevelCount)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DTexture8
DECLARE_INTERFACE_(IDirect3DTexture8, IDirect3DBaseTexture8)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
    STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
    STDMETHOD_(void, PreLoad)(THIS) PURE;
    STDMETHOD_(DWORD, GetType)(THIS) PURE;
    STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew) PURE;
    STDMETHOD_(DWORD, GetLOD)(THIS) PURE;
    STDMETHOD_(DWORD, GetLevelCount)(THIS) PURE;
    STDMETHOD(GetLevelDesc)(THIS_ UINT Level, void* pDesc) PURE;
    STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level, IDirect3DSurface8** ppSurfaceLevel) PURE;
    STDMETHOD(LockRect)(THIS_ UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) PURE;
    STDMETHOD(UnlockRect)(THIS_ UINT Level) PURE;
    STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DVertexBuffer8
DECLARE_INTERFACE_(IDirect3DVertexBuffer8, IDirect3DResource8)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
    STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
    STDMETHOD_(void, PreLoad)(THIS) PURE;
    STDMETHOD_(DWORD, GetType)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) PURE;
    STDMETHOD(Unlock)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ void* pDesc) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DIndexBuffer8
DECLARE_INTERFACE_(IDirect3DIndexBuffer8, IDirect3DResource8)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
    STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
    STDMETHOD_(void, PreLoad)(THIS) PURE;
    STDMETHOD_(DWORD, GetType)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) PURE;
    STDMETHOD(Unlock)(THIS) PURE;
    STDMETHOD(GetDesc)(THIS_ void* pDesc) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DSurface8
DECLARE_INTERFACE_(IDirect3DSurface8, IDirect3DResource8)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice8** ppDevice) PURE;
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) PURE;
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) PURE;
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) PURE;
    STDMETHOD_(DWORD, GetPriority)(THIS) PURE;
    STDMETHOD_(void, PreLoad)(THIS) PURE;
    STDMETHOD_(DWORD, GetType)(THIS) PURE;
    STDMETHOD(GetContainer)(THIS_ REFIID riid, void** ppContainer) PURE;
    STDMETHOD(GetDesc)(THIS_ D3DSURFACE_DESC* pDesc) PURE;
    STDMETHOD(LockRect)(THIS_ D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) PURE;
    STDMETHOD(UnlockRect)(THIS) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3D8
DECLARE_INTERFACE_(IDirect3D8, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(RegisterSoftwareDevice)(THIS_ void* pInitializeFunction) PURE;
    STDMETHOD_(UINT, GetAdapterCount)(THIS) PURE;
    STDMETHOD(GetAdapterIdentifier)(THIS_ UINT Adapter, DWORD Flags, void* pIdentifier) PURE;
    STDMETHOD_(UINT, GetAdapterModeCount)(THIS_ UINT Adapter) PURE;
    STDMETHOD(EnumAdapterModes)(THIS_ UINT Adapter, UINT Mode, void* pMode) PURE;
    STDMETHOD(GetAdapterDisplayMode)(THIS_ UINT Adapter, void* pMode) PURE;
    STDMETHOD(CheckDeviceType)(THIS_ UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed) PURE;
    STDMETHOD(CheckDeviceFormat)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, DWORD RType, D3DFORMAT CheckFormat) PURE;
    STDMETHOD(CheckDeviceMultiSampleType)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, DWORD MultiSampleType) PURE;
    STDMETHOD(CheckDepthStencilMatch)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) PURE;
    STDMETHOD(GetDeviceCaps)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, void* pCaps) PURE;
    STDMETHOD_(HMONITOR, GetAdapterMonitor)(THIS_ UINT Adapter) PURE;
    STDMETHOD(CreateDevice)(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice8** ppReturnedDeviceInterface) PURE;
};

#undef INTERFACE
#define INTERFACE IDirect3DDevice8
DECLARE_INTERFACE_(IDirect3DDevice8, IUnknown)
{
    // IUnknown (0..2)
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    // IDirect3DDevice8 (3..13)
    STDMETHOD(TestCooperativeLevel)(THIS) PURE;
    STDMETHOD_(UINT, GetAvailableTextureMem)(THIS) PURE;
    STDMETHOD(ResourceManagerDiscardBytes)(THIS_ DWORD Bytes) PURE;
    STDMETHOD(GetDirect3D)(THIS_ IDirect3D8** ppD3D8) PURE;
    STDMETHOD(GetDeviceCaps)(THIS_ void* pCaps) PURE;
    STDMETHOD(GetDisplayMode)(THIS_ void* pMode) PURE;
    STDMETHOD(GetCreationParameters)(THIS_ void* pParameters) PURE;
    STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) PURE;
    STDMETHOD_(void, SetCursorPosition)(THIS_ UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) PURE;
    STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow) PURE;
    STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, void** pSwapChain) PURE;

    // VTable Slot 14: Reset
    STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) PURE;
    // VTable Slot 15: Present
    STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) PURE;

    // 16..33
    STDMETHOD(GetBackBuffer)(THIS_ UINT BackBuffer, DWORD Type, IDirect3DSurface8** ppBackBuffer) PURE;
    STDMETHOD(GetRasterStatus)(THIS_ void* pRasterStatus) PURE;
    STDMETHOD_(void, SetGammaRamp)(THIS_ DWORD Flags, CONST void* pRamp) PURE;
    STDMETHOD_(void, GetGammaRamp)(THIS_ void* pRamp) PURE;
    STDMETHOD(CreateTexture)(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture) PURE;
    STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, void** ppVolumeTexture) PURE;
    STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, void** ppCubeTexture) PURE;
    STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) PURE;
    STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) PURE;
    STDMETHOD(CreateRenderTarget)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, DWORD MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) PURE;
    STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, DWORD MultiSample, IDirect3DSurface8** ppSurface) PURE;
    STDMETHOD(CreateImageSurface)(THIS_ UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) PURE;
    STDMETHOD(CopyRects)(THIS_ IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray) PURE;
    STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) PURE;
    STDMETHOD(GetFrontBuffer)(THIS_ IDirect3DSurface8* pDestSurface) PURE;
    STDMETHOD(SetRenderTarget)(THIS_ IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) PURE;
    STDMETHOD(GetRenderTarget)(THIS_ IDirect3DSurface8** ppRenderTarget) PURE;
    STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface8** ppZStencilSurface) PURE;

    // VTable Slot 34: BeginScene
    STDMETHOD(BeginScene)(THIS) PURE;
    // VTable Slot 35: EndScene
    STDMETHOD(EndScene)(THIS) PURE;

    // 36..49
    STDMETHOD(Clear)(THIS_ DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) PURE;
    STDMETHOD(SetTransform)(THIS_ DWORD State, CONST D3DMATRIX* pMatrix) PURE;
    STDMETHOD(GetTransform)(THIS_ DWORD State, D3DMATRIX* pMatrix) PURE;
    STDMETHOD(MultiplyTransform)(THIS_ DWORD State, CONST D3DMATRIX* pMatrix) PURE;
    STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT8* pViewport) PURE;
    STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT8* pViewport) PURE;
    STDMETHOD(SetMaterial)(THIS_ CONST void* pMaterial) PURE;
    STDMETHOD(GetMaterial)(THIS_ void* pMaterial) PURE;
    STDMETHOD(SetLight)(THIS_ DWORD Index, CONST void* pLight) PURE;
    STDMETHOD(GetLight)(THIS_ DWORD Index, void* pLight) PURE;
    STDMETHOD(LightEnable)(THIS_ DWORD Index, BOOL Enable) PURE;
    STDMETHOD(GetLightEnable)(THIS_ DWORD Index, BOOL* pEnable) PURE;
    STDMETHOD(SetClipPlane)(THIS_ DWORD Index, CONST float* pPlane) PURE;
    STDMETHOD(GetClipPlane)(THIS_ DWORD Index, float* pPlane) PURE;

    // VTable Slot 50: SetRenderState
    STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State, DWORD Value) PURE;
    STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State, DWORD* pValue) PURE;
    STDMETHOD(BeginStateBlock)(THIS) PURE;
    STDMETHOD(EndStateBlock)(THIS_ DWORD* pToken) PURE;
    STDMETHOD(ApplyStateBlock)(THIS_ DWORD Token) PURE;
    STDMETHOD(CaptureStateBlock)(THIS_ DWORD Token) PURE;
    STDMETHOD(DeleteStateBlock)(THIS_ DWORD Token) PURE;
    STDMETHOD(CreateStateBlock)(THIS_ DWORD Type, DWORD* pToken) PURE;
    STDMETHOD(SetClipStatus)(THIS_ CONST void* pClipStatus) PURE;
    STDMETHOD(GetClipStatus)(THIS_ void* pClipStatus) PURE;
    STDMETHOD(GetTexture)(THIS_ DWORD Stage, IDirect3DBaseTexture8** ppTexture) PURE;

    // VTable Slot 61: SetTexture
    STDMETHOD(SetTexture)(THIS_ DWORD Stage, IDirect3DBaseTexture8* pTexture) PURE;
    STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) PURE;
    // VTable Slot 63: SetTextureStageState
    STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) PURE;
    STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses) PURE;
    STDMETHOD(GetInfo)(THIS_ DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) PURE;
    STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber, CONST PALETTEENTRY* pEntries) PURE;
    STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber, PALETTEENTRY* pEntries) PURE;
    STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber) PURE;
    STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT* PaletteNumber) PURE;
    STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) PURE;

    // VTable Slot 71: DrawIndexedPrimitive
    STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount) PURE;
    STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) PURE;
    STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) PURE;
    STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags) PURE;
    STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage) PURE;

    // VTable Slot 76: SetVertexShader
    STDMETHOD(SetVertexShader)(THIS_ DWORD Handle) PURE;
    STDMETHOD(GetVertexShader)(THIS_ DWORD* pHandle) PURE;
    STDMETHOD(DeleteVertexShader)(THIS_ DWORD Handle) PURE;
    STDMETHOD(SetVertexShaderConstant)(THIS_ DWORD Register, CONST void* pConstantData, DWORD ConstantCount) PURE;
    STDMETHOD(GetVertexShaderConstant)(THIS_ DWORD Register, void* pConstantData, DWORD ConstantCount) PURE;
    STDMETHOD(GetVertexShaderDeclaration)(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(GetVertexShaderFunction)(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData) PURE;

    // VTable Slot 83: SetStreamSource
    STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride) PURE;
    STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride) PURE;

    // VTable Slot 85 (or 84 depending on index): SetIndices
    STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex) PURE;
    STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex) PURE;
    STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction, DWORD* pHandle) PURE;
    STDMETHOD(SetPixelShader)(THIS_ DWORD Handle) PURE;
    STDMETHOD(GetPixelShader)(THIS_ DWORD* pHandle) PURE;
    STDMETHOD(DeletePixelShader)(THIS_ DWORD Handle) PURE;
    STDMETHOD(SetPixelShaderConstant)(THIS_ DWORD Register, CONST void* pConstantData, DWORD ConstantCount) PURE;
    STDMETHOD(GetPixelShaderConstant)(THIS_ DWORD Register, void* pConstantData, DWORD ConstantCount) PURE;
    STDMETHOD(GetPixelShaderFunction)(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData) PURE;
    STDMETHOD(DrawRectPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST void* pRectPatchInfo) PURE;
    STDMETHOD(DrawTriPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST void* pTriPatchInfo) PURE;
    STDMETHOD(DeletePatch)(THIS_ UINT Handle) PURE;
};

extern "C" IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion);
