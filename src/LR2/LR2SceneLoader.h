#pragma once

#include "Scene.h"

namespace rhythmus
{

class LR2SceneLoader
{
public:
  LR2SceneLoader();
  ~LR2SceneLoader();
  void SetSubStitutePath(const std::string& theme_path);
  std::string SubstitutePath(const std::string& path);
  void Load(const std::string& filepath);

  std::vector<std::pair<std::string, std::string> >::iterator begin()
  {
    return commands_.begin();
  }

  std::vector<std::pair<std::string, std::string> >::iterator end()
  {
    return commands_.end();
  }

private:
  std::string scene_filepath_;
  std::string substitute_path_from;
  std::string substitute_path_to;

  struct IfStmt
  {
    int cond_match_count;
    bool cond_is_true;
  };

  std::vector<IfStmt> if_stack_;

  std::vector<std::pair<std::string, std::string> > commands_;

  void LoadCSV(const std::string& filepath);
  void ParseCSV(const char* p, size_t len);
};

int atoi_op(const char* op);

}