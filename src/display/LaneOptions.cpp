#include "LaneOptions.h"

LaneOptions::LaneOptions(Lane *l) : Fl_Window(365,30,"Lane Options")
{
  char title[255];
  sprintf(title, "Lane Options (%s)", l->name);

  this->lane = l;

  label(title);

  tipwin = new TipWindow();
  tipwin->hide();

  begin();
  textlabel = new Fl_Output(10,5,130,20);
  textlabel->value("Entry rate [veh/h]:");
  textlabel->box(FL_NO_BOX);
  rateSlider = new TipValueSlider(tipwin, 150,5,200,20);
  rateSlider->minimum(0.0);
  rateSlider->maximum(3000.0);
  rateSlider->callback(LaneOptions::onSlide, this);
  if (l && l->type == ENTRY) {
    rateSlider->value(l->entry_rate*3600.0);
  } else {
    rateSlider->deactivate();
  }
  end();

  show();
}

LaneOptions::~LaneOptions()
{
  hide();
  lane = NULL;

  delete rateSlider;
  rateSlider = NULL;
  delete textlabel;
  textlabel = NULL;

  tipwin->hide();
  delete tipwin;
}

void LaneOptions::setLane(Lane *l)
{
  this->lane = l;

  char title[255];
  sprintf(title, "Lane Options (%s)", l->name);
  label(title);

  if (l && l->type == ENTRY) {
    rateSlider->activate();
    rateSlider->value(l->entry_rate*3600.0);
    show();
  } else {
    rateSlider->deactivate();
  }
}

Lane *LaneOptions::getLane()
{
  return lane;
}

void LaneOptions::update()
{
  if (lane && lane->type == ENTRY) {
    rateSlider->value(lane->entry_rate*3600.0);
    rateSlider->redraw();
  }
}

void LaneOptions::onSlide(Fl_Widget *w, void *data)
{
  ((LaneOptions *)data)->tipwin->value(((LaneOptions *)data)->rateSlider->value());
  ((LaneOptions *)data)->tipwin->position(Fl::event_x_root(), Fl::event_y_root()+20);
  ((LaneOptions *)data)->lane->entry_rate = ((LaneOptions *)data)->rateSlider->value()/3600.0;
}
