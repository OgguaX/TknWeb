#include "tknGfx.h"
#include "tknCore.h"

void *tknCreateRenderPassBindingGroupLayout(uint32_t shaderPathCount, const char **shaderPaths)
{
    return tknCreateBindingGroupLayout(shaderPathCount, shaderPaths, TKN_RENDERPASS_DESCRIPTOR_SET);
}

void tknDestroyRenderPassBindingGroupLayout(void *pTknRenderPassBindingGroupLayout)
{
    tknDestroyBindingGroupLayout(pTknRenderPassBindingGroupLayout);
}

static void tknConvertClearColor(VkFormat format, const double in[4], VkClearColorValue *pOut)
{
    switch (format)
    {
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R64_SINT:
    case VK_FORMAT_R64G64_SINT:
    case VK_FORMAT_R64G64B64_SINT:
    case VK_FORMAT_R64G64B64A64_SINT:
        for (int i = 0; i < 4; i++)
            pOut->int32[i] = (int32_t)in[i];
        break;
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R64_UINT:
    case VK_FORMAT_R64G64_UINT:
    case VK_FORMAT_R64G64B64_UINT:
    case VK_FORMAT_R64G64B64A64_UINT:
        for (int i = 0; i < 4; i++)
            pOut->uint32[i] = (uint32_t)in[i];
        break;
    default:
        for (int i = 0; i < 4; i++)
            pOut->float32[i] = (float)in[i];
        break;
    }
}

static void tknTransitionImageLayout(VkCommandBuffer vkCommandBuffer,
                                     VkImage vkImage,
                                     VkImageAspectFlags aspectMask,
                                     VkImageLayout oldLayout,
                                     VkImageLayout newLayout,
                                     VkPipelineStageFlags srcStage,
                                     VkPipelineStageFlags dstStage,
                                     VkAccessFlags srcAccess,
                                     VkAccessFlags dstAccess)
{
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = srcAccess,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = vkImage,
        .subresourceRange = {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = VK_REMAINING_MIP_LEVELS,
            .baseArrayLayer = 0,
            .layerCount = VK_REMAINING_ARRAY_LAYERS,
        },
    };
    vkCmdPipelineBarrier(vkCommandBuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

void tknBeginRenderPass(void *pTknGfxContext, void *pVkCommandBuffer, uint32_t colorAttachmentCount, void **colorImageViewPtrs, const int *loadOps, const int *storeOps, const double (*colorClearValues)[4], void *pTknDepthImageView, int depthLoadOp, int depthStoreOp, float depthClearValue, uint32_t stencilClearValue, uint32_t width, uint32_t height)
{
    (void)pTknGfxContext;
    VkCommandBuffer vkCommandBuffer = (VkCommandBuffer)pVkCommandBuffer;
    TknImageView *pDepthView = (TknImageView *)pTknDepthImageView;
    VkExtent2D extent = {.width = width, .height = height};

    for (uint32_t i = 0; i < colorAttachmentCount; i++)
    {
        TknImageView *pColorView = (TknImageView *)colorImageViewPtrs[i];
        tknTransitionImageLayout(vkCommandBuffer,
                                 pColorView->pTknImage->vkImage,
                                 VK_IMAGE_ASPECT_COLOR_BIT,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0,
                                 VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
    }

    if (pDepthView != NULL)
    {
        tknTransitionImageLayout(vkCommandBuffer,
                                 pDepthView->pTknImage->vkImage,
                                 VK_IMAGE_ASPECT_DEPTH_BIT,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                 0,
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    }

    VkRenderingAttachmentInfo *pColorInfos = NULL;
    if (colorAttachmentCount > 0)
    {
        pColorInfos = (VkRenderingAttachmentInfo *)tknMalloc(sizeof(VkRenderingAttachmentInfo) * colorAttachmentCount);
        for (uint32_t i = 0; i < colorAttachmentCount; i++)
        {
            TknImageView *pColorView = (TknImageView *)colorImageViewPtrs[i];
            VkClearColorValue clearColor;
            tknConvertClearColor(pColorView->pTknImage->vkFormat, colorClearValues[i], &clearColor);
            pColorInfos[i] = (VkRenderingAttachmentInfo){
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = NULL,
                .imageView = pColorView->vkImageView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = (VkAttachmentLoadOp)loadOps[i],
                .storeOp = (VkAttachmentStoreOp)storeOps[i],
                .clearValue = {.color = clearColor},
            };
        }
    }

    VkRenderingAttachmentInfo depthInfo;
    VkRenderingAttachmentInfo *pDepthInfo = NULL;
    if (pDepthView != NULL)
    {
        depthInfo = (VkRenderingAttachmentInfo){
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = NULL,
            .imageView = pDepthView->vkImageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = (VkAttachmentLoadOp)depthLoadOp,
            .storeOp = (VkAttachmentStoreOp)depthStoreOp,
            .clearValue = {.depthStencil = {.depth = depthClearValue, .stencil = stencilClearValue}},
        };
        pDepthInfo = &depthInfo;
    }

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderArea = {.offset = {0, 0}, .extent = extent},
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = colorAttachmentCount,
        .pColorAttachments = pColorInfos,
        .pDepthAttachment = pDepthInfo,
        .pStencilAttachment = NULL,
    };
    vkCmdBeginRendering(vkCommandBuffer, &renderingInfo);

    if (pColorInfos != NULL)
    {
        tknFree(pColorInfos);
    }

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
}

void tknEndRenderPass(void *pTknGfxContext,
                      void *pVkCommandBuffer)
{
    (void)pTknGfxContext;
    vkCmdEndRendering((VkCommandBuffer)pVkCommandBuffer);
}
