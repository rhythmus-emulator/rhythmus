#include "Number.h"
#include "Script.h"

namespace rhythmus
{

// ---------------------- class NumberInterface

NumberInterface::NumberInterface()
  : number_(0), disp_number_(0),
    number_change_duration_(0), number_change_remain_(0), need_update_(true),
    number_format_("%d")
{
  *num_str_ = 0;
}

void NumberInterface::SetNumber(int number)
{
  number_ = (double)number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
}

void NumberInterface::SetNumber(double number) {
  number_ = number;
  number_change_remain_ = number_change_duration_;
  need_update_ = true;
}

template <> int NumberInterface::GetNumber() const { return number_; }
template <> double NumberInterface::GetNumber() const { return number_; }
void NumberInterface::SetNumberChangeTime(float msec)
{
  number_change_duration_ = (float)msec;
}

void NumberInterface::SetFormat(const std::string& number_format)
{
  number_format_ = number_format;
}

bool NumberInterface::UpdateNumber(float delta)
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

const char* NumberInterface::numstr() const
{
  return num_str_;
}


// ------------------------- class NumberSprite

NumberSprite::NumberSprite() : align_(0), data_index_(0) {}

void NumberSprite::SetNumberText(const char* num)
{
  TextVertexInfo tvi;
  tvi.texid = 0; /* unused */
  tvi.vi[0].a = tvi.vi[0].r = tvi.vi[0].g = tvi.vi[0].b = 1.0f;
  tvi.vi[1].a = tvi.vi[1].r = tvi.vi[1].g = tvi.vi[1].b = 1.0f;
  tvi.vi[2].a = tvi.vi[2].r = tvi.vi[2].g = tvi.vi[2].b = 1.0f;
  tvi.vi[3].a = tvi.vi[3].r = tvi.vi[3].g = tvi.vi[3].b = 1.0f;

  tvi_.clear();
  while (*num)
  {
    // TODO: position, cycle property
    tvi.vi[0].x = 0;
    tvi.vi[0].y = 0;
    tvi.vi[0].sx = 0;
    tvi.vi[0].sy = 0;
    tvi.vi[1].x = 0;
    tvi.vi[1].y = 0;
    tvi.vi[1].sx = 0;
    tvi.vi[1].sy = 0;
    tvi.vi[2].x = 0;
    tvi.vi[2].y = 0;
    tvi.vi[2].sx = 0;
    tvi.vi[2].sy = 0;
    tvi.vi[3].x = 0;
    tvi.vi[3].y = 0;
    tvi.vi[3].sx = 0;
    tvi.vi[3].sy = 0;

    tvi_.push_back(tvi);
    ++num;
  }
}

void NumberSprite::SetTextTableIndex(size_t idx)
{
  data_index_ = idx;
}

void NumberSprite::Load(const Metric& metric)
{
  Sprite::Load(metric);
}

void NumberSprite::SetTextFromTable()
{
  SetNumberText(
    Script::getInstance().GetString(data_index_).c_str()
  );
}

void NumberSprite::SetLR2Alignment(int type)
{
  align_ = type;
}

void NumberSprite::LoadFromLR2SRC(const std::string &cmd)
{
  // TODO: LR2 sprite font implementation
  // (image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align)
  CommandArgs args(cmd);

  /* track change of number table */
  std::string eventname = args.Get<std::string>(9);
  cmdfnmap_[eventname] = [](void *o, CommandArgs& args) {
    static_cast<NumberSprite*>(o)->SetTextFromTable();
  };
  SubscribeTo(eventname);
  /* set value instantly */
  SetTextTableIndex(atoi(eventname.c_str()));

  /* alignment */
  SetLR2Alignment(args.Get<int>(10));
}

void NumberSprite::CreateCommandFnMap()
{
  Sprite::CreateCommandFnMap();
  cmdfnmap_["number"] = [](void *o, CommandArgs& args) {
    static_cast<NumberSprite*>(o)->SetNumber(args.Get<int>(0));
  };
}

void NumberSprite::doUpdate(float delta)
{
  if (UpdateNumber(delta))
    SetNumberText(numstr());
  Sprite::doUpdate(delta);
}

void NumberSprite::doRender()
{
  // If hide, then not draw
  if (!img_ || !IsVisible())
    return;

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

void NumberText::doUpdate(float delta)
{
  if (UpdateNumber(delta))
    SetText(numstr());
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

void NumberText::SetTextFromTable()
{
  SetNumber(Script::getInstance().GetNumber(GetTextTableIndex()));
}

void NumberText::LoadFromLR2SRC(const std::string &cmd)
{
  // TODO: LR2 sprite font implementation
  // (image),(x),(y),(w),(h),(divx),(divy),(cycle),(timer),(num),(align)
  CommandArgs args(cmd);

  /* track change of number table */
  std::string eventname = args.Get<std::string>(9);
  cmdfnmap_[eventname] = [](void *o, CommandArgs& args) {
    static_cast<NumberText*>(o)->SetTextFromTable();
  };
  SubscribeTo(eventname);
  /* set value instantly */
  SetTextTableIndex(atoi(eventname.c_str()));

  /* alignment */
  SetLR2Alignment(args.Get<int>(10));
}

void NumberText::CreateCommandFnMap()
{
  Text::CreateCommandFnMap();
  cmdfnmap_["number"] = [](void *o, CommandArgs& args) {
    static_cast<NumberText*>(o)->SetNumber(args.Get<int>(0));
  };
}

}