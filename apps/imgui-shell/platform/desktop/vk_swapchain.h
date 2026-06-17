#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

struct GLFWwindow;

namespace desktop {

struct Swapchain {
    VkSwapchainKHR             handle      = VK_NULL_HANDLE;
    VkFormat                   format      = VK_FORMAT_UNDEFINED;
    VkExtent2D                 extent      = {0, 0};
    std::vector<VkImage>       images;
    std::vector<VkImageView>   imageViews;
    std::vector<VkFramebuffer> framebuffers;
    // Per-image "render-finished" semaphores: a presentation operation may
    // hold a binary semaphore until the image is re-acquired, so we cannot
    // safely re-use a per-frame-in-flight semaphore as the present-wait. Sized
    // to images.size().
    std::vector<VkSemaphore>   renderFinishedSemaphores;
    VkRenderPass               renderPass  = VK_NULL_HANDLE;
    uint32_t                   minImageCount = 0;
};

// Creates / re-creates a swapchain + image views + a single-color render pass +
// matching framebuffers. The host calls `recreate` after VK_ERROR_OUT_OF_DATE_KHR
// or VK_SUBOPTIMAL_KHR, or after a window resize. `destroy` tears everything
// down (in reverse order). Both call vkDeviceWaitIdle internally where needed.
Swapchain createSwapchain(VkPhysicalDevice pd,
                          VkDevice         device,
                          VkSurfaceKHR     surface,
                          GLFWwindow*      window);
void      destroySwapchain(VkDevice device, Swapchain& sc);
void      recreateSwapchain(VkPhysicalDevice pd,
                            VkDevice         device,
                            VkSurfaceKHR     surface,
                            GLFWwindow*      window,
                            Swapchain&       sc);

} // namespace desktop
