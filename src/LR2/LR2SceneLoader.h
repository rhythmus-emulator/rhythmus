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

  struct IfStmt
  {
    int cond_match_count;
    bool cond_is_true;
  };

  std::vector<IfStmt> if_stack_;

  void LoadCSV(const std::string& filepath);
  void ParseCSV(const char* p, size_t len);
  void GetImagePath(const std::string& value);
  std::string ConvertLR2Path(const std::string& lr2path);
};

}