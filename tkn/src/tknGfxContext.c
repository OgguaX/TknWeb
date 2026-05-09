#include "tknGfx.h"
#include "tkn.h"
static void createVkInstance(TknGfxContext *pTknGfxContext, int extensionCount, const char **extensions)
{
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "TknWeb",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Tickernel",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensions,
    };

    tknAssertVkResult(vkCreateInstance(&createInfo, NULL, &pTknGfxContext->vkInstance));
}
static void destroyVkInstance(TknGfxContext *pTknGfxContext)
{
    if (pTknGfxContext->vkInstance != NULL)
    {
        vkDestroyInstance(pTknGfxContext->vkInstance, NULL);
        pTknGfxContext->vkInstance = NULL;
    }
}

static void tknGetGfxAndPresentQueueFamilyIndices(TknGfxContext *pTknGfxContext, VkPhysicalDevice vkPhysicalDevice, uint32_t *pGfxQueueFamilyIndex, uint32_t *pPresentQueueFamilyIndex)
{
    VkSurfaceKHR vkSurface = pTknGfxContext->vkSurface;
    uint32_t queueFamilyPropertiesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertiesCount, NULL);
    VkQueueFamilyProperties *vkQueueFamilyPropertiesArray = tknMalloc(queueFamilyPropertiesCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyPropertiesCount, vkQueueFamilyPropertiesArray);
    *pGfxQueueFamilyIndex = UINT32_MAX;
    *pPresentQueueFamilyIndex = UINT32_MAX;
    for (int queueFamilyPropertiesIndex = 0; queueFamilyPropertiesIndex < queueFamilyPropertiesCount; queueFamilyPropertiesIndex++)
    {
        VkQueueFamilyProperties vkQueueFamilyProperties = vkQueueFamilyPropertiesArray[queueFamilyPropertiesIndex];
        if (vkQueueFamilyProperties.queueCount > 0 && vkQueueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            *pGfxQueueFamilyIndex = queueFamilyPropertiesIndex;
        }
        else
        {
        }
        VkBool32 pSupported = VK_FALSE;
        tknAssertVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, queueFamilyPropertiesIndex, vkSurface, &pSupported));
        if (vkQueueFamilyProperties.queueCount > 0 && pSupported)
        {
            *pPresentQueueFamilyIndex = queueFamilyPropertiesIndex;
        }
        else
        {
        }

        if (*pGfxQueueFamilyIndex != UINT32_MAX && *pPresentQueueFamilyIndex != UINT32_MAX)
        {
            break;
        }
    }
    tknFree(vkQueueFamilyPropertiesArray);
}
static void tknPickPhysicalDevice(TknGfxContext *pTknGfxContext)
{
    uint32_t deviceCount = -1;
    tknAssertVkResult(vkEnumeratePhysicalDevices(pTknGfxContext->vkInstance, &deviceCount, NULL));
    if (deviceCount <= 0)
    {
        printf("failed to find GPUs with Vulkan support!");
    }
    else
    {
        VkPhysicalDevice *devices = tknMalloc(deviceCount * sizeof(VkPhysicalDevice));
        tknAssertVkResult(vkEnumeratePhysicalDevices(pTknGfxContext->vkInstance, &deviceCount, devices));
        uint32_t maxScore = 0;
        char *targetDeviceName = NULL;
        pTknGfxContext->vkPhysicalDevice = VK_NULL_HANDLE;
        VkSurfaceKHR vkSurface = pTknGfxContext->vkSurface;
        for (uint32_t deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
        {
            uint32_t score = 0;
            VkPhysicalDevice vkPhysicalDevice = devices[deviceIndex];
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(vkPhysicalDevice, &deviceProperties);
            char *requiredExtensionNames[] = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            };
            uint32_t requiredExtensionCount = TKN_ARRAY_COUNT(requiredExtensionNames);
            uint32_t extensionCount = 0;
            tknAssertVkResult(vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &extensionCount, NULL));
            VkExtensionProperties *extensionProperties = tknMalloc(extensionCount * sizeof(VkExtensionProperties));
            tknAssertVkResult(vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &extensionCount, extensionProperties));
            uint32_t requiredExtensionIndex;
            for (requiredExtensionIndex = 0; requiredExtensionIndex < requiredExtensionCount; requiredExtensionIndex++)
            {
                char *requiredExtensionName = requiredExtensionNames[requiredExtensionIndex];
                uint32_t extensionIndex;
                for (extensionIndex = 0; extensionIndex < extensionCount; extensionIndex++)
                {
                    char *supportedExtensionName = extensionProperties[extensionIndex].extensionName;
                    if (0 == strcmp(supportedExtensionName, requiredExtensionName))
                    {
                        break;
                    }
                    else
                    {
                    }
                }
                if (extensionIndex < extensionCount)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            tknFree(extensionProperties);
            if (requiredExtensionIndex < requiredExtensionCount)
            {
                continue;
            }
            else
            {
            }

            uint32_t tknGfxQueueFamilyIndex;
            uint32_t tknPresentQueueFamilyIndex;
            tknGetGfxAndPresentQueueFamilyIndices(pTknGfxContext, vkPhysicalDevice, &tknGfxQueueFamilyIndex, &tknPresentQueueFamilyIndex);
            if (UINT32_MAX == tknGfxQueueFamilyIndex || UINT32_MAX == tknPresentQueueFamilyIndex)
            {
                continue;
            }

            uint32_t surfaceFormatCount;
            tknAssertVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &surfaceFormatCount, NULL));
            VkSurfaceFormatKHR *supportedSurfaceFormats = tknMalloc(surfaceFormatCount * sizeof(VkSurfaceFormatKHR));
            tknAssertVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, vkSurface, &surfaceFormatCount, supportedSurfaceFormats));
            uint32_t supportedSurfaceFormatIndex;
            for (supportedSurfaceFormatIndex = 0; supportedSurfaceFormatIndex < surfaceFormatCount; supportedSurfaceFormatIndex++)
            {
                VkSurfaceFormatKHR vkSurfaceFormat = supportedSurfaceFormats[supportedSurfaceFormatIndex];
                if (vkSurfaceFormat.colorSpace == pTknGfxContext->tknSurfaceFormat.colorSpace &&
                    vkSurfaceFormat.format == pTknGfxContext->tknSurfaceFormat.format)
                {
                    break;
                }
                else
                {
                }
            }
            tknFree(supportedSurfaceFormats);
            if (supportedSurfaceFormatIndex < surfaceFormatCount)
            {
            }
            else
            {
                continue;
            }

            uint32_t presentModeCount;
            tknAssertVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &presentModeCount, NULL));
            VkPresentModeKHR *supportedPresentModes = tknMalloc(presentModeCount * sizeof(VkPresentModeKHR));
            tknAssertVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, vkSurface, &presentModeCount, supportedPresentModes));
            uint32_t supportedPresentModeIndex;
            for (supportedPresentModeIndex = 0; supportedPresentModeIndex < presentModeCount; supportedPresentModeIndex++)
            {
                VkPresentModeKHR supportedPresentMode = supportedPresentModes[supportedPresentModeIndex];
                if (supportedPresentMode == pTknGfxContext->tknPresentMode)
                {
                    break;
                }
                else
                {
                }
            }
            tknFree(supportedPresentModes);
            if (supportedPresentModeIndex < presentModeCount)
            {
            }
            else
            {
                continue;
            }

            VkFormatProperties vkFormatProperties;
            vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice, VK_FORMAT_ASTC_4x4_UNORM_BLOCK, &vkFormatProperties);
            if (!(vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
            {
                continue;
            }

            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                score += 1000;
            }
            else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                score += 500;
            }
            else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            {
                score += 300;
            }
            else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
            {
                score += 100;
            }

            if (score >= maxScore)
            {
                maxScore = score;
                targetDeviceName = deviceProperties.deviceName;
                pTknGfxContext->vkPhysicalDevice = vkPhysicalDevice;
                pTknGfxContext->tknGfxQueueFamilyIndex = tknGfxQueueFamilyIndex;
                pTknGfxContext->tknPresentQueueFamilyIndex = tknPresentQueueFamilyIndex;
                pTknGfxContext->vkPhysicalDeviceProperties = deviceProperties;
            }
            else
            {
            }
        }
        tknFree(devices);

        if (NULL != pTknGfxContext->vkPhysicalDevice)
        {
            printf("Selected target physical device named %s\n", targetDeviceName);
        }
        else
        {
            tknError("failed to find GPUs with Vulkan support!");
        }
    }
}
static void tknPopulateLogicalDevice(TknGfxContext *pTknGfxContext)
{
    VkPhysicalDevice vkPhysicalDevice = pTknGfxContext->vkPhysicalDevice;
    uint32_t tknGfxQueueFamilyIndex = pTknGfxContext->tknGfxQueueFamilyIndex;
    uint32_t tknPresentQueueFamilyIndex = pTknGfxContext->tknPresentQueueFamilyIndex;
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo *queueCreateInfos;
    uint32_t queueCount;
    if (tknGfxQueueFamilyIndex == tknPresentQueueFamilyIndex)
    {
        queueCount = 1;
        queueCreateInfos = tknMalloc(sizeof(VkDeviceQueueCreateInfo) * queueCount);
        VkDeviceQueueCreateInfo gfxCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .queueFamilyIndex = tknGfxQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
            };
        queueCreateInfos[0] = gfxCreateInfo;
    }
    else
    {
        queueCount = 2;
        queueCreateInfos = tknMalloc(sizeof(VkDeviceQueueCreateInfo) * queueCount);
        VkDeviceQueueCreateInfo gfxCreateInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = NULL,
                .flags = 0,
                .queueFamilyIndex = tknGfxQueueFamilyIndex,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
            };
        VkDeviceQueueCreateInfo presentCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueFamilyIndex = tknPresentQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        queueCreateInfos[0] = gfxCreateInfo;
        queueCreateInfos[1] = presentCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures =
        {
            .fillModeNonSolid = VK_TRUE,
            .sampleRateShading = VK_TRUE,
        };
    char **enabledLayerNames = NULL;
    uint32_t enabledLayerCount = 0;

    char *extensionNames[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset",
    };
    uint32_t extensionCount = TKN_ARRAY_COUNT(extensionNames);
    VkDeviceCreateInfo vkDeviceCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .queueCreateInfoCount = queueCount,
            .pQueueCreateInfos = queueCreateInfos,
            .enabledLayerCount = enabledLayerCount,
            .ppEnabledLayerNames = (const char *const *)enabledLayerNames,
            .enabledExtensionCount = extensionCount,
            .ppEnabledExtensionNames = (const char *const *)extensionNames,
            .pEnabledFeatures = &deviceFeatures,
        };
    tknAssertVkResult(vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, NULL, &pTknGfxContext->vkDevice));
    vkGetDeviceQueue(pTknGfxContext->vkDevice, tknGfxQueueFamilyIndex, 0, &pTknGfxContext->vkGfxQueue);
    vkGetDeviceQueue(pTknGfxContext->vkDevice, tknPresentQueueFamilyIndex, 0, &pTknGfxContext->vkPresentQueue);
    tknFree(queueCreateInfos);
}

static void tknCleanupLogicalDevice(TknGfxContext *pTknGfxContext)
{
    vkDestroyDevice(pTknGfxContext->vkDevice, NULL);
}

void *tknCreateGfxContextPtr(int extensionCount, const char **extensions, void *pSurface, int width, int height, int globalShaderPathCount, const char **globalShaderPaths)
{
    TknGfxContext *pTknGfxContext = tknMalloc(sizeof(TknGfxContext));
    *pTknGfxContext = (TknGfxContext){
        .vkInstance = VK_NULL_HANDLE,
        .vkSurface = (VkSurfaceKHR)pSurface,
        .tknSurfaceFormat = {
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
        .tknPresentMode = VK_PRESENT_MODE_FIFO_KHR,
        .vkPhysicalDevice = VK_NULL_HANDLE,

        .tknGfxQueueFamilyIndex = 0,
        .tknPresentQueueFamilyIndex = 0,
        .vkPhysicalDeviceProperties = 0,
        .vkDevice = VK_NULL_HANDLE,
        .vkGfxQueue = VK_NULL_HANDLE,
        .vkPresentQueue = VK_NULL_HANDLE,
    };
    createVkInstance(pTknGfxContext, extensionCount, extensions);
    tknPickPhysicalDevice(pTknGfxContext);
    tknPopulateLogicalDevice(pTknGfxContext);
    VkExtent2D tknSwapchainExtent = {
        .width = width,
        .height = height,
    };
    uint32_t targetSwapchainImageCount = 2;
    return pTknGfxContext;
}
void tknDestroyGfxContextPtr(void *pTknGfxContext)
{
    TknGfxContext *pTknGfxContextCasted = (TknGfxContext *)pTknGfxContext;
    tknCleanupLogicalDevice(pTknGfxContextCasted);
    destroyVkInstance(pTknGfxContextCasted);
    tknFree(pTknGfxContextCasted);
}

VkImageType tknImageDimensionToVkImageType(TknImageDimension dimension)
{
    switch (dimension)
    {
    case TKN_IMAGE_1D:
        return VK_IMAGE_TYPE_1D;
    case TKN_IMAGE_2D:
        return VK_IMAGE_TYPE_2D;
    case TKN_IMAGE_3D:
        return VK_IMAGE_TYPE_3D;
    default:
        tknError("Invalid image dimension: %d", dimension);
        return VK_IMAGE_TYPE_2D;
    }
}

VkImageViewType tknImageViewDimensionToVkImageViewType(TknImageViewDimension dimension)
{
    switch (dimension)
    {
    case TKN_IMAGE_VIEW_1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case TKN_IMAGE_VIEW_2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TKN_IMAGE_VIEW_3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TKN_IMAGE_VIEW_1D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case TKN_IMAGE_VIEW_2D_ARRAY:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case TKN_IMAGE_VIEW_CUBE:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case TKN_IMAGE_VIEW_CUBE_ARRAY:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    default:
        tknError("Invalid image view dimension: %d", dimension);
        return VK_IMAGE_VIEW_TYPE_2D;
    }
}

VkFormat tknImageFormatToVkFormat(TknImageFormat format)
{
    switch (format)
    {
    case TKN_FORMAT_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case TKN_FORMAT_R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case TKN_FORMAT_R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case TKN_FORMAT_R32G32B32A32_SFLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case TKN_FORMAT_D32_SFLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case TKN_FORMAT_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case TKN_FORMAT_R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case TKN_FORMAT_R16_SFLOAT:
        return VK_FORMAT_R16_SFLOAT;
    case TKN_FORMAT_R32_SFLOAT:
        return VK_FORMAT_R32_SFLOAT;
    default:
        tknError("Invalid image format: %d", format);
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

VkImageAspectFlags tknImageAspectToVkImageAspectFlags(int aspectFlags)
{
    if (aspectFlags == 0)
    {
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageAspectFlags result = 0;
    if (aspectFlags & TKN_ASPECT_COLOR)
        result |= VK_IMAGE_ASPECT_COLOR_BIT;
    if (aspectFlags & TKN_ASPECT_DEPTH)
        result |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if (aspectFlags & TKN_ASPECT_STENCIL)
        result |= VK_IMAGE_ASPECT_STENCIL_BIT;

    return result == 0 ? VK_IMAGE_ASPECT_COLOR_BIT : result;
}
