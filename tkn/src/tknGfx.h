#include <vulkan/vulkan.h>
#include "tknCore.h"

#define TKN_ARRAY_COUNT(array) (NULL == array) ? 0 : (sizeof(array) / sizeof(array[0]))

// ============================================
// 格式枚举（Vulkan 格式的 C 友好包装）
// ============================================
typedef enum TknImageFormat
{
    TKN_FORMAT_R8G8B8A8_UNORM = 0,          // RGBA 8-bit unsigned
    TKN_FORMAT_R8G8B8A8_SRGB = 1,           // RGBA 8-bit SRGB
    TKN_FORMAT_R16G16B16A16_SFLOAT = 2,     // RGBA 16-bit float
    TKN_FORMAT_R32G32B32A32_SFLOAT = 3,     // RGBA 32-bit float
    TKN_FORMAT_D32_SFLOAT = 4,              // Depth 32-bit float
    TKN_FORMAT_D24_UNORM_S8_UINT = 5,       // Depth 24 + Stencil 8
    TKN_FORMAT_R8_UNORM = 6,                // Red 8-bit
    TKN_FORMAT_R16_SFLOAT = 7,              // Red 16-bit float
    TKN_FORMAT_R32_SFLOAT = 8,              // Red 32-bit float
} TknImageFormat;

// ============================================
// 图像维度枚举
// ============================================
typedef enum TknImageDimension
{
    TKN_IMAGE_1D = 1,
    TKN_IMAGE_2D = 2,
    TKN_IMAGE_3D = 3,
} TknImageDimension;

// ============================================
// 图像视图维度枚举
// ============================================
typedef enum TknImageViewDimension
{
    TKN_IMAGE_VIEW_1D = 1,
    TKN_IMAGE_VIEW_2D = 2,
    TKN_IMAGE_VIEW_3D = 3,
    TKN_IMAGE_VIEW_1D_ARRAY = 4,
    TKN_IMAGE_VIEW_2D_ARRAY = 5,
    TKN_IMAGE_VIEW_CUBE = 6,
    TKN_IMAGE_VIEW_CUBE_ARRAY = 7,
} TknImageViewDimension;

// ============================================
// 图像 Aspect 标志枚举
// ============================================
typedef enum TknImageAspect
{
    TKN_ASPECT_COLOR = 0x01,
    TKN_ASPECT_DEPTH = 0x02,
    TKN_ASPECT_STENCIL = 0x04,
} TknImageAspect;

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
} TknImage;
typedef struct TknImageView
{
    VkImageView vkImageView;
    TknHashSet TknBindGroupPtrHashSet;
} TknImageView;

typedef struct TknFrame
{
} TknFrame;

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
    TknImage *pTknSwapchainImages;

    // VkSemaphore vkImageAvailableSemaphore;
    // VkSemaphore vkRenderFinishedSemaphore;
    // VkFence vkRenderFinishedFence;

    // VkCommandPool vkGfxCommandPool;
    // VkCommandBuffer *vkGfxCommandBuffers;

    // TknHashSet tknRenderPassPtrHashSet;
    // TknBindGroup *pTknGlobalDescriptorSet;
    // TknHashSet tknVertexInputLayoutPtrHashSet;

    // // Empty resources for empty bindings
    // TknUniformBuffer *pTknEmptyUniformBuffer;
    // TknSampler *pTknEmptySampler;
    // TknImage *pTknEmptyImage;

    // TknFrame *pTknFrame;
} TknGfxContext;

void tknAssertVkResult(VkResult vkResult);