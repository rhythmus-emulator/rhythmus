#include "MusicWheel.h"
#include "SceneManager.h"
#include "Setting.h"

namespace rhythmus
{

MusicWheelItem::MusicWheelItem()
{
  AddChild(&title_);
  AddChild(&level_);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    AddChild(&background_[i]);
  }
}

void MusicWheelItem::Load(const Metric &metric)
{
  // TODO: SetFontFromPath() if BarTitleFontPath Exists? / BarTitleOnLoad command.
  title_.LoadFromLR2SRC(metric.get<std::string>("BarTitle"));
  title_.LoadFromLR2DST(metric.get<std::string>("BarTitlelr2cmd"));

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    // TODO: SetImageFromPath() if attr exists?
    background_[i].LoadFromLR2SRC(metric.get<std::string>("BarType" + std::to_string(i) + "lr2src"));
    // TODO: set proper width / height
    background_[i].SetSize(300, 20);
  }

  // TODO: Set NumberText or NumberSprite.
}

void MusicWheelItem::LoadFromMenuData(MenuData *d)
{
  MusicWheelData *data = static_cast<MusicWheelData*>(d);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    if (i == data->type)
      background_[i].Show();
    else
      background_[i].Hide();
  }

  title_.SetText(data->title);
}


// --------------------------- class MusicWheel

MusicWheel::MusicWheel()
{
  set_name("MusicWheel");
  set_infinite_scroll(true);
#if 0
  set_display_count(24);
  set_focus_max_index(12);
  set_focus_min_index(12);
  set_focus_index(12);
#endif
}

MusicWheel::~MusicWheel()
{
  Clear();
}

MusicWheelData& MusicWheel::get_data(int dataindex)
{
  return static_cast<MusicWheelData&>(Menu::GetMenuDataByIndex(dataindex));
}

MusicWheelData& MusicWheel::get_selected_data(int player_num)
{
  return static_cast<MusicWheelData&>(Menu::GetSelectedMenuData());
}

MenuItem* MusicWheel::CreateMenuItem()
{
  MusicWheelItem* item = new MusicWheelItem();
  item->set_parent(this);
  return item;
}

void MusicWheel::Load(const Metric &metric)
{
  Menu::Load(metric);
}

}