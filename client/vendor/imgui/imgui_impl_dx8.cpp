#include "imgui.h"
#include "imgui_impl_dx8.h"
#include "../d3d8/d3d8_minimal.h"
#include <cstdint>

struct ImGui_ImplDX8_Data {
    IDirect3DDevice8*       pd3dDevice;
    IDirect3DVertexBuffer8* pVB;
    IDirect3DIndexBuffer8*  pIB;
    IDirect3DTexture8*      FontTexture;
    int                     VertexBufferSize;
    int                     IndexBufferSize;

    ImGui_ImplDX8_Data() {
        memset(this, 0, sizeof(*this));
        VertexBufferSize = 5000;
        IndexBufferSize = 10000;
    }
};

static ImGui_ImplDX8_Data* ImGui_ImplDX8_GetBackendData() {
    return ImGui::GetCurrentContext() ? (ImGui_ImplDX8_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

struct CUSTOMVERTEX {
    float    x, y, z, rhw;
    D3DCOLOR color;
    float    u, v;
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

static bool ImGui_ImplDX8_CreateFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (bd->pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &bd->FontTexture) < 0) {
        if (bd->pd3dDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &bd->FontTexture) < 0)
            return false;
    }

    D3DLOCKED_RECT locked_rect;
    if (SUCCEEDED(bd->FontTexture->LockRect(0, &locked_rect, NULL, 0)))
    {
        for (int y = 0; y < height; y++)
        {
            unsigned char* dest_row = (unsigned char*)locked_rect.pBits + (y * locked_rect.Pitch);
            const unsigned char* src_row = pixels + (y * width * 4);
            memcpy(dest_row, src_row, width * 4);
        }
        bd->FontTexture->UnlockRect(0);
    }
    else
    {
        return false;
    }
    io.Fonts->SetTexID((ImTextureID)bd->FontTexture);
    return true;
}

bool ImGui_ImplDX8_CreateDeviceObjects() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    if (!bd || !bd->pd3dDevice) return false;

    if (!ImGui_ImplDX8_CreateFontsTexture())
        return false;

    return true;
}

void ImGui_ImplDX8_InvalidateDeviceObjects() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    if (!bd || !bd->pd3dDevice) return;

    if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
    if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
    if (bd->FontTexture) {
        bd->FontTexture->Release();
        bd->FontTexture = nullptr;
        ImGui::GetIO().Fonts->SetTexID(0);
    }
}

bool ImGui_ImplDX8_Init(IDirect3DDevice8* device) {
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    ImGui_ImplDX8_Data* bd = IM_NEW(ImGui_ImplDX8_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_dx8";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    bd->pd3dDevice = device;
    bd->pd3dDevice->AddRef();

    return true;
}

void ImGui_ImplDX8_Shutdown() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplDX8_InvalidateDeviceObjects();
    if (bd->pd3dDevice) { bd->pd3dDevice->Release(); bd->pd3dDevice = nullptr; }

    io.BackendRendererUserData = nullptr;
    io.BackendRendererName = nullptr;
    IM_DELETE(bd);
}

void ImGui_ImplDX8_NewFrame() {
    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplDX8_Init()?");

    if (!bd->FontTexture)
        ImGui_ImplDX8_CreateDeviceObjects();
}

void ImGui_ImplDX8_RenderDrawData(ImDrawData* draw_data) {
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    ImGui_ImplDX8_Data* bd = ImGui_ImplDX8_GetBackendData();
    IDirect3DDevice8* dev = bd->pd3dDevice;

    // Create or resize buffers if needed
    if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount) {
        if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
        bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
        if (dev->CreateVertexBuffer(bd->VertexBufferSize * sizeof(CUSTOMVERTEX),
                                    D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                    D3DFVF_CUSTOMVERTEX,
                                    D3DPOOL_DEFAULT,
                                    &bd->pVB) < 0)
            return;
    }

    if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount) {
        if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
        bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
        if (dev->CreateIndexBuffer(bd->IndexBufferSize * sizeof(ImDrawIdx),
                                   D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                   sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32,
                                   D3DPOOL_DEFAULT,
                                   &bd->pIB) < 0)
            return;
    }

    // Copy vertices & indices
    CUSTOMVERTEX* vtx_dst = nullptr;
    ImDrawIdx* idx_dst = nullptr;
    if (bd->pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)), (BYTE**)&vtx_dst, D3DLOCK_DISCARD) < 0)
        return;
    if (bd->pIB->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (BYTE**)&idx_dst, D3DLOCK_DISCARD) < 0) {
        bd->pVB->Unlock();
        return;
    }

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++) {
            vtx_dst->x = vtx_src->pos.x - 0.5f; // DirectX 8 half-pixel offset
            vtx_dst->y = vtx_src->pos.y - 0.5f;
            vtx_dst->z = 0.0f;
            vtx_dst->rhw = 1.0f;
            // ImGui color is RGBA32; D3D8 expects ARGB32
            ImU32 c = vtx_src->col;
            vtx_dst->color = (c & 0xFF00FF00) | ((c >> 16) & 0xFF) | ((c & 0xFF) << 16);
            vtx_dst->u = vtx_src->uv.x;
            vtx_dst->v = vtx_src->uv.y;
            vtx_dst++;
            vtx_src++;
        }
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    bd->pVB->Unlock();
    bd->pIB->Unlock();

    // Render states setup
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_LIGHTING, FALSE);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
    dev->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
    dev->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    dev->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    dev->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);

    dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

    dev->SetVertexShader(D3DFVF_CUSTOMVERTEX);
    dev->SetStreamSource(0, bd->pVB, sizeof(CUSTOMVERTEX));
    dev->SetIndices(bd->pIB, 0);

    // Setup viewport
    D3DVIEWPORT8 vp;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = (DWORD)draw_data->DisplaySize.x;
    vp.Height = (DWORD)draw_data->DisplaySize.y;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    dev->SetViewport(&vp);

    // Setup orthographic projection matrix for screen space
    D3DMATRIX ortho;
    memset(&ortho, 0, sizeof(ortho));
    ortho._11 = 2.0f / draw_data->DisplaySize.x;
    ortho._22 = -2.0f / draw_data->DisplaySize.y;
    ortho._33 = 1.0f;
    ortho._41 = -1.0f;
    ortho._42 = 1.0f;
    ortho._44 = 1.0f;
    dev->SetTransform(D3DTS_PROJECTION, &ortho);

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                dev->SetTexture(0, (IDirect3DBaseTexture8*)pcmd->GetTexID());
                dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                          vtx_offset + pcmd->VtxOffset,
                                          cmd_list->VtxBuffer.Size,
                                          idx_offset + pcmd->IdxOffset,
                                          pcmd->ElemCount / 3);
            }
        }
        idx_offset += cmd_list->IdxBuffer.Size;
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
}
