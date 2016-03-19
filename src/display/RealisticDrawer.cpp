#include "RealisticDrawer.h"
#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <iomanip>

#define TEXTURE_PATH "src/display/textures/"
#define SKYBOX_NAME  "calm/"

#define GET_TERRAIN_X(i) (((double)(i))/((double)(TERRAIN_RESOLUTION-1))*(terrain_xmax-terrain_xmin)+terrain_xmin)
#define GET_TERRAIN_Y(i) (((double)(i))/((double)(TERRAIN_RESOLUTION-1))*(terrain_ymax-terrain_ymin)+terrain_ymin)

#define NICE 0
#define RAIN 1
#define FOG  2

RealisticDrawer::RealisticDrawer(gengetopt_args_info *options, Map *map)
{
  this->options = options;
  
  /* Load the textures */
  textureManager = new TextureManager();
  texture_loaded = false;

  this->map = map;
  this->roadList = 0;

  this->weather = NICE;
  if (strcmp(options->weather_arg, "rain") == 0) {
    setWeather(RAIN);
  } else if (strcmp(options->weather_arg, "fog") == 0) {
    setWeather(FOG);
  } else if (strcmp(options->weather_arg, "rain+fog") == 0) {
    setWeather(RAIN | FOG);
  }

  computeTerrain();
}

RealisticDrawer::~RealisticDrawer()
{
  if (this->roadList != 0)
    glDeleteLists(this->roadList, 1);

  if (grass_texture)
    glDeleteTextures(1, &grass_texture);
  if (road_texture)
    glDeleteTextures(1, &road_texture);
  if (skybox_texture[0])
    glDeleteTextures(1, &skybox_texture[0]);
  if (skybox_texture[1])
    glDeleteTextures(1, &skybox_texture[1]);
  if (skybox_texture[2])
    glDeleteTextures(1, &skybox_texture[2]);
  if (skybox_texture[3])
    glDeleteTextures(1, &skybox_texture[3]);
  if (skybox_texture[4])
    glDeleteTextures(1, &skybox_texture[4]);
  if (skybox_texture[5])
    glDeleteTextures(1, &skybox_texture[5]);

  delete textureManager;
}

int RealisticDrawer::predraw()
{
  if (this->roadList == 0)
    this->roadList = glGenLists(1);
  glNewList(this->roadList, GL_COMPILE);

  /* Loading textures */
  if (!texture_loaded) {
    char path[256];
    snprintf(path, 256, "%s/%sgrass.bmp", options->exe_path_arg, TEXTURE_PATH);
    if (textureManager->loadTexture(&grass_texture, path, BMP_IMAGE))
      grass_texture = 0;
    Log::getStream(5) << "Grass texture loaded: " << grass_texture << endl;
    snprintf(path, 256, "%s/%sroad.bmp", options->exe_path_arg, TEXTURE_PATH);
    if (textureManager->loadTexture(&road_texture, path, BMP_IMAGE))
      road_texture = 0;
    Log::getStream(5) << "Road texture loaded: " << road_texture << endl;

    snprintf(path, 256, "%s/%s%snorth.png", options->exe_path_arg, TEXTURE_PATH, SKYBOX_NAME);
    if (textureManager->loadTexture(&skybox_texture[0], path, PNG_IMAGE, false)) skybox_texture[0] = 0;
    snprintf(path, 256, "%s/%s%seast.png", options->exe_path_arg, TEXTURE_PATH, SKYBOX_NAME);
    if (textureManager->loadTexture(&skybox_texture[1], path, PNG_IMAGE, false)) skybox_texture[1] = 0;
    snprintf(path, 256, "%s/%s%ssouth.png", options->exe_path_arg, TEXTURE_PATH, SKYBOX_NAME);
    if (textureManager->loadTexture(&skybox_texture[2], path, PNG_IMAGE, false)) skybox_texture[2] = 0;
    snprintf(path, 256, "%s/%s%swest.png", options->exe_path_arg, TEXTURE_PATH, SKYBOX_NAME);
    if (textureManager->loadTexture(&skybox_texture[3], path, PNG_IMAGE, false)) skybox_texture[3] = 0;
    snprintf(path, 256, "%s/%s%sup.png", options->exe_path_arg, TEXTURE_PATH, SKYBOX_NAME);
    if (textureManager->loadTexture(&skybox_texture[4], path, PNG_IMAGE, false)) skybox_texture[4] = 0;
    snprintf(path, 256, "%s/%s%sdown.png", options->exe_path_arg, TEXTURE_PATH, SKYBOX_NAME);
    if (textureManager->loadTexture(&skybox_texture[5], path, PNG_IMAGE, false)) skybox_texture[5] = 0;
    Log::getStream(5) << "Skybox textures loaded: " << skybox_texture[0] << " " << skybox_texture[1] << " " << skybox_texture[2] << " " << skybox_texture[3] << " " << skybox_texture[4] << " " << skybox_texture[5] << endl;

    texture_loaded = true;
  }

  drawRoad();
  drawEnvironment();

  glEndList();
  return this->roadList;
}

void RealisticDrawer::reset()
{
  texture_loaded = false;

  if (this->roadList != 0)
    glDeleteLists(this->roadList, 1);

  if (grass_texture)
    glDeleteTextures(1, &grass_texture);
  if (road_texture)
    glDeleteTextures(1, &road_texture);
  if (skybox_texture[0])
    glDeleteTextures(1, &skybox_texture[0]);
  if (skybox_texture[1])
    glDeleteTextures(1, &skybox_texture[1]);
  if (skybox_texture[2])
    glDeleteTextures(1, &skybox_texture[2]);
  if (skybox_texture[3])
    glDeleteTextures(1, &skybox_texture[3]);
  if (skybox_texture[4])
    glDeleteTextures(1, &skybox_texture[4]);
  if (skybox_texture[5])
    glDeleteTextures(1, &skybox_texture[5]);
}

void RealisticDrawer::update(double dt)
{

}

double RealisticDrawer::getTerrainHeight(double x, double y)
{
  double ix = (x-terrain_xmin)/(terrain_xmax-terrain_xmin)*(double)(TERRAIN_RESOLUTION-1);
  double iy = (y-terrain_ymin)/(terrain_ymax-terrain_ymin)*(double)(TERRAIN_RESOLUTION-1);

  int i = (int)ix;
  int j = (int)iy;
  double fx = ix - (double)i;
  double fy = iy - (double)j;

  if (i > TERRAIN_RESOLUTION-1) i = TERRAIN_RESOLUTION-1;
  if (i < 0) i = 0;
  if (j > TERRAIN_RESOLUTION-1) j = TERRAIN_RESOLUTION-1;
  if (j < 0) j = 0;

  if (i == TERRAIN_RESOLUTION-1 || j == TERRAIN_RESOLUTION-1) 
    return terrain[i][j];

  if (fx + fy < 1.0)
    return terrain[i][j]*(1.0-fx-fy)+fx*terrain[i+1][j]+fy*terrain[i][j+1];

  fx = 1.0-fx;
  fy = 1.0-fy;
  return terrain[i+1][j+1]*(1.0-fx-fy)+fx*terrain[i][j+1]+fy*terrain[i+1][j];
}

#define MARK_LENGTH 2.0
#define MARK_INTER  3.0
#define MARK_WIDTH  0.2
#define INNER_MARK_OFFSET 0.3

#define NUM_LANES   3
#define LANE_WIDTH  3.5
#define ROAD_LENGTH 1000.0

#define SIDE_WIDTH 2.0

void RealisticDrawer::drawRoad()
{ 
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glMatrixMode(GL_MODELVIEW);

  /* Due to the z-buffer not being updated with transparent object
     we need to create a tunnel to avoid the non-foggy background
     to be visible */
  if ((this->weather & RAIN) && (this->weather & FOG)) {
    for (unsigned int k = 0; k < map->segments.size(); k++) {
      Segment *s = map->segments[k];
      if (s->lanes.empty()) continue;
      if (s->geometry == STRAIGHT) {
        glPushMatrix();
        glTranslatef(s->x, 0.0, s->y);
        glRotatef(-s->a*180.0/M_PI, 0.0, 1.0, 0.0);  

        glColor3f(0.1, 0.1, 0.1);
        glBegin(GL_QUADS);
        glVertex3f(0.0, -10.0, 0.0);
        glVertex3f(s->length, -10.0, 0.0);
        glVertex3f(s->length, -10.0, (float)s->lanes.size()*map->lane_width);
        glVertex3f(0.0, -10.0, (float)s->lanes.size()*map->lane_width);
        glVertex3f(0.0, -10.0, 0.0);
        glVertex3f(s->length, -10.0, 0.0);
        glVertex3f(s->length, 0.0, 0.0);
        glVertex3f(0.0, 0.0, 0.0);
        glVertex3f(s->length, -10.0, (float)s->lanes.size()*map->lane_width);
        glVertex3f(0.0, -10.0, (float)s->lanes.size()*map->lane_width);
        glVertex3f(0.0, 0.0, (float)s->lanes.size()*map->lane_width);
        glVertex3f(s->length, 0.0, (float)s->lanes.size()*map->lane_width);
        glEnd();
        glPopMatrix();
      } else {
        double xc;
        double yc;
        if (s->angle > 0.0) {
          xc = s->x + cos(s->a+M_PI/2.0)*s->radius;
          yc = s->y + sin(s->a+M_PI/2.0)*s->radius;
        } else {
          xc = s->x + cos(s->a-M_PI/2.0)*s->radius;
          yc = s->y + sin(s->a-M_PI/2.0)*s->radius;
        }
        glPushMatrix();
        glTranslatef(xc, 0.0, yc);
        if (s->angle > 0.0)
          glRotatef(90.0-s->a*180.0/M_PI, 0.0, 1.0, 0.0);
        else
          glRotatef(-90.0-s->a*180.0/M_PI, 0.0, 1.0, 0.0);
        
        float r = s->radius;
        if (s->angle < 0.0) {
          r += map->lane_width*(float)s->lanes.size();
        } else {
          r -= map->lane_width*(float)s->lanes.size();
        }
        
        float dangle_length = MARK_LENGTH/s->radius;
        float f = (s->angle < 0.0)?-1.0:1.0;
        
        glColor3f(0.1, 0.1, 0.1);
        glBegin(GL_TRIANGLE_STRIP);
        for (float a = 0.0; a < fabs(s->angle); a += dangle_length) {
          glVertex3f(cos(f*a)*(s->radius), -10.0, sin(f*a)*(s->radius));
          glVertex3f(cos(f*a)*r, -10.0, sin(f*a)*r);
        }
        glVertex3f(cos(s->angle)*(s->radius), -10.0, sin(s->angle)*(s->radius));
        glVertex3f(cos(s->angle)*r, -10.0, sin(s->angle)*r);
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        for (float a = 0.0; a < fabs(s->angle); a += dangle_length) {
          glVertex3f(cos(f*a)*(s->radius), -10.0, sin(f*a)*(s->radius));
          glVertex3f(cos(f*a)*(s->radius),  0.0, sin(f*a)*(s->radius));
        }
        glVertex3f(cos(s->angle)*(s->radius), -10.0, sin(s->angle)*(s->radius));
        glVertex3f(cos(s->angle)*(s->radius), 0.0, sin(s->angle)*(s->radius));
        glEnd();

        glBegin(GL_TRIANGLE_STRIP);
        for (float a = 0.0; a < fabs(s->angle); a += dangle_length) {
          glVertex3f(cos(f*a)*r, -10.0, sin(f*a)*r);
          glVertex3f(cos(f*a)*r, 0.0, sin(f*a)*r);
        }
        glVertex3f(cos(s->angle)*r, -10.0, sin(s->angle)*r);
        glVertex3f(cos(s->angle)*r, 0.0, sin(s->angle)*r);
        glEnd();
        glPopMatrix();
      }

      // For entry and exit
      glBegin(GL_QUADS);
      for (unsigned int j = 0; j < s->lanes.size(); j++) {
        Lane *l = s->lanes[j];
        double x1, x2, y1, y2;
        if (l->type == ENTRY) {
          x1 = l->x_start + cos(l->a_start + M_PI/2.0)*map->lane_width/2.0;
          y1 = l->y_start + sin(l->a_start + M_PI/2.0)*map->lane_width/2.0;
          x2 = l->x_start + cos(l->a_start - M_PI/2.0)*map->lane_width/2.0;
          y2 = l->y_start + sin(l->a_start - M_PI/2.0)*map->lane_width/2.0;
          glVertex3f(x1, 0.0, y1);
          glVertex3f(x1, -10.0, y1);
          glVertex3f(x2, -10.0, y2);
          glVertex3f(x2, 0.0, y2);
        }
        if (l->type == EXIT || l->merge_direction != 0) {
          x1 = l->x_end + cos(l->a_end + M_PI/2.0)*map->lane_width/2.0;
          y1 = l->y_end + sin(l->a_end + M_PI/2.0)*map->lane_width/2.0;
          x2 = l->x_end + cos(l->a_end - M_PI/2.0)*map->lane_width/2.0;
          y2 = l->y_end + sin(l->a_end - M_PI/2.0)*map->lane_width/2.0;
          glVertex3f(x1, 0.0, y1);
          glVertex3f(x1, -10.0, y1);
          glVertex3f(x2, -10.0, y2);
          glVertex3f(x2, 0.0, y2);
        }
      }
      glEnd();
    }
  }

  for (unsigned int k = 0; k < map->segments.size(); k++) {
    Log::getStream(5) << "Handling segment #" << k << "..." << endl;

    Segment *s = map->segments[k];
    if (s->lanes.empty()) continue;

    if (s->geometry == STRAIGHT) {
      Log::getStream(5) << "  Straight segment: " << setiosflags(ios::fixed) << setprecision(2) << s->x << " " << setiosflags(ios::fixed) << setprecision(2) << s->y << " " << setiosflags(ios::fixed) << setprecision(2) << s->a << endl;

      // Transform
      glPushMatrix();
      glTranslatef(s->x, 0.0, s->y);
      glRotatef(-s->a*180.0/M_PI, 0.0, 1.0, 0.0);   

      // road
      if (this->weather & RAIN)
        glColor4f(1.0, 1.0, 1.0, 0.3);
      else
        glColor3f(1.0, 1.0, 1.0);
      glBindTexture(GL_TEXTURE_2D, road_texture);
      
      glEnable(GL_TEXTURE_2D);
      
      glBegin(GL_QUADS);
      glTexCoord2f(0.0, 0.0);
      glVertex3f(0.0, 0.0, 0.0);
      glTexCoord2f(s->length, 0.0);
      glVertex3f(s->length, 0.0, 0.0);
      glTexCoord2f(s->length, (float)s->lanes.size()*map->lane_width);
      glVertex3f(s->length, 0.0, (float)s->lanes.size()*map->lane_width);
      glTexCoord2f(0.0, (float)s->lanes.size()*map->lane_width);
      glVertex3f(0.0, 0.0, (float)s->lanes.size()*map->lane_width);
      glEnd();
      
      glDisable(GL_TEXTURE_2D);

      // Grass
      glColor3f(1.0, 1.0, 1.0);
      glBindTexture(GL_TEXTURE_2D, grass_texture);
      glEnable(GL_TEXTURE_2D);
      glBegin(GL_QUADS);
      glTexCoord2f(0.0, -SIDE_WIDTH);
      glVertex3f(0.0, -0.05, -SIDE_WIDTH);
      glTexCoord2f(s->length, -SIDE_WIDTH);
      glVertex3f(s->length, -0.05, -SIDE_WIDTH);
      glTexCoord2f(s->length, ((float)s->lanes.size())*map->lane_width+SIDE_WIDTH);
      glVertex3f(s->length, -0.05, ((float)s->lanes.size())*map->lane_width+SIDE_WIDTH);
      glTexCoord2f(0.0,((float)s->lanes.size())*map->lane_width+SIDE_WIDTH);
      glVertex3f(0.0, -0.05, ((float)s->lanes.size())*map->lane_width+SIDE_WIDTH);
      glEnd();
      glDisable(GL_TEXTURE_2D);

      // Mark start of segment
      glBegin(GL_LINES);
      glColor3f(1, 1, 0);
      glLineWidth(3);
      glVertex3f(0.0, 0.0, -SIDE_WIDTH-1.0);
      glVertex3f(0.0, 0.0, (float)(s->lanes.size())*map->lane_width+SIDE_WIDTH+1.0);
      glEnd();

      // broken lane markings
      glBegin(GL_QUADS);
      glColor3f(1, 1, 1);
      for (unsigned int i = 0; i < s->lanes.size()-1; i++) {
        float z = ((float)(i+1))*map->lane_width;
        Lane *l = s->lanes[i];
        for (float x = 0.0; x < s->length; x += MARK_LENGTH+MARK_INTER) {
          if ((l->allowedRight(x) == 0.0 || l->right->allowedLeft(x) == 0.0) &&
              (l->allowedRight(x+MARK_LENGTH) == 0.0 || l->right->allowedLeft(x+MARK_LENGTH) == 0.0)) {
            glVertex3f(x, 0.01, z-MARK_WIDTH/2.0);
            glVertex3f(x, 0.01, z+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH, 0.01, z+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH, 0.01, z-MARK_WIDTH/2.0);
          } else {
            // solid
            glVertex3f(x, 0.01, z-MARK_WIDTH/2.0);
            glVertex3f(x, 0.01, z+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH+MARK_INTER, 0.01, z+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH+MARK_INTER, 0.01, z-MARK_WIDTH/2.0);
          }
        }
      }
      glEnd();

      // solid markings
      glBegin(GL_QUADS);
      glColor3f(1, 1, 1);
      for (unsigned int i = 0; i < s->lanes.size()-1; i++) {
        float z = ((float)(i+1))*map->lane_width;
        Lane *l = s->lanes[i];
        for (float x = 0.0; x < s->length; x += MARK_LENGTH+MARK_INTER) {
          if ((l->allowedRight(x) != 0.0 || l->allowedRight(x+MARK_LENGTH) != 0.0) &&
              (l->right->allowedLeft(x) == 0.0 && l->right->allowedLeft(x+MARK_LENGTH) == 0.0)) {
            glVertex3f(x, 0.01, z-INNER_MARK_OFFSET-MARK_WIDTH/2.0);
            glVertex3f(x, 0.01, z-INNER_MARK_OFFSET+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH+MARK_INTER, 0.01, z-INNER_MARK_OFFSET+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH+MARK_INTER, 0.01, z-INNER_MARK_OFFSET-MARK_WIDTH/2.0);
          }
          if ((l->right->allowedLeft(x) != 0.0 || l->right->allowedLeft(x+MARK_LENGTH) != 0.0) &&
              (l->allowedRight(x) == 0.0 && l->allowedRight(x+MARK_LENGTH) == 0.0)) {
            glVertex3f(x, 0.01, z+INNER_MARK_OFFSET-MARK_WIDTH/2.0);
            glVertex3f(x, 0.01, z+INNER_MARK_OFFSET+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH+MARK_INTER, 0.01, z+INNER_MARK_OFFSET+MARK_WIDTH/2.0);
            glVertex3f(x+MARK_LENGTH+MARK_INTER, 0.01, z+INNER_MARK_OFFSET-MARK_WIDTH/2.0);
          }
        }
      }
      glEnd();
      
      // plain white
      glBegin(GL_QUADS);
      glColor3f(1, 1, 1);
      glVertex3f(0.0, 0.01, -MARK_WIDTH/2.0);
      glVertex3f(0.0, 0.01,  MARK_WIDTH/2.0);
      glVertex3f(s->length, 0.01,  MARK_WIDTH/2.0);
      glVertex3f(s->length, 0.01, -MARK_WIDTH/2.0);
      glVertex3f(0.0, 0.01, (float)s->lanes.size()*map->lane_width-MARK_WIDTH/2.0);
      glVertex3f(0.0, 0.01, (float)s->lanes.size()*map->lane_width+MARK_WIDTH/2.0);
      glVertex3f(s->length, 0.01, (float)s->lanes.size()*map->lane_width+MARK_WIDTH/2.0);
      glVertex3f(s->length, 0.01, (float)s->lanes.size()*map->lane_width-MARK_WIDTH/2.0);
      glEnd();

      glPopMatrix();
    } else if (s->geometry == CIRCULAR) {
      // Center of the circle
      double xc;
      double yc;
      if (s->angle > 0.0) {
        xc = s->x + cos(s->a+M_PI/2.0)*s->radius;
        yc = s->y + sin(s->a+M_PI/2.0)*s->radius;
      } else {
        xc = s->x + cos(s->a-M_PI/2.0)*s->radius;
        yc = s->y + sin(s->a-M_PI/2.0)*s->radius;
      }
      Log::getStream(5) << "  Circular segment: " << setiosflags(ios::fixed) << setprecision(2) << s->x << " " << setiosflags(ios::fixed) << setprecision(2) << s->y << " " << setiosflags(ios::fixed) << setprecision(2) << s->a << " (Center: " << setiosflags(ios::fixed) << setprecision(2) << xc << " " << setiosflags(ios::fixed) << setprecision(2) << yc << " - Radius: " << setiosflags(ios::fixed) << setprecision(2) << s->radius << ")" << endl;
      
      glPushMatrix();
      glTranslatef(xc, 0.0, yc);
      if (s->angle > 0.0)
        glRotatef(90.0-s->a*180.0/M_PI, 0.0, 1.0, 0.0);
      else
        glRotatef(-90.0-s->a*180.0/M_PI, 0.0, 1.0, 0.0);

      float r = s->radius;
      if (s->angle < 0.0) {
        r += map->lane_width*(float)s->lanes.size();
      } else {
        r -= map->lane_width*(float)s->lanes.size();
      }
      
      float dangle_length = MARK_LENGTH/s->radius;
      float f = (s->angle < 0.0)?-1.0:1.0;

      // road
      if (this->weather & RAIN)
        glColor4f(1.0, 1.0, 1.0, 0.3);
      else
        glColor3f(1.0, 1.0, 1.0);
      glBindTexture(GL_TEXTURE_2D, road_texture);
      
      glEnable(GL_TEXTURE_2D);
      glBegin(GL_TRIANGLE_STRIP);
      for (float a = 0.0; a < fabs(s->angle); a += dangle_length) {
        glTexCoord2f(cos(f*a)*(s->radius), sin(f*a)*(s->radius));
        glVertex3f(cos(f*a)*(s->radius), 0.0, sin(f*a)*(s->radius));
        glTexCoord2f(cos(f*a)*r, sin(f*a)*r);
        glVertex3f(cos(f*a)*r, 0.0, sin(f*a)*r);
      }
      glTexCoord2f(cos(s->angle)*(s->radius), sin(s->angle)*(s->radius));
      glVertex3f(cos(s->angle)*(s->radius), 0.0, sin(s->angle)*(s->radius));
      glTexCoord2f(cos(s->angle)*r, sin(s->angle)*r);
      glVertex3f(cos(s->angle)*r, 0.0, sin(s->angle)*r);
      glEnd();
      glDisable(GL_TEXTURE_2D);

      // Grass
      glColor3f(1.0, 1.0, 1.0);
      glBindTexture(GL_TEXTURE_2D, grass_texture);
      glEnable(GL_TEXTURE_2D);
      glBegin(GL_TRIANGLE_STRIP);
      for (float a = 0.0; a < fabs(s->angle); a += dangle_length) {
        glTexCoord2f(cos(f*a)*(s->radius+f*SIDE_WIDTH), sin(f*a)*(s->radius+f*SIDE_WIDTH));
        glVertex3f(cos(f*a)*(s->radius+f*SIDE_WIDTH), -0.05, sin(f*a)*(s->radius+f*SIDE_WIDTH));
        glTexCoord2f(cos(f*a)*(r-f*SIDE_WIDTH), sin(f*a)*(r-f*SIDE_WIDTH));
        glVertex3f(cos(f*a)*(r-f*SIDE_WIDTH), -0.05, sin(f*a)*(r-f*SIDE_WIDTH));
      }
      glTexCoord2f(cos(s->angle)*(s->radius+f*SIDE_WIDTH), sin(s->angle)*(s->radius+f*SIDE_WIDTH));
      glVertex3f(cos(s->angle)*(s->radius+f*SIDE_WIDTH), -0.05, sin(s->angle)*(s->radius+f*SIDE_WIDTH));
      glTexCoord2f(cos(s->angle)*(r-f*SIDE_WIDTH), sin(s->angle)*(r-f*SIDE_WIDTH));
      glVertex3f(cos(s->angle)*(r-f*SIDE_WIDTH), -0.05, sin(s->angle)*(r-f*SIDE_WIDTH));
      glEnd();
      glDisable(GL_TEXTURE_2D);

      glDisable(GL_TEXTURE_2D);

      // Mark start of segment
      glBegin(GL_LINES);
      glColor3f(1, 1, 0);
      glLineWidth(3);
      if (s->angle < 0.0) {
        glVertex3f(s->radius-SIDE_WIDTH-1.0, 0.0, 0.0);
        glVertex3f(s->radius+(float)(s->lanes.size())*map->lane_width+SIDE_WIDTH+1.0, 0.0, 0.0);
      } else {
        glVertex3f(s->radius+SIDE_WIDTH+1.0, 0.0, 0.0);
        glVertex3f(s->radius-(float)(s->lanes.size())*map->lane_width-SIDE_WIDTH-1.0, 0.0, 0.0);
      }
      glEnd();

      // broken lane markings
      glBegin(GL_QUADS);
      glColor3f(1, 1, 1);
      for (unsigned int i = 0; i < s->lanes.size()-1; i++) {
        float r = s->radius;
        if (s->angle < 0.0) {
          r += ((float)(i+1))*map->lane_width;
        } else {
          r -= ((float)(i+1))*map->lane_width;
        }

        float dangle_length = MARK_LENGTH/r;
        float dangle_inter  = MARK_INTER/r;
        float f = (s->angle < 0.0)?-1.0:1.0;
        Lane *l = s->lanes[i];

        for (float a = 0.0; a < fabs(s->angle); a += dangle_length+dangle_inter) {
          if ((l->allowedRight(a) == 0.0 || l->right->allowedLeft(a) == 0.0) &&
              (l->allowedRight(a+dangle_length) == 0.0 || l->right->allowedLeft(a+dangle_length) == 0.0)) {
            glVertex3f(cos(f*a)*(r-MARK_WIDTH/2.0), 0.01, sin(f*a)*(r-MARK_WIDTH/2.0));
            glVertex3f(cos(f*a)*(r+MARK_WIDTH/2.0), 0.01, sin(f*a)*(r+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length))*(r+MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length))*(r+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length))*(r-MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length))*(r-MARK_WIDTH/2.0));
          } else {
            glVertex3f(cos(f*a)*(r-MARK_WIDTH/2.0), 0.01, sin(f*a)*(r-MARK_WIDTH/2.0));
            glVertex3f(cos(f*a)*(r+MARK_WIDTH/2.0), 0.01, sin(f*a)*(r+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length+dangle_inter))*(r+MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length+dangle_inter))*(r+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length+dangle_inter))*(r-MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length+dangle_inter))*(r-MARK_WIDTH/2.0));
          }
        }
      }
      glEnd();

      // solid markings
      glBegin(GL_QUADS);
      glColor3f(1, 1, 1);
      for (unsigned int i = 0; i < s->lanes.size()-1; i++) {
        float r = s->radius;
        if (s->angle < 0.0) {
          r += ((float)(i+1))*map->lane_width;
        } else {
          r -= ((float)(i+1))*map->lane_width;
        }

        float dangle_length = MARK_LENGTH/r;
        float dangle_inter  = MARK_INTER/r;
        float f = (s->angle < 0.0)?-1.0:1.0;
        Lane *l = s->lanes[i];

        for (float a = 0.0; a < fabs(s->angle); a += dangle_length+dangle_inter) {
          if ((l->allowedRight(a) != 0.0 || l->allowedRight(a+dangle_length) != 0.0) &&
              (l->right->allowedLeft(a) == 0.0 && l->right->allowedLeft(a+dangle_length) == 0.0)) {
            glVertex3f(cos(f*a)*(r+f*INNER_MARK_OFFSET-MARK_WIDTH/2.0), 0.01, sin(f*a)*(r+f*INNER_MARK_OFFSET-MARK_WIDTH/2.0));
            glVertex3f(cos(f*a)*(r+f*INNER_MARK_OFFSET+MARK_WIDTH/2.0), 0.01, sin(f*a)*(r+f*INNER_MARK_OFFSET+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length+dangle_inter))*(r+f*INNER_MARK_OFFSET+MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length+dangle_inter))*(r+f*INNER_MARK_OFFSET+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length+dangle_inter))*(r+f*INNER_MARK_OFFSET-MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length+dangle_inter))*(r+f*INNER_MARK_OFFSET-MARK_WIDTH/2.0));
          }
          if ((l->right->allowedLeft(a) != 0.0 || l->right->allowedLeft(a+dangle_length) != 0.0) &&
              (l->allowedRight(a) == 0.0 && l->allowedRight(a+dangle_length) == 0.0)) {
            glVertex3f(cos(f*a)*(r-f*INNER_MARK_OFFSET-MARK_WIDTH/2.0), 0.01, sin(f*a)*(r-f*INNER_MARK_OFFSET-MARK_WIDTH/2.0));
            glVertex3f(cos(f*a)*(r-f*INNER_MARK_OFFSET+MARK_WIDTH/2.0), 0.01, sin(f*a)*(r-f*INNER_MARK_OFFSET+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length+dangle_inter))*(r-f*INNER_MARK_OFFSET+MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length+dangle_inter))*(r-f*INNER_MARK_OFFSET+MARK_WIDTH/2.0));
            glVertex3f(cos(f*(a+dangle_length+dangle_inter))*(r-f*INNER_MARK_OFFSET-MARK_WIDTH/2.0), 0.01, sin(f*(a+dangle_length+dangle_inter))*(r-f*INNER_MARK_OFFSET-MARK_WIDTH/2.0));
          }
        }
      }
      glEnd();

      // plain white
      glColor3f(1, 1, 1);
      for (unsigned int i = 0; i < 2; i++) {
        glBegin(GL_TRIANGLE_STRIP);
        float r = s->radius;
        if (s->angle < 0.0) {
          r += ((float)(i))*map->lane_width*(float)s->lanes.size();
        } else {
          r -= ((float)(i))*map->lane_width*(float)s->lanes.size();
        }

        float dangle_length = MARK_LENGTH/r;
        float f = (s->angle < 0.0)?-1.0:1.0;
        for (float a = 0.0; a < fabs(s->angle); a += dangle_length) {
          glVertex3f(cos(f*a)*(r-MARK_WIDTH/2.0), 0.01, sin(f*a)*(r-MARK_WIDTH/2.0));
          glVertex3f(cos(f*a)*(r+MARK_WIDTH/2.0), 0.01, sin(f*a)*(r+MARK_WIDTH/2.0));
        }
        glVertex3f(cos(s->angle)*(r-MARK_WIDTH/2.0), 0.01, sin(s->angle)*(r-MARK_WIDTH/2.0));
        glVertex3f(cos(s->angle)*(r+MARK_WIDTH/2.0), 0.01, sin(s->angle)*(r+MARK_WIDTH/2.0));
        glEnd();
      }

      glPopMatrix();
    }

    // Draw entry and exit
    glBegin(GL_TRIANGLES);
    for (unsigned int i = 0; i < s->lanes.size(); i++) {
      Lane *l = s->lanes[i];
      if (l->type == ENTRY) {
        glColor3f(0.0, 1.0, 0.0);
        double x = l->x_start + cos(l->a_start)*1.0;
        double y = l->y_start + sin(l->a_start)*1.0;
        glVertex3f(x + cos(l->a_start + M_PI/2.0)*0.5, 0.01, y + sin(l->a_start + M_PI/2.0)*0.5);
        glVertex3f(x + cos(l->a_start)*1.0, 0.01, y + sin(l->a_start)*1.0);
        glVertex3f(x + cos(l->a_start - M_PI/2.0)*0.5, 0.01, y + sin(l->a_start - M_PI/2.0)*0.5);
      } else if (l->type == EXIT) {
        glColor3f(1.0, 0.0, 0.0);
        double x = l->x_end + cos(l->a_end + M_PI)*2.0;
        double y = l->y_end + sin(l->a_end + M_PI)*2.0;
        glVertex3f(x + cos(l->a_end + M_PI/2.0)*0.5, 0.01, y + sin(l->a_end + M_PI/2.0)*0.5);
        glVertex3f(x + cos(l->a_end)*1.0, 0.01, y + sin(l->a_end)*1.0);
        glVertex3f(x + cos(l->a_end - M_PI/2.0)*0.5, 0.01, y + sin(l->a_end - M_PI/2.0)*0.5);
      }
    }
    glEnd();

    // Draw connections
    glBegin(GL_LINES);
    glLineWidth(3);
    glColor3f(0.0, 0.0, 1.0);
    for (unsigned int i = 0; i < s->lanes.size(); i++) {
      Lane *l = s->lanes[i];
      if (l->next) {
        double x1 = l->x_end + cos(l->a_end + M_PI)*1.0;
        double y1 = l->y_end + sin(l->a_end + M_PI)*1.0;
        double x2 = l->next->x_start + cos(l->next->a_start)*1.0;
        double y2 = l->next->y_start + sin(l->next->a_start)*1.0;
        glVertex3f(x1, 0.01, y1);
        glVertex3f(x2, 0.01, y2);
      }
    }
    glEnd();

    // Draw merge direction
    glColor3f(1.0, 1.0, 1.0);
    for (unsigned int i = 0; i < s->lanes.size(); i++) {
      Lane *l = s->lanes[i];
      if (l->merge_direction != 0) {
        double a = M_PI/6.0;
        if (l->merge_direction == 1) {
          a = -M_PI/6.0;
        }
        double x = l->x_end + cos(l->a_end + M_PI)*2.0;
        double y = l->y_end + sin(l->a_end + M_PI)*2.0;
        glPushMatrix();
        glTranslatef(x, 0.0, y);
        glRotatef((a - l->a_end)*180.0/M_PI, 0.0, 1.0, 0.0);

        glBegin(GL_TRIANGLES);
        glVertex3f(-0.5, 0.01, 0.2);
        glVertex3f(0.3, 0.01, 0.2);
        glVertex3f(0.3, 0.01, -0.2);

        glVertex3f(0.3, 0.01, 0.5);
        glVertex3f(0.6, 0.01, 0.0);
        glVertex3f(0.3, 0.01, -0.5);

        glVertex3f(-0.5, 0.01, 0.2);
        glVertex3f(0.3, 0.01, -0.2);
        glVertex3f(-0.5, 0.01, -0.2);
        glEnd();

        glPopMatrix();
      }
    }
  }

  return;
}

void RealisticDrawer::drawEnvironment()
{
  double lx1 = terrain_xmin + 0.3*(terrain_xmax-terrain_xmin);
  double ly1 = terrain_ymin + 0.3*(terrain_ymax-terrain_ymin);

  double lx2 = terrain_xmin + 0.6*(terrain_xmax-terrain_xmin);
  double ly2 = terrain_ymin + 0.4*(terrain_ymax-terrain_ymin);

  double d1 = (terrain_xmax-terrain_xmin)/5.0;
  double d2 = (terrain_xmax-terrain_xmin)/3.0;

  for (unsigned int j=0; j < TERRAIN_RESOLUTION-1; j++) {
    glBegin(GL_TRIANGLE_STRIP);
    double y1 = GET_TERRAIN_Y(j);
    double y2 = GET_TERRAIN_Y(j+1);
    for (unsigned int i=0; i < TERRAIN_RESOLUTION; i++) {
      double x = GET_TERRAIN_X(i);
      double z1 = terrain[i][j];
      double z2 = terrain[i][j+1];

      /* Create gradient */
      double f1 = sqrt((lx1-x)*(lx1-x)+(ly1-y1)*(ly1-y1));
      f1 = 1.0-f1/d1;
      if (f1 < 0.0) f1 = 0.0;

      double f2 = sqrt((lx2-x)*(lx2-x)+(ly2-y1)*(ly2-y1));
      f2 = 1.0-f2/d2;
      if (f2 < 0.0) f2 = 0.0;

      double f = (f1+f2)*0.2;
      double a = 1.0;
      if (this->weather & RAIN)
        a = 0.3;
      
      glColor3f(a*(60.0/255.0 + f), a*(74.0/255.0 + random_uniform()*50.0/255.0 + f), a*(14.0/255.0 + f));
      glVertex3f(x, z1, y1);
      glColor3f(a*(60.0/255.0 + f), a*(74.0/255.0 + random_uniform()*50.0/255.0 + f), a*(14.0/255.0 + f));
      glVertex3f(x, z2, y2);
    }
    glEnd();
  }

  return;
}

#define LIGHT_HEIGHT 2.0
#define LIGHT_SIDE   0.07
#define LIGHT_BOX_HEIGHT 0.6
#define LIGHT_BOX_SIDE 0.17
#define SPEED_RADIUS 1.0
#define SPEED_HEIGHT 10.0

void RealisticDrawer::draw()
{
  char str[30];

  // Draw the road actuators
  for (unsigned int i = 0; i < map->actuators.size(); i++) {
    RoadActuator *a = map->actuators[i];
    Lane *l = a->lane;
    double x1, y1, f, yaw;
    color_t color;
    
    switch (a->type) {
    case TRAFFICLIGHT:
      if (l->segment->geometry == STRAIGHT) {
        x1 = l->x_start + cos(l->a_start)*a->position + cos(l->a_start + M_PI/2.0)*map->lane_width/2.0;
        y1 = l->y_start + sin(l->a_start)*a->position + sin(l->a_start + M_PI/2.0)*map->lane_width/2.0;
        yaw = l->a_start - M_PI/2.0;
      } else {
        f = (l->segment->angle < 0.0)?-1.0:1.0;
        x1 = l->xc - cos(l->angle_start + f*a->position)*(l->radius-f*map->lane_width/2.0);
        y1 = l->yc - sin(l->angle_start + f*a->position)*(l->radius-f*map->lane_width/2.0);
        yaw = l->angle_start + f*a->position + ((f < 0.0)?0.0:M_PI);
      }

      glPushMatrix();
      glTranslatef(x1,LIGHT_HEIGHT/2.0,y1);
      glRotatef(-(yaw/M_PI)*180.0, 0.0, 1.0, 0.0);
      glScalef(LIGHT_SIDE,LIGHT_HEIGHT,LIGHT_SIDE);
      glColor3f(0.0,0.0,0.0);
      glutSolidCube(1.0);
      glPopMatrix();

      glPushMatrix();
      glTranslatef(x1,LIGHT_HEIGHT+LIGHT_BOX_HEIGHT/2.0,y1);
      glRotatef(-(yaw/M_PI)*180.0, 0.0, 1.0, 0.0);
      glScalef(LIGHT_BOX_SIDE,LIGHT_BOX_HEIGHT,LIGHT_BOX_SIDE);
      glColor3f(0.0,0.0,0.0);
      glutSolidCube(1.0);
      glPopMatrix();

      color = (static_cast<TrafficLightActuator *>(a))->color();
      if (color == GREEN) glColor3f(0.0,1.0,0.0);
      else glColor3f(0.0,0.3,0.0);
      glPushMatrix();
      glTranslatef(x1,LIGHT_HEIGHT+2.0*LIGHT_BOX_HEIGHT/3.0,y1);
      glRotatef(-(yaw/M_PI)*180.0, 0.0, 1.0, 0.0);
      glTranslatef(0.0, 0.0, -LIGHT_BOX_SIDE/2.0*0.9);
      glScalef(LIGHT_BOX_SIDE*0.4,LIGHT_BOX_SIDE*0.4, 0.05);
      glutSolidSphere(1.0,8,8);
      glPopMatrix();

      if (color == RED) glColor3f(1.0,0.0,0.0);
      else glColor3f(0.3,0.0,0.0);
      glPushMatrix();
      glTranslatef(x1,LIGHT_HEIGHT+1.0*LIGHT_BOX_HEIGHT/3.0,y1);
      glRotatef(-(yaw/M_PI)*180.0, 0.0, 1.0, 0.0);
      glTranslatef(0.0, 0.0, -LIGHT_BOX_SIDE/2.0*0.9);
      glScalef(LIGHT_BOX_SIDE*0.4,LIGHT_BOX_SIDE*0.4, 0.05);
      glutSolidSphere(1.0,8,8);
      glPopMatrix();
      
      break;
    case SPEEDLIMIT:
      if (l->segment->geometry == STRAIGHT) {
        x1 = l->x_start + cos(l->a_start)*a->position;
        y1 = l->y_start + sin(l->a_start)*a->position;
        yaw = l->a_start;
      } else {
        f = (l->segment->angle < 0.0)?-1.0:1.0;
        x1 = l->xc - cos(l->angle_start + f*a->position)*l->radius;
        y1 = l->yc - sin(l->angle_start + f*a->position)*l->radius;
        yaw = l->angle_start + f*a->position + ((f < 0.0)?0.0:M_PI) + M_PI/2.0;
      }

      glPushMatrix();
      glTranslatef(x1,SPEED_HEIGHT,y1);
      glRotatef(-(yaw/M_PI)*180.0, 0.0, 1.0, 0.0);
      glColor3f(1.0,0.0,0.0);
      glBegin(GL_QUAD_STRIP);
      for (double b = 0.0; b < 2.0*M_PI; b+=M_PI/8) {
        glVertex3f(0.0, sin(b)*SPEED_RADIUS, cos(b)*SPEED_RADIUS);
        glVertex3f(0.0, sin(b)*SPEED_RADIUS*0.8, cos(b)*SPEED_RADIUS*0.8);
      }
      glVertex3f(0.0, 0.0, SPEED_RADIUS);
      glVertex3f(0.0, 0.0, SPEED_RADIUS*0.8);
      glEnd();
      glColor3f(1.0,1.0,1.0);
      glBegin(GL_TRIANGLE_FAN);
      glVertex3f(0.0, 0.0, 0.0);
      for (double b = 0.0; b < 2.0*M_PI; b+=M_PI/8) {
        glVertex3f(0.0, sin(b)*SPEED_RADIUS*0.8, cos(b)*SPEED_RADIUS*0.8);
      }
      glVertex3f(0.0, 0.0, SPEED_RADIUS*0.8);
      glEnd();
      glColor3f(0.0, 0.0, 0.0);
      glTranslatef(-0.02, 0.0, 0.0);
      glRotatef(-90, 1.0, 0.0, 0.0);
      glRotatef(90, 0.0, 0.0, 1.0);
      snprintf(str, 30, "%.0f", a->lane->maximum_speed*3.6);
      drawText(0.0, 0.0, 0.0, 0.0, SPEED_RADIUS*0.5, 0.0, 0.0, SPEED_RADIUS*0.05, str);
      glPopMatrix();

      glPushMatrix();
      glTranslatef(x1,SPEED_HEIGHT-SPEED_RADIUS*2.0,y1);
      glRotatef(-(yaw/M_PI)*180.0, 0.0, 1.0, 0.0);
      glColor3f(1.0,1.0,1.0);
      glBegin(GL_QUAD_STRIP);
      for (double b = 0.0; b < 2.0*M_PI; b+=M_PI/8) {
        glVertex3f(0.0, sin(b)*SPEED_RADIUS*0.8, cos(b)*SPEED_RADIUS*0.8);
        glVertex3f(0.0, sin(b)*SPEED_RADIUS*0.8*0.8, cos(b)*SPEED_RADIUS*0.8*0.8);
      }
      glVertex3f(0.0, 0.0, SPEED_RADIUS*0.8);
      glVertex3f(0.0, 0.0, SPEED_RADIUS*0.8*0.8);
      glEnd();
      glColor3f(0.0,0.0,1.0);
      glBegin(GL_TRIANGLE_FAN);
      glVertex3f(0.0, 0.0, 0.0);
      for (double b = 0.0; b < 2.0*M_PI; b+=M_PI/8) {
        glVertex3f(0.0, sin(b)*SPEED_RADIUS*0.8*0.8, cos(b)*SPEED_RADIUS*0.8*0.8);
      }
      glVertex3f(0.0, 0.0, SPEED_RADIUS*0.8*0.8);
      glEnd();
      glColor3f(1.0, 1.0, 1.0);
      glTranslatef(-0.02, 0.0, 0.0);
      glRotatef(-90, 1.0, 0.0, 0.0);
      glRotatef(90, 0.0, 0.0, 1.0);
      snprintf(str, 30, "%.0f", a->lane->minimum_speed*3.6);
      drawText(0.0, 0.0, 0.0, 0.0, SPEED_RADIUS*0.8*0.5, 0.0, 0.0, SPEED_RADIUS*0.8*0.05, str);
      glPopMatrix();

      break;
    }
  }

  // Draw the road sensors
  for (unsigned int i = 0; i < map->sensors.size(); i++) {
    RoadSensor *r = map->sensors[i];
    Lane *l = r->lane;
    double x1, y1, x2, y2, f, x, y, yaw;
    
    switch (r->type) {
    case DENSITY:
      glBegin(GL_LINES);
      glLineWidth(3.0f);

      if (l->segment->geometry == STRAIGHT) {
        x1 = l->x_start + cos(l->a_start)*r->position + cos(l->a_start + M_PI/2.0)*map->lane_width/2.0;
        x2 = l->x_start + cos(l->a_start)*r->position + cos(l->a_start - M_PI/2.0)*map->lane_width/2.0;
        y1 = l->y_start + sin(l->a_start)*r->position + sin(l->a_start + M_PI/2.0)*map->lane_width/2.0;
        y2 = l->y_start + sin(l->a_start)*r->position + sin(l->a_start - M_PI/2.0)*map->lane_width/2.0;
        yaw = -l->a_start + M_PI/2.0;
        x = (x1+x2)/2.0;
        y = (y1+y2)/2.0;
      } else {
        f = (l->segment->angle < 0.0)?-1.0:1.0;
        x1 = l->xc - cos(l->angle_start + f*r->position)*(l->radius+map->lane_width/2.0);
        x2 = l->xc - cos(l->angle_start + f*r->position)*(l->radius-map->lane_width/2.0);
        y1 = l->yc - sin(l->angle_start + f*r->position)*(l->radius+map->lane_width/2.0);
        y2 = l->yc - sin(l->angle_start + f*r->position)*(l->radius-map->lane_width/2.0);
        yaw = -(l->angle_start + f*r->position) + ((f < 0.0)?0.0:M_PI);
        x = (x1+x2)/2.0;
        y = (y1+y2)/2.0;
      }
      if (r->wasEntryTriggered())
        glColor3f(0.0, 1.0, 1.0);
      else
        glColor3f(0.0, 0.5, 0.5);
      glVertex3f(x1, 0.01, y1);
      glVertex3f(x2, 0.01, y2);

      
      if (l->segment->geometry == STRAIGHT) {
        x1 = l->x_start + cos(l->a_start)*r->position2 + cos(l->a_start + M_PI/2.0)*map->lane_width/2.0;
        x2 = l->x_start + cos(l->a_start)*r->position2 + cos(l->a_start - M_PI/2.0)*map->lane_width/2.0;
        y1 = l->y_start + sin(l->a_start)*r->position2 + sin(l->a_start + M_PI/2.0)*map->lane_width/2.0;
        y2 = l->y_start + sin(l->a_start)*r->position2 + sin(l->a_start - M_PI/2.0)*map->lane_width/2.0;
      } else {
        f = (l->segment->angle < 0.0)?-1.0:1.0;
        x1 = l->xc - cos(l->angle_start + f*r->position2)*(l->radius+map->lane_width/2.0);
        x2 = l->xc - cos(l->angle_start + f*r->position2)*(l->radius-map->lane_width/2.0);
        y1 = l->yc - sin(l->angle_start + f*r->position2)*(l->radius+map->lane_width/2.0);
        y2 = l->yc - sin(l->angle_start + f*r->position2)*(l->radius-map->lane_width/2.0);
      }
      if (r->wasExitTriggered())
        glColor3f(0.0, 1.0, 1.0);
      else
        glColor3f(0.0, 0.5, 0.5);
      glVertex3f(x1, 0.01, y1);
      glVertex3f(x2, 0.01, y2);

      glEnd();

      glColor3f(0.0, 1.0, 1.0);
      snprintf(str, 30, "%.0f veh/km", r->getResult());
      drawText(x, 0.02, y, yaw, 0.2, 0.0, 0.0, -0.1, str);

      break;
    case SPEED:
      if (r->wasTriggered())
        glColor3f(1.0, 0.0, 0.0);
      else
        glColor3f(0.5, 0.0, 0.0);
      glBegin(GL_LINE_LOOP);
      glLineWidth(3.0f);

      if (l->segment->geometry == STRAIGHT) {
        x1 = l->x_start + cos(l->a_start)*r->position;
        y1 = l->y_start + sin(l->a_start)*r->position;
        yaw = -l->a_start + M_PI/2.0;
      } else {
        f = (l->segment->angle < 0.0)?-1.0:1.0;
        x1 = l->xc - cos(l->angle_start + f*r->position)*l->radius;
        y1 = l->yc - sin(l->angle_start + f*r->position)*l->radius;
        yaw = -(l->angle_start + f*r->position) + ((f < 0.0)?0.0:M_PI);
      }

      for (double a = 0.0; a < 2.0*M_PI; a+=M_PI/8) {
        glVertex3f(x1+cos(a)*map->lane_width/4.0, 0.01, y1+sin(a)*map->lane_width/4.0);
      }

      glEnd();

      glColor3f(1.0, 0.0, 0.0);
      snprintf(str, 30, "%.0f km/h", r->getResult());
      drawText(x1, 0.02, y1, yaw, 0.2, 0.0, 0.0, -map->lane_width/4.0-0.1, str);

      break;
    case FLOW:
      if (r->wasTriggered())
        glColor3f(1.0, 1.0, 0.0);
      else
        glColor3f(0.5, 0.5, 0.0);
      glBegin(GL_POLYGON);

      if (l->segment->geometry == STRAIGHT) {
        x1 = l->x_start + cos(l->a_start)*r->position;
        y1 = l->y_start + sin(l->a_start)*r->position;
        yaw = -l->a_start + M_PI/2.0;
      } else {
        f = (l->segment->angle < 0.0)?-1.0:1.0;
        x1 = l->xc - cos(l->angle_start + f*r->position)*l->radius;
        y1 = l->yc - sin(l->angle_start + f*r->position)*l->radius;
        yaw = -(l->angle_start + f*r->position) + ((f < 0.0)?0.0:M_PI);
      }

      for (double a = 0.0; a < 2.0*M_PI; a+=M_PI/8) {
        glVertex3f(x1+cos(a)*map->lane_width/8.0, 0.01, y1+sin(a)*map->lane_width/8.0);
      }

      glEnd();

      glColor3f(1.0, 1.0, 0.0);
      snprintf(str, 30, "%.0f veh/h", r->getResult());
      drawText(x1, 0.02, y1, yaw, 0.2, 0.0, 0.0, map->lane_width/8.0+0.1, str);

      break;
    }
  }

  return;
}

#define BOTTOM 0.5f
#define TOP    0.5f
#define SIDE   1.0f

void RealisticDrawer::drawSkybox()
{
  GLdouble modelview[16];
  GLdouble invview[16];

  glMatrixMode(GL_MODELVIEW);

  // Get current transformation
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

  // Store the current matrix
  glPushMatrix();
  // Transform to only rotate matrix
  invview[0] = modelview[0];
  invview[1] = modelview[1];
  invview[2] = modelview[2];
  invview[3] = modelview[3];
  invview[4] = modelview[4];
  invview[5] = modelview[5];
  invview[6] = modelview[6];
  invview[7] = modelview[7];
  invview[8] = modelview[8];
  invview[9] = modelview[9];
  invview[10] = modelview[10];
  invview[11] = modelview[11];
  invview[12] = 0.0;
  invview[13] = 0.0;
  invview[14] = 0.0;
  invview[15] = modelview[15];
  glLoadMatrixd(invview);
  glTranslatef(0, 0.1, 0);

  // Enable/Disable features
  glPushAttrib(GL_ENABLE_BIT);
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);

  // Just in case we set all vertices to white.
  if (this->weather & RAIN)
    glColor4f(0.3,0.3,0.3,1);
  else
    glColor4f(1,1,1,1);

  // If there is fog: we cannot see the sky!
  if (this->weather & FOG) {
    glColor3f(0.8, 0.8, 0.8);
    glDisable(GL_TEXTURE_2D);
  }
  
  // Render the front quad
  glBindTexture(GL_TEXTURE_2D, skybox_texture[0]);
  glBegin(GL_QUADS);
  glTexCoord2f(1, 1); glVertex3f(  SIDE,  TOP, -SIDE );
  glTexCoord2f(0, 1); glVertex3f( -SIDE,  TOP, -SIDE );
  glTexCoord2f(0, 0); glVertex3f( -SIDE, -BOTTOM, -SIDE );
  glTexCoord2f(1, 0); glVertex3f(  SIDE, -BOTTOM, -SIDE );
  glEnd();

    // Render the left quad
  glBindTexture(GL_TEXTURE_2D, skybox_texture[1]);
  glBegin(GL_QUADS);
  glTexCoord2f(1, 1); glVertex3f(  SIDE,  TOP,  SIDE );
  glTexCoord2f(0, 1); glVertex3f(  SIDE,  TOP, -SIDE );
  glTexCoord2f(0, 0); glVertex3f(  SIDE, -BOTTOM, -SIDE );
  glTexCoord2f(1, 0); glVertex3f(  SIDE, -BOTTOM,  SIDE );  
  glEnd();
  
  // Render the back quad
  glBindTexture(GL_TEXTURE_2D, skybox_texture[2]);
  glBegin(GL_QUADS);
  glTexCoord2f(1, 1); glVertex3f( -SIDE,  TOP,  SIDE );
  glTexCoord2f(0, 1); glVertex3f(  SIDE,  TOP,  SIDE );
  glTexCoord2f(0, 0); glVertex3f(  SIDE, -BOTTOM,  SIDE );
  glTexCoord2f(1, 0); glVertex3f( -SIDE, -BOTTOM,  SIDE );
  glEnd();
  
  // Render the right quad
  glBindTexture(GL_TEXTURE_2D, skybox_texture[3]);
  glBegin(GL_QUADS);
  glTexCoord2f(1, 1); glVertex3f( -SIDE,  TOP, -SIDE );
  glTexCoord2f(0, 1); glVertex3f( -SIDE,  TOP,  SIDE );
  glTexCoord2f(0, 0); glVertex3f( -SIDE, -BOTTOM,  SIDE );
  glTexCoord2f(1, 0); glVertex3f( -SIDE, -BOTTOM, -SIDE );
  glEnd();
  
    // Render the top quad
  glBindTexture(GL_TEXTURE_2D, skybox_texture[4]);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 1); glVertex3f(  SIDE,  TOP, -SIDE );
  glTexCoord2f(0, 0); glVertex3f(  SIDE,  TOP,  SIDE );
  glTexCoord2f(1, 0); glVertex3f( -SIDE,  TOP,  SIDE );
  glTexCoord2f(1, 1); glVertex3f( -SIDE,  TOP, -SIDE );
  glEnd();
  
  // Render the bottom quad
  glBindTexture(GL_TEXTURE_2D, skybox_texture[5]);
  glBegin(GL_QUADS);
  glTexCoord2f(1, 0); glVertex3f( -SIDE, -BOTTOM, -SIDE );
  glTexCoord2f(1, 1); glVertex3f( -SIDE, -BOTTOM,  SIDE );
  glTexCoord2f(0, 1); glVertex3f(  SIDE, -BOTTOM,  SIDE );
  glTexCoord2f(0, 0); glVertex3f(  SIDE, -BOTTOM, -SIDE );
  glEnd();
  
  // Restore enable bits and matrix
  glPopAttrib();
  glPopMatrix();
}

void RealisticDrawer::renderBitmapString(float x, float y, float z, void *font, char *string)
{  
  char *c;
  glRasterPos3f(x, y, z);
  for (c=string; *c != '\0'; c++) {
    glutBitmapCharacter(font, *c);
  }
}

void RealisticDrawer::drawText(float x, float y, float z, float angle, float size, const char *text)
{
  drawText(x, y, z, angle, size, 0.0, 0.0, 0.0, text);
}

// Draw a text box
void RealisticDrawer::drawText(float x, float y, float z, float angle, float size, float offset_x, float offset_y, float offset_z, const char *text)
{
  int i, count;
  float sx, sy;

  // Compute the scale factors in x/y directions, based on the 'M'
  // character.
  sx = glutStrokeWidth(GLUT_STROKE_MONO_ROMAN, 'M');
  sy = sx * 1.3;

  glPushMatrix();
  glTranslatef(x, y, z);
  glRotatef(180, 0, 0, 1);
  glRotatef(90, 1, 0, 0);
  glRotatef(angle*180.0/M_PI, 0, 0, 1);
  glTranslatef(offset_x, offset_z, offset_y);
    
  glScalef(size / sx, size / sy, 1);
  glTranslatef(0, -sy/2.0, 0);
  glTranslatef(-sx*(float)strlen(text)/2.0, 0.0, 0);

  for (i = 0, count = 0; i < (int) strlen(text); i++, count++)
  {
    if (text[i] == '\n')
    {
      glTranslatef(-count*sx, -sy, 0);
      count = 0;      
    }
    else
    {
      glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, text[i]);
    }
  }

  glPopMatrix();

  return;
}

#define FAULT_ITERATIONS 100
#define DZ_N   1.0
#define DZ_0  20.0
#define N     50
#define D    300.0
#define PZONE 10.0

#define FILTER_SIZE  7
#define FILTER_NPASS 5

void RealisticDrawer::computeTerrain()
{
  terrain_xmin = 0.0;
  terrain_xmax = 0.0;
  terrain_ymin = 0.0;
  terrain_ymax = 0.0;
  for (unsigned int i=0; i < TERRAIN_RESOLUTION; i++) {
    for (unsigned int j=0; j < TERRAIN_RESOLUTION; j++) {
      terrain[i][j] = 10.0;
    }
  }

  for (unsigned int k = 0; k < map->segments.size(); k++) {
    Segment *s = map->segments[k];
    if (s->lanes.empty()) continue;

    double xmax;
    double xmin;
    double ymax;
    double ymin;

    if (s->geometry == STRAIGHT) {
      xmax = s->x + s->length;
      xmin = s->x - s->length;
      ymax = s->y + s->length;
      ymin = s->y - s->length;
    } else {
      xmax = s->x + 2.0*s->radius;
      xmin = s->x - 2.0*s->radius;
      ymax = s->y + 2.0*s->radius;
      ymin = s->y - 2.0*s->radius;
    }

    if (xmin < terrain_xmin) terrain_xmin = xmin;
    if (ymin < terrain_ymin) terrain_ymin = ymin;
    if (xmax > terrain_xmax) terrain_xmax = xmax;
    if (ymax > terrain_ymax) terrain_ymax = ymax;
  }

  // Scale number of iterations
  int fault_iterations = ((double)FAULT_ITERATIONS*sqrt((terrain_xmax-terrain_xmin)*(terrain_ymax-terrain_ymin)))/2500.0;
  int n = fault_iterations/2;

  // Make sure that roads are at level 0
  for (unsigned int k = 0; k < map->segments.size(); k++) {
    Segment *s = map->segments[k];
    if (s->lanes.empty()) continue;

    for (unsigned int i=0; i < TERRAIN_RESOLUTION; i++) {
      for (unsigned int j=0; j < TERRAIN_RESOLUTION; j++) {
        double x = GET_TERRAIN_X(i);
        double y = GET_TERRAIN_Y(j);

        // Translate
        x -= s->x;
        y -= s->y;
        // Rotate
        double t = cos(-s->a)*x - sin(-s->a)*y;
        y = sin(-s->a)*x + cos(-s->a)*y;
        x = t;
        
        // Transform coordinates
        if (s->geometry == STRAIGHT) {  
          // Test
          if (x > -SIDE_WIDTH*PZONE && x < s->length + SIDE_WIDTH*PZONE && y > -SIDE_WIDTH*PZONE && y < ((double)(s->lanes.size()))*map->lane_width + SIDE_WIDTH*PZONE) {
            terrain[i][j] = -0.1;
          }
        } else {
          double f = (s->angle < 0.0)?-1.0:1.0;
          y -= f*s->radius;
          double angle = atan2(y,x) + f*M_PI/2.0;
          angle = f*angle;
          double r = sqrt(x*x + y*y);
          double radius = (s->angle > 0.0)?s->radius-((double)(s->lanes.size()))*map->lane_width:s->radius;
          if (r > radius-PZONE*SIDE_WIDTH && r < radius + ((double)(s->lanes.size()))*map->lane_width + PZONE*SIDE_WIDTH && angle < fabs(s->angle)+0.01 && angle > -0.01) {
            terrain[i][j] = -0.1;
          }
        }
      }
    }
  }

  // Generate the random terrain: We use the Fault Algorithm: http://www.lighthouse3d.com/opengl/terrain/
  double w = terrain_xmax-terrain_xmin;
  double h = terrain_ymax-terrain_ymin;
  for (int i = 0; i < fault_iterations; i++) {
    // Get 2 random point
    double x1 = random_uniform()*w + terrain_xmin;
    double x2 = random_uniform()*w + terrain_xmin;
    double y1 = random_uniform()*h + terrain_ymin;
    double y2 = random_uniform()*h + terrain_ymin;

    double a = y2-y1;
    double b = -(x2-x1);
    double c = -x1*a - y1*b;
    double f = 1.0/sqrt(a*a+b*b);

    double dz = DZ_0 + ((double)i/(double)n)*(DZ_N - DZ_0);
    if (i > n) dz = DZ_N;

    for (unsigned int i=0; i < TERRAIN_RESOLUTION; i++) {
      double x = GET_TERRAIN_X(i);
      for (unsigned int j=0; j < TERRAIN_RESOLUTION; j++) {
        if (terrain[i][j] == -0.1) continue;

        double y = GET_TERRAIN_Y(j);
        double dist = (a*x + b*y - c)*f;

        if (dist < -D || dist > D)
          terrain[i][j] -= dz;
        else
          terrain[i][j] += cos(dist/D*M_PI)*dz;
      }
    }
  }

  // Level-up
  double zt = 0.0;
  int cnt = 0;
  for (unsigned int i=0; i < TERRAIN_RESOLUTION; i++) {
    for (unsigned int j=0; j < TERRAIN_RESOLUTION; j++) {
      if (terrain[i][j] == -0.1) continue;
      zt += terrain[i][j];
      cnt++;
    }
  }
  zt /= (double)(cnt);
  for (unsigned int i=0; i < TERRAIN_RESOLUTION; i++) {
    for (unsigned int j=0; j < TERRAIN_RESOLUTION; j++) {
      if (terrain[i][j] == -0.1) continue;
      terrain[i][j] -= zt;
    }
  }

  // Filter
  double F[FILTER_SIZE][FILTER_SIZE];
  for (int i=-FILTER_SIZE/2; i < FILTER_SIZE/2+1; i++) {
    for (int j=-FILTER_SIZE/2; j < FILTER_SIZE/2+1; j++) {
      F[i+FILTER_SIZE/2][j+FILTER_SIZE/2] = 1.0;
    }
  }

  for (int i = 0; i < FILTER_NPASS; i++) {
    for (int j = FILTER_SIZE/2; j < TERRAIN_RESOLUTION-FILTER_SIZE/2-1; j++) {
      for (int k = FILTER_SIZE/2; k < TERRAIN_RESOLUTION-FILTER_SIZE/2-1; k++) {
        double v = 0.0;
        double s = 0.0;
        if (terrain[j][k] == -0.1) continue;
        for (int l=-FILTER_SIZE/2; l < FILTER_SIZE/2+1; l++) {
          for (int m=-FILTER_SIZE/2; m < FILTER_SIZE/2+1; m++) {
            if (terrain[j+l][k+m] == -0.1) {
              v += terrain[j+l][k+m]*10.0*F[l+FILTER_SIZE/2][m+FILTER_SIZE/2];
              s += 10.0*F[l+FILTER_SIZE/2][m+FILTER_SIZE/2];
            } else {
              v += terrain[j+l][k+m]*F[l+FILTER_SIZE/2][m+FILTER_SIZE/2];
              s += F[l+FILTER_SIZE/2][m+FILTER_SIZE/2];
            }
          }
        }
        terrain[j][k] = v/s;
      }
    }
  }
}

double RealisticDrawer::random_uniform()
{
  return (double)rand()/(double)RAND_MAX;
}

void RealisticDrawer::setWeather(int w)
{
  this->weather = w;
}
