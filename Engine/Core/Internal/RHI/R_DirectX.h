#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <M_Log.h>
#include <M_Window.h>
#include <M_Mesh.h>
#include <M_Texture.h>
#include <Shaders/S_Mesh.h>

// ============================================================================
// Pure Cross-Platform DirectX 11 Dynamic Driver (Zero Windows OS Headers)
// ============================================================================
// 1 Compile, 1 Build: Compiles identically on Windows, Linux, and macOS without
// any #ifdef _WIN32 or Windows-specific compilation guards.
// Uses SDL3's universal dynamic loader to interface with d3d11.dll / DXVK.
// ============================================================================
class R_DirectX {
    friend class RHI_Manager;

public:
    static constexpr UINT MAX_VERTICES_PER_FRAME = 65536;
    static constexpr UINT MAX_INDICES_PER_FRAME  = 131072;
    static constexpr UINT STATIC_VERTEX_CAPACITY = 32768; // First half reserved for zero-copy cached static meshes
    static constexpr UINT STATIC_INDEX_CAPACITY  = 65536;

    struct CachedMeshSlot {
        UINT  VertexOffset = 0;
        UINT  IndexOffset  = 0;
        UINT  VertexCount  = 0;
        UINT  IndexCount   = 0;
        UINT  Revision     = 0;
        DWORD FirstColor   = 0;
    };

    // Active Handles & Context
    static SDL_SharedObject *s_D3D11Module;
    static void             *s_Device;
    static void             *s_DeviceContext;
    static void             *s_SwapChain;
    static void             *s_RenderTargetView;
    static void             *s_DepthStencilBuffer;
    static void             *s_DepthStencilView;
    static SDL_Window       *s_Window;
    static INT               s_Width;
    static INT               s_Height;

    // Mesh Pipeline Handles
    static void             *s_VertexShader;
    static void             *s_PixelShader;
    static void             *s_InputLayout;
    static void             *s_ConstantBuffer;
    static void             *s_VertexBuffer;
    static void             *s_IndexBuffer;
    static void             *s_BlendState;
    static void             *s_DepthStencilState;
    static void             *s_RasterizerState;
    static void             *s_SamplerState;

    // Renderer State
    static BOOL   s_IsInitialized;
    static BOOL   s_IsFrameActive;
    static BOOL   s_VSync;
    static BOOL   s_AllowTearing;
    static COLOR  s_ClearColor;
    static String s_GPUName;

public:
    // Pure runtime probe: checks whether d3d11.dll is present and functional
    static BOOL Is_Supported();

    // Initializes DirectX 11 Device, Immediate Context, SwapChain, and Mesh Pipeline
    static BOOL Init(SDL_Window *window);

    // Shuts down all D3D11 and DXGI resources cleanly
    static void Shutdown();

    // Handles live window resizing by recreating swapchain buffers
    static void Recreate();

    // Binds all static pipeline states to D3D11 DeviceContext once (Zero per-frame virtual calls!)
    static void BindMeshPipelineStates();

    // Prepares the frame by clearing the render target view to s_ClearColor
    static BOOL Begin_Frame();

    // Draws a 3D/2D Mesh with transform, color tint, alpha transparency, and optional TextureID.
    // Automatically caches static meshes & textures in GPU memory so unchanged assets are NEVER re-uploaded!
    static void Draw_Mesh(
        const MeshData &mesh,
        const Vector3D &pos,
        const Vector3D &rot,
        const Vector3D &size,
        const COLOR    &color,
        FLOAT           transparency,
        UINT            textureID = 0);

    // Draws a 2D UI screen-space quad using orthographic projection
    static void Draw_UI(
        UINT            textureID,
        const Vector2D &pos,
        const Vector2D &size,
        const COLOR    &color,
        FLOAT           transparency);

    // Draws a 2D UI quad with custom UV sub-rectangle (sprite atlas / font glyph)
    static void Draw_UI_SubRect(
        UINT            textureID,
        const Vector2D &pos,
        const Vector2D &size,
        const Vector2D &uvMin,
        const Vector2D &uvMax,
        const COLOR    &color,
        FLOAT           transparency);

    // Batched 2D UI Quad rendering: draws an array of screen quads in a single GPU draw call!
    static void Draw_UI_Quads(
        UINT          textureID,
        const UIQuad *quads,
        UINT          count);

    // Presents the frame to screen, obeying s_VSync
    static void End_Frame();

    // VSync & Settings Accessors
    static inline void       Set_VSync(BOOL enabled)       { s_VSync = enabled; }
    static inline BOOL       Get_VSync()                   { return s_VSync; }
    static inline void       Set_ClearColor(const COLOR &c){ s_ClearColor = c; }
    static inline COLOR      Get_ClearColor()              { return s_ClearColor; }
    static inline StringView Get_GPUName()                 { return s_GPUName; }
    static inline BOOL       Is_Initialized()              { return s_IsInitialized; }
};
