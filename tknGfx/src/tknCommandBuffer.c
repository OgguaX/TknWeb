#include "tknGfx.h"
#include "tknGfxInternal.h"

void tknBeginCommandBuffer(void *pTknGfxContext, uint32_t width, uint32_t height)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    VkDevice vkDevice = pGfxContext->vkDevice;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    tknAssertVkResult(vkWaitForFences(vkDevice, 1, &pGfxContext->vkRenderFinishedFence, VK_TRUE, UINT64_MAX));

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(
        vkDevice, pGfxContext->vkSwapchain, UINT64_MAX,
        pGfxContext->vkImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        // Use provided dimensions if valid, otherwise try to get from surface capabilities as fallback
        uint32_t swapchainWidth = width;
        uint32_t swapchainHeight = height;
        
        // Fallback for platforms where width/height might be 0 (though caller should provide valid values)
        if (width == 0 || height == 0)
        {
            tknAssertVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pGfxContext->vkPhysicalDevice, pGfxContext->vkSurface, &pGfxContext->vkSurfaceCapabilities));
            if (pGfxContext->vkSurfaceCapabilities.currentExtent.width != 0xFFFFFFFFU)
            {
                swapchainWidth = pGfxContext->vkSurfaceCapabilities.currentExtent.width;
                swapchainHeight = pGfxContext->vkSurfaceCapabilities.currentExtent.height;
            }
        }
        
        tknWarning("Swapchain out of date or suboptimal, attempting recreation with dimensions %u x %u\n", swapchainWidth, swapchainHeight);
        tknUpdateSwapchainPtr(pGfxContext, swapchainWidth, swapchainHeight);
        // Retry acquiring next image with recreated swapchain
        acquireResult = vkAcquireNextImageKHR(
            vkDevice, pGfxContext->vkSwapchain, UINT64_MAX,
            pGfxContext->vkImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    }
    tknAssertVkResult(acquireResult);

    tknAssertVkResult(vkResetFences(vkDevice, 1, &pGfxContext->vkRenderFinishedFence));
    tknAssertVkResult(vkResetCommandBuffer(vkCommandBuffer, 0));

    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    tknAssertVkResult(vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo));
}

void tknEndCommandBuffer(void *pTknGfxContext)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    VkDevice vkDevice = pGfxContext->vkDevice;
    VkQueue vkQueue = pGfxContext->vkGfxQueue;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    tknAssertVkResult(vkEndCommandBuffer(vkCommandBuffer));

    VkPipelineStageFlags waitDstStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &pGfxContext->vkImageAvailableSemaphore,
        .pWaitDstStageMask = waitDstStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkCommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &pGfxContext->vkRenderFinishedSemaphore,
    };
    tknAssertVkResult(vkQueueSubmit(vkQueue, 1, &submitInfo, pGfxContext->vkRenderFinishedFence));

    uint32_t currentImageIndex = (pGfxContext->frameCount - 1) % pGfxContext->swapchainImageCount;
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &pGfxContext->vkRenderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &pGfxContext->vkSwapchain,
        .pImageIndices = &currentImageIndex,
    };

    VkResult presentResult = vkQueuePresentKHR(vkQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
    {
        tknWarning("Swapchain presentation out of date/suboptimal, recreation scheduled for next frame\\n");
    }
    else
    {
        tknAssertVkResult(presentResult);
    }

    pGfxContext->frameCount++;
}
