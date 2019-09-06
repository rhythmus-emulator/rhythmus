#include "MusicWheel.h"
#include "SceneManager.h"
#include "Setting.h"

namespace rhythmus
{

MusicWheelItem::MusicWheelItem(int index)
  : WheelItem(index) {}

Text& MusicWheelItem::title() { return title_; }

Text& MusicWheelItem::level() { return level_; }

MusicWheelItemData* MusicWheelItem::get_data()
{
  return static_cast<MusicWheelItemData*>(WheelItem::get_data());
}

void MusicWheelItem::Invalidate()
{
  if (!get_data())
    return;

  // set Sprite
  if (get_data()->type)
  {
    Sprite* obj =
      dynamic_cast<Sprite*>(get_parent()->FindChildByName(
        "BARIMG" + std::to_string(get_data()->type)
      ));
    if (obj)
      LoadSprite(*obj);
  }

  // update title text
  title().SetText(get_data()->title);

  // update level text
  // TODO
}


// --------------------------- class MusicWheel

MusicWheel::MusicWheel()
{}

const char* MusicWheel::get_selected_title()
{
  return GetItem(get_selected_index()).get_data()->title.c_str();
}

MusicWheelItemData* MusicWheel::NewData()
{
  auto *p = new MusicWheelItemData();
  AddData(p);
  return p;
}

WheelItem* MusicWheel::CreateWheelItem(int index)
{
  return new MusicWheelItem(index);
}

MusicWheelItem& MusicWheel::GetItem(int index)
{
  return *static_cast<MusicWheelItem*>(bar_[index]);
}

void MusicWheel::LoadProperty(const std::string& prop_name, const std::string& value)
{
  if (prop_name == "#SRC_BAR_TITLE")
  {
    // MUST be called after BAR_BODY commands
    std::vector<std::string> params;
    MakeParamCountSafe(value, params, 2);
    if (params[0] != "0") return;
    for (int i = 0; i < bar_.size(); ++i)
      GetItem(i).title().SetFontByName(params[1]);
    return;
  }
  else if (prop_name == "#DST_BAR_TITLE")
  {
    // TODO: only for "0" attribute now ...
    std::string wh = GetFirstParam(value);
    if (wh != "0") return;
    for (int i = 0; i < bar_.size(); ++i)
      GetItem(i).title().LoadProperty(prop_name, value);
    return;
  }
  // TODO: BAR_LEVEL, ...

  Wheel::LoadProperty(prop_name, value);
}

}