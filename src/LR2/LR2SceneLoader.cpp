#include "LR2SceneLoader.h"
#include "LR2Flag.h"
#include "Script.h"   /* flag_no */
#include "Util.h"
#include "rparser.h"  /* rutil: utf-8 file load */
#include <iostream>

namespace rhythmus
{

LR2SceneLoader::LR2SceneLoader()
  : substitute_path_from("LR2files/Theme")
{
}

LR2SceneLoader::~LR2SceneLoader()
{
}

void LR2SceneLoader::SetSubStitutePath(const std::string& sub_from, const std::string& theme_path)
{
  substitute_path_from = sub_from;
  substitute_path_to = theme_path;
}

std::string LR2SceneLoader::SubstitutePath(const std::string& path_)
{
  return Substitute(path_, substitute_path_from, substitute_path_to);
}

void LR2SceneLoader::Load(const std::string& path)
{
  scene_filepath_ = path;

  LoadCSV(path);
}

void LR2SceneLoader::LoadCSV(const std::string& filepath)
{
  rutil::FileData data;
  rutil::ReadFileData(filepath, data);
  if (data.IsEmpty())
    return;
  ParseCSV((char*)data.p, data.len);
}

int atoi_op(const char* op)
{
  if (*op == '!')
    return atoi(op + 1);
  else
    return atoi(op);
}

void LR2SceneLoader::ParseCSV(const char* p, size_t len)
{
  const char* p_end = p + len;
  const char* p_ls = p;
  while (*p == '\r' || *p == '\n' || *p == '\t' || *p == ' ')
  {
    if (p >= p_end) return;
    ++p, ++p_ls;
  }

  while (p < p_end)
  {
    while (*p != '\n' && *p != '\r' && p < p_end) ++p;
    std::string s(p_ls, p - p_ls);
    while (*p == '\n' || *p == '\r' || *p == ' ') ++p;
    p_ls = p;

    s = rutil::ConvertEncoding(s, rutil::E_UTF8, rutil::E_SHIFT_JIS);

    if (s.size() < 2) continue;
    if (s[0] != '#') continue;

    std::string cmd, val;
    rutil::split(s, ',', cmd, val);
    cmd = rutil::upper(cmd);

    /* conditional statement first */
    if (cmd == "#IF")
    {
      int cond = atoi_op(val.c_str());
      if (Script::getInstance().GetFlag(cond))
        if_stack_.emplace_back(IfStmt{ 0, false });
      else
        if_stack_.emplace_back(IfStmt{ 1, true });
      continue;
    }
    else if (cmd == "#ELSEIF")
    {
      if (if_stack_.empty())
        continue;
      if (if_stack_.back().cond_is_true ||
          if_stack_.back().cond_match_count > 0)
      {
        if_stack_.back().cond_is_true = false;
        continue;
      }

      int cond = atoi_op(val.c_str());
      if (Script::getInstance().GetFlag(cond))
      {
        if_stack_.back().cond_is_true = true;
        if_stack_.back().cond_match_count++;
      }
      continue;
    }
    else if (cmd == "#ELSE")
    {
      if (if_stack_.empty())
        continue;
      if (if_stack_.back().cond_is_true ||
          if_stack_.back().cond_match_count > 0)
      {
        if_stack_.back().cond_is_true = false;
        continue;
      }
      if_stack_.back().cond_is_true = true;
      if_stack_.back().cond_match_count++;
      continue;
    }
    else if (cmd == "#ENDIF")
    {
      if (if_stack_.empty())
      {
        std::cerr << "LR2Skin: #ENDIF without #IF statement, ignored." << std::endl;
        break;
      }
      if_stack_.pop_back();
      continue;
    }

    /* if conditional statement failed, cannot pass over it. */
    if (!if_stack_.empty() && !if_stack_.back().cond_is_true)
      continue;

    /* special command: INCLUDE */
    if (cmd == "#INCLUDE")
    {
      // before continue, need to change path of LR2
      while (val.back() == ',') val.pop_back();
      std::string path = SubstitutePath(val);
      LoadCSV(path);
      continue;
    }

    /* General commands : just add to commands */
    commands_.push_back({ cmd, val });
  }
}

}
