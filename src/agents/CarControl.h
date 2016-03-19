#ifndef CAR_CONTROL_H
#define CAR_CONTROL_H

#include <vector>
#include <cmdline.h>

#ifdef LUA
class LuaCar;
#endif

using namespace std;

class Car;
class CarState;
struct neighbor_struct;

/**
 * @brief The car control class.
 *
 * This class contains the control algorithm handling the
 * car motion.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class CarControl {
 public:

  /**
   * The unique constructor.
   * It initializes the control algorithm.
   * @param self A pointer to the car it controls.
   * @param options The commandline options
   */
  CarControl(Car *self, gengetopt_args_info *options);

  /**
   * The destructor.
   * It cleans the control algorithm and erases all structures
   * used by the control mechanism.
   */
  ~CarControl();

  /**
   * The most important function.
   * It sets the acceleration and lane offset that the car should
   * execute until the next time step.
   * @param dt The time step duration.
   * @param neighbors The neighboring cars (a.k.a. the perception).
   * @see setAcceleration()
   * @see setLaneOffset()
   */
  void think(double dt, vector<struct neighbor_struct> &neighbors);

  /**
   * Returns the acceleration set by the think function.
   * @see think()
   * @return The acceleration.
   */
  double getAcceleration();

  /**
   * Returns the lane offset set the think function.
   * @see think()
   * @return A negative integer if the car is willing the go on the left lane, positive on the right lane and 0 otherwise.
   */
  int getLaneChange();

  /**
   * Sets the acceleration wanted.
   * @param a The wanted acceleration.
   */
  void setAcceleration(double a);

  /**
   * Sets the lane offset.
   * @param l The lane offset. l should be negative if the car is willing the go on the left lane, positive on the right lane and 0 otherwise.
   */
  void setLaneChange(int l);

  /**
   * Gets the corresponding car.
   * @return A pointer to the car controller by this CarController
   */
  Car *getCar();

#ifdef LUA
  /**
   * Gets the LUA object corresponding to this CarControl.
   * @return A pointer to the LuaCar object.
   */
  LuaCar *getLuaCar();

  /**
   * Sets the LUA object corresponding to this CarControl.
   * @param car A pointer to the LuaCar object.
   */
  void setLuaCar(LuaCar *car);
#endif

 private:
  double a;
  int l;
  Car *self;
  double random_uniform();
  double random_normal();

#ifdef LUA
  LuaCar *luaCar;
#endif

  /**********************************
   * DEFINE HERE YOUR OWN VARIABLES *
   **********************************/
  double max_speed;
};

#endif
