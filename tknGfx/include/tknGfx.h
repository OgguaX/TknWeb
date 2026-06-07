#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

extern void *tknMalloc(size_t size);
extern void tknFree(void *ptr);

extern void tknError(char const *const _Format, ...);
extern void tknWarning(const char *format, ...);
extern void tknAssert(bool condition, char const *const _Format, ...);

typedef struct TknStencilOpState
{
    int failOp;
    int passOp;
    int depthFailOp;
    int compareOp;
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;
} TknStencilOpState;

typedef struct TknPipelineColorBlendAttachmentState
{
    bool blendEnable;
    int srcColorBlendFactor;
    int dstColorBlendFactor;
    int colorBlendOp;
    int srcAlphaBlendFactor;
    int dstAlphaBlendFactor;
    int alphaBlendOp;
    int colorWriteMask;
} TknPipelineColorBlendAttachmentState;

extern void *tknCreateGfxContextPtr(void *pInstance, void *pSurface, uint32_t width, uint32_t height, uint32_t globalShaderPathCount, const char **globalShaderPaths);
extern void tknDestroyGfxContextPtr(void *pTknGfxContext);
extern void tknUpdateSwapchainPtr(void *pTknGfxContext, uint32_t width, uint32_t height);

extern void *tknCreateImagePtr(void *pTknGfxContext, int dimension, int format, uint32_t mipLevelCount, int sampleCount, uint32_t width, uint32_t height, uint32_t depth, int imageUsageFlags);
extern void tknDestroyImagePtr(void *pTknGfxContext, void *pTknImage);
extern void tknWriteImagePtr(void *pTknGfxContext, void *pTknImage, const void *pData, uint64_t dataSize, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ);

extern void *tknCreateImageView(void *pTknGfxContext, uint32_t baseLayer, uint32_t layerCount, int aspectFlags, uint32_t baseMipLevel, uint32_t mipLevelCount, int dimension, int format, void *pTknImage);
extern void tknDestroyImageView(void *pTknGfxContext, void *pTknImageView);

extern void *tknCreateUniformBuffer(void *pTknBuffer, uint64_t offset, uint64_t range);
extern void tknDestroyUniformBuffer(void *pTknUniformBuffer);

extern void *tknCreateSampler(void *pTknGfxContext, int magFilter, int minFilter, int mipmapMode, int addressModeU, int addressModeV, int addressModeW, float mipLodBias, bool anisotropyEnable, float maxAnisotropy, bool compareEnable, int compareOp, float minLod, float maxLod, int borderColor, bool unnormalizedCoordinates);
extern void tknDestroySampler(void *pTknGfxContext, void *pTknSampler);

extern void *tknCreateBindingGroupLayout(void *pTknGfxContext, uint32_t shaderPathCount, const char **shaderPaths, uint32_t set);
extern void tknDestroyBindingGroupLayout(void *pTknGfxContext, void *pTknBindingGroupLayout);

extern void *tknCreateBindingGroup(void *pTknGfxContext, void *pTknBindingGroupLayout, uint32_t resourceCount, void **resourcePtrs);
extern void tknDestroyBindingGroup(void *pTknGfxContext, void *pTknBindingGroup);
extern void tknUpdateBindingGroup(void *pTknGfxContext, void *pTknBindingGroup, uint32_t resourceCount, uint32_t *indices, void **resourcePtrs);

extern void *tknCreatePipelinePtr(void *pTknGfxContext, void *pTknRenderPassBindingGroupLayout, uint32_t spvPathCount, const char **spvPaths, void *pTknMeshVertexInputLayoutPtr, void *pTknInstanceVertexInputLayoutPtr, int topology, int polygonMode, int cullMode, int frontFace, bool depthBiasEnable, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor, float lineWidth, uint32_t colorAttachmentCount, int *pColorAttachmentFormats, uint32_t attachmentCount, TknPipelineColorBlendAttachmentState *attachments, float blendConstants[4], int rasterizationSamples, bool alphaToCoverageEnable, bool depthTestEnable, bool depthWriteEnable, int depthCompareOp, bool stencilTestEnable, TknStencilOpState front, TknStencilOpState back, int depthAttachmentFormat);
extern void tknDestroyPipelinePtr(void *pTknGfxContext, void *pTknPipeline);

extern void tknBeginCommandBuffer(void *pTknGfxContext, uint32_t width, uint32_t height);
extern void tknEndCommandBuffer(void *pTknGfxContext);

extern void tknBeginRenderPass(void *pTknGfxContext, uint32_t colorAttachmentCount, void **colorImageViewPtrs, const int *loadOps, const int *storeOps, const double (*colorClearValues)[4], void *pTknDepthImageView, int depthLoadOp, int depthStoreOp, float depthClearValue, uint32_t stencilClearValue, uint32_t width, uint32_t height);
extern void tknEndRenderPass(void *pTknGfxContext);

// Platform-agnostic viewport and scissor commands
extern void tknSetViewport(void *pTknGfxContext, float x, float y, float width, float height, float minDepth, float maxDepth);
extern void tknSetScissor(void *pTknGfxContext, int32_t x, int32_t y, uint32_t width, uint32_t height);

extern void tknSetPipelinePtr(void *pTknGfxContext, void *pTknPipeline, void *pTknRenderPassBindingGroup, void *pTknPipelineBindingGroup);

extern void *tknCreateBufferPtr(void *pTknGfxContext, uint64_t size, int usage, bool mappedAtCreation, const void *pData);
extern void tknDestroyBufferPtr(void *pTknGfxContext, void *pTknBuffer);
extern void tknUpdateBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset, uint64_t size, const void *pData);

extern void tknBindVertexBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset);
extern void tknBindInstanceBuffer(void *pTknGfxContext, void *pTknBuffer, uint64_t offset);
extern void tknBindIndexBuffer(void *pTknGfxContext, void *pTknBuffer, int indexType, uint64_t offset);

extern void tknDraw(void *pTknGfxContext, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
extern void tknDrawIndexed(void *pTknGfxContext, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t baseVertex, uint32_t firstInstance);
