#ifndef LUACAR_H
#define LUACAR_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "lunar.h"

#include <agents/Car.h>
#include <agents/CarControl.h>

class LuaCar {
 public:
  // C++ functions
  void setSelf(Car *self, CarControl *control);
  Car *getSelf();
 
  // Lua functions
  LuaCar(lua_State *L);
  int getPosition(lua_State *L);
  int getLane(lua_State *L);
  int getSpeed(lua_State *L);
  int getType(lua_State *L);
  int getGeometry(lua_State *L);
  int isTracked(lua_State *L);
  int isLeftAllowed(lua_State *L);
  int isRightAllowed(lua_State *L);
  int nextTrafficLight(lua_State *L);
  int setAcceleration(lua_State *L);
  int setLaneChange(lua_State *L);
  int setSpeed(lua_State *L);
  int getDestination(lua_State *L);
  ~LuaCar();

  static const char className[];
  static Lunar<LuaCar>::RegType methods[];

 private:
  Car *self;
  CarControl *control;
};

#endif
