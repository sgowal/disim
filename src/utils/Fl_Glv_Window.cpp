/* 
 * Desc: An FLTK widget for doing GL stuff
 * Date: 19 October 2005
 * Author: Andrew Howard
 * CVS: $Id: Fl_Glv_Window.cc,v 1.16 2007/05/05 00:57:59 abhoward Exp $
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "Fl_Glv_Window.H"


// Constructor
Fl_Glv_Window::Fl_Glv_Window(int x, int y, int w, int h, void *data, Fl_Callback *draw)
    : Fl_Gl_Window(x, y, w, h, NULL)
{
  this->data = data;
  this->predraw_callback = NULL;
  this->draw_callback = draw;

  this->hfov = 30;
  this->near_clip = 0.3;
  this->far_clip = 1500.0;

  this->viewer_x = 0.0;
  this->viewer_y = 0.0;
  this->viewer_z = 0.0;
  this->viewer_dist = 300.0;
  this->viewer_phi = M_PI/4.0;  /* angle from ground (x-z plane) */
  this->viewer_alpha = 0.0;     /* angle from center's x-y plane */
  this->viewer_center_x = 0.0;
  this->viewer_center_y = 0.0;
  this->viewer_center_z = 0.0;
  this->viewer_center_yaw = 0.0;

  this->mouse_mode = IDLE;
  this->mouse_speedup = 1.0;
  this->mouse_x = 0;
  this->mouse_y = 0;
  this->mouse_start_x = 0;
  this->mouse_start_y = 0;
  this->click_set = false;
  this->mouse_3d_x = 0.0;
  this->mouse_3d_y = 0.0;
  this->mouse_3d_z = 0.0;
  this->mouse_click_mask = -1;

  this->camera_mode = FREE;
  this->follow_x = NULL;
  this->follow_y = NULL;
  this->follow_z = NULL;
  this->follow_yaw = NULL;

  this->muted = false;
  this->gl_context_changed = false;

  this->clear_r = 0.0;
  this->clear_g = 0.0;
  this->clear_b = 0.0;

  return;
}


// Constructor
Fl_Glv_Window::Fl_Glv_Window(int x, int y, int w, int h, void *data,
                             Fl_Callback *predraw, Fl_Callback *draw)
    : Fl_Gl_Window(x, y, w, h, NULL)
{
  this->data = data;
  this->predraw_callback = predraw;
  this->draw_callback = draw;

  this->hfov = 30;
  this->near_clip = 0.3;
  this->far_clip = 1500.0;

  this->viewer_x = 0.0;
  this->viewer_y = 0.0;
  this->viewer_z = 0.0;
  this->viewer_dist = 300.0;
  this->viewer_phi = M_PI/4.0;  /* angle from ground (x-z plane) */
  this->viewer_alpha = 0.0;     /* angle from center's x-y plane */
  this->viewer_center_x = 0.0;
  this->viewer_center_y = 0.0;
  this->viewer_center_z = 0.0;
  this->viewer_center_yaw = 0.0;

  this->mouse_mode = IDLE;
  this->mouse_speedup = 1.0;
  this->mouse_x = 0;
  this->mouse_y = 0;
  this->mouse_start_x = 0;
  this->mouse_start_y = 0;
  this->click_set = false;
  this->mouse_3d_x = 0.0;
  this->mouse_3d_y = 0.0;
  this->mouse_3d_z = 0.0;
  this->mouse_click_mask = -1;

  this->camera_mode = FREE;
  this->follow_x = NULL;
  this->follow_y = NULL;
  this->follow_z = NULL;
  this->follow_yaw = NULL;

  this->muted = false;
  this->gl_context_changed = false;

  this->clear_r = 0.0;
  this->clear_g = 0.0;
  this->clear_b = 0.0;
  
  return;
}

// Set the horizontal field-of-view
void Fl_Glv_Window::set_hfov(float hfov)
{
  this->hfov = hfov;
  return;
}


// Set the clipping planes
void Fl_Glv_Window::set_clip(float near, float far)
{
  this->near_clip = near;
  this->far_clip = far;
  return;
}

void Fl_Glv_Window::set_follow_coords(double *x, double *y, double *z, double *a)
{
  this->follow_x = x;
  this->follow_y = y;
  this->follow_z = z;
  this->follow_yaw = a;
  return;
}

void Fl_Glv_Window::set_camera(double x, double y, double z, double dist, double phi, double alpha)
{
  this->viewer_center_x = x;
  this->viewer_center_y = y;
  this->viewer_center_z = z;
  this->viewer_dist = dist;
  this->viewer_phi = phi;
  this->viewer_alpha = alpha;
}

void Fl_Glv_Window::get_camera(double *x, double *y, double *z, double *dist, double *phi, double *alpha)
{
  *x = this->viewer_x;
  *y = this->viewer_y;
  *z = this->viewer_z;
  *dist = this->viewer_dist;
  *phi = this->viewer_phi;
  *alpha = this->viewer_alpha;
}

void Fl_Glv_Window::set_camera_free()
{
  this->camera_mode = FREE;
}

void Fl_Glv_Window::set_camera_follow()
{
  this->camera_mode = FOLLOW;
}

bool Fl_Glv_Window::is_camera_free()
{
  return this->camera_mode == FREE;
}

bool Fl_Glv_Window::is_camera_follow()
{
  return this->camera_mode == FOLLOW;
}

int Fl_Glv_Window::get_3d_mouse_coords(double *x, double *y, double *z)
{
  if (x && y && z && click_set) {
    *x = mouse_3d_x;
    *y = mouse_3d_y;
    *z = mouse_3d_z;
    return 0;
  }

  return -1;
}

void Fl_Glv_Window::set_3d_mouse_mask(unsigned int mask)
{
  mouse_click_mask = mask;
}

void Fl_Glv_Window::get_viewer_coords(double *x, double *y, double *z)
{
  if (x && y && z) {
    *x = viewer_center_x;
    *y = viewer_center_y;
    *z = viewer_center_z;
  }
  return;
}

void Fl_Glv_Window::mute()
{
  muted = true;
  this->hide();
  
  return;
}

void Fl_Glv_Window::unmute()
{
  muted = false;
  this->show();
  this->gl_context_changed = true;

  return;
}

bool Fl_Glv_Window::has_changed_context()
{
  bool v = this->gl_context_changed;
  this->gl_context_changed = false;
  return v;
}

void Fl_Glv_Window::setClearColor(double r, double g, double b)
{
  this->clear_r = r;
  this->clear_g = g;
  this->clear_b = b;
}

// Draw the window
void Fl_Glv_Window::draw()
{
  // Make sure we are using the correct context
  this->make_current();

  if (!this->valid()) {
    // Use the whole window as the context
    glViewport(0, 0, this->w(), this->h());
  }
  glClearColor(this->clear_r, this->clear_g, this->clear_b, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_POINT_SMOOTH);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_COLOR_MATERIAL);

  if (this->muted) return;

  if (camera_mode == FOLLOW && follow_x && follow_y && follow_z && follow_yaw) {
    viewer_center_x = *follow_x;
    viewer_center_y = *follow_y;
    viewer_center_z = *follow_z;
    viewer_center_yaw = *follow_yaw;
  }
  update_viewer();

  if (this->predraw_callback)
    (*this->predraw_callback) (this, this->data);

  if (this->draw_callback) {
    (*this->draw_callback) (this, this->data);
    return;
  }

  return;
};


// Handle events
int Fl_Glv_Window::handle(int event)
{
  int button = Fl::event_button();
  double x = Fl::event_x();
  double y = Fl::event_y();
  double dx = 0.0, dy = 0.0;
  GLfloat dx_x, dx_z, dy_x, dy_z;
  bool d_override = false;

  if (this->muted) return 1;

  switch (event) {
    case FL_PUSH:
      // Check modifier buttons to emulate 3-button mouse on Mac.
      if (Fl::event_state() & FL_ALT)
        button = FL_MIDDLE_MOUSE;
      else if (Fl::event_state() & FL_META)
        button = FL_RIGHT_MOUSE;
      
      mouse_start_x = x;
      mouse_start_y = y;
      mouse_x = x;
      mouse_y = y;

      if (button == FL_MIDDLE_MOUSE) {
        mouse_mode = ZOOM;
      } else if (button == FL_LEFT_MOUSE) {
        if (mouse_mode == TRANSLATE) mouse_mode = ZOOM;
        else mouse_mode = ROTATE; 
      } else {
        if (mouse_mode == ROTATE) mouse_mode = ZOOM;
        else mouse_mode = TRANSLATE;
      }      
      return 1;

    case FL_MOUSEWHEEL:
      mouse_mode = ZOOM;
      dx = Fl::event_dx()*50.0;
      dy = Fl::event_dy()*50.0;
      d_override = true;
    case FL_DRAG:
      if (Fl::event_state() & FL_SHIFT)
        mouse_speedup = 10.0;
      else 
        mouse_speedup = 1.0;

      if (!d_override) {
        dx = x - mouse_x;
        dy = y - mouse_y;
      }

      switch (mouse_mode) {
      case ZOOM:
        viewer_dist += dy*ZOOM_SPEED*mouse_speedup;
        if (viewer_dist < this->near_clip) viewer_dist = this->near_clip;
        if (viewer_dist > this->far_clip) viewer_dist = this->far_clip;
        break;
      case ROTATE:
        viewer_alpha += dx*ROTATE_SPEED*mouse_speedup;
        if (viewer_alpha < -M_PI) viewer_alpha += 2.0*M_PI;
        if (viewer_alpha > M_PI) viewer_alpha -= 2.0*M_PI;
        viewer_phi += dy*ROTATE_SPEED*mouse_speedup;
        if (viewer_phi >= M_PI/2.0) viewer_phi = M_PI/2.0-0.0001; // MAGIC
        if (viewer_phi <= 0.0) viewer_phi = 0.0001; // MAGIC
        break;
      case TRANSLATE:
        if (camera_mode == FREE) {
          dy_x = cos(viewer_phi)*cos(viewer_center_yaw + viewer_alpha);
          dy_z = cos(viewer_phi)*sin(viewer_center_yaw + viewer_alpha);
          dx_x = - dy_z;
          dx_z = dy_x;
          viewer_center_x += (- dx*dx_x + dy*dy_x)*TRANSLATE_SPEED*mouse_speedup;
          viewer_center_z += (- dx*dx_z + dy*dy_z)*TRANSLATE_SPEED*mouse_speedup;
        }

        break;
      default:
        break;
      }

      mouse_x = x;
      mouse_y = y;
      update_viewer();
      redraw();

      if (d_override) {
        mouse_mode = IDLE;
      }
      
      return 1;

    case FL_RELEASE:
      int d = (mouse_start_x-x)*(mouse_start_x-x)+(mouse_start_y-y)*(mouse_start_y-y);
      if (d < 25) { 
        if ((Fl::event_state() & mouse_click_mask) == mouse_click_mask) set_3d_click(x, y);
        else  click_set = false;
      }
      else mouse_mode = IDLE;
      return 1;
  }

  return Fl_Gl_Window::handle(event);
}

void Fl_Glv_Window::set_3d_click(double x, double y)
{
  // Make sure we are using the correct context
  this->make_current();

  GLint viewport[4];
  GLdouble modelview[16];
  GLdouble projection[16];
  // GLfloat z;
  GLdouble p1x, p1y, p1z, p2x, p2y, p2z;

  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  glGetIntegerv(GL_VIEWPORT, viewport);

  y = viewport[3] - y;
  // glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
  // printf("z: %.2f\n", z);
  
  gluUnProject((GLfloat)x, (GLfloat)y, 0.0,
               modelview, projection, viewport,
               &p1x, &p1y, &p1z);

  gluUnProject((GLfloat)x, (GLfloat)y, 1.0,
               modelview, projection, viewport,
               &p2x, &p2y, &p2z);

  /* Now compute intersection with y = 0 */
  if (fabs(p2y - p1y) > 0.000001) {
    double t = -p2y/(p2y - p1y);
    mouse_3d_x = p2x + (p2x - p1x)*t;
    mouse_3d_y = p2y + (p2y - p1y)*t;
    mouse_3d_z = p2z + (p2z - p1z)*t;
    click_set = true;
  }

  return;
}

void Fl_Glv_Window::update_viewer()
{
  GLfloat dir_x, dir_y, dir_z;
  GLfloat upx = 0.0;
  GLfloat upy = 1.0;
  GLfloat upz = 0.0;

  dir_x = viewer_dist*cos(viewer_phi)*cos(viewer_center_yaw + M_PI + viewer_alpha);
  dir_y = viewer_dist*sin(viewer_phi);
  dir_z = viewer_dist*cos(viewer_phi)*sin(viewer_center_yaw + M_PI + viewer_alpha);
  
  viewer_x = viewer_center_x + dir_x;
  viewer_y = viewer_center_y + dir_y;
  viewer_z = viewer_center_z + dir_z;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, this->w(), this->h());
  gluPerspective(this->hfov,
                 (float) this->w() / (float) this->h(), this->near_clip, this->far_clip);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(viewer_x, viewer_y, viewer_z, viewer_center_x, viewer_center_y + 2.0, viewer_center_z, 
            upx, upy, upz);
}
