#ifndef REALISTIC_DRAWER_H
#define REALISTIC_DRAWER_H

#ifdef MAC
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cmdline.h>
#include <map/Map.h>
#include "TextureManager.h"

#define TERRAIN_RESOLUTION 256

class RealisticDrawer
{
 public:
  RealisticDrawer(gengetopt_args_info *options, Map *map);
  ~RealisticDrawer();
  void reset();
  int predraw();
  void drawSkybox();
  void draw();
  double getTerrainHeight(double x, double y);
  void setWeather(int w);
  void update(double dt);

 private:
  void drawRoad();
  void drawEnvironment();
  void renderBitmapString(float x, float y, float z, void *font, char *string);
  void drawText(float x, float y, float z, float angle, float size, float offset_x, float offset_y, float offset_z, const char *text);
  void drawText(float x, float y, float z, float angle, float size, const char *text);
  void computeTerrain();
  double random_uniform();

  // drawing list
  int roadList;
  
  // texture
  TextureManager *textureManager;
  bool texture_loaded;
  GLuint grass_texture;
  GLuint road_texture;
  GLuint skybox_texture[6];

  // map
  Map *map;

  // terrain
  double terrain_xmin;
  double terrain_xmax;
  double terrain_ymin;
  double terrain_ymax;
  double terrain[TERRAIN_RESOLUTION][TERRAIN_RESOLUTION]; // z in function of x,y indices

  // weather conditions
  int weather;

  // Save pointer on options
  gengetopt_args_info *options;
};

#endif
