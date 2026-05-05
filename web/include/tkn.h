#pragma once
#include <stdint.h>

extern void *tknCreateGfxContextPtr(
    void *pInstance,
    void *pSurface,
    uint32_t width,
    uint32_t height,
    uint32_t globalShaderPathCount,
    const char **globalShaderPaths);

extern void tknDestroyGfxContext(void *pTknGfxContext);
extern void tknRenderFrame(void *pTknGfxContext);


// TknRenderPass *tknCreateRenderPassPtr(TknGfxContext *pTknGfxContext, uint32_t tknAttachmentCount, VkAttachmentDescription *vkAttachmentDescriptions, TknAttachment **inputAttachmentPtrs, VkClearValue *vkClearValues, uint32_t tknSubpassCount, VkSubpassDescription *vkSubpassDescriptions, uint32_t *spvPathCounts, const char ***spvPathsArray, uint32_t vkSubpassDependencyCount, VkSubpassDependency *vkSubpassDependencies, uint32_t renderPassIndex);
extern void *tknCreateRenderPass(void *pTknGfxContext, uint32_t tknAttachmentCount, void *vkAttachmentDescriptions, void **inputAttachmentPtrs, void *vkClearValues, uint32_t tknSubpassCount, void *vkSubpassDescriptions, uint32_t *spvPathCounts, const char ***spvPathsArray, uint32_t vkSubpassDependencyCount, void *vkSubpassDependencies, uint32_t renderPassIndex);
extern void tknDestroyRenderPass(void *pTknRenderPass);
