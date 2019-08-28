#include "LR2SceneLoader.h"
#include "LR2Flag.h"
#include "LR2Sprite.h"
#include "LR2Font.h"
#include "rutil.h" /* utf-8 file load */
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

void LR2SceneLoader::SetSubStitutePath(const std::string& theme_path)
{
  substitute_path_to = theme_path;
}

std::string LR2SceneLoader::SubstitutePath(const std::string& path_)
{
  std::string path = path_;
  for (int i = 0; i < path.size(); ++i)
    if (path[i] == '\\') path[i] = '/';
  if (strncmp(path.c_str(), "./", 2) == 0)
    path = path.substr(2);
  if (strnicmp(path.c_str(),
    substitute_path_from.c_str(),
    substitute_path_from.size()) == 0)
  {
    path = substitute_path_to + path.substr(substitute_path_from.size());
  }
  return path;
}

void LR2SceneLoader::Load(const std::string& path)
{
  scene_filepath_ = path;

  LoadCSV(path);
}

void MakeParamCountSafe(std::vector<std::string> &v, size_t expected_count)
{
  while (v.size() < expected_count)
    v.emplace_back(std::string());
}

void LR2SceneLoader::LoadCSV(const std::string& filepath)
{
  auto data = rutil::ReadFileData(filepath);
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
      if (LR2Flag::GetFlag(cond))
        if_stack_.emplace_back(IfStmt{ 0, false });
      else
        if_stack_.emplace_back(IfStmt{ 1, true });
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
      if (LR2Flag::GetFlag(cond))
      {
        if_stack_.back().cond_is_true = true;
        if_stack_.back().cond_match_count++;
      }
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
    }
    else if (cmd == "#ENDIF")
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

    /* special command: INCLUDE */
    if (cmd == "#INCLUDE")
    {
      // before continue, need to change path of LR2
      std::string path = SubstitutePath(val);
      LoadCSV(path);
    }

    /* General commands : just add to commands */
    commands_.push_back({ cmd, val });

#if 0
    if (strncmp(cmd.c_str(), "#SRC_", 5) == 0)
    {
      std::string objtype = params[0].substr(5);

      MakeParamCountSafe(params, 14);
      int attr = atoi(params[1].c_str());
      int imgidx = atoi(params[2].c_str());
      int sx = atoi(params[3].c_str());
      int sy = atoi(params[4].c_str());
      int sw = atoi(params[5].c_str());
      int sh = atoi(params[6].c_str());
      int divx = atoi(params[7].c_str());
      int divy = atoi(params[8].c_str());
      int cycle = atoi(params[9].c_str()); /* total loop time */
      int timer = atoi(params[10].c_str()); /* timer id in LR2 form */
      int op1 = atoi_op(params[11].c_str());
      int op2 = atoi_op(params[12].c_str());
      int op3 = atoi_op(params[13].c_str());
      LR2SpriteSRC src = { attr,
          imgidx, sx, sy, sw, sh, divx, divy,
          cycle, timer, op1, op2, op3
      };

      LR2SprInfo* lr2_sprinfo = nullptr;
      /* BAR_FLASH is same as general sprite */
      if (objtype == "IMAGE")
      {
        sprites_.emplace_back(std::make_unique<LR2Sprite>());
        lr2_sprinfo = &((LR2Sprite*)sprites_.back().get())->get_sprinfo();
      }
      else if (objtype == "TEXT")
      {
        sprites_.emplace_back(std::make_unique<LR2Text>());
        lr2_sprinfo = &((LR2Text*)sprites_.back().get())->get_sprinfo();
      }
      else if (objtype == "BAR_BODY")
      {
        /* Do exceptional process */
        if (sprites_.back()->get_name() != "LR2SpriteSelectBar")
          sprites_.emplace_back(std::make_unique<LR2SpriteSelectBar>());
        ((LR2SpriteSelectBar*)sprites_.back().get())->get_bar_src(attr) = src;
        continue;
      }
      else if (objtype == "BAR_TITLE" ||
              objtype == "BAR_LEVEL" ||
              objtype == "BAR_FLASH" ||
              objtype == "BAR_LAMP" ||
              objtype == "BAR_MY_LAMP" ||
              objtype == "BAR_RIVAL_LAMP" ||
              objtype == "BAR_RIVAL")
      {
        sprites_.emplace_back(std::make_unique<LR2SpriteDummy>());
        lr2_sprinfo = &((LR2SpriteDummy*)sprites_.back().get())->get_sprinfo();
      }
      else
        std::cerr << "LR2SceneLoader: Unknown SRC objtype - " << objtype << std::endl;

      if (lr2_sprinfo)
        lr2_sprinfo->get_src() = src;
    }
    else if (strncmp(params[0].c_str(), "#DST_", 5) == 0)
    {
      if (sprites_.size() == 0)
      {
        std::cout << "LR2Skin Load warning : DST command found without SRC, ignored." << std::endl;
        continue;
      }

      std::string objtype = params[0].substr(5);

      MakeParamCountSafe(params, 21);
      int attr = atoi(params[1].c_str());
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
      int op1 = atoi_op(params[18].c_str());
      int op2 = atoi_op(params[19].c_str());
      int op3 = atoi_op(params[20].c_str());
      LR2SpriteDST dst = { attr,
          time, x, y, w, h, acc_type, a, r, g, b,
          blend, filter, angle, center, loop, timer,
          op1, op2, op3
      };

      LR2SprInfo* lr2_sprinfo = nullptr;
      if (objtype == "IMAGE")
        lr2_sprinfo = &((LR2Sprite*)sprites_.back().get())->get_sprinfo();
      else if (objtype == "TEXT")
        lr2_sprinfo = &((LR2Text*)sprites_.back().get())->get_sprinfo();
      else if (objtype == "BAR_BODY_OFF")
        lr2_sprinfo = &((LR2SpriteSelectBar*)sprites_.back().get())->get_bar_sprinfo(attr, 0);
      else if (objtype == "BAR_BODY_ON")
        lr2_sprinfo = &((LR2SpriteSelectBar*)sprites_.back().get())->get_bar_sprinfo(attr, 1);
      else if (objtype == "BAR_TITLE" ||
              objtype == "BAR_LEVEL" ||
              objtype == "BAR_FLASH" ||
              objtype == "BAR_LAMP" ||
              objtype == "BAR_MY_LAMP" ||
              objtype == "BAR_RIVAL_LAMP" ||
              objtype == "BAR_RIVAL")
        lr2_sprinfo = &((LR2SpriteDummy*)sprites_.back().get())->get_sprinfo();
      else
        std::cerr << "LR2SceneLoader: Unknown DST objtype - " << objtype << std::endl;

      if (lr2_sprinfo)
      {
        lr2_sprinfo->new_dst();
        lr2_sprinfo->get_cur_dst() = dst;
      }
    }
    else if (params[0] == "#BAR_CENTER" || params[0] == "#BAR_AVAILABLE")
    {
      // TODO: set attribute to theme_param_
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
      imgnames_.push_back(params[1]);
    }
    else if (params[0] == "#FONT")
    {
      MakeParamCountSafe(params, 2);
      fontnames_.push_back(params[1]);
    }
    else if (params[0] == "#LR2FONT")
    {
      MakeParamCountSafe(params, 2);
      lr2fontnames_.push_back(params[1]);
    }
    /*
    else if (params[0] == "#HELPFILE")
    {
    }*/
#endif
  }
}

}