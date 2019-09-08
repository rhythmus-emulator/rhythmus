#include "MusicWheel.h"
#include "SceneManager.h"
#include "Setting.h"

namespace rhythmus
{

MusicWheelItem::MusicWheelItem(int index)
  : WheelItem(index)
{
  AddChild(&title_);
  AddChild(&level_);
}

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

  // set Sprite background
  Sprite* obj =
    dynamic_cast<Sprite*>(get_parent()->FindChildByName(
      "BARIMG" + std::to_string(get_data()->type)
    ));
  if (obj)
    LoadSprite(*obj);

  // update title text
  title().SetText(get_data()->title);

  // update level text
  // TODO
}


// --------------------------- class MusicWheel

MusicWheel::MusicWheel()
{}

MusicWheelItemData& MusicWheel::get_selected_data()
{
  return get_data(get_selected_dataindex());
}

MusicWheelItemData& MusicWheel::NewData()
{
  auto *p = new MusicWheelItemData();
  AddData(p);
  return *p;
}

WheelItem* MusicWheel::CreateWheelItem(int index)
{
  return new MusicWheelItem(index);
}

MusicWheelItem& MusicWheel::get_item(int index)
{
  return *static_cast<MusicWheelItem*>(Wheel::get_item(index));
}

MusicWheelItemData& MusicWheel::get_data(int dataindex)
{
  return *static_cast<MusicWheelItemData*>(select_data_[dataindex]);
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
      get_item(i).title().SetFontByName(params[1]);
    return;
  }
  else if (prop_name == "#DST_BAR_TITLE")
  {
    // TODO: only for "0" attribute now ...
    std::string wh = GetFirstParam(value);
    if (wh != "0") return;
    for (int i = 0; i < bar_.size(); ++i)
      get_item(i).title().LoadProperty(prop_name, value);
    return;
  }
  // TODO: BAR_LEVEL, ...

  Wheel::LoadProperty(prop_name, value);
}

}