#pragma once

#include <vulkan/vulkan.h>

#include <array>

namespace desktop {

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

// Per-frame-in-flight resources. Note: render-finished semaphores are NOT here —
// they live in Swapchain (per swapchain image), because a binary semaphore
// signaled by vkQueuePresentKHR may be in use until the image is re-acquired.
struct PerFrame {
    VkCommandBuffer commandBuffer       = VK_NULL_HANDLE;
    VkSemaphore     imageAvailable      = VK_NULL_HANDLE;
    VkFence         inFlight            = VK_NULL_HANDLE;
};

struct FrameResources {
    VkCommandPool                        commandPool = VK_NULL_HANDLE;
    std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> frames{};
};

FrameResources createFrameResources(VkDevice device, uint32_t queueFamilyIndex);
void           destroyFrameResources(VkDevice device, FrameResources& fr);

} // namespace desktop
