#include "tknGfx.h"
#include "tknCore.h"

void *tknCreateBindingGroupLayout(void *pTknGfxContext, uint32_t shaderPathCount, const char **shaderPaths, uint32_t set)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    
    // Load all shader modules once
    SpvReflectShaderModule *modules = (SpvReflectShaderModule *)tknMalloc(sizeof(SpvReflectShaderModule) * shaderPathCount);
    for (uint32_t s = 0; s < shaderPathCount; s++)
    {
        modules[s] = tknCreateSpvReflectShaderModule(shaderPaths[s]);
    }

    // Pass 1: find the highest binding across all shaders to size the merge table
    uint32_t maxBinding = 0;
    for (uint32_t s = 0; s < shaderPathCount; s++)
    {
        uint32_t setCount = 0;
        spvReflectEnumerateDescriptorSets(&modules[s], &setCount, NULL);
        SpvReflectDescriptorSet **ppSets = (SpvReflectDescriptorSet **)tknMalloc(sizeof(SpvReflectDescriptorSet *) * setCount);
        spvReflectEnumerateDescriptorSets(&modules[s], &setCount, ppSets);
        for (uint32_t i = 0; i < setCount; i++)
        {
            if (ppSets[i]->set != set)
            {
                continue;
            }
            for (uint32_t b = 0; b < ppSets[i]->binding_count; b++)
            {
                uint32_t binding = ppSets[i]->bindings[b]->binding;
                if (binding > maxBinding)
                {
                    maxBinding = binding;
                }
            }
            break;
        }
        tknFree(ppSets);
    }

    // Merged binding table indexed by binding
    uint32_t bindingCount = maxBinding + 1;
    VkDescriptorType *vkDescriptorTypes = (VkDescriptorType *)tknMalloc(sizeof(VkDescriptorType) * bindingCount);
    uint32_t *vkDescriptorCounts = (uint32_t *)tknMalloc(sizeof(uint32_t) * bindingCount);
    VkShaderStageFlags *vkShaderStageFlags = (VkShaderStageFlags *)tknMalloc(sizeof(VkShaderStageFlags) * bindingCount);
    bool *bindingUsed = (bool *)tknMalloc(sizeof(bool) * bindingCount);
    for (uint32_t i = 0; i < bindingCount; i++)
    {
        bindingUsed[i] = false;
        vkShaderStageFlags[i] = 0;
    }

    // Pass 2: fill merge table (reuses already-loaded modules)
    for (uint32_t s = 0; s < shaderPathCount; s++)
    {
        uint32_t setCount = 0;
        spvReflectEnumerateDescriptorSets(&modules[s], &setCount, NULL);
        SpvReflectDescriptorSet **ppSets = (SpvReflectDescriptorSet **)tknMalloc(sizeof(SpvReflectDescriptorSet *) * setCount);
        spvReflectEnumerateDescriptorSets(&modules[s], &setCount, ppSets);

        for (uint32_t i = 0; i < setCount; i++)
        {
            if (ppSets[i]->set != set)
            {
                continue;
            }
            SpvReflectDescriptorSet *pTargetSet = ppSets[i];
            VkShaderStageFlags stageFlag = (VkShaderStageFlags)modules[s].shader_stage;

            for (uint32_t b = 0; b < pTargetSet->binding_count; b++)
            {
                SpvReflectDescriptorBinding *pBinding = pTargetSet->bindings[b];
                uint32_t binding = pBinding->binding;

                if (!bindingUsed[binding])
                {
                    vkDescriptorTypes[binding] = (VkDescriptorType)pBinding->descriptor_type;
                    vkDescriptorCounts[binding] = pBinding->count > 0 ? pBinding->count : 1;
                    bindingUsed[binding] = true;
                }
                vkShaderStageFlags[binding] |= stageFlag;
            }
            break;
        }

        tknFree(ppSets);
    }

    // Count distinct bindings actually used
    uint32_t usedBindingCount = 0;
    for (uint32_t i = 0; i < bindingCount; i++)
    {
        if (bindingUsed[i])
            usedBindingCount++;
    }

    tknAssert(usedBindingCount > 0, "set=%d not found in any shader", set);

    // Build layout bindings from merge table
    VkDescriptorSetLayoutBinding *layoutBindings = (VkDescriptorSetLayoutBinding *)tknMalloc(sizeof(VkDescriptorSetLayoutBinding) * usedBindingCount);
    uint32_t idx = 0;
    for (uint32_t i = 0; i < bindingCount; i++)
    {
        if (!bindingUsed[i])
            continue;
        layoutBindings[idx++] = (VkDescriptorSetLayoutBinding){
            .binding = i,
            .descriptorType = vkDescriptorTypes[i],
            .descriptorCount = vkDescriptorCounts[i],
            .stageFlags = vkShaderStageFlags[i],
            .pImmutableSamplers = NULL,
        };
    }

    // Create VkDescriptorSetLayout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = usedBindingCount,
        .pBindings = layoutBindings,
    };
    VkDescriptorSetLayout vkDescriptorSetLayout;
    tknAssertVkResult(vkCreateDescriptorSetLayout(pGfxContext->vkDevice, &layoutInfo, NULL, &vkDescriptorSetLayout));
    tknFree(layoutBindings);

    // Copy shader paths for caching
    const char **cachedShaderPaths = (const char **)tknMalloc(sizeof(const char *) * shaderPathCount);
    for (uint32_t i = 0; i < shaderPathCount; i++)
    {
        cachedShaderPaths[i] = shaderPaths[i];
    }

    // Create and return layout
    TknBindingGroupLayout *pLayout = (TknBindingGroupLayout *)tknMalloc(sizeof(TknBindingGroupLayout));
    *pLayout = (TknBindingGroupLayout){
        .bindingCount = bindingCount,
        .usedBindingCount = usedBindingCount,
        .vkDescriptorTypes = vkDescriptorTypes,
        .vkDescriptorCounts = vkDescriptorCounts,
        .vkShaderStageFlags = vkShaderStageFlags,
        .tknBindingUsed = bindingUsed,
        .shaderPathCount = shaderPathCount,
        .shaderPaths = cachedShaderPaths,
        .pSpvReflectShaderModules = modules,
        .vkDescriptorSetLayout = vkDescriptorSetLayout,
    };
    return pLayout;
}
void tknDestroyBindingGroupLayout(void *pTknGfxContext, void *pTknBindingGroupLayout)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBindingGroupLayout *pLayout = (TknBindingGroupLayout *)pTknBindingGroupLayout;
    // Destroy the Vulkan descriptor set layout
    if (pLayout->vkDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(pGfxContext->vkDevice, pLayout->vkDescriptorSetLayout, NULL);
        pLayout->vkDescriptorSetLayout = VK_NULL_HANDLE;
    }
    tknFree(pLayout->vkDescriptorTypes);
    tknFree(pLayout->vkDescriptorCounts);
    tknFree(pLayout->vkShaderStageFlags);
    tknFree(pLayout->tknBindingUsed);
    
    // Destroy cached shader modules
    for (uint32_t s = 0; s < pLayout->shaderPathCount; s++)
    {
        tknDestroySpvReflectShaderModule(&pLayout->pSpvReflectShaderModules[s]);
    }
    tknFree(pLayout->pSpvReflectShaderModules);
    tknFree(pLayout->shaderPaths);
    
    tknFree(pLayout);
}

void *tknCreateBindingGroup(void *pTknGfxContext, void *pTknBindingGroupLayout, uint32_t resourceCount, void **resourcePtrs)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBindingGroupLayout *pLayout = (TknBindingGroupLayout *)pTknBindingGroupLayout;

    uint32_t bindingCount = pLayout->bindingCount;
    uint32_t usedBindingCount = pLayout->usedBindingCount;
    VkDescriptorType *vkDescriptorTypes = pLayout->vkDescriptorTypes;
    uint32_t *vkDescriptorCounts = pLayout->vkDescriptorCounts;
    VkShaderStageFlags *vkShaderStageFlags = pLayout->vkShaderStageFlags;
    bool *bindingUsed = pLayout->tknBindingUsed;
    VkDescriptorSetLayout vkDescriptorSetLayout = pLayout->vkDescriptorSetLayout;

    // Build pool sizes (deduplicated by descriptor type) from merge table
    VkDescriptorPoolSize *poolSizes = (VkDescriptorPoolSize *)tknMalloc(sizeof(VkDescriptorPoolSize) * usedBindingCount);
    uint32_t poolSizeCount = 0;

    for (uint32_t i = 0; i < bindingCount; i++)
    {
        if (!bindingUsed[i])
            continue;

        // Merge into pool sizes, accumulating counts for the same type
        bool typeFound = false;
        for (uint32_t p = 0; p < poolSizeCount; p++)
        {
            if (poolSizes[p].type == vkDescriptorTypes[i])
            {
                poolSizes[p].descriptorCount += vkDescriptorCounts[i];
                typeFound = true;
                break;
            }
        }
        if (!typeFound)
        {
            poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
                .type = vkDescriptorTypes[i],
                .descriptorCount = vkDescriptorCounts[i],
            };
        }
    }

    // Create VkDescriptorPool
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = poolSizeCount,
        .pPoolSizes = poolSizes,
    };
    VkDescriptorPool vkDescriptorPool;
    tknAssertVkResult(vkCreateDescriptorPool(pGfxContext->vkDevice, &poolInfo, NULL, &vkDescriptorPool));

    // Allocate VkDescriptorSet from layout
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = vkDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vkDescriptorSetLayout,
    };
    VkDescriptorSet vkDescriptorSet;
    tknAssertVkResult(vkAllocateDescriptorSets(pGfxContext->vkDevice, &allocInfo, &vkDescriptorSet));

    TknBindingGroup *pBindingGroup = (TknBindingGroup *)tknMalloc(sizeof(TknBindingGroup));
    void **tknBindingResourcePtrs = (void **)tknMalloc(sizeof(void *) * bindingCount);
    for (uint32_t i = 0; i < bindingCount; i++)
    {
        tknBindingResourcePtrs[i] = NULL;
    }

    // Write provided binding resourcePtrs into the descriptor set in a single
    // vkUpdateDescriptorSets call. resourcePtrs[binding] is the resource bound
    // at descriptor binding `binding`.
    if (resourceCount > 0)
    {
        VkWriteDescriptorSet *writes = (VkWriteDescriptorSet *)tknMalloc(sizeof(VkWriteDescriptorSet) * resourceCount);
        VkDescriptorImageInfo *imgInfos = (VkDescriptorImageInfo *)tknMalloc(sizeof(VkDescriptorImageInfo) * resourceCount);
        VkDescriptorBufferInfo *bufferInfos = (VkDescriptorBufferInfo *)tknMalloc(sizeof(VkDescriptorBufferInfo) * resourceCount);
        uint32_t writeCount = 0;

        for (uint32_t binding = 0; binding < resourceCount; binding++)
        {
            if (!bindingUsed[binding] || resourcePtrs[binding] == NULL)
            {
                continue;
            }
            VkDescriptorType type = vkDescriptorTypes[binding];
            uint32_t w = writeCount;

            writes[w] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = NULL,
                .dstSet = vkDescriptorSet,
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = type,
                .pImageInfo = NULL,
                .pBufferInfo = NULL,
                .pTexelBufferView = NULL,
            };

            switch (type)
            {
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            {
                TknImageView *pTknImageView = (TknImageView *)resourcePtrs[binding];
                imgInfos[w] = (VkDescriptorImageInfo){
                    .sampler = VK_NULL_HANDLE,
                    .imageView = pTknImageView->vkImageView,
                    .imageLayout = (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                                       ? VK_IMAGE_LAYOUT_GENERAL
                                       : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                writes[w].pImageInfo = &imgInfos[w];
                tknBindingResourcePtrs[binding] = resourcePtrs[binding];
                void *pBindingGroupPtr = pBindingGroup;
                tknAddToHashSet(&pTknImageView->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                break;
            }
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            {
                TknSampler *pTknSampler = (TknSampler *)resourcePtrs[binding];
                imgInfos[w] = (VkDescriptorImageInfo){
                    .sampler = pTknSampler->vkSampler,
                    .imageView = VK_NULL_HANDLE,
                    .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                };
                writes[w].pImageInfo = &imgInfos[w];
                tknBindingResourcePtrs[binding] = resourcePtrs[binding];
                void *pBindingGroupPtr = pBindingGroup;
                tknAddToHashSet(&pTknSampler->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                break;
            }
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                tknAssert(false, "binding %u: descriptor type %d not implemented yet", binding, type);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            {
                TknUniformBuffer *pTknUniformBuffer = (TknUniformBuffer *)resourcePtrs[binding];
                bufferInfos[w] = (VkDescriptorBufferInfo){
                    .buffer = pTknUniformBuffer->pTknBuffer->vkBuffer,
                    .offset = pTknUniformBuffer->offset,
                    .range = pTknUniformBuffer->range,
                };
                writes[w].pBufferInfo = &bufferInfos[w];
                tknBindingResourcePtrs[binding] = resourcePtrs[binding];
                void *pBindingGroupPtr = pBindingGroup;
                tknAddToHashSet(&pTknUniformBuffer->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                break;
            }
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                // TODO: texel-buffer-class (allocate VkBufferView array alongside imgInfos)
                tknAssert(false, "binding %u: descriptor type %d not implemented yet", binding, type);
                break;
            default:
                tknAssert(false, "binding %u: unknown descriptor type %d", binding, type);
                break;
            }
            writeCount++;
        }

        if (writeCount > 0)
        {
            vkUpdateDescriptorSets(pGfxContext->vkDevice, writeCount, writes, 0, NULL);
        }
        tknFree(writes);
        tknFree(imgInfos);
        tknFree(bufferInfos);
    }

    // Cleanup temporaries
    tknFree(poolSizes);

    // Return opaque TknBindingGroup with reference to layout
    *pBindingGroup = (TknBindingGroup){
        .pLayout = pLayout,
        .vkDescriptorPool = vkDescriptorPool,
        .vkDescriptorSet = vkDescriptorSet,
        .tknBindingResourceCount = bindingCount,
        .tknBindingResourcePtrs = tknBindingResourcePtrs,
    };
    return pBindingGroup;
}

void tknDestroyBindingGroup(void *pTknGfxContext, void *pTknBindingGroup)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBindingGroup *pBindingGroup = (TknBindingGroup *)pTknBindingGroup;
    for (uint32_t binding = 0; binding < pBindingGroup->tknBindingResourceCount; binding++)
    {
        void *resourcePtr = pBindingGroup->tknBindingResourcePtrs[binding];
        if (resourcePtr == NULL || !pBindingGroup->pLayout->tknBindingUsed[binding])
        {
            continue;
        }

        void *pBindingGroupPtr = pBindingGroup;
        switch (pBindingGroup->pLayout->vkDescriptorTypes[binding])
        {
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            TknImageView *pImageView = (TknImageView *)resourcePtr;
            tknRemoveFromHashSet(&pImageView->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
            break;
        }
        case VK_DESCRIPTOR_TYPE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        {
            TknSampler *pSampler = (TknSampler *)resourcePtr;
            tknRemoveFromHashSet(&pSampler->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
            break;
        }
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        {
            TknUniformBuffer *pUniformBuffer = (TknUniformBuffer *)resourcePtr;
            tknRemoveFromHashSet(&pUniformBuffer->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
            break;
        }
        default:
            break;
        }
    }
    // Destroying the pool implicitly frees the descriptor set
    vkDestroyDescriptorPool(pGfxContext->vkDevice, pBindingGroup->vkDescriptorPool, NULL);
    // Note: Do NOT destroy vkDescriptorSetLayout here - it's owned by the layout
    tknFree(pBindingGroup->tknBindingResourcePtrs);
    tknFree(pBindingGroup);
}

void tknUpdateBindingGroup(void *pTknGfxContext, void *pTknBindingGroup, uint32_t resourceCount, uint32_t *indices, void **resourcePtrs)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBindingGroup *pBindingGroup = (TknBindingGroup *)pTknBindingGroup;
    TknBindingGroupLayout *pLayout = pBindingGroup->pLayout;

    uint32_t bindingCount = pLayout->bindingCount;
    VkDescriptorType *vkDescriptorTypes = pLayout->vkDescriptorTypes;
    VkShaderStageFlags *vkShaderStageFlags = pLayout->vkShaderStageFlags;
    bool *bindingUsed = pLayout->tknBindingUsed;
    VkDescriptorSet vkDescriptorSet = pBindingGroup->vkDescriptorSet;

    // Update provided binding resourcePtrs into the descriptor set in a single
    // vkUpdateDescriptorSets call. resourcePtrs[i] is the resource to bind
    // at descriptor binding indices[i].
    if (resourceCount > 0)
    {
        VkWriteDescriptorSet *writes = (VkWriteDescriptorSet *)tknMalloc(sizeof(VkWriteDescriptorSet) * resourceCount);
        VkDescriptorImageInfo *imgInfos = (VkDescriptorImageInfo *)tknMalloc(sizeof(VkDescriptorImageInfo) * resourceCount);
        VkDescriptorBufferInfo *bufferInfos = (VkDescriptorBufferInfo *)tknMalloc(sizeof(VkDescriptorBufferInfo) * resourceCount);
        uint32_t writeCount = 0;

        for (uint32_t i = 0; i < resourceCount; i++)
        {
            uint32_t binding = indices[i];
            
            if (binding >= bindingCount || !bindingUsed[binding] || resourcePtrs[i] == NULL)
            {
                continue;
            }
            VkDescriptorType type = vkDescriptorTypes[binding];
            void *oldResourcePtr = pBindingGroup->tknBindingResourcePtrs[binding];
            if (oldResourcePtr != NULL)
            {
                void *pBindingGroupPtr = pBindingGroup;
                switch (vkDescriptorTypes[binding])
                {
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                {
                    TknImageView *pImageView = (TknImageView *)oldResourcePtr;
                    tknRemoveFromHashSet(&pImageView->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                    break;
                }
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                {
                    TknSampler *pSampler = (TknSampler *)oldResourcePtr;
                    tknRemoveFromHashSet(&pSampler->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                    break;
                }
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                {
                    TknUniformBuffer *pUniformBuffer = (TknUniformBuffer *)oldResourcePtr;
                    tknRemoveFromHashSet(&pUniformBuffer->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                    break;
                }
                default:
                    break;
                }
            }
            uint32_t w = writeCount;

            writes[w] = (VkWriteDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = NULL,
                .dstSet = vkDescriptorSet,
                .dstBinding = binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = type,
                .pImageInfo = NULL,
                .pBufferInfo = NULL,
                .pTexelBufferView = NULL,
            };

            switch (type)
            {
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            {
                TknImageView *pTknImageView = (TknImageView *)resourcePtrs[i];
                imgInfos[w] = (VkDescriptorImageInfo){
                    .sampler = VK_NULL_HANDLE,
                    .imageView = pTknImageView->vkImageView,
                    .imageLayout = (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                                       ? VK_IMAGE_LAYOUT_GENERAL
                                       : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                };
                writes[w].pImageInfo = &imgInfos[w];
                pBindingGroup->tknBindingResourcePtrs[binding] = resourcePtrs[i];
                {
                    void *pBindingGroupPtr = pBindingGroup;
                    TknImageView *pImageView = (TknImageView *)resourcePtrs[i];
                    tknAddToHashSet(&pImageView->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                }
                break;
            }
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            {
                TknSampler *pTknSampler = (TknSampler *)resourcePtrs[i];
                imgInfos[w] = (VkDescriptorImageInfo){
                    .sampler = pTknSampler->vkSampler,
                    .imageView = VK_NULL_HANDLE,
                    .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                };
                writes[w].pImageInfo = &imgInfos[w];
                pBindingGroup->tknBindingResourcePtrs[binding] = resourcePtrs[i];
                {
                    void *pBindingGroupPtr = pBindingGroup;
                    TknSampler *pSampler = (TknSampler *)resourcePtrs[i];
                    tknAddToHashSet(&pSampler->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                }
                break;
            }
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                tknAssert(false, "binding %u: descriptor type %d not implemented yet", binding, type);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            {
                TknUniformBuffer *pTknUniformBuffer = (TknUniformBuffer *)resourcePtrs[i];
                bufferInfos[w] = (VkDescriptorBufferInfo){
                    .buffer = pTknUniformBuffer->pTknBuffer->vkBuffer,
                    .offset = pTknUniformBuffer->offset,
                    .range = pTknUniformBuffer->range,
                };
                writes[w].pBufferInfo = &bufferInfos[w];
                pBindingGroup->tknBindingResourcePtrs[binding] = resourcePtrs[i];
                {
                    void *pBindingGroupPtr = pBindingGroup;
                    TknUniformBuffer *pUniformBuffer = (TknUniformBuffer *)resourcePtrs[i];
                    tknAddToHashSet(&pUniformBuffer->tknBindingGroupPtrHashSet, &pBindingGroupPtr);
                }
                break;
            }
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                // TODO: texel-buffer-class (allocate VkBufferView array alongside imgInfos)
                tknAssert(false, "binding %u: descriptor type %d not implemented yet", binding, type);
                break;
            default:
                tknAssert(false, "binding %u: unknown descriptor type %d", binding, type);
                break;
            }
            writeCount++;
        }

        if (writeCount > 0)
        {
            vkUpdateDescriptorSets(pGfxContext->vkDevice, writeCount, writes, 0, NULL);
        }
        tknFree(writes);
        tknFree(imgInfos);
        tknFree(bufferInfos);
    }

}
