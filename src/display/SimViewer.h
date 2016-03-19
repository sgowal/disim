#ifndef _SIMVIEWER_H
#define _SIMVIEWER_H

#ifdef GUI
#ifdef MAC
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <utils/Fl_Glv_Window.H>
#include <pthread.h>
#include "RealisticDrawer.h"
#include "Model_3DS.h"
#include "LaneOptions.h"
#endif

#include <engine/Simulator.h>
#include <map/Map.h>

typedef enum {NORMAL, FAST, NODISPLAY} SIMULATION_MODE;

enum
{
  APP_ACTION_OPEN = 0x1000,
  APP_ACTION_RELOAD,
  APP_ACTION_PAUSE,
  APP_ACTION_FAST,
  APP_ACTION_NODISPLAY,
  APP_ACTION_SLOWER,
  APP_ACTION_FASTER,
  APP_ACTION_INFO,
  APP_VIEW_FOLLOW,
  APP_VIEW_PREV_CAR,
  APP_VIEW_NEXT_CAR,
  APP_VIEW_GRID,
  APP_VIEW_REALISTIC,
  APP_VIEW_SKYBOX,
  APP_VIEW_SHADOWS,
  APP_WEATHER_RAIN,
  APP_WEATHER_FOG,
  APP_CONTROLLER_LOAD,
  APP_CONTROLLER_RELOAD,
  APP_CONTROLLER_UNLOAD,
};

class SimViewer {
 public:
  SimViewer();
  ~SimViewer();

  int parseCmdLine(int argc, char *args[]);

#ifdef GUI
  int initGUI(int cols, int rows);
  int finiGUI();

  // Action callback
  static void onAction(Fl_Widget *w, int option);
  
  // View callback
  static void onView(Fl_Widget *w, int option);

#ifdef LUA
  // Controller callback
  static void onController(Fl_Widget *w, int option);
#endif

  // Exit callback
  static void onExit(Fl_Widget *w, int option);

  // Handle predraw callbacks
  static void onPredraw(Fl_Glv_Window *win, SimViewer *self);

  // Handle draw callbacks
  static void onDraw(Fl_Glv_Window *win, SimViewer *self);

  // Handle idle callbacks
  static void onIdle(SimViewer *self);
#endif

  int init();
  int fini();

 public:

#ifdef GUI
  void drawGrid();
  void drawRoad();
  void drawInfo();
  void drawVehicles();
  void drawVehicle(Car *car);
  void drawMouseClick();
  void drawInfoPoint();
  void renderBitmapString(float x, float y, float z, void *font, char *string);
  void setOrthographicProjection(void);
  void resetPerspectiveProjection(void);
  void resetCompleteStack(void);
#endif

 public:
  // cmdline options
  gengetopt_args_info options;

#ifdef GUI
  // Top-level window
  Fl_Window *mainwin;

  // Top menu bar
  Fl_Menu_Bar *menubar;
  
  // 3D window
  Fl_Glv_Window *worldwin;

  // Lane Options window
  LaneOptions *laneoptions;

  // Lists to predraw
  RealisticDrawer *realistic_drawer;
  Model_3DS *car_model;
  Model_3DS *truck_model;
  GLuint realisticDL;
  GLuint carDL;
  GLuint truckDL;
#endif
  
  // Center of initial attention
  double center_x;
  double center_y;
  double center_z;

  // Light source for fake shadows
  double light_alpha;
  double light_beta;

  // Vehicle of attention
  int car_followed;

  // Should we quit?
  bool quit;

  // Are lists already computed?
  bool lists_created;

  // Map
  Map *map;

  // Simulation specifics
  Simulator *simulator;
  SIMULATION_MODE simulation_mode;
  double simulation_speedup;
  double old_simulation_speedup;
  double min_step;
  double init_time;
  double offset_time;
  double current_simulation_time;

  // Display options
  bool draw_grid;
  bool draw_realistic;
  bool draw_skybox;
  bool draw_shadows;

  // Starting time of day (in minutes)
  int start_time;

  // Weather options
  bool rain;
  bool fog;

  // Action options
  bool pause;
  bool fast;
  bool nodisplay;

  // Displaying info
  bool display_interest_point;
  bool display_mouse_point;
  double mouse_x;
  double mouse_y;
  double mouse_z;
  double interest_point_x;
  double interest_point_y;
  double interest_point_z;

  // Window title
  char wintitle[256];

#ifdef GUI
  pthread_mutex_t mutex; // locks display
#endif

 private:
  static void ASCIItoUNICODE(unsigned char ch, unsigned char val[2]);
  static void UTF8toASCII(char *str, char *newstr);
};

#endif
