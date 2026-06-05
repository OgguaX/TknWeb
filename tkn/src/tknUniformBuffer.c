#include "tkn.h"
#include "tknGfx.h"

void *tknCreateUniformBuffer(void *pTknBuffer, uint64_t offset, uint64_t range)
{
    tknAssert(pTknBuffer != NULL, "Buffer cannot be NULL");

    TknBuffer *pBuffer = (TknBuffer *)pTknBuffer;
    tknAssert(offset < pBuffer->size, "Uniform buffer offset out of bounds");

    VkDeviceSize vkOffset = (VkDeviceSize)offset;
    VkDeviceSize vkRange = (range == 0) ? (VkDeviceSize)(pBuffer->size - offset) : (VkDeviceSize)range;
    tknAssert(vkRange > 0, "Uniform buffer range must be greater than 0");
    tknAssert(vkOffset + vkRange <= (VkDeviceSize)pBuffer->size, "Uniform buffer range out of bounds");

    TknUniformBuffer *pUniformBuffer = (TknUniformBuffer *)tknMalloc(sizeof(TknUniformBuffer));
    pUniformBuffer->pTknBuffer = pBuffer;
    pUniformBuffer->offset = vkOffset;
    pUniformBuffer->range = vkRange;
    pUniformBuffer->tknBindingGroupPtrHashSet = tknCreateHashSet(sizeof(void *));

    return (void *)pUniformBuffer;
}

void tknDestroyUniformBuffer(void *pTknUniformBuffer)
{
    if (pTknUniformBuffer == NULL)
    {
        tknWarning("Attempting to destroy NULL uniform buffer");
        return;
    }

    TknUniformBuffer *pUniformBuffer = (TknUniformBuffer *)pTknUniformBuffer;
    tknAssert(pUniformBuffer->tknBindingGroupPtrHashSet.count == 0, "Cannot destroy uniform buffer while it is still referenced by binding groups");
    tknDestroyHashSet(pUniformBuffer->tknBindingGroupPtrHashSet);

    pUniformBuffer->pTknBuffer = NULL;
    pUniformBuffer->offset = 0;
    pUniformBuffer->range = 0;

    tknFree(pUniformBuffer);
}
