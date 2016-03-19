#include "Map.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <agents/Car.h>
#include <utils/Log.h>
#include <iomanip>

#ifdef LUA
#include <bindings/lua/LuaBinding.h>
#endif

Lane::Lane()
{
#ifdef LUA
  this->luaLane = new LuaLane(NULL);
  this->luaLane->setSelf(this);
#endif
  pthread_mutex_init(&(this->cars_mutex), NULL);

  this->segment = NULL;
  this->left = NULL;
  this->right = NULL;
  this->next = NULL;
  this->prev = NULL;

  this->index = 0;
  this->entry_rate = 60.0/3600.0; // [veh/s]
  this->split_ratio = 0.1;
  this->entry_speed = -1; // -1 means we do not care
  this->merge_direction = 0;
  this->type = NONE;
  this->force_entry = 0;
  this->new_car = NULL;
  this->cumulative_rate = 0.0;
  this->cars.clear();
  this->left_markings.clear();
  this->right_markings.clear();

  this->minimum_speed = 120.0/3.6; // [m/s]
  this->maximum_speed = 120.0/3.6; // [m/s]

  memset(name, 0, sizeof(name));
}

Lane::~Lane()
{
  if (new_car) delete new_car;
  pthread_mutex_destroy(&(this->cars_mutex));

#ifdef LUA
  delete this->luaLane;
#endif
}

double Lane::allowedLeft(double p)
{
  if (!this->left) {
    if (this->segment->geometry == STRAIGHT) return this->segment->length;
    else return fabs(this->segment->angle);
  }
  for (unsigned int i = 0; i < this->left_markings.size(); i++) {
    if (this->left_markings[i].type == SOLID && this->left_markings[i].start < p && p < this->left_markings[i].end) {
      return this->left_markings[i].end;
    }
  }
  return 0.0;
}

double Lane::allowedRight(double p)
{
  if (!this->right) {
    if (this->segment->geometry == STRAIGHT) return this->segment->length;
    else return fabs(this->segment->angle);
  }
  for (unsigned int i = 0; i < this->right_markings.size(); i++) {
    if (this->right_markings[i].type == SOLID && this->right_markings[i].start < p && p < this->right_markings[i].end) {
      return this->right_markings[i].end;
    }
  }
  return 0.0;
}

#ifdef LUA
LuaLane *Lane::getLuaLane()
{
  return this->luaLane;
}

void Lane::setLuaLane(LuaLane *lane)
{
  this->luaLane = lane;
}
#endif

Segment::Segment()
{

}

Segment::~Segment()
{

}

Map::Map(gengetopt_args_info *options)
{
  this->options = options;
  /* Set starting time of day */
  int h, m;
  sscanf(options->start_time_arg, "%d:%d", &h,  &m);
  this->start_time = h*60+m;

#ifdef LUA
  this->luaInfrastructure = new LuaInfrastructure(NULL);
  this->luaInfrastructure->setSelf(this);
#endif

  createMap(options);
  createCoordinates();
  prepare();
}

Map::~Map()
{
#ifdef LUA
  delete this->luaInfrastructure;
#endif

  clear();
}

void Map::createMap(gengetopt_args_info *options)
{
  Segment *s;
  char cmd[100];
  if (!options->map_given) {
    char *str = (char *)malloc(strlen(options->map_arg)+1);
    if (str) {
      strcpy(str, options->map_arg);
      // free the string and recreate
      free(options->map_arg);
      options->map_arg = (char*)malloc(strlen(options->exe_path_arg) + strlen(str) + 2);
      if (options->map_arg) {
        sprintf(options->map_arg, "%s/%s", options->exe_path_arg, str);
        options->map_given = 1;
        free(str);
      }
    }
  }
  printf("Reading map: %s\n", options->map_arg);

  this->lane_width = 3.5;

  FILE *fp = fopen(options->map_arg, "r");
  if (!fp) {
    fprintf(stderr, "File does not exist: loading default map.\n");
    createDefaultMap();
    return;
  }

  while (fgets(cmd, 100, fp)) {
    cmd[strlen(cmd)-1] = '\0';
    char *cid = strtok(cmd, ",");
    if (!cid) continue;
    if  (strcmp(cid, "$END\n") == 0) break;

    if (strcmp(cid, "$LANE_WIDTH") == 0) {
      this->lane_width = atof(strtok(NULL, ","));
      Log::getStream(4) << "Lane width = " << setiosflags(ios::fixed) << setprecision(2) << this->lane_width << endl;

    } else if (strcmp(cid, "$NAME") == 0) {
      char *str = strtok(NULL, ",");
      this->name = (char *)malloc(strlen(str)+1);
      strcpy(this->name, str);
      //this->name[strlen(str)] = '\0';
      Log::getStream(4) << "Map name = " << this->name << endl;

    } else if (strcmp(cid, "$SEGMENT") == 0) {

      s = new Segment();
      s->type = NONE;
      s->next = NULL;
      if (segments.empty()) s->prev = NULL;
      else {
        s->prev = segments.back();
        segments.back()->next = s;
      }
      s->length = 100.0;
      s->geometry = STRAIGHT;
      s->side = RIGHT;
      s->offset = 0;
      if (s->prev) s->speed = s->prev->speed;
      else s->speed = 120.0/3.6;
      segments.push_back(s);

      char *type = strtok(NULL, ",");
      if (strcmp(type, "straight") == 0) {
        s->geometry = STRAIGHT;
        s->length = atof(strtok(NULL, ","));
      } else {
        s->geometry = CIRCULAR;
        s->radius = atof(strtok(NULL, ","));
        s->angle = atof(strtok(NULL, ","))/180.0*M_PI;
      }

    } else if (strcmp(cid, "$TYPE") == 0) {
      char *type = strtok(NULL, ",");
      if (strcmp(type, "entry") == 0) {
        s->type = ENTRY;
      } else if (strcmp(type, "exit") == 0) {
        s->type = EXIT;
      }

      type = strtok(NULL, ",");
      if (type && strcmp(type, "left") == 0) {
        s->side = LEFT;
      } else if (type && strcmp(type, "right") == 0) {
        s->side = RIGHT;
      }

    } else if (strcmp(cid, "$SPEED") == 0) {
      s->speed = atof(strtok(NULL, ","))/3.6;

    } else if (strcmp(cid, "$NUM_LANES") == 0) {
      int nlanes_keep = atoi(strtok(NULL, ","));
      int nlanes_new  = 0;
      char *str = strtok(NULL, ",");
      if (str) nlanes_new  = atoi(str);
      
      if (s->type == ENTRY) {
        s->nlanes = nlanes_keep + nlanes_new;
        s->base_offset = 0;
        if (s->side == RIGHT) {
          if (s->prev) {
            s->offset = s->prev->nlanes-nlanes_keep+s->prev->base_offset;
          }
          for (int i = 0; i < nlanes_keep; i++) {
            addNewLane(s, NONE);
          }
          for (int i = 0; i < nlanes_new; i++) {
            addNewLane(s, ENTRY);
          }
        } else {
          if (s->prev) {
            s->offset = -nlanes_new+s->prev->base_offset;
          }
          for (int i = 0; i < nlanes_new; i++) {
            addNewLane(s, ENTRY);
          }
          for (int i = 0; i < nlanes_keep; i++) {
            addNewLane(s, NONE);
          }
        }
      } else if (s->type == EXIT) {
        s->nlanes = nlanes_keep;
        if (s->side == RIGHT) {
          s->base_offset = 0;
          if (s->prev) {
            s->offset = s->prev->nlanes-nlanes_keep-nlanes_new+s->prev->base_offset;
            if (s->offset < 0) s->offset = 0;
          }
          for (int i = 0; i < nlanes_keep; i++) {
            addNewLane(s, NONE);
          }
          for (int i = 0; i < nlanes_new; i++) {
            addNewLane(s, EXIT);
          }
        } else {
          s->base_offset = nlanes_new;
          if (s->prev) {
            if (nlanes_keep == 0)
              s->offset = s->prev->base_offset;
            else
              s->offset = -s->prev->nlanes+nlanes_keep+nlanes_new+s->prev->base_offset;
            if (s->offset > 0) s->offset = 0;
          }
          for (int i = 0; i < nlanes_new; i++) {
            addNewLane(s, EXIT);
          }
          for (int i = 0; i < nlanes_keep; i++) {
            addNewLane(s, NONE);
          }
        }
      } else {
        s->base_offset = 0;
        s->nlanes = nlanes_keep;
        if (s->side == RIGHT) {
          if (s->prev) {
            s->offset = s->prev->nlanes-nlanes_keep+s->prev->base_offset;
          }
          for (int i = 0; i < nlanes_keep; i++) {
            addNewLane(s, NONE);
          }
        } else {
          if (s->prev) {
            s->offset = s->prev->base_offset;
          }
          for (int i = 0; i < nlanes_keep; i++) {
            addNewLane(s, NONE);
          }
        }
      }

      if (s->prev && !s->lanes.empty()) {
        for (unsigned int j = 0; j < s->prev->lanes.size(); j++) {
          Lane *lt = s->prev->lanes[j];
          if ((int)j < s->offset && lt->type != EXIT) {
            lt->next = s->lanes[0];
            lt->merge_direction = 1;
          }
          if ((int)j >= s->offset + (int)s->lanes.size() && lt->type != EXIT) {
            lt->next = s->lanes.back();
            lt->merge_direction = -1;
          }
        }
      }

    } else if (strcmp(cid, "$LANE") == 0) {
      int id = atoi(strtok(NULL, ","));
      double rate = atof(strtok(NULL, ","));
      char *name = strtok(NULL, ",");
      if (rate > 10000.0) rate = 10000.0;
      else if (rate < 0.0) rate = 0.0;
      if (s->lanes[id]->type == ENTRY)
        s->lanes[id]->entry_rate = rate/3600.0; // [veh/s]
      else if (s->lanes[id]->type == EXIT)
        s->lanes[id]->split_ratio = rate; // This is in percent
      if (name) strncpy(s->lanes[id]->name, name, 254);

    } else if (strcmp(cid, "$LEFT_MARKING") == 0) {
      int id = atoi(strtok(NULL, ","));
      double start = atof(strtok(NULL, ","));
      double end = atof(strtok(NULL, ","));
      Marking marking;
      char *type = strtok(NULL, ",");
      if (strcmp(type, "solid") == 0) {
        // ignore broken... it is assumed per default
        marking.type = SOLID;
        if (s->geometry == CIRCULAR) {
          start *= M_PI/180.0;
          end *= M_PI/180.0;
        }
        marking.start = start;
        marking.end = end;
        s->lanes[id]->left_markings.push_back(marking);
      }
      
    } else if (strcmp(cid, "$RIGHT_MARKING") == 0) {
      int id = atoi(strtok(NULL, ","));
      double start = atof(strtok(NULL, ","));
      double end = atof(strtok(NULL, ","));
      Marking marking;
      char *type = strtok(NULL, ",");
      if (strcmp(type, "solid") == 0) {
        // ignore broken... it is assumed per default
        marking.type = SOLID;
        if (s->geometry == CIRCULAR) {
          start *= M_PI/180.0;
          end *= M_PI/180.0;
        }
        marking.start = start;
        marking.end = end;
        s->lanes[id]->right_markings.push_back(marking);
      }

    } else if (strcmp(cid, "$DENSITY_SENSOR") == 0) {
      char *name = strtok(NULL, ",");
      int id = atoi(strtok(NULL, ","));
      double position = atof(strtok(NULL, ","));
      double position2 = atof(strtok(NULL, ","));
      char *lstr = strtok(NULL,",");
      if (s->geometry == CIRCULAR) {
        position *= M_PI/180.0;
        position2 *= M_PI/180.0;
      }
      Lane *l = s->lanes[id];
      RoadSensor *r = new RoadSensor(name, DENSITY, l, position, position2, options);
      r->lane = l;
      l->sensors.push_back(r);
      sensors.push_back(r);
      if (options->record_given && (!lstr || strcmp(lstr,"log")==0)) r->log();

    } else if (strcmp(cid, "$SPEED_SENSOR") == 0) {
      char *name = strtok(NULL, ",");
      int id = atoi(strtok(NULL, ","));
      double position = atof(strtok(NULL, ","));
      char *lstr = strtok(NULL,",");
      if (s->geometry == CIRCULAR) {
        position *= M_PI/180.0;
      }
      Lane *l = s->lanes[id];
      RoadSensor *r = new RoadSensor(name, SPEED, l, position, options);
      r->lane = l;
      l->sensors.push_back(r);
      sensors.push_back(r);
      if (options->record_given && (!lstr || strcmp(lstr,"log")==0)) r->log();
      
    } else if (strcmp(cid, "$FLOW_SENSOR") == 0) {
      char *name = strtok(NULL, ",");
      int id = atoi(strtok(NULL, ","));
      double position = atof(strtok(NULL, ","));
      char *lstr = strtok(NULL,",");
      if (s->geometry == CIRCULAR) {
        position *= M_PI/180.0;
      }
      Lane *l = s->lanes[id];
      RoadSensor *r = new RoadSensor(name, FLOW, l, position, options);
      r->lane = l;
      l->sensors.push_back(r);
      sensors.push_back(r);
      if (options->record_given && (!lstr || strcmp(lstr,"log")==0)) r->log();

    } else if (strcmp(cid, "$TRAFFIC_LIGHT") == 0) {
      char *name = strtok(NULL, ",");
      int id = atoi(strtok(NULL, ","));
      double position = atof(strtok(NULL, ","));
      char *lstr = strtok(NULL,",");
      if (s->geometry == CIRCULAR) {
        position *= M_PI/180.0;
      }
      Lane *l = s->lanes[id];
      TrafficLightActuator *r = new TrafficLightActuator(name, l, position, options);
      r->lane = l;
      l->actuators.push_back(r);
      actuators.push_back(r);
      if (options->record_given && (!lstr || strcmp(lstr,"log")==0)) r->log();

    } else if (strcmp(cid, "$SPEED_LIMIT") == 0) {
      char *name = strtok(NULL, ",");
      int id = atoi(strtok(NULL, ","));
      double position = atof(strtok(NULL, ","));
      if (s->geometry == CIRCULAR) {
        position *= M_PI/180.0;
      }
      Lane *l = s->lanes[id];
      SpeedLimitActuator *r = new SpeedLimitActuator(name, l, position, options);
      r->lane = l;
      l->actuators.push_back(r);
      actuators.push_back(r);
        
    } else if (strcmp(cid, "$CLOSE_THE_LOOP") == 0) {
      if (!segments.empty()) {
        Segment *e = segments[0];
        int cnt = s->base_offset;
        s->next = e;
        e->prev = s;
        for (unsigned int i = 0; i < e->lanes.size(); i++) {
          if (e->lanes[i]->type != ENTRY) {
            e->lanes[i]->prev = s->lanes[cnt];
            s->lanes[cnt]->next = e->lanes[i];
            cnt++;
          }
        }

        if (e->prev && !e->lanes.empty()) {
          for (unsigned int j = 0; j < e->prev->lanes.size(); j++) {
            Lane *lt = e->prev->lanes[j];
            if ((int)j < e->offset && lt->type != EXIT) {
              lt->next = e->lanes[0];
            }
            if ((int)j >= e->offset + (int)e->lanes.size() && lt->type != EXIT) {
              lt->next = e->lanes.back();
            }
          }
        }
      }
    }
  }
  fclose(fp);
}

Lane * Map::addNewLane(Segment *s, lane_t t)
{
  Lane *l = new Lane();
  
  l->segment = s;
  if (!s->lanes.empty()) {
    l->left = s->lanes.back();
    s->lanes.back()->right = l;
  }
  if (s->prev &&
      (int)s->lanes.size()+s->offset >= 0 &&
      (int)s->lanes.size()+s->offset < s->prev->nlanes+s->prev->base_offset) {
    l->prev = s->prev->lanes[s->lanes.size()+s->offset];
    s->prev->lanes[s->lanes.size()+s->offset]->next = l;
  }
  l->index = s->lanes.size();
  l->type = t;
  l->minimum_speed = s->speed;
  l->maximum_speed = s->speed;

  s->lanes.push_back(l);

  return l;
}

void Map::createDefaultMap()
{
  Segment *s1 = new Segment();
  Segment *s2 = new Segment();
  s1->type = ENTRY;
  s2->type = EXIT;
  s1->next = s2;
  s1->prev = NULL;
  s2->next = NULL;
  s2->prev = s1;
  s1->length = 500.0;
  s2->length = 500.0;
  s1->geometry = STRAIGHT;
  s2->geometry = STRAIGHT;
  s1->offset = 0;
  s2->offset = 0;
  s1->speed = 30.0;
  s2->speed = 30.0;

  Lane *l11 = new Lane();
  Lane *l12 = new Lane();
  Lane *l13 = new Lane();
  Lane *l21 = new Lane();
  Lane *l22 = new Lane();
  Lane *l23 = new Lane();
  l11->new_car = NULL;
  l11->cumulative_rate = NULL;
  l11->force_entry = 0;
  l12->new_car = NULL;
  l12->cumulative_rate = NULL;
  l12->force_entry = 0;
  l13->new_car = NULL;
  l13->cumulative_rate = NULL;
  l13->force_entry = 0;
  l11->merge_direction = 0;
  l12->merge_direction = 0;
  l13->merge_direction = 0;
  l21->merge_direction = 0;
  l22->merge_direction = 0;
  l23->merge_direction = 0;
  l11->type = s1->type;
  l12->type = s1->type;
  l13->type = s1->type;
  l21->type = s2->type;
  l22->type = s2->type;
  l23->type = s2->type;
  l11->segment = s1;
  l12->segment = s1;
  l13->segment = s1;
  l21->segment = s2;
  l22->segment = s2;
  l23->segment = s2;
  l11->left = NULL;
  l11->right = l12;
  l12->left = l11;
  l12->right = l13;
  l13->left = l12;
  l13->right = NULL;
  l21->left = NULL;
  l21->right = l22;
  l22->left = l21;
  l22->right = l23;
  l23->left = l22;
  l23->right = NULL;
  l11->next = l21;
  l11->prev = NULL;
  l12->next = l22;
  l12->prev = NULL;
  l13->next = l23;
  l13->prev = NULL;
  l21->prev = l11;
  l21->next = NULL;
  l22->prev = l12;
  l22->next = NULL;
  l23->prev = l13;
  l23->next = NULL;
  l11->entry_rate = 0.1;
  l12->entry_rate = 0.1;
  l13->entry_rate = 0.1;
  l11->index = 0;
  l12->index = 1;
  l13->index = 2;
  l21->index = 0;
  l22->index = 1;
  l23->index = 2;

  s1->lanes.push_back(l11);
  s1->lanes.push_back(l12);
  s1->lanes.push_back(l13);
  s2->lanes.push_back(l21);
  s2->lanes.push_back(l22);
  s2->lanes.push_back(l23);

  segments.push_back(s1);
  segments.push_back(s2);

  lane_width = 3.5;
}

#ifdef LUA
LuaInfrastructure *Map::getLuaInfrastructure()
{
  return this->luaInfrastructure;
}

void Map::setLuaInfrastructure(LuaInfrastructure *infrastructure)
{
  this->luaInfrastructure = infrastructure;
}
#endif

void Map::clear()
{
  for (unsigned int i = 0; i < segments.size(); i++) {
    Segment *s = segments[i];
    for (unsigned int j = 0; j < s->lanes.size(); j++) {
      Lane *l = s->lanes[j];
      l->sensors.clear();
      l->actuators.clear();
      delete l;
    }
    s->lanes.clear();
    delete s;
  }
  segments.clear();
  entries.clear();

  for (unsigned int k = 0; k < sensors.size(); k++) {
    RoadSensor *r = sensors[k];
    delete r;
  }
  sensors.clear();

  for (unsigned int k = 0; k < actuators.size(); k++) {
    RoadActuator *r = actuators[k];
    delete r;
  }
  actuators.clear();
}

void Map::prepare()
{
  for (unsigned int i = 0; i < segments.size(); i++) {
    Segment *s = segments[i];
    for (unsigned int j = 0; j < s->lanes.size(); j++) {
      Lane *l = s->lanes[j];
      if (l->type == ENTRY) {
        entries.push_back(l);
      }
    }
  }
}

void Map::createCoordinates()
{
  // Start point of the left most lane
  double x = 0.0;
  double y = 0.0;
  double a = 0.0;

  for (unsigned int i = 0; i < segments.size(); i++) {
    Segment *s = segments[i];

    // Correct x,y with offset
    double dir = (s->offset < 0.0)?-1.0:1.0;
    double len = fabs(s->offset)*lane_width;

    x += cos(a+dir*M_PI/2.0)*len;
    y += sin(a+dir*M_PI/2.0)*len;

    s->x = x;
    s->y = y;
    s->a = a;

    for (unsigned int j = 0; j < s->lanes.size(); j++) {
      Lane *l = s->lanes[j];
      l->x_start = x + cos(a+M_PI/2.0)*(lane_width/2.0 + ((double)j)*lane_width);
      l->y_start = y + sin(a+M_PI/2.0)*(lane_width/2.0 + ((double)j)*lane_width);
      l->a_start = a;
    }
    
    if (s->geometry == STRAIGHT) {
      x += cos(a)*s->length;
      y += sin(a)*s->length;
    } else {
      // Center of the circle
      double xc;
      double yc;
      if (s->angle > 0.0) {
        xc = x + cos(a+M_PI/2.0)*s->radius;
        yc = y + sin(a+M_PI/2.0)*s->radius;
      } else {
        xc = x + cos(a-M_PI/2.0)*s->radius;
        yc = y + sin(a-M_PI/2.0)*s->radius;
      }
      // Translate
      x -= xc;
      y -= yc;
      // Rotate
      double t = x*cos(s->angle) - y*sin(s->angle);
      y = x*sin(s->angle) + y*cos(s->angle) + yc;
      x = t + xc;
      a += s->angle;

      // Fill fast geometry
      for (unsigned int j = 0; j < s->lanes.size(); j++) {
        Lane *l = s->lanes[j];
        l->xc = xc;
        l->yc = yc;
        l->radius = sqrt((xc-l->x_start)*(xc-l->x_start) + (yc-l->y_start)*(yc-l->y_start));
        l->angle_start = atan2(yc-l->y_start, xc-l->x_start);
      }
    }

    for (unsigned int j = 0; j < s->lanes.size(); j++) {
      Lane *l = s->lanes[j];
      l->x_end = x + cos(a+M_PI/2.0)*(lane_width/2.0 + ((double)j)*lane_width);
      l->y_end = y + sin(a+M_PI/2.0)*(lane_width/2.0 + ((double)j)*lane_width);
      l->a_end = a;
    }
  }
}

RoadSensor::RoadSensor(char *name, sensor_t type, Lane *l, double position, gengetopt_args_info *options)
{
  this->type = type;
  this->position = position;
  this->position2 = position;
  this->t = 0.0;
  this->current_time = 0.0;
  this->count = 0;
  this->speed = 0.0;
  this->delta = 60.0; // Every 60 seconds the data is computed and logged
  this->result = 0.0;
  strncpy(this->name, name, 255);
  this->name[255] = '\0';
  this->lane = l;
  this->distance = 0.0;
  this->cnt = 0;
  this->density = 0.0;
  this->trigger_delay = 0.0;
  this->trigger_delay2 = 0.0;
  this->distance = 1.0;
  this->occupied = false;

  if (type == DENSITY) {
    fprintf(stderr, "Warning: A density sensor needs 2 positions defined.\n");
  }

  snprintf(filename, 255, "%s/%s.txt", options->record_path_arg, name);
  filename[255] = '\0';
  file = NULL;

#ifdef LUA
  this->luaRoadSensor = new LuaRoadSensor(NULL);
  this->luaRoadSensor->setSelf(this);
#endif

  pthread_mutex_init(&(this->mutex), NULL);
}

RoadSensor::RoadSensor(char *name, sensor_t type, Lane *l, double position1, double position2, gengetopt_args_info *options)
{
  this->type = type;
  this->position = position1;
  this->position2 = position2;
  this->t = 0.0;
  this->current_time = 0.0;
  this->count = 0;
  this->speed = 0.0;
  this->speed_std = 0.0;
  this->delta = 60.0; // Every 60 seconds the data is computed and logged
  this->result = 0.0;
  strncpy(this->name, name, 255);
  this->name[255] = '\0';
  this->lane = l;
  this->cnt = 0;
  this->density = 0.0;
  this->trigger_delay = 0.0;
  this->trigger_delay2 = 0.0;
  this->distance = position2 - position1;
  this->occupied = false;

  snprintf(filename, 255, "%s/%s.txt", options->record_path_arg, name);
  filename[255] = '\0';
  file = NULL;

#ifdef LUA
  this->luaRoadSensor = new LuaRoadSensor(NULL);
  this->luaRoadSensor->setSelf(this);
#endif

  pthread_mutex_init(&(this->mutex), NULL);
}

RoadSensor::~RoadSensor()
{
#ifdef LUA
  delete this->luaRoadSensor;
#endif

  if (file) fclose(file);
  pthread_mutex_destroy(&(this->mutex));
}

void RoadSensor::log()
{
  file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Unable to open file: %s\n", filename);
  } else {
    Log::getStream(4) << "File " << filename << " open to record data" << endl;
  }
}

void RoadSensor::update(double dt)
{
  this->cnt++;
  
  // Compute data
  if (type == DENSITY) {
    if (lane->segment->geometry == CIRCULAR) {
      density += (double)(count)/(distance*lane->radius)*1000.0; // veh/km
    } else {
      density += (double)(count)/distance*1000.0; // veh/km
    }
  }

  if (this->t > this->delta) {
    switch (type) {
    case DENSITY:
      result = density/(double)cnt; // average density
      density = 0.0;
      if (file) {
        fprintf(file, "%.2f %.2f\n", current_time, result);
      }
      break;
    case SPEED:
      result = speed/(double)count; // km/h
      speed_std = sqrt(speed_std/(double)count - (result*result));
      if (file) {
        fprintf(file, "%.2f %.2f %.2f\n", current_time, result, speed_std);
      }
      count = 0;
      speed = 0.0;
      speed_std = 0.0;
      break;
    case FLOW:
      result = (double)count/this->t*3600.0; // veh/h
      count = 0;
      if (file) {
        fprintf(file, "%.2f %.2f\n", current_time, result);
      }
      break;
    }

    // Reset timer
    this->cnt = 0;
    this->t = 0.0;
  }

  this->current_time += dt;
  this->t += dt;
  this->trigger_delay -= dt;
  this->trigger_delay2 -= dt;
  this->occupied = false;
}

bool RoadSensor::isOccupied()
{
  return occupied;
}

void RoadSensor::trigger(Car *c, double old_x, double new_x)
{
  double front, rear, p1, p2;
  c->getCarGeometry(&front, &rear);
  if (this->lane->segment->geometry == CIRCULAR) {
    p1 = front/this->lane->radius + new_x;
    p2 = -rear/this->lane->radius + new_x;
  } else {
    p1 = front + new_x;
    p2 = -rear + new_x;
  }

  switch (type) {
  case DENSITY:
    if (old_x < position && new_x > position) {
      pthread_mutex_lock(&(this->mutex));
      count++;
      pthread_mutex_unlock(&(this->mutex));
      trigger_delay = 0.5;
    }
    if (old_x < position2 && new_x > position2) {
      pthread_mutex_lock(&(this->mutex));
      count--;
      pthread_mutex_unlock(&(this->mutex));
      trigger_delay2 = 0.5;
    }
    if (p1 > position &&  p2 < position2) occupied = true;
    break;
  case SPEED:
    if (old_x < position && new_x > position) {
      pthread_mutex_lock(&(this->mutex));
      count++;
      speed += c->getSpeed()*3.6;
      speed_std += (c->getSpeed()*3.6)*(c->getSpeed()*3.6); // square of speed
      pthread_mutex_unlock(&(this->mutex));
      trigger_delay = 0.5;
    }
    if (p1 > position && p2 < position) occupied = true;
    break;
  case FLOW:
    if (old_x < position && new_x > position) {
      pthread_mutex_lock(&(this->mutex));
      count++;
      pthread_mutex_unlock(&(this->mutex));
      trigger_delay = 0.5;
    }
    if (p1 > position && p2 < position) occupied = true;
    break;
  }
}

unsigned int RoadSensor::getCount()
{
  return this->count;
}

void RoadSensor::setRate(double rate)
{
  this->delta = 1.0/rate;
}

bool RoadSensor::wasTriggered()
{
  return (this->trigger_delay > 0.0);
}

bool RoadSensor::wasEntryTriggered()
{
  return wasTriggered();
}

bool RoadSensor::wasExitTriggered()
{
  return (this->trigger_delay2 > 0.0);
}

double RoadSensor::getResult()
{
  return this->result;
}

void RoadSensor::notifyExit(Car *c)
{
  if (this->type != DENSITY) return;

  if (c->getPosition() < this->position2 && c->getPosition() > this->position) {
    pthread_mutex_lock(&(this->mutex));
    count--;
    pthread_mutex_unlock(&(this->mutex));
  }
}

void RoadSensor::notifyEntry(Car *c)
{
  if (this->type != DENSITY) return;

  if (c->getPosition() < this->position2 && c->getPosition() > this->position) {
    pthread_mutex_lock(&(this->mutex));
    count++;
    pthread_mutex_unlock(&(this->mutex));
  }
}

LuaRoadSensor *RoadSensor::getLuaRoadSensor()
{
  return this->luaRoadSensor;
}

void RoadSensor::setLuaRoadSensor(LuaRoadSensor *rs)
{
  this->luaRoadSensor = rs;
}

Marking::Marking()
{
  type = BROKEN;
  start = 0;
  end = 0;
}

Marking::~Marking()
{

}

RoadActuator::RoadActuator(char *name, actuator_t type, Lane *l, double position, gengetopt_args_info *options)
{
  strncpy(this->name, name, 255);
  this->name[255] = '\0';
  this->type = type;
  this->position = position;

  this->t = 0.0;
  this->result = 0.0;
  this->immediate_result = 0.0;
  this->immediate_read = false;
  this->result_count = 0;
  this->current_time = 0.0;
  this->queue_time = 0.0;
  this->count = 0;
  this->delta = 60.0; // Every 60 seconds the data is computed and logged

  snprintf(filename, 255, "%s/%s.txt", options->record_path_arg, name);
  filename[255] = '\0';
  file = NULL;

  pthread_mutex_init(&(this->mutex), NULL);


#ifdef LUA
  this->luaRoadActuator = new LuaRoadActuator(NULL);
  this->luaRoadActuator->setSelf(this);
#endif
}

RoadActuator::~RoadActuator()
{
#ifdef LUA
  delete this->luaRoadActuator;
#endif
  
  if (file) fclose(file);
  pthread_mutex_destroy(&(this->mutex));
}

void RoadActuator::trigger(Car *c, double old_x, double new_x)
{
  if (type != TRAFFICLIGHT) return;

  double front, rear, p1, p2;
  c->getCarGeometry(&front, &rear);
  if (this->lane->segment->geometry == CIRCULAR) {
    p1 = front/this->lane->radius + new_x;
    p2 = -rear/this->lane->radius + new_x;
  } else {
    p1 = front + new_x;
    p2 = -rear + new_x;
  }

  if (old_x < position && new_x > position) {
    pthread_mutex_lock(&(this->mutex));
    count++;
    immediate_result = c->getTimeAlive();
    immediate_read = true;
    queue_time += c->getTimeAlive();
    c->resetTimeAlive();
    pthread_mutex_unlock(&(this->mutex));
  }
}

void RoadActuator::log()
{
  file = fopen(filename, "w");
  if (!file) {
    fprintf(stderr, "Unable to open file: %s\n", filename);
  } else {
    Log::getStream(4) << "File " << filename << " open to record data" << endl;
  }
}

double RoadActuator::getResult()
{
  return this->result;
}

double RoadActuator::getInstantResult(bool *new_value)
{
  if (new_value) *new_value = immediate_read;
  immediate_read = false;
  return this->immediate_result;
}

unsigned int RoadActuator::getCount()
{
  return this->result_count;
}

void RoadActuator::update(double dt)
{
  if (type != TRAFFICLIGHT) return;

  if (this->t > this->delta) {
    result = queue_time/(double)count; // average queueing time
    result_count = count;
    queue_time = 0.0;
    count = 0;
    if (file) {
      fprintf(file, "%.2f %.2f %d\n", current_time, result, result_count);
    }

    // Reset timer
    this->t = 0.0;
  }

  this->current_time += dt;
  this->t += dt;
}

#ifdef LUA
LuaRoadActuator *RoadActuator::getLuaRoadActuator()
{
  return luaRoadActuator;
}

void RoadActuator::setLuaRoadActuator(LuaRoadActuator *ra)
{
  this->luaRoadActuator = ra;
}
#endif

TrafficLightActuator::TrafficLightActuator(char *name, Lane *l, double position, gengetopt_args_info *options):
  RoadActuator(name, TRAFFICLIGHT, l, position, options)
{
  state = GREEN;
}

TrafficLightActuator::~TrafficLightActuator()
{

}

void TrafficLightActuator::red()
{
  this->state = RED;
}

void TrafficLightActuator::green()
{
  this->state = GREEN;
}

color_t TrafficLightActuator::color()
{
  return this->state;
}

SpeedLimitActuator::SpeedLimitActuator(char *name, Lane *l, double position, gengetopt_args_info *options):
  RoadActuator(name, SPEEDLIMIT, l, position, options)
{
  
}

SpeedLimitActuator::~SpeedLimitActuator()
{

}

void SpeedLimitActuator::setSpeedLimit(double max, double min)
{
  Lane *l = this->lane;

  if (min == -1) min = max;
  while (l) {
    l->maximum_speed = max;
    l->minimum_speed = min;
    l = l->next;
    if (!l) break;
    // Check if there is a speed limiter there
    bool ok = true;
    for (unsigned int i = 0; i < l->actuators.size(); i++) {
      if (l->actuators[i]->type == SPEEDLIMIT) {
        ok = false;
        break;
      }
    }
    if (!ok) break;
  }  
}
