#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#include <agents/Car.h>
#include <engine/Simulator.h>
#include <utils/utils.h>
#include <utils/Log.h>

#ifdef GUI
#include <utils/Fl_Glv_Window.H>
#include <FL/Fl_File_Chooser.H>
#endif

#ifdef LUA
#include <bindings/lua/LuaBinding.h>
#endif

#ifdef MAC
#include <mach-o/dyld.h>
#endif

#include <cmdline.h>
#include "SimViewer.h"

/* Window properties */
#define INIT_WINDOW_WIDTH  700
#define INIT_WINDOW_HEIGHT 700
#define INIT_WINDOW_X      100
#define INIT_WINDOW_Y      100
#define WINDOW_TITLE       "Disim"

#define MAPS_PATH "maps/"
#define SCRIPTS_PATH "scripts/car/"
#define MODELS_PATH "src/display/models/"

/* Utilities */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MAX(x,y) (((x)>(y))?(x):(y))
#define MIN(x,y) (((x)<(y))?(x):(y))

SimViewer::SimViewer()
{
  /* Init flags */
  quit = false;
  lists_created = false;
  draw_grid = true;
  draw_realistic = true;
  draw_skybox = false;
  draw_shadows = false;
  pause = false;
  nodisplay = false;
  fast = false;
  rain = false;
  fog = false;
  start_time = 0;

  /* Light source direction for fake shadows */
  light_alpha = M_PI/4.0; // angle with the ground
  light_beta = M_PI/4.0;  // absolute angle around the 0,1,0 axis

  /* Center of attention */
  center_x = 0.0;
  center_y = 0.0;
  center_z = 0.0;

  /* Vehicle of attention */
  car_followed = 0;

  /* Simulation */
  simulation_mode = NORMAL;
  simulation_speedup = 1.0;
  old_simulation_speedup = 1.0;
  min_step = 0.064;
  offset_time = 0.0;
  current_simulation_time = 0.0;

  /* Info */
  display_interest_point = false;
  display_mouse_point = false;

#ifdef GUI
  /* Model */
  car_model = NULL;
  truck_model = NULL;

  /* Option windows */
  laneoptions = NULL;
#endif
}

SimViewer::~SimViewer()
{

}

int SimViewer::parseCmdLine(int argc, char *args[])
{
  /* Parse arguments */
  cmdline_parser(argc, args, &options);

  /* Init logging if needed */
  Log::setVerboseLevel(options.verbose_level_arg);
  Log::setConsoleOutput(true);
  if (options.log_given) {
    Log::setConsoleOutput(false);
    Log::setGenericLogFile(options.log_arg);
  }

  /* Set the step size */
  if (options.time_step_arg > 0.001)
    min_step = options.time_step_arg;

  /* Set the executable path */
  if (!options.exe_path_given) {
    char path[256];
#ifdef WINDOWS
    // To be tested and fixed!
    GetModuleFileName(NULL, path, 255);
#elif defined(MAC)
    uint32_t size = 256;
    if (_NSGetExecutablePath(path, &size) != 0) {
      printf("Cannot find executable name.\n");
    }
#else
    // Needs maybe fixing in MAC
    char tmp[32];
    sprintf(tmp, "/proc/%d/exe", getpid());
    int bytes = readlink(tmp, path, 256);
    if (bytes > 255) bytes = 255;
    if(bytes >= 0) path[bytes] = '\0';
#endif
    for (int i = strlen(path)-1; i >= 0; i--) {
      if (path[i] == '/' || path[i] == '\\') {
        path[i] = '\0';
        break;
      }
    }
    options.exe_path_arg = (char *)malloc(strlen(path)+1);
    strcpy(options.exe_path_arg, path);
  }
  printf("Executable directory: %s\n", options.exe_path_arg);

  /* Map */
  map = new Map(&options);

  /* Simulator initialization */
  simulator = new Simulator(&options, map);

  /* Set the weather */
  if (strcmp(options.weather_arg, "rain") == 0) {
    rain = true;
  } else if (strcmp(options.weather_arg, "fog") == 0) {
    fog = true;
  } else if (strcmp(options.weather_arg, "rain+fog") == 0) {
    rain = true;
    fog = true;
  }

  /* Set starting time of day */
  int h, m;
  sscanf(options.start_time_arg, "%d:%d", &h,  &m);
  start_time = h*60+m;

  return 0;
}

void SimViewer::ASCIItoUNICODE(unsigned char ch, unsigned char val[2])
{
  if ((ch < 192) && (ch != 168) && (ch != 184)) {
    val[0] = 0;
    val[1] = ch;
    return;
  }
  if (ch == 168) {
    val[0] = 208;
    val[1] = 129;
    return;
  }
  if (ch == 184) {
    val[0] = 209;
    val[1] = 145;
    return;
  }
  if (ch < 240)  {
    val[0] = 208;
    val[1] = ch-48;
    return;
  }
  if (ch < 249)  {
    val[0] = 209;
    val[1] = ch-112;
    return;
  }
  return;
}

void SimViewer::UTF8toASCII(char *str, char *newstr)
{
  unsigned int size = 0;
  unsigned char uni[2] = {0,0};

  while (str[size] != '\0') {
    ASCIItoUNICODE(str[size], uni);
    newstr[size] = uni[1];
    size++;
  }
  return;
}

#ifdef GUI
int SimViewer::initGUI(int cols, int rows)
{
  int fast_flag = FL_MENU_TOGGLE;
  int pause_flag = FL_MENU_TOGGLE;
  int rain_flag = FL_MENU_TOGGLE;
  int fog_flag = FL_MENU_TOGGLE;
  if (options.fast_given) {
    fast_flag |= FL_MENU_VALUE;
  } else if (options.pause_given) {
    pause_flag |= FL_MENU_VALUE;
  }
  if (rain) rain_flag |= FL_MENU_VALUE;
  if (fog) fog_flag |= FL_MENU_VALUE;

  /* The menu */
  Fl_Menu_Item menuitems[] =
    {
      {"&File", 0, 0, 0, FL_SUBMENU},
        {"&Open", 'o', (Fl_Callback *)SimViewer::onAction, (void*)APP_ACTION_OPEN},
        {"Reload", 0, (Fl_Callback *)SimViewer::onAction, (void*)APP_ACTION_RELOAD, FL_MENU_DIVIDER},
        {"E&xit", 'q', (Fl_Callback *)SimViewer::onExit},
        {0},
      {"&Action", 0, 0, 0, FL_SUBMENU},
        {"Pause", ' ', (Fl_Callback*)SimViewer::onAction, (void*)APP_ACTION_PAUSE, pause_flag},
        {"Fast", FL_CTRL + 'f', (Fl_Callback*)SimViewer::onAction, (void*)APP_ACTION_FAST, FL_MENU_TOGGLE},
        {"No display", 0, (Fl_Callback*)SimViewer::onAction, (void*)APP_ACTION_NODISPLAY, fast_flag},
        {"Slower", '-', (Fl_Callback*)SimViewer::onAction, (void*)APP_ACTION_SLOWER},
        {"Faster", '+', (Fl_Callback*)SimViewer::onAction, (void*)APP_ACTION_FASTER, FL_MENU_DIVIDER},
        {"Get info", 'i', (Fl_Callback*)SimViewer::onAction, (void*)APP_ACTION_INFO},
        {0},
      {"&View", 0, 0, 0, FL_SUBMENU},
        {"Follow car", 0, (Fl_Callback*)SimViewer::onView, (void*)APP_VIEW_FOLLOW, FL_MENU_TOGGLE},
        {" Previous car", ',', (Fl_Callback*)SimViewer::onView, (void*)APP_VIEW_PREV_CAR},
        {" Next car", '.', (Fl_Callback*)SimViewer::onView, (void*)APP_VIEW_NEXT_CAR, FL_MENU_DIVIDER},
        {"Show grid", 0, (Fl_Callback*)SimViewer::onView, (void*)APP_VIEW_GRID, FL_MENU_TOGGLE | FL_MENU_VALUE},
        {"Show Real World", 0, (Fl_Callback*)SimViewer::onView, (void*)APP_VIEW_REALISTIC, FL_MENU_TOGGLE | FL_MENU_VALUE},
        {"Show Skybox", 0, (Fl_Callback*)SimViewer::onView, (void*)APP_VIEW_SKYBOX, FL_MENU_TOGGLE},
        {"Draw Shadows", 0, (Fl_Callback*)SimViewer::onView, (void*)APP_VIEW_SHADOWS, FL_MENU_TOGGLE},
        {0},
      {"&Weather", 0, 0, 0, FL_SUBMENU},
        {"Rain", 0, (Fl_Callback*)SimViewer::onView, (void*)APP_WEATHER_RAIN, rain_flag},
        {"Fog", 0, (Fl_Callback*)SimViewer::onView, (void*)APP_WEATHER_FOG, fog_flag},
        {0},
#ifdef LUA
      {"Controller", 0, 0, 0, FL_SUBMENU},
        {"Load script", 0, (Fl_Callback*)SimViewer::onController, (void*)APP_CONTROLLER_LOAD},
        {"Reload", 0, (Fl_Callback*)SimViewer::onController, (void*)APP_CONTROLLER_RELOAD},
        {"Unload", 0, (Fl_Callback*)SimViewer::onController, (void*)APP_CONTROLLER_UNLOAD},
        {0},
#endif
      {0},
    };

  /* Create top-level window */
  this->mainwin = new Fl_Window(cols, rows, "Disim");
  this->mainwin->user_data(this);

  this->mainwin->begin();

  /* Crate the menu bar */
  this->menubar = new Fl_Menu_Bar(0, 0, cols, 30);
  this->menubar->user_data(this);
  this->menubar->copy(menuitems);

  /* Create world window */
  this->worldwin = new Fl_Glv_Window(0, 30, cols, rows - 30, this, (Fl_Callback*)onPredraw, (Fl_Callback*)onDraw);
  this->worldwin->set_3d_mouse_mask(FL_CTRL);

  this->mainwin->end();

  /* Make world window resizable */
  this->mainwin->resizable(this->worldwin);

  /* Exit when clicking the cross */
  this->mainwin->callback((Fl_Callback *)SimViewer::onExit);

  /* Initialize the realistic drawer */
  realistic_drawer = new RealisticDrawer(&options, map);

  /* Set the title */
  sprintf(this->wintitle, "Disim: %s", map->name);
  this->mainwin->label(this->wintitle);

  /* Set mode */
  if (options.fast_given) {
    worldwin->mute();
    simulation_mode = NODISPLAY;
    nodisplay = true;
  } else if (options.pause_given) {
    old_simulation_speedup = simulation_speedup;
    simulation_speedup = 0.0;
    pause = true;
  }

  /* This mutex locks when modifying graphical data */
  pthread_mutex_init(&(this->mutex), NULL);

  return 0;
}

int SimViewer::finiGUI()
{
  pthread_mutex_lock(&(this->mutex));

  assert(this->mainwin);
  delete this->mainwin;
  this->mainwin = NULL;

  pthread_mutex_unlock(&(this->mutex));

  assert(realistic_drawer);
  delete realistic_drawer;
  realistic_drawer = NULL;

  if (this->car_model)
    delete this->car_model;
  this->car_model = NULL;

  if (this->truck_model)
    delete this->truck_model;
  this->truck_model = NULL;

  if (this->laneoptions)
    delete this->laneoptions;
  this->laneoptions = NULL;

  pthread_mutex_destroy(&(this->mutex));

  return 0;
}
#endif

int SimViewer::init()
{
  this->init_time = _gettime();

  return 0;
}

int SimViewer::fini()
{
  assert(simulator);
  delete simulator;
  simulator = NULL;

  assert(map);
  delete map;
  map = NULL;

  return 0;
}

#ifdef GUI
void SimViewer::onAction(Fl_Widget *w, int option)
{
  SimViewer *self;
  double current_time = _gettime();

  self = (SimViewer *)w->user_data();

  if (option == APP_ACTION_OPEN) {
    Fl_File_Chooser *chooser = new Fl_File_Chooser("", "", Fl_File_Chooser::SINGLE, "");
    char path[256];
    snprintf(path, 256, "%s/%s", self->options.exe_path_arg, MAPS_PATH);
    chooser->directory(path);
    chooser->filter("*.map");
    chooser->show();
    while(chooser->shown()) {
        Fl::wait();
    }
    if (chooser->value()) {
      printf("Open file: %s\n", chooser->value());
      /* We need to reset the whole pipeline */
      free(self->options.map_arg);
      self->options.map_arg = (char*)malloc(strlen(chooser->value()) + 1);
      strcpy(self->options.map_arg, chooser->value());

      self->resetCompleteStack();
    }
  } else if (option == APP_ACTION_RELOAD) {
    self->resetCompleteStack();
  } else if (option == APP_ACTION_PAUSE) {
    /* Modify simulation speedup to match a pause */
    if (!self->pause) {
      self->old_simulation_speedup = self->simulation_speedup;
      self->simulation_speedup = 0.0;
      self->pause = true;
    } else {
      self->simulation_speedup = self->old_simulation_speedup;
      self->pause = false;
    }
    self->offset_time = self->current_simulation_time;
    self->init_time = current_time;
  } else if (option == APP_ACTION_FAST) {
    /* Change simulation mode */
    if (!self->fast) {
      if (!self->nodisplay)
        self->simulation_mode = FAST;
      self->fast = true;
    } else {
      if (self->nodisplay) {
        self->worldwin->mute();
        self->simulation_mode = NODISPLAY;
      }
      else self->simulation_mode = NORMAL;
      self->fast = false;
    }
    self->offset_time = self->current_simulation_time;
    self->init_time = current_time;
  } else if (option == APP_ACTION_NODISPLAY) {
    if (!self->nodisplay) {
      self->worldwin->mute();
      self->simulation_mode = NODISPLAY;
      self->nodisplay = true;
    } else {
      self->worldwin->unmute();
      if (self->fast) self->simulation_mode = FAST;
      else self->simulation_mode = NORMAL;
      self->nodisplay = false;
    }
    self->offset_time = self->current_simulation_time;
    self->init_time = current_time;
  } else if (option == APP_ACTION_SLOWER) {
    if (self->simulation_mode == NORMAL) {
      self->simulation_speedup = MAX(0.1, self->simulation_speedup - 0.1);
      self->offset_time = self->current_simulation_time;
      self->init_time = current_time;
    }
  } else if (option == APP_ACTION_FASTER) {
    if (self->simulation_mode == NORMAL) {
      self->simulation_speedup = MIN(10.0, self->simulation_speedup + 0.1);
      self->offset_time = self->current_simulation_time;
      self->init_time = current_time;
    }
  } else if (option == APP_ACTION_INFO) {
    if (!self->display_mouse_point) return;

    /* Find the closest lane */
    Lane *closest = NULL;
    double min_dist = 100.0;
    for (unsigned int i = 0; i < self->map->segments.size(); i++) {
      for (unsigned int j = 0; j < self->map->segments[i]->lanes.size(); j++) {
        Lane *l = self->map->segments[i]->lanes[j];
        double dist = (l->x_start-self->mouse_x)*(l->x_start-self->mouse_x) + (l->y_start-self->mouse_z)*(l->y_start-self->mouse_z);
        if (dist < min_dist) {
          min_dist = dist;
          closest = l;
        }
      }
    }

    if (!self->laneoptions) {
      self->laneoptions = new LaneOptions(closest);
    } else {
      self->laneoptions->setLane(closest);
    }
  }

  return;
}

void SimViewer::onView(Fl_Widget *w, int option)
{
  static double fixed_y = 0.0;
  SimViewer *self;

  self = (SimViewer *)w->user_data();
  if (option == APP_VIEW_GRID) {
    self->draw_grid = !self->draw_grid;
  } else if (option == APP_VIEW_FOLLOW) {
    if (self->worldwin->is_camera_free()) {
      CarState *state = self->simulator->trackCarID(&self->car_followed);
      if (state) {
        self->worldwin->set_follow_coords(&(state->x), &fixed_y, &(state->y), &(state->yaw));
        self->worldwin->set_camera_follow();
        self->simulator->trackingOn();
      } else {
        Fl_Menu_Item *m = (Fl_Menu_Item *)((Fl_Menu_*)w)->mvalue();
        m->clear();
        self->simulator->trackingOff();
      }
    } else {
      self->worldwin->set_camera_free();
      self->simulator->trackingOff();
    }
  } else if (option == APP_VIEW_PREV_CAR) {
    self->car_followed = self->simulator->getPrevCarID(self->car_followed);
    self->simulator->trackCarID(&self->car_followed);
    if (self->worldwin->is_camera_follow()) {
      self->simulator->trackingOn();
    }
  } else if (option == APP_VIEW_NEXT_CAR) {
    self->car_followed = self->simulator->getNextCarID(self->car_followed);
    self->simulator->trackCarID(&self->car_followed);
    if (self->worldwin->is_camera_follow()) {
      self->simulator->trackingOn();
    }
  } else if (option == APP_VIEW_REALISTIC) {
    self->draw_realistic = !self->draw_realistic;
    if (self->fog && self->draw_realistic) {
      glEnable(GL_FOG);
      float FogCol[3]={ 0.8f, 0.8f, 0.8f}; // Define a nice light grey
      glFogfv(GL_FOG_COLOR,FogCol);     // Set the fog color
      glFogi(GL_FOG_MODE, GL_LINEAR);   // Note the 'i' after glFog - the GL_LINEAR constant is an integer.
      glFogf(GL_FOG_START, 100.f);
      glFogf(GL_FOG_END, 500.f);
      self->worldwin->setClearColor(0.8, 0.8, 0.8);
    } else {
      glDisable(GL_FOG);
      self->worldwin->setClearColor(0.0, 0.0, 0.0);
    }
  } else if (option == APP_VIEW_SKYBOX) {
    self->draw_skybox = !self->draw_skybox;
  } else if (option == APP_VIEW_SHADOWS) {
    self->draw_shadows = !self->draw_shadows;
  }  else if (option == APP_WEATHER_RAIN) {
    self->rain = !self->rain;
    if (self->rain && self->fog) {
      self->simulator->setWeather(RAIN | FOG);
      self->realistic_drawer->setWeather(RAIN | FOG);
    } else if (self->rain) {
      self->simulator->setWeather(RAIN);
      self->realistic_drawer->setWeather(RAIN);
    } else if (self->fog) {
      self->simulator->setWeather(FOG);
      self->realistic_drawer->setWeather(FOG);
    } else {
      self->simulator->setWeather(NICE);
      self->realistic_drawer->setWeather(NICE);
    }
    /* We need to recompute the road list */
    self->realistic_drawer->reset();
    self->realisticDL = self->realistic_drawer->predraw();
  } else if (option == APP_WEATHER_FOG) {
    self->fog = !self->fog;
    if (self->rain && self->fog) {
      self->simulator->setWeather(RAIN | FOG);
      self->realistic_drawer->setWeather(RAIN | FOG);
    } else if (self->rain) {
      self->simulator->setWeather(RAIN);
      self->realistic_drawer->setWeather(RAIN);
    } else if (self->fog) {
      self->simulator->setWeather(FOG);
      self->realistic_drawer->setWeather(FOG);
    } else {
      self->simulator->setWeather(NICE);
      self->realistic_drawer->setWeather(NICE);
    }
    if (self->fog && self->draw_realistic) {
      glEnable(GL_FOG);
      float FogCol[3]={ 0.8f, 0.8f, 0.8f}; // Define a nice light grey
      glFogfv(GL_FOG_COLOR,FogCol);     // Set the fog color
      glFogi(GL_FOG_MODE, GL_LINEAR);   // Note the 'i' after glFog - the GL_LINEAR constant is an integer.
      glFogf(GL_FOG_START, 100.f);
      glFogf(GL_FOG_END, 500.f);
      self->worldwin->setClearColor(0.8, 0.8, 0.8);
    } else {
      glDisable(GL_FOG);
      self->worldwin->setClearColor(0.0, 0.0, 0.0);
    }
    /* We need to recompute the road list */
    self->realistic_drawer->reset();
    self->realisticDL = self->realistic_drawer->predraw();
  }

  return;
}

#ifdef LUA
void SimViewer::onController(Fl_Widget *w, int option)
{
  SimViewer *self;

  self = (SimViewer *)w->user_data();
  if (option == APP_CONTROLLER_LOAD) {
    Fl_File_Chooser *chooser = new Fl_File_Chooser("", "", Fl_File_Chooser::SINGLE, "");
    char path[256];
    snprintf(path, 256, "%s/%s", self->options.exe_path_arg, SCRIPTS_PATH);
    chooser->directory(path);
    chooser->filter("*.lua");
    chooser->show();
    while(chooser->shown()) {
      Fl::wait();
    }
    if (chooser->value()) {
      printf("Open script: %s\n", chooser->value());

      // Set the options
      self->options.lua_given = 1;
      free(self->options.lua_arg);
      self->options.lua_arg = (char*)malloc(strlen(chooser->value()) + 1);
      strcpy(self->options.lua_arg, chooser->value());

      // While we are loading the file we cannot have the cars continue normal execution
      LuaBinding::getInstance().loadFile(self->options.lua_arg);
      // We have to call init on all cars
      for (int i = 0; i < self->simulator->getCarsCount(); i++) {
        Car *c = self->simulator->getCar(i);
        LuaBinding::getInstance().callInit(c->getControl()->getLuaCar());
      }
      // There are also special waiting cars
      for (unsigned int i = 0; i < self->map->entries.size(); i++) {
        Lane *l = self->map->entries[i];
        if (l->new_car) {
          LuaBinding::getInstance().callInit(l->new_car->getControl()->getLuaCar());
        }
      }
    }
  } else if (option == APP_CONTROLLER_RELOAD) {
    if (self->options.lua_given) {
      LuaBinding::getInstance().loadFile(self->options.lua_arg);
      // We have to call init on all cars
      for (int i = 0; i < self->simulator->getCarsCount(); i++) {
        Car *c = self->simulator->getCar(i);
        LuaBinding::getInstance().callInit(c->getControl()->getLuaCar());
      }
      // There are also special waiting cars
      for (unsigned int i = 0; i < self->map->entries.size(); i++) {
        Lane *l = self->map->entries[i];
        if (l->new_car) {
          LuaBinding::getInstance().callInit(l->new_car->getControl()->getLuaCar());
        }
      }
    }
  } else if (option == APP_CONTROLLER_UNLOAD) {
    self->options.lua_given = 0;
    LuaBinding::getInstance().unload();
  }
}
#endif

void SimViewer::onExit(Fl_Widget *w, int option)
{
  SimViewer *self;

  self = (SimViewer *)w->user_data();
  self->quit = true;

  return;
}

void SimViewer::onPredraw(Fl_Glv_Window *win, SimViewer *self)
{
  if (self->draw_realistic && self->draw_skybox) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    self->realistic_drawer->drawSkybox();
    glClear(GL_DEPTH_BUFFER_BIT);
  }
}

void SimViewer::onDraw(Fl_Glv_Window *win, SimViewer *self)
{
  if (!self->lists_created || self->worldwin->has_changed_context()) {
    if (self->lists_created) {
      self->realistic_drawer->reset();
      if (self->car_model)
        delete self->car_model;
      if (self->truck_model)
        delete self->truck_model;
      if (self->carDL != 0)
        glDeleteLists(self->carDL, 1);
      if (self->truckDL != 0)
        glDeleteLists(self->truckDL, 1);
    }

    self->realisticDL = self->realistic_drawer->predraw();

    self->car_model = new Model_3DS();
    char path[256];
    snprintf(path, 256, "%s/%svan.3ds", self->options.exe_path_arg, MODELS_PATH);
    self->car_model->Load(path);
    self->carDL = glGenLists(1);
    glNewList(self->carDL, GL_COMPILE);
    self->car_model->Draw();
    glEndList();

    self->truck_model = new Model_3DS();
    snprintf(path, 256, "%s/%struck.3ds", self->options.exe_path_arg, MODELS_PATH);
    self->truck_model->Load(path);
    self->truckDL = glGenLists(1);
    glNewList(self->truckDL, GL_COMPILE);
    self->truck_model->Draw();
    glEndList();

    if (self->fog && self->draw_realistic) {
      glEnable(GL_FOG);
      float FogCol[3]={ 0.8f, 0.8f, 0.8f}; // Define a nice light grey
      glFogfv(GL_FOG_COLOR,FogCol);     // Set the fog color
      glFogi(GL_FOG_MODE, GL_LINEAR);   // Note the 'i' after glFog - the GL_LINEAR constant is an integer.
      glFogf(GL_FOG_START, 100.f);
      glFogf(GL_FOG_END, 500.f);
      self->worldwin->setClearColor(0.8, 0.8, 0.8);
    } else {
      glDisable(GL_FOG);
      self->worldwin->setClearColor(0.0, 0.0, 0.0);
    }

    if (!self->lists_created) {
      self->lists_created = true;
    }
  }

  if (self->draw_realistic && (self->draw_shadows || self->rain)) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  if (self->rain && self->draw_realistic) {
    glPushMatrix();
    glScalef(1.0, -1.0, 1.0);
    bool s = self->draw_shadows;
    self->draw_shadows = false;
    self->drawVehicles();
    self->draw_shadows = s;
    glPopMatrix();
  }
  self->drawRoad();
  self->drawVehicles();

  if (self->draw_realistic && (self->draw_shadows || self->rain)) {
    glDisable(GL_BLEND);
  }

  self->drawGrid();
  self->drawInfo();
  self->drawMouseClick();
  self->drawInfoPoint();

  self->realistic_drawer->draw();
}

#define GRID_SPACING 10.0
#define GRID_RESOLUTION 2.0
#define GRID_OFFSET 0.2

void SimViewer::drawGrid()
{
  if (!this->draw_grid) return;

  double vx, vy, vz;
  this->worldwin->get_viewer_coords(&vx, &vy, &vz);

  GLfloat x, z;
  GLfloat xavg = (double)(10*((int)(vx/10.0)));
  GLfloat xmin = xavg - 250.0;
  GLfloat xmax = xavg + 250.0;
  GLfloat zavg = (double)(10*((int)(vz/10.0)));
  GLfloat zmin = zavg - 250.0;
  GLfloat zmax = zavg + 250.0;

  if (this->draw_realistic) {
    glColor3f(0.2, 0.2, 0.2);
  } else {
    glColor3f(0.3, 0.3, 0.3);
  }

  glBegin(GL_LINES);
  glLineWidth(1.0);
  for (x = xmin; x <= xmax; x += GRID_SPACING) {
    if (this->draw_realistic) {
      for (z = zmin; z < zmax; z += GRID_RESOLUTION) {
        glVertex3f(x, realistic_drawer->getTerrainHeight(x,z)+GRID_OFFSET, z);
        glVertex3f(x, realistic_drawer->getTerrainHeight(x,z+GRID_RESOLUTION)+GRID_OFFSET, z+GRID_RESOLUTION);
      }
    } else {
      glVertex3f(x, 0.0, zmin);
      glVertex3f(x, 0.0, zmax);
    }
  }
  for (z = zmin; z <= zmax; z += GRID_SPACING) {
    if (this->draw_realistic) {
      for (x = xmin; x < xmax; x += GRID_RESOLUTION) {
        glVertex3f(x, realistic_drawer->getTerrainHeight(x,z)+GRID_OFFSET, z);
        glVertex3f(x+GRID_RESOLUTION, realistic_drawer->getTerrainHeight(x+GRID_RESOLUTION,z)+GRID_OFFSET, z);
      }
    } else {
      glVertex3f(xmin, 0.0, z);
      glVertex3f(xmax, 0.0, z);
    }
  }

  glEnd();
}

void SimViewer::drawRoad()
{
  /* Draw road */
  if (this->draw_realistic)
    glCallList(realisticDL);
  return;
}

void SimViewer::drawInfo()
{
  char str[40];

  /* Draw info text */
  setOrthographicProjection();
  glPushMatrix();
  glLoadIdentity();
  glColor3f(1.0, 1.0, 1.0);

  int m = (int)(current_simulation_time/60.0) + start_time;
  int h = m/60;
  m = m - h*60;
  h %= 24;
  snprintf(str, 40, "Time elapsed: %.2f (%02d:%02d)", current_simulation_time, h, m);
  renderBitmapString(5, 20, 0, GLUT_BITMAP_HELVETICA_18, str);
  if (this->worldwin->is_camera_follow()) {
    snprintf(str, 30, "Following car: %d", car_followed);
    renderBitmapString(this->worldwin->w()-200, 20, 0, GLUT_BITMAP_HELVETICA_18, str);
  }
  if (simulation_mode == NORMAL) {
    snprintf(str, 30, "Speed: %.2f", simulation_speedup);
    renderBitmapString(5, this->worldwin->h()-20, 0, GLUT_BITMAP_HELVETICA_12, str);
  } else if (simulation_mode == FAST) {
    snprintf(str, 30, "FAST");
    renderBitmapString(5, this->worldwin->h()-20, 0, GLUT_BITMAP_HELVETICA_12, str);
  }
  snprintf(str, 30, "%.2f x", (current_simulation_time-offset_time)/(_gettime() - init_time));
  renderBitmapString(this->worldwin->w()-50, this->worldwin->h()-20, 0, GLUT_BITMAP_HELVETICA_12, str);

  glPopMatrix();
  resetPerspectiveProjection();
}

void SimViewer::drawVehicles()
{
  /* Draw vehicles */
  for (int i = 0; i < simulator->getCarsCount(); i++) {
    drawVehicle(simulator->getCar(i));
  }
}

void SimViewer::drawVehicle(Car *car)
{
  static vector<neighbor_t> neighbors;

  GLfloat vehicle_x;
  GLfloat vehicle_y;
  GLfloat vehicle_z;
  double x, y, z;
  GLfloat vehicle_yaw;
  double REAR, FRONT, SIDE, TOP, TIRE_RADIUS = 0.3, AXLE_DIST;
  double steerAngle;

  if (this->worldwin->is_camera_follow() && simulator->getCarFromID(car_followed) == car) {
    // If we are following this car, then draw the neighbors
    neighbors.clear();
    simulator->getNeighbors(car, &neighbors);
    glColor3f(1.0, 0.0, 1.0);
    glBegin(GL_LINES);
    glLineWidth(3);
    for (unsigned int i = 0; i < neighbors.size(); i++) {
      Car *n = neighbors[i].car;
      if (n) {
        glVertex3f(car->getX(), 0.01, car->getY());
        glVertex3f(n->getX(), 0.01, n->getY());
      }
    }
    glEnd();
  }

  car->getCarGeometry(&FRONT, &REAR, &SIDE, &TOP);
  vehicle_x = car->getX();
  vehicle_y = 0.0;
  vehicle_z = car->getY();
  vehicle_yaw = car->getYaw();
  steerAngle = car->getSteeringAngle();

  glMatrixMode(GL_MODELVIEW);

  glPushMatrix();
  glTranslatef(vehicle_x, vehicle_y, vehicle_z);
  glRotatef(-(vehicle_yaw/M_PI)*180.0, 0.0, 1.0, 0.0);

  this->worldwin->get_viewer_coords(&x, &y, &z);
  double dist = sqrt((vehicle_x - x)*(vehicle_x - x)+(vehicle_z - z)*(vehicle_z - z));

  if (draw_realistic && dist < 500.0) {
    glRotatef(90.0, 0.0, 1.0, 0.0);
    if (car->getID() % 7 == 0) {
      glColor3f(1.0, 0.0, 0.0);
    } else if (car->getID() % 7 == 1) {
      glColor3f(0.0, 1.0, 0.0);
    } else if (car->getID() % 7 == 2) {
      glColor3f(0.0, 0.0, 1.0);
    } else if (car->getID() % 7 == 3) {
      glColor3f(1.0, 1.0, 0.0);
    } else if (car->getID() % 7 == 4) {
      glColor3f(1.0, 0.0, 1.0);
    } else if (car->getID() % 7 == 5) {
      glColor3f(0.0, 1.0, 1.0);
    } else {
      glColor3f(1.0, 1.0, 1.0);
    }
    if (car->getType() == CAR) {
      glCallList(carDL);
    } else
      glCallList(truckDL);

    if (draw_shadows && light_alpha > M_PI/20.0 && light_alpha < M_PI/2.0) {
      glColor4f(0.0,0.0,0.0,0.5);
      glBegin(GL_POLYGON);

      // Ported shadow intersection of line with ground
      double alpha = light_alpha;
      double beta = light_beta + vehicle_yaw;
      while (beta >= M_PI) beta -= 2.0*M_PI;
      while (beta <= -M_PI) beta += 2.0*M_PI;

      double x[3];
      double y[3];
      double z[3];

      // Draw under the car and select point to intersect
      if (beta >= 0.0 && beta <= M_PI/2.0) {
        glVertex3f(SIDE, 0.02, -REAR);
        glVertex3f(SIDE, 0.02, FRONT);
        glVertex3f(-SIDE, 0.02, FRONT);

        x[0] = FRONT; y[0] = -SIDE; z[0] = TOP;
        x[1] = -REAR; y[1] = -SIDE; z[1] = TOP;
        x[2] = -REAR; y[2] = SIDE; z[2] = TOP;
      } else if (beta >= -M_PI && beta <= -M_PI/2.0) {
        glVertex3f(SIDE, 0.02, -REAR);
        glVertex3f(-SIDE, 0.02, -REAR);
        glVertex3f(-SIDE, 0.02, FRONT);

        x[0] = FRONT; y[0] = -SIDE; z[0] = TOP;
        x[1] = FRONT; y[1] = SIDE; z[1] = TOP;
        x[2] = -REAR; y[2] = SIDE; z[2] = TOP;
      } else if (beta > -M_PI/2.0 && beta < 0.0) {
        glVertex3f(-SIDE, 0.02, -REAR);
        glVertex3f(-SIDE, 0.02, FRONT);
        glVertex3f(SIDE, 0.02, FRONT);

        x[0] = FRONT; y[0] = SIDE; z[0] = TOP;
        x[1] = -REAR; y[1] = SIDE; z[1] = TOP;
        x[2] = -REAR; y[2] = -SIDE; z[2] = TOP;
      } else {
        glVertex3f(-SIDE, 0.02, -REAR);
        glVertex3f(SIDE, 0.02, -REAR);
        glVertex3f(SIDE, 0.02, FRONT);

        x[0] = FRONT; y[0] = SIDE; z[0] = TOP;
        x[1] = FRONT; y[1] = -SIDE; z[1] = TOP;
        x[2] = -REAR; y[2] = -SIDE; z[2] = TOP;
      }

      // For each point intersection the light line with the ground
      for (int i = 0; i < 3; i++) {
        double t = -z[i]/sin(alpha);
        double xi = cos(alpha)*cos(beta)*t + x[i];
        double yi = cos(alpha)*sin(beta)*t + y[i];
        glVertex3f(yi, 0.02, xi);
      }

      glEnd();
    }
  } else {

    if (this->worldwin->is_camera_follow() && simulator->getCarFromID(car_followed) == car) {
      glLineWidth(1);
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);

      // Draw frame axes
      glPushMatrix();
      glTranslatef(0, TIRE_RADIUS, 0);
      glBegin(GL_LINES);
      glColor3f(1, 0, 0);
      glVertex3f(0, 0, 0);
      glVertex3f(1, 0, 0);
      glColor3f(0, 1, 0);
      glVertex3f(0, 0, 0);
      glVertex3f(0, 1, 0);
      glColor3f(0, 0, 1);
      glVertex3f(0, 0, 0);
      glVertex3f(0, 0, 1);
      glEnd();
      glPopMatrix();

      // Draw outer vehicle dimensions
      glPushMatrix();
      glTranslatef(+((FRONT+REAR)/2.0-REAR), (TOP + TIRE_RADIUS)/2.0, 0.0);
      glScalef(FRONT+REAR, TOP - TIRE_RADIUS, 2.0*SIDE);
      glColor3f(0, 0.70, 0);
      glutWireCube(1.0);
      glPopMatrix();

      // Rear left wheel
      glPushMatrix();
      glTranslatef(0, TIRE_RADIUS, -SIDE+0.3);
      glutWireTorus(0.1, TIRE_RADIUS - 0.15, 16, 64);
      glPopMatrix();

      // Rear right wheel
      glPushMatrix();
      glTranslatef(0, TIRE_RADIUS, SIDE-0.3);
      glutWireTorus(0.1, TIRE_RADIUS - 0.15, 16, 64);
      glPopMatrix();

      // Front left wheel
      AXLE_DIST = FRONT - REAR;
      glPushMatrix();
      glTranslatef(AXLE_DIST, TIRE_RADIUS, -SIDE+0.3);
      glRotatef(-steerAngle * 180/M_PI, 0, 1, 0);
      glutWireTorus(0.1, TIRE_RADIUS - 0.15, 8, 32);
      glPopMatrix();

      // Front right wheel
      glPushMatrix();
      glTranslatef(AXLE_DIST, TIRE_RADIUS, SIDE-0.3);
      glRotatef(-steerAngle * 180/M_PI, 0, 1, 0);
      glutWireTorus(0.1, TIRE_RADIUS - 0.15, 8, 32);
      glPopMatrix();

    } else {
      // Draw outer vehicle dimensions
      glPushMatrix();
      glTranslatef(+((FRONT+REAR)/2.0-REAR), TOP/2.0, 0.0);
      glScalef(FRONT+REAR, TOP, 2.0*SIDE);
      glColor4f(0.7, 0.7, 0.7, 1.0);
      glutWireCube(1.0);
      glPopMatrix();
    }
  }

  glLineWidth(1);
  glPopMatrix();
}

void SimViewer::drawMouseClick()
{
  static int first_time = 0;

  double x, y, z;
  if (this->worldwin->get_3d_mouse_coords(&x, &y, &z)) {
    display_mouse_point = false;
    if (laneoptions) laneoptions->hide();
    first_time = 0;
    return;
  }

  if (this->draw_realistic) {
    y = realistic_drawer->getTerrainHeight(x,z) + 0.5;
  }

  /* The mouse point was already displayed at a different location */
  if (x != mouse_x || y != mouse_y || z != mouse_z) {
    this->display_mouse_point = false;
    first_time = 0;
  }

  this->display_mouse_point = true;
  this->mouse_x = x;
  this->mouse_y = y;
  this->mouse_z = z;

  /* Display info box as well */
  if (first_time == 0) {
    first_time = 1;
    /* Find the closest lane */
    Lane *closest = NULL;
    double min_dist = 100.0;
    for (unsigned int i = 0; i < this->map->segments.size(); i++) {
      for (unsigned int j = 0; j < this->map->segments[i]->lanes.size(); j++) {
        Lane *l = this->map->segments[i]->lanes[j];
        double dist = (l->x_start-this->mouse_x)*(l->x_start-this->mouse_x) + (l->y_start-this->mouse_z)*(l->y_start-this->mouse_z);
        if (dist < min_dist) {
          min_dist = dist;
          closest = l;
        }
      }
    }

    if (closest) {
      this->mouse_x = closest->x_start;
      this->mouse_y = 0.0;
      this->mouse_z = closest->y_start;
      if (!this->laneoptions) {
        this->laneoptions = new LaneOptions(closest);
      } else {
        this->laneoptions->setLane(closest);
      }
    } else {
      this->display_mouse_point = false;
      first_time = 0;
    }
  }

  if (this->display_mouse_point) {
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    for (double angle = 0.0; angle < 2.0*M_PI; angle += M_PI/12.0) {
      glVertex3f(this->mouse_x + cos(angle)*1.0,
                 this->mouse_y,
                 this->mouse_z + sin(angle)*1.0);
    }
    glEnd();
  }

  return;
}

void SimViewer::drawInfoPoint()
{
  if (!this->display_interest_point) return;
  if (!this->display_mouse_point) return;

  glColor3f(1.0, 0.3, 0.0);
  glBegin(GL_LINE_LOOP);
  for (double angle = 0.0; angle < 2.0*M_PI; angle += M_PI/12.0) {
    glVertex3f(interest_point_x + cos(angle)*1.0,
               interest_point_y,
               interest_point_z + sin(angle)*1.0);
  }
  glEnd();
  glBegin(GL_LINE_STRIP);
  glVertex3f(mouse_x, mouse_y, mouse_z);
  glVertex3f(interest_point_x, interest_point_y, interest_point_z);
  glEnd();
}

void SimViewer::renderBitmapString(float x, float y, float z, void *font, char *string)
{
  char *c;
  glRasterPos3f(x, y, z);
  for (c=string; *c != '\0'; c++) {
    glutBitmapCharacter(font, *c);
  }
}

void SimViewer::setOrthographicProjection()
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, this->worldwin->w(), 0, this->worldwin->h());
  glScalef(1, -1, 1);
  glTranslatef(0, -this->worldwin->h(), 0);
  glMatrixMode(GL_MODELVIEW);
}

void SimViewer::resetPerspectiveProjection()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
}

void SimViewer::onIdle(SimViewer *self)
{
  static bool first = true;
  static double previous_time;

  double current_time = _gettime();
  double dt;

  if (first) {
    first = false;
    previous_time = self->init_time;
  }

  /* Update simulation time */
  if (self->simulation_mode != NORMAL && !self->pause) {
    dt = self->min_step;
    self->current_simulation_time += self->min_step;
  } else {
    dt = (current_time - previous_time)*self->simulation_speedup;
    /* Fail-safe mechanism */
    if (dt > self->min_step) {
      /* Step time was too big */
      dt = self->min_step;
      self->current_simulation_time += dt;
      self->offset_time = self->current_simulation_time;
      self->init_time = current_time;
    } else {
      /* Normal update */
      self->current_simulation_time = ((current_time - self->init_time)*self->simulation_speedup +
                                       self->offset_time);
    }
  }

  /* Simulates the cars */
  self->simulator->step(dt);

  /* Update previous time */
  previous_time = current_time;

  /* Shawdows update */
  if (self->draw_shadows) {
    int m = (int)(self->current_simulation_time/60.0) + self->start_time;
    int h = m/60;
    m = m - h*60;
    h %= 24;

    double w = 21.0-6.0;
    double f = (21.0+6.0)/2.0;
    double r = (((double)h+(double)m/60.0)-f)/w*2.0;
    if (r < -1.0 || r > 1.0) self->light_alpha = -M_PI/2.0;
    else {
      self->light_alpha = (1.0-r*r)*M_PI/3.0;
      self->light_beta = M_PI/4.0*r;
    }
  }

  /* Redraw */
  pthread_mutex_lock(&(self->mutex));
  self->realistic_drawer->update(dt);
  self->worldwin->redraw();
  pthread_mutex_unlock(&(self->mutex));

  /* Do we need to exit */
  if (self->options.duration_given && self->options.duration_arg > 0) {
    if (self->current_simulation_time > (double)self->options.duration_arg) {
      self->quit = true;
    }
  }

  /* Refresh the lane options */
  if (self->laneoptions) self->laneoptions->update();
}

void SimViewer::resetCompleteStack(void)
{
  pthread_mutex_lock(&(this->mutex));

  this->worldwin->set_camera(0.0, 0.0, 0.0, 300.0, M_PI/4.0, 0.0);

  /* Kill everything */
  delete this->simulator;
  delete this->map;
  delete this->laneoptions; this->laneoptions = NULL;
  delete this->realistic_drawer;

  /* Rebuild */
  this->map = new Map(&this->options);
  this->simulator = new Simulator(&this->options, this->map);
  if (this->rain && this->fog) this->simulator->setWeather(RAIN | FOG);
  else if (this->rain) this->simulator->setWeather(RAIN);
  else if (this->fog) this->simulator->setWeather(FOG);
  else this->simulator->setWeather(NICE);
  this->realistic_drawer = new RealisticDrawer(&this->options, this->map);
  if (this->rain && this->fog) this->realistic_drawer->setWeather(RAIN | FOG);
  else if (this->rain) this->realistic_drawer->setWeather(RAIN);
  else if (this->fog) this->realistic_drawer->setWeather(FOG);
  else this->realistic_drawer->setWeather(NICE);

  /* Set the title */
  sprintf(this->wintitle, "Disim: %s", this->map->name);
  this->mainwin->label(this->wintitle);

  this->lists_created = false;
  this->offset_time = 0.0;
  this->current_simulation_time = 0.0;
  this->init_time = _gettime();

  pthread_mutex_unlock(&(this->mutex));
}
#endif

int main(int argc, char *argv[])
{
  SimViewer *sim;
  sim = new SimViewer();

  /* Parse cmd line */
  if (sim->parseCmdLine(argc, argv) != 0)
    return -1;

#ifdef GUI
  if (sim->options.nogui_given) {
#endif
    /* Run the simulation loop as fast as possible */
    double dt = sim->min_step;

    while (1) {
      sim->current_simulation_time += dt;
      sim->simulator->step(dt);

      if (sim->options.duration_given && sim->options.duration_arg > 0) {
        if (sim->current_simulation_time > (double)sim->options.duration_arg) {
          break;
        }
      }

      usleep(100);
    }

#ifdef GUI
  } else {
    /* Init GLUT */
    glutInit(&argc, argv);

    /* Initialize gui */
    if (sim->initGUI(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT + 30) != 0)
      return -1;

    /* Initialize algorithms */
    if (sim->init() != 0)
      return -1;

    /* Idle callback */
    Fl::add_idle((void (*) (void*))SimViewer::onIdle, sim);

    /* Run */
    sim->mainwin->show();
    while (!sim->quit)
      Fl::wait();

    printf("Exiting...\n");

    /* Clean up */
    sim->finiGUI();
  }
#endif

  /* Clean up */
  sim->fini();
  delete sim;

  cmdline_parser_free(&(sim->options));
  printf("Exited cleanly\n");

  return 0;
}
