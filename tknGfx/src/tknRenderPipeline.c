
#include "tknGfx.h"
#include "tknGfxInternal.h"

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

static uint32_t tknComputeStride(const VkVertexInputAttributeDescription *pAttrs, uint32_t attrCount)
{
    uint32_t stride = 0;
    for (uint32_t i = 0; i < attrCount; i++)
    {
        uint32_t end = pAttrs[i].offset + tknGetVkFormatSize(pAttrs[i].format);
        if (end > stride)
            stride = end;
    }
    return stride;
}

void *tknCreatePipelinePtr(void *pTknGfxContext,
                           void *pTknRenderPassBindingGroupLayout,
                           uint32_t spvPathCount,
                           const char **spvPaths,
                           void *pTknMeshVertexInputLayoutPtr,
                           void *pTknInstanceVertexInputLayoutPtr,
                           int topology,
                           int polygonMode,
                           int cullMode,
                           int frontFace,
                           bool depthBiasEnable,
                           float depthBiasConstantFactor,
                           float depthBiasClamp,
                           float depthBiasSlopeFactor,
                           float lineWidth,
                           uint32_t colorAttachmentCount,
                           int *pColorAttachmentFormats,
                           uint32_t attachmentCount,
                           TknPipelineColorBlendAttachmentState *attachments,
                           float blendConstants[4],
                           int rasterizationSamples,
                           bool alphaToCoverageEnable,
                           bool depthTestEnable,
                           bool depthWriteEnable,
                           int depthCompareOp,
                           bool stencilTestEnable,
                           TknStencilOpState front,
                           TknStencilOpState back,
                           int depthAttachmentFormat)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBindingGroupLayout *pRenderPassBindingGroupLayout = (TknBindingGroupLayout *)pTknRenderPassBindingGroupLayout;
    TknVertexInputLayout *pMeshVertexInputLayout = (TknVertexInputLayout *)pTknMeshVertexInputLayoutPtr;
    TknVertexInputLayout *pInstanceVertexInputLayout = (TknVertexInputLayout *)pTknInstanceVertexInputLayoutPtr;

    // Extract vertex attributes from vertex state
    uint32_t vkMeshVertexInputAttributeDescriptionCount =
        pMeshVertexInputLayout != NULL ? pMeshVertexInputLayout->tknVertexInputAttributeDescriptionCount : 0;
    uint32_t vkInstanceVertexInputAttributeDescriptionCount =
        pInstanceVertexInputLayout != NULL ? pInstanceVertexInputLayout->tknVertexInputAttributeDescriptionCount : 0;

    // Convert mesh vertex attributes to Vulkan format
    VkVertexInputAttributeDescription *pMeshAttrs = NULL;
    if (vkMeshVertexInputAttributeDescriptionCount > 0 && pMeshVertexInputLayout != NULL && pMeshVertexInputLayout->tknVertexInputAttributeDescriptions != NULL)
    {
        pMeshAttrs = tknMalloc(sizeof(VkVertexInputAttributeDescription) * vkMeshVertexInputAttributeDescriptionCount);
        for (uint32_t i = 0; i < vkMeshVertexInputAttributeDescriptionCount; i++)
        {
            pMeshAttrs[i] = (VkVertexInputAttributeDescription){
                .location = pMeshVertexInputLayout->tknVertexInputAttributeDescriptions[i].location,
                .binding = TKN_VERTEX_BINDING_DESCRIPTION,
                .format = (VkFormat)pMeshVertexInputLayout->tknVertexInputAttributeDescriptions[i].format,
                .offset = pMeshVertexInputLayout->tknVertexInputAttributeDescriptions[i].offset,
            };
        }
    }

    // Convert instance vertex attributes to Vulkan format
    VkVertexInputAttributeDescription *pInstanceAttrs = NULL;
    if (vkInstanceVertexInputAttributeDescriptionCount > 0 && pInstanceVertexInputLayout != NULL && pInstanceVertexInputLayout->tknVertexInputAttributeDescriptions != NULL)
    {
        pInstanceAttrs = tknMalloc(sizeof(VkVertexInputAttributeDescription) * vkInstanceVertexInputAttributeDescriptionCount);
        for (uint32_t i = 0; i < vkInstanceVertexInputAttributeDescriptionCount; i++)
        {
            pInstanceAttrs[i] = (VkVertexInputAttributeDescription){
                .location = pInstanceVertexInputLayout->tknVertexInputAttributeDescriptions[i].location,
                .binding = TKN_INSTANCE_BINDING_DESCRIPTION,
                .format = (VkFormat)pInstanceVertexInputLayout->tknVertexInputAttributeDescriptions[i].format,
                .offset = pInstanceVertexInputLayout->tknVertexInputAttributeDescriptions[i].offset,
            };
        }
    }

    // Create pipeline binding group layout from shaders
    void *pTknPipelineLayout = tknCreateBindingGroupLayout(pGfxContext, spvPathCount, spvPaths, TKN_PIPELINE_DESCRIPTOR_SET);
    TknBindingGroupLayout *pPipelineGroupLayout = (TknBindingGroupLayout *)pTknPipelineLayout;

    // Convert primitive/rasterization state
    VkPipelineInputAssemblyStateCreateInfo vkInputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = (VkPrimitiveTopology)topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineRasterizationStateCreateInfo vkRasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = (VkPolygonMode)polygonMode,
        .cullMode = (VkCullModeFlags)cullMode,
        .frontFace = (VkFrontFace)frontFace,
        .depthBiasEnable = depthBiasEnable,
        .depthBiasConstantFactor = depthBiasConstantFactor,
        .depthBiasClamp = depthBiasClamp,
        .depthBiasSlopeFactor = depthBiasSlopeFactor,
        .lineWidth = lineWidth,
    };

    // Viewport and scissor are set at runtime, not in pipeline
    // Default to NULL - will be set via tknSetViewport/tknSetScissor
    VkPipelineViewportStateCreateInfo vkViewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = NULL, // Will be dynamically set
        .scissorCount = 1,
        .pScissors = NULL, // Will be dynamically set
    };

    // Convert multisample state
    VkPipelineMultisampleStateCreateInfo vkMultisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = (VkSampleCountFlagBits)rasterizationSamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .alphaToCoverageEnable = alphaToCoverageEnable,
        .alphaToOneEnable = VK_FALSE,
    };

    // Convert depth stencil state
    VkPipelineDepthStencilStateCreateInfo vkDepthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = depthTestEnable,
        .depthWriteEnable = depthWriteEnable,
        .depthCompareOp = (VkCompareOp)depthCompareOp,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = stencilTestEnable,
        .front = (VkStencilOpState){
            .failOp = (VkStencilOp)front.failOp,
            .passOp = (VkStencilOp)front.passOp,
            .depthFailOp = (VkStencilOp)front.depthFailOp,
            .compareOp = (VkCompareOp)front.compareOp,
            .compareMask = front.compareMask,
            .writeMask = front.writeMask,
            .reference = front.reference,
        },
        .back = (VkStencilOpState){
            .failOp = (VkStencilOp)back.failOp,
            .passOp = (VkStencilOp)back.passOp,
            .depthFailOp = (VkStencilOp)back.depthFailOp,
            .compareOp = (VkCompareOp)back.compareOp,
            .compareMask = back.compareMask,
            .writeMask = back.writeMask,
            .reference = back.reference,
        },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    // Convert color blend state
    VkPipelineColorBlendAttachmentState *pVkColorBlendAttachments = NULL;
    if (attachmentCount > 0 && attachments != NULL)
    {
        pVkColorBlendAttachments = tknMalloc(sizeof(VkPipelineColorBlendAttachmentState) * attachmentCount);
        for (uint32_t i = 0; i < attachmentCount; i++)
        {
            pVkColorBlendAttachments[i] = (VkPipelineColorBlendAttachmentState){
                .blendEnable = attachments[i].blendEnable,
                .srcColorBlendFactor = (VkBlendFactor)attachments[i].srcColorBlendFactor,
                .dstColorBlendFactor = (VkBlendFactor)attachments[i].dstColorBlendFactor,
                .colorBlendOp = (VkBlendOp)attachments[i].colorBlendOp,
                .srcAlphaBlendFactor = (VkBlendFactor)attachments[i].srcAlphaBlendFactor,
                .dstAlphaBlendFactor = (VkBlendFactor)attachments[i].dstAlphaBlendFactor,
                .alphaBlendOp = (VkBlendOp)attachments[i].alphaBlendOp,
                .colorWriteMask = (VkColorComponentFlags)attachments[i].colorWriteMask,
            };
        }
    }

    VkPipelineColorBlendStateCreateInfo vkColorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = attachmentCount,
        .pAttachments = pVkColorBlendAttachments,
    };
    // Copy blend constants (always present as fixed-size array in struct)
    for (int i = 0; i < 4; i++)
        vkColorBlendState.blendConstants[i] = blendConstants[i];

    // Dynamic state: viewport and scissor are set at render time
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo vkDynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

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

    // Merge mesh and instance attribute descriptions
    VkVertexInputAttributeDescription *vkVertexInputAttributeDescriptions = NULL;
    uint32_t vkVertexInputAttributeDescriptionCount = 0;

    if (vkMeshVertexInputAttributeDescriptionCount > 0 || vkInstanceVertexInputAttributeDescriptionCount > 0)
    {
        vkVertexInputAttributeDescriptionCount = vkMeshVertexInputAttributeDescriptionCount + vkInstanceVertexInputAttributeDescriptionCount;
        vkVertexInputAttributeDescriptions = tknMalloc(sizeof(VkVertexInputAttributeDescription) * vkVertexInputAttributeDescriptionCount);

        if (vkMeshVertexInputAttributeDescriptionCount > 0 && pMeshAttrs != NULL)
            memcpy(vkVertexInputAttributeDescriptions, pMeshAttrs, sizeof(VkVertexInputAttributeDescription) * vkMeshVertexInputAttributeDescriptionCount);

        if (vkInstanceVertexInputAttributeDescriptionCount > 0 && pInstanceAttrs != NULL)
            memcpy(vkVertexInputAttributeDescriptions + vkMeshVertexInputAttributeDescriptionCount, pInstanceAttrs,
                   sizeof(VkVertexInputAttributeDescription) * vkInstanceVertexInputAttributeDescriptionCount);
    }

    // Compute strides
    uint32_t vertexStride = tknComputeStride(pMeshAttrs, vkMeshVertexInputAttributeDescriptionCount);
    uint32_t instanceStride = tknComputeStride(pInstanceAttrs, vkInstanceVertexInputAttributeDescriptionCount);

    // Build vertex input state
    VkVertexInputBindingDescription vkBindingDescriptions[TKN_MAX_VERTEX_BINDING_DESCRIPTION] = {
        {
            .binding = TKN_VERTEX_BINDING_DESCRIPTION,
            .stride = vertexStride,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
        {
            .binding = TKN_INSTANCE_BINDING_DESCRIPTION,
            .stride = instanceStride,
            .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
        },
    };

    VkPipelineVertexInputStateCreateInfo vkVertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = TKN_MAX_VERTEX_BINDING_DESCRIPTION,
        .pVertexBindingDescriptions = vkBindingDescriptions,
        .vertexAttributeDescriptionCount = vkVertexInputAttributeDescriptionCount,
        .pVertexAttributeDescriptions = vkVertexInputAttributeDescriptions,
    };

    // Pipeline layout
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
        .pInputAssemblyState = &vkInputAssemblyState,
        .pViewportState = &vkViewportState,
        .pRasterizationState = &vkRasterizationState,
        .pMultisampleState = &vkMultisampleState,
        .pDepthStencilState = &vkDepthStencilState,
        .pColorBlendState = &vkColorBlendState,
        .pDynamicState = &vkDynamicState,
        .layout = vkPipelineLayout,
        .renderPass = VK_NULL_HANDLE,
    };
    VkPipeline vkPipeline;
    tknAssertVkResult(vkCreateGraphicsPipelines(pGfxContext->vkDevice, VK_NULL_HANDLE, 1, &vkPipelineCreateInfo, NULL, &vkPipeline));

    // Free temporary converted structures
    if (pVkColorBlendAttachments)
        tknFree(pVkColorBlendAttachments);
    if (vkVertexInputAttributeDescriptions != NULL)
        tknFree(vkVertexInputAttributeDescriptions);
    if (pMeshAttrs != NULL)
        tknFree(pMeshAttrs);
    if (pInstanceAttrs != NULL)
        tknFree(pInstanceAttrs);

    // Cleanup shader modules
    for (uint32_t i = 0; i < pPipelineGroupLayout->shaderPathCount; i++)
        vkDestroyShaderModule(pGfxContext->vkDevice, vkShaderModules[i], NULL);
    tknFree(vkShaderModules);
    tknFree(pStages);

    // Create cached vertex input attribute layout copies (storing original TknVertexInputAttributeLayout, not Vulkan format)
    TknVertexInputAttributeLayout *pMeshAttrCopy = NULL;
    TknVertexInputAttributeLayout *pInstanceAttrCopy = NULL;

    if (vkMeshVertexInputAttributeDescriptionCount > 0 && pMeshVertexInputLayout != NULL && pMeshVertexInputLayout->tknVertexInputAttributeDescriptions != NULL)
    {
        pMeshAttrCopy = tknMalloc(sizeof(TknVertexInputAttributeLayout) * vkMeshVertexInputAttributeDescriptionCount);
        memcpy(pMeshAttrCopy, pMeshVertexInputLayout->tknVertexInputAttributeDescriptions, sizeof(TknVertexInputAttributeLayout) * vkMeshVertexInputAttributeDescriptionCount);
    }

    if (vkInstanceVertexInputAttributeDescriptionCount > 0 && pInstanceVertexInputLayout != NULL && pInstanceVertexInputLayout->tknVertexInputAttributeDescriptions != NULL)
    {
        pInstanceAttrCopy = tknMalloc(sizeof(TknVertexInputAttributeLayout) * vkInstanceVertexInputAttributeDescriptionCount);
        memcpy(pInstanceAttrCopy, pInstanceVertexInputLayout->tknVertexInputAttributeDescriptions, sizeof(TknVertexInputAttributeLayout) * vkInstanceVertexInputAttributeDescriptionCount);
    }

    TknVertexInputLayout *pCachedMeshVertexInputLayout = tknMalloc(sizeof(TknVertexInputLayout));
    *pCachedMeshVertexInputLayout = (TknVertexInputLayout){
        .tknVertexBinding = TKN_VERTEX_BINDING_DESCRIPTION,
        .tknVertexInputAttributeDescriptionCount = vkMeshVertexInputAttributeDescriptionCount,
        .tknVertexInputAttributeDescriptions = pMeshAttrCopy,
    };

    TknVertexInputLayout *pCachedInstanceVertexInputLayout = tknMalloc(sizeof(TknVertexInputLayout));
    *pCachedInstanceVertexInputLayout = (TknVertexInputLayout){
        .tknVertexBinding = TKN_INSTANCE_BINDING_DESCRIPTION,
        .tknVertexInputAttributeDescriptionCount = vkInstanceVertexInputAttributeDescriptionCount,
        .tknVertexInputAttributeDescriptions = pInstanceAttrCopy,
    };

    TknPipeline *pTknPipeline = tknMalloc(sizeof(TknPipeline));
    *pTknPipeline = (TknPipeline){
        .vkPipeline = vkPipeline,
        .vkPipelineLayout = vkPipelineLayout,
        .pTknPipelineBindingGroupLayout = pPipelineGroupLayout,
        .tknVertexInputLayoutPtrs = {
            [TKN_VERTEX_BINDING_DESCRIPTION] = pCachedMeshVertexInputLayout,
            [TKN_INSTANCE_BINDING_DESCRIPTION] = pCachedInstanceVertexInputLayout,
        },
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

    // Destroy cached vertex input attribute descriptions for all vertex bindings.
    for (uint32_t i = 0; i < TKN_MAX_VERTEX_BINDING_DESCRIPTION; i++)
    {
        if (pPipeline->tknVertexInputLayoutPtrs[i] != NULL)
        {
            if (pPipeline->tknVertexInputLayoutPtrs[i]->tknVertexInputAttributeDescriptions != NULL)
            {
                tknFree(pPipeline->tknVertexInputLayoutPtrs[i]->tknVertexInputAttributeDescriptions);
                pPipeline->tknVertexInputLayoutPtrs[i]->tknVertexInputAttributeDescriptions = NULL;
            }
            tknFree(pPipeline->tknVertexInputLayoutPtrs[i]);
            pPipeline->tknVertexInputLayoutPtrs[i] = NULL;
        }
    }

    tknFree(pPipeline);
}

void tknSetViewport(void *pTknGfxContext, float x, float y, float width, float height, float minDepth, float maxDepth)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    VkViewport viewport = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .minDepth = minDepth,
        .maxDepth = maxDepth,
    };
    vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);
}

void tknSetScissor(void *pTknGfxContext, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    uint32_t frameIndex = pGfxContext->frameCount % pGfxContext->swapchainImageCount;
    VkCommandBuffer vkCommandBuffer = pGfxContext->vkGfxCommandBuffers[frameIndex];

    VkRect2D scissor = {
        .offset = {x, y},
        .extent = {width, height},
    };
    vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
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

void tknSetBindingGroupPtr(void *pTknGfxContext, void *pTknPipeline, void *pTknRenderPassBindingGroup, void *pTknPipelineBindingGroup)
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
