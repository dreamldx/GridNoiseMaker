#pragma once

#include <vulkan/vulkan.h>

namespace desktop {

struct Instance {
    VkInstance               handle        = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool                     validationEnabled = false;
};

// Creates a Vulkan 1.2 instance with GLFW-required extensions. In debug builds
// (NDEBUG not defined), enables VK_LAYER_KHRONOS_validation + VK_EXT_debug_utils
// and installs a debug messenger that writes validation messages to stderr.
Instance createInstance();
void destroyInstance(Instance& inst);

} // namespace desktop
