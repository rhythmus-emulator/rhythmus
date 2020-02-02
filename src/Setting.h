/**
 * @brief
 * Xml Setting Load & Save class, Used by Game / SceneManager class.
 * Generally used for game internal setting option.
 * This file also provides Serialization & Deserialization for objects
 * which is necessary for setting save & load.
 *
 * @warn
 * 
 */

#pragma once

#include "Error.h"
#include <string>
#include <vector>
#include <map>

namespace rhythmus
{

class Setting;
class BaseObject;
constexpr const char* kSettingOptionTagName = "option";
constexpr const char* kSettingKeyValueTagName = "value";

template <typename T>
void ConvertFromString(const std::string& src, T& dst);

template <typename T>
std::string ConvertToString(const T& dst);

/*
 * @brief Read-only key-value theme properties used for scene elements
 *
 * @params exist bool
 * @params get value
 * @params set value
 * @params set_default set value if not exist (set default value)
 */
class Metric
{
public:
  Metric();
  Metric(const std::string &metric_text);
  ~Metric();

  /* set metric from text. e.g. (attr):(value);(attr):(value) ... */
  void SetFromText(const std::string &metric_text);

  bool exist(const std::string &key) const;

  template <typename T>
  T get(const std::string &key) const;

  void set(const std::string &key, const std::string &v);
  void set(const std::string &key, const char *v);
  void set(const std::string &key, int v);
  void set(const std::string &key, double v);
  void set(const std::string &key, bool v);

  typedef std::map<std::string, std::string>::iterator iterator;
  typedef std::map<std::string, std::string>::const_iterator const_iterator;
  iterator begin();
  const_iterator cbegin() const;
  iterator end();
  const_iterator cend() const;

private:
  void resolve_fallback(const std::string &key);
  std::map<std::string, std::string> attr_;
};

class MetricList
{
public:
  void ReadMetricFromFile(const std::string &filepath);
  void Clear();

  bool exist(const std::string &group, const std::string &key) const;

  /* get metric considering fallback. */
  const Metric *get_metric(const std::string &group) const;
  Metric *get_metric(const std::string &group);
  Metric *create_metric(const std::string &group);
  void copy_metric(const std::string &name_from, const std::string &name_to);

  template <typename T>
  T get(const std::string &group, const std::string &key) const
  {
    auto *m = get_metric(group);
    R_ASSERT(m, "Metric group '" + group + "' is not found.");
    return m->get<T>(key);
  }

  void set(const std::string &group, const std::string &key, const std::string &v);
  void set(const std::string &group, const std::string &key, const char *v);
  void set(const std::string &group, const std::string &key, int v);
  void set(const std::string &group, const std::string &key, double v);
  void set(const std::string &group, const std::string &key, bool v);

private:
  std::map<std::string, Metric> metricmap_;

  void ReadLR2Metric(const std::string &filepath);
  void ReadLR2SS(const std::string &filepath);
  void ReadXml(const std::string &filepath);
};

class Option
{
public:
  Option();

  template <typename T> T value() const;

  const std::string& type() const;
  const std::string& desc() const;
  Option &set_description(const std::string& desc);

  // @brief get option string used when calling SetOption()
  const std::string& get_constraint() const;

  // @brief select next item / value
  virtual void Next();

  // @brief select previous item / value
  virtual void Prev();

  // @brief Clear all constraints.
  void Clear();

  // @brief set default option value.
  Option &SetDefault(const std::string &default__);

  // @brief separate selectable options with comma
  // Starts with '!F' : file option (input as file filter)
  // Starts with '!N' : number option (input as min,max)
  // @warn  must call reset() method after setting SetOption()
  //        to make option value valid.
  virtual void SetOption(const std::string& optionstr);

  // @brief set value
  // @warn may be value is changed due to validation check
  //       in case of constraint exists.
  virtual void set_value(const std::string& value);
  virtual void set_value(int value);

  // @brief reset value
  Option &reset_value();

  // @brief whether to save with constraint if it exists
  void save_with_constraint(bool v);

  // @brief utility function to create option
  static Option* CreateOption(const std::string &optionstr);

  Option &show();
  Option &hide();

protected:
  std::string desc_;
  std::string value_;
  std::string type_;
  std::string default_;

  // selectable options, if necessary.
  std::vector<std::string> options_;

  // currently selected index
  size_t curr_sel_idx_;

  // option for original string
  std::string constraint_;

  // save with constraint, not only option value.
  // (just for option item but not want to save the options)
  bool save_with_constraint_;

  // Is this option is visible for modifying?
  // default: true
  bool visible_;
};

class FileOption : public Option
{
public:
  FileOption();
  virtual void SetOption(const std::string& optionstr);
private:
};

class TextOption : public Option
{
public:
  TextOption();
  virtual void Next();
  virtual void Prev();
  virtual void set_value(const std::string& value);
private:
};

class NumberOption : public Option
{
public:
  NumberOption();

  // @brief get value as int (or index)
  int value_int() const;
  int value_min() const;
  int value_max() const;

  // @brief set number option with (min),(max)
  virtual void SetOption(const std::string& optionstr);
  virtual void Next();
  virtual void Prev();

  virtual void set_value(const std::string& value);
  virtual void set_value(int value);

private:
  // selected value (for number)
  int number_;

  // constraint for value (for number)
  int number_min_, number_max_;
};

/* @brief Option group container. */
class OptionList
{
public:
  OptionList();
  ~OptionList();

  bool LoadFromFile(const std::string &filepath);
  void LoadFromMetrics(const Metric &metric);

  /* @brief read only option value from file */
  bool ReadFromFile(const std::string &filepath);

  /* @brief save only option value to file */
  bool SaveToFile(const std::string &path);

  Option *GetOption(const std::string &key) const;
  Option &SetOption(const std::string &key, const std::string &optionstr);
  Option &SetOption(const std::string &key, Option *option);
  void SetOptionFromMetric(Metric *metric);
  void DeleteOption(const std::string &key);
  void Clear();

  template <typename T>
  T GetValue(const std::string &key) const
  {
    Option *option = GetOption(key);
    R_ASSERT(option);
    return option->value<T>();
  }

  template <typename T>
  void SetValue(const std::string &key, T val) const
  {
    Option *option = GetOption(key);
    R_ASSERT(option);
    return option->set_value(val);
  }

private:
  std::map<std::string, Option*> options_;
};


/* @brief Container for Option, ThemeOption, and ThemeMetrics. */
class Setting
{
public:
  static void ReadAll();
  static void ClearAll();
  static void ReloadAll();
  static void SaveAll();

  static OptionList &GetSystemSetting();
  static OptionList &GetThemeSetting();
  static MetricList &GetThemeMetricList();

private:
  /* static class. */
  Setting();
};

/* TODO: move these constants to game module? */

constexpr size_t kMaxPlaySession = 4;
constexpr size_t kMaxLaneCount = 16;

/* @brief Input setting for play. */
struct KeySetting
{
  int keycode_per_track_[kMaxLaneCount][4];
};

}
