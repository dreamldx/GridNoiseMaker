// imgui-shell desktop host (GLFW + Vulkan).
// See specs/render-backend/spec.md for the contract this implements.

#include "vk_check.h"
#include "vk_instance.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_frame.h"

#include "App.h"
#include "RenderContext.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <array>
#include <cstdio>
#include <stdexcept>

namespace {

bool g_framebufferResized = false;

// Live-resize state — see specs/render-backend "Live-resize behavior on desktop".
// Populated in runApp() before glfwSet*Callback registration, cleared at
// shutdown. Used by the resize/refresh callbacks to drive a full ImGui frame
// from inside Cocoa's modal-loop thread context so the window re-lays-out
// live during a drag rather than freezing.
struct ResizeCallbackState {
    VkDevice                 device         = VK_NULL_HANDLE;
    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR             surface        = VK_NULL_HANDLE;
    VkQueue                  queue          = VK_NULL_HANDLE;
    GLFWwindow*              window         = nullptr;
    desktop::Swapchain*      swapchain      = nullptr;
    desktop::FrameResources* frames         = nullptr;
    app::RenderContext*      ctx            = nullptr;
    uint32_t*                frameIndex     = nullptr;
};
ResizeCallbackState* g_resizeState = nullptr;

VkDescriptorPool createImGuiDescriptorPool(VkDevice device) {
    // Per specs/render-backend, ImGui descriptor pool sized for font + per-frame.
    constexpr uint32_t kPoolSize = 1000;
    VkDescriptorPoolSize sizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kPoolSize },
    };
    VkDescriptorPoolCreateInfo pci{};
    pci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pci.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pci.maxSets       = kPoolSize;
    pci.poolSizeCount = static_cast<uint32_t>(std::size(sizes));
    pci.pPoolSizes    = sizes;

    VkDescriptorPool pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(device, &pci, nullptr, &pool));
    return pool;
}

// Renders one full ImGui frame: backend NewFrames → ImGui::NewFrame (inside
// app::frame) → app UI → ImGui::Render → submit → present. Returns false if
// the swapchain had to be recreated (caller may want to skip-and-resume).
//
// Called from BOTH the main loop AND the GLFW resize/refresh callbacks so the
// window re-lays-out during a Cocoa live-resize gesture rather than stretching.
// Both call sites share `frameIndex` via the caller's variable, advanced here.
bool renderOneFrame(VkDevice                  vkdev,
                    VkPhysicalDevice          pd,
                    VkSurfaceKHR              surface,
                    VkQueue                   queue,
                    GLFWwindow*               window,
                    desktop::Swapchain&       swapchain,
                    desktop::FrameResources&  frames,
                    app::RenderContext&       ctx,
                    uint32_t&                 frameIndex)
{
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0) return false; // minimized

    desktop::PerFrame& f = frames.frames[frameIndex];

    vkWaitForFences(vkdev, 1, &f.inFlight, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(
        vkdev, swapchain.handle, UINT64_MAX,
        f.imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        desktop::recreateSwapchain(pd, vkdev, surface, window, swapchain);
        ImGui_ImplVulkan_SetMinImageCount(swapchain.minImageCount);
        ctx.renderPass    = swapchain.renderPass;
        ctx.minImageCount = swapchain.minImageCount;
        ctx.imageCount    = static_cast<uint32_t>(swapchain.images.size());
        return false;
    } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    vkResetFences(vkdev, 1, &f.inFlight);
    vkResetCommandBuffer(f.commandBuffer, 0);

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(f.commandBuffer, &bi);

    VkClearValue clear{};
    clear.color = { { 0.10f, 0.10f, 0.12f, 1.0f } };

    VkRenderPassBeginInfo rbi{};
    rbi.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rbi.renderPass        = swapchain.renderPass;
    rbi.framebuffer       = swapchain.framebuffers[imageIndex];
    rbi.renderArea.extent = swapchain.extent;
    rbi.clearValueCount   = 1;
    rbi.pClearValues      = &clear;
    vkCmdBeginRenderPass(f.commandBuffer, &rbi, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ctx.currentCommandBuffer = f.commandBuffer;
    app::frame(ctx); // builds UI; calls ImGui::NewFrame + ImGui::Render
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), f.commandBuffer);

    vkCmdEndRenderPass(f.commandBuffer);
    vkEndCommandBuffer(f.commandBuffer);

    // Multi-viewport — see specs/render-backend "Multi-viewport support on
    // desktop". When the user has dragged any ImGui sub-window outside the
    // main host, ImGui owns a per-viewport `ImGui_ImplVulkanH_Window` for it
    // with its own swapchain + command buffers + sync. UpdatePlatformWindows
    // walks the viewport list and invokes the platform callbacks (window
    // create/destroy/move/resize); RenderPlatformWindowsDefault walks it
    // again and invokes the renderer callbacks (submit + present each
    // secondary viewport). The main viewport is flagged ImGuiViewportFlags_
    // OwnedByApp so it's skipped here — we present it ourselves below.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    VkSemaphore           renderFinished = swapchain.renderFinishedSemaphores[imageIndex];
    VkPipelineStageFlags  waitStage      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &f.imageAvailable;
    submit.pWaitDstStageMask    = &waitStage;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &f.commandBuffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &renderFinished;
    vkQueueSubmit(queue, 1, &submit, f.inFlight);

    VkPresentInfoKHR present{};
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = &renderFinished;
    present.swapchainCount     = 1;
    present.pSwapchains        = &swapchain.handle;
    present.pImageIndices      = &imageIndex;
    VkResult presentResult = vkQueuePresentKHR(queue, &present);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        desktop::recreateSwapchain(pd, vkdev, surface, window, swapchain);
        ImGui_ImplVulkan_SetMinImageCount(swapchain.minImageCount);
        ctx.renderPass    = swapchain.renderPass;
        ctx.minImageCount = swapchain.minImageCount;
        ctx.imageCount    = static_cast<uint32_t>(swapchain.images.size());
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    frameIndex = (frameIndex + 1) % desktop::MAX_FRAMES_IN_FLIGHT;
    return true;
}

// Recreate the swapchain at the current framebuffer size if it has changed,
// then render one full ImGui frame so the window re-lays-out live during a
// macOS Cocoa modal-loop resize gesture. Safe to call from a GLFW callback
// because the main loop is paused inside glfwPollEvents — ImGui is between
// frames, so re-entering NewFrame from this thread context is well-defined.
void resizeCallbackWork(GLFWwindow* w) {
    if (!g_resizeState) return;
    auto& s = *g_resizeState;

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(w, &fbw, &fbh);
    if (fbw <= 0 || fbh <= 0) return; // minimized

    if (s.swapchain->extent.width  != static_cast<uint32_t>(fbw) ||
        s.swapchain->extent.height != static_cast<uint32_t>(fbh))
    {
        vkDeviceWaitIdle(s.device);
        desktop::destroySwapchain(s.device, *s.swapchain);
        *s.swapchain = desktop::createSwapchain(
            s.physicalDevice, s.device, s.surface, s.window);
        ImGui_ImplVulkan_SetMinImageCount(s.swapchain->minImageCount);
        s.ctx->renderPass    = s.swapchain->renderPass;
        s.ctx->minImageCount = s.swapchain->minImageCount;
        s.ctx->imageCount    = static_cast<uint32_t>(s.swapchain->images.size());
    }

    renderOneFrame(s.device, s.physicalDevice, s.surface, s.queue, s.window,
                   *s.swapchain, *s.frames, *s.ctx, *s.frameIndex);
}

void onResize(GLFWwindow* w, int /*width*/, int /*height*/) {
    g_framebufferResized = true;
    resizeCallbackWork(w);
}

void onRefresh(GLFWwindow* w) {
    resizeCallbackWork(w);
}

void runApp() {
    if (!glfwInit()) {
        throw std::runtime_error("glfwInit failed");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!glfwVulkanSupported()) {
        glfwTerminate();
        throw std::runtime_error("GLFW reports no Vulkan support on this system.");
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "imgui-shell", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    desktop::Instance       instance  = desktop::createInstance();
    desktop::Device         device    = desktop::createDevice(instance.handle, window);
    desktop::Swapchain      swapchain = desktop::createSwapchain(
                                            device.physicalDevice, device.device, device.surface, window);
    desktop::FrameResources frames    = desktop::createFrameResources(
                                            device.device, device.queueFamilyIndex);
    VkDescriptorPool        imguiPool = createImGuiDescriptorPool(device.device);

    // ---- App + ImGui init ----
    app::RenderContext ctx{};
    ctx.window           = window;
    ctx.instance         = instance.handle;
    ctx.physicalDevice   = device.physicalDevice;
    ctx.device           = device.device;
    ctx.queue            = device.queue;
    ctx.queueFamilyIndex = device.queueFamilyIndex;
    ctx.renderPass       = swapchain.renderPass;
    ctx.descriptorPool   = imguiPool;
    ctx.minImageCount    = swapchain.minImageCount;
    ctx.imageCount       = static_cast<uint32_t>(swapchain.images.size());

    app::init(ctx);

    // Backend init order — platform first, then renderer (per spec).
    ImGui_ImplGlfw_InitForVulkan(window, /*install_callbacks=*/true);

    ImGui_ImplVulkan_InitInfo info{};
    info.Instance        = instance.handle;
    info.PhysicalDevice  = device.physicalDevice;
    info.Device          = device.device;
    info.QueueFamily     = device.queueFamilyIndex;
    info.Queue           = device.queue;
    info.PipelineCache   = VK_NULL_HANDLE;
    info.DescriptorPool  = imguiPool;
    info.Subpass         = 0;
    info.MinImageCount   = swapchain.minImageCount;
    info.ImageCount      = static_cast<uint32_t>(swapchain.images.size());
    info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
    info.RenderPass      = swapchain.renderPass;
    info.Allocator       = nullptr;
    info.CheckVkResultFn = nullptr;

    ImGui_ImplVulkan_Init(&info);
    ImGui_ImplVulkan_CreateFontsTexture();

    // ---- Wire the resize callbacks AFTER ImGui backends are initialized ----
    // The callbacks call renderOneFrame, which calls ImGui_Impl{Vulkan,Glfw}_NewFrame.
    // Those require the backends to exist; therefore registration happens here,
    // not earlier in runApp.
    uint32_t frameIndex = 0;
    ResizeCallbackState resizeState{};
    resizeState.device         = device.device;
    resizeState.physicalDevice = device.physicalDevice;
    resizeState.surface        = device.surface;
    resizeState.queue          = device.queue;
    resizeState.window         = window;
    resizeState.swapchain      = &swapchain;
    resizeState.frames         = &frames;
    resizeState.ctx            = &ctx;
    resizeState.frameIndex     = &frameIndex;
    g_resizeState              = &resizeState;
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSetWindowRefreshCallback(window, onRefresh);

    // ---- Main loop ----
    while (!glfwWindowShouldClose(window) && !app::quitRequested()) {
        glfwPollEvents();
        renderOneFrame(device.device, device.physicalDevice, device.surface,
                       device.queue, window, swapchain, frames, ctx, frameIndex);
        // g_framebufferResized is set by onResize but no longer drives a
        // dedicated recreate block — renderOneFrame handles swapchain
        // out-of-date / suboptimal results directly. Clear the flag to keep
        // it tidy.
        g_framebufferResized = false;
    }

    // ---- Reverse-order teardown ----
    // Per specs/render-backend "Correct shutdown order": ImGui backends MUST
    // release their BackendRendererUserData / BackendPlatformUserData (via
    // *_Shutdown()) BEFORE app::shutdown() destroys the ImGui context, because
    // ImGui::Shutdown() itself asserts on those fields being null.
    vkDeviceWaitIdle(device.device);
    // Stop the callbacks from firing into freed Vulkan state during shutdown.
    glfwSetFramebufferSizeCallback(window, nullptr);
    glfwSetWindowRefreshCallback(window, nullptr);
    g_resizeState = nullptr;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    app::shutdown();
    desktop::destroyFrameResources(device.device, frames);
    vkDestroyDescriptorPool(device.device, imguiPool, nullptr);
    desktop::destroySwapchain(device.device, swapchain);
    desktop::destroyDevice(instance.handle, device);
    desktop::destroyInstance(instance);
    glfwDestroyWindow(window);
    glfwTerminate();
}

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    try {
        runApp();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "imgui-shell fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
