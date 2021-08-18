#include "OptionMenu.h"
#include "ResourceManager.h"
#include "Font.h"

namespace rhythmus
{

// --------------------------- class OptionItem

void OptionItem::Load(const MetricGroup &metric)
{
  option_name.SetFont("SystemFont");  // XXX: fix later
  BaseObject::Load(metric);
}

// --------------------------- class OptionMenu

OptionMenu::OptionMenu()
{
}

void OptionMenu::Load(const MetricGroup &metric)
{
  BaseObject::Load(metric);
}

void* OptionMenu::GetSelectedMenuData()
{
  // TODO
  return nullptr;
}

}