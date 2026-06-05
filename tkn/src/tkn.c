// Tickernel Integration Layer Implementation
// Integrates TickernelGfx graphics core with Lua scripting engine

#include "tkn.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Helper function to check Lua errors
static void assertLuaResult(lua_State *pLuaState, int result)
{
    if (result != LUA_OK)
    {
        const char *errorMsg = lua_tostring(pLuaState, -1);
        tknError("Lua Error: %s", errorMsg ? errorMsg : "Unknown error");
        lua_pop(pLuaState, 1);
    }
}

// Lua error handler function
static int errorHandler(lua_State *pLuaState)
{
    const char *errorMsg = lua_tostring(pLuaState, -1);
    tknError("Lua Error: %s", errorMsg ? errorMsg : "Unknown error");
    return 1;
}

void *tknCreateContextPtr(const char *assetsPath, uint32_t luaLibraryCount, LuaLibrary *luaLibraries, uint32_t extensionCount, const char **extensions, void *pSurface, uint32_t width, uint32_t height)
{
    TknContext *pTknContext = tknMalloc(sizeof(TknContext));

    char globalVertSpvPath[FILENAME_MAX];
    char globalFragSpvPath[FILENAME_MAX];
    snprintf(globalVertSpvPath, FILENAME_MAX, "%s/shaders/global.vert.spv", assetsPath);
    snprintf(globalFragSpvPath, FILENAME_MAX, "%s/shaders/global.frag.spv", assetsPath);

    const char *spvPaths[] = {
        globalVertSpvPath,
        globalFragSpvPath,
    };

    void *pTknGfxContext = tknCreateGfxContextPtr(extensionCount, extensions, pSurface, width, height, TKN_ARRAY_COUNT(spvPaths), spvPaths);

    lua_State *pLuaState = luaL_newstate();
    tknAssert(pLuaState, "Failed to create Lua state");
    luaL_openlibs(pLuaState);

    char packagePath[FILENAME_MAX];
    snprintf(packagePath, FILENAME_MAX, "%s/lua/?.lua", assetsPath);
    lua_getglobal(pLuaState, "package");
    lua_pushstring(pLuaState, packagePath);
    lua_setfield(pLuaState, -2, "path");
    lua_pop(pLuaState, 1);

    // Register Lua libraries
    for (uint32_t luaLibraryIndex = 0; luaLibraryIndex < luaLibraryCount; luaLibraryIndex++)
    {
        LuaLibrary luaLibrary = luaLibraries[luaLibraryIndex];
        lua_createtable(pLuaState, 0, luaLibrary.luaRegCount - 1);
        luaL_setfuncs(pLuaState, luaLibrary.luaRegs, 0);
        lua_setglobal(pLuaState, luaLibrary.name);
    }

    // Load main Lua engine script
    char tknEngineLuaPath[FILENAME_MAX];
    snprintf(tknEngineLuaPath, FILENAME_MAX, "%s/lua/tknEngine.lua", assetsPath);
    int result = luaL_dofile(pLuaState, tknEngineLuaPath);
    assertLuaResult(pLuaState, result);

    // Call Lua start function with error handler
    lua_getfield(pLuaState, -1, "start");
    lua_pushlightuserdata(pLuaState, pTknGfxContext);
    lua_pushstring(pLuaState, assetsPath);
    lua_pushcfunction(pLuaState, errorHandler);
    lua_insert(pLuaState, -4);
    assertLuaResult(pLuaState, lua_pcall(pLuaState, 2, 0, -4));
    lua_pop(pLuaState, 1);

    TknContext tknContext = {
        .pGfxContext = pTknGfxContext,
        .pLuaState = pLuaState,
    };
    *pTknContext = tknContext;
    return pTknContext;
}

void tknDestroyContextPtr(TknContext *pTknContext)
{
    if (!pTknContext)
        return;

    void *pTknGfxContext = pTknContext->pGfxContext;
    lua_State *pLuaState = pTknContext->pLuaState;

    // Call Lua stop function with error handler
    lua_pushcfunction(pLuaState, errorHandler);
    lua_getglobal(pLuaState, "tknEngine");
    lua_getfield(pLuaState, -1, "stop");
    lua_pushlightuserdata(pLuaState, pTknGfxContext);
    assertLuaResult(pLuaState, lua_pcall(pLuaState, 1, 0, -4));
    lua_pop(pLuaState, 2);

    tknDestroyGfxContextPtr(pTknGfxContext);
    lua_close(pLuaState);
    tknFree(pTknContext);
}

void tknUpdateContext(TknContext *pTknContext, Extent2D extent, uint32_t keyCodeStateCount, InputState *keyCodeStates, uint32_t mouseCodeStateCount, InputState *mouseCodeStates, float scrollingDeltaX, float scrollingDeltaY, float mousePositionNDCX, float mousePositionNDCY, const char *inputText, bool *pShouldQuit, bool *pImeEnabled)
{
    lua_State *pLuaState = pTknContext->pLuaState;
    *pShouldQuit = false;
    *pImeEnabled = false;

    // Update input states first
    if (keyCodeStates && keyCodeStateCount > 0)
    {
        lua_getglobal(pLuaState, "require");
        lua_pushstring(pLuaState, "input");
        lua_call(pLuaState, 1, 1);

        lua_getfield(pLuaState, -1, "keyCodeStates");
        // Use the actual keyCodeStateCount parameter for safety
        for (uint32_t i = 0; i < keyCodeStateCount; i++)
        {
            lua_pushinteger(pLuaState, i);
            lua_pushinteger(pLuaState, keyCodeStates[i]);
            lua_settable(pLuaState, -3);
        }
        lua_pop(pLuaState, 1);

        lua_getfield(pLuaState, -1, "mouseCodeStates");
        for (uint32_t i = 0; i < mouseCodeStateCount; i++)
        {
            lua_pushinteger(pLuaState, i);
            lua_pushinteger(pLuaState, mouseCodeStates[i]);
            lua_settable(pLuaState, -3);
        }
        lua_pop(pLuaState, 1);

        lua_getfield(pLuaState, -1, "scrollingDelta");
        lua_pushnumber(pLuaState, scrollingDeltaX);
        lua_setfield(pLuaState, -2, "x");
        lua_pushnumber(pLuaState, scrollingDeltaY);
        lua_setfield(pLuaState, -2, "y");
        lua_pop(pLuaState, 1);

        lua_getfield(pLuaState, -1, "mousePositionNDC");
        lua_pushnumber(pLuaState, mousePositionNDCX);
        lua_setfield(pLuaState, -2, "x");
        lua_pushnumber(pLuaState, mousePositionNDCY);
        lua_setfield(pLuaState, -2, "y");
        lua_pop(pLuaState, 1);

        lua_pushstring(pLuaState, inputText ? inputText : "");
        lua_setfield(pLuaState, -2, "inputText");
        lua_pop(pLuaState, 1);
    }

    void *pTknGfxContext = pTknContext->pGfxContext;

    // Push error handler once at the beginning
    lua_pushcfunction(pLuaState, errorHandler);
    lua_getglobal(pLuaState, "tknEngine");

    // Call update with pTknGfxContext, width, height - Lua controls frame synchronization
    lua_getfield(pLuaState, -1, "update");
    lua_pushlightuserdata(pLuaState, pTknGfxContext);
    lua_pushinteger(pLuaState, extent.width);
    lua_pushinteger(pLuaState, extent.height);
    assertLuaResult(pLuaState, lua_pcall(pLuaState, 3, 1, -6));

    // Get return value if present
    if (lua_isboolean(pLuaState, -1))
    {
        *pShouldQuit = lua_toboolean(pLuaState, -1);
    }
    lua_pop(pLuaState, 1); // Pop return value, errorHandler and tknEngine table
    
    // ========================================================================
    // WebGPU 风格的帧生命周期：
    // 1. tknBeginCommandBuffer - 内部处理 acquire + fence wait + reset
    // 2. recordFrame - Lua 脚本记录绘制命令
    // 3. tknEndCommandBuffer - 内部处理 submit + present
    // ========================================================================
    
    tknBeginCommandBuffer(pTknGfxContext);
    
    // Call recordFrame to record draw commands
    lua_getfield(pLuaState, -1, "recordFrame");
    lua_pushlightuserdata(pLuaState, pTknGfxContext);
    int recordFrameResult = lua_pcall(pLuaState, 1, 0, -3);
    if (recordFrameResult == LUA_OK)
    {
        // Submit and present (handles out-of-date swapchain)
        tknEndCommandBuffer(pTknGfxContext);
    }
    else
    {
        assertLuaResult(pLuaState, recordFrameResult);
    }
    lua_pop(pLuaState, 2);

    // Read imeEnabled from input module
    lua_getglobal(pLuaState, "require");
    lua_pushstring(pLuaState, "input");
    lua_call(pLuaState, 1, 1);
    lua_getfield(pLuaState, -1, "imeEnabled");
    *pImeEnabled = lua_toboolean(pLuaState, -1);
    lua_pop(pLuaState, 2);
}

