#include "Number.h"
#include "Script.h"

namespace rhythmus
{

// ---------------------- class NumberInterface

NumberFormatter::NumberFormatter()
  : number_(0), from_number_(0), disp_number_(0),
    number_change_duration_(0), number_change_remain_(0), need_update_(true),
    number_format_("%d")
{
  *num_str_ = 0;
}

void NumberFormatter::SetNumber(int number)
{
  from_number_ = number_;
  number_ = (double)number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
  UpdateNumber(0);
}

void NumberFormatter::SetNumber(double number) {
  from_number_ = number_;
  number_ = number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
  UpdateNumber(0);
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

    if (number_change_duration_ == 0)
      disp_number_ = number_;
    else
      disp_number_ = number_ +
        (from_number_ - number_) * (number_change_remain_ / number_change_duration_);

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


// --------------------------- class NumberText

Number::Number() : tvi_glyphs_(nullptr) {}

Number::~Number() { AllocNumberGlyph(0); }

void Number::SetNumber(int number)
{
  formatter_.SetNumber(number);
  SetText(formatter_.numstr());
}

void Number::SetNumber(double number)
{
  formatter_.SetNumber(number);
  SetText(formatter_.numstr());
}

void Number::SetText(const std::string &num)
{
  if (!tvi_glyphs_)
    return;
  ClearTextVertex();
  if (num.empty())
    return;

  size_t gidx = 0;
  bool is_positive = (num[0] != '-');
  float x = 0;

  for (size_t i = 0; i < num.size(); ++i)
  {
    if (num[i] >= '0' && num[i] <= '9')
      gidx = num[i] - '0';
    else if (num[i] == '-' || num[i] == '+')
      gidx = 11;
    else if (num[i] == 'O') /* transparent zero */
      gidx = 10;
    if (!is_positive)
      gidx += 12;
    if (tvi_glyphs_[gidx].texid == 0)
      continue;

    auto *vi = AddTextVertex(tvi_glyphs_[gidx]).vi;
    vi[0].x += x;
    vi[1].x += x;
    vi[2].x += x;
    vi[3].x += x;
    x += vi[1].x - vi[0].x;
  }

  // TODO: cycle property, add tvi with cycles

  UpdateTextRenderContext();
}

NumberFormatter &Number::GetFormatter()
{
  return formatter_;
}

void Number::doUpdate(float delta)
{
  if (formatter_.UpdateNumber(delta))
    SetText(formatter_.numstr());
  Text::doUpdate(delta);
}

void Number::Load(const Metric& metric)
{
  Text::Load(metric);

  /* Use font if loaded (by Text::Load).
   * If not, use texture if necessary (by Number::LoadFromLR2SRC). */
  if (font())
  {
    CreateTextVertexFromFont();
  }
  else if (numbersprite_.image())
  {
    CreateTextVertexFromSprite();
  }
}

void Number::Refresh()
{
  /* kind of trick to compatible with LR2:
   * if value is UINT_MIX, then set with empty value. */
  int v = Script::getInstance().GetNumber(GetResourceId());
  if (v == 0xFFFFFFFF)
    SetText(std::string());
  else
    SetNumber(v);
}

void Number::AllocNumberGlyph(size_t cycles)
{
  if (tvi_glyphs_)
  {
    free(tvi_glyphs_);
    tvi_glyphs_ = nullptr;
  }
  if (cycles == 0)
    return;
  tvi_glyphs_ = (TextVertexInfo*)calloc(24 * cycles, sizeof(TextVertexInfo));
}

void Number::LoadFromLR2SRC(const std::string &cmd)
{
  // This method only supports number with sprite, not font.
  numbersprite_.LoadFromLR2SRC(cmd);

  // (image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align)
  CommandArgs args(cmd);

  /* track change of number table */
  int eventid = args.Get<int>(9);
  std::string eventname = "Number" + args.Get<std::string>(9);
  AddCommand(eventname, "refresh");
  SubscribeTo(eventname);

  /* set value instantly */
  SetResourceId(eventid);
  Refresh();

  /* alignment (not use LR2 alignment here) */
  switch (args.Get<int>(10))
  {
  case 0:
    SetTextAlignment(TextAlignments::kTextAlignLeft);
    break;
  case 1:
    SetTextAlignment(TextAlignments::kTextAlignRight);
    break;
  case 2:
    SetTextAlignment(TextAlignments::kTextAlignCenter);
    break;
  }

  /* keta processing: tween width multiply & set number formatter */
  int keta = args.Get<int>(11);
  SetWidthMultiply((float)keta);
}

void Number::CreateTextVertexFromSprite()
{
  /* register glyphs by divx / divy. */
  int divx = numbersprite_.GetSpriteAnimationInfo().divx;
  int divy = numbersprite_.GetSpriteAnimationInfo().divy;
  float divw = (float)numbersprite_.GetImageCoordRect().w / divx;
  float divh = (float)numbersprite_.GetImageCoordRect().h / divy;
  int cycle_sprite = numbersprite_.GetSpriteAnimationInfo().cnt;
  int cycle_number = 0;
  if (cycle_sprite % 10 == 0)
  {
    cycle_number = cycle_sprite / 10;
    AllocNumberGlyph(cycle_number);
    for (int i = 0; i < cycle_number; ++i)
    {
      for (int j = 0; j < 10; ++j)
      {
        const int ii = i * 24 + j;
        tvi_glyphs_[ii].texid = numbersprite_.image()->get_texture_ID();
        tvi_glyphs_[ii].vi[0].a = tvi_glyphs_[ii].vi[1].a
          = tvi_glyphs_[ii].vi[2].a = tvi_glyphs_[ii].vi[3].a = 1.0f;
        tvi_glyphs_[ii].vi[0].r = tvi_glyphs_[ii].vi[1].r
          = tvi_glyphs_[ii].vi[2].r = tvi_glyphs_[ii].vi[3].r = 1.0f;
        tvi_glyphs_[ii].vi[0].g = tvi_glyphs_[ii].vi[1].g
          = tvi_glyphs_[ii].vi[2].g = tvi_glyphs_[ii].vi[3].g = 1.0f;
        tvi_glyphs_[ii].vi[0].b = tvi_glyphs_[ii].vi[1].b
          = tvi_glyphs_[ii].vi[2].b = tvi_glyphs_[ii].vi[3].b = 1.0f;
        tvi_glyphs_[ii].vi[0].x = 0;
        tvi_glyphs_[ii].vi[0].y = 0;
        tvi_glyphs_[ii].vi[1].x = divw;
        tvi_glyphs_[ii].vi[1].y = 0;
        tvi_glyphs_[ii].vi[2].x = divw;
        tvi_glyphs_[ii].vi[2].y = divh;
        tvi_glyphs_[ii].vi[3].x = 0;
        tvi_glyphs_[ii].vi[3].y = divh;
        numbersprite_.FillTextureCoordToVertexInfo(tvi_glyphs_[ii].vi, ii);
      }
    }
  }
  else if (cycle_sprite % 11 == 0)
  {
    cycle_number = cycle_sprite / 11;
    AllocNumberGlyph(cycle_number);
    // TODO: fill glyphs
  }
  else if (cycle_sprite % 24 == 0)
  {
    cycle_number = cycle_sprite / 24;
    AllocNumberGlyph(cycle_number);
    // TODO: fill glyphs
  }
  else return;
  SetTextVertexCycle(cycle_number, 0);
}

void Number::CreateTextVertexFromFont()
{
  AllocNumberGlyph(1);
  for (int i = 0; i < 10; ++i)
  {
    auto *g = font()->GetGlyph('0' + i);
    if (!g) continue;

    tvi_glyphs_[i].texid = numbersprite_.image()->get_texture_ID();
    tvi_glyphs_[i].vi[0].a = tvi_glyphs_[i].vi[1].a
      = tvi_glyphs_[i].vi[2].a = tvi_glyphs_[i].vi[3].a = 1.0f;
    tvi_glyphs_[i].vi[0].r = tvi_glyphs_[i].vi[1].r
      = tvi_glyphs_[i].vi[2].r = tvi_glyphs_[i].vi[3].r = 1.0f;
    tvi_glyphs_[i].vi[0].g = tvi_glyphs_[i].vi[1].g
      = tvi_glyphs_[i].vi[2].g = tvi_glyphs_[i].vi[3].g = 1.0f;
    tvi_glyphs_[i].vi[0].b = tvi_glyphs_[i].vi[1].b
      = tvi_glyphs_[i].vi[2].b = tvi_glyphs_[i].vi[3].b = 1.0f;
    tvi_glyphs_[i].vi[0].x = 0;
    tvi_glyphs_[i].vi[0].y = 0;
    tvi_glyphs_[i].vi[1].x = g->width;
    tvi_glyphs_[i].vi[1].y = 0;
    tvi_glyphs_[i].vi[2].x = g->width;
    tvi_glyphs_[i].vi[2].y = g->height;
    tvi_glyphs_[i].vi[3].x = 0;
    tvi_glyphs_[i].vi[3].y = g->height;
    tvi_glyphs_[i].vi[0].sx = g->sx1;
    tvi_glyphs_[i].vi[0].sy = g->sy1;
    tvi_glyphs_[i].vi[1].sx = g->sx2;
    tvi_glyphs_[i].vi[1].sy = g->sy1;
    tvi_glyphs_[i].vi[2].sx = g->sx2;
    tvi_glyphs_[i].vi[2].sy = g->sy2;
    tvi_glyphs_[i].vi[3].sx = g->sx1;
    tvi_glyphs_[i].vi[3].sy = g->sy2;
  }
  SetTextVertexCycle(1, 0);
}

}