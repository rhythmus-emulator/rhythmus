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

void MusicWheelItem::set_data(MusicWheelData* d)
{
  SetImage(((MusicWheel*)get_parent())->select_bar_img_[d->type]);
  title().SetText(get_data()->title);
  // TODO: update level text
}


// --------------------------- class MusicWheel

MusicWheel::MusicWheel()
{
  set_name("MusicWheel");
  set_infinite_scroll(true);
  set_display_count(24);
  set_focus_max_index(12);
  set_focus_min_index(12);
  set_focus_index(12);
  wheel_sound_.set_name("");
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
  item->title().SetFontByPath(title_font_);
  item->title().RunCommand("OnLoad", title_dst_);
  item->title().Show();
  return item;
}

void SetBarTypeCount(size_t typesize)
{

}

void MusicWheel::Load(const Metric &metric)
{
  Menu::Load(metric);

  title_font_ = metric.get<std::string>("BarTitleFont");
  title_dst_ = metric.get<std::string>("BarTitleFontOnLoad");
  bar_type_count_ = metric.get<int>("BarType");
  for (size_t i = 0; i < bar_type_count_; ++i)
  {
    select_bar_img_[i] = ResourceManager::LoadImage(
      metric.get<std::string>("BarType" + std::to_string(i))
    );
  }
}

void MusicWheel::Clear()
{
  bar_type_count_ = 0;
}

}