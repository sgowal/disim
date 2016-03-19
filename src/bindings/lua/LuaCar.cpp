#include "LuaCar.h"
#include "LuaLane.h"
#include "LuaRoadActuator.h"

LuaCar::LuaCar(lua_State *L)
{

}

int LuaCar::getPosition(lua_State *L)
{
  lua_pushnumber(L, (lua_Number)this->self->getPosition());
  return 1;
}

int LuaCar::getLane(lua_State *L)
{
  LuaLane *l = this->self->getLane()->getLuaLane();
  Lunar<LuaLane>::push(L, l);
  return 1;
}


int LuaCar::getDestination(lua_State *L)
{
  if (this->self->getDestination()) {
    LuaLane *l = this->self->getDestination()->getLuaLane();
    Lunar<LuaLane>::push(L, l);
  } else {
    Lunar<LuaLane>::push(L, NULL);
  }
  return 1;
}
int LuaCar::isLeftAllowed(lua_State *L)
{
  double p = this->self->getLane()->allowedLeft(this->self->getPosition());
  lua_pushboolean(L, (p == 0.0));
  return 1;
}

int LuaCar::isRightAllowed(lua_State *L)
{
  double p = this->self->getLane()->allowedRight(this->self->getPosition());
  lua_pushboolean(L, (p == 0.0));
  return 1;
}

int LuaCar::nextTrafficLight(lua_State *L)
{
  int index = -1;
  Lane *l = this->self->getLane();
  double mindist = l->segment->length;
  for (unsigned int i = 0; i < l->actuators.size(); i++) {
    double dist = l->actuators[i]->position - self->getPosition();
    if (dist > 0.0 && dist < mindist) {
      mindist = dist;
      index = i;
    }
  }

  if (index == -1)
    Lunar<LuaRoadActuator>::push(L, NULL);
  else
    Lunar<LuaRoadActuator>::push(L, l->actuators[index]->getLuaRoadActuator());
  
  return 1;
}

int LuaCar::getSpeed(lua_State *L)
{
  lua_pushnumber(L, (lua_Number)this->self->getSpeed());
  return 1;
}

int LuaCar::getType(lua_State *L)
{
  lua_pushnumber(L, (lua_Number)this->self->getType());
  return 1;
}

int LuaCar::isTracked(lua_State *L)
{
  lua_pushboolean(L, this->self->isTracked());
  return 1;
}

int LuaCar::getGeometry(lua_State *L)
{
  double f, r, s, t;
  this->self->getCarGeometry(&f, &r, &s, &t);
  lua_pushnumber(L, f);
  lua_pushnumber(L, r);
  lua_pushnumber(L, s);
  lua_pushnumber(L, t);
  return 4;
}

int LuaCar::setAcceleration(lua_State *L)
{
  control->setAcceleration(luaL_checknumber(L, 1));
  return 0;
}

int LuaCar::setLaneChange(lua_State *L)
{
  control->setLaneChange(luaL_checknumber(L, 1));
  return 0;
}

int LuaCar::setSpeed(lua_State *L)
{
  self->setSpeed(luaL_checknumber(L, 1));
  control->setAcceleration(0.0);
  return 0;
}

LuaCar::~LuaCar()
{

}

void LuaCar::setSelf(Car *self, CarControl *control)
{
  this->self = self;
  this->control = control;
}

Car *LuaCar::getSelf()
{
  return this->self;
}

const char LuaCar::className[] = "LuaCar";
Lunar<LuaCar>::RegType LuaCar::methods[] = {
  LUNAR_DECLARE_METHOD(LuaCar, getPosition),
  LUNAR_DECLARE_METHOD(LuaCar, getLane),
  LUNAR_DECLARE_METHOD(LuaCar, getSpeed),
  LUNAR_DECLARE_METHOD(LuaCar, getType),
  LUNAR_DECLARE_METHOD(LuaCar, getGeometry),
  LUNAR_DECLARE_METHOD(LuaCar, isTracked),
  LUNAR_DECLARE_METHOD(LuaCar, isLeftAllowed),
  LUNAR_DECLARE_METHOD(LuaCar, isRightAllowed),
  LUNAR_DECLARE_METHOD(LuaCar, nextTrafficLight),
  LUNAR_DECLARE_METHOD(LuaCar, setAcceleration),
  LUNAR_DECLARE_METHOD(LuaCar, setSpeed),
  LUNAR_DECLARE_METHOD(LuaCar, setLaneChange),
  LUNAR_DECLARE_METHOD(LuaCar, getDestination),
  {0,0}
};
