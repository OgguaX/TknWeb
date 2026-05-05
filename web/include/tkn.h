#pragma once
#include <stdint.h>

extern void *tknCreateGfxContextPtr(void *pTknInstance, void *pTknSurface, uint32_t width, uint32_t height, uint32_t globalShaderPathCount, const char **globalShaderPaths);
extern void tknDestroyGfxContext(void *pTknGfxContext);
extern void *tknCreateRenderPass(void *pTknGfxContext, uint32_t tknAttachmentCount, void **tknAttachmentPtrs, void **tknAttachmentDescriptions, void *vkClearValues, uint32_t shaderPathCount, const char **shaderPaths);
extern void tknDestroyRenderPass(void *pTknRenderPass);
extern void tknRenderFrame(void *pTknGfxContext);
