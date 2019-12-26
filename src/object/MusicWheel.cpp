#include "MusicWheel.h"
#include "SceneManager.h"
#include "Script.h"
#include "Setting.h"

namespace rhythmus
{

MusicWheelItem::MusicWheelItem()
{
}

/* @warn use same metric that MusicWheel used. */
void MusicWheelItem::Load(const Metric &metric)
{
  title_ = std::make_unique<Text>();
  title_->set_name("MusicWheelTitle");
  title_->LoadByName();
  title_->LoadCommandWithNamePrefix(metric);

  /* title object don't affected by op code. */
  title_->SetIgnoreVisibleGroup(true);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    background_[i].set_name("MusicWheelType" + std::to_string(i));
    background_[i].LoadByName();
    background_[i].LoadCommandWithPrefix("Bar" + std::to_string(i), metric);
    // Call LoadFromLR2DST and clear position data after it if necessary
    // (for LR2 compatibility)
    if (true /* TODO: BarTypeSpritePosRemove metric */)
      background_[i].SetAllTweenPos(0, 0);
  }

  // TODO: Set NumberText or NumberSprite.
  /* TODO: NumberSpriteLevel metric */
  level_ = std::make_unique<Number>();
  level_->set_name("MusicWheelLevel0");
  level_->LoadByName();
  level_->LoadCommandWithNamePrefix(metric);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    AddChild(&background_[i]);
  }
  AddChild(&*title_);
  AddChild(&*level_);
}

bool MusicWheelItem::LoadFromMenuData(MenuData *d)
{
  if (!MenuItem::LoadFromMenuData(d))
    return false;

  MusicWheelData *data = static_cast<MusicWheelData*>(d);

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    if (i == data->type)
      background_[i].Show();
    else
      background_[i].Hide();
  }

  for (size_t i = 0; i < NUM_SELECT_BAR_TYPES; ++i)
  {
    // TODO: display proper bar type
  }

  title_->SetText(data->title);
  level_->SetNumber(data->level);

  return true;
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

void MusicWheel::OnSelectChange(const MenuData *data, int direction)
{
  const auto *d = static_cast<const MusicWheelData*>(data);
  /* Song stat */
  Script::getInstance().SetString(10, d->title);
  Script::getInstance().SetString(11, d->subtitle);
  Script::getInstance().SetString(12, d->title + " " + d->subtitle);
  Script::getInstance().SetString(13, d->genre);
  Script::getInstance().SetString(14, d->artist);
  Script::getInstance().SetNumber(160, 140);
  Script::getInstance().SetNumber(160, 140);
  Script::getInstance().SetNumber(70, 180000);
  Script::getInstance().SetNumber(71, 2880);
  Script::getInstance().SetNumber(72, 2880);
  Script::getInstance().SetNumber(73, 100);
  Script::getInstance().SetNumber(74, 1235);
  Script::getInstance().SetNumber(75, 1234);
  Script::getInstance().SetNumber(76, 9);
  Script::getInstance().SetNumber(77, 5);
  Script::getInstance().SetNumber(78, 3);
  Script::getInstance().SetNumber(79, 1);
  /* Playrecord */
  Script::getInstance().SetNumber(80, 1000);
  Script::getInstance().SetNumber(81, 300);
  Script::getInstance().SetNumber(82, 100);
  Script::getInstance().SetNumber(83, 12);
  Script::getInstance().SetNumber(84, 9);
  Script::getInstance().SetNumber(85, 90);
  Script::getInstance().SetNumber(86, 10);
  Script::getInstance().SetNumber(87, 5);
  Script::getInstance().SetNumber(88, 3);
  Script::getInstance().SetNumber(89, 2);
  /* BPM */
  Script::getInstance().SetNumber(90, 140);
  Script::getInstance().SetNumber(91, 140);
  /* IR */
  Script::getInstance().SetNumber(92, 0);
  Script::getInstance().SetNumber(93, 0);
  Script::getInstance().SetNumber(94, 0);

  EventManager::SendEvent("SongSelectChange");
  if (direction == -1)
    EventManager::SendEvent("SongSelectChangeUp");
  else if (direction == 1)
    EventManager::SendEvent("SongSelectChangeDown");
}

void MusicWheel::OnSelectChanged()
{
  EventManager::SendEvent("SongSelectChanged");
}

}