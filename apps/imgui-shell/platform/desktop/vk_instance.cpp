// vk_instance.cpp
// VkInstance creation/teardown for the desktop host: gathers GLFW-required
// surface extensions, opts into MoltenVK portability enumeration when present,
// and in debug builds enables the Khronos validation layer + a debug-utils
// messenger that logs to stderr and aborts on validation errors.

#include "vk_instance.h"
#include "vk_check.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace desktop {

namespace {

#if defined(NDEBUG)
constexpr bool kEnableValidationByDefault = false;
#else
constexpr bool kEnableValidationByDefault = true;
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*userData*/)
{
    const char* level = "INFO";
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)        level = "ERROR";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) level = "WARN";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    level = "INFO";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) level = "VERBOSE";

    std::fprintf(stderr, "[vk %s] %s\n", level, data->pMessage);

    // Per design.md: assert on error severity.
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::fprintf(stderr, "[vk] validation error — aborting.\n");
        std::fflush(stderr);
        std::abort();
    }
    return VK_FALSE;
}

bool hasLayer(const char* name) {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto& l : layers) {
        if (std::strcmp(l.layerName, name) == 0) return true;
    }
    return false;
}

bool hasInstanceExtension(const char* name) {
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> exts(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, exts.data());
    for (const auto& e : exts) {
        if (std::strcmp(e.extensionName, name) == 0) return true;
    }
    return false;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info) {
    info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugCallback;
}

} // namespace

Instance createInstance() {
    Instance out{};
    out.validationEnabled = kEnableValidationByDefault;

    VkApplicationInfo app{};
    app.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "imgui-shell";
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName        = "imgui-shell";
    app.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
    app.apiVersion         = VK_API_VERSION_1_2;

    // GLFW-required surface extensions.
    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

    // macOS / MoltenVK is a "portability" implementation — required since Vulkan 1.3.216.
    bool portabilityEnumeration = hasInstanceExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    if (portabilityEnumeration) {
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    }

    if (out.validationEnabled) {
        if (hasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        } else {
            out.validationEnabled = false;
        }
    }

    std::vector<const char*> layers;
    if (out.validationEnabled && hasLayer("VK_LAYER_KHRONOS_validation")) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    } else {
        out.validationEnabled = false;
    }

    VkInstanceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &app;
    ci.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    ci.enabledLayerCount       = static_cast<uint32_t>(layers.size());
    ci.ppEnabledLayerNames     = layers.empty() ? nullptr : layers.data();
    if (portabilityEnumeration) {
        ci.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    VkDebugUtilsMessengerCreateInfoEXT dbg{};
    if (out.validationEnabled) {
        populateDebugMessengerCreateInfo(dbg);
        ci.pNext = &dbg; // catches validation issues during vkCreateInstance itself
    }

    VK_CHECK(vkCreateInstance(&ci, nullptr, &out.handle));

    if (out.validationEnabled) {
        auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(out.handle, "vkCreateDebugUtilsMessengerEXT"));
        if (fn) {
            VkDebugUtilsMessengerCreateInfoEXT mi{};
            populateDebugMessengerCreateInfo(mi);
            VK_CHECK(fn(out.handle, &mi, nullptr, &out.debugMessenger));
        }
    }

    return out;
}

void destroyInstance(Instance& inst) {
    if (inst.debugMessenger != VK_NULL_HANDLE) {
        auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(inst.handle, "vkDestroyDebugUtilsMessengerEXT"));
        if (fn) fn(inst.handle, inst.debugMessenger, nullptr);
        inst.debugMessenger = VK_NULL_HANDLE;
    }
    if (inst.handle != VK_NULL_HANDLE) {
        vkDestroyInstance(inst.handle, nullptr);
        inst.handle = VK_NULL_HANDLE;
    }
}

} // namespace desktop
