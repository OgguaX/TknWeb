#include <vulkan/vulkan.h>
#include "tknCore.h"
#include <spirv_reflect.h>
#define TKN_ARRAY_COUNT(array) (NULL == array) ? 0 : (sizeof(array) / sizeof(array[0]))
typedef enum
{
    TKN_GLOBAL_DESCRIPTOR_SET,
    TKN_RENDERPASS_DESCRIPTOR_SET,
    TKN_PIPELINE_DESCRIPTOR_SET,
    TKN_MAX_DESCRIPTOR_SET,
} TknTickernelDescriptorSet;

typedef enum
{
    TKN_VERTEX_BINDING_DESCRIPTION = 0,
    TKN_INSTANCE_BINDING_DESCRIPTION = 1,
    TKN_MAX_VERTEX_BINDING_DESCRIPTION = 2,
} TknVertexBinding;

typedef struct TknUniformBuffer
{
} TknUniformBuffer;

typedef struct TknSampler
{
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
    TknHashSet TknBindingGroupPtrHashSet;
} TknImageView;

typedef struct TknBindingGroupLayout
{
    uint32_t bindingCount;
    uint32_t usedBindingCount;
    VkDescriptorType *vkDescriptorTypes;
    uint32_t *descriptorCounts;
    VkShaderStageFlags *vkShaderStageFlags;
    bool *bindingUsed;

    uint32_t shaderPathCount;
    const char **shaderPaths;
    SpvReflectShaderModule *pSpvReflectShaderModules;
} TknBindingGroupLayout;

typedef struct TknBindingGroup
{
    VkDescriptorSetLayout vkDescriptorSetLayout;
    VkDescriptorPool vkDescriptorPool;
    VkDescriptorSet vkDescriptorSet;
} TknBindingGroup;

typedef struct TknVertexInputLayout
{
    TknVertexBinding tknVertexBinding;
    uint32_t vkVertexInputAttributeDescriptionCount;
    VkVertexInputAttributeDescription *vkVertexInputAttributeDescriptions;
} TknVertexInputLayout;

typedef struct TknPipeline
{
    VkPipeline vkPipeline;
    VkPipelineLayout vkPipelineLayout;
    TknVertexInputLayout tknVertexInputLayouts[TKN_MAX_VERTEX_BINDING_DESCRIPTION];
} TknPipeline;

typedef struct TknGfxContext
{
    VkInstance vkInstance;
    VkSurfaceKHR vkSurface;
    VkSurfaceFormatKHR tknSurfaceFormat;
    VkPresentModeKHR tknPresentMode;
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

void *tknCreateBindingGroupLayout(uint32_t shaderPathCount, const char **shaderPaths, uint32_t set);
void tknDestroyBindingGroupLayout(void *pTknBindingGroupLayout);
