#include <vulkan/vulkan.h>
#include "tknCore.h"

#define TKN_ARRAY_COUNT(array) (NULL == array) ? 0 : (sizeof(array) / sizeof(array[0]))

typedef struct TknBindGroup
{
} TknBindGroup;

typedef struct TknUniformBuffer
{
} TknUniformBuffer;

typedef struct TknSampler
{
} TknSampler;

typedef struct TknImage
{
    VkImage vkImage;
    VkDeviceMemory vkDeviceMemory;
    TknHashSet tknImageViewPtrHashSet;
    VkExtent3D tknSwapchainExtent;
} TknImage;
typedef struct TknImageView
{
    TknImage *pTknImage;
    VkImageView vkImageView;
    TknHashSet TknBindGroupPtrHashSet;
} TknImageView;

typedef struct TknRenderPass
{
} TknRenderPass;

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

    VkCommandPool vkGfxCommandPool;

} TknGfxContext;

void tknAssertVkResult(VkResult vkResult);
void tknCreateVkBuffer(TknGfxContext *pTknGfxContext, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer *pVkBuffer, VkDeviceMemory *pVkDeviceMemory);
void tknDestroyVkBuffer(TknGfxContext *pTknGfxContext, VkBuffer vkBuffer, VkDeviceMemory vkDeviceMemory);
VkCommandBuffer tknBeginSingleTimeCommands(TknGfxContext *pTknGfxContext);
void tknEndSingleTimeCommands(TknGfxContext *pTknGfxContext, VkCommandBuffer vkCommandBuffer);