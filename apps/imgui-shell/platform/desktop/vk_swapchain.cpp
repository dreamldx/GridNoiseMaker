#include "vk_swapchain.h"
#include "vk_check.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace desktop {

namespace {

VkSurfaceFormatKHR chooseFormat(VkPhysicalDevice pd, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &count, formats.data());

    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    if (formats.empty()) {
        throw std::runtime_error("Surface reports zero supported formats.");
    }
    return formats[0];
}

VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& caps, GLFWwindow* window) {
    if (caps.currentExtent.width != UINT32_MAX) {
        return caps.currentExtent;
    }
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    VkExtent2D extent{static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    extent.width  = std::clamp(extent.width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return extent;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat colorFormat) {
    VkAttachmentDescription color{};
    color.format         = colorFormat;
    color.samples        = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref{};
    ref.attachment = 0;
    ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &ref;

    VkSubpassDependency dep{};
    dep.srcSubpass   = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass   = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpci{};
    rpci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments    = &color;
    rpci.subpassCount    = 1;
    rpci.pSubpasses      = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies   = &dep;

    VkRenderPass rp = VK_NULL_HANDLE;
    VK_CHECK(vkCreateRenderPass(device, &rpci, nullptr, &rp));
    return rp;
}

} // namespace

Swapchain createSwapchain(VkPhysicalDevice pd,
                          VkDevice         device,
                          VkSurfaceKHR     surface,
                          GLFWwindow*      window)
{
    VkSurfaceCapabilitiesKHR caps{};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &caps));

    VkSurfaceFormatKHR fmt = chooseFormat(pd, surface);
    VkExtent2D         extent = chooseExtent(caps, window);

    uint32_t minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && minImageCount > caps.maxImageCount) {
        minImageCount = caps.maxImageCount;
    }
    if (minImageCount < 2) minImageCount = 2;

    VkSwapchainCreateInfoKHR sci{};
    sci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface          = surface;
    sci.minImageCount    = minImageCount;
    sci.imageFormat      = fmt.format;
    sci.imageColorSpace  = fmt.colorSpace;
    sci.imageExtent      = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform     = caps.currentTransform;
    sci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped          = VK_TRUE;
    sci.oldSwapchain     = VK_NULL_HANDLE;

    Swapchain out{};
    VK_CHECK(vkCreateSwapchainKHR(device, &sci, nullptr, &out.handle));
    out.format        = fmt.format;
    out.extent        = extent;
    out.minImageCount = minImageCount;

    uint32_t count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, out.handle, &count, nullptr));
    out.images.resize(count);
    VK_CHECK(vkGetSwapchainImagesKHR(device, out.handle, &count, out.images.data()));

    out.imageViews.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        VkImageViewCreateInfo vci{};
        vci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vci.image            = out.images[i];
        vci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        vci.format           = out.format;
        vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vci.subresourceRange.levelCount = 1;
        vci.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(device, &vci, nullptr, &out.imageViews[i]));
    }

    out.renderPass = createRenderPass(device, out.format);

    out.framebuffers.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        VkFramebufferCreateInfo fci{};
        fci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass      = out.renderPass;
        fci.attachmentCount = 1;
        fci.pAttachments    = &out.imageViews[i];
        fci.width           = out.extent.width;
        fci.height          = out.extent.height;
        fci.layers          = 1;
        VK_CHECK(vkCreateFramebuffer(device, &fci, nullptr, &out.framebuffers[i]));
    }

    out.renderFinishedSemaphores.resize(count);
    VkSemaphoreCreateInfo semci{};
    semci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    for (uint32_t i = 0; i < count; ++i) {
        VK_CHECK(vkCreateSemaphore(device, &semci, nullptr, &out.renderFinishedSemaphores[i]));
    }

    return out;
}

void destroySwapchain(VkDevice device, Swapchain& sc) {
    for (auto s : sc.renderFinishedSemaphores) vkDestroySemaphore(device, s, nullptr);
    sc.renderFinishedSemaphores.clear();
    for (auto fb : sc.framebuffers) vkDestroyFramebuffer(device, fb, nullptr);
    sc.framebuffers.clear();
    if (sc.renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, sc.renderPass, nullptr);
        sc.renderPass = VK_NULL_HANDLE;
    }
    for (auto v : sc.imageViews) vkDestroyImageView(device, v, nullptr);
    sc.imageViews.clear();
    sc.images.clear();
    if (sc.handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, sc.handle, nullptr);
        sc.handle = VK_NULL_HANDLE;
    }
}

void recreateSwapchain(VkPhysicalDevice pd,
                       VkDevice         device,
                       VkSurfaceKHR     surface,
                       GLFWwindow*      window,
                       Swapchain&       sc)
{
    // Wait for window to be non-minimized — Vulkan can't create a swapchain
    // with a zero-area surface.
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    while (w == 0 || h == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &w, &h);
    }

    vkDeviceWaitIdle(device);
    destroySwapchain(device, sc);
    sc = createSwapchain(pd, device, surface, window);
}

} // namespace desktop
