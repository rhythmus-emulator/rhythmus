#include "Number.h"
#include "Script.h"
#include "Util.h"
#include "config.h"
#include <string.h>
#include <algorithm>

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


// ------------------------------- class Number

Number::Number() :
  img_(nullptr), font_(nullptr), tvi_glyphs_(nullptr),
  cycle_count_(0), cycle_time_(0), val_ptr_(nullptr), display_style_(0), keta_(1)
{
  *num_chrs = 0;
  UpdateVertex();
}

Number::~Number()
{
  ClearAll();
}

void Number::Load(const MetricGroup& metric)
{
  BaseObject::Load(metric);

  // use font if exists
  // XXX: need to use 'path' attribute?
  if (metric.exist("font"))
    SetGlyphFromFont(metric);

  if (metric.exist("lr2src"))
  {
    SetGlyphFromLR2SRC(metric.get_str("lr2src"));
  }
}

void Number::SetGlyphFromFont(const MetricGroup &m)
{
  ClearAll();
  font_ = FONTMAN->Load(m);
  if (!font_)
    return;
  SleepUntilLoadFinish(font_);
  AllocNumberGlyph(1);

  std::vector<TextVertexInfo> textvertex;
  font_->GetTextVertexInfo("0123456789+-0123456789+-", textvertex, false);
  for (unsigned i = 0; i < 24; ++i)
    tvi_glyphs_[i] = textvertex[i];
}

void Number::SetGlyphFromLR2SRC(const std::string &lr2src)
{
#if USE_LR2_FEATURE == 1
  // (null),(image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align),(keta)
  CommandArgs args(lr2src);

  ClearAll();
  img_ = IMAGEMAN->Load(args.Get_str(1));
  if (!img_)
    return;
  SleepUntilLoadFinish(img_);

  /* add glyphs */
  Vector4 imgcoord{
    args.Get<int>(2), args.Get<int>(3), args.Get<int>(4), args.Get<int>(5)
  };
  int divx = args.Get<int>(6);
  int divy = args.Get<int>(7);
  unsigned cnt = (unsigned)(divx * divy);
  AllocNumberGlyph(cnt);
  for (unsigned j = 0; j < divy; ++j)
  {
    for (unsigned i = 0; i < divx; ++i)
    {
      auto &tvi = tvi_glyphs_[j * divx + i];
      // XXX: sprite must be loaded first, or invalid texcoord..
      tvi.vi[0].t = Vector2(0.0f, 0.0f);
      tvi.vi[1].t = Vector2(0.0f, 0.0f);
      tvi.vi[2].t = Vector2(0.0f, 0.0f);
      tvi.vi[3].t = Vector2(0.0f, 0.0f);
      tvi.vi[0].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[1].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[2].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[3].c = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      tvi.vi[0].p = Vector3(0.0f, 0.0f, 0.0f);
      tvi.vi[1].p = Vector3(0.0f, 0.0f, 0.0f);
      tvi.vi[2].p = Vector3(0.0f, 0.0f, 0.0f);
      tvi.vi[3].p = Vector3(0.0f, 0.0f, 0.0f);
    }
  }

  /* track change of number table */
  int eventid = args.Get<int>(9);
  std::string eventname = "Number" + args.Get<std::string>(9);
  AddCommand(eventname, "refresh");
  SubscribeTo(eventname);

  /* alignment (not use LR2 alignment here) */
  switch (args.Get<int>(11))
  {
  case 0:
    GetCurrentFrame().align.x = 0.0f;
    display_style_ = 0;
    break;
  case 1:
    GetCurrentFrame().align.x = 1.0f;
    display_style_ = 1;
    break;
  case 2:
    // XXX: different from LR2 if keta enabled
    GetCurrentFrame().align.x = 0.5f;
    display_style_ = 0;
    break;
  }

  /* keta processing: tween width multiply & set number formatter */
  int keta = args.Get<int>(12);
  keta_ = keta;
  value_params_.max_int = keta;
  value_params_.max_decimal = 0;

  /* set value instantly: TODO */
  //SetResourceId(eventid);
  //Refresh();
#endif
}

void Number::SetNumber(int number)
{
  UpdateVertex();
}

void Number::SetNumber(double number)
{
  UpdateVertex();
}

void Number::SetText(const std::string &num)
{
  strcpy(num_chrs, num.c_str());
  UpdateVertex();
}

void Number::Refresh()
{
  /* kind of trick to compatible with LR2:
   * if value is UINT_MAX, then set with empty value. */
  if (val_ptr_)
  {
    if (*val_ptr_ == 0xFFFFFFFF)
      SetText(std::string());
    else
      SetNumber(*val_ptr_);
  }
}

void Number::ClearAll()
{
  AllocNumberGlyph(0);

  if (font_)
  {
    FONTMAN->Unload(font_);
    font_ = nullptr;
  }

  if (img_)
  {
    IMAGEMAN->Unload(img_);
    img_ = nullptr;
  }
}

void Number::AllocNumberGlyph(unsigned cycles)
{
  if (tvi_glyphs_)
  {
    free(tvi_glyphs_);
    tvi_glyphs_ = nullptr;
  }
  if (cycles == 0)
    return;
  tvi_glyphs_ = (TextVertexInfo*)calloc(24 * cycles, sizeof(TextVertexInfo));
  cycle_count_ = cycles;
}

void Number::UpdateVertex()
{
  if (tvi_glyphs_ == nullptr)
  {
    render_glyphs_count_ = 0;
    return;
  }

  // vertex alignment.
}

void Number::doUpdate(double delta)
{
  // update current number
  if (!value_params_.time > 0)
  {
    value_params_.time -= delta;
    if (value_params_.time < 0) value_params_.time = 0;

    // since number is updated here, need to update displaying glyphs.
    // TODO: more detailed spec ...
    if (value_params_.max_decimal == 0)
      sprintf(num_chrs, "%d", (int)value_params_.curr);
    else
      gcvt(value_params_.curr, 10, num_chrs);
    UpdateVertex();
  }
}

void Number::UpdateRenderingSize(Vector2 &d, Vector3 &p)
{
  d.x *= keta_;
}

}
