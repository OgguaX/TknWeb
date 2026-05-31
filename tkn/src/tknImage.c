#include "tknGfx.h"
#include "tknCore.h"

void *tknCreateImagePtr(void *pTknGfxContext, int dimension, int format, uint32_t mipLevelCount,
                        int sampleCount, uint32_t width, uint32_t height, uint32_t depth, int imageUsageFlags)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = (VkImageType)dimension,
        .format = (VkFormat)format,
        .extent = {
            .width = width,
            .height = height,
            .depth = depth,
        },
        .mipLevels = mipLevelCount,
        .arrayLayers = 1,
        .samples = (VkSampleCountFlagBits)sampleCount,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = (VkImageUsageFlags)imageUsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    TknImage *pTknImage = (TknImage *)tknMalloc(sizeof(TknImage));
    tknAssertVkResult(vkCreateImage(pGfxContext->vkDevice, &imageCreateInfo, NULL, &pTknImage->vkImage));
    pTknImage->vkFormat = (VkFormat)format;
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(pGfxContext->vkDevice, pTknImage->vkImage, &memoryRequirements);
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(pGfxContext->vkPhysicalDevice, &memoryProperties);
    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            memoryTypeIndex = i;
            break;
        }
    }

    tknAssert(memoryTypeIndex != UINT32_MAX, "Failed to find suitable memory type for image");
    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };

    tknAssertVkResult(vkAllocateMemory(pGfxContext->vkDevice, &allocateInfo, NULL, &pTknImage->vkDeviceMemory));
    tknAssertVkResult(vkBindImageMemory(pGfxContext->vkDevice, pTknImage->vkImage, pTknImage->vkDeviceMemory, 0));
    pTknImage->tknImageViewPtrHashSet = tknCreateHashSet(sizeof(void *));
    return (void *)pTknImage;
}

void tknDestroyImagePtr(void *pTknGfxContext, void *pTknImage)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknImage *pImage = (TknImage *)pTknImage;
    vkDestroyImage(pGfxContext->vkDevice, pImage->vkImage, NULL);
    pImage->vkImage = VK_NULL_HANDLE;

    vkFreeMemory(pGfxContext->vkDevice, pImage->vkDeviceMemory, NULL);
    pImage->vkDeviceMemory = VK_NULL_HANDLE;

    tknDestroyHashSet(pImage->tknImageViewPtrHashSet);
    tknFree(pImage);
}

void tknUpdateImagePtr(TknGfxContext *pTknGfxContext, TknImage *pTknImage, void *imageData, int width, int height, int depth, int bytesPerPixel, int mipLevel, int originX, int originY, int originZ, int aspectMask)
{
    // Calculate total staging buffer size
    VkDeviceSize dataSize = (VkDeviceSize)width * height * depth * bytesPerPixel;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    tknCreateVkBuffer(pTknGfxContext, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingBuffer, &stagingBufferMemory);

    // Copy image data to staging buffer
    void *mappedData;
    vkMapMemory(pTknGfxContext->vkDevice, stagingBufferMemory, 0, dataSize, 0, &mappedData);
    memcpy(mappedData, imageData, (size_t)dataSize);
    vkUnmapMemory(pTknGfxContext->vkDevice, stagingBufferMemory);

    // Begin command buffer - all operations in one submission
    VkCommandBuffer commandBuffer = tknBeginSingleTimeCommands(pTknGfxContext);

    // Transition image layout for transfer (SHADER_READ_ONLY -> TRANSFER_DST)
    VkImageMemoryBarrier barrier1 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = pTknImage->vkImage,
        .subresourceRange = {
            .aspectMask = (VkImageAspectFlags)aspectMask,
            .baseMipLevel = (uint32_t)mipLevel,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
    };

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier1);

    // Build copy region
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = (VkImageAspectFlags)aspectMask,
            .mipLevel = (uint32_t)mipLevel,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {
            .x = (int32_t)originX,
            .y = (int32_t)originY,
            .z = (int32_t)originZ,
        },
        .imageExtent = {
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .depth = (uint32_t)depth,
        },
    };

    // Copy buffer to image
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, pTknImage->vkImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout back to shader access (TRANSFER_DST -> SHADER_READ_ONLY)
    VkImageMemoryBarrier barrier2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = pTknImage->vkImage,
        .subresourceRange = {
            .aspectMask = (VkImageAspectFlags)aspectMask,
            .baseMipLevel = (uint32_t)mipLevel,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    };

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier2);

    // Submit all commands
    tknEndSingleTimeCommands(pTknGfxContext, commandBuffer);

    // Clean up staging buffer
    tknDestroyVkBuffer(pTknGfxContext, stagingBuffer, stagingBufferMemory);
}