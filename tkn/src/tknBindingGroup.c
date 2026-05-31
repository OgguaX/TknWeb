#include "tknGfx.h"
#include "tknCore.h"

void *tknCreateBindingGroup(void *pTknGfxContext, uint32_t shaderPathCount, const char **shaderPaths, uint32_t set
)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    // Load all shader modules once
    SpvReflectShaderModule *modules = (SpvReflectShaderModule *)tknMalloc(sizeof(SpvReflectShaderModule) * shaderPathCount);
    for (uint32_t s = 0; s < shaderPathCount; s++)
    {
        modules[s] = tknCreateSpvReflectShaderModule(shaderPaths[s]);
    }

    // Pass 1: find the highest binding slot across all shaders to size the merge table
    uint32_t maxBindingSlot = 0;
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
                uint32_t slot = ppSets[i]->bindings[b]->binding;
                if (slot > maxBindingSlot)
                {
                    maxBindingSlot = slot;
                }
            }
            break;
        }
        tknFree(ppSets);
    }

    // Merged binding table indexed by binding slot
    uint32_t tableSize = maxBindingSlot + 1;
    VkDescriptorType *mergedTypes = (VkDescriptorType *)tknMalloc(sizeof(VkDescriptorType) * tableSize);
    uint32_t *mergedCounts = (uint32_t *)tknMalloc(sizeof(uint32_t) * tableSize);
    VkShaderStageFlags *mergedStages = (VkShaderStageFlags *)tknMalloc(sizeof(VkShaderStageFlags) * tableSize);
    bool *mergedUsed = (bool *)tknMalloc(sizeof(bool) * tableSize);
    for (uint32_t i = 0; i < tableSize; i++)
    {
        mergedUsed[i] = false;
        mergedStages[i] = 0;
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
                uint32_t slot = pBinding->binding;

                if (!mergedUsed[slot])
                {
                    mergedTypes[slot] = (VkDescriptorType)pBinding->descriptor_type;
                    mergedCounts[slot] = pBinding->count > 0 ? pBinding->count : 1;
                    mergedUsed[slot] = true;
                }
                mergedStages[slot] |= stageFlag;
            }
            break;
        }

        tknFree(ppSets);
    }

    // Destroy modules now that reflection is done
    for (uint32_t s = 0; s < shaderPathCount; s++)
    {
        tknDestroySpvReflectShaderModule(&modules[s]);
    }
    tknFree(modules);

    // Count distinct binding slots
    uint32_t bindingCount = 0;
    for (uint32_t i = 0; i < tableSize; i++)
    {
        if (mergedUsed[i])
            bindingCount++;
    }

    tknAssert(bindingCount > 0, "set=%d not found in any shader", set);

    // Build layout bindings and pool sizes (deduplicated by descriptor type) from merge table
    VkDescriptorSetLayoutBinding *layoutBindings = (VkDescriptorSetLayoutBinding *)tknMalloc(sizeof(VkDescriptorSetLayoutBinding) * bindingCount);
    VkDescriptorPoolSize *poolSizes = (VkDescriptorPoolSize *)tknMalloc(sizeof(VkDescriptorPoolSize) * bindingCount);
    uint32_t poolSizeCount = 0;

    uint32_t idx = 0;
    for (uint32_t i = 0; i < tableSize; i++)
    {
        if (!mergedUsed[i])
            continue;
        layoutBindings[idx++] = (VkDescriptorSetLayoutBinding){
            .binding = i,
            .descriptorType = mergedTypes[i],
            .descriptorCount = mergedCounts[i],
            .stageFlags = mergedStages[i],
            .pImmutableSamplers = NULL,
        };

        // Merge into pool sizes, accumulating counts for the same type
        bool typeFound = false;
        for (uint32_t p = 0; p < poolSizeCount; p++)
        {
            if (poolSizes[p].type == mergedTypes[i])
            {
                poolSizes[p].descriptorCount += mergedCounts[i];
                typeFound = true;
                break;
            }
        }
        if (!typeFound)
        {
            poolSizes[poolSizeCount++] = (VkDescriptorPoolSize){
                .type = mergedTypes[i],
                .descriptorCount = mergedCounts[i],
            };
        }
    }

    // Create VkDescriptorSetLayout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = bindingCount,
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

    // Cleanup temporaries
    tknFree(layoutBindings);
    tknFree(poolSizes);
    tknFree(mergedTypes);
    tknFree(mergedCounts);
    tknFree(mergedStages);
    tknFree(mergedUsed);

    // Return opaque TknBindingGroup
    TknBindingGroup *pBindingGroup = (TknBindingGroup *)tknMalloc(sizeof(TknBindingGroup));
    *pBindingGroup = (TknBindingGroup){
        .vkDescriptorSetLayout = vkDescriptorSetLayout,
        .vkDescriptorPool = vkDescriptorPool,
        .vkDescriptorSet = vkDescriptorSet,
        .tknDescriptorCount = bindingCount,
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

void tknUpdateBindingGroup(void *pTknGfxContext, void *pTknBindingGroup, uint32_t writeCount, void *pVkWriteDescriptorSets)
{
    TknGfxContext *pGfxContext = (TknGfxContext *)pTknGfxContext;
    TknBindingGroup *pBindingGroup = (TknBindingGroup *)pTknBindingGroup;
    VkWriteDescriptorSet *pWrites = (VkWriteDescriptorSet *)pVkWriteDescriptorSets;
    for (uint32_t i = 0; i < writeCount; i++)
    {
        pWrites[i].dstSet = pBindingGroup->vkDescriptorSet;
    }
    vkUpdateDescriptorSets(pGfxContext->vkDevice, writeCount, pWrites, 0, NULL);
}
