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

/* @warn use same metric that MusicWheel used. */
void MusicWheelItem::Load(const Metric &metric)
{
  title_.set_name("MusicWheelTitle");
  title_.LoadByName();
  title_.LoadCommandWithNamePrefix(metric);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    background_[i].set_name("MusicWheelType" + std::to_string(i));
    background_[i].LoadByName();
    background_[i].LoadCommandWithPrefix("Bar" + std::to_string(i), metric);
    // Call LoadFromLR2DST and clear position data after it if necessary
    // (for LR2 compatibility)
    if (true)
      background_[i].SetAllTweenPos(0, 0);
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
  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    // TODO: display proper bar type
  }
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