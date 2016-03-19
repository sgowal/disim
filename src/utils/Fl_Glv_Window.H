#ifndef FL_GLV_WINDOW_H
#define FL_GLV_WINDOW_H

#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_Gl_Window.H>

#ifdef MAC
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

typedef enum {IDLE, ZOOM, ROTATE, TRANSLATE} MOUSE_MODE;
typedef enum {FREE, FOLLOW} CAMERA_MODE;

#define ROTATE_SPEED    0.01
#define ZOOM_SPEED      0.1
#define TRANSLATE_SPEED 0.5

/**
 * @brief The opengl display class.
 * 
 * This class extends the Fl_Gl_Window from FLTK (version 1.1) to
 * additional functionalities such as easy camera setup, tracking and stopping the OpenGL display.
 *
 * @author Sven Gowal (svenadrian.gowal@epfl.ch) - Idea borrowed from Andrew Howard.
 */
class Fl_Glv_Window : public Fl_Gl_Window {

public:
  /**
   * The contructor.
   * @param x,y The position of the window on the screen.
   * @param w,h The size of the window.
   * @param draw The function called when OpenGL can draw on the screen.
   */
  Fl_Glv_Window(int x, int y, int w, int h, void *data, Fl_Callback *draw);

  /**
   * The constructor.
   * @param x,y The position of the window on the screen.
   * @param w,h The size of the window.
   * @param draw The function called when OpenGL can draw on the screen.
   * @param predraw The function called when OpenGL can draw (this will be called before draw).
   */
  Fl_Glv_Window(int x, int y, int w, int h, void *data, Fl_Callback *predraw, Fl_Callback *draw);

  /**
   * Sets the field of view.
   * @param hfov The field of view.
   */
  void set_hfov(float hfov);

  /**
   * Sets the clipping planes.
   * @param near The near clipping plane distance.
   * @param far The far clipping plane distance.
   */
  void set_clip(float near, float far);

  /**
   * Sets following variables.
   * The camera will automatically follow the movement of the x,y,z,a variables.
   * @param x,y,z Pointers to the x,y,z coordinates.
   * @param a Pointer to the orientation of the followed object.
   */
  void set_follow_coords(double *x, double *y, double *z, double *a);

  /**
   * Set the camera position.
   * @param x,y,z The position of the point.
   * @param dist The distance to that point.
   * @param phi,alpha The angles from the object.
   */
  void set_camera(double x, double y, double z, double dist, double phi, double alpha);

  /**
   * Gets the camera position variables.
   * @see set_camera()
   */
  void get_camera(double *x, double *y, double *z, double *dist, double *phi, double *alpha);

  /**
   * Stops the tracking mode.
   * @see set_follow_coords()
   */
  void set_camera_free();

  /**
   * Starts the tracking mode.
   */
  void set_camera_follow();

  /**
   * Returns whether the camera is in tracking mode.
   */
  bool is_camera_follow();

  /**
   * Returns whether the camera is not tracking.
   * This is equivalent to !is_camera_follow().
   */
  bool is_camera_free();

  /**
   * Gets the latest mouse click coordinates in 3D space.
   * It intersects the ray from the camera with the z=0 plane.
   * @param x,y,z Pointers to the x,y,z variables where the position of the mouse click can be stored.
   * @return 0 on success and -1 on failure.
   */
  int get_3d_mouse_coords(double *x, double *y, double *z);

  /**
   * Sets the keyboard key combination to hit before clicking the mouse.
   * @param mask The combination of keys.
   */
  void set_3d_mouse_mask(unsigned int mask);

  /**
   * Gets the camera center of attention
   * @param x,y,z Pointers to the x,y,z variables where the position can be stored.
   */
  void get_viewer_coords(double *x, double *y, double *z);

  /**
   * Silences inputs and outputs of the OpenGL window.
   */
  void mute();

  /**
   * The opposite of mute.
   */
  void unmute();

  /**
   * Check whether the OpenGL context has changed, in which
   * case the list, textures, ... should be reloaded.
   */
  bool has_changed_context();

  /**
   * Changes the clear color of OpenGL
   * @param r,g,b The three color channels
   */
  void setClearColor(double r, double g, double b);
  
protected:

  // Draw the window
  virtual void draw();
  
  // Handle events
  virtual int handle(int event);

  /**
   * Sets the 3D click position from the coordinates on the screen where the mouse clicked.
   */
  void set_3d_click(double x, double y);

  /**
   * Updates the camera position.
   */
  void update_viewer();

private:

  // Callback functions
  void *data;
  Fl_Callback *predraw_callback;
  Fl_Callback *draw_callback;

  // mouse
  MOUSE_MODE mouse_mode;
  double mouse_speedup;
  int mouse_x;
  int mouse_y;
  int mouse_start_x;
  int mouse_start_y;
  bool click_set;
  double mouse_3d_x;
  double mouse_3d_y;
  double mouse_3d_z;
  unsigned int mouse_click_mask;

  // camera
  GLfloat viewer_x;
  GLfloat viewer_y;
  GLfloat viewer_z;
  GLfloat viewer_dist;
  GLfloat viewer_phi;       /* angle from ground (x-z plane) */
  GLfloat viewer_alpha;     /* angle from center's x-y plane */
  GLfloat viewer_center_x;
  GLfloat viewer_center_y;
  GLfloat viewer_center_z;
  GLfloat viewer_center_yaw;

  CAMERA_MODE camera_mode;
  double *follow_x;
  double *follow_y;
  double *follow_z;
  double *follow_yaw;

  // Field of view
  float hfov;
  
  // Near and far clipping distances
  float near_clip, far_clip;

  // Mute input
  bool muted;
  bool gl_context_changed;

  // Clear color
  double clear_r, clear_g, clear_b;
};


#endif
