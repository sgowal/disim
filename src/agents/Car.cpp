#include "Car.h"

#include <stdlib.h>
#include <map/Map.h>

#define  CAR_VEHICLE_WIDTH             2.1   // [m]
#define  CAR_VEHICLE_HEIGHT            2.0   // [m]
#define  CAR_VEHICLE_LENGTH            5.0   // [m]
#define  CAR_DIST_REAR_TO_REAR_AXLE    1.0   // [m]
#define  CAR_DIST_REAR_AXLE_TO_FRONT   (CAR_VEHICLE_LENGTH - CAR_DIST_REAR_TO_REAR_AXLE) // [m]

#define  TRUCK_VEHICLE_WIDTH             3.0   // [m]
#define  TRUCK_VEHICLE_HEIGHT            3.0   // [m]
#define  TRUCK_VEHICLE_LENGTH            8.0   // [m]
#define  TRUCK_DIST_REAR_TO_REAR_AXLE    1.8   // [m]
#define  TRUCK_DIST_REAR_AXLE_TO_FRONT   (TRUCK_VEHICLE_LENGTH - TRUCK_DIST_REAR_TO_REAR_AXLE) // [m]

Car::Car(int identifier, gengetopt_args_info *options)
{
  id = identifier;
  is_tracked = false;
  delete_me = false;
  destination = NULL;
  time_alive = 0.0;

  type = CAR;
  double r = (double)rand()/(double)RAND_MAX;
  if (options && r < options->truck_arg) {
    type = TRUCK;
  }

  state = new CarState();
  control = new CarControl(this, options);

  if (type == CAR) {
    front = CAR_DIST_REAR_AXLE_TO_FRONT;
    rear = CAR_DIST_REAR_TO_REAR_AXLE;
    side = CAR_VEHICLE_WIDTH/2.0;
    top = CAR_VEHICLE_HEIGHT;
  } else {
    front = TRUCK_DIST_REAR_AXLE_TO_FRONT;
    rear = TRUCK_DIST_REAR_TO_REAR_AXLE;
    side = TRUCK_VEHICLE_WIDTH/2.0;
    top = TRUCK_VEHICLE_HEIGHT;
  }
}

Car::~Car()
{
  delete state;
  delete control;
}

int Car::getID(void)
{
  return id;
}

double Car::getX(void)
{
  return state->x;
}

double Car::getY(void)
{
  return state->y;
}

double Car::getYaw(void)
{
  return state->yaw;
}

double Car::getSpeed(void)
{
  return state->speed;
}

double Car::getSteeringAngle(void)
{
  return state->steering_angle;
}

double *Car::getXPointer()
{
  return &state->x;
}

double *Car::getYPointer()
{
  return &state->y;  
}

double *Car::getYawPointer()
{
  return &state->yaw;
}

CarState *Car::getState(void)
{
  return state;
}

CarControl *Car::getControl()
{
  return control;
}

double Car::getAcceleration()
{
  return control->getAcceleration();
}

int Car::getLaneChange()
{
  return control->getLaneChange();
}

void Car::setX(double x)
{
  state->x = x;
}

void Car::setY(double y)
{
  state->y = y;
}

void Car::setYaw(double yaw)
{
  state->yaw = yaw;
}

void Car::setSpeed(double speed)
{
  state->speed = speed;
}

void Car::setSteeringAngle(double s)
{
  state->steering_angle = s;
}

void Car::setLane(Lane *l)
{
  state->lane = l;
}

void Car::setPosition(double p)
{
  state->position = p;
}

Lane *Car::getLane()
{
  return state->lane;
}

double Car::getPosition()
{
  return state->position;
}

car_t Car::getType()
{
  return this->type;
}

Lane *Car::getDestination()
{
  return this->destination;
}

void Car::setDestination(Lane *lane)
{
  this->destination = lane;
}

double Car::getTimeAlive()
{
  return this->time_alive;
}

void Car::resetTimeAlive()
{
  this->time_alive = 0.0;
}

void Car::getCarGeometry(double *front, double *rear, double *side, double *top)
{
  if (front) *front = this->front;
  if (rear) *rear = this->rear;
  if (side) *side = this->side;
  if (top) *top = this->top;
}

void Car::trackingOn()
{
  is_tracked = true;
}

void Car::trackingOff()
{
  is_tracked = false;
}

bool Car::isTracked()
{
  return is_tracked;
}

void Car::simulate(double dt, vector<neighbor_t> &neighbors, bool fake)
{
  control->think(dt, neighbors);

  if (!fake) time_alive += dt;
}
