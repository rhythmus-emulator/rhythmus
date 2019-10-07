#include "Scene.h"
#include "SceneManager.h"       /* preference */
#include "LR2/LR2SceneLoader.h" /* for lr2skin file load */
#include "LR2/LR2Sprite.h"
#include "LR2/LR2Font.h"
#include "LR2/LR2Text.h"
#include "rutil.h"              /* for string modification */
#include "Util.h"
#include <iostream>

namespace rhythmus
{

// ----------------------- ThemeParameter

ThemeParameter::ThemeParameter()
{
  memset(transcolor, 0, sizeof(transcolor));
  begin_input_time = 0;
  fade_in_time = fade_out_time = 0;
  next_scene_time = 0;
}

// ---------------------------- SceneTask
SceneTask::SceneTask(const std::string& name, std::function<void()> func)
  : name_(name), func_(std::move(func)), wait_time_(0), is_ran_(false) {}

void SceneTask::wait_for(float wait_time)
{
  wait_time_ = wait_time;
}

void SceneTask::wait_cond(std::function<bool()> cond)
{
  cond_ = std::move(cond);
}

void SceneTask::run()
{
  if (func_)
    func_();
}

bool SceneTask::update(float delta)
{
  wait_time_ -= delta;
  if (wait_time_ <= 0)
  {
    bool cond_val = cond_ ? cond_() : true;
    if (cond_val)
    {
      return true;
    }
  }
  return false;
}

bool SceneTask::update_for_run(float delta)
{
  wait_time_ -= delta;
  if (wait_time_ <= 0)
  {
    bool cond_val = cond_ ? cond_() : true;
    if (cond_val)
    {
      if (!is_ran_)
        run();
      is_ran_ = true;
    }
  }
  return is_ran_;
}

// ------------------------ SceneTaskPool
SceneTaskQueue::SceneTaskQueue() : allow_enqueue_(true)
{}

void SceneTaskQueue::Enqueue(SceneTask *task)
{
  if (!allow_enqueue_)
    delete task;
  else
    tasks_.push_back(task);
}

void SceneTaskQueue::EnqueueFront(SceneTask *task)
{
  if (!allow_enqueue_)
    delete task;
  else
    tasks_.push_front(task);
}

void SceneTaskQueue::IsEnqueueable(bool allow_enqueue)
{
  allow_enqueue_ = allow_enqueue;
}

void SceneTaskQueue::FlushAll()
{
  for (auto *t : tasks_)
  {
    t->run();
    delete t;
  }
  tasks_.clear();
}

void SceneTaskQueue::DeleteAll()
{
  for (auto *t : tasks_)
    delete t;
  tasks_.clear();
}

void SceneTaskQueue::Update(float delta)
{
  if (tasks_.empty())
    return;

  if (tasks_.front()->update(delta))
  {
    SceneTask *task = tasks_.front();
    tasks_.pop_front();
    task->run();
    delete task;
  }
}

// -------------------------------- Scene

Scene::Scene()
  : fade_time_(0), fade_duration_(0), is_input_available_(true),
    focused_object_(nullptr), next_scene_mode_(GameSceneMode::kGameSceneClose)
{
}

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
        // @warn LoadFromCsv() also includes option loading.
        LoadFromCsv(scene_path);
      }
      else
      {
        std::cerr << "[Error] Scene " << scene_name <<
          " does not support file: " << scene_path << std::endl;
      }
    }
  }
  else
  {
    std::cerr << "[Warning] Scene has no name." << std::endl;
  }

  for (auto *obj : children_)
    obj->Load();
}

void Scene::StartScene()
{
  int event_time = 0;

  // trigger fade_in if it exist
  TriggerFadeIn();

  // constraints for theme parameters
  if (theme_param_.next_scene_time > 0)
  {
    if (theme_param_.begin_input_time > theme_param_.next_scene_time)
      theme_param_.begin_input_time = theme_param_.next_scene_time;
  }

  // set input avail time
  if (theme_param_.begin_input_time > 0)
  {
    EnableInput(false);
    SceneTask *task = new SceneTask("inputenableevent", [this] {
      EnableInput(true);
    });
    task->wait_for(theme_param_.begin_input_time);
    scenetask_.Enqueue(task);
    event_time += theme_param_.begin_input_time;
  }

  // set scenetime if necessary
  if (theme_param_.next_scene_time > 0)
  {
    EventManager::SendEvent(Events::kEventSceneChange);
    SceneTask *task = new SceneTask("timeoutevent", [this] {
      this->TriggerFadeOut();
      EventManager::SendEvent(Events::kEventSceneTimeout);
    });
    task->wait_for(theme_param_.next_scene_time - event_time);
    scenetask_.Enqueue(task);
    event_time += theme_param_.next_scene_time;
  }
}

void Scene::TriggerFadeOut()
{
  float duration = theme_param_.fade_out_time;
  if (duration <= 0)
  {
    // immediate close scene
    CloseScene();
    return;
  }

  // fade effect
  if (fade_duration_ != 0) return;
  fade_duration_ = -duration;
  fade_time_ = 0;

  // disable input & disable event enqueue
  EnableInput(false);
  SceneTask *task = new SceneTask("fadeoutevent", [this] {
    this->CloseScene();
  });
  task->wait_for(theme_param_.fade_out_time);
  scenetask_.DeleteAll();
  scenetask_.Enqueue(task);
  scenetask_.IsEnqueueable(false);
}

void Scene::TriggerFadeIn()
{
  float duration = theme_param_.fade_in_time;
  if (fade_duration_ != 0) return;
  fade_duration_ = duration;
  fade_time_ = 0;
}

void Scene::CloseScene()
{
  SaveOptions();
  Game::getInstance().SetNextScene(next_scene_mode_);
}

void Scene::EnableInput(bool enable_input)
{
  is_input_available_ = enable_input;
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

FontAuto Scene::GetFontByName(const std::string& name)
{
  for (const auto& font : fonts_)
    if (font->get_name() == name) return font;
  return nullptr;
}

void Scene::LoadOptions()
{
  /* no scene name --> return */
  if (get_name().empty())
    return;

  // attempt to load values from setting
  // (may failed but its okay)
  // XXX: we need more general option ... possible?
  setting_.ReloadValues("../config/" + get_name() + ".xml");

  // apply settings to system
  std::vector<Option*> opts;
  setting_.GetAllOptions(opts);
  for (auto *opt : opts)
  {
    if (opt->type() == "file")
    {
      /* file option */
      ResourceManager::getInstance()
        .AddPathReplacement(opt->get_option_string(), opt->value());
    }
    else
    {
      /* general option with op flag */
      if (opt->value_op() != 0)
        theme_param_.attributes[ std::to_string(opt->value_op()) ] = "true";
    }
  }

  // send event to update LR2Flag
  EventManager::SendEvent(Events::kEventSceneConfigLoaded);
}

void Scene::SaveOptions()
{
  /* no scene name --> return */
  if (get_name().empty())
    return;

  setting_.Save();
}

bool Scene::is_input_available() const
{
  return is_input_available_;
}

void Scene::doUpdate(float delta)
{
  // Update images
  for (const auto& img: images_)
    img->Update(delta);

  // scheduled events
  scenetask_.Update(delta);

  // fade in/out processing
  if (fade_duration_ != 0)
  {
    fade_time_ += delta;
    if (fade_duration_ > 0 && fade_time_ > fade_duration_)
    {
      fade_duration_ = 0;
      fade_time_ = 0;
    }
  }
}

void Scene::doRenderAfter()
{
  static VertexInfo vi[4] = {
    {0, 0, 0.1f, 0, 0, 1, 1, 1, 1},
    {0, 0, 0.1f, 1, 0, 1, 1, 1, 1},
    {0, 0, 0.1f, 1, 1, 1, 1, 1, 1},
    {0, 0, 0.1f, 0, 1, 1, 1, 1, 1}
  };

  // implementation of fadeout effect
  if (fade_duration_ != 0)
  {
    float fade_alpha_ = fade_duration_ > 0 ?
      1.0f - fade_time_ / fade_duration_ :
      fade_time_ / -fade_duration_;
    if (fade_alpha_ > 1)
      fade_alpha_ = 1;

    float w = Game::getInstance().get_window_width();
    float h = Game::getInstance().get_window_height();
    vi[1].x = w;
    vi[2].x = w;
    vi[2].y = h;
    vi[3].y = h;
    vi[0].a = vi[1].a = vi[2].a = vi[3].a = fade_alpha_;

    Graphic::SetTextureId(0);
    Graphic::SetBlendMode(GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(0, 0, 0);
    memcpy(Graphic::get_vertex_buffer(), vi, sizeof(VertexInfo) * 4);
    Graphic::RenderQuad();
  }
}

const ThemeParameter& Scene::get_theme_parameter() const
{
  return theme_param_;
}

constexpr char* kLR2SubstitutePath = "LR2files/Theme";
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

    if (obj)
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
    obj->LoadProperty(prop_name, value);
  }
  // XXX: register such objects first?
  else if (prop_name == "#IMAGE")
  {
    ImageAuto img;
    std::string imgname = GetFirstParam(value);
    std::string imgpath = Substitute(imgname, kLR2SubstitutePath, kSubstitutePath);
    img = ResourceManager::getInstance().LoadImage(imgpath);
    // TODO: set colorkey
    img->CommitImage();
    char name[10];
    itoa(images_.size(), name, 10);
    img->set_name(name);
    images_.push_back(img);
  }
  else if (prop_name == "#LR2FONT")
  {
    std::string fntname = GetFirstParam(value);
    std::string fntpath = Substitute(fntname, kLR2SubstitutePath, kSubstitutePath);
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
  else if (prop_name == "#TRANSCLOLR")
  {
    MakeParamCountSafe(value, params, 3);
    theme_param_.transcolor[0] = atoi(params[0].c_str());
    theme_param_.transcolor[1] = atoi(params[1].c_str());
    theme_param_.transcolor[2] = atoi(params[2].c_str());
  }
  else if (prop_name == "#STARTINPUT" || prop_name == "#IGNOREINPUT")
  {
    std::string v = GetFirstParam(value);
    theme_param_.begin_input_time = atoi(v.c_str());
  }
  else if (prop_name == "#FADEOUT")
  {
    theme_param_.fade_out_time = atoi(GetFirstParam(value).c_str());
  }
  else if (prop_name == "#FADEIN")
  {
    theme_param_.fade_in_time = atoi(GetFirstParam(value).c_str());
  }
  else if (prop_name == "#SCENETIME")
  {
    theme_param_.next_scene_time = atoi(GetFirstParam(value).c_str());
  }
  /*
  else if (prop_name == "#HELPFILE")
  {
  }*/
}

void Scene::LoadFromCsv(const std::string& filepath)
{
  LR2SceneLoader loader;
  loader.SetSubStitutePath("LR2files/Theme", kSubstitutePath);
  loader.Load(filepath);

  // first: do option loading
  for (auto &v : loader)
  {
    if (v.first == "#ENDOFHEADER")
      break;
    setting_.LoadProperty(v.first, v.second);
  }
  setting_.ValidateAll();

  // open option file (if exists)
  // must do it before loading elements, as option affects to it.
  LoadOptions();

  // now load elements
  for (auto &v : loader)
  {
    LoadProperty(v.first, v.second);
  }
}

}