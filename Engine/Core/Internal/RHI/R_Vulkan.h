#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <M_Log.h>
#include <M_Window.h>
#include <M_Mesh.h>
#include <M_Texture.h>
#include <S_Mesh.h>

// ============================================================================
// Vulkan 1.3 / 1.4 Core Graphics Backend (Dynamic Rendering & Sync 2)
// ============================================================================
// Features:
//   - Pure Cross-Platform (Windows, Linux Wayland / X11 via SDL3)
//   - Modern Vulkan 1.3+ Dynamic Rendering (No VkRenderPass / VkFramebuffer bloat)
//   - Modern Synchronization 2 (vkCmdPipelineBarrier2 & vkQueueSubmit2)
//   - Triple Buffering CPU-GPU pipeline (MAX_FRAMES_IN_FLIGHT = 3)
//   - Dynamic VSync Toggle (IMMEDIATE for uncapped, FIFO for VSync)
//   - Built-in 3D/2D Mesh Pipeline with Alpha Blending, Textures & Push Constants
// ============================================================================
class R_Vulkan {
    friend class RHI_Manager;

public:
    static constexpr UINT MAX_FRAMES_IN_FLIGHT   = 3;
    static constexpr UINT MAX_VERTICES_PER_FRAME = 65536;
    static constexpr UINT MAX_INDICES_PER_FRAME  = 131072;
    static constexpr UINT STATIC_VERTEX_CAPACITY = 32768; // First half reserved for zero-copy cached static meshes
    static constexpr UINT STATIC_INDEX_CAPACITY  = 65536;

private:
    struct CachedMeshSlot {
        UINT  VertexOffset = 0;
        UINT  IndexOffset  = 0;
        UINT  VertexCount  = 0;
        UINT  IndexCount   = 0;
        UINT  Revision     = 0;
        DWORD FirstColor   = 0;
    };

    struct GPUTextureSlot {
        VkImage         Image         = VK_NULL_HANDLE;
        VkDeviceMemory  Memory        = VK_NULL_HANDLE;
        VkImageView     View          = VK_NULL_HANDLE;
        VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
        UINT            Revision      = 0;
    };

    // Core Vulkan Device Handles
    static VkInstance               s_Instance;
    static VkDebugUtilsMessengerEXT s_DebugMessenger;
    static VkSurfaceKHR             s_Surface;
    static VkPhysicalDevice         s_PhysicalDevice;
    static VkDevice                 s_Device;
    static VkQueue                  s_GraphicsQueue;
    static VkQueue                  s_PresentQueue;
    static UINT                     s_GraphicsQueueFamily;
    static UINT                     s_PresentQueueFamily;
    static VkCommandPool            s_CommandPool;
    static String                   s_GPUName;

    // Swapchain, Depth Buffer & Presentation Handles
    static VkSwapchainKHR           s_Swapchain;
    static VkFormat                 s_ImageFormat;
    static VkFormat                 s_DepthFormat;
    static VkColorSpaceKHR          s_ColorSpace;
    static VkExtent2D               s_Extent;
    static FLOAT                    s_AspectRatio;
    static Array<VkImage>                              s_Images;
    static Array<VkImageView>                          s_ImageViews;
    static fArray<VkImage, MAX_FRAMES_IN_FLIGHT>        s_DepthImages;
    static fArray<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> s_DepthMemories;
    static fArray<VkImageView, MAX_FRAMES_IN_FLIGHT>    s_DepthImageViews;

    // Graphics Pipeline, Texture Descriptors & Per-Frame Dynamic Mesh Buffers
    static VkDescriptorSetLayout    s_DescriptorSetLayout;
    static VkDescriptorPool         s_DescriptorPool;
    static VkSampler                s_TextureSampler;
    static uMap<UINT, GPUTextureSlot> s_TextureCache;
    static fArray<VkDescriptorSet, 64> s_FastTextureSets;
    static UINT                     s_LastBoundTextureID;
    static VkPipelineLayout         s_PipelineLayout;
    static VkPipeline               s_Pipeline;
    static fArray<VkBuffer, MAX_FRAMES_IN_FLIGHT>       s_VertexBuffers;
    static fArray<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> s_VertexMemories;
    static fArray<Vertex *, MAX_FRAMES_IN_FLIGHT>       s_MappedVertices;
    static fArray<VkBuffer, MAX_FRAMES_IN_FLIGHT>       s_IndexBuffers;
    static fArray<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> s_IndexMemories;
    static fArray<UINT *, MAX_FRAMES_IN_FLIGHT>         s_MappedIndices;
    static fArray<uMap<UINT, CachedMeshSlot>, MAX_FRAMES_IN_FLIGHT> s_MeshCache;
    static fArray<UINT, MAX_FRAMES_IN_FLIGHT>           s_StaticVertexCursor;
    static fArray<UINT, MAX_FRAMES_IN_FLIGHT>           s_StaticIndexCursor;
    static UINT                                         s_FrameVertexOffset;
    static UINT                                         s_FrameIndexOffset;

    // Per-frame Synchronization Primitives
    static fArray<VkSemaphore, MAX_FRAMES_IN_FLIGHT>     s_ImageAvailableSemaphores;
    static Array<VkSemaphore>                            s_RenderFinishedSemaphores;
    static fArray<VkFence, MAX_FRAMES_IN_FLIGHT>         s_InFlightFences;
    static fArray<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> s_CommandBuffers;

    // State Flags
    static UINT  s_CurrentFrameIndex;
    static UINT  s_CurrentImageIndex;
    static BOOL  s_IsInitialized;
    static BOOL  s_IsFrameActive;
    static BOOL  s_NeedsResize;
    static BOOL  s_VSync;
    static COLOR s_ClearColor;

    static constexpr BOOL s_EnableValidationLayers = false;
    static const fArray<const Char *, 1> s_ValidationLayers;

    // Internal Helper Functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);

    static BOOL CheckValidationLayerSupport();
    static VkSurfaceFormatKHR ChooseSurfaceFormat(const Array<VkSurfaceFormatKHR> &formats);
    static VkPresentModeKHR ChoosePresentMode(const Array<VkPresentModeKHR> &modes, BOOL vSync);
    static VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    static UINT FindMemoryType(UINT typeFilter, VkMemoryPropertyFlags properties);
    static BOOL CreateHostBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer &outBuffer, VkDeviceMemory &outMemory, void **outMapped);
    static BOOL CreateMeshPipelineAndBuffers();
    static void DestroyMeshPipelineAndBuffers();
    static VkDescriptorSet GetOrUploadTextureSet(UINT textureID);
    static BOOL CreateSwapchain();
    static void CleanupSwapchain();

public:
    // Initializes Vulkan 1.3 Instance, Physical & Logical Devices, Swapchain, and Pipelines
    static BOOL Init(SDL_Window *window);

    // Shuts down all Vulkan resources cleanly
    static void Shutdown();

    // Handles window resizing & minimizing smoothly
    static void Recreate();

    // Prepares the screen for drawing. Clears screen to s_ClearColor using Dynamic Rendering.
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

    // Finishes rendering, transitions image to PRESENT layout and presents to screen
    static void End_Frame();

    // ========================================================================
    // Setters & Getters
    // ========================================================================
    static inline void Set_VSync(BOOL enabled) {
        if (s_VSync == enabled) return;
        s_VSync = enabled;
        s_NeedsResize = true;
    }

    static inline BOOL       Get_VSync()                     { return s_VSync; }
    static inline void       Set_ClearColor(const COLOR &c)  { s_ClearColor = c; }
    static inline COLOR      Get_ClearColor()                { return s_ClearColor; }
    static inline StringView Get_GPUName()                   { return s_GPUName; }
    static inline BOOL       Is_Initialized()                { return s_IsInitialized; }
    static inline VkCommandBuffer Get_CurrentCommandBuffer() { return s_CommandBuffers[s_CurrentFrameIndex]; }
};
