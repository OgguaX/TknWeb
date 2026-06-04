#include "tkn.h"
#include "tknGfx.h"

void *tknCreateBufferPtr(void *pTknGfxContext, uint64_t size, int usage, bool mappedAtCreation, const void *pData)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");
    tknAssert(size > 0, "Buffer size must be greater than 0");
    tknAssert(mappedAtCreation || pData != NULL, "Buffer must be either mapped at creation or have initial data");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    VkBufferUsageFlags vkUsageFlags = (VkBufferUsageFlags)usage;

    VkMemoryPropertyFlags memoryPropertyFlags = 0;

    if (mappedAtCreation)
    {
        memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    else
    {
        memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    VkBuffer vkBuffer;
    VkDeviceMemory vkDeviceMemory;
    tknCreateVkBuffer(pGfxContext, (VkDeviceSize)size, vkUsageFlags, memoryPropertyFlags, &vkBuffer, &vkDeviceMemory);

    TknBuffer *pBuffer = (TknBuffer *)tknMalloc(sizeof(TknBuffer));
    pBuffer->vkBuffer = vkBuffer;
    pBuffer->vkDeviceMemory = vkDeviceMemory;
    pBuffer->size = size;
    pBuffer->vkBufferUsageFlags = vkUsageFlags;
    pBuffer->vkMemoryPropertyFlags = memoryPropertyFlags;
    pBuffer->mappedAtCreation = mappedAtCreation;
    pBuffer->pMappedData = NULL;

    if (mappedAtCreation)
    {
        VkResult result = vkMapMemory(pGfxContext->vkDevice, vkDeviceMemory, 0, (VkDeviceSize)size, 0, &pBuffer->pMappedData);
        tknAssertVkResult(result);
    }
    else
    {
        pBuffer->pMappedData = NULL;
    }

    if (pData != NULL)
    {
        if (!mappedAtCreation)
        {
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingMemory;
            VkMemoryPropertyFlags stagingMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            tknCreateVkBuffer(pGfxContext, (VkDeviceSize)size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingMemFlags, &stagingBuffer, &stagingMemory);

            void *pStagingData = NULL;
            VkResult result = vkMapMemory(pGfxContext->vkDevice, stagingMemory, 0, (VkDeviceSize)size, 0, &pStagingData);
            tknAssertVkResult(result);
            memcpy(pStagingData, pData, size);
            vkUnmapMemory(pGfxContext->vkDevice, stagingMemory);

            VkCommandBuffer cmdBuffer = tknBeginSingleTimeCommands(pGfxContext);
            VkBufferCopy copyRegion = {.srcOffset = 0, .dstOffset = 0, .size = (VkDeviceSize)size};
            vkCmdCopyBuffer(cmdBuffer, stagingBuffer, vkBuffer, 1, &copyRegion);
            tknEndSingleTimeCommands(pGfxContext, cmdBuffer);

            vkDestroyBuffer(pGfxContext->vkDevice, stagingBuffer, NULL);
            vkFreeMemory(pGfxContext->vkDevice, stagingMemory, NULL);
        }
        else
        {
            memcpy(pBuffer->pMappedData, pData, size);
        }
    }

    return (void *)pBuffer;
}

void tknDestroyBufferPtr(void *pTknGfxContext, void *pTknBuffer)
{
    if (pTknBuffer == NULL)
    {
        tknWarning("Attempting to destroy NULL buffer");
        return;
    }

    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBuffer *pBuffer = (TknBuffer *)pTknBuffer;

    if (pBuffer->pMappedData != NULL)
    {
        vkUnmapMemory(pGfxContext->vkDevice, pBuffer->vkDeviceMemory);
        pBuffer->pMappedData = NULL;
    }

    if (pBuffer->vkBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(pGfxContext->vkDevice, pBuffer->vkBuffer, NULL);
        pBuffer->vkBuffer = VK_NULL_HANDLE;
    }

    if (pBuffer->vkDeviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(pGfxContext->vkDevice, pBuffer->vkDeviceMemory, NULL);
        pBuffer->vkDeviceMemory = VK_NULL_HANDLE;
    }

    tknFree(pBuffer);
}

void tknBindVertexBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");
    tknAssert(pTknBuffer != NULL, "Buffer cannot be NULL");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBuffer *pBuffer = (TknBuffer *)pTknBuffer;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    VkBuffer vertexBuffers[] = {pBuffer->vkBuffer};
    VkDeviceSize offsets[] = {(VkDeviceSize)offset};

    vkCmdBindVertexBuffers(vkCommandBuffer, TKN_VERTEX_BINDING_DESCRIPTION, 1, vertexBuffers, offsets);
}

void tknBindInstanceBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");
    tknAssert(pTknBuffer != NULL, "Buffer cannot be NULL");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBuffer *pBuffer = (TknBuffer *)pTknBuffer;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    VkBuffer instanceBuffers[] = {pBuffer->vkBuffer};
    VkDeviceSize offsets[] = {(VkDeviceSize)offset};

    vkCmdBindVertexBuffers(vkCommandBuffer, TKN_INSTANCE_BINDING_DESCRIPTION, 1, instanceBuffers, offsets);
}

void tknBindIndexBuffer(void *pTknGfxContext, void *pTknBuffer, int indexType, uint64_t offset)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");
    tknAssert(pTknBuffer != NULL, "Buffer cannot be NULL");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBuffer *pBuffer = (TknBuffer *)pTknBuffer;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    vkCmdBindIndexBuffer(vkCommandBuffer, pBuffer->vkBuffer, (VkDeviceSize)offset, (VkIndexType)indexType);
}

void tknUpdateBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset, uint64_t size, const void *pData)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");
    tknAssert(pTknBuffer != NULL, "Buffer cannot be NULL");
    tknAssert(pData != NULL, "Data cannot be NULL");
    tknAssert(size > 0, "Buffer update size must be greater than 0");
    tknAssert(offset + size <= ((TknBuffer *)pTknBuffer)->size, "Buffer update out of bounds");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBuffer *pBuffer = (TknBuffer *)pTknBuffer;

    if (pBuffer->vkMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        if (pBuffer->pMappedData != NULL)
        {
            memcpy((uint8_t *)pBuffer->pMappedData + offset, pData, size);
        }
        else
        {
            void *pMappedData = NULL;
            VkResult result = vkMapMemory(pGfxContext->vkDevice, pBuffer->vkDeviceMemory, (VkDeviceSize)offset, (VkDeviceSize)size, 0, &pMappedData);
            tknAssertVkResult(result);
            memcpy(pMappedData, pData, size);
            vkUnmapMemory(pGfxContext->vkDevice, pBuffer->vkDeviceMemory);
        }
    }
    else
    {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        VkMemoryPropertyFlags stagingMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        tknCreateVkBuffer(pGfxContext, (VkDeviceSize)size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingMemFlags, &stagingBuffer, &stagingMemory);

        void *pStagingData = NULL;
        VkResult result = vkMapMemory(pGfxContext->vkDevice, stagingMemory, 0, (VkDeviceSize)size, 0, &pStagingData);
        tknAssertVkResult(result);
        memcpy(pStagingData, pData, size);
        vkUnmapMemory(pGfxContext->vkDevice, stagingMemory);

        uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
        VkCommandBuffer cmdBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];
        VkBufferCopy copyRegion = {.srcOffset = 0, .dstOffset = (VkDeviceSize)offset, .size = (VkDeviceSize)size};
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, pBuffer->vkBuffer, 1, &copyRegion);

        vkDestroyBuffer(pGfxContext->vkDevice, stagingBuffer, NULL);
        vkFreeMemory(pGfxContext->vkDevice, stagingMemory, NULL);
    }
}
