// Tickernel Graphics API Lua Binding Layer
// Provides Lua access to TickernelGfx graphics core functions

#include "tkn.h"
#include "tknFont.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// Graphics Context Functions
// ============================================================================

static int luaCreateGfxContextPtr(lua_State *L)
{
    uint32_t extensionCount = (uint32_t)luaL_checkinteger(L, 1);
    
    // Extract extensions table
    const char **extensions = NULL;
    if (lua_istable(L, 2) && extensionCount > 0)
    {
        extensions = tknMalloc(extensionCount * sizeof(const char *));
        for (uint32_t i = 0; i < extensionCount; i++)
        {
            lua_geti(L, 2, i + 1);
            extensions[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
    }
    
    void *pSurface = lua_touserdata(L, 3);
    uint32_t width = (uint32_t)luaL_checkinteger(L, 4);
    uint32_t height = (uint32_t)luaL_checkinteger(L, 5);
    uint32_t globalShaderPathCount = (uint32_t)luaL_checkinteger(L, 6);
    
    // Extract shader paths table
    const char **globalShaderPaths = NULL;
    if (lua_istable(L, 7) && globalShaderPathCount > 0)
    {
        globalShaderPaths = tknMalloc(globalShaderPathCount * sizeof(const char *));
        for (uint32_t i = 0; i < globalShaderPathCount; i++)
        {
            lua_geti(L, 7, i + 1);
            globalShaderPaths[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
    }
    
    void *pGfxContext = tknCreateGfxContextPtr(extensionCount, extensions, pSurface, width, height, globalShaderPathCount, globalShaderPaths);
    
    tknFree(extensions);
    tknFree(globalShaderPaths);
    
    lua_pushlightuserdata(L, pGfxContext);
    return 1;
}

static int luaDestroyGfxContextPtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    tknDestroyGfxContextPtr(pGfxContext);
    return 0;
}

// ============================================================================
// Image Functions
// ============================================================================

static int luaCreateImagePtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    int dimension = (int)luaL_checkinteger(L, 2);
    int format = (int)luaL_checkinteger(L, 3);
    uint32_t mipLevelCount = (uint32_t)luaL_checkinteger(L, 4);
    int sampleCount = (int)luaL_checkinteger(L, 5);
    uint32_t width = (uint32_t)luaL_checkinteger(L, 6);
    uint32_t height = (uint32_t)luaL_checkinteger(L, 7);
    uint32_t depth = (uint32_t)luaL_checkinteger(L, 8);
    int imageUsageFlags = (int)luaL_checkinteger(L, 9);
    
    void *pImage = tknCreateImagePtr(pGfxContext, dimension, format, mipLevelCount, sampleCount, width, height, depth, imageUsageFlags);
    lua_pushlightuserdata(L, pImage);
    return 1;
}

static int luaDestroyImagePtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pImage = lua_touserdata(L, 2);
    tknDestroyImagePtr(pGfxContext, pImage);
    return 0;
}

static int luaCreateImageView(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    uint32_t baseLayer = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t layerCount = (uint32_t)luaL_checkinteger(L, 3);
    int aspectFlags = (int)luaL_checkinteger(L, 4);
    uint32_t baseMipLevel = (uint32_t)luaL_checkinteger(L, 5);
    uint32_t mipLevelCount = (uint32_t)luaL_checkinteger(L, 6);
    int dimension = (int)luaL_checkinteger(L, 7);
    int format = (int)luaL_checkinteger(L, 8);
    void *pImage = lua_touserdata(L, 9);
    
    void *pImageView = tknCreateImageView(pGfxContext, baseLayer, layerCount, aspectFlags, baseMipLevel, mipLevelCount, dimension, format, pImage);
    lua_pushlightuserdata(L, pImageView);
    return 1;
}

static int luaDestroyImageView(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pImageView = lua_touserdata(L, 2);
    tknDestroyImageView(pGfxContext, pImageView);
    return 0;
}

static int luaWriteImagePtr(lua_State *L)
{
    // Parameter validation
    void *pGfxContext = lua_touserdata(L, 1);
    void *pImage = lua_touserdata(L, 2);
    
    if (!pGfxContext || !pImage)
    {
        lua_pushboolean(L, false);
        lua_pushstring(L, "Invalid graphics context or image pointer");
        return 2;
    }
    
    // Handle string data source for binary image data (ASTC, ETC2, etc.)
    const void *pData = NULL;
    uint64_t dataSize = 0;
    
    if (lua_type(L, 3) == LUA_TSTRING)
    {
        size_t strLen;
        pData = lua_tolstring(L, 3, &strLen);
        dataSize = (uint64_t)strLen;
        
        if (dataSize == 0)
        {
            lua_pushboolean(L, false);
            lua_pushstring(L, "Image data is empty");
            return 2;
        }
    }
    else
    {
        lua_pushboolean(L, false);
        lua_pushstring(L, "Invalid data type for tknWriteImagePtr: expected string");
        return 2;
    }
    
    uint32_t width = (uint32_t)luaL_checkinteger(L, 4);
    uint32_t height = (uint32_t)luaL_checkinteger(L, 5);
    uint32_t depth = (uint32_t)luaL_checkinteger(L, 6);
    uint32_t mipLevel = (uint32_t)luaL_checkinteger(L, 7);
    uint32_t offsetX = (uint32_t)luaL_checkinteger(L, 8);
    uint32_t offsetY = (uint32_t)luaL_checkinteger(L, 9);
    uint32_t offsetZ = (uint32_t)luaL_checkinteger(L, 10);
    
    // Validate dimensions
    if (width == 0 || height == 0 || depth == 0)
    {
        lua_pushboolean(L, false);
        lua_pushstring(L, "Image dimensions cannot be zero");
        return 2;
    }
    
    tknWriteImagePtr(pGfxContext, pImage, pData, dataSize, width, height, depth, mipLevel, offsetX, offsetY, offsetZ);
    
    lua_pushboolean(L, true);
    return 1;
}

// ============================================================================
// Buffer Functions
// ============================================================================

static int luaCreateUniformBuffer(lua_State *L)
{
    void *pBuffer = lua_touserdata(L, 1);
    uint64_t offset = (uint64_t)luaL_checkinteger(L, 2);
    uint64_t range = (uint64_t)luaL_checkinteger(L, 3);
    
    void *pUniformBuffer = tknCreateUniformBuffer(pBuffer, offset, range);
    lua_pushlightuserdata(L, pUniformBuffer);
    return 1;
}

static int luaDestroyUniformBuffer(lua_State *L)
{
    void *pUniformBuffer = lua_touserdata(L, 1);
    tknDestroyUniformBuffer(pUniformBuffer);
    return 0;
}

static int luaCreateBufferPtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    uint64_t size = (uint64_t)luaL_checkinteger(L, 2);
    int usage = (int)luaL_checkinteger(L, 3);
    bool mappedAtCreation = lua_toboolean(L, 4);
    
    // Handle string data source
    const void *pData = NULL;
    
    if (lua_type(L, 5) == LUA_TSTRING)
    {
        // If string, use string data directly
        size_t strLen;
        pData = lua_tolstring(L, 5, &strLen);
        if (strLen > size)
        {
            tknWarning("Lua string size (%zu) exceeds buffer size (%llu), truncating", strLen, size);
        }
    }
    else if (lua_type(L, 5) != LUA_TNIL && lua_type(L, 5) != LUA_TNONE)
    {
        tknWarning("Invalid data type for buffer: expected string or nil, got %s", lua_typename(L, lua_type(L, 5)));
    }
    
    void *pTknBuffer = tknCreateBufferPtr(pGfxContext, size, usage, mappedAtCreation, pData);
    lua_pushlightuserdata(L, pTknBuffer);
    return 1;
}

static int luaDestroyBufferPtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pTknBuffer = lua_touserdata(L, 2);
    tknDestroyBufferPtr(pGfxContext, pTknBuffer);
    return 0;
}

static int luaUpdateBuffer(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pTknBuffer = lua_touserdata(L, 2);
    uint64_t offset = (uint64_t)luaL_checkinteger(L, 3);
    uint64_t size = (uint64_t)luaL_checkinteger(L, 4);
    
    // Handle string data source
    const void *pData = NULL;
    
    if (lua_type(L, 5) == LUA_TSTRING)
    {
        // If string, use string data directly
        size_t strLen;
        pData = lua_tolstring(L, 5, &strLen);
        if (strLen < size)
        {
            tknWarning("Lua string size (%zu) less than requested update size (%llu)", strLen, size);
        }
    }
    else if (lua_type(L, 5) != LUA_TNIL && lua_type(L, 5) != LUA_TNONE)
    {
        tknWarning("Invalid data type for buffer update: expected string or nil, got %s", lua_typename(L, lua_type(L, 5)));
        return 0;
    }
    
    tknUpdateBuffer(pGfxContext, pTknBuffer, offset, size, pData);
    return 0;
}

// ============================================================================
// Sampler Functions
// ============================================================================

static int luaCreateSampler(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    int magFilter = (int)luaL_checkinteger(L, 2);
    int minFilter = (int)luaL_checkinteger(L, 3);
    int mipmapMode = (int)luaL_checkinteger(L, 4);
    int addressModeU = (int)luaL_checkinteger(L, 5);
    int addressModeV = (int)luaL_checkinteger(L, 6);
    int addressModeW = (int)luaL_checkinteger(L, 7);
    float mipLodBias = (float)luaL_checknumber(L, 8);
    bool anisotropyEnable = lua_toboolean(L, 9);
    float maxAnisotropy = (float)luaL_checknumber(L, 10);
    bool compareEnable = lua_toboolean(L, 11);
    int compareOp = (int)luaL_checkinteger(L, 12);
    float minLod = (float)luaL_checknumber(L, 13);
    float maxLod = (float)luaL_checknumber(L, 14);
    int borderColor = (int)luaL_checkinteger(L, 15);
    bool unnormalizedCoordinates = lua_toboolean(L, 16);
    
    void *pSampler = tknCreateSampler(pGfxContext, magFilter, minFilter, mipmapMode, addressModeU, addressModeV, addressModeW, mipLodBias, anisotropyEnable, maxAnisotropy, compareEnable, compareOp, minLod, maxLod, borderColor, unnormalizedCoordinates);
    lua_pushlightuserdata(L, pSampler);
    return 1;
}

static int luaDestroySampler(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pSampler = lua_touserdata(L, 2);
    tknDestroySampler(pGfxContext, pSampler);
    return 0;
}

// ============================================================================
// Binding Group Functions
// ============================================================================

static int luaCreateBindingGroupLayout(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    uint32_t shaderPathCount = (uint32_t)luaL_checkinteger(L, 2);
    
    // Extract shader paths table
    const char **shaderPaths = NULL;
    if (lua_istable(L, 3) && shaderPathCount > 0)
    {
        shaderPaths = tknMalloc(shaderPathCount * sizeof(const char *));
        for (uint32_t i = 0; i < shaderPathCount; i++)
        {
            lua_geti(L, 3, i + 1);
            shaderPaths[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
    }
    
    uint32_t set = (uint32_t)luaL_checkinteger(L, 4);
    
    void *pLayout = tknCreateBindingGroupLayout(pGfxContext, shaderPathCount, shaderPaths, set);
    tknFree(shaderPaths);
    
    lua_pushlightuserdata(L, pLayout);
    return 1;
}

static int luaDestroyBindingGroupLayout(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pLayout = lua_touserdata(L, 2);
    tknDestroyBindingGroupLayout(pGfxContext, pLayout);
    return 0;
}

static int luaCreateBindingGroup(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pLayout = lua_touserdata(L, 2);
    uint32_t resourceCount = (uint32_t)luaL_checkinteger(L, 3);
    
    // Extract resource pointers table
    void **resourcePtrs = NULL;
    if (lua_istable(L, 4) && resourceCount > 0)
    {
        resourcePtrs = tknMalloc(resourceCount * sizeof(void *));
        for (uint32_t i = 0; i < resourceCount; i++)
        {
            lua_geti(L, 4, i + 1);
            resourcePtrs[i] = lua_touserdata(L, -1);
            lua_pop(L, 1);
        }
    }
    
    void *pBindingGroup = tknCreateBindingGroup(pGfxContext, pLayout, resourceCount, resourcePtrs);
    tknFree(resourcePtrs);
    
    lua_pushlightuserdata(L, pBindingGroup);
    return 1;
}

static int luaDestroyBindingGroup(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pBindingGroup = lua_touserdata(L, 2);
    tknDestroyBindingGroup(pGfxContext, pBindingGroup);
    return 0;
}

static int luaUpdateBindingGroup(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pBindingGroup = lua_touserdata(L, 2);
    uint32_t resourceCount = (uint32_t)luaL_checkinteger(L, 3);
    
    // Extract indices table
    uint32_t *indices = NULL;
    if (lua_istable(L, 4) && resourceCount > 0)
    {
        indices = tknMalloc(resourceCount * sizeof(uint32_t));
        for (uint32_t i = 0; i < resourceCount; i++)
        {
            lua_geti(L, 4, i + 1);
            indices[i] = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
    }
    
    // Extract resource pointers table
    void **resourcePtrs = NULL;
    if (lua_istable(L, 5) && resourceCount > 0)
    {
        resourcePtrs = tknMalloc(resourceCount * sizeof(void *));
        for (uint32_t i = 0; i < resourceCount; i++)
        {
            lua_geti(L, 5, i + 1);
            resourcePtrs[i] = lua_touserdata(L, -1);
            lua_pop(L, 1);
        }
    }
    
    tknUpdateBindingGroup(pGfxContext, pBindingGroup, resourceCount, indices, resourcePtrs);
    tknFree(indices);
    tknFree(resourcePtrs);
    
    return 0;
}

// ============================================================================
// Pipeline State Structure Parsing Helpers
// ============================================================================
// Note: These functions are only available when Vulkan headers are included
// For WebGPU builds, the Vulkan pipeline creation functions are not available

#ifdef VK_API_VERSION_1_3

static VkPipelineInputAssemblyStateCreateInfo *luaExtractInputAssemblyState(lua_State *L, int tableIndex)
{
    if (!lua_istable(L, tableIndex))
        return NULL;
    
    VkPipelineInputAssemblyStateCreateInfo *pState = tknMalloc(sizeof(VkPipelineInputAssemblyStateCreateInfo));
    
    lua_getfield(L, tableIndex, "sType");
    pState->sType = (VkStructureType)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    pState->pNext = NULL;
    
    lua_getfield(L, tableIndex, "flags");
    pState->flags = (VkPipelineInputAssemblyStateCreateFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "topology");
    pState->topology = (VkPrimitiveTopology)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "primitiveRestartEnable");
    pState->primitiveRestartEnable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    return pState;
}

static VkPipelineViewportStateCreateInfo *luaExtractViewportState(lua_State *L, int tableIndex)
{
    if (!lua_istable(L, tableIndex))
        return NULL;
    
    VkPipelineViewportStateCreateInfo *pState = tknMalloc(sizeof(VkPipelineViewportStateCreateInfo));
    
    lua_getfield(L, tableIndex, "sType");
    pState->sType = (VkStructureType)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    pState->pNext = NULL;
    
    lua_getfield(L, tableIndex, "flags");
    pState->flags = (VkPipelineViewportStateCreateFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "viewportCount");
    pState->viewportCount = (uint32_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    // Extract viewports table
    VkViewport *pViewports = NULL;
    if (pState->viewportCount > 0)
    {
        lua_getfield(L, tableIndex, "pViewports");
        if (lua_istable(L, -1))
        {
            pViewports = tknMalloc(pState->viewportCount * sizeof(VkViewport));
            for (uint32_t i = 0; i < pState->viewportCount; i++)
            {
                lua_geti(L, -1, i + 1);
                if (lua_istable(L, -1))
                {
                    lua_getfield(L, -1, "x");
                    pViewports[i].x = (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "y");
                    pViewports[i].y = (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "width");
                    pViewports[i].width = (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "height");
                    pViewports[i].height = (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "minDepth");
                    pViewports[i].minDepth = (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "maxDepth");
                    pViewports[i].maxDepth = (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }
    pState->pViewports = pViewports;
    
    lua_getfield(L, tableIndex, "scissorCount");
    pState->scissorCount = (uint32_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    // Extract scissors table
    VkRect2D *pScissors = NULL;
    if (pState->scissorCount > 0)
    {
        lua_getfield(L, tableIndex, "pScissors");
        if (lua_istable(L, -1))
        {
            pScissors = tknMalloc(pState->scissorCount * sizeof(VkRect2D));
            for (uint32_t i = 0; i < pState->scissorCount; i++)
            {
                lua_geti(L, -1, i + 1);
                if (lua_istable(L, -1))
                {
                    lua_getfield(L, -1, "offsetX");
                    pScissors[i].offset.x = (int32_t)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "offsetY");
                    pScissors[i].offset.y = (int32_t)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "width");
                    pScissors[i].extent.width = (uint32_t)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "height");
                    pScissors[i].extent.height = (uint32_t)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }
    pState->pScissors = pScissors;
    
    return pState;
}

static VkPipelineRasterizationStateCreateInfo *luaExtractRasterizationState(lua_State *L, int tableIndex)
{
    if (!lua_istable(L, tableIndex))
        return NULL;
    
    VkPipelineRasterizationStateCreateInfo *pState = tknMalloc(sizeof(VkPipelineRasterizationStateCreateInfo));
    
    lua_getfield(L, tableIndex, "sType");
    pState->sType = (VkStructureType)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    pState->pNext = NULL;
    
    lua_getfield(L, tableIndex, "flags");
    pState->flags = (VkPipelineRasterizationStateCreateFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_pop(L, 1);
    
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "polygonMode");
    pState->polygonMode = (VkPolygonMode)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "cullMode");
    pState->cullMode = (VkCullModeFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "frontFace");
    pState->frontFace = (VkFrontFace)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "depthBiasEnable");
    pState->depthBiasEnable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "depthBiasConstantFactor");
    pState->depthBiasConstantFactor = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "depthBiasClamp");
    pState->depthBiasClamp = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "depthBiasSlopeFactor");
    pState->depthBiasSlopeFactor = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "lineWidth");
    pState->lineWidth = (float)lua_tonumber(L, -1);
    lua_pop(L, 1);
    
    return pState;
}

static VkPipelineMultisampleStateCreateInfo *luaExtractMultisampleState(lua_State *L, int tableIndex)
{
    if (!lua_istable(L, tableIndex))
        return NULL;
    
    VkPipelineMultisampleStateCreateInfo *pState = tknMalloc(sizeof(VkPipelineMultisampleStateCreateInfo));
    
    lua_getfield(L, tableIndex, "sType");
    pState->sType = (VkStructureType)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    pState->pNext = NULL;
    
    lua_getfield(L, tableIndex, "flags");
    pState->flags = (VkPipelineMultisampleStateCreateFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "rasterizationSamples");
    pState->rasterizationSamples = (VkSampleCountFlagBits)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_pop(L, 1);
    
    lua_pop(L, 1);
    
    pState->pSampleMask = NULL;
    
    lua_getfield(L, tableIndex, "alphaToCoverageEnable");
    pState->alphaToCoverageEnable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    lua_pop(L, 1);
    
    return pState;
}

static VkPipelineDepthStencilStateCreateInfo *luaExtractDepthStencilState(lua_State *L, int tableIndex)
{
    if (!lua_istable(L, tableIndex))
        return NULL;
    
    VkPipelineDepthStencilStateCreateInfo *pState = tknMalloc(sizeof(VkPipelineDepthStencilStateCreateInfo));
    
    lua_getfield(L, tableIndex, "sType");
    pState->sType = (VkStructureType)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    pState->pNext = NULL;
    
    lua_getfield(L, tableIndex, "flags");
    pState->flags = (VkPipelineDepthStencilStateCreateFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "depthTestEnable");
    pState->depthTestEnable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "depthWriteEnable");
    pState->depthWriteEnable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "depthCompareOp");
    pState->depthCompareOp = (VkCompareOp)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "stencilTestEnable");
    pState->stencilTestEnable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    // front and back are VkStencilOpState - set to defaults for now
    memset(&pState->front, 0, sizeof(VkStencilOpState));
    memset(&pState->back, 0, sizeof(VkStencilOpState));
    
    lua_pop(L, 1);
    
    lua_pop(L, 1);
    
    return pState;
}

static VkPipelineColorBlendStateCreateInfo *luaExtractColorBlendState(lua_State *L, int tableIndex)
{
    if (!lua_istable(L, tableIndex))
        return NULL;
    
    VkPipelineColorBlendStateCreateInfo *pState = tknMalloc(sizeof(VkPipelineColorBlendStateCreateInfo));
    
    lua_getfield(L, tableIndex, "sType");
    pState->sType = (VkStructureType)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    pState->pNext = NULL;
    
    lua_getfield(L, tableIndex, "flags");
    pState->flags = (VkPipelineColorBlendStateCreateFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "logicOpEnable");
    pState->logicOpEnable = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "logicOp");
    pState->logicOp = (VkLogicOp)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "attachmentCount");
    pState->attachmentCount = (uint32_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    // Extract attachments array
    VkPipelineColorBlendAttachmentState *pAttachments = NULL;
    if (pState->attachmentCount > 0)
    {
        lua_getfield(L, tableIndex, "pAttachments");
        if (lua_istable(L, -1))
        {
            pAttachments = tknMalloc(pState->attachmentCount * sizeof(VkPipelineColorBlendAttachmentState));
            for (uint32_t i = 0; i < pState->attachmentCount; i++)
            {
                lua_geti(L, -1, i + 1);
                if (lua_istable(L, -1))
                {
                    lua_getfield(L, -1, "blendEnable");
                    pAttachments[i].blendEnable = lua_toboolean(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "srcColorBlendFactor");
                    pAttachments[i].srcColorBlendFactor = (VkBlendFactor)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "dstColorBlendFactor");
                    pAttachments[i].dstColorBlendFactor = (VkBlendFactor)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "colorBlendOp");
                    pAttachments[i].colorBlendOp = (VkBlendOp)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "srcAlphaBlendFactor");
                    pAttachments[i].srcAlphaBlendFactor = (VkBlendFactor)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "dstAlphaBlendFactor");
                    pAttachments[i].dstAlphaBlendFactor = (VkBlendFactor)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "alphaBlendOp");
                    pAttachments[i].alphaBlendOp = (VkBlendOp)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                    
                    lua_getfield(L, -1, "colorWriteMask");
                    pAttachments[i].colorWriteMask = (VkColorComponentFlags)lua_tointeger(L, -1);
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }
    pState->pAttachments = pAttachments;
    
    // Extract blend constants
    lua_getfield(L, tableIndex, "blendConstants");
    if (lua_istable(L, -1))
    {
        for (int i = 0; i < 4; i++)
        {
            lua_geti(L, -1, i + 1);
            pState->blendConstants[i] = (float)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    
    return pState;
}

static VkPipelineDynamicStateCreateInfo *luaExtractDynamicState(lua_State *L, int tableIndex)
{
    if (!lua_istable(L, tableIndex))
        return NULL;
    
    VkPipelineDynamicStateCreateInfo *pState = tknMalloc(sizeof(VkPipelineDynamicStateCreateInfo));
    
    lua_getfield(L, tableIndex, "sType");
    pState->sType = (VkStructureType)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    pState->pNext = NULL;
    
    lua_getfield(L, tableIndex, "flags");
    pState->flags = (VkPipelineDynamicStateCreateFlags)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    lua_getfield(L, tableIndex, "dynamicStateCount");
    pState->dynamicStateCount = (uint32_t)lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    // Extract dynamic states array
    VkDynamicState *pDynamicStates = NULL;
    if (pState->dynamicStateCount > 0)
    {
        lua_getfield(L, tableIndex, "pDynamicStates");
        if (lua_istable(L, -1))
        {
            pDynamicStates = tknMalloc(pState->dynamicStateCount * sizeof(VkDynamicState));
            for (uint32_t i = 0; i < pState->dynamicStateCount; i++)
            {
                lua_geti(L, -1, i + 1);
                pDynamicStates[i] = (VkDynamicState)lua_tointeger(L, -1);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }
    pState->pDynamicStates = pDynamicStates;
    
    return pState;
}
// Note: binding is determined by parameter position (mesh=0, instance=1), not parsed from table
static TknVertexInputAttributeLayout *luaExtractVertexInputAttributeDescriptions(lua_State *L, int tableIndex, uint32_t *pCount)
{
    if (!lua_istable(L, tableIndex))
    {
        *pCount = 0;
        return NULL;
    }
    
    uint32_t count = (uint32_t)lua_rawlen(L, tableIndex);
    if (count == 0)
    {
        *pCount = 0;
        return NULL;
    }
    
    TknVertexInputAttributeLayout *descriptions = tknMalloc(count * sizeof(TknVertexInputAttributeLayout));
    
    for (uint32_t i = 0; i < count; i++)
    {
        lua_geti(L, tableIndex, i + 1);
        if (!lua_istable(L, -1))
        {
            tknFree(descriptions);
            *pCount = 0;
            lua_pop(L, 1);
            return NULL;
        }
        
        // Extract fields: location, format, offset
        lua_getfield(L, -1, "location");
        descriptions[i].location = (uint32_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "format");
        descriptions[i].format = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, -1, "offset");
        descriptions[i].offset = (uint32_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_pop(L, 1); // Pop the attribute description table
    }
    
    *pCount = count;
    return descriptions;
}

static int luaCreatePipelinePtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pRenderPassBindingGroupLayout = lua_touserdata(L, 2);
    uint32_t spvPathCount = (uint32_t)luaL_checkinteger(L, 3);
    
    // Extract SPV paths table
    const char **spvPaths = NULL;
    if (lua_istable(L, 4) && spvPathCount > 0)
    {
        spvPaths = tknMalloc(spvPathCount * sizeof(const char *));
        for (uint32_t i = 0; i < spvPathCount; i++)
        {
            lua_geti(L, 4, i + 1);
            spvPaths[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
    }
    
    // Extract mesh and instance vertex input attribute descriptions
    uint32_t meshAttrCount = 0;
    TknVertexInputAttributeLayout *pMeshAttrs = luaExtractVertexInputAttributeDescriptions(L, 5, &meshAttrCount);
    
    uint32_t instanceAttrCount = 0;
    TknVertexInputAttributeLayout *pInstanceAttrs = luaExtractVertexInputAttributeDescriptions(L, 6, &instanceAttrCount);
    
    // Build TknVertexState
    TknVertexState vertexState = {
        .meshAttributeCount = meshAttrCount,
        .pMeshAttributes = pMeshAttrs,
        .instanceAttributeCount = instanceAttrCount,
        .pInstanceAttributes = pInstanceAttrs,
    };
    
    // Build TknPrimitiveState from table at index 7
    TknPrimitiveState primitiveState = {0};
    if (lua_istable(L, 7))
    {
        lua_getfield(L, 7, "topology");
        primitiveState.topology = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "polygonMode");
        primitiveState.polygonMode = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "cullMode");
        primitiveState.cullMode = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "frontFace");
        primitiveState.frontFace = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "depthBiasEnable");
        primitiveState.depthBiasEnable = (bool)lua_toboolean(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "depthBiasConstantFactor");
        primitiveState.depthBiasConstantFactor = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "depthBiasClamp");
        primitiveState.depthBiasClamp = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "depthBiasSlopeFactor");
        primitiveState.depthBiasSlopeFactor = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 7, "lineWidth");
        primitiveState.lineWidth = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    
    // Build TknFragmentState from table at index 8
    int *pColorAttachmentFormats = NULL;
    uint32_t colorAttachmentCount = 0;
    TknPipelineColorBlendState colorBlendState = {0};
    
    if (lua_istable(L, 8))
    {
        lua_getfield(L, 8, "colorAttachmentCount");
        colorAttachmentCount = (uint32_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        // Extract color attachment formats
        lua_getfield(L, 8, "pColorAttachmentFormats");
        if (lua_istable(L, -1) && colorAttachmentCount > 0)
        {
            pColorAttachmentFormats = tknMalloc(colorAttachmentCount * sizeof(int));
            for (uint32_t i = 0; i < colorAttachmentCount; i++)
            {
                lua_geti(L, -1, i + 1);
                pColorAttachmentFormats[i] = (int)lua_tointeger(L, -1);
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
        
        // Extract color blend state
        lua_getfield(L, 8, "colorBlend");
        if (lua_istable(L, -1))
        {
            lua_getfield(L, -1, "attachmentCount");
            colorBlendState.attachmentCount = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            // Extract blend attachments
            lua_getfield(L, -1, "pAttachments");
            if (lua_istable(L, -1) && colorBlendState.attachmentCount > 0)
            {
                colorBlendState.pAttachments = tknMalloc(colorBlendState.attachmentCount * sizeof(TknPipelineColorBlendAttachmentState));
                for (uint32_t i = 0; i < colorBlendState.attachmentCount; i++)
                {
                    lua_geti(L, -1, i + 1);
                    if (lua_istable(L, -1))
                    {
                        lua_getfield(L, -1, "blendEnable");
                        colorBlendState.pAttachments[i].blendEnable = (bool)lua_toboolean(L, -1);
                        lua_pop(L, 1);
                        
                        lua_getfield(L, -1, "srcColorBlendFactor");
                        colorBlendState.pAttachments[i].srcColorBlendFactor = (int)lua_tointeger(L, -1);
                        lua_pop(L, 1);
                        
                        lua_getfield(L, -1, "dstColorBlendFactor");
                        colorBlendState.pAttachments[i].dstColorBlendFactor = (int)lua_tointeger(L, -1);
                        lua_pop(L, 1);
                        
                        lua_getfield(L, -1, "colorBlendOp");
                        colorBlendState.pAttachments[i].colorBlendOp = (int)lua_tointeger(L, -1);
                        lua_pop(L, 1);
                        
                        lua_getfield(L, -1, "srcAlphaBlendFactor");
                        colorBlendState.pAttachments[i].srcAlphaBlendFactor = (int)lua_tointeger(L, -1);
                        lua_pop(L, 1);
                        
                        lua_getfield(L, -1, "dstAlphaBlendFactor");
                        colorBlendState.pAttachments[i].dstAlphaBlendFactor = (int)lua_tointeger(L, -1);
                        lua_pop(L, 1);
                        
                        lua_getfield(L, -1, "alphaBlendOp");
                        colorBlendState.pAttachments[i].alphaBlendOp = (int)lua_tointeger(L, -1);
                        lua_pop(L, 1);
                        
                        lua_getfield(L, -1, "colorWriteMask");
                        colorBlendState.pAttachments[i].colorWriteMask = (int)lua_tointeger(L, -1);
                        lua_pop(L, 1);
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
            
            // Extract blend constants
            lua_getfield(L, -1, "blendConstants");
            if (lua_istable(L, -1))
            {
                for (int i = 0; i < 4; i++)
                {
                    lua_geti(L, -1, i + 1);
                    colorBlendState.blendConstants[i] = (float)lua_tonumber(L, -1);
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    
    TknFragmentState fragmentState = {
        .colorAttachmentCount = colorAttachmentCount,
        .pColorAttachmentFormats = pColorAttachmentFormats,
        .colorBlend = colorBlendState,
    };
    
    // Build TknPipelineMultisampleState from table at index 9
    TknPipelineMultisampleState multisampleState = {0};
    if (lua_istable(L, 9))
    {
        lua_getfield(L, 9, "rasterizationSamples");
        multisampleState.rasterizationSamples = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 9, "alphaToCoverageEnable");
        multisampleState.alphaToCoverageEnable = (bool)lua_toboolean(L, -1);
        lua_pop(L, 1);
    }
    
    // Build TknPipelineDepthStencilState from table at index 10
    TknPipelineDepthStencilState depthStencilState = {0};
    if (lua_istable(L, 10))
    {
        lua_getfield(L, 10, "depthTestEnable");
        depthStencilState.depthTestEnable = (bool)lua_toboolean(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 10, "depthWriteEnable");
        depthStencilState.depthWriteEnable = (bool)lua_toboolean(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 10, "depthCompareOp");
        depthStencilState.depthCompareOp = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        
        lua_getfield(L, 10, "stencilTestEnable");
        depthStencilState.stencilTestEnable = (bool)lua_toboolean(L, -1);
        lua_pop(L, 1);
        
        // Extract front stencil op state
        lua_getfield(L, 10, "front");
        if (lua_istable(L, -1))
        {
            lua_getfield(L, -1, "failOp");
            depthStencilState.front.failOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "passOp");
            depthStencilState.front.passOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "depthFailOp");
            depthStencilState.front.depthFailOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "compareOp");
            depthStencilState.front.compareOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "compareMask");
            depthStencilState.front.compareMask = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "writeMask");
            depthStencilState.front.writeMask = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "reference");
            depthStencilState.front.reference = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
        
        // Extract back stencil op state
        lua_getfield(L, 10, "back");
        if (lua_istable(L, -1))
        {
            lua_getfield(L, -1, "failOp");
            depthStencilState.back.failOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "passOp");
            depthStencilState.back.passOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "depthFailOp");
            depthStencilState.back.depthFailOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "compareOp");
            depthStencilState.back.compareOp = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "compareMask");
            depthStencilState.back.compareMask = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "writeMask");
            depthStencilState.back.writeMask = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
            
            lua_getfield(L, -1, "reference");
            depthStencilState.back.reference = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    
    // Extract depth attachment format from index 11
    int depthAttachmentFormat = (int)luaL_checkinteger(L, 11);
    
    // Call the new C API with individual state parameters
    void *pPipeline = tknCreatePipelinePtr(
        pGfxContext,
        pRenderPassBindingGroupLayout,
        spvPathCount,
        spvPaths,
        &vertexState,
        &primitiveState,
        &fragmentState,
        &multisampleState,
        &depthStencilState,
        depthAttachmentFormat
    );
    
    // Cleanup
    tknFree(spvPaths);
    tknFree(pMeshAttrs);
    tknFree(pInstanceAttrs);
    tknFree(pColorAttachmentFormats);
    if (colorBlendState.pAttachments)
        tknFree(colorBlendState.pAttachments);
    
    lua_pushlightuserdata(L, pPipeline);
    return 1;
}

static int luaDestroyPipelinePtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pPipeline = lua_touserdata(L, 2);
    tknDestroyPipelinePtr(pGfxContext, pPipeline);
    return 0;
}

#endif // VK_API_VERSION_1_3

// ============================================================================
// Command Buffer Functions
// ============================================================================

static int luaBeginCommandBuffer(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    tknBeginCommandBuffer(pGfxContext);
    return 0;
}

static int luaEndCommandBuffer(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    tknEndCommandBuffer(pGfxContext);
    return 0;
}

// ============================================================================
// Render Pass Functions
// ============================================================================

static int luaBeginRenderPass(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    uint32_t colorAttachmentCount = (uint32_t)luaL_checkinteger(L, 2);
    
    // Extract color image view pointers table
    void **colorImageViewPtrs = NULL;
    if (lua_istable(L, 3) && colorAttachmentCount > 0)
    {
        colorImageViewPtrs = tknMalloc(colorAttachmentCount * sizeof(void *));
        for (uint32_t i = 0; i < colorAttachmentCount; i++)
        {
            lua_geti(L, 3, i + 1);
            colorImageViewPtrs[i] = lua_touserdata(L, -1);
            lua_pop(L, 1);
        }
    }
    
    // Extract load ops table
    int *loadOps = NULL;
    if (lua_istable(L, 4) && colorAttachmentCount > 0)
    {
        loadOps = tknMalloc(colorAttachmentCount * sizeof(int));
        for (uint32_t i = 0; i < colorAttachmentCount; i++)
        {
            lua_geti(L, 4, i + 1);
            loadOps[i] = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
    }
    
    // Extract store ops table
    int *storeOps = NULL;
    if (lua_istable(L, 5) && colorAttachmentCount > 0)
    {
        storeOps = tknMalloc(colorAttachmentCount * sizeof(int));
        for (uint32_t i = 0; i < colorAttachmentCount; i++)
        {
            lua_geti(L, 5, i + 1);
            storeOps[i] = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
    }
    
    // Extract color clear values table (array of arrays)
    double (*colorClearValues)[4] = NULL;
    if (lua_istable(L, 6) && colorAttachmentCount > 0)
    {
        colorClearValues = tknMalloc(colorAttachmentCount * sizeof(double[4]));
        for (uint32_t i = 0; i < colorAttachmentCount; i++)
        {
            lua_geti(L, 6, i + 1);
            if (lua_istable(L, -1))
            {
                for (int j = 0; j < 4; j++)
                {
                    lua_geti(L, -1, j + 1);
                    colorClearValues[i][j] = lua_tonumber(L, -1);
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }
    }
    
    void *pDepthImageView = lua_touserdata(L, 7);
    int depthLoadOp = (int)luaL_checkinteger(L, 8);
    int depthStoreOp = (int)luaL_checkinteger(L, 9);
    float depthClearValue = (float)luaL_checknumber(L, 10);
    uint32_t stencilClearValue = (uint32_t)luaL_checkinteger(L, 11);
    uint32_t width = (uint32_t)luaL_checkinteger(L, 12);
    uint32_t height = (uint32_t)luaL_checkinteger(L, 13);
    
    tknBeginRenderPass(pGfxContext, colorAttachmentCount, colorImageViewPtrs, loadOps, storeOps, colorClearValues, pDepthImageView, depthLoadOp, depthStoreOp, depthClearValue, stencilClearValue, width, height);
    
    tknFree(colorImageViewPtrs);
    tknFree(loadOps);
    tknFree(storeOps);
    tknFree(colorClearValues);
    
    return 0;
}

static int luaEndRenderPass(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    tknEndRenderPass(pGfxContext);
    return 0;
}

// ============================================================================
// Pipeline Set Function
// ============================================================================

static int luaSetPipelinePtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pPipeline = lua_touserdata(L, 2);
    void *pRenderPassBindingGroup = lua_touserdata(L, 3);
    void *pPipelineBindingGroup = lua_touserdata(L, 4);
    
    tknSetPipelinePtr(pGfxContext, pPipeline, pRenderPassBindingGroup, pPipelineBindingGroup);
    return 0;
}

// ============================================================================
// Binding Functions
// ============================================================================

static int luaBindVertexBuffer(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pBuffer = lua_touserdata(L, 2);
    uint64_t offset = (uint64_t)luaL_checkinteger(L, 3);
    
    tknBindVertexBuffer(pGfxContext, pBuffer, offset);
    return 0;
}

static int luaBindInstanceBuffer(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pBuffer = lua_touserdata(L, 2);
    uint64_t offset = (uint64_t)luaL_checkinteger(L, 3);
    
    tknBindInstanceBuffer(pGfxContext, pBuffer, offset);
    return 0;
}

static int luaBindIndexBuffer(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    void *pBuffer = lua_touserdata(L, 2);
    int indexType = (int)luaL_checkinteger(L, 3);
    uint64_t offset = (uint64_t)luaL_checkinteger(L, 4);
    
    tknBindIndexBuffer(pGfxContext, pBuffer, indexType, offset);
    return 0;
}

// ============================================================================
// Draw Functions
// ============================================================================

static int luaDraw(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    uint32_t vertexCount = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t instanceCount = (uint32_t)luaL_checkinteger(L, 3);
    uint32_t firstVertex = (uint32_t)luaL_checkinteger(L, 4);
    uint32_t firstInstance = (uint32_t)luaL_checkinteger(L, 5);
    
    tknDraw(pGfxContext, vertexCount, instanceCount, firstVertex, firstInstance);
    return 0;
}

static int luaDrawIndexed(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    uint32_t indexCount = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t instanceCount = (uint32_t)luaL_checkinteger(L, 3);
    uint32_t firstIndex = (uint32_t)luaL_checkinteger(L, 4);
    int32_t baseVertex = (int32_t)luaL_checkinteger(L, 5);
    uint32_t firstInstance = (uint32_t)luaL_checkinteger(L, 6);
    
    tknDrawIndexed(pGfxContext, indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
    return 0;
}

// ============================================================================
// Font Functions
// ============================================================================

static int luaCreateTknFontLibraryPtr(lua_State *L)
{
    TknFontLibrary *pTknFontLibrary = createTknFontLibraryPtr();
    lua_pushlightuserdata(L, pTknFontLibrary);
    return 1;
}

static int luaDestroyTknFontLibraryPtr(lua_State *L)
{
    void *pTknFontLibrary = lua_touserdata(L, 1);
    void *pGfxContext = lua_touserdata(L, 2);
    destroyTknFontLibraryPtr((TknFontLibrary *)pTknFontLibrary, pGfxContext);
    return 0;
}

static int luaCreateTknFontPtr(lua_State *L)
{
    void *pTknFontLibrary = lua_touserdata(L, 1);
    void *pGfxContext = lua_touserdata(L, 2);
    uint32_t fontPathCount = (uint32_t)luaL_checkinteger(L, 3);
    
    // Extract font paths table
    const char **fontPaths = NULL;
    if (lua_istable(L, 4) && fontPathCount > 0)
    {
        fontPaths = tknMalloc(fontPathCount * sizeof(const char *));
        for (uint32_t i = 0; i < fontPathCount; i++)
        {
            lua_geti(L, 4, i + 1);
            fontPaths[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
    }
    
    uint32_t fontSize = (uint32_t)luaL_checkinteger(L, 5);
    uint32_t atlasLength = (uint32_t)luaL_checkinteger(L, 6);
    
    // Extract bold strengths table (optional)
    FT_Pos *boldStrengths = NULL;
    if (lua_istable(L, 7) && fontPathCount > 0)
    {
        boldStrengths = tknMalloc(fontPathCount * sizeof(FT_Pos));
        for (uint32_t i = 0; i < fontPathCount; i++)
        {
            lua_geti(L, 7, i + 1);
            boldStrengths[i] = (FT_Pos)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
    }
    
    void *pTknFont = createTknFontPtr((TknFontLibrary *)pTknFontLibrary, pGfxContext, fontPathCount, fontPaths, fontSize, atlasLength, boldStrengths);
    
    tknFree(fontPaths);
    tknFree(boldStrengths);
    
    lua_pushlightuserdata(L, pTknFont);
    return 1;
}

static int luaDestroyTknFontPtr(lua_State *L)
{
    void *pTknFontLibrary = lua_touserdata(L, 1);
    void *pTknFont = lua_touserdata(L, 2);
    void *pGfxContext = lua_touserdata(L, 3);
    destroyTknFontPtr((TknFontLibrary *)pTknFontLibrary, (TknFont *)pTknFont, pGfxContext);
    return 0;
}

static int luaLoadTknChar(lua_State *L)
{
    void *pTknFont = lua_touserdata(L, 1);
    uint32_t unicode = (uint32_t)luaL_checkinteger(L, 2);
    
    bool hasLoaded = false;
    void *pTknChar = loadTknChar((TknFont *)pTknFont, unicode, &hasLoaded);
    
    lua_pushlightuserdata(L, pTknChar);
    lua_pushboolean(L, hasLoaded);
    return 2;
}

static int luaFlushTknFontPtr(lua_State *L)
{
    void *pTknFont = lua_touserdata(L, 1);
    void *pGfxContext = lua_touserdata(L, 2);
    flushTknFontPtr((TknFont *)pTknFont, pGfxContext);
    return 0;
}

// ============================================================================
// Lua Binding Registration
// ============================================================================

void bindTknGfxFunctions(lua_State *pLuaState)
{
    luaL_Reg tknGfxRegs[] = {
        // Graphics Context
        {"tknCreateGfxContextPtr", luaCreateGfxContextPtr},
        {"tknDestroyGfxContextPtr", luaDestroyGfxContextPtr},
        
        // Image
        {"tknCreateImagePtr", luaCreateImagePtr},
        {"tknDestroyImagePtr", luaDestroyImagePtr},
        {"tknCreateImageView", luaCreateImageView},
        {"tknDestroyImageView", luaDestroyImageView},
        {"tknWriteImagePtr", luaWriteImagePtr},
        
        // Uniform Buffer
        {"tknCreateUniformBuffer", luaCreateUniformBuffer},
        {"tknDestroyUniformBuffer", luaDestroyUniformBuffer},
        
        // Buffer
        {"tknCreateBufferPtr", luaCreateBufferPtr},
        {"tknDestroyBufferPtr", luaDestroyBufferPtr},
        {"tknUpdateBuffer", luaUpdateBuffer},
        
        // Sampler
        {"tknCreateSampler", luaCreateSampler},
        {"tknDestroySampler", luaDestroySampler},
        
        // Binding Group
        {"tknCreateBindingGroupLayout", luaCreateBindingGroupLayout},
        {"tknDestroyBindingGroupLayout", luaDestroyBindingGroupLayout},
        {"tknCreateBindingGroup", luaCreateBindingGroup},
        {"tknDestroyBindingGroup", luaDestroyBindingGroup},
        {"tknUpdateBindingGroup", luaUpdateBindingGroup},
        
        // Pipeline (Vulkan only)
#ifdef VK_API_VERSION_1_3
        {"tknCreatePipelinePtr", luaCreatePipelinePtr},
        {"tknDestroyPipelinePtr", luaDestroyPipelinePtr},
#endif
        
        // Command Buffer
        {"tknBeginCommandBuffer", luaBeginCommandBuffer},
        {"tknEndCommandBuffer", luaEndCommandBuffer},
        
        // Render Pass
        {"tknBeginRenderPass", luaBeginRenderPass},
        {"tknEndRenderPass", luaEndRenderPass},
        
        // Pipeline Set
        {"tknSetPipelinePtr", luaSetPipelinePtr},
        
        // Buffer Binding
        {"tknBindVertexBuffer", luaBindVertexBuffer},
        {"tknBindInstanceBuffer", luaBindInstanceBuffer},
        {"tknBindIndexBuffer", luaBindIndexBuffer},
        
        // Draw
        {"tknDraw", luaDraw},
        {"tknDrawIndexed", luaDrawIndexed},
        
        // Font
        {"tknCreateTknFontLibraryPtr", luaCreateTknFontLibraryPtr},
        {"tknDestroyTknFontLibraryPtr", luaDestroyTknFontLibraryPtr},
        {"tknCreateTknFontPtr", luaCreateTknFontPtr},
        {"tknDestroyTknFontPtr", luaDestroyTknFontPtr},
        {"tknLoadTknChar", luaLoadTknChar},
        {"tknFlushTknFontPtr", luaFlushTknFontPtr},
        
        {NULL, NULL},
    };
    
    luaL_newlib(pLuaState, tknGfxRegs);
    lua_setglobal(pLuaState, "tkn");
}
