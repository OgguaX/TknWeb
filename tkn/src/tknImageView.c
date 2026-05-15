#include "tknGfx.h"
#include "tknCore.h"

void *tknCreateImageView(void *pTknGfxContext, int baseLayer, int layerCount, int aspectFlags,
                         int baseMipLevel, int mipLevelCount, int dimension, int format,
                         void *pTknImage)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknImage *pImage = (TknImage *)pTknImage;

    VkImageSubresourceRange subresourceRange = {
        .aspectMask = (VkImageAspectFlags)aspectFlags,
        .baseMipLevel = (uint32_t)baseMipLevel,
        .levelCount = (uint32_t)mipLevelCount,
        .baseArrayLayer = (uint32_t)baseLayer,
        .layerCount = (uint32_t)layerCount,
    };

    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = pImage->vkImage,
        .viewType = (VkImageViewType)dimension,
        .format = (VkFormat)format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },
        .subresourceRange = subresourceRange,
    };

    TknImageView *pTknImageView = (TknImageView *)tknMalloc(sizeof(TknImageView));
    tknAssertVkResult(vkCreateImageView(pGfxContext->vkDevice, &imageViewCreateInfo, NULL, &pTknImageView->vkImageView));
    pTknImageView->pTknImage = pImage;
    pTknImageView->TknBindGroupPtrHashSet = tknCreateHashSet(sizeof(void *));

    // Add image view pointer to the parent image's hash set
    void *pImageViewPtr = pTknImageView;
    tknAddToHashSet(&pImage->tknImageViewPtrHashSet, &pImageViewPtr);

    return (void *)pTknImageView;
}

void tknDestroyImageView(void *pTknGfxContext, void *pTknImageView)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknImageView *pImageView = (TknImageView *)pTknImageView;

    // Remove from parent image's hash set
    void *pImageViewPtr = pImageView;
    tknRemoveFromHashSet(&pImageView->pTknImage->tknImageViewPtrHashSet, &pImageViewPtr);

    tknDestroyHashSet(pImageView->TknBindGroupPtrHashSet);
    vkDestroyImageView(pGfxContext->vkDevice, pImageView->vkImageView, NULL);
    pImageView->vkImageView = VK_NULL_HANDLE;
    pImageView->pTknImage = NULL;

    tknFree(pImageView);
}