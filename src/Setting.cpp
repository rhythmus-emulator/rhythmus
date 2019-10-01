#include "Setting.h"
#include "Util.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include "tinyxml2.h"

using namespace tinyxml2;

namespace rhythmus
{

constexpr char* kDefaultRootTagName = "setting";

inline const char* GetSafeString(const char* p)
{
  return p ? p : "";
}

// ----------------------------- class Option

Option::Option(const std::string& name)
  : name_(name), type_("text"),
    number_(0), number_min_(0), number_max_(0x7FFFFFFF),
    op_(0), save_with_constraint_(false) {}

const std::string& Option::name() const
{
  return name_;
}

const std::string& Option::value() const
{
  return value_;
}

const std::string& Option::type() const
{
  if (save_with_constraint_)
    return "text";
  return type_;
}

const std::string& Option::desc() const
{
  return desc_;
}

const std::string& Option::ns() const
{
  return ns_;
}

void Option::set_namespace(const std::string& ns)
{
  ns_ = ns;
}

void Option::set_description(const std::string& desc)
{
  desc_ = desc;
}

const std::string& Option::get_option_string() const
{
  return option_filter_;
}

int Option::value_int() const
{
  return number_min_ + number_;
}

int Option::value_min() const
{
  return number_min_;
}

int Option::value_max() const
{
  return number_max_;
}

void Option::set_op(int op)
{
  op_ = op;
}

int Option::value_op() const
{
  return op_;
}

bool Option::is_constraint() const
{
  return save_with_constraint_;
}

void Option::Next()
{
  number_ = std::min(number_ + 1, number_max_);

  if (type_ == "number")
  {
    value_ = std::to_string(number_);
  }
  else /* file, option */
  {
    value_ = options_[number_];
  }
}

void Option::Prev()
{
  number_ = std::max(number_ - 1, number_min_);

  if (type_ == "number")
  {
    value_ = std::to_string(number_);
  }
  else /* file, option */
  {
    value_ = options_[number_];
  }
}

void Option::SetTextOption()
{
  type_ = "text";
  option_filter_.clear();
  options_.clear();
}

void Option::SetOption(const std::string& options)
{
  type_ = "option";
  option_filter_ = options;
  Split(options, ',', options_);
}

void Option::SetFileOption(const std::string& file_filter)
{
  type_ = "file";
  option_filter_ = file_filter;
  GetFilesFromPath(file_filter, options_);
}

void Option::SetNumberOption(int min, int max)
{
  type_ = "number";
  number_min_ = min;
  number_max_ = max;
}

void Option::set_value(const std::string& option)
{
  value_ = option;
  number_ = atoi(option.c_str());
  Validate();
}

void Option::set_value(int value)
{
  number_ = value;
  value_ = std::to_string(value);
  Validate(true);
}

void Option::save_with_constraint(bool v)
{
  save_with_constraint_ = v;
}

// @brief validate option properly after setting value_.
//        internally called in SetSelected(). for internal use.
void Option::Validate(bool valid_using_number)
{
  if (type_ == "text")
  {
    /* do nothing */
  }
  if (type_ == "number")
  {
    if (number_ > number_max_ || number_ < number_min_)
    {
      // constraint failed, do re-validate
      number_ = std::min(std::max(number_, number_min_), number_max_);
      value_ = std::to_string(number_);
      Validate();
    }
  }
  else /* file, option */
  {
    if (options_.empty())
      return;

    if (!valid_using_number)
    {
      auto iter = std::find(options_.begin(), options_.end(), value_);
      if (iter == options_.end())
      {
        // constraint failed, set it to first value (or default)
        value_ = options_.front();
        number_ = 0;
      }
      else
      {
        number_ = (int)(iter - options_.begin());
      }
    }
    else
    {
      if (options_.empty())
      {
        number_ = 0;
        value_.clear();
        return;
      }
      number_ = std::max(0, std::min(number_, (int)options_.size() - 1));
      value_ = options_[number_];
    }
  }
}

void Option::CopyConstraint(const Option& option)
{
  desc_ = option.desc_;
  value_ = option.value_;
  type_ = option.type_;
  options_ = option.options_;
  option_filter_ = option.option_filter_;
  number_ = option.number_;
  number_min_ = option.number_min_;
  number_max_ = option.number_max_;
  op_ = option.op_;
}

// -------------------- xml related common code

Option* CreateOptionFromXmlElement(tinyxml2::XMLElement* e)
{
  /* validation check */
  if (!e) return nullptr;
  if (strcmp(e->Name(), kSettingOptionTagName) != 0) return nullptr;
  const char *name_p = e->Attribute("name");
  if (!name_p) return nullptr;

  Option *opt = new Option(name_p);

  /* no type attr --> no constraint */
  const char *type_p = e->Attribute("type");
  if (!type_p)
  {
    opt->save_with_constraint(false);
    type_p = "text";
  }
  std::string type = type_p;
  std::string value = GetSafeString(e->Attribute("value"));
  std::string option = GetSafeString(e->Attribute("options"));

  if (type == "option")
  {
    opt->SetOption(option);
  }
  else if (type == "file")
    opt->SetFileOption(option);
  else if (type == "number")
  {
    int min = 0, max = 0x7fffffff;
    if (e->Attribute("min")) min = atoi(e->Attribute("min"));
    if (e->Attribute("max")) max = atoi(e->Attribute("max"));
    opt->SetNumberOption(min, max);
  }
  else /* text */
    opt->SetTextOption();

  // special code for LR2 opt
  if (e->Attribute("op")) opt->set_op(atoi(e->Attribute("op")));

  opt->set_value(value);
  return opt;
}

tinyxml2::XMLElement*
CreateXmlElementFromOption(const Option& opt, tinyxml2::XMLDocument& doc)
{
  std::string type = opt.type();
  tinyxml2::XMLElement *e = doc.NewElement(kSettingOptionTagName);
  if (!e) return nullptr;

  e->SetName(kSettingOptionTagName);
  e->SetAttribute("name", opt.name().c_str());
  e->SetAttribute("value", opt.value().c_str());

  /* no constraint : no description (only value) */
  if (!opt.is_constraint() /* || type == "text"*/)
    return e;

  e->SetAttribute("type", type.c_str());
  if (!opt.desc().empty())
    e->SetAttribute("desc", opt.desc().c_str());
  if (!opt.get_option_string().empty())
    e->SetAttribute("options", opt.get_option_string().c_str());
  if (opt.value_min() )
    e->SetAttribute("options", opt.get_option_string().c_str());
  if (!opt.get_option_string().empty())
    e->SetAttribute("options", opt.get_option_string().c_str());
  // special case: LR2 op flag
  if (opt.value_op() > 0)
    e->SetAttribute("op", opt.value_op());

  if (type == "file" || type == "option")
  {
    e->SetAttribute("options", opt.get_option_string().c_str());
  }
  else if (type == "number")
  {
    e->SetAttribute("min", opt.value_min());
    e->SetAttribute("max", opt.value_max());
    // XXX: step attribute?
  }

  return e;
}

// ------------------------------ class Setting

Setting::Setting() {}

Setting::~Setting() { Close(); }

bool Setting::Open(const std::string& setting_path)
{
  path_ = setting_path;
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLElement *root, *ele, *ns_ele;
  XMLError errcode;

  if ((errcode = doc.LoadFile(setting_path.c_str())) != XML_SUCCESS)
  {
    std::cerr << "Game settings reading failed, TinyXml2 ErrorCode: " << errcode << std::endl;
    return false;
  }

  root = doc.RootElement();
  if (!root)
  {
    std::cerr << "Invalid xml setting file: no root node." << std::endl;
    return false;
  }

  for (ele = root->FirstChildElement(); ele; ele = ele->NextSiblingElement())
  {
    Option *option = CreateOptionFromXmlElement(ele);
    if (option)
    {
      options_.push_back(option);
    }
    else if (ele->FirstChildElement())
    {
      // nested options (namespace)
      std::string ns = ele->Name();
      for (ns_ele = ele->FirstChildElement(); ns_ele; ns_ele = ns_ele->NextSiblingElement())
      {
        option = CreateOptionFromXmlElement(ns_ele);
        if (option)
        {
          option->set_namespace(ns);
          options_.push_back(option);
        }
      }
    }
  }

  return true;
}

bool Setting::Save()
{
  if (path_.empty()) return false;
  tinyxml2::XMLDocument doc;
  XMLError errcode;
  XMLElement *root = doc.NewElement(kDefaultRootTagName);
  doc.InsertFirstChild(root);

  for (const auto* option : options_)
  {
    // get proper parent node (make/fetch parent if namespace exists)
    XMLElement *ele = nullptr, *parent = root;
    std::string ns = option->ns();
    if (!ns.empty())
    {
      parent = root->FirstChildElement(ns.c_str());
      if (!parent)
      {
        parent = doc.NewElement(ns.c_str());
        root->InsertEndChild(parent);
      }
    }

    // create xml element
    ele = CreateXmlElementFromOption(*option, doc);
    if (ele)
      parent->InsertEndChild(ele);
  }

  return doc.SaveFile(path_.c_str()) == XML_SUCCESS;
}

bool Setting::SaveAs(const std::string& setting_path)
{
  path_ = setting_path;
  return Save();
}

void Setting::Close()
{
  for (auto *p : options_)
    delete p;
  options_.clear();
}

bool Setting::ReloadValues(const std::string& setting_path)
{
  if (!setting_path.empty())
    path_ = setting_path;

  tinyxml2::XMLDocument doc;
  tinyxml2::XMLElement *root, *ele, *ns_ele;
  XMLError errcode;

  if ((errcode = doc.LoadFile(path_.c_str())) != XML_SUCCESS)
  {
    std::cerr << "Game settings reading failed, TinyXml2 ErrorCode: " << errcode << std::endl;
    return false;
  }

  root = doc.RootElement();
  if (!root)
  {
    std::cerr << "Invalid xml setting file: no root node." << std::endl;
    return false;
  }

  std::map<std::string, std::string> key_value_map;
  for (ele = root->FirstChildElement(); ele; ele = ele->NextSiblingElement())
  {
    if (strcmp(ele->Name(), kSettingOptionTagName) != 0)
      continue;

    std::string name = GetSafeString(ele->Attribute("name"));
    std::string value = GetSafeString(ele->Attribute("value"));
    if (name.empty())
      continue;
    key_value_map[name] = value;
  }

  doc.Clear();

  for (auto *opt : options_)
  {
    auto i = key_value_map.find(opt->name());
    if (i != key_value_map.end())
      opt->set_value(i->second);
  }

  return true;
}

void Setting::SetNamespace(const std::string& ns)
{
  ns_ = ns;
}

Option* Setting::GetOption(const std::string& name)
{
  for (auto *p : options_)
    if (p->name() == name && p->ns() == ns_) return p;
  return nullptr;
}

const Option* Setting::GetOption(const std::string& name) const
{
  return const_cast<Setting*>(this)->GetOption(name);
}

Option* Setting::NewOption(const std::string& name, bool get_if_exists)
{
  Option* opt = GetOption(name);
  if (opt)
  {
    if (get_if_exists) return opt;
    else return nullptr;
  }

  // from here, create new option object.
  opt = new Option(name);
  if (!ns_.empty()) opt->set_namespace(ns_);
  options_.push_back(opt);
  return opt;
}

void Setting::GetAllOptions(std::vector<Option*> out)
{
  for (auto *p : options_)
    if (p->ns() == ns_) out.push_back(p);
}

bool Setting::Exist(const std::string& key) const
{
  for (auto *p : options_)
    if (p->name() == key) return true;
  return false;
}

std::string Setting::GetPathFromOptions(const std::string& path_filter)
{
  for (auto *p : options_)
  {
    if (p->type() == "file" && p->get_option_string() == path_filter)
      return p->value();
  }

  return path_filter;
}

constexpr char* kLR2SubstitutePath = "LR2files/Theme";
constexpr char* kSubstitutePath = "../themes";

void Setting::LoadProperty(const std::string& prop_name, const std::string& value)
{
  std::vector<std::string> params;

  if (prop_name == "#CUSTOMOPTION")
  {
    MakeParamCountSafe(value, params, 4);
    std::string name = params[0];
    Option* option = NewOption(name);
    if (!option) return;

    std::string desc = params[0] + " with op code " + params[1];
    int op_code = atoi(params[1].c_str());
    std::string options_str;
    std::string option_default;
    {
      int i;
      for (i = 2; i < params.size(); ++i)
        options_str += params[i] + ",";
      options_str.pop_back();
      option_default = params[2]; /* first option is default */
    }

    option->set_description(desc);
    option->SetOption(options_str);
    //option->set_value(option_default);
    option->value_ = option_default;  // set without validate
  }
  else if (prop_name == "#CUSTOMFILE")
  {
    MakeParamCountSafe(value, params, 3);
    std::string name = params[0];
    Option* option = NewOption(name);
    if (!option) return;

    std::string desc = "supported file: " + params[1];
    std::string path_prefix = Substitute(params[1], kLR2SubstitutePath, kSubstitutePath);

    option->set_description(desc);
    option->SetFileOption(path_prefix);
    //option->set_value(Replace(path_prefix, "*", params[2]));
    option->value_ = path_prefix;  // set without validate
  }
}

void Setting::ValidateAll()
{
  for (auto *opt : options_)
    opt->Validate();
}


// ------------------------ Serialization

template <>
void ConvertFromString(const std::string& src, std::string& dst)
{
  dst = src;
}

template <>
void ConvertFromString(const std::string& src, int& dst)
{
  // for LR2 style
  if (src.size() > 0 && src[0] == '!')
    dst = -atoi(src.c_str() + 1);
  else
    dst = atoi(src.c_str());
}

template <>
void ConvertFromString(const std::string& src, double& dst)
{
  dst = atof(src.c_str());
}

template <>
void ConvertFromString(const std::string& src, bool& dst)
{
  dst = (stricmp(src.c_str(), "true") == 0);
}

template <>
void ConvertFromString(const std::string& src, unsigned short& dst)
{
  dst = static_cast<unsigned short>(atoi(src.c_str()));
}

template <>
void ConvertFromString(const std::string& src, size_t& dst)
{
  dst = static_cast<size_t>(atoi(src.c_str()));
}

template <>
void ConvertFromString(const std::string& src, std::vector<std::string>& dst)
{
  // separated by comma
  std::istringstream ss(src);
  std::string token;
  while (std::getline(ss, token, ',')) {
    dst.push_back(token);
  }
}

template <>
void ConvertFromString(const std::string& src, std::vector<int>& dst)
{
  // separated by comma
  std::istringstream ss(src);
  std::string token;
  while (std::getline(ss, token, ',')) {
    dst.push_back(atoi(token.c_str()));
  }
}

template <>
void ConvertFromString(const std::string& src, std::vector<double>& dst)
{
  // separated by comma
  std::istringstream ss(src);
  std::string token;
  while (std::getline(ss, token, ',')) {
    dst.push_back(atof(token.c_str()));
  }
}



// ---------------------- Deserialization

template <>
std::string ConvertToString(const std::string& dst)
{
  return dst;
}

template <>
std::string ConvertToString(const int& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const unsigned& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const unsigned short& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const double& dst)
{
  return std::to_string(dst);
}

template <>
std::string ConvertToString(const bool& dst)
{
  return dst ? "true" : "false";
}

template <>
std::string ConvertToString(const std::vector<std::string>& dst)
{
  std::stringstream ss;
  unsigned i = 0;
  for (auto& s : dst)
  {
    ss << s;
    if (++i < dst.size()) ss << ",";
  }
  return ss.str();
}

template <>
std::string ConvertToString(const std::vector<int>& dst)
{
  std::stringstream ss;
  unsigned i = 0;
  for (auto& s : dst)
  {
    ss << s;
    if (++i < dst.size()) ss << ",";
  }
  return ss.str();
}

template <>
std::string ConvertToString(const std::vector<double>& dst)
{
  std::stringstream ss;
  unsigned i = 0;
  for (auto& s : dst)
  {
    ss << s;
    if (++i < dst.size()) ss << ",";
  }
  return ss.str();
}

}