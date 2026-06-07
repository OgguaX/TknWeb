#include <vulkan/vulkan.h>
#include "tknCore.h"
#include "tknGfx.h"
#include <spirv_reflect.h>
#define TKN_ARRAY_COUNT(array) (NULL == array) ? 0 : (sizeof(array) / sizeof(array[0]))
typedef enum
{
    TKN_GLOBAL_DESCRIPTOR_SET,
    TKN_RENDERPASS_DESCRIPTOR_SET,
    TKN_PIPELINE_DESCRIPTOR_SET,
    TKN_MAX_DESCRIPTOR_SET,
} TknTickernelDescriptorSet;

#define TKN_MAX_VERTEX_BINDING_DESCRIPTION 2

typedef struct TknBuffer
{
    VkBuffer vkBuffer;
    VkDeviceMemory vkDeviceMemory;
    uint64_t size;
    VkBufferUsageFlags vkBufferUsageFlags;
    VkMemoryPropertyFlags vkMemoryPropertyFlags;
    bool mappedAtCreation;
    void *pMappedData;
} TknBuffer;

typedef struct TknUniformBuffer
{
    TknBuffer *pTknBuffer;
    VkDeviceSize offset;
    VkDeviceSize range;
    TknHashSet tknBindingGroupPtrHashSet;
} TknUniformBuffer;

typedef struct TknSampler
{
    VkSampler vkSampler;
    TknHashSet tknBindingGroupPtrHashSet;
} TknSampler;

typedef struct TknImage
{
    VkImage vkImage;
    VkFormat vkFormat;
    VkDeviceMemory vkDeviceMemory;
    TknHashSet tknImageViewPtrHashSet;
    VkExtent3D tknSwapchainExtent;
} TknImage;
typedef struct TknImageView
{
    TknImage *pTknImage;
    VkImageView vkImageView;
    TknHashSet tknBindingGroupPtrHashSet;
} TknImageView;

typedef struct TknBindingGroupLayout
{
    uint32_t bindingCount;
    uint32_t usedBindingCount;
    VkDescriptorType *vkDescriptorTypes;
    uint32_t *vkDescriptorCounts;
    VkShaderStageFlags *vkShaderStageFlags;
    bool *tknBindingUsed;

    uint32_t shaderPathCount;
    const char **shaderPaths;
    SpvReflectShaderModule *pSpvReflectShaderModules;

    VkDescriptorSetLayout vkDescriptorSetLayout;
} TknBindingGroupLayout;

typedef struct TknBindingGroup
{
    TknBindingGroupLayout *pLayout;
    VkDescriptorPool vkDescriptorPool;
    VkDescriptorSet vkDescriptorSet;
    uint32_t tknBindingResourceCount;
    void **tknBindingResourcePtrs;
} TknBindingGroup;

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

typedef struct TknPipeline
{
    VkPipeline vkPipeline;
    VkPipelineLayout vkPipelineLayout;
    TknBindingGroupLayout *pTknPipelineBindingGroupLayout;
    TknVertexInputLayout *tknVertexInputLayoutPtrs[TKN_MAX_VERTEX_BINDING_DESCRIPTION];
} TknPipeline;

typedef struct TknGfxContext
{
    VkInstance vkInstance;
    VkSurfaceKHR vkSurface;
    VkSurfaceFormatKHR vkSurfaceFormat;
    VkPresentModeKHR vkPresentMode;
    VkPhysicalDevice vkPhysicalDevice;

    uint32_t tknGfxQueueFamilyIndex;
    uint32_t tknPresentQueueFamilyIndex;
    VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
    VkDevice vkDevice;
    VkQueue vkGfxQueue;
    VkQueue vkPresentQueue;

    VkSurfaceCapabilitiesKHR vkSurfaceCapabilities;
    uint32_t swapchainImageCount;
    TknImage **tknSwapchainImagePtrs;
    TknImageView **tknSwapchainImageViewPtrs;
    VkSwapchainKHR vkSwapchain;

    VkSemaphore vkImageAvailableSemaphore;
    VkSemaphore vkRenderFinishedSemaphore;
    VkFence vkRenderFinishedFence;

    VkCommandPool vkGfxCommandPool;
    VkCommandBuffer *vkGfxCommandBuffers;

    TknBindingGroupLayout *pTknGlobalBindingGroupLayout;
    TknBindingGroup *pTknGlobalBindingGroup;

    uint32_t frameCount;
} TknGfxContext;

void tknAssertVkResult(VkResult vkResult);
void tknCreateVkBuffer(TknGfxContext *pTknGfxContext, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer *pVkBuffer, VkDeviceMemory *pVkDeviceMemory);
void tknDestroyVkBuffer(TknGfxContext *pTknGfxContext, VkBuffer vkBuffer, VkDeviceMemory vkDeviceMemory);
VkCommandBuffer tknBeginSingleTimeCommands(TknGfxContext *pTknGfxContext);
void tknEndSingleTimeCommands(TknGfxContext *pTknGfxContext, VkCommandBuffer vkCommandBuffer);

SpvReflectShaderModule tknCreateSpvReflectShaderModule(const char *filePath);
void tknDestroySpvReflectShaderModule(SpvReflectShaderModule *pSpvReflectShaderModule);

void *tknCreateUniformBuffer(void *pTknBuffer, uint64_t offset, uint64_t range);
void tknDestroyUniformBuffer(void *pTknUniformBuffer);

void *tknCreateSampler(void *pTknGfxContext, int magFilter, int minFilter, int mipmapMode,
                       int addressModeU, int addressModeV, int addressModeW, float mipLodBias,
                       bool anisotropyEnable, float maxAnisotropy, bool compareEnable, int compareOp,
                       float minLod, float maxLod, int borderColor, bool unnormalizedCoordinates);
void tknDestroySampler(void *pTknGfxContext, void *pTknSampler);

void *tknCreateBindingGroupLayout(void *pTknGfxContext, uint32_t shaderPathCount, const char **shaderPaths, uint32_t set);
void tknDestroyBindingGroupLayout(void *pTknGfxContext, void *pTknBindingGroupLayout);
