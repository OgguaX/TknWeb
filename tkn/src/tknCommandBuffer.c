#include "tknGfx.h"
#include "tknCore.h"

void tknBeginCommandBuffer(void *pTknGfxContext)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    tknAssertVkResult(vkResetCommandBuffer(vkCommandBuffer, 0));

    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    tknAssertVkResult(vkBeginCommandBuffer(vkCommandBuffer, &vkCommandBufferBeginInfo));
}

void tknEndCommandBuffer(void *pTknGfxContext)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    tknAssertVkResult(vkEndCommandBuffer(vkCommandBuffer));

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkCommandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL,
    };

    tknAssertVkResult(vkQueueSubmit(pGfxContext->vkGfxQueue, 1, &submitInfo, VK_NULL_HANDLE));
    tknAssertVkResult(vkQueueWaitIdle(pGfxContext->vkGfxQueue));

    pGfxContext->frameCount++;
}
