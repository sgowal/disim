#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "LuaBinding.h"

#define lua_setConst(L,name) { lua_pushnumber(L,name); lua_setglobal(L,#name); }

LuaBinding::LuaBinding()
{
  ninstances = 0;
  path = NULL;
  controlpath = NULL;

  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    cwd[0] = '.';
    cwd[1] = '\0';
  }
}

LuaBinding::~LuaBinding()
{
  unload();
  for (int i = 0; i < ninstances; i++) {
    pthread_mutex_destroy(&(this->mutex[i]));
  }
  delete [] this->L;

  unloadControl();
}

void LuaBinding::unload()
{
  for (int i = 0; i < ninstances; i++) {
    pthread_mutex_lock(&(this->mutex[i]));
    if (this->L[i]) {
      lua_gc(this->L[i], LUA_GCCOLLECT, 0);
      lua_close(this->L[i]);
      this->L[i] = NULL;
    }
    pthread_mutex_unlock(&(this->mutex[i]));
  }

  if (path) {
    free(path);
    path = NULL;
  }
}

void LuaBinding::unloadControl()
{
  if (this->controlL) {
    lua_gc(this->controlL, LUA_GCCOLLECT, 0);
    lua_close(this->controlL);
    this->controlL = NULL;
  }

  if (controlpath) {
    free(controlpath);
    controlpath = NULL;
  }
}

int LuaBinding::loadControlFile(char *filename)
{
  if (access(filename, R_OK)) {
    fprintf(stderr, "The LUA script %s is not readable.\n", filename);
    return -1;
  }

  // Garbage collect and close if necessary
  if (this->controlL) {
    lua_gc(this->controlL, LUA_GCCOLLECT, 0);
    lua_close(this->controlL);
    this->controlL = NULL;
  }

  // Create a new stack
  this->controlL = luaL_newstate();

  // Open all libraries
  luaL_openlibs(this->controlL);

  // Set all constants
  lua_setConst(this->controlL, LEAD);
  lua_setConst(this->controlL, TRAIL);
  lua_setConst(this->controlL, LEFT_LEAD);
  lua_setConst(this->controlL, LEFT_TRAIL);
  lua_setConst(this->controlL, RIGHT_LEAD);
  lua_setConst(this->controlL, RIGHT_TRAIL);
  lua_setConst(this->controlL, REMOTE);
  lua_setConst(this->controlL, CAR);
  lua_setConst(this->controlL, TRUCK);
  lua_setConst(this->controlL, ENTRY);
  lua_setConst(this->controlL, EXIT);
  lua_setConst(this->controlL, NONE);
  lua_setConst(this->controlL, STRAIGHT);
  lua_setConst(this->controlL, CIRCULAR);
  lua_setConst(this->controlL, TRAFFICLIGHT);
  lua_setConst(this->controlL, SPEEDLIMIT);
  lua_setConst(this->controlL, DENSITY);
  lua_setConst(this->controlL, SPEED);
  lua_setConst(this->controlL, FLOW);
  lua_setConst(this->controlL, GREEN);
  lua_setConst(this->controlL, RED);

  // Register all functions
  Lunar<LuaCar>::Register(this->controlL);
  Lunar<LuaLane>::Register(this->controlL);
  Lunar<LuaRoadSensor>::Register(this->controlL);
  Lunar<LuaRoadActuator>::Register(this->controlL);
  Lunar<LuaInfrastructure>::Register(this->controlL);

  // Change the path to script
  if (controlpath) free(controlpath);
  controlpath = strdup(filename);
  char *p = strrchr(controlpath, '/');
  int index = 0;
  if (!p) {
    // It is safe to assume a non empty string here
    controlpath[0] = '.';
    controlpath[1] = '\0';
  } else {
    *p = '\0';
    index = (int)(p-controlpath)+1;
  }
  chdir(controlpath);

  // Launch the file
  if (luaL_dofile(this->controlL, filename+index)) {
    // error
    fprintf(stderr, "The LUA script is invalid: %s\n", lua_tostring(this->controlL, -1));
    lua_pop(this->controlL, 1);
    lua_gc(this->controlL, LUA_GCCOLLECT, 0);
    lua_close(this->controlL);
    this->controlL = NULL;
    chdir(cwd);
    return -1;
  }

  // Change it back
  chdir(cwd);

  return 0;
}

int LuaBinding::loadFile(char *filename)
{
  if (access(filename, R_OK)) {
    fprintf(stderr, "The LUA script %s is not readable.\n", filename);
    return -1;
  }

  if (path) free(path);
  path = strdup(filename);
  char *p = strrchr(path, '/');
  int index = 0;
  if (!p) {
    // It is safe to assume a non empty string here
    path[0] = '.';
    path[1] = '\0';
  } else {
    *p = '\0';
    index = (int)(p-path)+1;
  }

  for (int i = 0; i < ninstances; i++) {
    pthread_mutex_lock(&(this->mutex[i]));

    // Garbage collect and close if necessary
    if (this->L[i]) {
      lua_gc(this->L[i], LUA_GCCOLLECT, 0);
      lua_close(this->L[i]);
      this->L[i] = NULL;
    }

    // Create a new stack
    this->L[i] = luaL_newstate();

    // Open all libraries
    luaL_openlibs(this->L[i]);

    // Set all constants
    lua_setConst(this->L[i], LEAD);
    lua_setConst(this->L[i], TRAIL);
    lua_setConst(this->L[i], LEFT_LEAD);
    lua_setConst(this->L[i], LEFT_TRAIL);
    lua_setConst(this->L[i], RIGHT_LEAD);
    lua_setConst(this->L[i], RIGHT_TRAIL);
    lua_setConst(this->L[i], REMOTE);
    lua_setConst(this->L[i], CAR);
    lua_setConst(this->L[i], TRUCK);
    lua_setConst(this->L[i], ENTRY);
    lua_setConst(this->L[i], EXIT);
    lua_setConst(this->L[i], NONE);
    lua_setConst(this->L[i], STRAIGHT);
    lua_setConst(this->L[i], CIRCULAR);
    lua_setConst(this->L[i], TRAFFICLIGHT);
    lua_setConst(this->L[i], SPEEDLIMIT);
    lua_setConst(this->L[i], DENSITY);
    lua_setConst(this->L[i], SPEED);
    lua_setConst(this->L[i], FLOW);
    lua_setConst(this->L[i], GREEN);
    lua_setConst(this->L[i], RED);

    // Register all functions
    Lunar<LuaCar>::Register(this->L[i]);
    Lunar<LuaLane>::Register(this->L[i]);
    Lunar<LuaRoadSensor>::Register(this->L[i]);
    Lunar<LuaRoadActuator>::Register(this->L[i]);
    Lunar<LuaInfrastructure>::Register(this->L[i]);

    // Change the path to script
    chdir(path);

    // Launch the file
    if (luaL_dofile(this->L[i], filename+index)) {
      // error
      fprintf(stderr, "The LUA script is invalid: %s\n", lua_tostring(this->L[i], -1));
      lua_pop(this->L[i], 1);
      lua_gc(this->L[i], LUA_GCCOLLECT, 0);
      lua_close(this->L[i]);
      this->L[i] = NULL;
      /* This can be ignored since the script will fail for the first one */
#if 0
      for (int j = 0; j < i; j++) {
        pthread_mutex_lock(&(this->mutex[j]));
        lua_gc(this->L[j], LUA_GCCOLLECT, 0);
        lua_close(this->L[j]);
        this->L[j] = NULL;
        pthread_mutex_unlock(&(this->mutex[j]));
      }
#endif
      chdir(cwd);
      pthread_mutex_unlock(&(this->mutex[i]));
      return -1;
    }

    chdir(cwd);
    pthread_mutex_unlock(&(this->mutex[i]));
  }

  return 0;
}

int LuaBinding::callInit(LuaCar *self)
{
  // This pattern has to be the same than in Simulator otherwise
  // there will be performances loss.
  int i = self->getSelf()->getID() % ninstances;

  pthread_mutex_lock(&(this->mutex[i]));
  if (!this->L[i]) {
    pthread_mutex_unlock(&(this->mutex[i]));
    return -1;
  }

  // Get the function
  lua_getglobal(L[i], "init");

  // Push arguments
  Lunar<LuaCar>::push(this->L[i], self);
  lua_pushstring(this->L[i], options->lua_args_arg);

  // Call the function with 1 argument and 0 returns
  int r = lua_pcall(L[i], 2, 0, 0);

  // Check error
  if (r) {
    fprintf(stderr, "Controller error in init: %s\n", lua_tostring(this->L[i], -1));
    lua_pop(this->L[i], 1);
    lua_gc(this->L[i], LUA_GCCOLLECT, 0);
    lua_close(this->L[i]);
    this->L[i] = NULL;
    pthread_mutex_unlock(&(this->mutex[i]));
    return -1;
  }

  pthread_mutex_unlock(&(this->mutex[i]));

  return 0;
}

int LuaBinding::callThink(LuaCar *self, double dt, vector<struct neighbor_struct> &neighbors)
{
  int j = self->getSelf()->getID() % ninstances;

  pthread_mutex_lock(&(this->mutex[j]));
  if (!this->L[j]) {
    pthread_mutex_unlock(&(this->mutex[j]));
    return -1;
  }

    // Get the function
  lua_getglobal(L[j], "think");

  // Push arguments
  Lunar<LuaCar>::push(this->L[j], self);
  lua_pushnumber(this->L[j], dt);
  // Push the whole array
  lua_newtable(this->L[j]);
  for (unsigned int i = 0; i < neighbors.size(); i++) {
    // Put the table index
    lua_pushnumber(L[j], i);

    // Push the cell value

    // Here we have to store another table with keys car and distance
    lua_newtable(this->L[j]);
    // Store the car key
    lua_pushstring(L[j], "car");
    // Store car pointer
    if (neighbors[i].car)
      Lunar<LuaCar>::push(this->L[j], neighbors[i].car->getControl()->getLuaCar());
    else
      Lunar<LuaCar>::push(this->L[j], NULL);
    lua_rawset(L[j], -3);
    // Store the distance key
    lua_pushstring(L[j], "distance");
    lua_pushnumber(this->L[j], neighbors[i].distance);
    lua_rawset(L[j], -3);

    // Put the whole in the main table
    lua_rawset(L[j], -3);
  }

  // Call the function with 3 argument and 0 returns
  int r = lua_pcall(L[j], 3, 0, 0);

  // Check error
  if (r) {
    fprintf(stderr, "Controller error think: %s\n", lua_tostring(this->L[j], -1));
    lua_pop(this->L[j], 1);
    lua_gc(this->L[j], LUA_GCCOLLECT, 0);
    lua_close(this->L[j]);
    this->L[j] = NULL;
    pthread_mutex_unlock(&(this->mutex[j]));
    return -1;
  }

  pthread_mutex_unlock(&(this->mutex[j]));

  return 0;
}

int LuaBinding::callDestroy(LuaCar *self)
{
  int i = self->getSelf()->getID() % ninstances;

  pthread_mutex_lock(&(this->mutex[i]));
  if (!this->L[i]) {
    pthread_mutex_unlock(&(this->mutex[i]));
    return -1;
  }

  // Get the function
  lua_getglobal(L[i], "destroy");

  // Push arguments
  Lunar<LuaCar>::push(this->L[i], self);

  // Call the function with 1 argument and 0 returns
  int r = lua_pcall(L[i], 1, 0, 0);

  // Check error
  if (r) {
    fprintf(stderr, "Controller error in destroy: %s\n", lua_tostring(this->L[i], -1));
    lua_pop(this->L[i], 1);
    lua_gc(this->L[i], LUA_GCCOLLECT, 0);
    lua_close(this->L[i]);
    this->L[i] = NULL;
    pthread_mutex_unlock(&(this->mutex[i]));
    return -1;
  }

  pthread_mutex_unlock(&(this->mutex[i]));

  return 0;
}

int LuaBinding::callControlInit(LuaInfrastructure *self)
{
  if (!this->controlL) return -1;

  // Get the function
  lua_getglobal(controlL, "init");

  // Push arguments
  Lunar<LuaInfrastructure>::push(this->controlL, self);

  // Call the function with 1 arguments and 0 return
  chdir(controlpath);
  int r = lua_pcall(controlL, 1, 0, 0);
  chdir(cwd);

  // Check error
  if (r) {
    fprintf(stderr, "Infrastructure controller error in init: %s\n", lua_tostring(this->controlL, -1));
    lua_pop(this->controlL, 1);
    lua_gc(this->controlL, LUA_GCCOLLECT, 0);
    lua_close(this->controlL);
    this->controlL = NULL;
    return -1;
  }

  return 0;
}

int LuaBinding::callControlUpdate(LuaInfrastructure *self, double t, double dt)
{
  if (!this->controlL) return -1;

  // Get the function
  lua_getglobal(controlL, "update");

  // Push arguments
  Lunar<LuaInfrastructure>::push(this->controlL, self);
  lua_pushnumber(this->controlL, t);
  lua_pushnumber(this->controlL, dt);

  // Call the function with 3 arguments and 0 return
  chdir(controlpath);
  int r = lua_pcall(controlL, 3, 0, 0);
  chdir(cwd);

  // Check error
  if (r) {
    fprintf(stderr, "Infrastructure controller error in update: %s\n", lua_tostring(this->controlL, -1));
    lua_pop(this->controlL, 1);
    lua_gc(this->controlL, LUA_GCCOLLECT, 0);
    lua_close(this->controlL);
    this->controlL = NULL;
    return -1;
  }

  return 0;
}

int LuaBinding::callControlDestroy(LuaInfrastructure *self)
{
  if (!this->controlL) return -1;

  // Get the function
  lua_getglobal(controlL, "destroy");

  // Push arguments
  Lunar<LuaInfrastructure>::push(this->controlL, self);

  // Call the function with 3 arguments and 0 return
  chdir(controlpath);
  int r = lua_pcall(controlL, 1, 0, 0);
  chdir(cwd);

  // Check error
  if (r) {
    fprintf(stderr, "Infrastructure controller error in destroy: %s\n", lua_tostring(this->controlL, -1));
    lua_pop(this->controlL, 1);
    lua_gc(this->controlL, LUA_GCCOLLECT, 0);
    lua_close(this->controlL);
    this->controlL = NULL;
    return -1;
  }

  return 0;
}

void LuaBinding::setOptions(gengetopt_args_info *options)
{
  int n = options->ncpu_arg;
  if (n == 0) n = 1;

  getInstance().options = options;
  getInstance().ninstances = n;
  getInstance().L = new lua_State *[n];
  getInstance().mutex = new pthread_mutex_t[n];
  for (int i = 0; i < n; i++) {
    getInstance().L[i] = NULL;
    pthread_mutex_init(&(getInstance().mutex[i]), NULL);
  }
}

LuaBinding &LuaBinding::getInstance()
{
  static LuaBinding instance;
  return instance;
}
