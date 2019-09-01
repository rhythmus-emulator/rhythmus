#include "Scene.h"
#include "SceneManager.h"       /* preference */
#include "LR2/LR2SceneLoader.h" /* for lr2skin file load */
#include "LR2/LR2Sprite.h"
#include "LR2/LR2Font.h"
#include "rutil.h"              /* for string modification */
#include <iostream>

namespace rhythmus
{

// -------------------------- ThemeOption

void ThemeOption::GetSelectionList(std::vector<std::string> &list)
{
  // TODO
}

std::string ThemeOption::GetSelectedValue()
{
  switch (type)
  {
  case 2:
  {
    std::string path;
    const char* opt = options.c_str();
    while (*opt)
    {
      if (*opt == '*')
        path += selected;
      else
        path.push_back(*opt);
      opt++;
    }
    // TODO: if file is not exist, then try to GetSelectionList and set as first one
    return path;
  }
  case 1:
  default:
    return selected;
  }

  // NOT REACHABLE
  return "";
}


// -------------------------------- Scene

void Scene::LoadScene()
{
  // Check is preference file exist for current scene
  // If so, load that file to actor.
  std::string scene_name = get_name();
  if (!scene_name.empty())
  {
    std::string scene_path =
      Game::getInstance().GetAttribute<std::string>(scene_name);
    if (!scene_path.empty())
    {
      if (rutil::endsWith(scene_path, ".lr2skin", false))
      {
        LoadFromCsv(scene_path);
      }
      else
      {
        std::cerr << "[Error] Scene " << scene_name <<
          " does not support file: " << scene_path << std::endl;
      }
    }
    LoadOptions();
  }
  else
  {
    std::cerr << "[Warning] Scene has no name." << std::endl;
  }
}

void Scene::CloseScene()
{
  if (!get_name().empty())
    SaveOptions();
}

void Scene::RegisterImage(ImageAuto img)
{
  images_.push_back(img);
}

ImageAuto Scene::GetImageByName(const std::string& name)
{
  for (const auto& img : images_)
    if (img->get_name() == name) return img;
  return nullptr;
}

void Scene::LoadOptions()
{
  ASSERT(get_name().size() > 0);
  auto& setting = SceneManager::getSetting();
  setting.SetPreferenceGroup(get_name());

  SettingList slist;
  setting.GetAllPreference(slist);
  for (auto ii : slist)
  {
    SetThemeConfig(ii.first, ii.second);
  }
}

void Scene::SaveOptions()
{
  ASSERT(get_name().size() > 0);
  auto& setting = SceneManager::getSetting();
  setting.SetPreferenceGroup(get_name());

  for (auto& t : theme_options_)
  {
    setting.Set(t.id, t.selected);
  }
}

void Scene::doUpdate(float delta)
{
  // Update images
  for (const auto& img: images_)
    img->Update(delta);
}

constexpr char* kSubstitutePath = "../themes";

void Scene::LoadProperty(const std::string& prop_name, const std::string& value)
{
  static std::string _prev_prop_name = prop_name;
  std::vector<std::string> params;

  // LR2 type properties
  if (strncmp(prop_name.c_str(), "#SRC_", 5) == 0)
  {
    // If #SRC came in serial, we need to reuse last object...
    // kinda trick: check previous command
    BaseObject* obj = nullptr;
    if (_prev_prop_name == prop_name)
      obj = GetLastChild();
    else
    {
      std::string type_name = prop_name.substr(5);
      if (type_name == "IMAGE")
        obj = new LR2Sprite();
      else if (type_name == "TEXT")
        obj = new LR2Text();
      else
      {
        obj = new BaseObject();
        obj->set_name(type_name);
      }
      RegisterChild(obj);
      AddChild(obj);
    }
    obj->LoadProperty(prop_name, value);
  }
  else if (strncmp(prop_name.c_str(), "#DST_", 5) == 0)
  {
    BaseObject* obj = GetLastChild();
    if (!obj)
    {
      std::cout << "LR2Skin Load warning : DST command found without SRC, ignored." << std::endl;
      return;
    }
    // skip BAR related object now
    if (prop_name == "#DST_BAR_BODY_ON" || prop_name == "#DST_BAR_BODY_OFF")
      return;
    obj->LoadProperty(prop_name, value);
  }
  // XXX: register such objects first?
  else if (prop_name == "#IMAGE")
  {
    // TODO: nasty, need to remove this code
    LR2SceneLoader loader;
    loader.SetSubStitutePath(kSubstitutePath);

    ImageAuto img;
    std::string imgname = GetFirstParam(value);
    std::string imgpath;
    ThemeOption* option = GetThemeOption(imgname);
    if (option)
    {
      // create img path from option
      imgpath = option->GetSelectedValue();
    }
    else
    {
      imgpath = imgname;
    }
    imgpath = loader.SubstitutePath(imgpath);
    img = ResourceManager::getInstance().LoadImage(imgpath);
    // TODO: set colorkey
    img->CommitImage();
    char name[10];
    itoa(images_.size(), name, 10);
    img->set_name(name);
    images_.push_back(img);
  }
  else if (prop_name == "#FONT")
  {
    // TODO: nasty, need to remove this code
    LR2SceneLoader loader;
    loader.SetSubStitutePath(kSubstitutePath);

    std::string fntname = GetFirstParam(value);
    std::string fntpath = loader.SubstitutePath(fntname);
    // convert filename path to .dxa
    auto ri = fntpath.rfind('/');
    if (ri != std::string::npos && stricmp(fntpath.substr(ri).c_str(), "/font.lr2font") == 0)
      fntpath = fntpath.substr(0, ri) + ".dxa";
    char name[10];
    itoa(fonts_.size(), name, 10);
    fonts_.push_back(ResourceManager::getInstance().LoadLR2Font(fntpath));
    fonts_.back()->set_name(name);
  }
  else if (prop_name == "#BAR_CENTER" || prop_name == "#BAR_AVAILABLE")
  {
    // TODO: set attribute to theme_param_
  }
  else if (prop_name == "#INFORMATION")
  {
    MakeParamCountSafe(value, params, 4);
    theme_param_.gamemode = params[0];
    theme_param_.title = params[1];
    theme_param_.maker = params[2];
    theme_param_.preview = params[3];
  }
  else if (prop_name == "#CUSTOMOPTION")
  {
    MakeParamCountSafe(value, params, 4);
    ThemeOption options;
    options.type = 1;
    options.id = options.name = params[1];
    options.desc = params[0];
    int i;
    for (i = 2; i < params.size() - 1; ++i)
      options.options += params[i] + ",";
    options.options.pop_back();
    options.selected = params[i];
    theme_options_.push_back(options);
  }
  else if (prop_name == "#CUSTOMFILE")
  {
    MakeParamCountSafe(value, params, 3);
    ThemeOption options;
    options.type = 2;
    options.id = options.name = params[1];
    options.desc = params[0];
    options.options = params[1];
    options.selected = params[2];
    theme_options_.push_back(options);
  }
  else if (prop_name == "#TRANSCLOLR")
  {
    MakeParamCountSafe(value, params, 3);
    theme_param_.transcolor[0] = atoi(params[0].c_str());
    theme_param_.transcolor[1] = atoi(params[1].c_str());
    theme_param_.transcolor[2] = atoi(params[2].c_str());
  }
  else if (prop_name == "#STARTINPUT")
  {
    std::string v = GetFirstParam(value);
    theme_param_.begin_input_time = atoi(v.c_str());
  }
  else if (prop_name == "#FADEOUT")
  {
    std::string v = GetFirstParam(value);
    theme_param_.fade_out_time = atoi(GetFirstParam(value).c_str());
  }
  else if (prop_name == "#FADEIN")
  {
    std::string v = GetFirstParam(value);
    theme_param_.fade_in_time = atoi(GetFirstParam(value).c_str());
  }
  /*
  else if (prop_name == "#LR2FONT")
  {
    MakeParamCountSafe(params, 1);
    lr2fontnames_.push_back(params[1]);
    return true;
  }
  else if (prop_name == "#HELPFILE")
  {
  }*/
}

void Scene::LoadFromCsv(const std::string& filepath)
{
  LR2SceneLoader loader;
  loader.SetSubStitutePath(kSubstitutePath);
  loader.Load(filepath);

  for (auto &v : loader)
  {
    LoadProperty(v.first, v.second);
  }
}

void Scene::SetThemeConfig(const std::string& key, const std::string& value)
{
  for (auto& t : theme_options_)
  {
    if (t.id == key)
      t.selected = value;
    return;
  }
}

ThemeOption* Scene::GetThemeOption(const std::string& key)
{
  for (auto& t : theme_options_)
  {
    if (t.id == key)
      return &t;
  }
  return nullptr;
}

void Scene::GetThemeOptionSelectList(
  const ThemeOption &option, std::vector<std::string>& selectable)
{
}

}