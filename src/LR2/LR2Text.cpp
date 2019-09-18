#include "LR2Text.h"

RHYTHMUS_NAMESPACE_BEGIN

// ------------------------------ class LR2Text

LR2Text::LR2Text() : lr2_source_id_(0), align_(0), editable_(0)
{
  set_name("LR2Text");
}

LR2Text::~LR2Text()
{
}

void LR2Text::LoadProperty(const std::string& prop_name, const std::string& value)
{
  Text::LoadProperty(prop_name, value);

  if (prop_name.compare(0, 5, "#SRC_") == 0)
  {
    std::vector<std::string> params;
    MakeParamCountSafe(value, params, 5);
    lr2_source_id_ = atoi(params[2].c_str());
    align_ = atoi(params[3].c_str());
    editable_ = atoi(params[4].c_str());

    switch (lr2_source_id_)
    {
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
      SubscribeTo(Events::kEventSongSelectChanged);
      break;
    }

    switch (align_)
    {
    case 0:
      // left
      SetAlignment(TextAlignments::kTextAlignFitMaxsize);
      break;
    case 1:
      // center
      SetAlignment(TextAlignments::kTextAlignCenterFitMaxsize);
      SetTextPosition(1);
      break;
    case 2:
      // right
      SetAlignment(TextAlignments::kTextAlignRightFitMaxsize);
      SetTextPosition(2);
      break;
    }

    SetFontByName(params[1]);
  }
  else if (prop_name.compare(0, 5, "#DST_") == 0)
  {
    set_op(GetAttribute<int>("op0"),
      GetAttribute<int>("op1"),
      GetAttribute<int>("op2"));
    set_timer_id(GetAttribute<int>("timer"));
  }
}

bool LR2Text::IsVisible() const
{
  return IsLR2Visible() && BaseObject::IsVisible();
}

void LR2Text::Load()
{
  const char* c;
  if (lr2_source_id_ && (c = LR2Flag::GetText(lr2_source_id_)) != 0)
    SetText(c);
}

bool LR2Text::OnEvent(const EventMessage &e)
{
  const char* c;
  if ((c = LR2Flag::GetText(lr2_source_id_)) != 0)
    SetText(LR2Flag::GetText(lr2_source_id_));
  return true;
}

RHYTHMUS_NAMESPACE_END