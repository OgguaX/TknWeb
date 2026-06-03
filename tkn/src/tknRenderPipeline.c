
#include "tknGfx.h"

static uint32_t tknGetVkFormatSize(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R32_SFLOAT:
        return 4;
    case VK_FORMAT_R32G32_SFLOAT:
        return 8;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 12;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;
    case VK_FORMAT_R32_SINT:
        return 4;
    case VK_FORMAT_R32G32_SINT:
        return 8;
    case VK_FORMAT_R32G32B32_SINT:
        return 12;
    case VK_FORMAT_R32G32B32A32_SINT:
        return 16;
    case VK_FORMAT_R32_UINT:
        return 4;
    case VK_FORMAT_R32G32_UINT:
        return 8;
    case VK_FORMAT_R32G32B32_UINT:
        return 12;
    case VK_FORMAT_R32G32B32A32_UINT:
        return 16;
    default:
        return 0;
    }
}

static uint32_t tknComputeStride(const TknVertexInputLayout *pLayout)
{
    uint32_t stride = 0;
    for (uint32_t i = 0; i < pLayout->vkVertexInputAttributeDescriptionCount; i++)
    {
        uint32_t end = pLayout->vkVertexInputAttributeDescriptions[i].offset + tknGetVkFormatSize(pLayout->vkVertexInputAttributeDescriptions[i].format);
        if (end > stride)
            stride = end;
    }
    return stride;
}

void *tknCreatePipelinePtr(void *pTknGfxContext, uint32_t colorAttachmentCount, const int *pColorAttachmentFormats, int depthAttachmentFormat, void *pTknRenderPassBindingGroupLayout, uint32_t spvPathCount, const char **spvPaths, void *pTknMeshVertexInputLayout, void *pTknInstanceVertexInputLayout, void *pVkPipelineInputAssemblyStateCreateInfo, void *pVkPipelineViewportStateCreateInfo, void *pVkPipelineRasterizationStateCreateInfo, void *pVkPipelineMultisampleStateCreateInfo, void *pVkPipelineDepthStencilStateCreateInfo, void *pVkPipelineColorBlendStateCreateInfo, void *pVkPipelineDynamicStateCreateInfo)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknVertexInputLayout *pMeshLayout = (TknVertexInputLayout *)pTknMeshVertexInputLayout;
    TknVertexInputLayout *pInstanceLayout = (TknVertexInputLayout *)pTknInstanceVertexInputLayout;
    TknBindingGroupLayout *pRenderPassBindingGroupLayout = (TknBindingGroupLayout *)pTknRenderPassBindingGroupLayout;

    // Create pipeline binding group layout from shaders
    void *pTknPipelineLayout = tknCreateBindingGroupLayout(pGfxContext, spvPathCount, spvPaths, TKN_PIPELINE_DESCRIPTOR_SET);
    TknBindingGroupLayout *pPipelineGroupLayout = (TknBindingGroupLayout *)pTknPipelineLayout;

    // Get shader modules
    VkShaderModule *vkShaderModules = tknMalloc(sizeof(VkShaderModule) * pPipelineGroupLayout->shaderPathCount);
    VkPipelineShaderStageCreateInfo *pStages = tknMalloc(sizeof(VkPipelineShaderStageCreateInfo) * pPipelineGroupLayout->shaderPathCount);

    for (uint32_t i = 0; i < pPipelineGroupLayout->shaderPathCount; i++)
    {
        SpvReflectShaderModule spvModule = pPipelineGroupLayout->pSpvReflectShaderModules[i];

        VkShaderModuleCreateInfo vkShaderModuleCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spvModule._internal->spirv_size,
            .pCode = spvModule._internal->spirv_code,
        };
        tknAssertVkResult(vkCreateShaderModule(pGfxContext->vkDevice, &vkShaderModuleCreateInfo, NULL, &vkShaderModules[i]));
        pStages[i] = (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = (VkShaderStageFlagBits)spvModule.shader_stage,
            .module = vkShaderModules[i],
            .pName = "main",
        };
    }

    // Build vertex input state
    TknVertexInputLayout layouts[TKN_MAX_VERTEX_BINDING_DESCRIPTION] = {
        pMeshLayout != NULL ? *pMeshLayout : (TknVertexInputLayout){0},
        pInstanceLayout != NULL ? *pInstanceLayout : (TknVertexInputLayout){0},
    };
    VkVertexInputRate inputRates[TKN_MAX_VERTEX_BINDING_DESCRIPTION] = {VK_VERTEX_INPUT_RATE_VERTEX, VK_VERTEX_INPUT_RATE_INSTANCE};

    VkVertexInputBindingDescription vkBindings[TKN_MAX_VERTEX_BINDING_DESCRIPTION];
    uint32_t bindingCount = 0;
    uint32_t totalAttrCount = 0;
    for (uint32_t i = 0; i < TKN_MAX_VERTEX_BINDING_DESCRIPTION; i++)
    {
        if (layouts[i].vkVertexInputAttributeDescriptionCount > 0)
        {
            vkBindings[bindingCount++] = (VkVertexInputBindingDescription){
                .binding = (uint32_t)layouts[i].tknVertexBinding,
                .stride = tknComputeStride(&layouts[i]),
                .inputRate = inputRates[i],
            };
            totalAttrCount += layouts[i].vkVertexInputAttributeDescriptionCount;
        }
    }

    VkVertexInputAttributeDescription *vkVertexInputAttributeDescriptions = NULL;
    if (totalAttrCount > 0)
    {
        vkVertexInputAttributeDescriptions = tknMalloc(sizeof(VkVertexInputAttributeDescription) * totalAttrCount);
        uint32_t attrOffset = 0;
        for (uint32_t i = 0; i < TKN_MAX_VERTEX_BINDING_DESCRIPTION; i++)
        {
            if (layouts[i].vkVertexInputAttributeDescriptionCount > 0 && layouts[i].vkVertexInputAttributeDescriptions != NULL)
            {
                memcpy(vkVertexInputAttributeDescriptions + attrOffset, layouts[i].vkVertexInputAttributeDescriptions,
                       sizeof(VkVertexInputAttributeDescription) * layouts[i].vkVertexInputAttributeDescriptionCount);
                attrOffset += layouts[i].vkVertexInputAttributeDescriptionCount;
            }
        }
    }

    VkPipelineVertexInputStateCreateInfo vkVertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = bindingCount,
        .pVertexBindingDescriptions = vkBindings,
        .vertexAttributeDescriptionCount = totalAttrCount,
        .pVertexAttributeDescriptions = vkVertexInputAttributeDescriptions,
    };

    // Pipeline layout with all three descriptor set layouts
    VkDescriptorSetLayout vkSetLayouts[TKN_MAX_DESCRIPTOR_SET] = {0};
    vkSetLayouts[TKN_GLOBAL_DESCRIPTOR_SET] = pGfxContext->pTknGlobalBindingGroupLayout->vkDescriptorSetLayout;
    vkSetLayouts[TKN_RENDERPASS_DESCRIPTOR_SET] = pRenderPassBindingGroupLayout->vkDescriptorSetLayout;
    vkSetLayouts[TKN_PIPELINE_DESCRIPTOR_SET] = pPipelineGroupLayout->vkDescriptorSetLayout;

    VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = TKN_MAX_DESCRIPTOR_SET,
        .pSetLayouts = vkSetLayouts,
    };
    VkPipelineLayout vkPipelineLayout;
    tknAssertVkResult(vkCreatePipelineLayout(pGfxContext->vkDevice, &vkPipelineLayoutCreateInfo, NULL, &vkPipelineLayout));

    // Dynamic rendering
    VkPipelineRenderingCreateInfoKHR vkRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = colorAttachmentCount,
        .pColorAttachmentFormats = (const VkFormat *)pColorAttachmentFormats,
        .depthAttachmentFormat = (VkFormat)depthAttachmentFormat,
    };

    // Create graphics pipeline
    VkGraphicsPipelineCreateInfo vkPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &vkRenderingCreateInfo,
        .stageCount = pPipelineGroupLayout->shaderPathCount,
        .pStages = pStages,
        .pVertexInputState = &vkVertexInputState,
        .pInputAssemblyState = (const VkPipelineInputAssemblyStateCreateInfo *)pVkPipelineInputAssemblyStateCreateInfo,
        .pViewportState = (const VkPipelineViewportStateCreateInfo *)pVkPipelineViewportStateCreateInfo,
        .pRasterizationState = (const VkPipelineRasterizationStateCreateInfo *)pVkPipelineRasterizationStateCreateInfo,
        .pMultisampleState = (const VkPipelineMultisampleStateCreateInfo *)pVkPipelineMultisampleStateCreateInfo,
        .pDepthStencilState = (const VkPipelineDepthStencilStateCreateInfo *)pVkPipelineDepthStencilStateCreateInfo,
        .pColorBlendState = (const VkPipelineColorBlendStateCreateInfo *)pVkPipelineColorBlendStateCreateInfo,
        .pDynamicState = (const VkPipelineDynamicStateCreateInfo *)pVkPipelineDynamicStateCreateInfo,
        .layout = vkPipelineLayout,
        .renderPass = VK_NULL_HANDLE,
    };
    VkPipeline vkPipeline;
    tknAssertVkResult(vkCreateGraphicsPipelines(pGfxContext->vkDevice, VK_NULL_HANDLE, 1, &vkPipelineCreateInfo, NULL, &vkPipeline));

    // Cleanup temporary resources
    for (uint32_t i = 0; i < pPipelineGroupLayout->shaderPathCount; i++)
        vkDestroyShaderModule(pGfxContext->vkDevice, vkShaderModules[i], NULL);
    tknFree(vkShaderModules);
    tknFree(pStages);
    if (vkVertexInputAttributeDescriptions != NULL)
        tknFree(vkVertexInputAttributeDescriptions);

    TknPipeline *pTknPipeline = tknMalloc(sizeof(TknPipeline));
    *pTknPipeline = (TknPipeline){
        .vkPipeline = vkPipeline,
        .vkPipelineLayout = vkPipelineLayout,
        .pTknPipelineBindingGroupLayout = pPipelineGroupLayout,
        .tknVertexInputLayouts = {layouts[0], layouts[1]},
    };
    return pTknPipeline;
}

void tknDestroyPipelinePtr(void *pTknGfxContext, void *pTknPipeline)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknPipeline *pPipeline = (TknPipeline *)pTknPipeline;

    vkDestroyPipeline(pGfxContext->vkDevice, pPipeline->vkPipeline, NULL);
    vkDestroyPipelineLayout(pGfxContext->vkDevice, pPipeline->vkPipelineLayout, NULL);

    // Destroy cached binding group layout
    if (pPipeline->pTknPipelineBindingGroupLayout != NULL)
    {
        tknDestroyBindingGroupLayout(pGfxContext, pPipeline->pTknPipelineBindingGroupLayout);
        pPipeline->pTknPipelineBindingGroupLayout = NULL;
    }

    tknFree(pPipeline);
}

void tknSetPipelinePtr(void *pTknGfxContext, void *pTknPipeline, void *pTknRenderPassBindingGroup, void *pTknPipelineBindingGroup)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknPipeline *pPipeline = (TknPipeline *)pTknPipeline;
    TknBindingGroup *pRenderPassBindingGroup = (TknBindingGroup *)pTknRenderPassBindingGroup;
    TknBindingGroup *pPipelineBindingGroup = (TknBindingGroup *)pTknPipelineBindingGroup;

    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    VkDescriptorSet vkDescriptorSets[TKN_MAX_DESCRIPTOR_SET] = {
        pGfxContext->pTknGlobalBindingGroup->vkDescriptorSet,
        pRenderPassBindingGroup != NULL ? pRenderPassBindingGroup->vkDescriptorSet : VK_NULL_HANDLE,
        pPipelineBindingGroup != NULL ? pPipelineBindingGroup->vkDescriptorSet : VK_NULL_HANDLE,
    };

    vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->vkPipelineLayout, 0, TKN_MAX_DESCRIPTOR_SET, vkDescriptorSets, 0, NULL);
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->vkPipeline);
}
