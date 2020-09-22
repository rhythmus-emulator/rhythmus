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
#include "config.h"
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
 * @brief   Key-value properties used for setting object property.
 *
 * @params  exist bool
 * @params  get value
 * @params  set value
 * @params  set_default set value if not exist (set default value)
 *
 * @warn    Key cannot be duplicated in a MetricGroup (overwritten)
 *          But MetricGroups can be duplicated.
 *          You can iterate them with group_begin() / group_end() method.
 *
 * @usage
 * Suppose there is 'Setting' MetricGroup and 'debug' int attribute exists.
 * To get children property, you can use one of these methods:
 *
 * 1.
 * metric.get<int>("Setting.debug");
 *
 * 2.
 * auto &metric_setting = metric.get_group("Setting");
 * metric_setting.get<int>("debug");
 */
class MetricGroup
{
public:
  MetricGroup();
  MetricGroup(const std::string &name);
  ~MetricGroup();

  const std::string &name() const;
  void set_name(const std::string &name);

  bool Load(const std::string &path);
  bool LoadFromXml(const std::string &path);
  bool LoadFromIni(const std::string &path);

  bool Save(const std::string &path);

  /* set metric from text. e.g. (attr):(value);(attr):(value) ... */
  void SetFromText(const std::string &metric_text);

  bool exist(const std::string &key) const;

  void clear();

  // @warn may contain duplicated group.
  MetricGroup& add_group(const std::string &group_name);

  // @warn lastest used group is returned.
  MetricGroup* get_group(const std::string &group_name);
  const MetricGroup* get_group(const std::string &group_name) const;

  template <typename T>
  T get(const std::string &key) const;
  const std::string &get_str(const std::string &key) const;

  bool get_safe(const std::string &key, std::string &v) const;
  bool get_safe(const std::string &key, int &v) const;
  bool get_safe(const std::string &key, unsigned &v) const;
  bool get_safe(const std::string &key, double &v) const;
  bool get_safe(const std::string &key, float &v) const;
  bool get_safe(const std::string &key, bool &v) const;

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

  typedef std::vector<MetricGroup>::iterator group_iterator;
  typedef std::vector<MetricGroup>::const_iterator const_group_iterator;
  group_iterator group_begin();
  group_iterator group_end();
  const_group_iterator group_cbegin() const;
  const_group_iterator group_cend() const;

private:
  void resolve_fallback(const std::string &key);
  std::string name_;

  // attributes. may not be duplicated.
  std::map<std::string, std::string> attr_;

  // children groups. can be duplicated.
  std::vector<MetricGroup> children_;
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
  void LoadFromMetrics(const MetricGroup &metric);

  /* @brief read only option value from file */
  bool ReadFromFile(const std::string &filepath);

  /* @brief save only option value to file */
  bool SaveToFile(const std::string &path);

  Option *GetOption(const std::string &key) const;
  Option &SetOption(const std::string &key, const std::string &optionstr);
  Option &SetOption(const std::string &key, Option *option);
  void AddOptionFromMetric(MetricGroup *metric);
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
  bool GetValueSafe(const std::string &key, T& value) const
  {
    Option *option = GetOption(key);
    if (!option)
      return false;
    value = option->value<T>();
    return true;
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

/* @brief Preference element for set/get value with type. */
template <typename T>
class Preference
{
public:
  Preference() : is_static_(false) {};
  Preference(const T& default_value) : v_(default_value), is_static_(false) {};
  Preference(const T& default_value, const std::string &desc, const std::string &constraint, bool is_static)
    : v_(default_value), desc_(desc), constraint_(constraint), is_static_(is_static) {};
  T& operator*() { return v_; }
  const T& operator*() const { return v_; }

  const void set_desc(const std::string &v) { desc_ = v; }
  const void set_constraint(const std::string &v) { constraint_ = v; }
  const std::string &get_desc() { return desc_; }
  const std::string &get_constraint() { return constraint_; }
  bool is_static() const { return is_static_; }

private:
  T v_;
  std::string desc_;        // description of preference
  std::string constraint_;  // constraint of preference (used by option class)
  bool is_static_;          // is static preference (which means irremovable)
};

/* @brief Load/Save preference values by types, and Get/Set easily. */
class PreferenceList
{
public:
  virtual ~PreferenceList();

  void AddInt(const std::string &key, Preference<int> *pf);
  void AddFloat(const std::string &key, Preference<float> *pf);
  void AddString(const std::string &key, Preference<std::string> *pf);
  void AddFile(const std::string &key, Preference<std::string> *pf);

  void SetInt(const std::string &key, int v);
  void SetFloat(const std::string &key, float v);
  void SetString(const std::string &key, const std::string &v);
  void SetFile(const std::string &key, const std::string &v);

  Preference<int>* GetInt(const std::string &key);
  Preference<float>* GetFloat(const std::string &key);
  Preference<std::string>* GetString(const std::string &key);
  Preference<std::string>* GetFile(const std::string &key);

  void Load(const std::string &path);
  void Load(const MetricGroup &m);
  bool Save(const std::string &path);

  // clear all preference except static preference.
  void Clear();

protected:
  std::map<std::string, Preference<int>*> pref_i_;
  std::map<std::string, Preference<float>*> pref_f_;
  std::map<std::string, Preference<std::string>*> pref_s_;
  std::map<std::string, Preference<std::string>*> pref_file_;
  std::vector<void*> non_static_prefs_;
};

/* @brief Preference for global use. */
class SystemPreference : public PreferenceList
{
public:
  SystemPreference();
  virtual ~SystemPreference();

  Preference<std::string> resolution;

  Preference<int> sound_buffer_size;
  Preference<int> volume;

  Preference<int> gamemode;
  Preference<int> logging;
  Preference<int> maximum_thread;

  Preference<std::string> soundset;
  Preference<std::string> theme;
  Preference<std::string> theme_test;
  Preference<std::string> theme_select;
  Preference<std::string> theme_decide;
  Preference<std::string> theme_play_7key;
  Preference<std::string> theme_result;
  Preference<std::string> theme_courseresult;
  Preference<int> theme_load_async;

  Preference<int> select_sort_type;
  Preference<int> select_difficulty_mode;
};

/* @brief Container for Option, ThemeOption, and ThemeMetrics. */
class Setting
{
public:
  static void Initialize();
  static void Cleanup();
  static void Reload();
  static void Save();

private:
  /* static class. */
  Setting();
};

/* @brief Input setting for play. */
class KeySetting
{
public:
  KeySetting();
  void Default();
  void Load(const MetricGroup &metric);
  void Save(MetricGroup &metric);
  int GetTrackFromKeycode(int keycode) const;

private:
  int keycode_per_track_[kMaxLaneCount][4];
};

extern SystemPreference *PREFERENCE;
extern MetricGroup      *METRIC;

}
