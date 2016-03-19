#include "LuaRoadActuator.h"
#include "LuaLane.h"

void LuaRoadActuator::setSelf(RoadActuator *self)
{
  this->self = self;
}

RoadActuator *LuaRoadActuator::getSelf()
{
  return this->self;
}

LuaRoadActuator::LuaRoadActuator(lua_State *L)
{

}

int LuaRoadActuator::getColor(lua_State *L)
{
  if (this->self->type == TRAFFICLIGHT) {
    lua_pushnumber(L, (lua_Number)(static_cast<TrafficLightActuator *>(this->self))->color());
  } else {
    lua_pushnumber(L, -1);
  }
  return 1;
}

int LuaRoadActuator::red(lua_State *L)
{
  if (this->self->type == TRAFFICLIGHT) {
    (static_cast<TrafficLightActuator *>(this->self))->red();
  }
  return 0;
}

int LuaRoadActuator::green(lua_State *L)
{
  if (this->self->type == TRAFFICLIGHT) {
    (static_cast<TrafficLightActuator *>(this->self))->green();
  }
  return 0;
}

int LuaRoadActuator::getAverageQueueLength(lua_State *L)
{
  if (this->self->type == TRAFFICLIGHT) {
    lua_pushnumber(L, (this->self)->getResult());
  } else {
    lua_pushnumber(L, -1);
  }
  return 1;
}

int LuaRoadActuator::getInstantQueueLength(lua_State *L)
{
  if (this->self->type == TRAFFICLIGHT) {
    bool r;
    lua_pushnumber(L, (this->self)->getInstantResult(&r));
    lua_pushboolean(L, r);
  } else {
    lua_pushnumber(L, -1);
    lua_pushboolean(L, false);
  }
  return 2;
}

int LuaRoadActuator::getVehicleCount(lua_State *L)
{
  if (this->self->type == TRAFFICLIGHT) {
    lua_pushnumber(L, (this->self)->getCount());
  } else {
    lua_pushnumber(L, -1);
  }
  return 1;
}

int LuaRoadActuator::setSpeedLimit(lua_State *L)
{
  if (this->self->type == SPEEDLIMIT) {
    double maxspeed = luaL_checknumber(L, 1)/3.6; // [m/s]
    (static_cast<SpeedLimitActuator *>(this->self))->setSpeedLimit(maxspeed);
  }
  return 1;
}

int LuaRoadActuator::getType(lua_State *L)
{
  lua_pushnumber(L, (lua_Number)this->self->type);
  return 1;
}

int LuaRoadActuator::getPosition(lua_State *L)
{
  lua_pushnumber(L, this->self->position);
  return 1;
}

int LuaRoadActuator::getName(lua_State *L)
{
  lua_pushstring(L, this->self->name);
  return 1;
}

int LuaRoadActuator::getLane(lua_State *L)
{
  Lunar<LuaLane>::push(L, self->lane->getLuaLane());
  return 1;
}

LuaRoadActuator::~LuaRoadActuator()
{

}

const char LuaRoadActuator::className[] = "LuaRoadActuator";
Lunar<LuaRoadActuator>::RegType LuaRoadActuator::methods[] = {
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getColor),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, red),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, green),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, setSpeedLimit),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getType),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getName),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getLane),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getPosition),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getAverageQueueLength),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getInstantQueueLength),
  LUNAR_DECLARE_METHOD(LuaRoadActuator, getVehicleCount),
  {0,0}
};
