#ifndef LUALANE_H
#define LUALANE_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "lunar.h"

#include <map/Map.h>

class LuaLane {
 public:
  // C++ functions
  void setSelf(Lane *self);
 
  // Lua functions
  LuaLane(lua_State *L);
  int getNext(lua_State *L);
  int getPrev(lua_State *L);
  int getLeft(lua_State *L);
  int getRight(lua_State *L);
  int getMergeDirection(lua_State *L);
  int getType(lua_State *L);
  int getGeometry(lua_State *L);
  int getLength(lua_State *L);
  int getRadius(lua_State *L);
  int getAngleSpan(lua_State *L);
  int getSpeedLimit(lua_State *L);
  int getName(lua_State *L);
  int getIndex(lua_State *L);
  int getEntryRate(lua_State *L);
  int setEntryRate(lua_State *L);
  int getEntrySpeed(lua_State *L);
  int setEntrySpeed(lua_State *L);
  int getVehicleCount(lua_State *L);
  ~LuaLane();

  static const char className[];
  static Lunar<LuaLane>::RegType methods[];

 private:
  Lane *self;
};

#endif
