#include "vk_device.h"
#include "vk_check.h"

#include <GLFW/glfw3.h>

#include <cstring>
#include <optional>
#include <stdexcept>
#include <vector>

namespace desktop {

namespace {

bool deviceHasExtension(VkPhysicalDevice pd, const char* name) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(pd, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> exts(count);
    vkEnumerateDeviceExtensionProperties(pd, nullptr, &count, exts.data());
    for (const auto& e : exts) {
        if (std::strcmp(e.extensionName, name) == 0) return true;
    }
    return false;
}

std::optional<uint32_t> pickQueueFamily(VkPhysicalDevice pd, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        bool graphics = (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &present);
        if (graphics && present) return i;
    }
    return std::nullopt;
}

} // namespace

Device createDevice(VkInstance instance, GLFWwindow* window) {
    Device out{};

    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &out.surface));

    // Enumerate physical devices and pick the first that satisfies our needs.
    uint32_t pdCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &pdCount, nullptr));
    if (pdCount == 0) {
        throw std::runtime_error("No Vulkan-capable physical devices found.");
    }
    std::vector<VkPhysicalDevice> pds(pdCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &pdCount, pds.data()));

    VkPhysicalDevice chosen = VK_NULL_HANDLE;
    uint32_t chosenQueue = 0;
    for (VkPhysicalDevice pd : pds) {
        if (!deviceHasExtension(pd, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;
        auto q = pickQueueFamily(pd, out.surface);
        if (!q) continue;
        chosen      = pd;
        chosenQueue = *q;
        break;
    }
    if (chosen == VK_NULL_HANDLE) {
        throw std::runtime_error("No physical device supports graphics + present + swapchain.");
    }
    out.physicalDevice   = chosen;
    out.queueFamilyIndex = chosenQueue;

    // Build device extension list. swapchain is required. portability_subset is
    // required-if-present (MoltenVK exposes it).
    std::vector<const char*> deviceExts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (deviceHasExtension(chosen, "VK_KHR_portability_subset")) {
        deviceExts.push_back("VK_KHR_portability_subset");
    }

    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = out.queueFamilyIndex;
    qci.queueCount       = 1;
    qci.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures features{}; // none required by v1

    VkDeviceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = 1;
    ci.pQueueCreateInfos       = &qci;
    ci.enabledExtensionCount   = static_cast<uint32_t>(deviceExts.size());
    ci.ppEnabledExtensionNames = deviceExts.data();
    ci.pEnabledFeatures        = &features;

    VK_CHECK(vkCreateDevice(out.physicalDevice, &ci, nullptr, &out.device));
    vkGetDeviceQueue(out.device, out.queueFamilyIndex, 0, &out.queue);
    return out;
}

void destroyDevice(VkInstance instance, Device& d) {
    if (d.device != VK_NULL_HANDLE) {
        vkDestroyDevice(d.device, nullptr);
        d.device = VK_NULL_HANDLE;
    }
    if (d.surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, d.surface, nullptr);
        d.surface = VK_NULL_HANDLE;
    }
    d.physicalDevice = VK_NULL_HANDLE;
    d.queue          = VK_NULL_HANDLE;
}

} // namespace desktop
