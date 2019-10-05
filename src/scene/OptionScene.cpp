#include "OptionScene.h"

namespace rhythmus
{


struct OptionEntry
{
  std::string option_name;
  std::vector<std::string> option_list;
};
static std::vector<OptionEntry> option_entries;

void AddOption(const std::string& option_name,
  const std::initializer_list<std::string>& option_values)
{
  option_entries.emplace_back(OptionEntry());
  auto &e = option_entries.back();
  e.option_name = option_name;
  e.option_list = option_values;
}

void AddOption(const std::string& option_name,
  void (*lst)(std::vector<std::string>&))
{
  option_entries.emplace_back(OptionEntry());
  auto &e = option_entries.back();
  e.option_name = option_name;
  lst(e.option_list);
}

void EnumResolution(std::vector<std::string>& out)
{
  Graphic::GetResolutions(out);
}

void EnumUserList(std::vector<std::string>& out)
{
  // TODO
}

void EnumBaseTheme(std::vector<std::string>& out)
{
  // TODO
}

void EnumTheme(const std::string& filter, std::vector<std::string>& out)
{

}

void EnumSelectTheme(std::vector<std::string>& out) { EnumTheme("../themes/*/select/*.lr2skin", out); }
void EnumDecideTheme(std::vector<std::string>& out) { EnumTheme("../themes/*/decide/*.lr2skin", out); }
// TODO: 7k, 14k, 9k filter
void EnumPlayTheme(std::vector<std::string>& out) { EnumTheme("../themes/*/play/*.lr2skin", out); }
void EnumResultTheme(std::vector<std::string>& out) { EnumTheme("../themes/*/decide/*.lr2skin", out); }
void EnumCourseResultTheme(std::vector<std::string>& out) { EnumTheme("../themes/*/decide/*.lr2skin", out); }
void EnumThemeSound(std::vector<std::string>& out) { EnumTheme("../themes/*/decide/*.lr2ss", out); }

void BuildOptions()
{
  option_entries.clear();

  // Graphic, sound, and game settings
  AddOption("Resolution", EnumResolution);
  AddOption("SkinResolution", { "Auto", "640x480", "800x600", "1024x768", "1280x720", "1920x1080", "1920x1200" });
  AddOption("ScreenMode", { "Window", "Fullscreen (windowed)", "Fullscreen" });
  AddOption("GameMode", { "Home", "Arcade" });
  AddOption("Vsync", { "No", "Yes" });
  AddOption("SoundVolume", { "0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100" });
  AddOption("SoundCacheSize", { "1024", "2048", "4096", "8192", "16384" });
  AddOption("ThreadCount", { "1", "2", "3", "4", "6", "8" });

  // Gameplay settings
  AddOption("JudgeOffset", { "*number", "-999", "999", "1" });  /* number, min -999, max 999, updown 1 */
  AddOption("JudgeLevel", { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" });
  AddOption("ShowFastSlow", { "On", "Off" });
  AddOption("GameSpeed", { "0.5", "0.6", "0.7", "0.8", "0.9", "1.0", "1.1", "1.2", "1.3", "1.4", "1.5" });
  AddOption("ShowBga", { "On", "Off", "Autoplay" });
  AddOption("AutoJudgeOffset", { "On", "Off" });

  // User settings
  AddOption("CreateUser", { "NewUser" } );
  AddOption("Login", EnumUserList);
  AddOption("Online", { "Login", "Logout" } );
  AddOption("KeyConfig", { "Common", "4Key", "5Key", "7Key", "BmsSP", "BmsDP", "Popn", "Ez2Dj", "25Key" });

  // Theme settings
  AddOption("BaseTheme", EnumBaseTheme);
  AddOption("SelectTheme", EnumSelectTheme);
  AddOption("DecideTheme", EnumDecideTheme);
  AddOption("PlayTheme(7k)", EnumPlayTheme);
  AddOption("PlayTheme(14k)", EnumPlayTheme);
  AddOption("PlayTheme(9k)", EnumPlayTheme);
  AddOption("ResultTheme", EnumResultTheme);
  AddOption("CourseResultTheme", EnumCourseResultTheme);
  AddOption("ThemeCustomize", { "SelectTheme", "DecideTheme", "PlayTheme(7k)", "PlayTheme(14k)", "PlayTheme(9k)", "ResultTheme", "CourseResultTheme" });
  AddOption("ThemeSound", EnumThemeSound);
}

// -------------------------- class OptionScene
OptionScene::OptionScene() : option_mode_(1)
{
  BuildOptions();
  next_scene_mode_ = GameSceneMode::kGameSceneModeSelect;
}

void OptionScene::LoadScene()
{
  AddChild(&menu_);
  menu_.set_infinite_scroll(false);
  menu_.set_display_count(10);
  menu_.set_focus_min_index(1);
  menu_.set_focus_max_index(9);
  BuildOptionItemData();

  Scene::LoadScene();
}

void OptionScene::StartScene()
{
  // go to previous start / select scene
  // TODO: go to start scene
  Scene::StartScene();
}

void OptionScene::CloseScene()
{
  ApplyOptions();
  Scene::CloseScene();
}

bool OptionScene::ProcessEvent(const EventMessage& e)
{
  if (is_input_available())
    return true;

  if (e.IsKeyUp())
  {
    if (e.GetKeycode() == GLFW_KEY_ENTER)
    {
      EnterOptionItem();
    }
    else if (e.GetKeycode() == GLFW_KEY_ESCAPE)
    {
      EscapeOptionItem();
    }
  }
  return true;
}

void OptionScene::EnterOptionItem()
{
  OptionData &data = static_cast<OptionData&>(menu_.GetSelectedMenuData());

  // just change index of current selected data
  data.index = (data.index + 1) % data.values.size();
  data.value = data.values[data.index];
}

void OptionScene::EscapeOptionItem()
{
  //BuildOptionItemData();
  CloseScene();
}

void OptionScene::BuildOptionItemData()
{
  // 1. create option items from optiondata

  // 2. make index / value set from current option

}

void OptionScene::ApplyOptions()
{
  // set game option
}

}