// Tickernel Graphics API Lua Binding Layer
// Provides Lua access to TickernelGfx graphics core functions

#include "tkn.h"
#include <stdio.h>

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
// Pipeline Functions
// ============================================================================

static int luaCreatePipelinePtr(lua_State *L)
{
    void *pGfxContext = lua_touserdata(L, 1);
    uint32_t colorAttachmentCount = (uint32_t)luaL_checkinteger(L, 2);
    
    // Extract color attachment formats table
    int *pColorAttachmentFormats = NULL;
    if (lua_istable(L, 3) && colorAttachmentCount > 0)
    {
        pColorAttachmentFormats = tknMalloc(colorAttachmentCount * sizeof(int));
        for (uint32_t i = 0; i < colorAttachmentCount; i++)
        {
            lua_geti(L, 3, i + 1);
            pColorAttachmentFormats[i] = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
        }
    }
    
    int depthAttachmentFormat = (int)luaL_checkinteger(L, 4);
    void *pRenderPassBindingGroupLayout = lua_touserdata(L, 5);
    uint32_t spvPathCount = (uint32_t)luaL_checkinteger(L, 6);
    
    // Extract SPV paths table
    const char **spvPaths = NULL;
    if (lua_istable(L, 7) && spvPathCount > 0)
    {
        spvPaths = tknMalloc(spvPathCount * sizeof(const char *));
        for (uint32_t i = 0; i < spvPathCount; i++)
        {
            lua_geti(L, 7, i + 1);
            spvPaths[i] = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
    }
    
    void *pMeshVertexInputLayout = lua_touserdata(L, 8);
    void *pInstanceVertexInputLayout = lua_touserdata(L, 9);
    void *pVkPipelineInputAssemblyStateCreateInfo = lua_touserdata(L, 10);
    void *pVkPipelineViewportStateCreateInfo = lua_touserdata(L, 11);
    void *pVkPipelineRasterizationStateCreateInfo = lua_touserdata(L, 12);
    void *pVkPipelineMultisampleStateCreateInfo = lua_touserdata(L, 13);
    void *pVkPipelineDepthStencilStateCreateInfo = lua_touserdata(L, 14);
    void *pVkPipelineColorBlendStateCreateInfo = lua_touserdata(L, 15);
    void *pVkPipelineDynamicStateCreateInfo = lua_touserdata(L, 16);
    
    void *pPipeline = tknCreatePipelinePtr(pGfxContext, colorAttachmentCount, pColorAttachmentFormats, depthAttachmentFormat, pRenderPassBindingGroupLayout, spvPathCount, spvPaths, pMeshVertexInputLayout, pInstanceVertexInputLayout, pVkPipelineInputAssemblyStateCreateInfo, pVkPipelineViewportStateCreateInfo, pVkPipelineRasterizationStateCreateInfo, pVkPipelineMultisampleStateCreateInfo, pVkPipelineDepthStencilStateCreateInfo, pVkPipelineColorBlendStateCreateInfo, pVkPipelineDynamicStateCreateInfo);
    
    tknFree(pColorAttachmentFormats);
    tknFree(spvPaths);
    
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
        
        // Pipeline
        {"tknCreatePipelinePtr", luaCreatePipelinePtr},
        {"tknDestroyPipelinePtr", luaDestroyPipelinePtr},
        
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
        
        {NULL, NULL},
    };
    
    luaL_newlib(pLuaState, tknGfxRegs);
    lua_setglobal(pLuaState, "tknGfx");
}
