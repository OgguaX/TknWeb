#pragma once
#include "tknGfx.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <stdbool.h>
#include <stdint.h>

#define TKN_ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Input state enumeration
typedef enum
{
    INPUT_STATE_PRESSED = 0,
    INPUT_STATE_RELEASED = 1,
    INPUT_STATE_REPEAT = 2
} InputState;

// Extent type for dimensions
typedef struct
{
    uint32_t width;
    uint32_t height;
} Extent2D;

typedef struct TknContext
{
    void *pGfxContext;
    lua_State *pLuaState;
} TknContext;

typedef struct
{
    const char *name;
    uint32_t luaRegCount;
    luaL_Reg *luaRegs;
} LuaLibrary;

// Create Tickernel context (integrates graphics and Lua)
extern void *tknCreateContextPtr(const char *assetsPath, uint32_t luaLibraryCount, LuaLibrary *luaLibraries, uint32_t extensionCount, const char **extensions, void *pSurface, uint32_t width, uint32_t height);

// Destroy Tickernel context
extern void tknDestroyContextPtr(TknContext *pTknContext);

// Update Tickernel context (called each frame)
extern void tknUpdateContext(TknContext *pTknContext, Extent2D extent, uint32_t keyCodeStateCount, InputState *keyCodeStates, uint32_t mouseCodeStateCount, InputState *mouseCodeStates, float scrollingDeltaX, float scrollingDeltaY, float mousePositionNDCX, float mousePositionNDCY, const char *inputText, bool *pShouldQuit, bool *pImeEnabled);
