#ifndef LUAROADACTUATOR_H
#define LUAROADACTUATOR_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "lunar.h"

#include <map/Map.h>

class LuaRoadActuator {
 public:
  // C++ functions
  void setSelf(RoadActuator *self);
  RoadActuator *getSelf();
 
  // Lua functions
  LuaRoadActuator(lua_State *L);
  int getColor(lua_State *L);
  int red(lua_State *L);
  int green(lua_State *L);
  int setSpeedLimit(lua_State *L);
  int getType(lua_State *L);
  int getName(lua_State *L);
  int getLane(lua_State *L);
  int getPosition(lua_State *L);
  int getAverageQueueLength(lua_State *L);
  int getVehicleCount(lua_State *L);
  int getInstantQueueLength(lua_State *L);
  ~LuaRoadActuator();

  static const char className[];
  static Lunar<LuaRoadActuator>::RegType methods[];

 private:
  RoadActuator *self;
};

#endif
