#ifndef LUAROADSENSOR_H
#define LUAROADSENSOR_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "lunar.h"

#include <map/Map.h>

class LuaRoadSensor {
 public:
  // C++ functions
  void setSelf(RoadSensor *self);
  RoadSensor *getSelf();
 
  // Lua functions
  LuaRoadSensor(lua_State *L);
  int getValue(lua_State *L);
  int getVehicleCount(lua_State *L);
  int isOccupied(lua_State *L);
  int getType(lua_State *L);
  int getName(lua_State *L);
  int getLane(lua_State *L);
  ~LuaRoadSensor();

  static const char className[];
  static Lunar<LuaRoadSensor>::RegType methods[];

 private:
  RoadSensor *self;
};

#endif
