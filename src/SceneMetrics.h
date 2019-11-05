#pragma once

#include <string>
#include <map>
#include <vector>

namespace rhythmus
{
  
class Scene;
class BaseObject;

/* @brief Theme metrics used for elements */
class ThemeMetrics
{
public:
  ThemeMetrics(const std::string &name);
  ~ThemeMetrics();

  void set_name(const std::string &name);
  const std::string& name() const;

  bool exist(const std::string &key) const;
  bool get(const std::string &key, std::string &v) const;
  bool get(const std::string &key, int &v) const;
  bool get(const std::string &key, std::vector<std::string> &v) const;
  bool get(const std::string &key, std::vector<int> &v) const;

  typedef std::map<std::string, std::string>::iterator iterator;
  typedef std::map<std::string, std::string>::const_iterator const_iterator;
  iterator begin();
  const_iterator cbegin();
  iterator end();
  const_iterator cend();

private:
  std::string name_;
  std::map<std::string, std::string> attr_;
};

typedef std::map<std::string, ThemeMetrics> ThemeMetricsList;
typedef std::vector<ThemeMetrics> ThemeMetricsAppendList;

void LoadMetrics(const std::string &filename, Scene &scene);

BaseObject* CreateObjectFromMetrics(ThemeMetrics &metrics);

}