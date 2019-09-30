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

namespace rhythmus
{

class Setting;
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
  const std::string& ns() const;
  void set_namespace(const std::string& ns);
  void set_description(const std::string& desc);

  // @brief get option string used when calling SetOption()
  const std::string& get_option_string() const;

  // @brief get value as int (or index)
  int value_int() const;
  int value_min() const;
  int value_max() const;

  void set_op(int op);
  int value_op() const;
  bool is_constraint() const;

  // @brief select next selectable item / value
  void Next();

  // @brief select previous selectable item / value
  void Prev();

  // @brief set general option
  void SetTextOption();

  // @brief separate selectable options with comma
  void SetOption(const std::string& options);

  // @brief file mask(filter) option
  void SetFileOption(const std::string& file_filter);

  // @brief set as numeric option
  void SetNumberOption(int min = 0, int max = 0x7FFFFFFF);

  // @brief set value
  // @warn may be value is changed due to validation check
  //       in case of constraint exists.
  void set_value(const std::string& value);
  void set_value(int value);

  // @brief whether to save with constraint if it exists
  void save_with_constraint(bool v = true);

  // @brief copy constraint from other option
  void CopyConstraint(const Option& option);

  friend class Setting;

private:
  std::string name_;
  std::string ns_; /* namespace */
  std::string desc_;
  std::string value_;
  std::string type_;

  // selectable options, if necessary.
  std::vector<std::string> options_;

  // option for original string
  std::string option_filter_;

  // selected value (for number)
  int number_;

  // constraint for value (for number)
  int number_min_, number_max_;

  // kind of id constrain (for LR2)
  int op_;

  // save with constraint, not only option value.
  // (just for option item but not want to save the options)
  bool save_with_constraint_;

  // @brief validate option if it's not existing in selectable options.
  // @param valid_using_number use number first to check validation
  void Validate(bool valid_using_number = false);
};

/* @brief Xml setting load & save class
 * @warn  Key is not duplicable. */
class Setting
{
public:
  Setting();
  ~Setting();

  bool Open(const std::string& setting_path);
  bool Save();
  bool SaveAs(const std::string& setting_path);
  void Close();

  // @brief only reloads value, not loading entire options / attributes.
  bool ReloadValues(const std::string& setting_path = std::string());

  // @brief set scope for searching option
  void SetNamespace(const std::string& ns = "");

  /* @brief if failed to find specified node, then calls LoadPref internally. */
  template <typename T>
  void LoadOptionValue(const std::string& key, T& value) const
  {
    const Option *opt = GetOption(key);
    if (!opt)
      return;
    ConvertFromString(opt->value(), value);
  }

  /* @brief Set setting with node with specified name. */
  template <typename T>
  void SetOptionValue(const std::string& key, const T& value)
  {
    Option *opt = GetOption(key);
    if (!opt)
      return;
    std::string v = ConvertToString(value);
    opt->set_value(v);
  }

  const Option* GetOption(const std::string& name) const;
  Option* GetOption(const std::string& name);

  /* @brief Set setting with pref node, which can store option setting. */
  Option* NewOption(const std::string& option_name, bool get_if_exists = true);

  void GetAllOptions(std::vector<Option*> out);

  bool Exist(const std::string& key) const;

  /**
   * @brief Get path from path filter
   * @warn input string itself will be returned if no proper file option is found
   */
  std::string GetPathFromOptions(const std::string& path_filter);

  /* @brief command to settings. */
  void LoadProperty(const std::string& prop_name, const std::string& value);

private:
  std::string path_;
  std::string ns_; /* currently saelected namespace */
  std::vector<Option*> options_;
};

template <typename T>
void ConvertFromString(const std::string& src, T& dst);

template <typename T>
std::string ConvertToString(const T& dst);

}