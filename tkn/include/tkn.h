#pragma once
#include <stdbool.h>
#include <stdint.h>

extern void *tknCreateGfxContextPtr(uint32_t extensionCount, const char **extensions, void *pSurface, uint32_t width, uint32_t height, uint32_t globalShaderPathCount, const char **globalShaderPaths);
extern void tknDestroyGfxContextPtr(void *pTknGfxContext);
extern void tknRecordGfxContextPtr(void *pTknGfxContext);

extern void *tknCreateImagePtr(void *pTknGfxContext, int dimension, int format, uint32_t mipLevelCount, int sampleCount, uint32_t width, uint32_t height, uint32_t depth, int imageUsageFlags);
extern void tknDestroyImagePtr(void *pTknGfxContext, void *pTknImage);

extern void *tknCreateImageView(void *pTknGfxContext, uint32_t baseLayer, uint32_t layerCount, int aspectFlags, uint32_t baseMipLevel, uint32_t mipLevelCount, int dimension, int format, void *pTknImage);
extern void tknDestroyImageView(void *pTknGfxContext, void *pTknImageView);

extern void *tknCreateBindingGroup(void *pTknGfxContext, uint32_t shaderPathCount, const char **shaderPaths, uint32_t set, uint32_t resourceCount, void **resourcePtrs);
extern void tknDestroyBindingGroup(void *pTknGfxContext, void *pTknBindingGroup);

extern void tknBeginRenderPass(void *pTknGfxContext, void *pVkCommandBuffer, uint32_t colorAttachmentCount, void **colorImageViewPtrs, const int *loadOps, const int *storeOps, const double (*colorClearValues)[4], void *pTknDepthImageView, int depthLoadOp, int depthStoreOp, float depthClearValue, uint32_t stencilClearValue, uint32_t width, uint32_t height);
extern void tknEndRenderPass(void *pTknGfxContext, void *pVkCommandBuffer);

extern void *tknCreatePipelinePtr(void *pTknGfxContext, uint32_t colorAttachmentCount, const int *pColorAttachmentFormats, int depthAttachmentFormat, void *pTknRenderPassBindingGroup, uint32_t spvPathCount, const char **spvPaths, void *pTknMeshVertexInputLayout, void *pTknInstanceVertexInputLayout, void *pVkPipelineInputAssemblyStateCreateInfo, void *pVkPipelineViewportStateCreateInfo, void *pVkPipelineRasterizationStateCreateInfo, void *pVkPipelineMultisampleStateCreateInfo, void *pVkPipelineDepthStencilStateCreateInfo, void *pVkPipelineColorBlendStateCreateInfo, void *pVkPipelineDynamicStateCreateInfo);
extern void tknDestroyPipelinePtr(void *pTknGfxContext, void *pTknPipeline);
