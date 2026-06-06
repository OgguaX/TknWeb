#pragma once
#include "lua.h"

// Bind TickernelGfx functions to Lua as the 'tknGfx' global table
extern void bindTknGfxFunctions(lua_State *pLuaState);
