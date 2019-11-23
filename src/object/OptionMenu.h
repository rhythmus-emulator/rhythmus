#pragma once

#include "Menu.h"
#include "Text.h"

namespace rhythmus
{

class OptionData : public MenuData
{
public:
  // displayed option name
  std::string displayname;

  // selectable values
  std::vector<std::string> values;

  // selected index
  int index;
  
  // selected value
  std::string value;
};

class OptionItem : public MenuItem
{
public:
  virtual void Load(const Metric &metric);

private:
  Text option_name;
  Text option_value;
};

class OptionMenu : public Menu
{
public:
  OptionMenu();
  virtual void Load(const Metric &metric);

#if 0
  void SetAsNavigator(int mode_to_change);
  void SetAsNumberOption(int min, int max, int val);
  void SetAsDecimalOption(float min, float max, int val);
  void SetAsKeysetting();
  void SetAsListOption(const std::vector<std::string>& list);
  void SetSelectedValue(const std::string& selected_value);
  void SetSelectedValue(float value);
  void SetSelectedValue(int value);
#endif

private:
  virtual MenuItem* CreateMenuItem();
};

}