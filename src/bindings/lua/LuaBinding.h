#ifndef LUA_BINDING_H
#define LUA_BINDING_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "lunar.h"

#include "LuaCar.h"
#include "LuaLane.h"
#include "LuaRoadSensor.h"
#include "LuaRoadActuator.h"
#include "LuaInfrastructure.h"

#include "cmdline.h"

/**
 * @brief The lua binding class
 *
 * This class is reponsible to bind the LUA scripts with the
 * CarControl interace. It sets up the LUA stack and creates
 * the appropriate tables and accessible class for LUA.
 * This class is Singleton (the Simulator engine is responsible to
 * load the file).
 */
class LuaBinding {
 private:
  LuaBinding();
  LuaBinding(const LuaBinding &);
  LuaBinding & operator=(const LuaBinding &);
  ~LuaBinding();

 public:
  /**
   * Sets the options of LuaBidding given the command arguments.
   * This needs to be called before any load command and not called
   * afterwards!
   * @param options the command line arguments
   */
  static void setOptions(gengetopt_args_info *options);

  /**
   * Gets the singleton instance of LuaBinding.
   * @return The LuaBinding instance.
   */
  static LuaBinding &getInstance();

  /**
   * Sets the LUA file to be executed.
   * @param filename The path to the file.
   * @return 0 on success.
   */
  int loadFile(char *filename);

  /**
   * Sets the LUA Lane file to be executed.
   * @param filename The path to the file.
   * @return 0 on success.
   */
  int loadControlFile(char *filename);

  /**
   * Unloads the LUA script
   */
  void unload();

  /**
   * Unloads the LUA Lane script
   */
  void unloadControl();

  /**
   * Calls the initialization function
   * that should be provided in the LUA script (init())
   * @return 0 on success
   */
  int callInit(LuaCar *self);

  /**
   * Calls the destruction function
   * that should be provided in the LUA script (destroy())
   * @return 0 on success
   */
  int callDestroy(LuaCar *self);

  /**
   * Calls the think function
   * that should be provided in the LUA script (think())
   * @return 0 on success
   */
  int callThink(LuaCar *self, double dt, vector<struct neighbor_struct> &neighbors);

  /**
   * Calls the update function from the infrastructure controller.
   * This function should be provided in the LUA control script (update())
   * @return 0 on success
   */
  int callControlUpdate(LuaInfrastructure *self, double t, double dt);

  /**
   * Calls the destroy function from the infrastructure controller.
   * This function should be provided in the LUA control script (destroy())
   * @return 0 on success
   */
  int callControlDestroy(LuaInfrastructure *self);

  /**
   * Calls the init function from the infrastructure controller.
   * This function should be provided in the LUA control script (init())
   * @return 0 on success
   */
  int callControlInit(LuaInfrastructure *self);

 private:
  int ninstances;
  lua_State **L;    // Car controllers
  lua_State *controlL; // Lane controller
  pthread_mutex_t *mutex;
  char *path;
  char *controlpath;
  char cwd[1024];
  gengetopt_args_info *options;
};


#endif
