#include "LuaRoadSensor.h"
#include "LuaLane.h"

void LuaRoadSensor::setSelf(RoadSensor *self)
{
  this->self = self;
}

RoadSensor *LuaRoadSensor::getSelf()
{
  return this->self;
}

LuaRoadSensor::LuaRoadSensor(lua_State *L)
{

}

int LuaRoadSensor::getValue(lua_State *L)
{
  lua_pushnumber(L, (lua_Number)this->self->getResult());
  return 1;
}

int LuaRoadSensor::getVehicleCount(lua_State *L)
{
  if (this->self->type == DENSITY) {
    lua_pushnumber(L, (this->self)->getCount());
  } else {
    lua_pushnumber(L, -1);
  }
  return 1;
}

int LuaRoadSensor::getType(lua_State *L)
{
  lua_pushnumber(L, (lua_Number)this->self->type);
  return 1;
}

int LuaRoadSensor::isOccupied(lua_State *L)
{
  lua_pushnumber(L, (lua_Number)this->self->isOccupied());
  return 1;
}

int LuaRoadSensor::getName(lua_State *L)
{
  lua_pushstring(L, this->self->name);
  return 1;
}

int LuaRoadSensor::getLane(lua_State *L)
{
  Lunar<LuaLane>::push(L, self->lane->getLuaLane());
  return 1;
}

LuaRoadSensor::~LuaRoadSensor()
{

}

const char LuaRoadSensor::className[] = "LuaRoadSensor";
Lunar<LuaRoadSensor>::RegType LuaRoadSensor::methods[] = {
  LUNAR_DECLARE_METHOD(LuaRoadSensor, getValue),
  LUNAR_DECLARE_METHOD(LuaRoadSensor, getVehicleCount),
  LUNAR_DECLARE_METHOD(LuaRoadSensor, isOccupied),
  LUNAR_DECLARE_METHOD(LuaRoadSensor, getName),
  LUNAR_DECLARE_METHOD(LuaRoadSensor, getLane),
  LUNAR_DECLARE_METHOD(LuaRoadSensor, getType),
  {0,0}
};
