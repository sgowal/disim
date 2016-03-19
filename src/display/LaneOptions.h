#ifndef LANE_OPTIONS_H
#define LANE_OPTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Output.H>

#include <map/Map.h>

using namespace std;

/* Floating tip window.
 * This displays a value on a yellow background.
 * Code taken from: http://seriss.com/people/erco/fltk/#SliderTooltip
 */
class TipWindow : public Fl_Menu_Window {
  char tip[40];
public:
  TipWindow():Fl_Menu_Window(1,1)
  {
    strcpy(tip, "X.XX");
    set_override();
    end();
  }
  void draw()
  {
    draw_box(FL_BORDER_BOX, 0, 0, w(), h(), Fl_Color(175));
    fl_color(FL_BLACK);
    fl_font(labelfont(), labelsize());
    fl_draw(tip, 3, 3, w()-6, h()-6, Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_WRAP));
  }
  void value(float f)
  {
    sprintf(tip, "%.0f", f);
    // Recalc size of window
    fl_font(labelfont(), labelsize());
    int W = w(), H = h();
    fl_measure(tip, W, H, 0);
    W += 8;
    size(W, H);
    redraw();
  }
};

/* Slider tied with a TipWindow to
 * display its value on the TipWindow.
 */
class TipValueSlider : public Fl_Slider {
  TipWindow *tipwin;

 public:
  TipValueSlider(TipWindow *tipwin, int x, int y, int w, int h, const char*l=0) : Fl_Slider(x,y,w,h,l)
  {
    this->tipwin = tipwin;
    type(FL_HOR_SLIDER);
    Fl_Group *save = Fl_Group::current();   // save current widget..
    Fl_Group::current(save);                // ..then back to previous.
  }

  int handle(int e) {
    switch(e) {
    case FL_PUSH:
      // XXX: if offscreen, move tip ABOVE mouse instead
      tipwin->position(Fl::event_x_root(), Fl::event_y_root()+20);
      tipwin->value(value());
      tipwin->show();
      break;
    case FL_HIDE:       // valuator goes away
    case FL_RELEASE:    // release mouse
    case FL_LEAVE:      // leave focus
      // Make sure tipwin closes when app closes
      tipwin->hide();
      break;
    }
    return(Fl_Slider::handle(e));
  }
};

class LaneOptions : public Fl_Window {
 public:
  LaneOptions(Lane *l);
  ~LaneOptions();
  void setLane(Lane *l);
  void update();
  Lane *getLane();

 private:
  TipWindow *tipwin;
  TipValueSlider *rateSlider;
  Fl_Output *textlabel;
  Lane *lane;
  static void onSlide(Fl_Widget *w, void *data);
};

#endif
