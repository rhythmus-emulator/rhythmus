#include "Dialog.h"
#include "common.h"

namespace rhythmus
{

Dialog::Dialog()
{
  memset(src_rect_, 0, sizeof(src_rect_));
}

Dialog::~Dialog() {}

void StringToRect(const std::string &s, Vector4 &out)
{
  CommandArgs a(s, 4);
  out = Vector4{ a.Get<int>(0), a.Get<int>(1), a.Get<int>(2), a.Get<int>(3) };
}

void Dialog::Load(const MetricGroup &metric)
{
  static const char* _attrs[] = {
    "tl", "t", "tr", "l", "body", "r", "bl", "b", "br"
  };

  BaseObject::Load(metric);
  std::string src = metric.get<std::string>("src");
  for (size_t i = 0; i < 9; ++i)
  {
    StringToRect(metric.get<std::string>(_attrs[i]), src_rect_[i]);
    sprites_[i].SetImage(src);
    sprites_[i].SetImageCoord(src_rect_[i]);
  }

  title_.set_name(get_name() + "_title");
  text_.set_name(get_name() + "_text");
  title_.SetPos(src_rect_[0].z, src_rect_[0].w);
  text_.SetPos(src_rect_[0].z, src_rect_[0].w + 50);

  const MetricGroup *title_m = metric.get_group("title");
  if (title_m) title_.Load(*title_m);
  const MetricGroup *text_m = metric.get_group("text");
  if (text_m) text_.Load(*text_m);

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
#if 0
  // do later
  int body_w = GetCurrentFrame().w - src_rect_[0].width() - src_rect_[2].width();
  int body_h = current_prop_.h - src_rect_[0].height() - src_rect_[6].height();
  if (body_w < 0) body_w = 0;
  if (body_h < 0) body_h = 0;
  src_rect_[1].set_width(body_w);
  src_rect_[4].set_width(body_w);
  src_rect_[7].set_width(body_w);
  src_rect_[3].set_height(body_h);
  src_rect_[4].set_height(body_h);
  src_rect_[5].set_height(body_h);
  int xpos, ypos=0;
  for (int i = 0; i < 3; ++i)
  {
    xpos = 0;
    for (int j = 0; j < 3; ++j)
    {
      const int idx = i * 3 + j;
      const Rect &_r = src_rect_[idx];
      sprites_[idx].SetPos(xpos, ypos);
      sprites_[idx].SetSize(_r.width(), _r.height());
      xpos += _r.width();
    }
    ypos += src_rect_[i * 3].height();
  }
#endif
}

}
