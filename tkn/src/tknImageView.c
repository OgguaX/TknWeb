#include "tknGfx.h"
#include "tknCore.h"

// 创建 VkImageView
void *tknCreateImageView(void *pTknGfxContext, int baseLayer, int layerCount, int aspectFlags,
                         int baseMipLevel, int mipLevelCount, int dimension, int format,
                         int imageUsageFlags, void *pTknImage)
{
    if (pTknGfxContext == NULL || pTknImage == NULL)
    {
        tknError("TknGfxContext or TknImage pointer is NULL");
        return NULL;
    }

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknImage *pImage = (TknImage *)pTknImage;

    // 创建 TknImageView 结构体
    TknImageView *pTknImageView = (TknImageView *)tknMalloc(sizeof(TknImageView));
    if (pTknImageView == NULL)
    {
        tknError("Failed to allocate memory for TknImageView");
        return NULL;
    }

    // 默认值处理
    int finalLayerCount = (layerCount <= 0) ? 1 : layerCount;
    int finalMipLevelCount = (mipLevelCount <= 0) ? 1 : mipLevelCount;

    // 准备图像视图创建信息
    VkImageViewCreateInfo viewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = pImage->vkImage,
        .viewType = tknImageViewDimensionToVkImageViewType((TknImageViewDimension)dimension),
        .format = tknImageFormatToVkFormat((TknImageFormat)format),
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = {
            .aspectMask = tknImageAspectToVkImageAspectFlags(aspectFlags),
            .baseMipLevel = (uint32_t)(baseMipLevel >= 0 ? baseMipLevel : 0),
            .levelCount = (uint32_t)finalMipLevelCount,
            .baseArrayLayer = (uint32_t)(baseLayer >= 0 ? baseLayer : 0),
            .layerCount = (uint32_t)finalLayerCount,
        },
    };

    // 创建 VkImageView
    VkResult result = vkCreateImageView(pGfxContext->vkDevice, &viewCreateInfo, NULL, &pTknImageView->vkImageView);
    if (result != VK_SUCCESS)
    {
        tknError("vkCreateImageView failed with result: %d", result);
        tknFree(pTknImageView);
        return NULL;
    }

    // 初始化 BindGroup HashSet
    pTknImageView->TknBindGroupPtrHashSet = tknCreateHashSet(sizeof(void *));

    // 将 ImageView 指针添加到 Image 的 HashSet 中
    if (!tknAddToHashSet(&pImage->tknImageViewPtrHashSet, &pTknImageView))
    {
        tknWarning("Failed to add ImageView to Image's HashSet");
    }

    return pTknImageView;
}

// 销毁 VkImageView
void tknDestroyImageView(void *pTknGfxContext, void *pTknImageView)
{
    if (pTknGfxContext == NULL || pTknImageView == NULL)
    {
        return;
    }

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknImageView *pImageView = (TknImageView *)pTknImageView;

    // 销毁所有关联的 BindGroup（如果实现了 BindGroup 销毁）
    if (pImageView->TknBindGroupPtrHashSet.capacity > 0)
    {
        tknDestroyHashSet(pImageView->TknBindGroupPtrHashSet);
    }

    // 销毁 VkImageView
    if (pImageView->vkImageView != NULL)
    {
        vkDestroyImageView(pGfxContext->vkDevice, pImageView->vkImageView, NULL);
    }

    // 释放 TknImageView 结构体
    tknFree(pImageView);
}
