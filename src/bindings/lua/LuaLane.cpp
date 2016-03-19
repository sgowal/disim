#include "LuaLane.h"

LuaLane::LuaLane(lua_State *L)
{

}

int LuaLane::getNext(lua_State *L)
{
  if (this->self->next) {
    LuaLane *l = this->self->next->getLuaLane();
    Lunar<LuaLane>::push(L, l);
  } else {
    Lunar<LuaLane>::push(L, NULL);
  }

  return 1;
}

int LuaLane::getPrev(lua_State *L)
{
  if (this->self->prev) {
    LuaLane *l = this->self->prev->getLuaLane();
    Lunar<LuaLane>::push(L, l);
  } else {
    Lunar<LuaLane>::push(L, NULL);
  }

  return 1;
}

int LuaLane::getLeft(lua_State *L)
{
  if (this->self->left) {
    LuaLane *l = this->self->left->getLuaLane();
    Lunar<LuaLane>::push(L, l);
  } else {
    Lunar<LuaLane>::push(L, NULL);
  }

  return 1;
}

int LuaLane::getRight(lua_State *L)
{
  if (this->self->right) {
    LuaLane *l = this->self->right->getLuaLane();
    Lunar<LuaLane>::push(L, l);
  } else {
    Lunar<LuaLane>::push(L, NULL);
  }

  return 1;
}

int LuaLane::getMergeDirection(lua_State *L)
{
  lua_pushnumber(L, this->self->merge_direction);
  return 1;
}

int LuaLane::getType(lua_State *L)
{
  lua_pushnumber(L, this->self->type);
  return 1;
}

int LuaLane::getGeometry(lua_State *L)
{
  lua_pushnumber(L, this->self->segment->geometry);
  return 1;
}

int LuaLane::getLength(lua_State *L)
{
  lua_pushnumber(L, this->self->segment->length);
  return 1;
}

int LuaLane::getRadius(lua_State *L)
{
  lua_pushnumber(L, this->self->radius);
  return 1;
}

int LuaLane::getAngleSpan(lua_State *L)
{
  lua_pushnumber(L, this->self->segment->angle);
  return 1;
}

int LuaLane::getSpeedLimit(lua_State *L)
{
  lua_pushnumber(L, this->self->maximum_speed);
  return 1;
}

int LuaLane::getIndex(lua_State *L)
{
  lua_pushnumber(L, this->self->index);
  return 1;
}

int LuaLane::getEntryRate(lua_State *L)
{
  // Always in veh/h
  lua_pushnumber(L, this->self->entry_rate*3600.0);
  return 1;
}

int LuaLane::setEntryRate(lua_State *L)
{
  // Always in veh/s
  self->entry_rate = luaL_checknumber(L, 1)/3600.0;
  if (self->entry_rate < 0.0) self->entry_rate = 0.0;
  return 0;
}

int LuaLane::getEntrySpeed(lua_State *L)
{
  // Always in km/h
  lua_pushnumber(L, this->self->entry_speed*3.6);
  return 1;
}

int LuaLane::setEntrySpeed(lua_State *L)
{
  // Always in m/s
  self->entry_speed = luaL_checknumber(L, 1)/3.6;
  return 0;
}

int LuaLane::getName(lua_State *L)
{
  lua_pushstring(L, this->self->name);
  return 1;
}

int LuaLane::getVehicleCount(lua_State *L)
{
  lua_pushnumber(L, this->self->cars.size());
  return 1;
}

LuaLane::~LuaLane()
{

}

void LuaLane::setSelf(Lane *self)
{
  this->self = self;
}

const char LuaLane::className[] = "LuaLane";
Lunar<LuaLane>::RegType LuaLane::methods[] = {
  LUNAR_DECLARE_METHOD(LuaLane, getNext),
  LUNAR_DECLARE_METHOD(LuaLane, getPrev),
  LUNAR_DECLARE_METHOD(LuaLane, getLeft),
  LUNAR_DECLARE_METHOD(LuaLane, getRight),
  LUNAR_DECLARE_METHOD(LuaLane, getMergeDirection),
  LUNAR_DECLARE_METHOD(LuaLane, getType),
  LUNAR_DECLARE_METHOD(LuaLane, getGeometry),
  LUNAR_DECLARE_METHOD(LuaLane, getLength),
  LUNAR_DECLARE_METHOD(LuaLane, getRadius),
  LUNAR_DECLARE_METHOD(LuaLane, getAngleSpan),
  LUNAR_DECLARE_METHOD(LuaLane, getSpeedLimit),
  LUNAR_DECLARE_METHOD(LuaLane, getIndex),
  LUNAR_DECLARE_METHOD(LuaLane, getName),
  LUNAR_DECLARE_METHOD(LuaLane, getEntryRate),
  LUNAR_DECLARE_METHOD(LuaLane, setEntryRate),
  LUNAR_DECLARE_METHOD(LuaLane, getEntrySpeed),
  LUNAR_DECLARE_METHOD(LuaLane, setEntrySpeed),
  LUNAR_DECLARE_METHOD(LuaLane, getVehicleCount),
  {0,0}
};
