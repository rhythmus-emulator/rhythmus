#pragma once

#include "Scene.h"

namespace rhythmus
{

class LR2SceneLoader : public SceneLoader
{
public:
  LR2SceneLoader(Scene *s);
  ~LR2SceneLoader();
  virtual void Load(const std::string& filepath);

private:
  std::string scene_filepath_;
  std::string folder_;
  std::string base_theme_folder_;

  struct IfStmt
  {
    int cond_match_count;
    bool cond_is_true;
  };

  std::vector<IfStmt> if_stack_;

  std::vector<std::string> imgnames_;     // #IMAGE param
  std::vector<std::string> fontnames_;    // #FONT param (not used)
  std::vector<std::string> lr2fontnames_; // #LR2FONT param

  void LoadCSV(const std::string& filepath);
  void ParseCSV(const char* p, size_t len);
  std::string ConvertLR2Path(const std::string& lr2path);

  /* @brief Get theme option by name. nullptr if no name found. */
  ThemeOption* GetThemeOption(const std::string& option_name);
};

}