#ifndef MAP_H
#define MAP_H

#include <vector>
#include <cmdline.h>
#include <stdio.h>
#include <pthread.h>

#ifdef LUA
class LuaInfrastructure;
class LuaLane;
class LuaRoadSensor;
class LuaRoadActuator;
#endif

using namespace std;

/**
 * This enumerates the different kind of lane types.
 * ENTRY means that the lane is an entry of the road network;
 * the simulator should create vehicles at those entry points at the specified rate.
 * EXIT means that the lane is an exit of the road network; any
 * vehicles going through this lane should exit the system.
 * NONE means that the lane is neither an entry or an exit.
 */
typedef enum {ENTRY, EXIT, NONE} lane_t;

/**
 * This enumerates the different segment geometries.
 * We have decided to keep only straight and circular segments.
 */
typedef enum {STRAIGHT, CIRCULAR} segment_t;

/**
 * This enumerates the different sides the entry/exit can be.
 * E.g.: if the segment is an exit segment and the side is set to left,
 * the exit will be on the left.
 */
typedef enum {LEFT, RIGHT} side_t;

/**
 * The road network can be augmented with different sensors that
 * can keep track of different quantities: the density [veh/km], 
 * the average time mean speed [km/h] or the flow [veh/h].
 */
typedef enum {DENSITY, SPEED, FLOW} sensor_t;

/**
 * The road network can be augmented with different actuator that
 * can modify their state to give to driver informations.
 */
typedef enum {TRAFFICLIGHT, SPEEDLIMIT} actuator_t;

/**
 * The traffic lights may either be green or red.
 */
typedef enum {GREEN, RED} color_t;

/**
 * Each lane can have specific markings (broken white line or solid white).
 * These markings may indicate the inablility for a car to cross to the left or right
 * neighboring lane.
 */
typedef enum {BROKEN, SOLID} marking_t;

class Segment;
class Car;
class Lane;

/**
 * @brief The road sensor class.
 *
 * This class defines the different road sensors.
 * Each sensors is part of a specific lane at a specific position.
 * If asked the sensor will generate logs of the data.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class RoadSensor {
 public:

  /**
   * The constructor.
   * This creates a road sensor of certain type on a lane at specific position.
   * @param name The name of the sensor, it is used to determine the filename where the sensors logs the data.
   * @param type The sensor type.
   * @param l The lane.
   * @param position The position of the sensor.
   * @see sensor_t
   */
  RoadSensor(char *name, sensor_t type, Lane *l, double position, gengetopt_args_info *options);
  /**
   * The constructor of the density sensor.
   * The density sensor needs to have an area of monitoring defines inbetween position1 and position2.
   * @param name,type,l as in the other constructor.
   * @param position1 The entry of the monitored area.
   * @param position2 The exit of the monitored area.
   */
  RoadSensor(char *name, sensor_t type, Lane *l, double position1, double position2, gengetopt_args_info *options);

  /**
   * The destructor.
   */
  ~RoadSensor();

  /**
   * This function updates the sensor data through time and
   * logs the data is necessary.
   * @param dt The simulation time step.
   */
  void update(double dt);

  /**
   * This function will trigger the sensor if old_x < position < new_x (always call trigger before update)
   * @param c The car that might trigger the sensor.
   * @param old_x The old car position.
   * @param new_x The new car position.
   */
  void trigger(Car *c, double old_x, double new_x);

  /**
   * Sets the rate in [Hz] at which the sensor computes the needed data
   * @param rate The log rate (in Hz)
   */
  void setRate(double rate);

  /**
   * Checks if the sensor has been triggered during the last 0.5 seconds (this will reset the triggered variable)
   * This is useful to display the sensor differently when it has been triggered.
   */
  bool wasTriggered();

  /**
   * Checks if entry of the density sensor has been triggered during the last 0.5 seconds (this will reset the triggered variable)
   * This is useful to display the sensor differently when it has been triggered.
   */
  bool wasEntryTriggered();
  
  /**
   * Checks if exit of the density sensor has been triggered during the last 0.5 seconds (this will reset the triggered variable)
   * This is useful to display the sensor differently when it has been triggered.
   */
  bool wasExitTriggered();

  /**
   * Returns the resulting data of the last computation cycle.
   */
  double getResult();

  /**
   * Returns the number of vehicles that passed through the actuator (this value is really only useful for density sensors).
   * @return The number of vehicles that passed through the actuator.
   */
  unsigned int getCount();

  /**
   * Starts the logging.
   */
  void log();

  /**
   * Notifies if a car changed lanes and exited the monitored area.
   * This is useful for the density sensor to keep track of its specific lane.
   * @param c The car.
   */
  void notifyExit(Car *c);

  /**
   * Notifies if a car changed lanes and entered the monitored area.
   * This is useful for the density sensor to keep track of its specific lane.
   * @param c The car.
   */
  void notifyEntry(Car *c);

  /**
   * Returns whether a car sits on top of the sensor.
   * @return true if a car is on top of the sensor.
   */
  bool isOccupied();

#ifdef LUA
  /**
   * Gets the LUA object corresponding to this Lane.
   * @return A pointer to the LuaLane object.
   */
  LuaRoadSensor *getLuaRoadSensor();

  /**
   * Sets the LUA object corresponding to this Lane.
   * @param car A pointer to the LuaLane object.
   */
  void setLuaRoadSensor(LuaRoadSensor *rs);
#endif

 public:

  /**
   * The type of sensor.
   */
  sensor_t type;

  /**
   * The position of the sensor with respect to its lane. The position
   * is either specified in meters or in radians.
   */
  double position;

  /**
   * The exit position of the density sensor with respect to its lane. The position
   * is either specified in meters or in radians.
   */
  double position2;

  /**
   * The sensor's lane.
   */
  Lane *lane;

  /**
   * The sensor's name.
   */
  char name[256];
  
 private:
  double t;
  double current_time;
  unsigned int count;
  unsigned int cnt;    // Counts the number of time update was called
  double density;
  double speed;
  double speed_std;
  double delta;
  double distance;
  double result;
  double trigger_delay;
  double trigger_delay2;
  char filename[256];
  FILE *file;
  pthread_mutex_t mutex;
  bool occupied;
#ifdef LUA
  LuaRoadSensor *luaRoadSensor;
#endif
};

class RoadActuator {
 public:

  /**
   * The constructor.
   * This creates a road actuator of certain type on a lane at specific position.
   * @param name The name of the sensor.
   * @param type The actuator type.
   * @param l The lane.
   * @param position The position of the sensor.
   * @see sensor_t
   */
  RoadActuator(char *name, actuator_t type, Lane *l, double position, gengetopt_args_info *options);

  /**
   * The destructor.
   */
  ~RoadActuator();

  /**
   * This function updates the actuator.
   * @param dt The simulation time step.
   */
  void update(double dt);

  /**
   * This function will trigger the sensor if old_x < position < new_x (always call trigger before update)
   * @param c The car that might trigger the sensor.
   * @param old_x The old car position.
   * @param new_x The new car position.
   */
  void trigger(Car *c, double old_x, double new_x);

  /**
   * Starts the logging.
   */
  void log();

  /**
   * Returns the resulting data of the last computation cycle.
   * @return The resulting data of the last computation cycle.
   */
  double getResult();

  /**
   * Returns the number of vehicles that passed through the actuator.
   * @return The number of vehicles that passed through the actuator.
   */
  unsigned int getCount();

  /**
   * Returns the intermidiate result
   */
  double getInstantResult(bool *new_value = NULL);

#ifdef LUA
  /**
   * Gets the LUA object corresponding to this actuator.
   * @return A pointer to the LuaRoadActuator object.
   */
  LuaRoadActuator *getLuaRoadActuator();

  /**
   * Sets the LUA object corresponding to this actuator.
   * @param car A pointer to the LuaRoadActuator object.
   */
  void setLuaRoadActuator(LuaRoadActuator *ra);
#endif

 public:

  /**
   * The type of sensor.
   */
  actuator_t type;

  /**
   * The position of the actuator with respect to its lane. The position
   * is either specified in meters or in radians.
   */
  double position;

  /**
   * The sensor's lane.
   */
  Lane *lane;

  /**
   * The sensor's name.
   */
  char name[256];
  
 private:
  double t;
  double current_time;
  unsigned int count;
  double delta;
  double queue_time;
  double result;
  double immediate_result;
  bool immediate_read;
  unsigned int result_count;
  char filename[256];
  FILE *file;
  pthread_mutex_t mutex;
#ifdef LUA
  LuaRoadActuator *luaRoadActuator;
#endif
};

class TrafficLightActuator : public RoadActuator {
 public:

  /**
   * The constructor.
   * This creates a traffic light actuator on a lane at specific position.
   * @param name The name of the sensor.
   * @param l The lane.
   * @param position The position of the sensor.
   * @see sensor_t
   */
  TrafficLightActuator(char *name, Lane *l, double position, gengetopt_args_info *options);

  /**
   * The destructor.
   */
  ~TrafficLightActuator();

  /**
   * Set the light to red.
   */
  void red();

  /**
   * Set the light to green.
   */
  void green();

  /**
   * Gets the lights color.
   */
  color_t color();

 private:
  color_t state;
};

class SpeedLimitActuator : public RoadActuator {
 public:
  /**
   * The constructor.
   * This creates a speed limit actuator on a lane at specific position.
   * @param name The name of the sensor.
   * @param l The lane.
   * @param position The position of the sensor.
   * @see sensor_t
   */
  SpeedLimitActuator(char *name, Lane *l, double position, gengetopt_args_info *options);

  /**
   * The destructor.
   */
  ~SpeedLimitActuator();

  /**
   * Sets the speed limits of all lane (including the actuator's lane)
   * until another speed limit actuator is met.
   */
  void setSpeedLimit(double max, double min = -1.0);
};

/**
 * @brief The marking class.
 *
 * This class hold the information about the marking of a lane.
 * A lane may have several markings.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class Marking {
 public:
  /**
   * The unique constructor
   */
  Marking();

  /**
   * The destructor
   */
  ~Marking();

  /**
   * The marking type
   */
  marking_t type;

  /**
   * The starting position of the marking with respect to its lane.
   * This position can be specified in meters or in radians.
   */
  double start;

  /**
   * The end position of the marking with respect to its lane.
   * This position can be specified in meters or in radians.
   */
  double end;
};

/**
 * @brief The lane class.
 *
 * This class describes the lanes of the road network.
 * A lane is attached to a specific segment and has a entry and exit point.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class Lane {
 public:

  /**
   * The unique constructor.
   */
  Lane();

  /**
   * The destructor.
   */
  ~Lane();

#ifdef LUA
  /**
   * Gets the LUA object corresponding to this Lane.
   * @return A pointer to the LuaLane object.
   */
  LuaLane *getLuaLane();

  /**
   * Sets the LUA object corresponding to this Lane.
   * @param car A pointer to the LuaLane object.
   */
  void setLuaLane(LuaLane *lane);
#endif

  /**
   * Returns the legallity of a lane change at that position to the left lane.
   * @return 0.0 if a broken line is present on the left at position p, otherwise it returns the end of the restricted position.
   */
  double allowedLeft(double p);

  /**
   * Returns the legallity of a lane change at that position to the right lane.
   * @return true if a broken line is present on the right at position p, otherwise it returns the end of the restricted position.
   */
  double allowedRight(double p);

 public:

  /**
   * The parent segment.
   */
  Segment *segment;

  // Next, previous lanes
  /**
   * The next lane. A car navigating on a lane and exceeding its limits
   * will automatically go on the next lane.
   * Currently only one next lane is possible (which is ok for a highway).
   */
  Lane *next;

  /**
   * The previous lane.
   */
  Lane *prev;

  /**
   * The lane on the left.
   */
  Lane *left;

  /**
   * The lane on the right.
   */
  Lane *right;

  /**
   * The index of lane in the segment (0 is the left-most lane).
   */
  int index;

  /**
   * The maximum speed allowed on this lane.
   */
  double maximum_speed;

  /**
   * The minimum speed allowed on this lane.
   */
  double minimum_speed;

  /**
   * If the lane is an entry lane. One can specify an entry rate
   * in veh/s. Cars will randomly enter at the specified rate.
   * @see lane_t
   */
  double entry_rate;

  /**
   * If the lane is an exit lane. One can specify a split ratio
   * in %. Cars will randomly exit at the specified ratio.
   * @see lane_t
   */
  double split_ratio;

  /**
   * If the lane is an entry lane. One can specify an entry speed
   * in m/s. Cars will randomly enter at the specified speed.
   * @see lane_t
   */
  double entry_speed;

  /**
   * Indicates whether the lane finishes and merging is needed.
   * If -1, the merging is on the left, 1 on the right and 0 otherwise.
   */
  int merge_direction;

  /**
   * The lane type: entry/exit or none.
   */
  lane_t type;

  /**
   * Geometry help to draw the lane and car on it.
   * It speicifies the x coordinate of the entry.
   */
  double x_start;

  /**
   * Geometry help to draw the lane and car on it.
   * It speicifies the y coordinate of the entry.
   */
  double y_start;

  /**
   * Geometry help to draw the lane and car on it.
   * It speicifies the angle/orientation of the entry.
   */
  double a_start;

  /**
   * Geometry help to draw the lane and car on it.
   * It speicifies the x coordinate of the exit point.
   */
  double x_end;

  /**
   * Geometry help to draw the lane and car on it.
   * It speicifies the y coordinate of the exit point.
   */
  double y_end;

  /**
   * Geometry help to draw the lane and car on it.
   * It speicifies the orientation coordinate of the exit point.
   */
  double a_end;

  /**
   * Geometry help to draw the lane and car on it when the lane is circular.
   * It speicifies the radius of curvature of the lane.
   */
  double radius;

  /**
   * Geometry help to draw the lane and car on it when the lane is circular.
   * It speicifies the start angle from the center of the circle.
   */
  double angle_start;

  /**
   * Geometry help to draw the lane and car on it when the lane is circular.
   * It speicifies the x coordinate of the circle center.
   */
  double xc;

  /**
   * Geometry help to draw the lane and car on it when the lane is circular.
   * It speicifies the x coordinate of the circle center.
   */
  double yc;

  /**
   * This vector holds all the markings on the left related to that lane.
   * If the vector is empty, broken white markings are assumed.
   */
  vector<Marking> left_markings;

  /**
   * This vector holds all the markings on the right related to that lane.
   * If the vector is empty, broken white markings are assumed.
   */
  vector<Marking> right_markings;

  /**
   * This vector holds all the cars that are on that lane.
   */
  vector<Car *> cars;

  /**
   * If this value is bigger than 0, the simulator will put a car no matter what is the entry_rate.
   * This is useful to keep the entry rate as precise as possible.
   */
  unsigned int force_entry;

  /**
   * Next car to enter on the lane (if this is an entry lane).
   */
  Car *new_car;

  /**
   * Cumulative rate to new car (when this is bigger than 1 a car needs to come in).
   */
  double cumulative_rate;

  /**
   * This vector holds all sensors on that lane.
   */
  vector<RoadSensor *> sensors;

  /**
   * This vector holds all actuators on that lane.
   */
  vector<RoadActuator *> actuators;

  /**
   * Mutex to the list of cars.
   */
  pthread_mutex_t cars_mutex;

  /**
   * Name of the lane. It is sent to the entry rate script (if any).
   */
  char name[256];

 private:
#ifdef LUA
  LuaLane *luaLane;
#endif
};

/**
 * @brief The segment class.
 *
 * This class describes the segment of the road network.
 * Each segment contains different lanes that have the same geometry
 * are a next to each other.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class Segment {
 public:

  /**
   * The unique constructor.
   */
  Segment();

  /**
   * The destructor.
   */
  ~Segment();
  
 public:

  /**
   * This vector contains all the lane that are part of this segment.
   */
  vector<Lane *> lanes;
  // Previous and next segment
  /**
   * The previous segment.
   */
  Segment *prev;

  /**
   * The next segment. As we are interested in highway simulation,
   * there is only one next segment.
   */
  Segment *next;

  /**
   * The offset in "lanes" unit with respect to the previous segment.
   * A negative offset means that the segment is to be shifted to the left with respect to
   * its nominal position.
   */
  int offset;

  /**
   * Number of effective lanes at the end of the segment.
   * This number is equal to the size of the lanes vector minus the number of exit lanes.
   */
  int nlanes;

  /**
   * The nominal offset of the next segment to best match the available lanes.
   */
  int base_offset;

  /**
   * The segment type. It indicates whether there are entry or exit lanes.
   */
  lane_t type;

  /**
   * The segment geometry. Either straight or circular.
   */
  segment_t geometry;

  /**
   * Entry/exit lane side or prefered position with respect to the previous segment.
   */
  side_t side;

  /**
   * The length of a straight segment
   */
  double length;

  /**
   * The radius of the left most lane of the segment.
   */
  double radius;

  /**
   * The angle span of the segment.
   */
  double angle;

  /**
   * The speed allowed on the segment.
   */
  double speed;

  /**
   * Geometry help to draw the segment.
   * This is the x coordinate of the left most corner of the segment.
   */
  double x;

  /**
   * Geometry help to draw the segment.
   * This is the y coordinate of the left most corner of the segment.
   */
  double y;

  /**
   * Geometry help to draw the segment.
   * This is the orientation of the left most corner of the segment.
   */
  double a;
};

/**
 * @brief The map class.
 *
 * This class describes the road network.
 * The map contains a set of segments.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch)
 */
class Map {
 public:

  /**
   * The unique constructor.
   * @param options The commandline options. These options specify which map to load.
   */
  Map(gengetopt_args_info *options);

  /**
   * The destructor.
   */
  ~Map();

 public:

  /**
   * The list of segments.
   */
  vector<Segment *> segments;

  /**
   * The list of lanes that are entries of the system.
   */
  vector<Lane *> entries;

  /**
   * The list of sensors on the network.
   */
  vector<RoadSensor *> sensors;

  /**
   * The list of actuators on the network.
   */
  vector<RoadActuator *> actuators;

  /**
   * The lane width (only for display).
   */
  double lane_width;

  /**
   * The name of the map.
   */
  char *name;

  /**
   * The time of day at which the simulation started.
   */
  int start_time;

#ifdef LUA
  /**
   * Gets the LUA object corresponding to this Map.
   * @return A pointer to the LuaInfrastructure object.
   */
  LuaInfrastructure *getLuaInfrastructure();

  /**
   * Sets the LUA object corresponding to this Map.
   * @param infrastructure A pointer to the LuaInfrastrcuture object.
   */
  void setLuaInfrastructure(LuaInfrastructure *infrastructure);
#endif

 private:
  void createMap(gengetopt_args_info *options);
  void createDefaultMap();
  void createCoordinates();
  Lane * addNewLane(Segment *s, lane_t t);
  void clear();
  void prepare();
  gengetopt_args_info *options;
#ifdef LUA
  LuaInfrastructure *luaInfrastructure;
#endif
};

#endif
