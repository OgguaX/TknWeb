#include "tknGfx.h"
#include "tknGfxInternal.h"

void *tknCreateSampler(void *pTknGfxContext, int magFilter, int minFilter, int mipmapMode,
                       int addressModeU, int addressModeV, int addressModeW, float mipLodBias,
                       bool anisotropyEnable, float maxAnisotropy, bool compareEnable, int compareOp,
                       float minLod, float maxLod, int borderColor, bool unnormalizedCoordinates)
{
    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;

    VkBool32 vkAnisotropyEnable = anisotropyEnable ? VK_TRUE : VK_FALSE;
    VkBool32 vkCompareEnable = compareEnable ? VK_TRUE : VK_FALSE;
    VkBool32 vkUnnormalizedCoordinates = unnormalizedCoordinates ? VK_TRUE : VK_FALSE;

    float clampedMaxAnisotropy = maxAnisotropy;
    if (vkAnisotropyEnable)
    {
        float deviceMaxAnisotropy = pGfxContext->vkPhysicalDeviceProperties.limits.maxSamplerAnisotropy;
        tknAssert(deviceMaxAnisotropy > 0.0f, "Device does not support anisotropy");
        if (clampedMaxAnisotropy < 1.0f)
        {
            clampedMaxAnisotropy = 1.0f;
        }
        if (clampedMaxAnisotropy > deviceMaxAnisotropy)
        {
            clampedMaxAnisotropy = deviceMaxAnisotropy;
        }
    }
    else
    {
        clampedMaxAnisotropy = 1.0f;
    }

    VkSamplerCreateInfo samplerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .magFilter = (VkFilter)magFilter,
        .minFilter = (VkFilter)minFilter,
        .mipmapMode = (VkSamplerMipmapMode)mipmapMode,
        .addressModeU = (VkSamplerAddressMode)addressModeU,
        .addressModeV = (VkSamplerAddressMode)addressModeV,
        .addressModeW = (VkSamplerAddressMode)addressModeW,
        .mipLodBias = mipLodBias,
        .anisotropyEnable = vkAnisotropyEnable,
        .maxAnisotropy = clampedMaxAnisotropy,
        .compareEnable = vkCompareEnable,
        .compareOp = (VkCompareOp)compareOp,
        .minLod = minLod,
        .maxLod = maxLod,
        .borderColor = (VkBorderColor)borderColor,
        .unnormalizedCoordinates = vkUnnormalizedCoordinates,
    };

    TknSampler *pTknSampler = (TknSampler *)tknMalloc(sizeof(TknSampler));
    tknAssertVkResult(vkCreateSampler(pGfxContext->vkDevice, &samplerCreateInfo, NULL, &pTknSampler->vkSampler));
    pTknSampler->tknBindingGroupPtrHashSet = tknCreateHashSet(sizeof(void *));

    return (void *)pTknSampler;
}

void tknDestroySampler(void *pTknGfxContext, void *pTknSampler)
{
    if (pTknSampler == NULL)
    {
        tknWarning("Attempting to destroy NULL sampler");
        return;
    }

    tknAssert(pTknGfxContext != NULL, "Graphics context cannot be NULL");

    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknSampler *pSampler = (TknSampler *)pTknSampler;

    tknAssert(pSampler->tknBindingGroupPtrHashSet.count == 0, "Cannot destroy sampler while it is still referenced by binding groups");
    tknDestroyHashSet(pSampler->tknBindingGroupPtrHashSet);
    vkDestroySampler(pGfxContext->vkDevice, pSampler->vkSampler, NULL);
    pSampler->vkSampler = VK_NULL_HANDLE;

    tknFree(pSampler);
}
