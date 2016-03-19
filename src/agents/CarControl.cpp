#include "CarControl.h"
#include "Car.h"

#ifdef LUA
#include <bindings/lua/LuaBinding.h>
#endif

#include <stdlib.h>
#include <math.h>

#define MAX(x,y) (((x)>(y))?(x):(y))
#define MIN(x,y) (((x)<(y))?(x):(y))

CarControl::CarControl(Car *self, gengetopt_args_info *options)
{
  this->a = 0.0;
  this->l = 0;
  this->self = self;

#ifdef LUA
  this->luaCar = new LuaCar(NULL);
  this->luaCar->setSelf(self, this);
  LuaBinding::getInstance().callInit(this->luaCar);
#endif

  /*********************************
   * INITIALIZE YOUR OWN VARIABLES *
   *********************************/
  this->max_speed = 120.0/3.6; // [m/s]
  if (self->getType() == TRUCK)
    this->max_speed = 80.0/3.6;
}

CarControl::~CarControl()
{
#ifdef LUA
  LuaBinding::getInstance().callDestroy(this->luaCar);
  delete this->luaCar;
#endif
}

void CarControl::think(double dt, vector<neighbor_t> &neighbors)
{
#ifdef LUA
  // If there is a lua binding then call the think function there
  if (LuaBinding::getInstance().callThink(this->luaCar, dt, neighbors) != 0) {
    // Do the C++ code
#endif

  /***********************************************
   * THIS IS THE FUNCTION YOU HAVE  TO           *
   * MODIFY TO CHANGE THE CAR BEHAVIOR           *
   * see: http://www.vwi.tu-dresden.de/~treiber/ *
   * for examples                                *
   ***********************************************/

  /* The car ahead is: neighbors[LEAD].car, its distance
     to you is neighbors[LEAD].distance (see Car.h) */

  // Go at the lane speed (if possible) or go to the speed of the leading vehicle
  double lane_speed = self->getLane()->maximum_speed;
  Car *car = neighbors[LEAD].car;
  if (car) lane_speed = car->getSpeed();
  lane_speed = MIN(lane_speed, this->max_speed);
  setAcceleration((lane_speed - self->getSpeed())*1.0);

  // Random lane changes
  if (random_uniform() < 0.1*dt && self->getLane()->allowedRight(self->getPosition()) == 0.0) {
    setLaneChange(1);
  } else if (random_uniform() < 0.1*dt && self->getLane()->allowedLeft(self->getPosition()) == 0.0) {
    setLaneChange(-1);
  } else {
    setLaneChange(0);
  }

#ifdef LUA
  }
#endif
}

double CarControl::getAcceleration()
{
  return a;
}

int CarControl::getLaneChange()
{
  return l;
}

void CarControl::setAcceleration(double a)
{
  this->a = a;
}

void CarControl::setLaneChange(int l)
{
  this->l = l;
}

Car *CarControl::getCar()
{
  return this->self;
}

double CarControl::random_uniform()
{
  return (double)rand()/(double)RAND_MAX;
}

double CarControl::random_normal()
{
  double x1, x2, w;

  do {
    x1 = 2.0 * random_uniform() - 1.0;
    x2 = 2.0 * random_uniform() - 1.0;
    w = x1*x1 + x2*x2;
  } while (w >= 1.0);

  w = sqrt((-2.0 * log(w))/w);
  return(x1*w);
}

#ifdef LUA
LuaCar *CarControl::getLuaCar()
{
  return this->luaCar;
}

void CarControl::setLuaCar(LuaCar *car)
{
  this->luaCar = car;
}
#endif
