#include "OptionMenu.h"
#include "ResourceManager.h"
#include "Font.h"

namespace rhythmus
{

// --------------------------- class OptionItem

void OptionItem::Load(const Metric &metric)
{
  option_name.SetSystemFont();  // XXX: fix later
  MenuItem::Load(metric);
}

// --------------------------- class OptionMenu

OptionMenu::OptionMenu()
{
  set_infinite_scroll(false);
  set_display_count(10);
  set_focus_min_index(1);
  set_focus_max_index(9);
  set_focus_index(1);
}

void OptionMenu::Load(const Metric &metric)
{
  Menu::Load(metric);
}

MenuItem* OptionMenu::CreateMenuItem()
{
  return new OptionItem();
}

}