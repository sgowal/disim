#ifndef LUAINFRASTRUCTURE_H
#define LUAINFRASTRUCTURE_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "lunar.h"

#include <map/Map.h>

class LuaInfrastructure {
 public:
  // C++ functions
  void setSelf(Map *self);
  Map*getSelf();
 
  // Lua functions
  LuaInfrastructure(lua_State *L);
  int getName(lua_State *L);
  int getTimeOfDay(lua_State *L);
  int getLane(lua_State *L);
  int getLanes(lua_State *L);
  int getEntryLanes(lua_State *L);
  int getRoadSensor(lua_State *L);
  int getRoadActuator(lua_State *L);
  ~LuaInfrastructure();

  static const char className[];
  static Lunar<LuaInfrastructure>::RegType methods[];

 private:
  Map *self;
  char timeofday[6];
};

#endif
