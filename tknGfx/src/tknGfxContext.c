#include "tknGfx.h"
#include "tknGfxInternal.h"

static void tknCreateVkInstance(TknGfxContext *pTknGfxContext, int extensionCount, const char **extensions)
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
static void tknDestroyVkInstance(TknGfxContext *pTknGfxContext)
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
                if (vkSurfaceFormat.colorSpace == pTknGfxContext->vkSurfaceFormat.colorSpace &&
                    vkSurfaceFormat.format == pTknGfxContext->vkSurfaceFormat.format)
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
                if (supportedPresentMode == pTknGfxContext->vkPresentMode)
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

static void tknCreateSwapchain(TknGfxContext *pTknGfxContext, VkExtent2D targetSwapchainExtent)
{
    VkPhysicalDevice vkPhysicalDevice = pTknGfxContext->vkPhysicalDevice;
    VkSurfaceKHR vkSurface = pTknGfxContext->vkSurface;
    VkDevice vkDevice = pTknGfxContext->vkDevice;

    tknAssertVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &pTknGfxContext->vkSurfaceCapabilities));

    uint32_t tknSwapchainImageCount = TKN_CLAMP(pTknGfxContext->swapchainImageCount, pTknGfxContext->vkSurfaceCapabilities.minImageCount, pTknGfxContext->vkSurfaceCapabilities.maxImageCount);
    VkExtent2D tknSwapchainExtent;
    tknSwapchainExtent.width = TKN_CLAMP(targetSwapchainExtent.width, pTknGfxContext->vkSurfaceCapabilities.minImageExtent.width, pTknGfxContext->vkSurfaceCapabilities.maxImageExtent.width);
    tknSwapchainExtent.height = TKN_CLAMP(targetSwapchainExtent.height, pTknGfxContext->vkSurfaceCapabilities.minImageExtent.height, pTknGfxContext->vkSurfaceCapabilities.maxImageExtent.height);

    VkSharingMode imageSharingMode = pTknGfxContext->tknGfxQueueFamilyIndex != pTknGfxContext->tknPresentQueueFamilyIndex ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    uint32_t queueFamilyIndexCount = pTknGfxContext->tknGfxQueueFamilyIndex != pTknGfxContext->tknPresentQueueFamilyIndex ? 2 : 0;
    uint32_t pQueueFamilyIndices[] = {pTknGfxContext->tknGfxQueueFamilyIndex, pTknGfxContext->tknPresentQueueFamilyIndex};

    VkSwapchainCreateInfoKHR swapchainCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .surface = vkSurface,
            .minImageCount = tknSwapchainImageCount,
            .imageFormat = pTknGfxContext->vkSurfaceFormat.format,
            .imageColorSpace = pTknGfxContext->vkSurfaceFormat.colorSpace,
            .imageExtent = tknSwapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = imageSharingMode,
            .queueFamilyIndexCount = queueFamilyIndexCount,
            .pQueueFamilyIndices = pQueueFamilyIndices,
            .preTransform = pTknGfxContext->vkSurfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = pTknGfxContext->vkPresentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };
    tknAssertVkResult(vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, NULL, &pTknGfxContext->vkSwapchain));

    VkImage *tknSwapchainImages = tknMalloc(tknSwapchainImageCount * sizeof(VkImage));
    tknAssertVkResult(vkGetSwapchainImagesKHR(vkDevice, pTknGfxContext->vkSwapchain, &tknSwapchainImageCount, tknSwapchainImages));

    pTknGfxContext->tknSwapchainImagePtrs = tknMalloc(tknSwapchainImageCount * sizeof(TknImage *));
    pTknGfxContext->tknSwapchainImageViewPtrs = tknMalloc(tknSwapchainImageCount * sizeof(TknImageView *));
    for (uint32_t i = 0; i < tknSwapchainImageCount; i++)
    {
        TknImage *pTknImage = (TknImage *)tknMalloc(sizeof(TknImage));
        pTknImage->vkImage = tknSwapchainImages[i];
        pTknImage->vkFormat = pTknGfxContext->vkSurfaceFormat.format;
        pTknImage->vkDeviceMemory = VK_NULL_HANDLE;
        pTknImage->tknImageViewPtrHashSet = tknCreateHashSet(sizeof(void *));
        pTknGfxContext->tknSwapchainImagePtrs[i] = pTknImage;

        TknImageView *pTknImageView = (TknImageView *)tknCreateImageView(
            pTknGfxContext,
            0,
            1,
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            1,
            VK_IMAGE_VIEW_TYPE_2D,
            pTknGfxContext->vkSurfaceFormat.format,
            pTknImage);
        pTknGfxContext->tknSwapchainImageViewPtrs[i] = pTknImageView;
    }
    tknFree(tknSwapchainImages);
    pTknGfxContext->swapchainImageCount = tknSwapchainImageCount;
};
static void tknDestroySwapchain(TknGfxContext *pTknGfxContext)
{
    VkDevice vkDevice = pTknGfxContext->vkDevice;

    for (uint32_t i = 0; i < pTknGfxContext->swapchainImageCount; i++)
    {
        tknDestroyImageView(pTknGfxContext, pTknGfxContext->tknSwapchainImageViewPtrs[i]);
        // Swapchain's VkImage is managed by vkDestroySwapchainKHR, only free the TknImage wrapper
        tknDestroyHashSet(pTknGfxContext->tknSwapchainImagePtrs[i]->tknImageViewPtrHashSet);
        tknFree(pTknGfxContext->tknSwapchainImagePtrs[i]);
    }
    tknFree(pTknGfxContext->tknSwapchainImagePtrs);
    tknFree(pTknGfxContext->tknSwapchainImageViewPtrs);
    vkDestroySwapchainKHR(vkDevice, pTknGfxContext->vkSwapchain, NULL);

    // Reset swapchain state
    pTknGfxContext->swapchainImageCount = 0;
    pTknGfxContext->tknSwapchainImagePtrs = NULL;
    pTknGfxContext->tknSwapchainImageViewPtrs = NULL;
    pTknGfxContext->vkSwapchain = VK_NULL_HANDLE;
}
static void tknUpdateSwapchain(TknGfxContext *pTknGfxContext, VkExtent2D tknSwapchainExtent)
{
    VkPhysicalDevice vkPhysicalDevice = pTknGfxContext->vkPhysicalDevice;
    VkSurfaceKHR vkSurface = pTknGfxContext->vkSurface;
    VkDevice vkDevice = pTknGfxContext->vkDevice;
    tknAssertVkResult(vkDeviceWaitIdle(vkDevice));

    // Destroy only VkImageView (keep TknImageView objects intact)
    for (uint32_t i = 0; i < pTknGfxContext->swapchainImageCount; i++)
    {
        vkDestroyImageView(vkDevice, pTknGfxContext->tknSwapchainImageViewPtrs[i]->vkImageView, NULL);
    }

    // Destroy VkSwapchain
    vkDestroySwapchainKHR(vkDevice, pTknGfxContext->vkSwapchain, NULL);

    // Update surface capabilities
    tknAssertVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, vkSurface, &pTknGfxContext->vkSurfaceCapabilities));

    // Clamp new extent
    tknSwapchainExtent.width = TKN_CLAMP(tknSwapchainExtent.width, pTknGfxContext->vkSurfaceCapabilities.minImageExtent.width, pTknGfxContext->vkSurfaceCapabilities.maxImageExtent.width);
    tknSwapchainExtent.height = TKN_CLAMP(tknSwapchainExtent.height, pTknGfxContext->vkSurfaceCapabilities.minImageExtent.height, pTknGfxContext->vkSurfaceCapabilities.maxImageExtent.height);

    // Create new VkSwapchain
    VkSharingMode imageSharingMode = pTknGfxContext->tknGfxQueueFamilyIndex != pTknGfxContext->tknPresentQueueFamilyIndex ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    uint32_t queueFamilyIndexCount = pTknGfxContext->tknGfxQueueFamilyIndex != pTknGfxContext->tknPresentQueueFamilyIndex ? 2 : 0;
    uint32_t pQueueFamilyIndices[] = {pTknGfxContext->tknGfxQueueFamilyIndex, pTknGfxContext->tknPresentQueueFamilyIndex};
    VkSwapchainCreateInfoKHR swapchainCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .surface = vkSurface,
            .minImageCount = pTknGfxContext->swapchainImageCount,
            .imageFormat = pTknGfxContext->vkSurfaceFormat.format,
            .imageColorSpace = pTknGfxContext->vkSurfaceFormat.colorSpace,
            .imageExtent = tknSwapchainExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = imageSharingMode,
            .queueFamilyIndexCount = queueFamilyIndexCount,
            .pQueueFamilyIndices = pQueueFamilyIndices,
            .preTransform = pTknGfxContext->vkSurfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = pTknGfxContext->vkPresentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE,
        };
    tknAssertVkResult(vkCreateSwapchainKHR(vkDevice, &swapchainCreateInfo, NULL, &pTknGfxContext->vkSwapchain));

    // Get new VkImage handles
    uint32_t updatedSwapchainImageCount = pTknGfxContext->swapchainImageCount;
    VkImage *tknSwapchainImages = tknMalloc(updatedSwapchainImageCount * sizeof(VkImage));
    tknAssertVkResult(vkGetSwapchainImagesKHR(vkDevice, pTknGfxContext->vkSwapchain, &updatedSwapchainImageCount, tknSwapchainImages));

    // Update existing TknImage objects and recreate VkImageView in existing TknImageView objects
    for (uint32_t i = 0; i < updatedSwapchainImageCount; i++)
    {
        // Update VkImage in existing TknImage
        pTknGfxContext->tknSwapchainImagePtrs[i]->vkImage = tknSwapchainImages[i];

        // Recreate VkImageView in existing TknImageView
        VkComponentMapping components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        };
        VkImageSubresourceRange subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = tknSwapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = pTknGfxContext->vkSurfaceFormat.format,
            .components = components,
            .subresourceRange = subresourceRange,
        };
        tknAssertVkResult(vkCreateImageView(vkDevice, &imageViewCreateInfo, NULL, &pTknGfxContext->tknSwapchainImageViewPtrs[i]->vkImageView));
    }
    tknFree(tknSwapchainImages);
}

static void tknCreateSignals(TknGfxContext *pTknGfxContext)
{
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
    };
    VkDevice vkDevice = pTknGfxContext->vkDevice;
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    tknAssertVkResult(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &pTknGfxContext->vkImageAvailableSemaphore));
    tknAssertVkResult(vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, NULL, &pTknGfxContext->vkRenderFinishedSemaphore));
    tknAssertVkResult(vkCreateFence(vkDevice, &fenceCreateInfo, NULL, &pTknGfxContext->vkRenderFinishedFence));
}
static void tknDestroySignals(TknGfxContext *pTknGfxContext)
{
    VkDevice vkDevice = pTknGfxContext->vkDevice;
    vkDestroySemaphore(vkDevice, pTknGfxContext->vkImageAvailableSemaphore, NULL);
    vkDestroySemaphore(vkDevice, pTknGfxContext->vkRenderFinishedSemaphore, NULL);
    vkDestroyFence(vkDevice, pTknGfxContext->vkRenderFinishedFence, NULL);
}
static void tknCreateCommandPools(TknGfxContext *pTknGfxContext)
{
    VkCommandPoolCreateInfo vkCommandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = pTknGfxContext->tknGfxQueueFamilyIndex,
    };
    tknAssertVkResult(vkCreateCommandPool(pTknGfxContext->vkDevice, &vkCommandPoolCreateInfo, NULL, &pTknGfxContext->vkGfxCommandPool));
}
static void tknDestroyCommandPools(TknGfxContext *pTknGfxContext)
{
    vkDestroyCommandPool(pTknGfxContext->vkDevice, pTknGfxContext->vkGfxCommandPool, NULL);
}

static void tknCreateVkCommandBuffers(TknGfxContext *pTknGfxContext)
{
    pTknGfxContext->vkGfxCommandBuffers = tknMalloc(sizeof(VkCommandBuffer) * pTknGfxContext->swapchainImageCount);
    VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = pTknGfxContext->vkGfxCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = pTknGfxContext->swapchainImageCount,
    };
    tknAssertVkResult(vkAllocateCommandBuffers(pTknGfxContext->vkDevice, &vkCommandBufferAllocateInfo, pTknGfxContext->vkGfxCommandBuffers));
}
static void tknDestroyVkCommandBuffers(TknGfxContext *pTknGfxContext)
{
    vkFreeCommandBuffers(pTknGfxContext->vkDevice, pTknGfxContext->vkGfxCommandPool, pTknGfxContext->swapchainImageCount, pTknGfxContext->vkGfxCommandBuffers);
    tknFree(pTknGfxContext->vkGfxCommandBuffers);
}

static void tknCreateGlobalBindingGroup(TknGfxContext *pTknGfxContext, uint32_t shaderPathCount, const char **shaderPaths)
{
    pTknGfxContext->pTknGlobalBindingGroupLayout = tknCreateBindingGroupLayout(pTknGfxContext, shaderPathCount, shaderPaths, TKN_GLOBAL_DESCRIPTOR_SET);
    pTknGfxContext->pTknGlobalBindingGroup = tknCreateBindingGroup(pTknGfxContext, pTknGfxContext->pTknGlobalBindingGroupLayout, 0, NULL);
}
static void tknDestroyGlobalBindingGroup(TknGfxContext *pTknGfxContext)
{
    tknDestroyBindingGroup(pTknGfxContext, pTknGfxContext->pTknGlobalBindingGroup);
    pTknGfxContext->pTknGlobalBindingGroup = NULL;
    tknDestroyBindingGroupLayout(pTknGfxContext, pTknGfxContext->pTknGlobalBindingGroupLayout);
    pTknGfxContext->pTknGlobalBindingGroupLayout = NULL;
}

void *tknCreateGfxContextPtr(uint32_t extensionCount, const char **extensions, void *pSurface, uint32_t width, uint32_t height, uint32_t globalShaderPathCount, const char **globalShaderPaths)
{
    TknGfxContext *pTknGfxContext = tknMalloc(sizeof(TknGfxContext));
    *pTknGfxContext = (TknGfxContext){
        .vkInstance = VK_NULL_HANDLE,
        .vkSurface = VK_NULL_HANDLE,
        .vkSurfaceFormat = {
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        },
        .vkPresentMode = VK_PRESENT_MODE_FIFO_KHR,
        .vkPhysicalDevice = VK_NULL_HANDLE,

        .tknGfxQueueFamilyIndex = 0,
        .tknPresentQueueFamilyIndex = 0,
        .vkPhysicalDeviceProperties = 0,
        .vkDevice = VK_NULL_HANDLE,
        .vkGfxQueue = VK_NULL_HANDLE,
        .vkPresentQueue = VK_NULL_HANDLE,

        .vkSurfaceCapabilities = 0,
        .swapchainImageCount = 2,
        .tknSwapchainImagePtrs = NULL,
        .tknSwapchainImageViewPtrs = NULL,
        .vkSwapchain = VK_NULL_HANDLE,

        .vkImageAvailableSemaphore = VK_NULL_HANDLE,
        .vkRenderFinishedSemaphore = VK_NULL_HANDLE,
        .vkRenderFinishedFence = VK_NULL_HANDLE,

        .vkGfxCommandPool = VK_NULL_HANDLE,
        .vkGfxCommandBuffers = NULL,

        .pTknGlobalBindingGroupLayout = NULL,
        .pTknGlobalBindingGroup = NULL,

        .frameCount = 0,
    };
    tknCreateVkInstance(pTknGfxContext, extensionCount, extensions);
    tknPickPhysicalDevice(pTknGfxContext);
    tknPopulateLogicalDevice(pTknGfxContext);

    tknCreateSwapchain(pTknGfxContext, (VkExtent2D){
                                           .width = width,
                                           .height = height,
                                       });
    tknCreateSignals(pTknGfxContext);
    tknCreateCommandPools(pTknGfxContext);
    tknCreateVkCommandBuffers(pTknGfxContext);

    tknCreateGlobalBindingGroup(pTknGfxContext, globalShaderPathCount, globalShaderPaths);
    return pTknGfxContext;
}

void tknDestroyGfxContextPtr(void *pTknGfxContext)
{
    TknGfxContext *pTknGfxContextCasted = (TknGfxContext *)pTknGfxContext;
    tknDestroyGlobalBindingGroup(pTknGfxContextCasted);

    tknDestroyVkCommandBuffers(pTknGfxContextCasted);
    tknDestroyCommandPools(pTknGfxContextCasted);
    tknDestroySignals(pTknGfxContextCasted);
    tknDestroySwapchain(pTknGfxContextCasted);
    tknCleanupLogicalDevice(pTknGfxContextCasted);
    tknDestroyVkInstance(pTknGfxContextCasted);
    tknFree(pTknGfxContextCasted);
}
