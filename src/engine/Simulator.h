#ifndef _SIMULATOR_H
#define _SIMULATOR_H

/**
 * @mainpage Disim
 *
 * @image html img_00.png
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 *
 * @section intro Introduction
 *
 * Disim is a lightweight highway simulator that enables
 * you to emulate a complete traffic over several kilometers.
 * You can define your own road network and change the
 * different entry rates and road conditions. Disim also
 * offers the possiblity to add road sensors to measure
 * the density, speed and traffic flow and log this data.
 * 
 * This simulator display is based on OpenGL and FLTK which
 * make it particularly fast and enjoyable to use.
 * Another particularity is that the simulator can be run
 * without graphical interface (making it fast).
 *
 * Disim offers a nice API enabling everyone to modify the
 * individual car behavior using either C++ and LUA.
 * As LUA is interpreted it requires no compilation and
 * allows quick prototyping of behaviors.
 *
 * Future extensions will parallelize the computation when
 * possible (since traffic is highly parallelizable).
 * 
 * @section install Installation
 *
 * @subsection linux Linux
 *
 * @li Download Disim: http://www.disim.org/disim.tar.gz (not working yet).
 * @li Extract Disim in your favorite folder.
 * @li Make sure that <em>gengetopt</em> is installed on your system.
 *     To install <em>gengetopt</em> under Ubuntu: <em>sudo apt-get install gengetopt</em>.
 *     If you do not wish to install gengetopt, you can modify the <em>Makefile.include</em> file
 *     to specify that gengetopt is not present on your system.
 * @li If you want to use the graphical interface, make sure OpenGL and FLTK 1.1 are installed (You can
 *     modify the <em>Makefile.include</em> file to specify that you do not wish to use the graphical interface).
 *     To install OpenGL under Ubutu: <em>sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev</em>.
 *     To install FLTK under Ubuntu: <em>sudo apt-get install libfltk1.1-dev</em>.
 *     If you do not wish to install FLTK, you can modify the <em>Makefile.include</em> file
 *     to specify that FLTK is not present on your system.
 * @li If you want to use the LUA scripting capabilities, make Lua 5.1 is installed.
 *     To install Lua under Ubuntu: <em>sudo apt-get install liblua5.1-0-dev</em>.
 *     If you do not wish to use LUA, you can specify so in the <em>Makefile.include</em> file.
 * @li Open a terminal, navigate to the Disim folder and type <em>make</em>.
 * @li You can now start disim with the command: <em>./disim</em>.
 *
 * @subsection mac MacOS X
 *
 * @li Download Disim: http://www.disim.org/disim.tar.gz (not working yet).
 * @li Extract Disim in your favorite folder.
 * @li Make sure that <em>gengetopt</em> is installed on your system.
 *     To install gengetopt under MacOS, download and install the extremely useful Fink (http://www.finkproject.org/),
 *     then you can open a Terminal and type: <em>fink -b install gengetopt</em>.
 *     To install Fink, it is necessary to install X-Code on your system: http://developer.apple.com/technologies/tools/xcode.html.
 * @li If you want to use the graphical interface, make sure OpenGL and FLTK 1.1 are installed.
 *     To install FLTK, open a terminal and type: <em>fink -b install fltk-aqua</em>.
 *     To install OpenGL, you need to install X-Code on your system.
 *     If you do not wish to use the graphical interface, you can specify so by modifying the <em>Makefile.include</em> file.
 * @li If you want to use the LUA scripting capabilities, Lua 5.1 must be installed.
 *     To install Lua, type: <em>fink -b install liblua5.1-dev</em>.
 *     If you do not wish to use LUA, you can specify so in the <em>Makefile.include</em> file.
 * @li Open a terminal, navigate to the Disim folder and type <em>make</em>.
 * @li You can now start disim with the command: <em>./disim</em>.
 *
 * @subsection win Window
 *
 * Windows is not supported for the moment, to use Disim your need to be able to compile C++ code.
 * You may need to have Cygwin and properly install OpenGL, FLTK, gengetopt and Lua.
 * Under cygwin, if your installation is correctly configured you should be able to open a
 * cygwin terminal follow the instruction of the Linux distribution.
 *
 * @section use Usage
 *
 * Disim offers several commandline arguments that are self-explanatory
 * (<em>./disim --help</em> for help on those).
 *
 * After starting Disim a generic highway map will be loaded and cars will
 * start to follow it using a "wrong" behavior. You can look
 * at http://www.vwi.tu-dresden.de/~treiber/ for ways to
 * implement a proper lane following and lane keeping strategy.
 *
 * The graphical interface enables you to navigate through the map using the
 * mouse. Drag-and-drop with the left button of the mouse to orient the camera,
 * use the right button to zoom in and out and the third button (or both the left and right
 * buttons) to translate horizontally. If you hit SHIFT while performing
 * those gestures, you can do them faster.
 *
 * You can use the menu at the top to open a new map, exit the application or
 * adjust different vueing alternative. The best is to try for yourself.
 *
 * @image html img_01.png You can adjust the weather conditions for example.
 *
 * If you CTRL+click you can select a location in space, Disim
 * will display additional information about that location (if it is available).
 * In particular selecting an entry point and using the information menu will enable you
 * to change the entry rate live. This interface is still elementary and we
 * apologize for it. Alternatively you can use the <em>Action>Get info</em>
 * menu to bring the information window again (without CTRL+click) if you closed it.
 *
 * @section map Constructing your own maps
 *
 * Constructing maps for Disim is quite intuitive and easy. As an example you can
 * look at the files <em>maps/default.map</em> and <em>maps/loop.txt</em>.
 *
 * The map file should start will the following preamble:
 *
 * @code
 * $NAME,The default map
 * $LANE_WIDTH,3.5
 * @endcode
 *
 * This signals the name of the map and displayed lane width
 * that your lanes have. The default lane width if this line is not present is 3.5 meters.
 *
 * Then you can declare your highway segments in the direction of the traffic flow. Each segment
 * starts with a mandatory line:
 * @code
 * $SEGMENT,straight,X
 * @endcode
 * to indicate a straight segment of X meters, or
 * @code
 * $SEGMENT,circular,X,Y
 * @endcode
 * to indicate a circular segment with a radius of X meters to the left-most lane
 * and an angle span of Y degrees. If Y is positive the lane will turn to the right and if
 * it is negative to the left.
 *
 * Next, you can optionally indicate if your segment should contain entry lanes on the left or right:
 * @code
 * $TYPE,entry,[left|right]
 * @endcode
 * or exit lanes on the left or right:
 * @code
 * $TYPE,exit,[left|right]
 * @endcode
 * The second argument <em>[left|right]</em> is optional (right is the default).
 *
 * You can then specify the speed limit on the segment:
 * @code
 * $SPEED,X
 * @endcode
 * where X is the speed limit in kilometer per hour.
 *
 * It is then mandatory to declare the number of lanes to keep from the previous segment
 * and the number of new lanes (for entry and exit segment) with:
 * @code
 * $NUM_LANES,X,Y
 * @endcode
 * where X is the number of lanes to keep and Y the number of new lanes. In some
 * circumstances it might be useful to change the number of lanes from one segment to
 * the other without having an entry or exit segment. In such an event you can declare
 * the segment type to be none,[left|right] to be able to specify the lanes to be
 * on the left or on the right of the previous segment, e.g.:
 * @code
 * $TYPE,none,left
 * $NUM_LANES,2
 * @endcode
 * This will continue the highway with 2 lanes on the left of the previous segment.
 *
 * Finally if your segment is an entry segment, you can specify the entry rate of
 * specific lanes with:
 * @code
 * $LANE,X,Y
 * @endcode
 * where X is the lane index (0 is the left-most lane, 1 would be the lane on the right of the left-most
 * lane, ...) and Y is the entry rate in vehicles per hour.
 *
 * Finally if you decide that you want to place a road sensor on this segment, you can
 * declare a sensor named <em>name</em> with:
 * @code
 * $DENSITY_SENSOR,name,X,Y
 * @endcode
 * to record traffic density (in vehicles per kilometer) with X being the entry position of
 * sensor and Y the exit position (if the segment is straight X is in meters, if it is
 * circular X is in degrees). Or
 * @code
 * $SPEED_SENSOR,name,X
 * @endcode
 * to record time mean speed data in kilometers per hour with X being the position of the detector
 * on the segment. Or
 * @code
 * $FLOW_SENSOR,name,X
 * @endcode
 * to record the flow in vehicles per hour. The sensors average the data from the traffic
 * every minute and dump this value in a log file (if the option <em>--record</em> is given to
 * Disim).
 *
 * Also, if you want to have a closed loop you can use
 * @code
 * $CLOSE_THE_LOOP
 * @endcode
 * at the end of the file to link the first segment with the last.
 *
 * @section code Creating your own car behavior
 *
 * @subsection cpp In C++
 *
 * To do this, you need to be able to program in C++, but the only thing you need to
 * modify are the <em>agents/CarControl.h</em> and <em>agents/CarControl.cpp</em> files.
 * The CarControl class offers a function called <em>think</em> (have a look here: CarControl).
 * This function takes as input the time step <em>dt</em> in seconds
 * and a vector containing the neighboring cars. This vector contains at least 6 elements
 * indicating the leading and trailing vehicles of the current lane and both the left and right lanes
 * and the longitudinal distance to those vehicles. You can access the vector elements with
 * @code
 * neighbors[LEAD]
 * neighbors[TRAIL]
 * neighbors[LEFT_LEAD]
 * neighbors[LEFT_TRAIL]
 * neighbors[RIGHT_LEAD]
 * neighbors[RIGHT_TRAIL]
 * @endcode
 * You can access the different properties of the corresponding vehicles using the car pointer:
 * @code
 * neighbors[LEAD].car->getType()
 * neighbors[LEAD].car->getSpeed()
 * neighbors[LEAD].car->getCarGeometry(&front, &rear, &side, &top)
 * @endcode
 * and the longitudinal distance between your car rear axle and the neighbor's rear axle:
 * @code
 * neighbors[LEAD].distance
 * @endcode
 * If <em>neighbors[X].car</em> is <em>NULL</em>, the neighbor is outside the region of
 * visibility.
 *
 * Depending the neighbors information and the current segment/lane geometry (accessible
 * via the <em>self</em> pointer pointing to the current car), the <em>think</em> function has
 * to actuate the acceleration and next lane to go to using the helper functions:
 * @code
 * setAcceleration(double a) // a is the acceleration in m/s2
 * setLaneOffset(int offset) // offset is 1 to change to the right, -1 to the left and 0 otherwise
 * @endcode
 *
 * When you finished coding your controller, you can type <em>make</em> and restart Disim.
 *
 * @subsection lua In LUA
 *
 * Creating your car behavior in LUA has several advantages: compilation is not needed,
 * a simplified API is available and you benefit from all the LUA capabilities (tables,
 * anonymous functions, dictionaries, ...). To start off, you can create a file (e.g. my_controller.lua) or
 * look at the file <em>scripts/default.lua</em>.
 * This lua script needs to provide three functions described below:
 * @code
 * function init(self)
 * @endcode
 *
 * This function takes as input a single variable. This variable is a pointer on
 * the current car being initialized (this car just came into the highway). Note
 * that if the script needs to save specific data related to that car, it should
 * do so in an external variable or array (e.g. vars[self] = { max_speed = 10.0 }).
 * The script is not individually loaded for each vehicle but each vehicle is
 * the same functions. Hence self is bound to be different at each call of this
 * function. This function does not return any values.
 *
 * @code
 * function think(self, dt, neighbors)
 * @endcode
 *
 * This function takes three variables as input:
 * @li <b>self</b>: Just like the input function, this is a pointer to the car
 *                  currently being controlled.
 * @li <b>dt</b>: This is the time-step duration in seconds since the last call
 *                to that function for that specific car.
 * @li <b>neighbors</b>: This array contains at least 6 elements that you can
 *                       access with the <em>LEAD, TRAIL, LEFT_LEAD, LEFT_TRAIL, RIGHT_LEAD,
 *                       RIGHT_TRAIL</em> keys (e.g. <em>neighbors[LEFT_LEAD]</em>). Each element
 *                       of this array is an array indexed with 2 keys: <em>car</em> and <em>distance</em>.
 *                       Hence you can access the car pointer of the leading vehicle with <em>neighbors[LEAD].car</em>
 *                       and the longitudinal distance to that car with <em>neighbors[LEAD].distance</em>.
 *
 * This function needs to return 2 values: first an acceleration (that will be applied to the car
 * until the next time-step) and a lane change command. As an example, for make the car
 * accelerate at 2.0 m/s2 and change to the left lane, one would do: <em>return 2.0, -1</em>.
 * To change to the right lane breaking at -1.0 m/s2 one could do <em>return -1.0, 1</em> and to not do anything
 * <em>return 0, 0</em>.
 *
 * @code
 * function destroy(self)
 * @endcode
 *
 * This function takes as input a single variable. This variable is a pointer on
 * the current car being destroyed (this car just went out of the highway). In this
 * function you should clean up any variable that has been initialized by init for that
 * vehicle.
 *
 * The car object (LuaCar) has 5 functions that you can access:
 * @code
 * function LuaCar:getPosition() -- Returns the position of the car on its current segment.
 *                                  This position can be in meters or radians (see Car)
 * function LuaCar:getLane()     -- Returns a lane object (LuaLane) corresponding to the lane
 *                                  the car is navigating in.
 * function LuaCar:getSpeed()    -- Returns the current speed on the car.
 * function LuaCar:getType()     -- Returns the type of car (CAR or TRUCK).
 * function LuaCar:getGeometry() -- Returns 4 values representing the distance from the middle
 *                                  of the rear axle to the front bumper, rear bumper, side and height.
 * @endcode
 *
 * The lane object (LuaLane) has 10 functions:
 *
 * @code
 * function LuaLane:getNext()           -- Returns the next lane.
 * function LuaLane:getPrev()           -- Returns the previous lane.
 * function LuaLane:getLeft()           -- Returns the left lane.
 * function LuaLane:getRight()          -- Returns the right lane.
 * function LuaLane:getMergeDirection() -- Returns the direction to merge to (if any).
 * function LuaLane:getType()           -- Returns the type (ENTRY, EXIT or NONE).
 * function LuaLane:getGeometry()       -- Returns the geometry (STRAIGHT or CIRCULAR).
 * function LuaLane:getLength()         -- Returns the length of a straight lane in meters.
 * function LuaLane:getRadius()         -- Returns the radius of curvature of a circular lane in meters.
 * function LuaLane:getAngleSpan()      -- Returns the angle spanned by a circular lane in radians.
 * function LuaLane:getSpeedLimit()     -- Returns the speed limit on that segment.
 * @endcode
 *
 * This should enable the creation of many car behaviors.
 * To test your controller, you can either use the menu <em>File/Open LUA controller</em> or
 * use the <em>--lua</em> option of the command line (e.g. <em>./disim --lua="scripts/my_controller.lua"</em>).
 *
 * For any help please email me: svenadrian.gowal@epfl.ch
 */

#include <vector>

#include <agents/Car.h>
#include <utils/Log.h>
#include <map/Map.h>
#include <cmdline.h>
#include <pthread.h>

using namespace std;

/**
 * This enumerates the different weather conditions.
 * To indicate rain and fog, use the binary operation (RAIN | FOG).
 */
typedef enum {NICE=0, RAIN=1, FOG=2} weather_t;

/**
 * This structure holds all arguments given the threads.
 */
class Simulator;
typedef struct {
  int id;
  Simulator *s;
  double dt;
  vector<Car *> cars;
} thread_arg_t;

/**
 * @brief The simulator class.
 *
 * This class is responsible to run the simulation.
 * It provides mainly one function: step, which will
 * advance the simulation by dt seconds.
 * It moves the cars around and is responsible for the
 * creation of cars and controlling their movements.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class Simulator {
 public:

  /**
   * The unique constructor.
   * @param options The commandline options.
   * @param map The map upon which the cars run.
   */
  Simulator(gengetopt_args_info *options, Map *map);

  /**
   * The destructor.
   */
  ~Simulator();

  /**
   * Returns the number of cars currently running.
   * @return the number of running cars.
   */
  int getCarsCount();

  /**
   * Returns a pointer to the car at i position in the cars vector.
   * @param i The index of the car.
   * @return The pointer to the car.
   */
  Car *getCar(int i);

  /**
   * Returns a pointer on the car with the specified ID.
   * @param id The ID of the car.
   * @return The index of the car.
   */
  Car *getCarFromID(int id);

  /**
   * Gets the ID of the next car (with respect to the ID of the car).
   * The IDs might not be consecutive numbers.
   * @param id The ID of the car.
   * @return The index of the car.
   */
  int getNextCarID(int id);

  /**
   * Gets the ID of the previous car (with respect to the ID of the car).
   * The IDs might not be consecutive numbers.
   * @param id The ID of the car.
   * @return The index of the car.
   */
  int getPrevCarID(int id);

  /**
   * Returns the car state of the car with ID id. It modifies the
   * value of id if it doesn't find the specified ID.
   * @param id A pointer to the ID of the car to track.
   * @return The pointer of the CarState of the tracked car.
   */
  CarState *trackCarID(int *id);

  /**
   * Tells the simulator that tracking is enabled.
   */
  void trackingOn();

  /**
   * Tells the simulator that tracking ended.
   */
  void trackingOff();

  /**
   * The most important function.
   * This function advances the simulation by dt seconds.
   * It moves the cars around and is responsible for the
   * creation of cars and controlling their movements.
   * @param dt The time step in seconds.
   */
  void step(double dt);

  /**
   * Fills in the neighbors vector with the neighbors of the
   * specified car.
   * @param c The car.
   * @param neighbors A pointer to the neighbors vector.
   */
  void getNeighbors(Car *c, vector<neighbor_t> *neighbors);

  /**
   * Sets the weather condition
   * @param w The weather.
   * @see weather_t
   */
  void setWeather(int w);

  /**
   * Locks any access to the cars in the simulation.
   * Effectively calls pthread_mutex_lock on the simulator mutex.
   */
  void lock();

  /**
   * Unlocks the access to the cars in the simulation.
   * Effectively calls pthread_mutex_unlock on the simulator mutex.
   */
  void unlock();

 private:
  double random_uniform();
  double random_normal();
  bool enoughSpace(Car *c, Lane *l);
  Car *getNextCar(Car *c, Lane *l, double position, double *distance);
  Car *getPrevCar(Car *c, Lane *l, double position, double *distance);
  Car *getExtendedNextCar(Car *c, Lane *l, double position, double *distance);
  Car *getExtendedPrevCar(Car *c, Lane *l, double position, double *distance);
  void moveCarAlongStraight(Car *car, double dx);
  void moveCarAlongCircular(Car *car, double dx);
  void clearCar(Car *car);
  int exchangeCar(Car *car, Lane *o, Lane *n, bool force=false);
  static void *thread_simulate(void *ptr);
  static void *thread_move(void *ptr);
  static void *thread_cleanup(void *ptr);
  static bool comparePosition(Car *c1, Car *c2);

  int current_car_id;
  vector<Car *> cars;
  pthread_t *threads;
  thread_arg_t *threads_arg;
  Map *map;

  Car *trackedCar;
  CarState trackedCarState;

  gengetopt_args_info *options;
  double visibility;
  double acceleration_factor;

  pthread_mutex_t mutex;
};

#endif
