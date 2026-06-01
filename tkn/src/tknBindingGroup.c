#include "tknGfx.h"
#include "tknCore.h"

void *tknCreateBindingGroupLayout(uint32_t shaderPathCount, const char **shaderPaths, uint32_t set)
{
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
    uint32_t *descriptorCounts = (uint32_t *)tknMalloc(sizeof(uint32_t) * bindingCount);
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
                    descriptorCounts[binding] = pBinding->count > 0 ? pBinding->count : 1;
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
        .descriptorCounts = descriptorCounts,
        .vkShaderStageFlags = vkShaderStageFlags,
        .bindingUsed = bindingUsed,
        .shaderPathCount = shaderPathCount,
        .shaderPaths = cachedShaderPaths,
        .pSpvReflectShaderModules = modules,
    };
    return pLayout;
}

void tknDestroyBindingGroupLayout(void *pTknBindingGroupLayout)
{
    TknBindingGroupLayout *pLayout = (TknBindingGroupLayout *)pTknBindingGroupLayout;
    tknFree(pLayout->vkDescriptorTypes);
    tknFree(pLayout->descriptorCounts);
    tknFree(pLayout->vkShaderStageFlags);
    tknFree(pLayout->bindingUsed);
    
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
    uint32_t *descriptorCounts = pLayout->descriptorCounts;
    VkShaderStageFlags *vkShaderStageFlags = pLayout->vkShaderStageFlags;
    bool *bindingUsed = pLayout->bindingUsed;

    // Build layout bindings and pool sizes (deduplicated by descriptor type) from merge table
    VkDescriptorSetLayoutBinding *layoutBindings = (VkDescriptorSetLayoutBinding *)tknMalloc(sizeof(VkDescriptorSetLayoutBinding) * usedBindingCount);
    VkDescriptorPoolSize *poolSizes = (VkDescriptorPoolSize *)tknMalloc(sizeof(VkDescriptorPoolSize) * usedBindingCount);
    uint32_t poolSizeCount = 0;

    uint32_t idx = 0;
    for (uint32_t i = 0; i < bindingCount; i++)
    {
        if (!bindingUsed[i])
            continue;
        layoutBindings[idx++] = (VkDescriptorSetLayoutBinding){
            .binding = i,
            .descriptorType = vkDescriptorTypes[i],
            .descriptorCount = descriptorCounts[i],
            .stageFlags = vkShaderStageFlags[i],
            .pImmutableSamplers = NULL,
        };

        // Merge into pool sizes, accumulating counts for the same type
        bool typeFound = false;
        for (uint32_t p = 0; p < poolSizeCount; p++)
        {
            if (poolSizes[p].type == vkDescriptorTypes[i])
            {
                poolSizes[p].descriptorCount += descriptorCounts[i];
                typeFound = true;
                break;
            }
        }
        if (!typeFound)
        {
            poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
                .type = vkDescriptorTypes[i],
                .descriptorCount = descriptorCounts[i],
            };
        }
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

    // Allocate VkDescriptorSet
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = vkDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vkDescriptorSetLayout,
    };
    VkDescriptorSet vkDescriptorSet;
    tknAssertVkResult(vkAllocateDescriptorSets(pGfxContext->vkDevice, &allocInfo, &vkDescriptorSet));

    // Write provided binding resourcePtrs into the descriptor set in a single
    // vkUpdateDescriptorSets call. resourcePtrs[binding] is the resource bound
    // at descriptor binding `binding`. Only image-view backed types are
    // implemented; per-type info arrays are sized to resourceCount (transient).
    if (resourceCount > 0)
    {
        VkWriteDescriptorSet *writes = (VkWriteDescriptorSet *)tknMalloc(sizeof(VkWriteDescriptorSet) * resourceCount);
        VkDescriptorImageInfo *imgInfos = (VkDescriptorImageInfo *)tknMalloc(sizeof(VkDescriptorImageInfo) * resourceCount);
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
                break;
            }
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                // TODO: image-class with sampler
                tknAssert(false, "binding %u: descriptor type %d not implemented yet", binding, type);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                // TODO: buffer-class (allocate VkDescriptorBufferInfo array alongside imgInfos)
                tknAssert(false, "binding %u: descriptor type %d not implemented yet", binding, type);
                break;
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
    }

    // Cleanup temporaries
    tknFree(layoutBindings);
    tknFree(poolSizes);

    // Return opaque TknBindingGroup
    TknBindingGroup *pBindingGroup = (TknBindingGroup *)tknMalloc(sizeof(TknBindingGroup));
    *pBindingGroup = (TknBindingGroup){
        .vkDescriptorSetLayout = vkDescriptorSetLayout,
        .vkDescriptorPool = vkDescriptorPool,
        .vkDescriptorSet = vkDescriptorSet,
    };
    return pBindingGroup;
}

void tknDestroyBindingGroup(void *pTknGfxContext, void *pTknBindingGroup)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBindingGroup *pBindingGroup = (TknBindingGroup *)pTknBindingGroup;
    // Destroying the pool implicitly frees the descriptor set
    vkDestroyDescriptorPool(pGfxContext->vkDevice, pBindingGroup->vkDescriptorPool, NULL);
    vkDestroyDescriptorSetLayout(pGfxContext->vkDevice, pBindingGroup->vkDescriptorSetLayout, NULL);
    tknFree(pBindingGroup);
}
