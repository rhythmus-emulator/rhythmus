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

#include <string>
#include <vector>
#include <map>

namespace rhythmus
{

class Setting;
class BaseObject;
class Metric;
constexpr char* kSettingOptionTagName = "option";
constexpr char* kSettingKeyValueTagName = "value";

class Option
{
public:
  Option(const std::string& key);

  const std::string& name() const;
  const std::string& value() const;
  const std::string& type() const;
  const std::string& desc() const;
  void set_description(const std::string& desc);

  // @brief get option string used when calling SetOption()
  const std::string& get_constraint() const;

  // @brief select next item / value
  virtual void Next();

  // @brief select previous item / value
  virtual void Prev();

  // @brief Clear all constraints.
  void Clear();

  // @brief set default option value.
  void SetDefault(const std::string &default__);

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
  void reset_value();

  // @brief whether to save with constraint if it exists
  void save_with_constraint(bool v);

  // @brief utility function to create option
  static Option* CreateOption(
    const std::string &name, const std::string &optionstr);

protected:
  std::string name_;
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
};

class FileOption : public Option
{
public:
  FileOption(const std::string &key);
  virtual void SetOption(const std::string& optionstr);
private:
};

class TextOption : public Option
{
public:
  TextOption(const std::string &key);
  virtual void Next();
  virtual void Prev();
  virtual void set_value(const std::string& value);
private:
};

class NumberOption : public Option
{
public:
  NumberOption(const std::string &key);

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
  void DeleteOption(const std::string &key);
  void Clear();

private:
  std::map<std::string, Option*> options_;
};


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
  Metric(const std::string &name);
  ~Metric();

  void set_name(const std::string &name);
  const std::string& name() const;

  bool exist(const std::string &key) const;

  template <typename T>
  T get(const std::string &key) const;

  void set(const std::string &key, const std::string &v);
  void set(const std::string &key, int v);
  void set(const std::string &key, double v);
  void set(const std::string &key, bool v);

  template <typename T>
  void set_fallback(const std::string &key, T v)
  {
    if (!exist(key)) set(key, v);
  }

  typedef std::map<std::string, std::string>::iterator iterator;
  typedef std::map<std::string, std::string>::const_iterator const_iterator;
  iterator begin();
  const_iterator cbegin() const;
  iterator end();
  const_iterator cend() const;

private:
  std::string name_;
  std::map<std::string, std::string> attr_;
};

class MetricList
{
public:
  void ReadMetricFromFile(const std::string &filepath);
  void Clear();

  bool exist(const std::string &group, const std::string &key) const;

  template <typename T>
  T get(const std::string &group, const std::string &key) const
  {
    auto it = metricmap_.find(group);
    ASSERT_M(it != metricmap_.end(), "Metric group '" + group + "' is not found.");
    return it->second.get<T>(key);
  }

  Metric *get_metric(const std::string &group);

  void set(const std::string &group, const std::string &key, const std::string &v);
  void set(const std::string &group, const std::string &key, int v);
  void set(const std::string &group, const std::string &key, double v);
  void set(const std::string &group, const std::string &key, bool v);

private:
  std::map<std::string, Metric> metricmap_;

  void ReadLR2Metric(const std::string &filepath);
  void ReadLR2SS(const std::string &filepath);
};


/* @brief Metric for single object */


/* @brief Container for Option, ThemeOption, and ThemeMetrics. */
class Setting
{
public:
  static void ReadAll();
  static void ClearAll();
  static void ReloadAll();

  static OptionList &GetSystemSetting();
  static OptionList &GetThemeSetting();
  static MetricList &GetThemeMetricList();

private:
  /* static class. */
  Setting();
};

template <typename T>
void ConvertFromString(const std::string& src, T& dst);

template <typename T>
std::string ConvertToString(const T& dst);

}