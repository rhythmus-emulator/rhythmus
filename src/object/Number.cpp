#include "Number.h"
#include "Script.h"

namespace rhythmus
{

// ---------------------- class NumberInterface

NumberFormatter::NumberFormatter()
  : number_(0), disp_number_(0),
    number_change_duration_(0), number_change_remain_(0), need_update_(true),
    number_format_("%d")
{
  *num_str_ = 0;
}

void NumberFormatter::SetNumber(int number)
{
  number_ = (double)number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
}

void NumberFormatter::SetNumber(double number) {
  number_ = number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
}

template <> int NumberFormatter::GetNumber() const { return (int)number_; }
template <> double NumberFormatter::GetNumber() const { return number_; }
void NumberFormatter::SetNumberChangeTime(float msec)
{
  number_change_duration_ = (float)msec;
}

void NumberFormatter::SetFormat(const std::string& number_format)
{
  number_format_ = number_format;
}

bool NumberFormatter::UpdateNumber(float delta)
{
  if (need_update_)
  {
    number_change_remain_ -= delta;
    if (number_change_remain_ <= 0)
      need_update_ = false;

    char* out_str = num_str_;
    const char* fmt_str = number_format_.c_str();
    if (*fmt_str == '+')
    {
      if (disp_number_ >= 0)
      {
        *out_str = '+';
        ++out_str;
      }
      fmt_str++;
    }
    *out_str = '\0';
    char last = number_format_.back();
    if (last == 'd')
      sprintf(out_str, fmt_str, (int)disp_number_);
    else if (last == 'f')
      sprintf(out_str, fmt_str, disp_number_);

    return true;
  }
  return false;
}

const char* NumberFormatter::numstr() const
{
  return num_str_;
}


// ------------------------- class NumberSprite

NumberSprite::NumberSprite() : align_(0), data_index_(0), tvi_glyphs_(nullptr)
{
}

NumberSprite::~NumberSprite()
{
  if (tvi_glyphs_)
    free(tvi_glyphs_);
}

void NumberSprite::SetText(const std::string &num)
{
  if (!tvi_glyphs_)
    return;

  TextVertexInfo tvi;
  size_t gidx;
  float x = 0;
  tvi.texid = 0; /* unused */
  tvi.vi[0].a = tvi.vi[0].r = tvi.vi[0].g = tvi.vi[0].b = 1.0f;
  tvi.vi[1].a = tvi.vi[1].r = tvi.vi[1].g = tvi.vi[1].b = 1.0f;
  tvi.vi[2].a = tvi.vi[2].r = tvi.vi[2].g = tvi.vi[2].b = 1.0f;
  tvi.vi[3].a = tvi.vi[3].r = tvi.vi[3].g = tvi.vi[3].b = 1.0f;

  tvi_.clear();
  for (size_t i = 0; i < num.size(); ++i)
  {
    if (num[i] >= '0' && num[i] <= '9')
      gidx = num[i] - '0';
    if (num[i] == '-' || num[i] == '+')
      gidx = 12;
    else if (num[i] == 'O') /* transparent zero */
      gidx = 11;

    // TODO: cycle property

    tvi_.push_back(tvi_glyphs_[gidx]);
    auto *vi = tvi_.back().vi;
    vi[0].x += x;
    vi[1].x += x;
    vi[2].x += x;
    vi[3].x += x;
    x += vi[1].x - vi[0].x;
  }
}

void NumberSprite::SetNumber(int number)
{
  formatter_.SetNumber(number);
  SetText(formatter_.numstr());
}

void NumberSprite::SetNumber(double number)
{
  formatter_.SetNumber(number);
  SetText(formatter_.numstr());
}

void NumberSprite::Refresh()
{
  SetNumber(Script::getInstance().GetNumber(GetResourceId()));
}

NumberFormatter &NumberSprite::GetFormatter()
{
  return formatter_;
}

void NumberSprite::SetTextTableIndex(size_t idx)
{
  data_index_ = idx;
}

void NumberSprite::SetLR2Alignment(int type)
{
  align_ = type;
}

void NumberSprite::Load(const Metric& metric)
{
  Sprite::Load(metric);
}

void NumberSprite::LoadFromLR2SRC(const std::string &cmd)
{
  Sprite::LoadFromLR2SRC(cmd);
  if (!img_)
    return;

  // TODO: LR2 sprite font implementation
  // (image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align)
  CommandArgs args(cmd);

  /* TODO: create LR2SpriteFont and add Update() method to Font ? */

  /* register glyphs by divx / divy. */
  int cycle_count = 0;
  int dx, dy;
  float divw = (float)source_width / divx_;
  float divh = (float)source_height / divy_;
  float dsw = sw_ / divx_;
  float dsh = sh_ / divy_;
  if (cnt_ % 10 == 0)
  {
    cycle_count = cnt_ / 10;
    tvi_glyphs_ = (TextVertexInfo*)calloc(1, sizeof(TextVertexInfo) * 24 * cycle_count);
    for (int i = 0; i < cycle_count; ++i)
    {
      for (int j = 0; j < 10; ++j)
      {
        const int ii = i * 24 + j;
        dx = j % divx_;
        dy = j / divx_;
        tvi_glyphs_[ii].texid = img_->get_texture_ID();
        tvi_glyphs_[ii].vi[0].a = tvi_glyphs_[i].vi[1].a
          = tvi_glyphs_[i].vi[2].a = tvi_glyphs_[i].vi[3].a = 1.0f;
        tvi_glyphs_[ii].vi[0].r = tvi_glyphs_[i].vi[1].r
          = tvi_glyphs_[i].vi[2].r = tvi_glyphs_[i].vi[3].r = 1.0f;
        tvi_glyphs_[ii].vi[0].g = tvi_glyphs_[i].vi[1].g
          = tvi_glyphs_[i].vi[2].g = tvi_glyphs_[i].vi[3].g = 1.0f;
        tvi_glyphs_[ii].vi[0].b = tvi_glyphs_[i].vi[1].b
          = tvi_glyphs_[i].vi[2].b = tvi_glyphs_[i].vi[3].b = 1.0f;
        tvi_glyphs_[ii].vi[0].sx = sx_ + dsw * dx;
        tvi_glyphs_[ii].vi[0].sy = sy_ + dsh * dy;
        tvi_glyphs_[ii].vi[1].sx = tvi_glyphs_[i].vi[0].sx + dsw;
        tvi_glyphs_[ii].vi[1].sy = tvi_glyphs_[i].vi[0].sy;
        tvi_glyphs_[ii].vi[2].sx = tvi_glyphs_[i].vi[1].sx;
        tvi_glyphs_[ii].vi[2].sy = tvi_glyphs_[i].vi[1].sy + dsh;
        tvi_glyphs_[ii].vi[3].sx = tvi_glyphs_[i].vi[0].sx;
        tvi_glyphs_[ii].vi[3].sy = tvi_glyphs_[i].vi[0].sy + dsh;
        tvi_glyphs_[ii].vi[0].x = 0;
        tvi_glyphs_[ii].vi[0].y = 0;
        tvi_glyphs_[ii].vi[1].x = divw;
        tvi_glyphs_[ii].vi[1].y = 0;
        tvi_glyphs_[ii].vi[2].x = divw;
        tvi_glyphs_[ii].vi[2].y = divh;
        tvi_glyphs_[ii].vi[3].x = 0;
        tvi_glyphs_[ii].vi[3].y = divh;
      }
    }
  }
  else if (cnt_ % 11 == 0)
  {
    cycle_count = cnt_ / 11;
    tvi_glyphs_ = (TextVertexInfo*)calloc(1, sizeof(TextVertexInfo) * 24 * cycle_count);
    // TODO: fill glyphs
  }
  else if (cnt_ % 24 == 0)
  {
    cycle_count = cnt_ / 24;
    tvi_glyphs_ = (TextVertexInfo*)calloc(1, sizeof(TextVertexInfo) * 24 * cycle_count);
    // TODO: fill glyphs
  }
  else return;

  /* track change of number table */
  std::string eventname = args.Get<std::string>(9);
  AddCommand(eventname, "refresh");
  SubscribeTo(eventname);

  /* set number value index instantly */
  SetResourceId(atoi(eventname.c_str()));
  Refresh();

  /* alignment */
  SetLR2Alignment(args.Get<int>(10));
}

void NumberSprite::doUpdate(float delta)
{
  if (formatter_.UpdateNumber(delta))
    SetText(formatter_.numstr());
  Sprite::doUpdate(delta);
}

void NumberSprite::doRender()
{
  // If hide, then not draw
  if (!img_ || !IsVisible())
    return;

  /* we know what sprite to use, so just call SetTextureId once. */
  Graphic::SetTextureId(image()->get_texture_ID());
  for (const auto &tvi : tvi_)
  {
    memcpy(Graphic::get_vertex_buffer(),
      tvi.vi,
      sizeof(VertexInfo) * 4);
  }
}


// --------------------------- class NumberText

NumberText::NumberText() {}

void NumberText::SetNumber(int number)
{
  formatter_.SetNumber(number);
  SetText(formatter_.numstr());
}

void NumberText::SetNumber(double number)
{
  formatter_.SetNumber(number);
  SetText(formatter_.numstr());
}

NumberFormatter &NumberText::GetFormatter()
{
  return formatter_;
}

void NumberText::doUpdate(float delta)
{
  if (formatter_.UpdateNumber(delta))
    SetText(formatter_.numstr());
  Text::doUpdate(delta);
}

void NumberText::Load(const Metric& metric)
{
  Text::Load(metric);
  if (metric.exist("sprite"))
  {
    LoadFromLR2SRC(metric.get<std::string>("sprite"));
  }
  if (metric.exist("number"))
    SetText(metric.get<std::string>("number"));
}

void NumberText::Refresh()
{
  SetNumber(Script::getInstance().GetNumber(GetResourceId()));
}

void NumberText::LoadFromLR2SRC(const std::string &cmd)
{
  // TODO: LR2 sprite font implementation
  // (image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align)
  CommandArgs args(cmd);

  /* track change of number table */
  std::string eventname = args.Get<std::string>(9);
  AddCommand(eventname, "refresh");
  SubscribeTo(eventname);
  /* set value instantly */
  SetResourceId(atoi(eventname.c_str()));
  Refresh();

  /* alignment */
  SetLR2Alignment(args.Get<int>(10));
}

}