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

class PrefData;
class KeySetting;
class MetricGroup;

extern PrefData* PREFDATA;
extern KeySetting* KEYPREF;
extern MetricGroup* METRIC;

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

/**
 * @brief Utility for get/setting Preference value
 */
template <typename T>
class MetricValue
{
public:
  MetricValue(const std::string& name) : name_(name), updated_(false)
  {
    R_ASSERT(METRIC != nullptr);
    _value_load();
  }
  MetricValue(const std::string& name, const T& _default) :
    name_(name), v_(_default), updated_(false)
  {
    R_ASSERT(METRIC != nullptr);
    _value_load();
  }
  ~MetricValue()
  {
    if (updated_) _value_set();
  }
  T& operator*() { return v_; }
  const T& operator*() const { return v_; }
  T& get() { return v_; }
  const T& get() const { return v_; }
  bool operator==(const T& v) const { return v_ == v; }
  T& operator=(const T& v) { v_ = v; updated_ = true; return v_; }
private:
  void _value_load();
  void _value_set();
  std::string name_;
  T v_;
  bool updated_;
};

/* TODO: data should contain constraint info?
 * no... it's "Option List", not option value */
struct OptionDesc
{
  std::string name;
  std::string constraint;
  std::string desc;
  std::string def;
};

class PrefData
{
public:
  PrefData();
  bool LoadOption(const std::string &path);
  bool LoadOptionFromMetric(const MetricGroup& m);
  bool LoadValue(const std::string &path);
  bool SaveValue(const std::string &path);
  bool SaveValue();
  void Clear();
  void set(const std::string& key, const std::string& value);
  const std::string* get(const std::string& key) const;
  std::string* get(const std::string& key);
  OptionDesc* get_option(const std::string& key);
  void add_option(const OptionDesc& option);
private:
  std::string path_;
  std::vector<OptionDesc> options_;
  std::map<std::string, std::string> values_;
  void InitValueWithOption(const OptionDesc& option);
};

/**
 * @brief Utility for get/setting Preference value
 */
template <typename T>
class PrefValue
{
public:
  PrefValue(const std::string& name) : name_(name), updated_(false)
  {
    R_ASSERT(PREFDATA != nullptr);
    _value_load();
  }
  PrefValue(const std::string& name, const T& _default) :
    name_(name), v_(_default), updated_(false)
  {
    R_ASSERT(PREFDATA != nullptr);
    _value_load();
  }
  ~PrefValue()
  {
    if (updated_) _value_set();
  }
  T& operator*() { return v_; }
  const T& operator*() const { return v_; }
  T& get() { return v_; }
  const T& get() const { return v_; }
  bool operator==(const T& v) const { return v_ == v; }
  T& operator=(const T& v) { v_ = v; updated_ = true; return v_; }
private:
  void _value_load();
  void _value_set();
  std::string name_;
  T v_;
  bool updated_;
};

class PrefOptionList
{
public:
  PrefOptionList(const std::string& name, const std::string& values);
  PrefOptionList(const std::string& name, const std::vector<std::string>& list);
  void set(const std::string& value);
  void set(unsigned i);
  const std::string& get() const;
  void next();
  void prev();
  unsigned index() const;
  const std::string& get_constraint() const;
private:
  PrefValue<std::string> v_;
  std::vector<std::string> list_;
  std::string desc_;
  unsigned index_;
protected:
  std::string constraint_;
};

class PrefOptionFile : public PrefOptionList
{
public:
  PrefOptionFile(const std::string& name, const std::string& path);
};

/* @brief Input setting for play. */
class KeySetting
{
public:
  KeySetting();
  void Default();
  void Load(const MetricGroup &metric);
  void Load(const std::string& s);
  void Save(MetricGroup &metric);
  void Save(std::string& s);
  int GetTrackFromKeycode(int keycode) const;

private:
  int keycode_per_track_[kMaxLaneCount][4];
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

}
