#include "tknGfx.h"
#include "tknGfxInternal.h"

void tknDraw(void *pTknGfxContext, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");
    tknAssert(vertexCount > 0, "Vertex count must be greater than 0");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    vkCmdDraw(vkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void tknDrawIndexed(void *pTknGfxContext, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");
    tknAssert(indexCount > 0, "Index count must be greater than 0");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    vkCmdDrawIndexed(vkCommandBuffer, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}