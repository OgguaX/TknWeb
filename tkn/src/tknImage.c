#include "tknGfx.h"
#include "tknCore.h"

// 创建 VkImage
void *tknCreateImagePtr(void *pTknGfxContext, int dimension, int format, int mipLevelCount, 
                        int sampleCount, int width, int height, int depth, int imageUsageFlags)
{
    if (pTknGfxContext == NULL)
    {
        tknError("TknGfxContext pointer is NULL");
        return NULL;
    }

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;

    // 创建 TknImage 结构体
    TknImage *pTknImage = (TknImage *)tknMalloc(sizeof(TknImage));
    if (pTknImage == NULL)
    {
        tknError("Failed to allocate memory for TknImage");
        return NULL;
    }

    // 准备图像创建信息
    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = tknImageDimensionToVkImageType((TknImageDimension)dimension),
        .format = tknImageFormatToVkFormat((TknImageFormat)format),
        .extent = {
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .depth = (uint32_t)(depth > 0 ? depth : 1),
        },
        .mipLevels = (uint32_t)(mipLevelCount > 0 ? mipLevelCount : 1),
        .arrayLayers = 1,
        .samples = (VkSampleCountFlagBits)(sampleCount > 0 ? sampleCount : VK_SAMPLE_COUNT_1_BIT),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = (VkImageUsageFlags)imageUsageFlags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    // 创建 VkImage
    VkResult result = vkCreateImage(pGfxContext->vkDevice, &imageCreateInfo, NULL, &pTknImage->vkImage);
    if (result != VK_SUCCESS)
    {
        tknError("vkCreateImage failed with result: %d", result);
        tknFree(pTknImage);
        return NULL;
    }

    // 获取图像内存需求
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(pGfxContext->vkDevice, pTknImage->vkImage, &memoryRequirements);

    // 查找合适的内存类型
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

    if (memoryTypeIndex == UINT32_MAX)
    {
        tknError("Failed to find suitable memory type for VkImage");
        vkDestroyImage(pGfxContext->vkDevice, pTknImage->vkImage, NULL);
        tknFree(pTknImage);
        return NULL;
    }

    // 分配内存
    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };

    result = vkAllocateMemory(pGfxContext->vkDevice, &memoryAllocateInfo, NULL, &pTknImage->vkDeviceMemory);
    if (result != VK_SUCCESS)
    {
        tknError("vkAllocateMemory failed with result: %d", result);
        vkDestroyImage(pGfxContext->vkDevice, pTknImage->vkImage, NULL);
        tknFree(pTknImage);
        return NULL;
    }

    // 绑定内存到图像
    result = vkBindImageMemory(pGfxContext->vkDevice, pTknImage->vkImage, pTknImage->vkDeviceMemory, 0);
    if (result != VK_SUCCESS)
    {
        tknError("vkBindImageMemory failed with result: %d", result);
        vkFreeMemory(pGfxContext->vkDevice, pTknImage->vkDeviceMemory, NULL);
        vkDestroyImage(pGfxContext->vkDevice, pTknImage->vkImage, NULL);
        tknFree(pTknImage);
        return NULL;
    }

    // 初始化 ImageView HashSet
    pTknImage->tknImageViewPtrHashSet = tknCreateHashSet(sizeof(TknImageView *));

    return pTknImage;
}

// 销毁 VkImage
void tknDestroyImagePtr(void *pTknGfxContext, void *pTknImage)
{
    if (pTknGfxContext == NULL || pTknImage == NULL)
    {
        return;
    }

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknImage *pImage = (TknImage *)pTknImage;

    // 销毁所有关联的 ImageView
    if (pImage->tknImageViewPtrHashSet.capacity > 0)
    {
        tknDestroyHashSet(pImage->tknImageViewPtrHashSet);
    }

    // 销毁 VkImage 和内存
    if (pImage->vkImage != NULL)
    {
        vkDestroyImage(pGfxContext->vkDevice, pImage->vkImage, NULL);
    }

    if (pImage->vkDeviceMemory != NULL)
    {
        vkFreeMemory(pGfxContext->vkDevice, pImage->vkDeviceMemory, NULL);
    }

    // 释放 TknImage 结构体
    tknFree(pImage);
}
