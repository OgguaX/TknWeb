#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

extern void *tknMalloc(size_t size);
extern void tknFree(void *ptr);

extern void tknError(char const *const _Format, ...);
extern void tknWarning(const char *format, ...);
extern void tknAssert(bool condition, char const *const _Format, ...);

typedef enum
{
    TKN_VERTEX_BINDING_DESCRIPTION = 0,
    TKN_INSTANCE_BINDING_DESCRIPTION = 1,
} TknVertexBinding;

typedef struct TknVertexInputAttributeLayout
{
    uint32_t location;
    int format;
    uint32_t offset;
} TknVertexInputAttributeLayout;

typedef struct TknVertexInputLayout
{
    TknVertexBinding tknVertexBinding;
    uint32_t tknVertexInputAttributeDescriptionCount;
    TknVertexInputAttributeLayout *tknVertexInputAttributeDescriptions;
} TknVertexInputLayout;

// Unified vertex state - supports both Vulkan and WebGPU
typedef struct TknVertexState
{
    uint32_t meshAttributeCount;
    TknVertexInputAttributeLayout *pMeshAttributes;
    uint32_t instanceAttributeCount;
    TknVertexInputAttributeLayout *pInstanceAttributes;
} TknVertexState;

// Unified primitive state - combines topology and rasterization
typedef struct TknPrimitiveState
{
    int topology;                      // VkPrimitiveTopology / WebGPU topology
    int polygonMode;                   // VkPolygonMode equivalent (fill/line)
    int cullMode;                      // VkCullModeFlagBits equivalent
    int frontFace;                     // VkFrontFace equivalent
    bool depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
    float lineWidth;
} TknPrimitiveState;

typedef struct TknPipelineMultisampleState
{
    int rasterizationSamples; // Sample count
    bool alphaToCoverageEnable;
} TknPipelineMultisampleState;

typedef struct TknStencilOpState
{
    int failOp;      // VkStencilOp equivalent
    int passOp;      // VkStencilOp equivalent
    int depthFailOp; // VkStencilOp equivalent
    int compareOp;   // VkCompareOp equivalent
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;
} TknStencilOpState;

typedef struct TknPipelineDepthStencilState
{
    bool depthTestEnable;
    bool depthWriteEnable;
    int depthCompareOp; // VkCompareOp equivalent
    bool stencilTestEnable;
    TknStencilOpState front;
    TknStencilOpState back;
} TknPipelineDepthStencilState;

typedef struct TknPipelineColorBlendAttachmentState
{
    bool blendEnable;
    int srcColorBlendFactor; // VkBlendFactor equivalent
    int dstColorBlendFactor; // VkBlendFactor equivalent
    int colorBlendOp;        // VkBlendOp equivalent
    int srcAlphaBlendFactor; // VkBlendFactor equivalent
    int dstAlphaBlendFactor; // VkBlendFactor equivalent
    int alphaBlendOp;        // VkBlendOp equivalent
    int colorWriteMask;      // VkColorComponentFlagBits equivalent
} TknPipelineColorBlendAttachmentState;

typedef struct TknPipelineColorBlendState
{
    uint32_t attachmentCount;
    TknPipelineColorBlendAttachmentState *pAttachments;
    float blendConstants[4];
} TknPipelineColorBlendState;

// Fragment/color output state - combines formats and blending
typedef struct TknFragmentState
{
    uint32_t colorAttachmentCount;
    int *pColorAttachmentFormats;      // Format array
    TknPipelineColorBlendState colorBlend;
} TknFragmentState;

typedef struct TknPipelineDynamicState
{
    uint32_t dynamicStateCount;
    int *pDynamicStates; // VkDynamicState equivalent
} TknPipelineDynamicState;

extern void *tknCreateGfxContextPtr(uint32_t extensionCount, const char **extensions, void *pSurface, uint32_t width, uint32_t height, uint32_t globalShaderPathCount, const char **globalShaderPaths);
extern void tknDestroyGfxContextPtr(void *pTknGfxContext);

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

extern void *tknCreatePipelinePtr(void *pTknGfxContext, void *pTknRenderPassBindingGroupLayout, uint32_t spvPathCount, const char **spvPaths, const TknVertexState *pVertexState, const TknPrimitiveState *pPrimitiveState, const TknFragmentState *pFragmentState, const TknPipelineMultisampleState *pMultisampleState, const TknPipelineDepthStencilState *pDepthStencilState, int depthAttachmentFormat);
extern void tknDestroyPipelinePtr(void *pTknGfxContext, void *pTknPipeline);

extern void tknBeginCommandBuffer(void *pTknGfxContext);
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
