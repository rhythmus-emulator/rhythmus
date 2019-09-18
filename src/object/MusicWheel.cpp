#include "MusicWheel.h"
#include "SceneManager.h"
#include "Setting.h"

namespace rhythmus
{

MusicWheelItem::MusicWheelItem()
{
  AddChild(&title_);
  AddChild(&level_);
}

Text& MusicWheelItem::title() { return title_; }

NumberText& MusicWheelItem::level() { return level_; }

MusicWheelData* MusicWheelItem::get_data()
{
  return static_cast<MusicWheelData*>(MenuItem::get_data());
}

void MusicWheelItem::Load()
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
{
  set_infinite_scroll(true);
  set_display_count(24);
  set_focus_max_index(12);
  set_focus_min_index(12);
  set_focus_index(12);
}

MusicWheelData& MusicWheel::get_data(int dataindex)
{
  return static_cast<MusicWheelData&>(Menu::GetMenuDataByIndex(dataindex));
}

MusicWheelData& MusicWheel::get_selected_data()
{
  return static_cast<MusicWheelData&>(Menu::GetSelectedMenuData());
}

MenuItem* MusicWheel::CreateMenuItem()
{
  MusicWheelItem* item = new MusicWheelItem();
  item->set_parent(this);
  item->title().SetFontByName(title_font_);
  item->title().LoadTween(title_dst_);
  item->title().Show();
  return item;
}

void MusicWheel::LoadProperty(const std::string& prop_name, const std::string& value)
{
  if (prop_name == "#SRC_BAR_TITLE")
  {
    // MUST be called after BAR_BODY commands
    std::vector<std::string> params;
    MakeParamCountSafe(value, params, 2);
    if (params[0] != "0") return;
    title_font_ = params[1];
    return;
  }
  else if (prop_name == "#DST_BAR_TITLE")
  {
    // TODO: only for "0" attribute now ...
    std::string wh = GetFirstParam(value);
    if (wh != "0") return;
    title_dst_.LoadProperty(prop_name, value);
    return;
  }
  // TODO: BAR_LEVEL, ...

  Menu::LoadProperty(prop_name, value);
}

}