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

extern void *tknCreateBindingGroupLayout(void *pTknGfxContext, uint32_t shaderPathCount, const char **shaderPaths, uint32_t set);
extern void tknDestroyBindingGroupLayout(void *pTknGfxContext, void *pTknBindingGroupLayout);

extern void *tknCreateBindingGroup(void *pTknGfxContext, void *pTknBindingGroupLayout, uint32_t resourceCount, void **resourcePtrs);
extern void tknDestroyBindingGroup(void *pTknGfxContext, void *pTknBindingGroup);
extern void tknUpdateBindingGroup(void *pTknGfxContext, void *pTknBindingGroup, uint32_t resourceCount, uint32_t *indices, void **resourcePtrs);

extern void *tknCreatePipelinePtr(void *pTknGfxContext, uint32_t colorAttachmentCount, const int *pColorAttachmentFormats, int depthAttachmentFormat, void *pTknRenderPassBindingGroupLayout, uint32_t spvPathCount, const char **spvPaths, void *pTknMeshVertexInputLayout, void *pTknInstanceVertexInputLayout, void *pVkPipelineInputAssemblyStateCreateInfo, void *pVkPipelineViewportStateCreateInfo, void *pVkPipelineRasterizationStateCreateInfo, void *pVkPipelineMultisampleStateCreateInfo, void *pVkPipelineDepthStencilStateCreateInfo, void *pVkPipelineColorBlendStateCreateInfo, void *pVkPipelineDynamicStateCreateInfo);
extern void tknDestroyPipelinePtr(void *pTknGfxContext, void *pTknPipeline);

extern void tknBeginCommandBuffer(void *pTknGfxContext);
extern void tknEndCommandBuffer(void *pTknGfxContext);

extern void tknBeginRenderPass(void *pTknGfxContext, uint32_t colorAttachmentCount, void **colorImageViewPtrs, const int *loadOps, const int *storeOps, const double (*colorClearValues)[4], void *pTknDepthImageView, int depthLoadOp, int depthStoreOp, float depthClearValue, uint32_t stencilClearValue, uint32_t width, uint32_t height);
extern void tknEndRenderPass(void *pTknGfxContext);

extern void tknSetPipelinePtr(void *pTknGfxContext, void *pTknPipeline, void *pTknRenderPassBindingGroup, void *pTknPipelineBindingGroup);

extern void *tknCreateBufferPtr(void *pTknGfxContext, uint64_t size, int usage, bool mappedAtCreation, const void *pData);
extern void tknDestroyBufferPtr(void *pTknGfxContext, void *pTknBuffer);
extern void tknUpdateBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset, uint64_t size, const void *pData);

extern void tknBindVertexBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset);
extern void tknBindInstanceBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset);
extern void tknBindIndexBuffer(void *pTknGfxContext, void *pTknBuffer, int indexType, uint64_t offset);

extern void tknDraw(void *pTknGfxContext, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
extern void tknDrawIndexed(void *pTknGfxContext, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
