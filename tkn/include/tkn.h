#pragma once
#include <stdbool.h>
#include <stdint.h>

extern void *tknCreateGfxContextPtr(int extensionCount, const char **extensions, void *pSurface, int width, int height, int globalShaderPathCount, const char **globalShaderPaths);
extern void tknDestroyGfxContextPtr(void *pTknGfxContext);


extern void *tknCreateImagePtr(void *pTknGfxContext, int dimension, int format, int mipLevelCount, int sampleCount, int width, int height, int depth, int imageUsageFlags);
extern void tknDestroyImagePtr(void *pTknGfxContext, void *pTknImage);

extern void *tknCreateImageView(void *pTknGfxContext, int baseLayer, int layerCount, int aspectFlags, int baseMipLevel, int mipLevelCount, int dimension, int format, int imageUsageFlags, void *pTknImage);
extern void tknDestroyImageView(void *pTknGfxContext, void *pTknImageView);