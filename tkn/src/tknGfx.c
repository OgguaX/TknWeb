#include "tknGfx.h"

void tknAssertVkResult(VkResult vkResult)
{
    tknAssert(vkResult == VK_SUCCESS, "Vulkan error: %d", vkResult);
}