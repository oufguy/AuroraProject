#include <R_DirectX.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define DX_CALL __stdcall
#else
    #define DX_CALL
#endif

// COM GUID definition
struct DX_GUID {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    BYTE   Data4[8];
};

static constexpr DX_GUID IID_Texture2D = {
    0x6f15aaf2, 0xd208, 0x4e89, { 0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c }
};

static constexpr DX_GUID IID_DXGIDevice = {
    0x54ec77fa, 0x1377, 0x44e6, { 0x8c, 0x32, 0x88, 0xfd, 0x5f, 0x44, 0xc8, 0x4c }
};

struct DX_RATIONAL {
    UINT Numerator;
    UINT Denominator;
};

struct DX_MODE_DESC {
    UINT        Width;
    UINT        Height;
    DX_RATIONAL RefreshRate;
    UINT        Format;
    UINT        ScanlineOrdering;
    UINT        Scaling;
};

struct DX_SAMPLE_DESC {
    UINT Count;
    UINT Quality;
};

struct DX_SWAP_CHAIN_DESC {
    DX_MODE_DESC   BufferDesc;
    DX_SAMPLE_DESC SampleDesc;
    UINT           BufferUsage;
    UINT           BufferCount;
    void          *OutputWindow;
    INT            Windowed;
    UINT           SwapEffect;
    UINT           Flags;
};

struct DX_VIEWPORT {
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
};

struct DX_ADAPTER_DESC {
    wchar_t Description[128];
    UINT    VendorId;
    UINT    DeviceId;
    UINT    SubSysId;
    UINT    Revision;
    size_t  DedicatedVideoMemory;
    size_t  DedicatedSystemMemory;
    size_t  SharedSystemMemory;
    UINT64  AdapterLuid;
};

struct DX_BUFFER_DESC {
    UINT ByteWidth;
    UINT Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    UINT StructureByteStride;
};

struct DX_MAPPED_SUBRESOURCE {
    void *pData;
    UINT  RowPitch;
    UINT  DepthPitch;
};

struct DX_INPUT_ELEMENT_DESC {
    const char *SemanticName;
    UINT        SemanticIndex;
    UINT        Format;
    UINT        InputSlot;
    UINT        AlignedByteOffset;
    UINT        InputSlotClass;
    UINT        InstanceDataStepRate;
};

struct DX_RENDER_TARGET_BLEND_DESC {
    INT  BlendEnable;
    UINT SrcBlend;
    UINT DestBlend;
    UINT BlendOp;
    UINT SrcBlendAlpha;
    UINT DestBlendAlpha;
    UINT BlendOpAlpha;
    BYTE RenderTargetWriteMask;
};

struct DX_BLEND_DESC {
    INT                         AlphaToCoverageEnable;
    INT                         IndependentBlendEnable;
    DX_RENDER_TARGET_BLEND_DESC RenderTarget[8];
};

struct DX_RASTERIZER_DESC {
    UINT  FillMode;
    UINT  CullMode;
    INT   FrontCounterClockwise;
    INT   DepthBias;
    FLOAT DepthBiasClamp;
    FLOAT SlopeScaledDepthBias;
    INT   DepthClipEnable;
    INT   ScissorEnable;
    INT   MultisampleEnable;
    INT   AntialiasedLineEnable;
};

struct DX_TEXTURE2D_DESC {
    UINT           Width;
    UINT           Height;
    UINT           MipLevels;
    UINT           ArraySize;
    UINT           Format;
    DX_SAMPLE_DESC SampleDesc;
    UINT           Usage;
    UINT           BindFlags;
    UINT           CPUAccessFlags;
    UINT           MiscFlags;
};

struct DX_DEPTH_STENCILOP_DESC {
    UINT StencilFailOp;
    UINT StencilDepthFailOp;
    UINT StencilPassOp;
    UINT StencilFunc;
};

struct DX_DEPTH_STENCIL_DESC {
    INT                     DepthEnable;
    UINT                    DepthWriteMask;
    UINT                    DepthFunc;
    INT                     StencilEnable;
    BYTE                    StencilReadMask;
    BYTE                    StencilWriteMask;
    DX_DEPTH_STENCILOP_DESC FrontFace;
    DX_DEPTH_STENCILOP_DESC BackFace;
};

struct DX_SUBRESOURCE_DATA {
    const void *pSysMem;
    UINT        SysMemPitch;
    UINT        SysMemSlicePitch;
};

struct DX_SAMPLER_DESC {
    UINT  Filter;
    UINT  AddressU;
    UINT  AddressV;
    UINT  AddressW;
    FLOAT MipLODBias;
    UINT  MaxAnisotropy;
    UINT  ComparisonFunc;
    FLOAT BorderColor[4];
    FLOAT MinLOD;
    FLOAT MaxLOD;
};

struct GPUTextureSlot {
    void *Texture  = nullptr;
    void *SRV      = nullptr;
    UINT  Revision = 0;
};

using PFN_D3D11CreateDeviceAndSwapChain = int (DX_CALL *)(
    void       *pAdapter,
    int         DriverType,
    void       *Software,
    UINT        Flags,
    const int  *pFeatureLevels,
    UINT        FeatureLevels,
    UINT        SDKVersion,
    const void *pSwapChainDesc,
    void      **ppSwapChain,
    void      **ppDevice,
    int        *pFeatureLevel,
    void      **ppImmediateContext
);

using PFN_Map            = int  (DX_CALL *)(void *, void *, UINT, UINT, UINT, DX_MAPPED_SUBRESOURCE *);
using PFN_Unmap          = void (DX_CALL *)(void *, void *, UINT);
using PFN_DrawIndexed    = void (DX_CALL *)(void *, UINT, UINT, INT);
using PFN_PSSetSRV       = void (DX_CALL *)(void *, UINT, UINT, void *const *);
using PFN_OMSetRT        = void (DX_CALL *)(void *, UINT, void *const *, void *);
using PFN_RSSetViewports = void (DX_CALL *)(void *, UINT, const void *);
using PFN_ClearRTV       = void (DX_CALL *)(void *, void *, const FLOAT *);
using PFN_ClearDSV       = void (DX_CALL *)(void *, void *, UINT, FLOAT, BYTE);

// Static definitions
SDL_SharedObject *R_DirectX::s_D3D11Module        = nullptr;
void             *R_DirectX::s_Device             = nullptr;
void             *R_DirectX::s_DeviceContext      = nullptr;
void             *R_DirectX::s_SwapChain          = nullptr;
void             *R_DirectX::s_RenderTargetView   = nullptr;
void             *R_DirectX::s_DepthStencilBuffer = nullptr;
void             *R_DirectX::s_DepthStencilView   = nullptr;
SDL_Window       *R_DirectX::s_Window             = nullptr;
INT               R_DirectX::s_Width              = 0;
INT               R_DirectX::s_Height             = 0;

static PFN_Map            s_pfnMap            = nullptr;
static PFN_Unmap          s_pfnUnmap          = nullptr;
static PFN_DrawIndexed    s_pfnDrawIndexed    = nullptr;
static PFN_PSSetSRV       s_pfnPSSetSRV       = nullptr;
static PFN_OMSetRT        s_pfnOMSetRT        = nullptr;
static PFN_RSSetViewports s_pfnRSSetViewports = nullptr;
static PFN_ClearRTV       s_pfnClearRTV       = nullptr;
static PFN_ClearDSV       s_pfnClearDSV       = nullptr;

void             *R_DirectX::s_VertexShader       = nullptr;
void             *R_DirectX::s_PixelShader        = nullptr;
void             *R_DirectX::s_InputLayout        = nullptr;
void             *R_DirectX::s_ConstantBuffer     = nullptr;
void             *R_DirectX::s_VertexBuffer       = nullptr;
void             *R_DirectX::s_IndexBuffer        = nullptr;
void             *R_DirectX::s_BlendState         = nullptr;
void             *R_DirectX::s_DepthStencilState  = nullptr;
void             *R_DirectX::s_RasterizerState    = nullptr;
void             *R_DirectX::s_SamplerState       = nullptr;

static uMap<UINT, GPUTextureSlot>           s_TextureCache;
static UINT                                 s_LastBoundTextureID = 0xFFFFFFFFu;
static uMap<UINT, R_DirectX::CachedMeshSlot> s_MeshCache;
static UINT                                 s_StaticVertexCursor = 0;
static UINT                                 s_StaticIndexCursor  = 0;
static UINT                                 s_FrameVertexOffset  = R_DirectX::STATIC_VERTEX_CAPACITY;
static UINT                                 s_FrameIndexOffset   = R_DirectX::STATIC_INDEX_CAPACITY;
static BOOL                                 s_BufferNeedsDiscard = true;

BOOL   R_DirectX::s_IsInitialized = false;
BOOL   R_DirectX::s_IsFrameActive = false;
BOOL   R_DirectX::s_VSync         = true;
BOOL   R_DirectX::s_AllowTearing  = false;
COLOR  R_DirectX::s_ClearColor    = COLOR(30, 40, 55, 255);
String R_DirectX::s_GPUName       = "DirectX 11 Device";

static void COM_Release(void *&ptr) {
    if (!ptr) return;
    using PFN_Release = unsigned long (DX_CALL *)(void *);
    void **vtable = *reinterpret_cast<void***>(ptr);
    reinterpret_cast<PFN_Release>(vtable[2])(ptr);
    ptr = nullptr;
}

static BOOL CreateMeshPipelineAndBuffers() {
    void **devVtbl = *reinterpret_cast<void***>(R_DirectX::s_Device);
    int hr = 0;

    using PFN_CreateVS = int (DX_CALL *)(void *, const void *, size_t, void *, void **);
    hr = reinterpret_cast<PFN_CreateVS>(devVtbl[12])(R_DirectX::s_Device, Engine::Shaders::Mesh::DirectX_Vertex_DXBC, sizeof(Engine::Shaders::Mesh::DirectX_Vertex_DXBC), nullptr, &R_DirectX::s_VertexShader);
    if (hr < 0 || !R_DirectX::s_VertexShader) {
        LOG(Error, LogLevel::Core, "DirectX: CreateVertexShader failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    using PFN_CreatePS = int (DX_CALL *)(void *, const void *, size_t, void *, void **);
    hr = reinterpret_cast<PFN_CreatePS>(devVtbl[15])(R_DirectX::s_Device, Engine::Shaders::Mesh::DirectX_Pixel_DXBC, sizeof(Engine::Shaders::Mesh::DirectX_Pixel_DXBC), nullptr, &R_DirectX::s_PixelShader);
    if (hr < 0 || !R_DirectX::s_PixelShader) {
        LOG(Error, LogLevel::Core, "DirectX: CreatePixelShader failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    DX_INPUT_ELEMENT_DESC layoutDesc[4] = {
        { "POSITION", 0,  6, 0, offsetof(Vertex, Position), 0, 0 },
        { "NORMAL",   0,  6, 0, offsetof(Vertex, Normal),   0, 0 },
        { "TEXCOORD", 0, 16, 0, offsetof(Vertex, UV),       0, 0 },
        { "COLOR",    0, 28, 0, offsetof(Vertex, Color),    0, 0 }
    };
    using PFN_CreateIL = int (DX_CALL *)(void *, const void *, UINT, const void *, size_t, void **);
    hr = reinterpret_cast<PFN_CreateIL>(devVtbl[11])(R_DirectX::s_Device, layoutDesc, 4, Engine::Shaders::Mesh::DirectX_Vertex_DXBC, sizeof(Engine::Shaders::Mesh::DirectX_Vertex_DXBC), &R_DirectX::s_InputLayout);
    if (hr < 0 || !R_DirectX::s_InputLayout) {
        LOG(Error, LogLevel::Core, "DirectX: CreateInputLayout failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    using PFN_CreateBuffer = int (DX_CALL *)(void *, const void *, const void *, void **);

    DX_BUFFER_DESC vbDesc{ R_DirectX::MAX_VERTICES_PER_FRAME * sizeof(Vertex), 2, 1, 0x10000, 0, 0 };
    hr = reinterpret_cast<PFN_CreateBuffer>(devVtbl[3])(R_DirectX::s_Device, &vbDesc, nullptr, &R_DirectX::s_VertexBuffer);
    if (hr < 0 || !R_DirectX::s_VertexBuffer) {
        LOG(Error, LogLevel::Core, "DirectX: CreateBuffer(VB) failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    DX_BUFFER_DESC ibDesc{ R_DirectX::MAX_INDICES_PER_FRAME * sizeof(UINT), 2, 2, 0x10000, 0, 0 };
    hr = reinterpret_cast<PFN_CreateBuffer>(devVtbl[3])(R_DirectX::s_Device, &ibDesc, nullptr, &R_DirectX::s_IndexBuffer);
    if (hr < 0 || !R_DirectX::s_IndexBuffer) {
        LOG(Error, LogLevel::Core, "DirectX: CreateBuffer(IB) failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    DX_BUFFER_DESC cbDesc{ sizeof(DrawConstants), 2, 4, 0x10000, 0, 0 };
    hr = reinterpret_cast<PFN_CreateBuffer>(devVtbl[3])(R_DirectX::s_Device, &cbDesc, nullptr, &R_DirectX::s_ConstantBuffer);
    if (hr < 0 || !R_DirectX::s_ConstantBuffer) {
        LOG(Error, LogLevel::Core, "DirectX: CreateBuffer(CB) failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    DX_BLEND_DESC blendDesc{};
    for (int i = 0; i < 8; ++i) {
        blendDesc.RenderTarget[i].BlendEnable           = (i == 0) ? 1 : 0;
        blendDesc.RenderTarget[i].SrcBlend              = 5;
        blendDesc.RenderTarget[i].DestBlend             = 6;
        blendDesc.RenderTarget[i].BlendOp               = 1;
        blendDesc.RenderTarget[i].SrcBlendAlpha         = 2;
        blendDesc.RenderTarget[i].DestBlendAlpha        = 1;
        blendDesc.RenderTarget[i].BlendOpAlpha          = 1;
        blendDesc.RenderTarget[i].RenderTargetWriteMask = 0x0F;
    }

    using PFN_CreateBlend = int (DX_CALL *)(void *, const void *, void **);
    hr = reinterpret_cast<PFN_CreateBlend>(devVtbl[20])(R_DirectX::s_Device, &blendDesc, &R_DirectX::s_BlendState);
    if (hr < 0 || !R_DirectX::s_BlendState) {
        LOG(Error, LogLevel::Core, "DirectX: CreateBlendState failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    DX_DEPTH_STENCIL_DESC depthDesc{};
    depthDesc.DepthEnable      = 1;
    depthDesc.DepthWriteMask   = 1;
    depthDesc.DepthFunc        = 4;
    depthDesc.StencilEnable    = 0;
    depthDesc.StencilReadMask  = 0xFF;
    depthDesc.StencilWriteMask = 0xFF;
    depthDesc.FrontFace        = { 1, 1, 1, 8 };
    depthDesc.BackFace         = { 1, 1, 1, 8 };

    using PFN_CreateDepthState = int (DX_CALL *)(void *, const void *, void **);
    hr = reinterpret_cast<PFN_CreateDepthState>(devVtbl[21])(R_DirectX::s_Device, &depthDesc, &R_DirectX::s_DepthStencilState);
    if (hr < 0 || !R_DirectX::s_DepthStencilState) {
        LOG(Error, LogLevel::Core, "DirectX: CreateDepthStencilState failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    DX_RASTERIZER_DESC rastDesc{};
    rastDesc.FillMode              = 3;
    rastDesc.CullMode              = 3;
    rastDesc.FrontCounterClockwise = 0;
    rastDesc.DepthClipEnable       = 1;

    using PFN_CreateRast = int (DX_CALL *)(void *, const void *, void **);
    hr = reinterpret_cast<PFN_CreateRast>(devVtbl[22])(R_DirectX::s_Device, &rastDesc, &R_DirectX::s_RasterizerState);
    if (hr < 0 || !R_DirectX::s_RasterizerState) {
        LOG(Error, LogLevel::Core, "DirectX: CreateRasterizerState failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    DX_SAMPLER_DESC sampDesc{};
    sampDesc.Filter         = 0x15;
    sampDesc.AddressU       = 1;
    sampDesc.AddressV       = 1;
    sampDesc.AddressW       = 1;
    sampDesc.MipLODBias     = 0.0f;
    sampDesc.MaxAnisotropy  = 1;
    sampDesc.ComparisonFunc = 8;
    sampDesc.MinLOD         = 0.0f;
    sampDesc.MaxLOD         = 3.402823466e+38f;

    using PFN_CreateSampler = int (DX_CALL *)(void *, const void *, void **);
    hr = reinterpret_cast<PFN_CreateSampler>(devVtbl[23])(R_DirectX::s_Device, &sampDesc, &R_DirectX::s_SamplerState);
    if (hr < 0 || !R_DirectX::s_SamplerState) {
        LOG(Error, LogLevel::Core, "DirectX: CreateSamplerState failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    return true;
}

static void *GetOrUploadTextureSRV(UINT textureID) {
    if (!R_DirectX::s_Device) return nullptr;

    auto it = s_TextureCache.find(textureID);
    if (it != s_TextureCache.end() && it->second.SRV != nullptr) {
        return it->second.SRV;
    }

    const TextureData &tex = M_Texture::Get(textureID);
    if (!tex.Is_Valid()) return nullptr;

    if (it != s_TextureCache.end()) {
        COM_Release(it->second.SRV);
        COM_Release(it->second.Texture);
    }

    DX_TEXTURE2D_DESC texDesc{};
    texDesc.Width              = tex.Width;
    texDesc.Height             = tex.Height;
    texDesc.MipLevels          = 1;
    texDesc.ArraySize          = 1;
    texDesc.Format             = 28;
    texDesc.SampleDesc.Count   = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage              = 0;
    texDesc.BindFlags          = 0x08;

    DX_SUBRESOURCE_DATA subData{};
    subData.pSysMem          = tex.Pixels.data();
    subData.SysMemPitch      = tex.Width * sizeof(COLOR);
    subData.SysMemSlicePitch = 0;

    void **devVtbl = *reinterpret_cast<void***>(R_DirectX::s_Device);
    using PFN_CreateTex2D = int (DX_CALL *)(void *, const void *, const void *, void **);
    using PFN_CreateSRV   = int (DX_CALL *)(void *, void *, const void *, void **);

    void *gpuTex = nullptr;
    if (reinterpret_cast<PFN_CreateTex2D>(devVtbl[5])(R_DirectX::s_Device, &texDesc, &subData, &gpuTex) < 0 || !gpuTex) {
        return nullptr;
    }

    void *gpuSRV = nullptr;
    if (reinterpret_cast<PFN_CreateSRV>(devVtbl[7])(R_DirectX::s_Device, gpuTex, nullptr, &gpuSRV) < 0 || !gpuSRV) {
        COM_Release(gpuTex);
        return nullptr;
    }

    s_TextureCache[tex.TextureID] = { gpuTex, gpuSRV, tex.Revision };
    return gpuSRV;
}

static BOOL CreateDepthBuffer(INT w, INT h) {
    if (!R_DirectX::s_Device || w <= 0 || h <= 0) return false;

    COM_Release(R_DirectX::s_DepthStencilView);
    COM_Release(R_DirectX::s_DepthStencilBuffer);

    DX_TEXTURE2D_DESC depthTexDesc{};
    depthTexDesc.Width              = static_cast<UINT>(w);
    depthTexDesc.Height             = static_cast<UINT>(h);
    depthTexDesc.MipLevels          = 1;
    depthTexDesc.ArraySize          = 1;
    depthTexDesc.Format             = 40;
    depthTexDesc.SampleDesc.Count   = 1;
    depthTexDesc.SampleDesc.Quality = 0;
    depthTexDesc.Usage              = 0;
    depthTexDesc.BindFlags          = 0x40;

    void **devVtbl = *reinterpret_cast<void***>(R_DirectX::s_Device);
    using PFN_CreateTex2D = int (DX_CALL *)(void *, const void *, const void *, void **);
    int hr = reinterpret_cast<PFN_CreateTex2D>(devVtbl[5])(R_DirectX::s_Device, &depthTexDesc, nullptr, &R_DirectX::s_DepthStencilBuffer);
    if (hr < 0 || !R_DirectX::s_DepthStencilBuffer) {
        LOG(Error, LogLevel::Core, "DirectX: CreateTexture2D(Depth) failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    using PFN_CreateDSV = int (DX_CALL *)(void *, void *, const void *, void **);
    hr = reinterpret_cast<PFN_CreateDSV>(devVtbl[10])(R_DirectX::s_Device, R_DirectX::s_DepthStencilBuffer, nullptr, &R_DirectX::s_DepthStencilView);
    if (hr < 0 || !R_DirectX::s_DepthStencilView) {
        LOG(Error, LogLevel::Core, "DirectX: CreateDepthStencilView failed (0x{:08X})", static_cast<UINT>(hr));
        return false;
    }

    return true;
}

static void DestroyMeshPipelineAndBuffers() {
    for (auto &kv : s_TextureCache) {
        COM_Release(kv.second.SRV);
        COM_Release(kv.second.Texture);
    }
    s_TextureCache.clear();
    s_LastBoundTextureID = 0xFFFFFFFFu;

    s_MeshCache.clear();
    s_StaticVertexCursor = 0;
    s_StaticIndexCursor  = 0;
    s_FrameVertexOffset  = R_DirectX::STATIC_VERTEX_CAPACITY;
    s_FrameIndexOffset   = R_DirectX::STATIC_INDEX_CAPACITY;
    s_BufferNeedsDiscard = true;

    COM_Release(R_DirectX::s_SamplerState);
    COM_Release(R_DirectX::s_RasterizerState);
    COM_Release(R_DirectX::s_DepthStencilState);
    COM_Release(R_DirectX::s_BlendState);
    COM_Release(R_DirectX::s_ConstantBuffer);
    COM_Release(R_DirectX::s_IndexBuffer);
    COM_Release(R_DirectX::s_VertexBuffer);
    COM_Release(R_DirectX::s_InputLayout);
    COM_Release(R_DirectX::s_PixelShader);
    COM_Release(R_DirectX::s_VertexShader);
}

BOOL R_DirectX::Is_Supported() {
    SDL_SharedObject *mod = SDL_LoadObject("d3d11.dll");
    if (!mod) return false;
    BOOL bHasFunc = (SDL_LoadFunction(mod, "D3D11CreateDeviceAndSwapChain") != nullptr);
    SDL_UnloadObject(mod);
    return bHasFunc;
}

BOOL R_DirectX::Init(SDL_Window *window) {
    if (s_IsInitialized) return true;
    if (!window) return false;

    s_D3D11Module = SDL_LoadObject("d3d11.dll");
    if (!s_D3D11Module) {
        LOG(Error, LogLevel::Core, "DirectX: d3d11.dll / DXVK dynamic library not found!");
        return false;
    }

    auto pfnCreate = reinterpret_cast<PFN_D3D11CreateDeviceAndSwapChain>(
        SDL_LoadFunction(s_D3D11Module, "D3D11CreateDeviceAndSwapChain"));

    if (!pfnCreate) {
        LOG(Error, LogLevel::Core, "DirectX: Failed to locate D3D11CreateDeviceAndSwapChain in d3d11.dll!");
        SDL_UnloadObject(s_D3D11Module);
        s_D3D11Module = nullptr;
        return false;
    }

    s_Window = window;

    void *hwnd = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window),
        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        nullptr);

    if (!hwnd) {
        LOG(Error, LogLevel::Core, "DirectX: Failed to obtain window handle from SDL3!");
        Shutdown();
        return false;
    }

    INT w = 0, h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (w <= 0 || h <= 0) { w = 1280; h = 720; }
    s_Width  = w;
    s_Height = h;

    DX_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = static_cast<UINT>(w);
    sd.BufferDesc.Height                  = static_cast<UINT>(h);
    sd.BufferDesc.Format                  = 28;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage                        = 0x20;
    sd.OutputWindow                       = hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.SampleDesc.Quality                 = 0;
    sd.Windowed                           = 1;
    sd.SwapEffect                         = 4;
    sd.Flags                              = 2048;
    s_AllowTearing                        = true;

    const int featureLevels[] = {
        0xb100,
        0xb000,
        0xa100,
        0xa000
    };
    int chosenLevel = 0;

    int hr = pfnCreate(
        nullptr,
        1,
        nullptr,
        0,
        featureLevels,
        4,
        7,
        &sd,
        &s_SwapChain,
        &s_Device,
        &chosenLevel,
        &s_DeviceContext
    );

    if (hr < 0) {
        s_AllowTearing = false;
        sd.Flags       = 0;
        hr = pfnCreate(nullptr, 1, nullptr, 0, featureLevels, 4, 7, &sd, &s_SwapChain, &s_Device, &chosenLevel, &s_DeviceContext);
    }

    if (hr < 0) {
        sd.SwapEffect = 0;
        sd.Flags      = 2;
        hr = pfnCreate(nullptr, 1, nullptr, 0, featureLevels, 4, 7, &sd, &s_SwapChain, &s_Device, &chosenLevel, &s_DeviceContext);
    }

    if (hr < 0 || !s_Device || !s_SwapChain || !s_DeviceContext) {
        LOG(Error, LogLevel::Core, "DirectX: Failed to create D3D11 Device and SwapChain (Code: 0x{:08X})", static_cast<UINT32>(hr));
        Shutdown();
        return false;
    }

    void **ctxVtbl       = *reinterpret_cast<void***>(s_DeviceContext);
    s_pfnMap             = reinterpret_cast<PFN_Map>(ctxVtbl[14]);
    s_pfnUnmap           = reinterpret_cast<PFN_Unmap>(ctxVtbl[15]);
    s_pfnDrawIndexed     = reinterpret_cast<PFN_DrawIndexed>(ctxVtbl[12]);
    s_pfnPSSetSRV        = reinterpret_cast<PFN_PSSetSRV>(ctxVtbl[8]);
    s_pfnOMSetRT         = reinterpret_cast<PFN_OMSetRT>(ctxVtbl[33]);
    s_pfnRSSetViewports  = reinterpret_cast<PFN_RSSetViewports>(ctxVtbl[44]);
    s_pfnClearRTV        = reinterpret_cast<PFN_ClearRTV>(ctxVtbl[50]);
    s_pfnClearDSV        = reinterpret_cast<PFN_ClearDSV>(ctxVtbl[53]);

    void *dxgiDevice = nullptr;
    void **devVtbl = *reinterpret_cast<void***>(s_Device);
    using PFN_QI = int (DX_CALL *)(void *, const void *, void **);
    if (reinterpret_cast<PFN_QI>(devVtbl[0])(s_Device, &IID_DXGIDevice, &dxgiDevice) >= 0 && dxgiDevice) {
        void **dxgiVtbl = *reinterpret_cast<void***>(dxgiDevice);
        void *adapter = nullptr;
        using PFN_GetAdapter = int (DX_CALL *)(void *, void **);
        if (reinterpret_cast<PFN_GetAdapter>(dxgiVtbl[7])(dxgiDevice, &adapter) >= 0 && adapter) {
            void **adapterVtbl = *reinterpret_cast<void***>(adapter);
            DX_ADAPTER_DESC desc{};
            using PFN_GetDesc = int (DX_CALL *)(void *, void *);
            if (reinterpret_cast<PFN_GetDesc>(adapterVtbl[8])(adapter, &desc) >= 0) {
                String name;
                for (int i = 0; i < 128 && desc.Description[i] != 0; ++i) {
                    name += static_cast<char>(desc.Description[i]);
                }
                if (!name.empty()) {
                    s_GPUName = name;
                }
            }
            COM_Release(adapter);
        }
        COM_Release(dxgiDevice);
    }

    void *pBackBuffer = nullptr;
    void **scVtbl = *reinterpret_cast<void***>(s_SwapChain);
    using PFN_GetBuffer = int (DX_CALL *)(void *, UINT, const void *, void **);
    hr = reinterpret_cast<PFN_GetBuffer>(scVtbl[9])(s_SwapChain, 0, &IID_Texture2D, &pBackBuffer);
    if (hr < 0 || !pBackBuffer) {
        LOG(Error, LogLevel::Core, "DirectX: Failed to obtain BackBuffer from SwapChain!");
        Shutdown();
        return false;
    }

    using PFN_CreateRTV = int (DX_CALL *)(void *, void *, const void *, void **);
    hr = reinterpret_cast<PFN_CreateRTV>(devVtbl[9])(s_Device, pBackBuffer, nullptr, &s_RenderTargetView);
    COM_Release(pBackBuffer);

    if (hr < 0 || !s_RenderTargetView) {
        LOG(Error, LogLevel::Core, "DirectX: Failed to create RenderTargetView!");
        Shutdown();
        return false;
    }

    if (!CreateDepthBuffer(w, h)) {
        Shutdown();
        return false;
    }

    DX_VIEWPORT vp{};
    vp.Width    = static_cast<FLOAT>(w);
    vp.Height   = static_cast<FLOAT>(h);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    using PFN_RSSetViewportsType = void (DX_CALL *)(void *, UINT, const void *);
    reinterpret_cast<PFN_RSSetViewportsType>(ctxVtbl[44])(s_DeviceContext, 1, &vp);

    if (!CreateMeshPipelineAndBuffers()) {
        LOG(Error, LogLevel::Core, "DirectX: Failed to create Mesh Pipeline!");
        Shutdown();
        return false;
    }

    BindMeshPipelineStates();

    s_IsInitialized = true;
    LOG(Info, LogLevel::Core, "DirectX 11: Backend initialized successfully on [{}] ({}x{})", s_GPUName, w, h);
    return true;
}

void R_DirectX::Shutdown() {
    if (!s_IsInitialized && !s_D3D11Module) return;

    DestroyMeshPipelineAndBuffers();

    COM_Release(s_DepthStencilView);
    COM_Release(s_DepthStencilBuffer);
    COM_Release(s_RenderTargetView);
    COM_Release(s_SwapChain);
    COM_Release(s_DeviceContext);
    COM_Release(s_Device);

    if (s_D3D11Module) {
        SDL_UnloadObject(s_D3D11Module);
        s_D3D11Module = nullptr;
    }

    s_Window        = nullptr;
    s_IsFrameActive = false;
    s_IsInitialized = false;
    LOG(Info, LogLevel::Core, "DirectX 11: Backend shutdown cleanly.");
}

void R_DirectX::Recreate() {
    if (!s_IsInitialized || !s_SwapChain || !s_Device || !s_DeviceContext) return;

    INT w = 0, h = 0;
    if (s_Window) {
        SDL_GetWindowSizeInPixels(s_Window, &w, &h);
    }
    if (w <= 0 || h <= 0) return;

    COM_Release(s_DepthStencilView);
    COM_Release(s_DepthStencilBuffer);
    COM_Release(s_RenderTargetView);

    void **scVtbl = *reinterpret_cast<void***>(s_SwapChain);
    using PFN_ResizeBuffers = int (DX_CALL *)(void *, UINT, UINT, UINT, UINT, UINT);
    UINT resizeFlags = s_AllowTearing ? 2048u : 0u;
    int hr = reinterpret_cast<PFN_ResizeBuffers>(scVtbl[13])(s_SwapChain, 0, static_cast<UINT>(w), static_cast<UINT>(h), 0, resizeFlags);
    if (hr < 0) {
        LOG(Error, LogLevel::Core, "DirectX: Failed to resize SwapChain buffers!");
        return;
    }

    void *pBackBuffer = nullptr;
    using PFN_GetBuffer = int (DX_CALL *)(void *, UINT, const void *, void **);
    if (reinterpret_cast<PFN_GetBuffer>(scVtbl[9])(s_SwapChain, 0, &IID_Texture2D, &pBackBuffer) >= 0 && pBackBuffer) {
        void **devVtbl = *reinterpret_cast<void***>(s_Device);
        using PFN_CreateRTV = int (DX_CALL *)(void *, void *, const void *, void **);
        reinterpret_cast<PFN_CreateRTV>(devVtbl[9])(s_Device, pBackBuffer, nullptr, &s_RenderTargetView);
        COM_Release(pBackBuffer);
    }

    CreateDepthBuffer(w, h);

    DX_VIEWPORT vp{};
    vp.Width    = static_cast<FLOAT>(w);
    vp.Height   = static_cast<FLOAT>(h);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    void **ctxVtbl = *reinterpret_cast<void***>(s_DeviceContext);
    using PFN_RSSetViewportsType = void (DX_CALL *)(void *, UINT, const void *);
    reinterpret_cast<PFN_RSSetViewportsType>(ctxVtbl[44])(s_DeviceContext, 1, &vp);

    s_Width  = w;
    s_Height = h;

    BindMeshPipelineStates();
}

void R_DirectX::BindMeshPipelineStates() {
    if (!s_DeviceContext) return;
    void **ctxVtbl = *reinterpret_cast<void***>(s_DeviceContext);
    using PFN_IASetIL   = void (DX_CALL *)(void *, void *);
    using PFN_IASetVB   = void (DX_CALL *)(void *, UINT, UINT, void *const *, const UINT *, const UINT *);
    using PFN_IASetIB   = void (DX_CALL *)(void *, void *, UINT, UINT);
    using PFN_IASetTopo = void (DX_CALL *)(void *, UINT);
    using PFN_VSSetS    = void (DX_CALL *)(void *, void *, void *const *, UINT);
    using PFN_VSSetCB   = void (DX_CALL *)(void *, UINT, UINT, void *const *);
    using PFN_PSSetS    = void (DX_CALL *)(void *, void *, void *const *, UINT);
    using PFN_PSSetSamp = void (DX_CALL *)(void *, UINT, UINT, void *const *);
    using PFN_RSSetSt   = void (DX_CALL *)(void *, void *);
    using PFN_OMSetBS   = void (DX_CALL *)(void *, void *, const FLOAT *, UINT);
    using PFN_OMSetDSS  = void (DX_CALL *)(void *, void *, UINT);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    reinterpret_cast<PFN_IASetIL>(ctxVtbl[17])(s_DeviceContext, s_InputLayout);
    reinterpret_cast<PFN_IASetVB>(ctxVtbl[18])(s_DeviceContext, 0, 1, &s_VertexBuffer, &stride, &offset);
    reinterpret_cast<PFN_IASetIB>(ctxVtbl[19])(s_DeviceContext, s_IndexBuffer, 42, 0);
    reinterpret_cast<PFN_IASetTopo>(ctxVtbl[24])(s_DeviceContext, 4);
    reinterpret_cast<PFN_VSSetS>(ctxVtbl[11])(s_DeviceContext, s_VertexShader, nullptr, 0);
    reinterpret_cast<PFN_VSSetCB>(ctxVtbl[7])(s_DeviceContext, 0, 1, &s_ConstantBuffer);
    reinterpret_cast<PFN_PSSetS>(ctxVtbl[9])(s_DeviceContext, s_PixelShader, nullptr, 0);
    reinterpret_cast<PFN_PSSetSamp>(ctxVtbl[10])(s_DeviceContext, 0, 1, &s_SamplerState);
    reinterpret_cast<PFN_RSSetSt>(ctxVtbl[43])(s_DeviceContext, s_RasterizerState);
    reinterpret_cast<PFN_OMSetBS>(ctxVtbl[35])(s_DeviceContext, s_BlendState, nullptr, 0xFFFFFFFF);
    reinterpret_cast<PFN_OMSetDSS>(ctxVtbl[36])(s_DeviceContext, s_DepthStencilState, 0);
}

BOOL R_DirectX::Begin_Frame() {
    if (!s_IsInitialized || !s_DeviceContext || M_Window::Is_Minimized()) return false;

    INT curPxW = 0, curPxH = 0;
    if (s_Window) {
        SDL_GetWindowSizeInPixels(s_Window, &curPxW, &curPxH);
    }

    if (M_Window::Was_Resized() || curPxW != s_Width || curPxH != s_Height || !s_RenderTargetView) {
        Recreate();
        if (!s_RenderTargetView) return false;
    }

    FLOAT clearColor[4] = {
        static_cast<FLOAT>(s_ClearColor.r) / 255.0f,
        static_cast<FLOAT>(s_ClearColor.g) / 255.0f,
        static_cast<FLOAT>(s_ClearColor.b) / 255.0f,
        static_cast<FLOAT>(s_ClearColor.a) / 255.0f
    };

    s_pfnOMSetRT(s_DeviceContext, 1, &s_RenderTargetView, s_DepthStencilView);

    DX_VIEWPORT vp{ 0.0f, 0.0f, static_cast<FLOAT>(s_Width), static_cast<FLOAT>(s_Height), 0.0f, 1.0f };
    s_pfnRSSetViewports(s_DeviceContext, 1, &vp);

    s_pfnClearRTV(s_DeviceContext, s_RenderTargetView, clearColor);
    if (s_DepthStencilView) {
        s_pfnClearDSV(s_DeviceContext, s_DepthStencilView, 1, 1.0f, 0);
    }

    s_LastBoundTextureID = 0xFFFFFFFFu;
    s_IsFrameActive = true;
    return true;
}

void R_DirectX::Draw_Mesh(
    const MeshData &mesh,
    const Vector3D &pos,
    const Vector3D &rot,
    const Vector3D &size,
    const COLOR    &color,
    FLOAT           transparency,
    UINT            textureID)
{
    if (!s_IsFrameActive || !s_DeviceContext || !mesh.Is_Valid() || transparency <= 0.001f) return;

    const UINT vCount = mesh.Get_VertexCount();
    const UINT iCount = mesh.Get_IndexCount();

    if (textureID != s_LastBoundTextureID) {
        void *srv = GetOrUploadTextureSRV(textureID);
        if (srv) {
            s_pfnPSSetSRV(s_DeviceContext, 0, 1, &srv);
            s_LastBoundTextureID = textureID;
        }
    }

    if (s_BufferNeedsDiscard) {
        DX_MAPPED_SUBRESOURCE dummy{};
        if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
        }
        if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
        }
        s_BufferNeedsDiscard = false;
    }

    UINT drawVertexOffset = 0;
    UINT drawIndexOffset  = 0;
    BOOL bUsedStaticCache = false;

    if (mesh.MeshID != 0) {
        const COLOR &c0 = mesh.Vertices[0].Color;
        const DWORD firstColor = (DWORD(c0.r) << 24) | (DWORD(c0.g) << 16) | (DWORD(c0.b) << 8) | DWORD(c0.a);

        auto it = s_MeshCache.find(mesh.MeshID);
        if (it != s_MeshCache.end()) {
            const CachedMeshSlot &slot = it->second;
            if (slot.Revision    == mesh.Revision &&
                slot.FirstColor  == firstColor    &&
                slot.VertexCount == vCount        &&
                slot.IndexCount  == iCount)
            {
                drawVertexOffset = slot.VertexOffset;
                drawIndexOffset  = slot.IndexOffset;
                bUsedStaticCache = true;
            }
        } else if (s_StaticVertexCursor + vCount <= STATIC_VERTEX_CAPACITY &&
                   s_StaticIndexCursor  + iCount <= STATIC_INDEX_CAPACITY)
        {
            drawVertexOffset = s_StaticVertexCursor;
            drawIndexOffset  = s_StaticIndexCursor;

            DX_MAPPED_SUBRESOURCE mappedVB{};
            if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 5, 0, &mappedVB) >= 0 && mappedVB.pData) {
                std::memcpy(static_cast<Vertex *>(mappedVB.pData) + drawVertexOffset, mesh.Vertices.data(), vCount * sizeof(Vertex));
                s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
            }

            DX_MAPPED_SUBRESOURCE mappedIB{};
            if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 5, 0, &mappedIB) >= 0 && mappedIB.pData) {
                std::memcpy(static_cast<UINT *>(mappedIB.pData) + drawIndexOffset, mesh.Indices.data(), iCount * sizeof(UINT));
                s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
            }

            s_StaticVertexCursor += vCount;
            s_StaticIndexCursor  += iCount;

            s_MeshCache[mesh.MeshID] = { drawVertexOffset, drawIndexOffset, vCount, iCount, mesh.Revision, firstColor };
            bUsedStaticCache         = true;
        }
    }

    if (!bUsedStaticCache) {
        if (s_FrameVertexOffset + vCount > MAX_VERTICES_PER_FRAME ||
            s_FrameIndexOffset  + iCount > MAX_INDICES_PER_FRAME)
        {
            s_FrameVertexOffset = STATIC_VERTEX_CAPACITY;
            s_FrameIndexOffset  = STATIC_INDEX_CAPACITY;
        }

        drawVertexOffset = s_FrameVertexOffset;
        drawIndexOffset  = s_FrameIndexOffset;

        DX_MAPPED_SUBRESOURCE mappedVB{};
        if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 5, 0, &mappedVB) >= 0 && mappedVB.pData) {
            std::memcpy(static_cast<Vertex *>(mappedVB.pData) + drawVertexOffset, mesh.Vertices.data(), vCount * sizeof(Vertex));
            s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
        }

        DX_MAPPED_SUBRESOURCE mappedIB{};
        if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 5, 0, &mappedIB) >= 0 && mappedIB.pData) {
            std::memcpy(static_cast<UINT *>(mappedIB.pData) + drawIndexOffset, mesh.Indices.data(), iCount * sizeof(UINT));
            s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
        }

        s_FrameVertexOffset += vCount;
        s_FrameIndexOffset  += iCount;
    }

    FLOAT aspect = (s_Height > 0)
        ? (static_cast<FLOAT>(s_Width) / static_cast<FLOAT>(s_Height))
        : (16.0f / 9.0f);

    BOOL bShade3D = (vCount > 4);
    DrawConstants dc = M_Mesh::Build_DrawConstants(pos, rot, size, color, transparency, aspect, false, bShade3D);

    DX_MAPPED_SUBRESOURCE mappedCB{};
    if (s_pfnMap(s_DeviceContext, s_ConstantBuffer, 0, 4, 0, &mappedCB) >= 0 && mappedCB.pData) {
        std::memcpy(mappedCB.pData, &dc, sizeof(DrawConstants));
        s_pfnUnmap(s_DeviceContext, s_ConstantBuffer, 0);
    }

    s_pfnDrawIndexed(s_DeviceContext, iCount, drawIndexOffset, static_cast<INT>(drawVertexOffset));
}

void R_DirectX::Draw_UI(
    UINT            textureID,
    const Vector2D &pos,
    const Vector2D &size,
    const COLOR    &color,
    FLOAT           transparency)
{
    if (!s_IsFrameActive || !s_DeviceContext || transparency <= 0.001f) return;

    static const MeshData s_UIQuad = M_Mesh::Create_Quad(COLOR_WHITE);
    const UINT vCount = s_UIQuad.Get_VertexCount();
    const UINT iCount = s_UIQuad.Get_IndexCount();

    if (textureID != s_LastBoundTextureID) {
        void *srv = GetOrUploadTextureSRV(textureID);
        if (srv) {
            s_pfnPSSetSRV(s_DeviceContext, 0, 1, &srv);
            s_LastBoundTextureID = textureID;
        }
    }

    if (s_BufferNeedsDiscard) {
        DX_MAPPED_SUBRESOURCE dummy{};
        if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
        }
        if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
        }
        s_BufferNeedsDiscard = false;
    }

    UINT drawVertexOffset = 0;
    UINT drawIndexOffset  = 0;
    BOOL bUsedStaticCache = false;

    if (s_UIQuad.MeshID != 0) {
        const COLOR &c0 = s_UIQuad.Vertices[0].Color;
        const DWORD firstColor = (DWORD(c0.r) << 24) | (DWORD(c0.g) << 16) | (DWORD(c0.b) << 8) | DWORD(c0.a);

        auto it = s_MeshCache.find(s_UIQuad.MeshID);
        if (it != s_MeshCache.end()) {
            const CachedMeshSlot &slot = it->second;
            if (slot.Revision    == s_UIQuad.Revision &&
                slot.FirstColor  == firstColor        &&
                slot.VertexCount == vCount            &&
                slot.IndexCount  == iCount)
            {
                drawVertexOffset = slot.VertexOffset;
                drawIndexOffset  = slot.IndexOffset;
                bUsedStaticCache = true;
            }
        } else if (s_StaticVertexCursor + vCount <= STATIC_VERTEX_CAPACITY &&
                   s_StaticIndexCursor  + iCount <= STATIC_INDEX_CAPACITY)
        {
            drawVertexOffset = s_StaticVertexCursor;
            drawIndexOffset  = s_StaticIndexCursor;

            DX_MAPPED_SUBRESOURCE mappedVB{};
            if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 5, 0, &mappedVB) >= 0 && mappedVB.pData) {
                std::memcpy(static_cast<Vertex *>(mappedVB.pData) + drawVertexOffset, s_UIQuad.Vertices.data(), vCount * sizeof(Vertex));
                s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
            }

            DX_MAPPED_SUBRESOURCE mappedIB{};
            if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 5, 0, &mappedIB) >= 0 && mappedIB.pData) {
                std::memcpy(static_cast<UINT *>(mappedIB.pData) + drawIndexOffset, s_UIQuad.Indices.data(), iCount * sizeof(UINT));
                s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
            }

            s_StaticVertexCursor += vCount;
            s_StaticIndexCursor  += iCount;

            s_MeshCache[s_UIQuad.MeshID] = { drawVertexOffset, drawIndexOffset, vCount, iCount, s_UIQuad.Revision, firstColor };
            bUsedStaticCache             = true;
        }
    }

    if (!bUsedStaticCache) {
        if (s_FrameVertexOffset + vCount > MAX_VERTICES_PER_FRAME ||
            s_FrameIndexOffset  + iCount > MAX_INDICES_PER_FRAME)
        {
            s_FrameVertexOffset = STATIC_VERTEX_CAPACITY;
            s_FrameIndexOffset  = STATIC_INDEX_CAPACITY;
        }

        drawVertexOffset = s_FrameVertexOffset;
        drawIndexOffset  = s_FrameIndexOffset;

        DX_MAPPED_SUBRESOURCE mappedVB{};
        if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 5, 0, &mappedVB) >= 0 && mappedVB.pData) {
            std::memcpy(static_cast<Vertex *>(mappedVB.pData) + drawVertexOffset, s_UIQuad.Vertices.data(), vCount * sizeof(Vertex));
            s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
        }

        DX_MAPPED_SUBRESOURCE mappedIB{};
        if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 5, 0, &mappedIB) >= 0 && mappedIB.pData) {
            std::memcpy(static_cast<UINT *>(mappedIB.pData) + drawIndexOffset, s_UIQuad.Indices.data(), iCount * sizeof(UINT));
            s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
        }

        s_FrameVertexOffset += vCount;
        s_FrameIndexOffset  += iCount;
    }

    DrawConstants dc = M_Mesh::Build_UIConstants(pos, size, color, transparency, false /* bFlipClipY */);

    DX_MAPPED_SUBRESOURCE mappedCB{};
    if (s_pfnMap(s_DeviceContext, s_ConstantBuffer, 0, 4, 0, &mappedCB) >= 0 && mappedCB.pData) {
        std::memcpy(mappedCB.pData, &dc, sizeof(DrawConstants));
        s_pfnUnmap(s_DeviceContext, s_ConstantBuffer, 0);
    }

    s_pfnDrawIndexed(s_DeviceContext, iCount, drawIndexOffset, static_cast<INT>(drawVertexOffset));
}

void R_DirectX::Draw_UI_SubRect(
    UINT            textureID,
    const Vector2D &pos,
    const Vector2D &size,
    const Vector2D &uvMin,
    const Vector2D &uvMax,
    const COLOR    &color,
    FLOAT           transparency)
{
    if (!s_IsFrameActive || !s_DeviceContext || transparency <= 0.001f || size.x <= 0.0f || size.y <= 0.0f) return;

    if (textureID != s_LastBoundTextureID) {
        void *srv = GetOrUploadTextureSRV(textureID);
        if (srv) {
            s_pfnPSSetSRV(s_DeviceContext, 0, 1, &srv);
            s_LastBoundTextureID = textureID;
        }
    }

    if (s_BufferNeedsDiscard) {
        DX_MAPPED_SUBRESOURCE dummy{};
        if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
        }
        if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
        }
        s_BufferNeedsDiscard = false;
    }

    constexpr UINT vCount = 4;
    constexpr UINT iCount = 6;

    if (s_FrameVertexOffset + vCount > MAX_VERTICES_PER_FRAME ||
        s_FrameIndexOffset  + iCount > MAX_INDICES_PER_FRAME)
    {
        s_FrameVertexOffset = STATIC_VERTEX_CAPACITY;
        s_FrameIndexOffset  = STATIC_INDEX_CAPACITY;
    }

    const UINT drawVertexOffset = s_FrameVertexOffset;
    const UINT drawIndexOffset  = s_FrameIndexOffset;

    const Vertex subVerts[4] = {
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { uvMin.x, uvMin.y }, COLOR_WHITE },
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { uvMax.x, uvMin.y }, COLOR_WHITE },
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { uvMax.x, uvMax.y }, COLOR_WHITE },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { uvMin.x, uvMax.y }, COLOR_WHITE }
    };
    static constexpr UINT s_SubIndices[6] = { 0, 1, 2, 2, 3, 0 };

    DX_MAPPED_SUBRESOURCE mappedVB{};
    if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 5, 0, &mappedVB) >= 0 && mappedVB.pData) {
        std::memcpy(static_cast<Vertex *>(mappedVB.pData) + drawVertexOffset, subVerts, vCount * sizeof(Vertex));
        s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
    }

    DX_MAPPED_SUBRESOURCE mappedIB{};
    if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 5, 0, &mappedIB) >= 0 && mappedIB.pData) {
        std::memcpy(static_cast<UINT *>(mappedIB.pData) + drawIndexOffset, s_SubIndices, iCount * sizeof(UINT));
        s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
    }

    s_FrameVertexOffset += vCount;
    s_FrameIndexOffset  += iCount;

    DrawConstants dc = M_Mesh::Build_UIConstants(pos, size, color, transparency, false /* bFlipClipY */);

    DX_MAPPED_SUBRESOURCE mappedCB{};
    if (s_pfnMap(s_DeviceContext, s_ConstantBuffer, 0, 4, 0, &mappedCB) >= 0 && mappedCB.pData) {
        std::memcpy(mappedCB.pData, &dc, sizeof(DrawConstants));
        s_pfnUnmap(s_DeviceContext, s_ConstantBuffer, 0);
    }

    s_pfnDrawIndexed(s_DeviceContext, iCount, drawIndexOffset, static_cast<INT>(drawVertexOffset));
}

void R_DirectX::Draw_UI_Quads(
    UINT          textureID,
    const UIQuad *quads,
    UINT          count)
{
    if (!s_IsFrameActive || !s_DeviceContext || !quads || count == 0) return;

    if (textureID != s_LastBoundTextureID) {
        void *srv = GetOrUploadTextureSRV(textureID);
        if (srv) {
            s_pfnPSSetSRV(s_DeviceContext, 0, 1, &srv);
            s_LastBoundTextureID = textureID;
        }
    }

    if (s_BufferNeedsDiscard) {
        DX_MAPPED_SUBRESOURCE dummy{};
        if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
        }
        if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 4, 0, &dummy) >= 0) {
            s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
        }
        s_BufferNeedsDiscard = false;
    }

    const UINT vCount = count * 4;
    const UINT iCount = count * 6;

    if (s_FrameVertexOffset + vCount > MAX_VERTICES_PER_FRAME ||
        s_FrameIndexOffset  + iCount > MAX_INDICES_PER_FRAME)
    {
        s_FrameVertexOffset = STATIC_VERTEX_CAPACITY;
        s_FrameIndexOffset  = STATIC_INDEX_CAPACITY;
    }

    const UINT drawVertexOffset = s_FrameVertexOffset;
    const UINT drawIndexOffset  = s_FrameIndexOffset;

    const FLOAT invW = (s_Width > 0)  ? (2.0f / static_cast<FLOAT>(s_Width))  : (2.0f / 1280.0f);
    const FLOAT invH = (s_Height > 0) ? (2.0f / static_cast<FLOAT>(s_Height)) : (2.0f / 720.0f);

    DX_MAPPED_SUBRESOURCE mappedVB{};
    if (s_pfnMap(s_DeviceContext, s_VertexBuffer, 0, 5, 0, &mappedVB) >= 0 && mappedVB.pData) {
        Vertex *outVerts = static_cast<Vertex *>(mappedVB.pData) + drawVertexOffset;
        for (UINT i = 0; i < count; ++i) {
            const UIQuad &q = quads[i];
            const FLOAT x0 = q.Min.x * invW - 1.0f;
            const FLOAT x1 = q.Max.x * invW - 1.0f;
            const FLOAT y0 = 1.0f - q.Min.y * invH;
            const FLOAT y1 = 1.0f - q.Max.y * invH;

            outVerts[i * 4 + 0] = { { x0, y0, 0.0f }, { 0.0f, 0.0f, -1.0f }, { q.UVMin.x, q.UVMin.y }, q.Color };
            outVerts[i * 4 + 1] = { { x1, y0, 0.0f }, { 0.0f, 0.0f, -1.0f }, { q.UVMax.x, q.UVMin.y }, q.Color };
            outVerts[i * 4 + 2] = { { x1, y1, 0.0f }, { 0.0f, 0.0f, -1.0f }, { q.UVMax.x, q.UVMax.y }, q.Color };
            outVerts[i * 4 + 3] = { { x0, y1, 0.0f }, { 0.0f, 0.0f, -1.0f }, { q.UVMin.x, q.UVMax.y }, q.Color };
        }
        s_pfnUnmap(s_DeviceContext, s_VertexBuffer, 0);
    }

    DX_MAPPED_SUBRESOURCE mappedIB{};
    if (s_pfnMap(s_DeviceContext, s_IndexBuffer, 0, 5, 0, &mappedIB) >= 0 && mappedIB.pData) {
        UINT *outIndices = static_cast<UINT *>(mappedIB.pData) + drawIndexOffset;
        for (UINT i = 0; i < count; ++i) {
            const UINT base = i * 4;
            outIndices[i * 6 + 0] = base + 0;
            outIndices[i * 6 + 1] = base + 1;
            outIndices[i * 6 + 2] = base + 2;
            outIndices[i * 6 + 3] = base + 2;
            outIndices[i * 6 + 4] = base + 3;
            outIndices[i * 6 + 5] = base + 0;
        }
        s_pfnUnmap(s_DeviceContext, s_IndexBuffer, 0);
    }

    s_FrameVertexOffset += vCount;
    s_FrameIndexOffset  += iCount;

    DrawConstants dc{};
    dc.MVP[0]  = 1.0f;
    dc.MVP[5]  = 1.0f;
    dc.MVP[10] = 1.0f;
    dc.MVP[15] = 1.0f;
    dc.Tint[0] = 1.0f;
    dc.Tint[1] = 1.0f;
    dc.Tint[2] = 1.0f;
    dc.Tint[3] = 1.0f;
    dc.Light[0] = 1.0f;
    dc.Light[1] = 1.0f;
    dc.Light[2] = 1.0f;
    dc.Light[3] = 0.0f; // Unlit 2D

    DX_MAPPED_SUBRESOURCE mappedCB{};
    if (s_pfnMap(s_DeviceContext, s_ConstantBuffer, 0, 4, 0, &mappedCB) >= 0 && mappedCB.pData) {
        std::memcpy(mappedCB.pData, &dc, sizeof(DrawConstants));
        s_pfnUnmap(s_DeviceContext, s_ConstantBuffer, 0);
    }

    s_pfnDrawIndexed(s_DeviceContext, iCount, drawIndexOffset, static_cast<INT>(drawVertexOffset));
}

void R_DirectX::End_Frame() {
    if (!s_IsInitialized || !s_SwapChain || !s_IsFrameActive) return;
    s_IsFrameActive = false;

    UINT syncInterval = s_VSync ? 1 : 0;
    UINT presentFlags = (!s_VSync && s_AllowTearing) ? 512u : 0u;
    void **scVtbl = *reinterpret_cast<void***>(s_SwapChain);

    using PFN_Present = int (DX_CALL *)(void *, UINT, UINT);
    reinterpret_cast<PFN_Present>(scVtbl[8])(s_SwapChain, syncInterval, presentFlags);
}

#undef DX_CALL
