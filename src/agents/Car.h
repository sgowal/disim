#ifndef _CAR_H
#define _CAR_H

#include "CarState.h"
#include "CarControl.h"
#include <vector>
#include <cmdline.h>

using namespace std;

class Lane;

/**
 * This enumeration defines the types of neighbors a car might have:
 * either leading or trailing, on the left or the right or remote (through communication for example).
 */
typedef enum {LEAD = 0, TRAIL, LEFT_LEAD, LEFT_TRAIL, RIGHT_LEAD, RIGHT_TRAIL, REMOTE} role_t;

/**
 * This enumeration defines the types of vehicles.
 * A vehicles can be a car or a truck.
 */
typedef enum {CAR, TRUCK} car_t;

/**
 * @brief The neighbor structure.
 *
 * This structure defines the neighbor of a car, giving the actual neighboring car instance,
 * the distance to it and the role it has.
 * @see Car
 * @see role_t
 */
typedef struct neighbor_struct {

  /**
   * Neighboring car pointer.
   */
  Car *car;

  /**
   * Longitudinal distance to that neighboring car.
   */
  double distance;

  /**
   * Role of that neighboring car.
   */
  role_t role;
} neighbor_t;

/**
 * @brief The car class.
 *
 * This class contains the description of a car in our environment.
 * It has a state, a geometry and a control.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class Car {
 public:

  /**
   * The unique constructor.
   * It initializes the state, control and identifier of the vehicle.
   * @param id a unique identifier for the car.
   * @param options a pointer to the parsed commandline options.
   */
  Car(int id, gengetopt_args_info *options);

  /**
   * The destructor.
   * It deletes the state and control.
   */
  ~Car();

  /**
   * Returns the identifier of the car.
   * @return The unique identifier of the car.
   */
  int getID(void);

  /**
   * Returns the X coordinate of the car.
   * The X coordinates should be set as to
   * match the position of the vehicle on this current lane.
   * @see getLane()
   * @see setLane()
   * @see getPosition()
   * @see setPosition()
   * @return The X coordinate.
   */
  double getX(void);

  /**
   * Returns the Y coordinate of the car.
   * The Y coordinates should be set as to
   * match the position of the vehicle on this current lane.
   * @see getLane()
   * @see setLane()
   * @see getPosition()
   * @see setPosition()
   * @return The Y coordinate.
   */
  double getY(void);

  /**
   * Returns the yaw/orientation of the car.
   * The yaw should be set as to
   * match the position of the vehicle on this current lane.
   * @see getLane()
   * @see setLane()
   * @see getPosition()
   * @see setPosition()
   * @return The orientation of the vehicle.
   */
  double getYaw(void);
  
  /**
   * Returns the steering angle of the front wheels of the car.
   * This is not used in this project.
   * @return The steering angle.
   */
  double getSteeringAngle(void);

  /**
   * Returns the current speed of the vehicle.
   * @return The speed.
   */
  double getSpeed(void);

  /**
   * Sets the X coordinate.
   * @see getX()
   * @param x the new x position of the car.
   */
  void setX(double x);

  /**
   * Sets the Y coordinate.
 * @see getY()
   * @param y the new y position of the car.
   */
  void setY(double y);

  /**
   * Sets the orientation.
   * @see getYaw()
   * @param yaw the new yaw of the car.
   */
  void setYaw(double yaw);

/**
   * Sets the speed.
 * @see getSpeed()
   * @param v the new speed of the car.
   */
  void setSpeed(double v);

  /**
   * Sets the steering angle.
   * @see getSteeringAngle()
   * @param s the new steering angle.
   */
  void setSteeringAngle(double s);

  /**
   * Returns a pointer on the X coordinate variable.
   * This is useful to track a specific vehicle without having poll
   * for this data.
   * @return The pointer to X.
   */
  double *getXPointer();

  /**
   * Returns a pointer on the Y coordinate variable.
   * This is useful to track a specific vehicle without having poll
   * for this data.
   * @return The pointer to Y.
   */
  double *getYPointer();

  /**
   * Returns a pointer on the orientation variable.
   * This is useful to track a specific vehicle without having poll
   * for this data.
   * @return The pointer to the yaw.
   */
  double *getYawPointer();

  /**
   * Returns a pointer on the complete state variable.
   * This is useful to track a specific vehicle without having poll
   * for this data.
   * @return The pointer to a CarState.
   */
  CarState *getState();

  /**
   * Returns a pointer to the control variable.
   * This might not be very useful indeed...
   * @return The pointer to a CarControl.
   */
  CarControl *getControl();

  /**
   * Returns the acceleration computed by the control.
   * The simulator should use this information to update the car position.
   * @return The commanded acceleration.
   */
  double getAcceleration();

  /**
   * Returns the lane shift computed by the control.
   * A negative value should move the car on the left lane and
   * a positive value to the right (0 means stay on the current alane).
   * @return The desired new lane offset.
   */
  int getLaneChange();

  /**
   * Returns the box geometry of the car.
   * @param front distance from the rear axle to the front bumper.
   * @param rear distance from the rear axle to the rear bumper.
   * @param side distance from the center to the side.
   * @param top height of the car.
   */
  void getCarGeometry(double *front = NULL, double *rear = NULL, double *side = NULL, double *top = NULL);

  /**
   * This function directly calls the control.
   * @param dt time difference between the last call and now.
   * @param neighbors other nearby cars on which the control can plan.
   */
  void simulate(double dt, vector<neighbor_t> &neighbors, bool fake = false);

  /**
   * Sets the current lane of the car. This does not
   * move the car in the state space, but only sets the
   * current expected lane where car think it navigates.
   * @see getLane()
   * @param l current lane
   */
  void setLane(Lane *l);

  /**
   * Sets the position of the vehicle on the current lane.
   * This position is either in meters or in radians depending
   * on the current lane geometry (either straight or circular).
   * @see getPosition()
   * @param p position of the car (meters or radians)
   */
  void setPosition(double p);

  /**
   * Returns the current lane on which the car navigates.
   * @see setLane()
   * @return The current lane.
   */
  Lane *getLane();

  /**
   * Returns the position of the car on the current lane.
   * @see setPosition()
   * @return The position of the car.
   */
  double getPosition();

  /**
   * Returns the destination lane of the car.
   * @return The lane where the car should exit the highway.
   */
  Lane *getDestination();

  /**
   * Sets the destination lane of the car.
   * @param lane destination lane
   */
  void setDestination(Lane *lane);

  /**
   * Returns the type of vehicle
   */
  car_t getType();

  /**
   * Notifies the car that it is being tracked.
   */
  void trackingOn();

  /**
   * Notifies that it is not being tracked anymore.
   */
  void trackingOff();

  /**
   * Returns whether it is being tracked.
   * @return true if the car is being tracked.
   */
  bool isTracked();

  /**
   * Returns how long this car has been running.
   * @return time spent since birth in seconds.
   */
  double getTimeAlive();

  /**
   * Resets the time alive to zero.
   */
  void resetTimeAlive();

  /**
   * DO NOT USE.
   * Special flag to notify that the car should be
   * deleted from the simulation stack at the next loop.
   */
  bool delete_me;

 private:
  int id;
  CarState *state;
  CarControl *control;

  Lane *destination;

  double front;
  double rear;
  double side;
  double top;

  car_t type;

  bool is_tracked;

  double time_alive;
};

#endif
