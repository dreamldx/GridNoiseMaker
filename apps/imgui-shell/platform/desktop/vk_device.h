#pragma once

#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace desktop {

struct Device {
    VkSurfaceKHR     surface         = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice  = VK_NULL_HANDLE;
    VkDevice         device          = VK_NULL_HANDLE;
    VkQueue          queue           = VK_NULL_HANDLE;
    uint32_t         queueFamilyIndex = 0;
};

// Creates a window surface via GLFW, picks a suitable physical device (graphics +
// presents to the surface + supports VK_KHR_swapchain), and creates a logical
// device with one graphics+present queue.
Device createDevice(VkInstance instance, GLFWwindow* window);
void   destroyDevice(VkInstance instance, Device& d);

} // namespace desktop
