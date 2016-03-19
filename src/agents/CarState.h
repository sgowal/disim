#ifndef CAR_STATE_H
#define CAR_STATE_H

#include <map/Map.h>

/**
 * @brief The car state class.
 *
 * This class holds the state variables of the car it corresponds to.
 * This class varaibles are automatically set by the simulator.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class CarState {

 public:
  /**
   * The unique constructor
   */
  CarState();

  /**
   * The destructor
   */
  ~CarState();

  /**
   * The x coordinate in meters.
   */
  double x;

  /**
   * The y coordinate in meters.
   */
  double y;

  /**
   * The orientation with respect to the z axis (from x axis to y axis) in radians.
   */
  double yaw;

  /**
   * The steering angle of the front wheels in radians.
   */
  double steering_angle;

  /**
   * The speed of the car in meters per seconds.
   */
  double speed;

  /**
   * The current lane on which the car navigates
   */
  Lane *lane;

  /**
   * The current position of the vehicle on the current lane.
   * When the lane is straight, this is in [m].
   * When the lane is circular, this is in [rad].
   */
  double position;
};

#endif
