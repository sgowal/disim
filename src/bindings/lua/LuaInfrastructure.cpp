#include "LuaInfrastructure.h"
#include "LuaRoadSensor.h"
#include "LuaRoadActuator.h"
#include "LuaLane.h"

#include <string.h>

void LuaInfrastructure::setSelf(Map *self)
{
  this->self = self;
}

Map *LuaInfrastructure::getSelf()
{
  return this->self;
}

LuaInfrastructure::LuaInfrastructure(lua_State *L)
{

}

int LuaInfrastructure::getName(lua_State *L)
{
  lua_pushstring(L, this->self->name);
  return 1;
}

int LuaInfrastructure::getTimeOfDay(lua_State *L)
{
  double t = luaL_checknumber(L,1);
  
  int m = (int)(t/60.0) + this->self->start_time;
  int h = m/60;
  m = m - h*60;
  h %= 24;
  snprintf(this->timeofday, 6, "%02d:%02d", h, m);
  lua_pushstring(L, this->timeofday);
  return 1;
}

int LuaInfrastructure::getLane(lua_State *L)
{
  const char *name = luaL_checkstring(L,1);
  /* Return the first lane with name */
  for (unsigned int i = 0; i < self->segments.size(); i++) {
    Segment *s = self->segments[i];
    for (unsigned int j = 0; j < s->lanes.size(); j++) {
      Lane *l = s->lanes[j];
      if (l->name[0] != '\0' && strcmp(l->name, name) == 0) {
        Lunar<LuaLane>::push(L, l->getLuaLane());
        return 1;
      }
    }
  }
  Lunar<LuaLane>::push(L, NULL);
  return 1;
}

int LuaInfrastructure::getEntryLanes(lua_State *L)
{
  lua_newtable(L);
  /* Push array of entries lanes */
  for (unsigned int i = 0; i < self->entries.size(); i++) {
    Lane *l = self->entries[i];
    lua_pushnumber(L, i+1);
    Lunar<LuaLane>::push(L, l->getLuaLane());
    lua_rawset(L, -3);
  }

  return 1;
}

int LuaInfrastructure::getLanes(lua_State *L)
{
  int cnt = 1;
  const char *name = luaL_checkstring(L,1);
  /* Return all lanes with name */
  lua_newtable(L);
  for (unsigned int i = 0; i < self->segments.size(); i++) {
    Segment *s = self->segments[i];
    for (unsigned int j = 0; j < s->lanes.size(); j++) {
      Lane *l = s->lanes[j];
      if (l->name[0] != '\0' && strcmp(l->name, name) == 0) {
        lua_pushnumber(L, cnt); cnt++;
        Lunar<LuaLane>::push(L, l->getLuaLane());
        lua_rawset(L, -3);
      }
    }
  }
  return 1;
}

int LuaInfrastructure::getRoadSensor(lua_State *L)
{
  const char *name = luaL_checkstring(L,1);

  /* Return the first sensor with name */
  for (unsigned int k = 0; k < self->sensors.size(); k++) {
    RoadSensor *rs = self->sensors[k];
    if (rs->name[0] != '\0' && strcmp(rs->name, name) == 0) {
      Lunar<LuaRoadSensor>::push(L, rs->getLuaRoadSensor());
      return 1;
    }
  }
  Lunar<LuaRoadSensor>::push(L, NULL);
  return 1;
}

int LuaInfrastructure::getRoadActuator(lua_State *L)
{
  const char *name = luaL_checkstring(L,1);

  /* Return the first sensor with name */
  for (unsigned int k = 0; k < self->actuators.size(); k++) {
    RoadActuator *rs = self->actuators[k];
    if (rs->name[0] != '\0' && strcmp(rs->name, name) == 0) {
      Lunar<LuaRoadActuator>::push(L, rs->getLuaRoadActuator());
      return 1;
    }
  }
  Lunar<LuaRoadActuator>::push(L, NULL);
  return 1;
}

LuaInfrastructure::~LuaInfrastructure()
{

}

const char LuaInfrastructure::className[] = "LuaInfrastructure";
Lunar<LuaInfrastructure>::RegType LuaInfrastructure::methods[] = {
  LUNAR_DECLARE_METHOD(LuaInfrastructure, getName),
  LUNAR_DECLARE_METHOD(LuaInfrastructure, getLane),
  LUNAR_DECLARE_METHOD(LuaInfrastructure, getLanes),
  LUNAR_DECLARE_METHOD(LuaInfrastructure, getEntryLanes),
  LUNAR_DECLARE_METHOD(LuaInfrastructure, getRoadSensor),
  LUNAR_DECLARE_METHOD(LuaInfrastructure, getRoadActuator),
  LUNAR_DECLARE_METHOD(LuaInfrastructure, getTimeOfDay),
  {0,0}
};
