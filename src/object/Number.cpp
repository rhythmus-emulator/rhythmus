#include "Number.h"
#include "Script.h"
#include "config.h"

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

Number::Number() : tvi_glyphs_(nullptr), res_ptr_(nullptr) {}

Number::~Number() { AllocNumberGlyph(0); }

void Number::Load(const MetricGroup& metric)
{
  Text::Load(metric);

#if USE_LR2_FEATURE == 1
#if 0
  if (metric.exist("lr2src"))
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
#endif
#endif

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
  ClearText();
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
    vi[0].p.x += x;
    vi[1].p.x += x;
    vi[2].p.x += x;
    vi[3].p.x += x;
    x += vi[1].p.x - vi[0].p.x;
  }

  // TODO: cycle property, add tvi with cycles

  UpdateTextRenderContext();
}

NumberFormatter &Number::GetFormatter()
{
  return formatter_;
}

void Number::doUpdate(double delta)
{
  if (formatter_.UpdateNumber((float)delta))
    SetText(formatter_.numstr());
  Text::doUpdate(delta);
}

void Number::Refresh()
{
  /* kind of trick to compatible with LR2:
   * if value is UINT_MAX, then set with empty value. */
  if (res_ptr_)
  {
    if (*res_ptr_ == 0xFFFFFFFF)
      SetText(std::string());
    else
      SetNumber(*res_ptr_);
  }
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

void Number::CreateTextVertexFromSprite()
{
#if 0
  /* register glyphs by divx / divy. */
  int divx = numbersprite_.GetSpriteAnimationInfo().divx;
  int divy = numbersprite_.GetSpriteAnimationInfo().divy;
  float divw = (float)numbersprite_.GetImageCoordRect().width() / divx;
  float divh = (float)numbersprite_.GetImageCoordRect().height() / divy;
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
        tvi_glyphs_[ii].vi[0].c = Vector4{ 1.0f };
        tvi_glyphs_[ii].vi[1].c = Vector4{ 1.0f };
        tvi_glyphs_[ii].vi[2].c = Vector4{ 1.0f };
        tvi_glyphs_[ii].vi[3].c = Vector4{ 1.0f };
        tvi_glyphs_[ii].vi[0].p = Vector3{ 0.0f, 0.0f, 0.0f };
        tvi_glyphs_[ii].vi[1].p = Vector3{ divw, 0.0f, 0.0f };
        tvi_glyphs_[ii].vi[2].p = Vector3{ divw, divh, 0.0f };
        tvi_glyphs_[ii].vi[3].p = Vector3{ 0.0f, divh, 0.0f };
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
#endif
}

void Number::CreateTextVertexFromFont()
{
  AllocNumberGlyph(1);
  for (int i = 0; i < 10; ++i)
  {
    auto *g = font()->GetGlyph('0' + i);
    if (!g) continue;

    tvi_glyphs_[i].texid = numbersprite_.image()->get_texture_ID();
    tvi_glyphs_[i].vi[0].c = Vector4{ 1.0f };
    tvi_glyphs_[i].vi[1].c = Vector4{ 1.0f };
    tvi_glyphs_[i].vi[2].c = Vector4{ 1.0f };
    tvi_glyphs_[i].vi[3].c = Vector4{ 1.0f };
    tvi_glyphs_[i].vi[0].p = Vector3{ 0.0f, 0.0f, 0.0f };
    tvi_glyphs_[i].vi[1].p = Vector3{ g->width, 0.0f, 0.0f };
    tvi_glyphs_[i].vi[2].p = Vector3{ g->width, g->height, 0.0f };
    tvi_glyphs_[i].vi[3].p = Vector3{ 0.0f, g->height, 0.0f };
    tvi_glyphs_[i].vi[0].t = Vector2{ g->sx1, g->sy1 };
    tvi_glyphs_[i].vi[1].t = Vector2{ g->sx2, g->sy1 };
    tvi_glyphs_[i].vi[2].t = Vector2{ g->sx2, g->sy2 };
    tvi_glyphs_[i].vi[3].t = Vector2{ g->sx1, g->sy2 };
  }
  SetTextVertexCycle(1, 0);
}

}
