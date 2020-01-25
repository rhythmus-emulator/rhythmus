#include "Dialog.h"

namespace rhythmus
{

Dialog::Dialog()
{
  memset(src_rect_, 0, sizeof(src_rect_));
}

Dialog::~Dialog() {}

void StringToRect(const std::string &s, Rect &out)
{
  CommandArgs a(s, 4);
  out.x = a.Get<int>(0);
  out.y = a.Get<int>(1);
  out.w = a.Get<int>(2);
  out.h = a.Get<int>(3);
}

void Dialog::Load(const Metric &metric)
{
  static const char* _attrs[] = {
    "tl", "t", "tr", "l", "body", "r", "bl", "b", "br"
  };

  BaseObject::Load(metric);
  std::string src = metric.get<std::string>("src");
  for (size_t i = 0; i < 9; ++i)
  {
    StringToRect(metric.get<std::string>(_attrs[i]), src_rect_[i]);
    sprites_[i].SetImageByPath(src);
    sprites_[i].SetImageCoordRect(src_rect_[i]);
  }

  title_.set_name(get_name() + "_title");
  text_.set_name(get_name() + "_text");
  title_.SetPos(src_rect_[0].x + src_rect_[0].w, src_rect_[0].y + src_rect_[0].h);
  text_.SetPos(src_rect_[0].x + src_rect_[0].w, src_rect_[0].y + src_rect_[0].h + 50);
  title_.LoadByName();
  text_.LoadByName();

  for (size_t i = 0; i < 9; ++i)
    AddChild(&sprites_[i]);
  AddChild(&title_);
  AddChild(&text_);
}

void Dialog::SetTitle(const std::string &text)
{
  title_.SetText(text);
}

void Dialog::SetText(const std::string &text)
{
  text_.SetText(text);
}

void Dialog::doUpdate(float delta)
{
  BaseObject::doUpdate(delta);
  int body_w = current_prop_.w - src_rect_[0].w - src_rect_[2].w;
  int body_h = current_prop_.h - src_rect_[0].h - src_rect_[6].h;
  src_rect_[1].w = src_rect_[4].w = src_rect_[7].w = body_w;
  src_rect_[4].h = src_rect_[5].h = src_rect_[6].h = body_h;
  int xpos, ypos=0;
  for (int i = 0; i < 3; ++i)
  {
    xpos = 0;
    for (int j = 0; j < 3; ++i)
    {
      const int idx = i * 3 + j;
      const Rect &_r = src_rect_[idx];
      sprites_[idx].SetPos(_r.x, _r.y);
      sprites_[idx].SetSize(_r.w, _r.h);
      xpos += _r.w;
    }
    ypos += src_rect_[i * 3].h;
  }
}

}