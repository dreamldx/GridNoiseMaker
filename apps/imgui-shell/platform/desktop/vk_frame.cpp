#include "vk_frame.h"
#include "vk_check.h"

namespace desktop {

FrameResources createFrameResources(VkDevice device, uint32_t queueFamilyIndex) {
    FrameResources out{};

    VkCommandPoolCreateInfo pci{};
    pci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = queueFamilyIndex;
    VK_CHECK(vkCreateCommandPool(device, &pci, nullptr, &out.commandPool));

    VkCommandBufferAllocateInfo abi{};
    abi.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    abi.commandPool        = out.commandPool;
    abi.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    abi.commandBufferCount = 1;

    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT; // first frame doesn't wait on anything

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VK_CHECK(vkAllocateCommandBuffers(device, &abi, &out.frames[i].commandBuffer));
        VK_CHECK(vkCreateSemaphore(device, &sci, nullptr, &out.frames[i].imageAvailable));
        VK_CHECK(vkCreateFence(device, &fci, nullptr, &out.frames[i].inFlight));
    }
    return out;
}

void destroyFrameResources(VkDevice device, FrameResources& fr) {
    for (auto& f : fr.frames) {
        if (f.inFlight       != VK_NULL_HANDLE) vkDestroyFence(device, f.inFlight, nullptr);
        if (f.imageAvailable != VK_NULL_HANDLE) vkDestroySemaphore(device, f.imageAvailable, nullptr);
        f = {};
    }
    if (fr.commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, fr.commandPool, nullptr);
        fr.commandPool = VK_NULL_HANDLE;
    }
}

} // namespace desktop
