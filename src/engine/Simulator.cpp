#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "Simulator.h"

#ifdef LUA
#include <bindings/lua/LuaBinding.h>
#endif

#define MIN_CAR_SPACING 10.0

#define MIN(x,y) (((x)>(y))?(y):(x))

Simulator::Simulator(gengetopt_args_info *options, Map *map)
{
  /* Save options */
  this->options = options;

  /* Init current car id */
  this->current_car_id = 0;

  /* Store map of the environment */
  this->map = map;

  /* User of the simulator can follow a car */
  trackedCarState.x = 0.0;
  trackedCarState.y = 0.0;
  trackedCarState.yaw = 0.0;
  trackedCarState.steering_angle = 0.0;
  trackedCarState.speed = 0.0;
  trackedCar = NULL;

  /* Random seed */
  srand(time(NULL));

  /* Set weather conditions */
  setWeather(NICE);
  if (strcmp(options->weather_arg, "rain") == 0) {
    setWeather(RAIN);
  } else if (strcmp(options->weather_arg, "fog") == 0) {
    setWeather(FOG);
  } else if (strcmp(options->weather_arg, "rain+fog") == 0) {
    setWeather(RAIN | FOG);
  }

#ifdef LUA
  LuaBinding::setOptions(options);
  if (options->lua_given) {
    LuaBinding::getInstance().loadFile(options->lua_arg);
  }
  if (options->luacontrol_given) {
    LuaBinding::getInstance().loadControlFile(options->luacontrol_arg);
    LuaBinding::getInstance().callControlInit(this->map->getLuaInfrastructure());
  }
#endif

  /* Threading */
  if (options->ncpu_arg > 0) {
    threads = new pthread_t[options->ncpu_arg];
    threads_arg = new thread_arg_t[options->ncpu_arg];
    for (int i = 0; i < options->ncpu_arg; i++) {
      threads_arg[i].id = i;
      threads_arg[i].s = this;
    }
  }

  /* Initialize the mutex: this mutex should be taken before modifying cars */
  pthread_mutex_init(&(this->mutex), NULL);

  /* Initialize cars to the proper density */
  lock();
  for (unsigned int i = 0; i < map->segments.size(); i++) {
    Segment *s = map->segments[i];
    for (unsigned int j = 0; j < s->lanes.size(); j++) {
      Lane *l = s->lanes[j];
      if (s->geometry == CIRCULAR) {
        double length = l->radius*fabs(s->angle);
        int ncars = MIN((int)((double)options->density_arg*length/1000.0), (int)(length/MIN_CAR_SPACING));
        double dp = ((length - MIN_CAR_SPACING)/(double)ncars)/l->radius;
        for (int k = 0; k < ncars; k++) {
          Car *car = new Car(current_car_id, options);
          cars.push_back(car);
          if (options->ncpu_arg > 0) {
            threads_arg[car->getID() % options->ncpu_arg].cars.push_back(car);
          }
          car->setLane(l);
          car->setPosition(dp*(double)k);
          l->cars.push_back(car);
          car->setSpeed(l->segment->speed);
          moveCarAlongCircular(car, 0.0);
          current_car_id++;
          for (unsigned int a = 0; a < l->sensors.size(); a++) {
            l->sensors[a]->notifyEntry(car);
          }
        }
      } else {
        int ncars = MIN((int)((double)options->density_arg*s->length/1000.0), (int)(s->length/MIN_CAR_SPACING));
        double dp = (s->length - MIN_CAR_SPACING)/(double)ncars;
        for (int k = 0; k < ncars; k++) {
          Car *car = new Car(current_car_id, options);
          cars.push_back(car);
          if (options->ncpu_arg > 0) {
            threads_arg[car->getID() % options->ncpu_arg].cars.push_back(car);
          }
          car->setLane(l);
          car->setPosition(dp*(double)k);
          l->cars.push_back(car);
          car->setSpeed(l->segment->speed);
          moveCarAlongStraight(car, 0.0);
          current_car_id++;
          for (unsigned int a = 0; a < l->sensors.size(); a++) {
            l->sensors[a]->notifyEntry(car);
          }
        }
      }
    }
  }
  unlock();
}

Simulator::~Simulator()
{
  /* Stop logging */
  Log::stop();
  
#ifdef LUA
  LuaBinding::getInstance().callControlDestroy(this->map->getLuaInfrastructure());
#endif
  
  /* Delete cars */
  lock();
  for (unsigned int i = 0; i < cars.size(); i++) {
    delete cars[i];
  }
  cars.clear();
  unlock();

  pthread_mutex_destroy(&(this->mutex));

  /* Destroy threading */
  if (options->ncpu_arg > 0) {
    for (int i = 0; i < options->ncpu_arg; i++)
      threads_arg[i].cars.clear();
    delete [] threads;
    delete [] threads_arg;
  }
}

void Simulator::lock()
{
  pthread_mutex_lock(&(this->mutex));
}

void Simulator::unlock()
{
  pthread_mutex_unlock(&(this->mutex));
}

int Simulator::getCarsCount()
{
  return cars.size();
}

Car *Simulator::getCar(int i)
{
  return cars[i];
}

Car *Simulator::getCarFromID(int id)
{
  for (unsigned int i = 0; i < cars.size(); i++) {
    if (id == cars[i]->getID())
      return cars[i];
  }
  return NULL;
}

int Simulator::getNextCarID(int id)
{
  int closest_id = id;

  for (unsigned int i = 0; i < cars.size(); i++) {
    int cid = cars[i]->getID();
    if ((cid > id && cid < closest_id) || (closest_id == id && cid > id))
      closest_id = cid;
  }

  return closest_id;
}

int Simulator::getPrevCarID(int id)
{
  int closest_id = id;

  for (unsigned int i = 0; i < cars.size(); i++) {
    int cid = cars[i]->getID();
    if ((cid < id && cid > closest_id) || (closest_id == id && cid < id))
      closest_id = cid;
  }

  return closest_id;
}

CarState *Simulator::trackCarID(int *id)
{
  trackingOff();

  Car *car = getCarFromID(*id);
  if (car == NULL) {
    int nid = getNextCarID(*id);
    if (nid == *id) {
      nid = getPrevCarID(*id);
      if (nid == *id) {
        car = NULL;
      } else {
        car = getCarFromID(nid);
      }
    } else {
      car = getCarFromID(nid);
    }
  }

  if (car == NULL) {
    trackedCar = NULL;
    return NULL;
  }

  *id = car->getID();
  trackedCar = car;
  trackedCarState = *(car->getState());
  return &trackedCarState;
}

void Simulator::trackingOn()
{
  if (trackedCar) trackedCar->trackingOn();
}

void Simulator::trackingOff()
{
  if (trackedCar) trackedCar->trackingOff();
}

void Simulator::step(double dt)
{
  static unsigned int steps_count = 0;
  static double current_time = 0.0;
  vector<neighbor_t> neighbors;
  double rear, front;

  /* Log beginning of step */
  Log::getStream(9) << "Simulation step #" << steps_count
                    << " at time " << current_time << " seconds" << endl;

  /* Update the infrastructure */
#ifdef LUA
  // If there is a lua binding then call the update function there
  LuaBinding::getInstance().callControlUpdate(map->getLuaInfrastructure(), current_time, dt);
#endif

  /* Create new cars if needed */
  lock();
  for (unsigned int i = 0; i < map->entries.size(); i++) {
    Lane *l = map->entries[i];
    
    // Time inbetween two cars such the entry_rate is respected.
    l->cumulative_rate += l->entry_rate*dt;
    if (l->cumulative_rate > 1.0) {
      l->cumulative_rate = 0.0;
      l->force_entry = 1;
    }

    if (l->force_entry) {
      // Check if there is space to put the car
      double dist;
      Car *f = getNextCar(NULL, l, 0.0, &dist);
      if (f)
        f->getCarGeometry(NULL, &rear);
      else
        rear = 0.0;

      if (!l->new_car) {
        l->new_car = new Car(current_car_id, options);
        l->new_car->setLane(l);
        l->new_car->setPosition(0.0);
        current_car_id++;
      }
      l->new_car->getCarGeometry(&front);

      // Find the good entry speed to maintain the entry rate
      if (f) l->new_car->setSpeed(f->getSpeed());
      else l->new_car->setSpeed(l->segment->speed);
      if (l->entry_speed >= 0.0) l->new_car->setSpeed(l->entry_speed);
      neighbors.clear();
      getNeighbors(l->new_car, &neighbors);
      l->new_car->simulate(dt, neighbors, true);
      double acc1 = l->new_car->getAcceleration();
      for (unsigned int j = 0; j < neighbors.size(); j++) {
        neighbors[j].car = NULL;
        neighbors[j].distance = visibility;
      }
      l->new_car->simulate(dt, neighbors, true);
      double acc2 = l->new_car->getAcceleration();

      if ((dist > rear + front + 1.0 && acc1-MIN(acc2,0) > -0.5)) {
        Car *car = l->new_car;
        l->new_car = NULL;
        cars.push_back(car);
        if (options->ncpu_arg > 0) {
          threads_arg[car->getID() % options->ncpu_arg].cars.push_back(car);
        }
        l->cars.push_back(car);
        if (l->force_entry) l->force_entry--;
        car->setX(l->x_start);
        car->setY(l->y_start);
        car->setYaw(l->a_start);

        // Find entry_speed to maintain entry_rate
        if (l->entry_speed >= 0.0) {
          car->setSpeed(l->entry_speed);
        } else {
          if (f) car->setSpeed(f->getSpeed());
          else car->setSpeed(l->segment->speed);
        }
      }
    }
  }

  /* Update sensors -- This has to come here, otherwise the occupied status of
   the sensors is wrong! */
  for (unsigned int i = 0; i < map->sensors.size(); i++) {
    map->sensors[i]->update(dt);
  }
  for (unsigned int i = 0; i < map->actuators.size(); i++) {
    map->actuators[i]->update(dt);
  }

  /* Start threading */
  for (int i = 0; i < options->ncpu_arg; i++) {
    threads_arg[i].dt = dt;
    pthread_create(&threads[i], NULL, thread_simulate, &threads_arg[i]); // We assume everything works
  }
  
  /* Simulate car behaviors */
  if (options->ncpu_arg == 0) {
    for (unsigned int i = 0; i < cars.size(); i++) {
      neighbors.clear();
      getNeighbors(cars[i], &neighbors);
      cars[i]->simulate(dt, neighbors);
    }
  }
  
  /* Finish threading */
  for (int i = 0; i < options->ncpu_arg; i++)
    pthread_join(threads[i], NULL);

  /* Start threading */
  for (int i = 0; i < options->ncpu_arg; i++) {
    pthread_create(&threads[i], NULL, thread_move, &threads_arg[i]);
  }

  /* Update car position */
  if (options->ncpu_arg == 0) {
    for (unsigned int i = 0; i < cars.size(); i++) {
      Car *car = cars[i];
      
      // Swith lane if asked
      Lane *lane = car->getLane();
      if (car->getLaneChange() < 0 && lane->left) {
        if (exchangeCar(car, lane, lane->left) == 0) {
          for (unsigned int j = 0; j < lane->sensors.size(); j++) {
            lane->sensors[j]->notifyExit(car);
          }
          for (unsigned int j = 0; j < lane->left->sensors.size(); j++) {
            lane->left->sensors[j]->notifyEntry(car);
          }
        }
      } else if (car->getLaneChange() > 0 && lane->right) {
        if (exchangeCar(car, lane, lane->right) == 0) {
          for (unsigned int j = 0; j < lane->sensors.size(); j++) {
            lane->sensors[j]->notifyExit(car);
          }
          for (unsigned int j = 0; j < lane->right->sensors.size(); j++) {
            lane->right->sensors[j]->notifyEntry(car);
          }
        }
      }
      
      // Update Lane, Position, X, Y, Yaw, Speed
      double v = car->getSpeed() + dt*acceleration_factor*car->getAcceleration();
      if (v < 0.0) v = 0.0;
      car->setSpeed(v);
      double dx = v*dt;
      
      if (car->getLane()->segment->geometry == STRAIGHT) {
        moveCarAlongStraight(car, dx);
      } else {
        moveCarAlongCircular(car, dx);
      }
    }
  }

  /* Finish threading */
  for (int i = 0; i < options->ncpu_arg; i++)
    pthread_join(threads[i], NULL);

  /* Delete cars */
  for (unsigned int i = 0; i < cars.size(); i++) {
    if (cars[i]->delete_me) {
      Log::getStream(5) << "Travel time of car " << cars[i]->getID() << ": " << cars[i]->getTimeAlive()
                        << " seconds (" << current_time << ")" << endl;
      clearCar(cars[i]);
      delete cars[i];
      cars.erase(cars.begin() + i);
      i--;
    }
  }
  unlock();

  /* Increment static counters */
  steps_count++;
  current_time += dt;

  /* Update tracked car */
  if (trackedCar)
    trackedCarState = *(trackedCar->getState());
}

void Simulator::clearCar(Car *car)
{
  // Remove from lane
  Lane *l = car->getLane();
  for (unsigned int i = 0; i < l->cars.size(); i++) {
    if (l->cars[i] == car) {
      l->cars.erase(l->cars.begin() + i);
    }
  }

  // Keep trackedCar up-to-date
  if (car == trackedCar) trackedCar = NULL;
}

void *Simulator::thread_simulate(void *ptr)
{
  vector<neighbor_t> neighbors;
  
  Simulator *s = ((thread_arg_t *)ptr)->s;
  double dt = ((thread_arg_t *)ptr)->dt;
  vector<Car *> &cars = ((thread_arg_t *)ptr)->cars;

  /* Simulate car behaviors */
  for (unsigned int i = 0; i < cars.size(); i++) {
    neighbors.clear();
    s->getNeighbors(cars[i], &neighbors);
    cars[i]->simulate(dt, neighbors);
  }

  return NULL;
}

void *Simulator::thread_move(void *ptr)
{
  Simulator *s = ((thread_arg_t *)ptr)->s;
  double dt = ((thread_arg_t *)ptr)->dt;
  vector<Car *> &cars = ((thread_arg_t *)ptr)->cars;

  /* Update car position */
  for (unsigned int i = 0; i < cars.size(); i++) {
    Car *car = cars[i];
    
    // Swith lane if asked
    Lane *lane = car->getLane();
    if (car->getLaneChange() < 0 && lane->left) {
      if (s->exchangeCar(car, lane, lane->left) == 0) {
        for (unsigned int j = 0; j < lane->sensors.size(); j++) {
          lane->sensors[j]->notifyExit(car);
        }
        for (unsigned int j = 0; j < lane->left->sensors.size(); j++) {
          lane->left->sensors[j]->notifyEntry(car);
        }
      }
    } else if (car->getLaneChange() > 0 && lane->right) {
      if (s->exchangeCar(car, lane, lane->right) == 0) {
        for (unsigned int j = 0; j < lane->sensors.size(); j++) {
          lane->sensors[j]->notifyExit(car);
        }
        for (unsigned int j = 0; j < lane->right->sensors.size(); j++) {
          lane->right->sensors[j]->notifyEntry(car);
        }
      }
    }
    
    // Update Lane, Position, X, Y, Yaw, Speed
    double v = car->getSpeed() + dt*s->acceleration_factor*car->getAcceleration();
    if (v < 0.0) v = 0.0;
    car->setSpeed(v);
    double dx = v*dt;
    
    if (car->getLane()->segment->geometry == STRAIGHT) {
      s->moveCarAlongStraight(car, dx);
    } else {
      s->moveCarAlongCircular(car, dx);
    }

    if (cars[i]->delete_me) {
      cars.erase(cars.begin()+i);
      i--;
    }
  }

  return NULL;
}

void *Simulator::thread_cleanup(void *ptr)
{
  return NULL;
}

double Simulator::random_uniform()
{
  return (double)rand()/(double)RAND_MAX;
}

double Simulator::random_normal()
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

bool Simulator::enoughSpace(Car *c, Lane *l)
{
  double f, r, d;
  c->getCarGeometry(&f, &r);
  
  Car *n = getExtendedNextCar(c, l, c->getPosition(), &d);
  if (n) {
    double nr;
    n->getCarGeometry(NULL, &nr);
    if (d - nr - f < 0.0) return false;
  }

  Car *p = getExtendedPrevCar(c, l, c->getPosition(), &d);
  if (p) {
    double pf;
    p->getCarGeometry(&pf);
    if (d - pf - r < 0.0) return false;
  }

  return true;
}

Car *Simulator::getNextCar(Car *c, Lane *l, double position, double *distance)
{
  Car *min_car = NULL;
  double min_dp = l->segment->length - position;

  if (l->segment->geometry == CIRCULAR) min_dp = fabs(l->segment->angle) - position;

  for (unsigned int i = 0; i < l->cars.size(); i++) {
    Car *car = l->cars[i];
    if (car == c) continue;
    double dp = car->getPosition() - position;
    if (dp >= 0.0 && dp < min_dp) {
      min_dp = dp;
      min_car = car;
    }
  }

  *distance = min_dp;
  if (l->segment->geometry == CIRCULAR) {
    *distance = l->radius*min_dp;
  }
  return min_car;
}

Car *Simulator::getPrevCar(Car *c, Lane *l, double position, double *distance)
{
  Car *min_car = NULL;
  double min_dp = position;

  for (unsigned int i = 0; i < l->cars.size(); i++) {
    Car *car = l->cars[i];
    if (car == c) continue;
    double dp = position - car->getPosition();
    if (dp >= 0.0 && dp < min_dp) {
      min_dp = dp;
      min_car = car;
    }
  }

  *distance = min_dp;
  if (l->segment->geometry == CIRCULAR) {
    *distance = l->radius*min_dp;
  }
  return min_car;
}

Car *Simulator::getExtendedNextCar(Car *c, Lane *l, double position, double *distance)
{
  Car *car = NULL;
  double d;
  *distance = 0.0;

  while (!car && l && *distance < visibility) {
    car = getNextCar(c, l, position, &d);
    if (!car) {
      if (l->next && l->next->prev == l) {
        position = 0.0;
        *distance += d;
        l = l->next;
      } else {
        l = NULL;
        *distance = visibility;
      }
    } else {
      *distance += d;
    }
  }

  if (!car) *distance = visibility;
  if (*distance > visibility) {
    *distance = visibility;
    car = NULL;
  }
  return car;
}

Car *Simulator::getExtendedPrevCar(Car *c, Lane *l, double position, double *distance)
{
  Car *car = NULL;
  double d;
  *distance = 0.0;

  while (!car && l && *distance < visibility) {
    car = getPrevCar(c, l, position, &d);
    if (!car) {
      if (l->prev) {
        position = l->prev->segment->length;
        if (l->prev->segment->geometry == CIRCULAR) position = fabs(l->prev->segment->angle);
        *distance += d;
        l = l->prev;
      } else {
        l = NULL;
        *distance = visibility;
      }
    } else {
      *distance += d;
    }
  }

  if (!car) *distance = visibility;
  if (*distance > visibility) {
    *distance = visibility;
    car = NULL;
  }
  return car;
}

void Simulator::moveCarAlongStraight(Car *car, double dx)
{
  Lane *l = car->getLane();
  double position = car->getPosition();
  position += dx;

  // Trigger sensors
  for (unsigned int i = 0; i < l->sensors.size(); i++) {
    l->sensors[i]->trigger(car, car->getPosition(), position);
  }
  for (unsigned int i = 0; i < l->actuators.size(); i++) {
    l->actuators[i]->trigger(car, car->getPosition(), position);
  }

  // The car stay on the same segment
  if (position < l->segment->length) {
    car->setPosition(position);
    car->setX(l->x_start + cos(l->a_start)*position);
    car->setY(l->y_start + sin(l->a_start)*position);
    car->setYaw(l->a_start);
    return;
  }

  Lane *nl = l->next;
  // if the car took an exit
  if (!nl) {
    car->delete_me = true;
    return;
  }

  // Set destination
  Segment *s = nl->segment;
  if (s->type == EXIT) {
    // Get exit segments
    for (unsigned int i = 0; i < s->lanes.size(); i++) {
      Lane *sl = s->lanes[i];
      if (sl->type != EXIT) continue;
      if (random_uniform() < sl->split_ratio) {
        car->setDestination(sl);
        break;
      }
    }
  } else {
    // Reset destination
    car->setDestination(NULL);
  }

  // otherwise call the function on the new segment
  dx = position - l->segment->length;
  car->setPosition(0.0);
  exchangeCar(car, l, nl, true);
  if (nl->segment->geometry == STRAIGHT) {
    moveCarAlongStraight(car, dx);
  } else {
    moveCarAlongCircular(car, dx);
  }
}

void Simulator::moveCarAlongCircular(Car *car, double dx)
{
  Lane *l = car->getLane();
  double position = car->getPosition();
  position += dx/l->radius;

  // Trigger sensors
  for (unsigned int i = 0; i < l->sensors.size(); i++) {
    l->sensors[i]->trigger(car, car->getPosition(), position);
  }
  for (unsigned int i = 0; i < l->actuators.size(); i++) {
    l->actuators[i]->trigger(car, car->getPosition(), position);
  }

  // The car stay on the same segment
  if (position < fabs(l->segment->angle)) {
    double f = (l->segment->angle < 0.0)?-1.0:1.0;
    car->setPosition(position);
    car->setX(l->xc - cos(l->angle_start + f*position)*l->radius);
    car->setY(l->yc - sin(l->angle_start + f*position)*l->radius);
    car->setYaw(l->angle_start + f*position - f*M_PI/2.0);
    return;
  }

  Lane *nl = l->next;
  // if the car took an exit
  if (!nl) {
    car->delete_me = true;
    return;
  }

  // Set destination
  Segment *s = nl->segment;
  if (s->type == EXIT) {
    // Get exit segments
    for (unsigned int i = 0; i < s->lanes.size(); i++) {
      Lane *sl = s->lanes[i];
      if (sl->type != EXIT) continue;
      if (random_uniform() < sl->split_ratio) {
        car->setDestination(sl);
        break;
      }
    }
  } else {
    // Reset destination
    car->setDestination(NULL);
  }

  // otherwise call the function on the new segment
  dx = (position - fabs(l->segment->angle))*l->radius;
  car->setPosition(0.0);
  exchangeCar(car, l, nl, true);
  if (nl->segment->geometry == STRAIGHT) {
    moveCarAlongStraight(car, dx);
  } else {
    moveCarAlongCircular(car, dx);
  }
}

int Simulator::exchangeCar(Car *car, Lane *o, Lane *n, bool force)
{
  // Add to new lane
  pthread_mutex_lock(&(n->cars_mutex));
  // Check lane first
  if (!force) {
    for (unsigned int i = 0; i < n->cars.size(); i++) {
      double d = fabs(n->cars[i]->getPosition() - car->getPosition());
      if (n->segment->geometry == CIRCULAR) d *= n->radius;
      if (d < 1.0) {
        // Ooops cannot change lanes
        pthread_mutex_unlock(&(n->cars_mutex));
        return -1;
      }
    }
  }
  n->cars.push_back(car);
  pthread_mutex_unlock(&(n->cars_mutex));
  car->setLane(n);

  // Remove from lane
  pthread_mutex_lock(&(o->cars_mutex));
  for (unsigned int i = 0; i < o->cars.size(); i++) {
    if (o->cars[i] == car) {
      o->cars.erase(o->cars.begin() + i);
      break;
    }
  }
  pthread_mutex_unlock(&(o->cars_mutex));

  return 0;
}

void Simulator::getNeighbors(Car *c, vector<neighbor_t> *neighbors)
{
  // The order must be LEAD = 0, TRAIL, LEFT_LEAD, LEFT_TRAIL, RIGHT_LEAD, RIGHT_TRAIL
  neighbor_t n;
  n.car = getExtendedNextCar(c, c->getLane(), c->getPosition(), &n.distance);
  n.role = LEAD;
  neighbors->push_back(n);

  n.car = getExtendedPrevCar(c, c->getLane(), c->getPosition(), &n.distance);
  n.role = TRAIL;
  neighbors->push_back(n);

  n.car = getExtendedNextCar(c, c->getLane()->left, c->getPosition(), &n.distance);
  n.role = LEFT_LEAD;
  neighbors->push_back(n);

  n.car = getExtendedPrevCar(c, c->getLane()->left, c->getPosition(), &n.distance);
  n.role = LEFT_TRAIL;
  neighbors->push_back(n);

  n.car = getExtendedNextCar(c, c->getLane()->right, c->getPosition(), &n.distance);
  n.role = RIGHT_LEAD;
  neighbors->push_back(n);

  n.car = getExtendedPrevCar(c, c->getLane()->right, c->getPosition(), &n.distance);
  n.role = RIGHT_TRAIL;
  neighbors->push_back(n);
}

void Simulator::setWeather(int w)
{
  if (w == RAIN) {
    visibility = 80.0;
    acceleration_factor = 0.8;
  } else if (w == FOG) {
    visibility = 50.0;
    acceleration_factor = 1.0;
  } else if (w == (RAIN | FOG)) {
    visibility = 40.0;
    acceleration_factor = 0.8;
  } else {
    visibility = 100.0;
    acceleration_factor = 1.0;
  }
}
