#pragma once

#include <cstdint>

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)
    #include <vulkan/vulkan.h>
    struct GLFWwindow;
#endif

namespace app {

#if defined(IMGUI_SHELL_PLATFORM_DESKTOP)

// Populated by the desktop host (platform/desktop) before app::init / app::frame.
// app/ code may read these fields but is not required to.
struct RenderContext {
    GLFWwindow*       window           = nullptr;
    VkInstance        instance         = VK_NULL_HANDLE;
    VkPhysicalDevice  physicalDevice   = VK_NULL_HANDLE;
    VkDevice          device           = VK_NULL_HANDLE;
    VkQueue           queue            = VK_NULL_HANDLE;
    uint32_t          queueFamilyIndex = 0;
    VkRenderPass      renderPass       = VK_NULL_HANDLE;
    VkDescriptorPool  descriptorPool   = VK_NULL_HANDLE;
    uint32_t          minImageCount    = 2;
    uint32_t          imageCount       = 2;

    // Per-frame command buffer; host updates this before each app::frame call.
    VkCommandBuffer   currentCommandBuffer = VK_NULL_HANDLE;
};

#elif defined(IMGUI_SHELL_PLATFORM_IOS)

// iOS handles are kept as void* so this header stays valid in both .cpp and
// .mm translation units. iOS host code casts to id<MTLDevice> / id<MTLCommandQueue>
// / CAMetalLayer* at the boundary.
struct RenderContext {
    void* device       = nullptr;
    void* commandQueue = nullptr;
    void* metalLayer   = nullptr;
};

#else
    #error "RenderContext.h requires IMGUI_SHELL_PLATFORM_DESKTOP or IMGUI_SHELL_PLATFORM_IOS"
#endif

} // namespace app
