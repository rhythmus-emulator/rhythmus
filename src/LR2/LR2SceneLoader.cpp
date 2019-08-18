#include "LR2SceneLoader.h"
#include "rutil.h" /* utf-8 file load */
#include "LR2Flag.h"
#include "LR2Sprite.h"
#include <iostream>

namespace rhythmus
{

LR2SceneLoader::LR2SceneLoader(Scene *s) : SceneLoader(s)
{
}

LR2SceneLoader::~LR2SceneLoader()
{
}

void LR2SceneLoader::Load(const std::string& path)
{
  scene_filepath_ = path;
  folder_ = rutil::GetDirectory(path);

  LoadCSV(path);

  // set all LR2Sprites valid
  for (auto s : sprites_)
  {
    LR2Sprite* sp = (LR2Sprite*)s.get();
    if (images_.size() < sp->get_src().imgidx)
    {
      // TODO: Some imgidx is special(e.g. CoverImage, BG ...)
      // need to deal with it.
      std::cerr << "LR2SceneLoader: Sprite invalid imgidx(" <<
        sp->get_src().imgidx << "), ignored." << std::endl;
      continue;
    }
    sp->SetImage(images_[sp->get_src().imgidx]);
    sp->SetSpriteFromLR2Data();
  }
}

void MakeParamCountSafe(std::vector<std::string> &v, size_t expected_count)
{
  while (v.size() < expected_count)
    v.push_back("");
}

void LR2SceneLoader::LoadCSV(const std::string& filepath)
{
  auto data = rutil::ReadFileData(filepath);
  if (data.IsEmpty())
    return;
  ParseCSV((char*)data.p, data.len);
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

    std::vector<std::string> params;
    rutil::split(s, ',', params);
    params[0] = rutil::upper(params[0]);

    /* conditional statement first */
    if (params[0] == "#IF")
    {
      MakeParamCountSafe(params, 2);
      int cond = atoi(params[1].c_str());
      if (GetLR2Flag(cond))
        if_stack_.emplace_back(IfStmt{ 0, false });
      else
        if_stack_.emplace_back(IfStmt{ 1, true });
    }
    else if (params[0] == "#ELSEIF")
    {
      if (if_stack_.empty())
        continue;
      if (if_stack_.back().cond_is_true ||
          if_stack_.back().cond_match_count > 0)
      {
        if_stack_.back().cond_is_true = false;
        continue;
      }

      MakeParamCountSafe(params, 2);
      int cond = atoi(params[1].c_str());
      if (GetLR2Flag(cond))
      {
        if_stack_.back().cond_is_true = true;
        if_stack_.back().cond_match_count++;
      }
    }
    else if (params[0] == "#ELSE")
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
    }
    else if (params[0] == "#ENDIF")
    {
      if (if_stack_.empty())
      {
        std::cerr << "LR2Skin: #ENDIF without #IF statement, ignored." << std::endl;
        break;
      }
      if_stack_.pop_back();
    }

    /* if conditional statement failed, cannot pass over it. */
    if (!if_stack_.empty() && !if_stack_.back().cond_is_true)
      continue;

    /* general commands */
    if (params[0] == "#INCLUDE")
    {
      MakeParamCountSafe(params, 2);
      // before continue, need to change path of LR2
      std::string path = ConvertLR2Path(params[1]);
      LoadCSV(path);
    }
    else if (params[0] == "#INFORMATION")
    {
      MakeParamCountSafe(params, 5);
      theme_param_.gamemode = params[1];
      theme_param_.title = params[2];
      theme_param_.maker = params[3];
      theme_param_.preview = params[4];
    }
    else if (params[0] == "#CUSTOMOPTION")
    {
      MakeParamCountSafe(params, 5);
      ThemeOption options;
      options.type = 1;
      options.name = params[2];
      options.desc = params[1];
      int i;
      for (i = 3; i < params.size() - 1; ++i)
        options.options += params[i] + ",";
      options.options.pop_back();
      options.selected = params[i];
      theme_options_.push_back(options);
    }
    else if (params[0] == "#CUSTOMFILE")
    {
      MakeParamCountSafe(params, 4);
      ThemeOption options;
      options.type = 2;
      options.name = params[2];
      options.desc = params[1];
      options.options = params[2];
      options.selected = params[3];
      theme_options_.push_back(options);
    }
    else if (params[0] == "#TRANSCLOLR")
    {
      MakeParamCountSafe(params, 4);
      theme_param_.transcolor[0] = atoi(params[1].c_str());
      theme_param_.transcolor[1] = atoi(params[2].c_str());
      theme_param_.transcolor[2] = atoi(params[3].c_str());
    }
    else if (params[0] == "#STARTINPUT")
    {
      MakeParamCountSafe(params, 2);
      theme_param_.begin_input_time = atoi(params[1].c_str());
    }
    else if (params[0] == "#FADEOUT")
    {
      MakeParamCountSafe(params, 2);
      theme_param_.fade_out_time = atoi(params[1].c_str());
    }
    else if (params[0] == "#FADEIN")
    {
      MakeParamCountSafe(params, 2);
      theme_param_.fade_in_time = atoi(params[1].c_str());
    }
    else if (params[0] == "#IMAGE")
    {
      MakeParamCountSafe(params, 2);
      std::string imgpath = ConvertLR2Path(params[1]);
      ImageAuto img = ResourceManager::getInstance().LoadImage(imgpath);
      img->CommitImage();
      images_.push_back(img);
    }
    /*
    else if (params[0] == "#HELPFILE")
    {
    }*/
    else if (params[0] == "#SRC_IMAGE")
    {
      MakeParamCountSafe(params, 14);
      int imgidx = atoi(params[2].c_str());
      int sx = atoi(params[3].c_str());
      int sy = atoi(params[4].c_str());
      int sw = atoi(params[5].c_str());
      int sh = atoi(params[6].c_str());
      int divx = atoi(params[7].c_str());
      int divy = atoi(params[8].c_str());
      int cycle = atoi(params[9].c_str()); /* total loop time */
      int timer = atoi(params[10].c_str()); /* timer id in LR2 form */
      int op1 = atoi(params[11].c_str());
      int op2 = atoi(params[12].c_str());
      int op3 = atoi(params[13].c_str());

      sprites_.emplace_back(std::make_unique<LR2Sprite>());
      LR2Sprite* lr2_sprite = (LR2Sprite*)sprites_.back().get();
      lr2_sprite->get_src() = {
        imgidx, sx, sy, sw, sh, divx, divy,
        cycle, timer, op1, op2, op3
      };
    }
    else if (params[0] == "#DST_IMAGE")
    {
      if (sprites_.size() == 0)
      {
        std::cout << "LR2Skin Load warning : DST command found without SRC, ignored." << std::endl;
        continue;
      }

      MakeParamCountSafe(params, 21);
      int time = atoi(params[2].c_str());
      int x = atoi(params[3].c_str());
      int y = atoi(params[4].c_str());
      int w = atoi(params[5].c_str());
      int h = atoi(params[6].c_str());
      int acc_type = atoi(params[7].c_str());
      int a = atoi(params[8].c_str());
      int r = atoi(params[9].c_str());
      int g = atoi(params[10].c_str());
      int b = atoi(params[11].c_str());
      int blend = atoi(params[12].c_str());
      int filter = atoi(params[13].c_str());
      int angle = atoi(params[14].c_str());
      int center = atoi(params[15].c_str());
      int loop = atoi(params[16].c_str());
      int timer = atoi(params[17].c_str());
      int op1 = atoi(params[18].c_str());
      int op2 = atoi(params[19].c_str());
      int op3 = atoi(params[20].c_str());

      LR2Sprite* lr2_sprite = (LR2Sprite*)sprites_.back().get();
      lr2_sprite->get_cur_dst() = {
        time, x, y, w, h, acc_type, a, r, g, b,
        blend, filter, angle, center, loop, timer,
        op1, op2, op3
      };
    }
  }
}

void LR2SceneLoader::GetImagePath(const std::string& value)
{

}

// we need to change path of LR2 into general relative path.
std::string LR2SceneLoader::ConvertLR2Path(const std::string& lr2path)
{
  std::string path = lr2path;
  for (int i = 0; i < path.size(); ++i)
    if (path[i] == '\\') path[i] = '/';
  if (strncmp(path.c_str(), "./", 2) == 0)
    path = path.substr(2);
  if (strnicmp(path.c_str(), "LR2files/Theme", 14) == 0)
  {
    std::vector<std::string> path_seps;
    rutil::split(path, '/', path_seps);
    path = folder_;

    /* Need to check that folder depth is 3 or 4 */
    int fld_depth = 1;
    for (int i = 0; i < folder_.size(); ++i)
      if (folder_[i] == '/' || folder_[i] == '\\') fld_depth++;

    for (int i = fld_depth; i < path_seps.size(); ++i)
    {
      path += "/" + path_seps[i];
    }
  }
  return path;
}

}