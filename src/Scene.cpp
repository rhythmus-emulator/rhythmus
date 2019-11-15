#include "Scene.h"
#include "SceneManager.h"       /* preference */
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
    do_sort_objects_(false), enable_caching_(false),
    focused_object_(nullptr)
{
}

void Scene::LoadScene()
{
  // Load scene metric (if exist)
  std::string scene_name(get_name());
  ASSERT(!scene_name.empty());
  std::string scene_path =
    Game::getInstance().GetAttribute<std::string>(scene_name);
  if (!scene_path.empty())
    LoadMetrics(scene_path);

  // Read scene-specific option if necessary.
  LoadOptions();

  // Load resource aliases.
  LoadResourceAlias();

  // Load additional script if necessary.
  LoadScript();

  // Initialize objects.
  for (auto *obj : children_)
    obj->Initialize();

  // sort object if necessary.
  if (do_sort_objects_)
    std::sort(children_.begin(), children_.end());

  // send event to update LR2Flag from option.
  EventManager::SendEvent("SceneLoaded");
}

void Scene::LoadResourceAlias()
{
  std::string alias_concat;
  std::vector<std::string> aliases;
  auto *metric = SceneManager::getMetrics(get_name());
  if (!metric) return;

  if (metric->get("Alias", alias_concat))
  {
    Split(alias_concat, ',', aliases);
    for (const auto &alias : aliases)
    {
      std::string resolvpath;
      if (!metric->get("Alias" + alias, resolvpath))
        continue;
      ResourceManager::SetAlias(alias, resolvpath);
    }
  }

#if 0
  img->CommitImage();
  images_.push_back(img);

  fonts_.push_back(ResourceManager::getInstance().LoadLR2Font(fontpath));
  fonts_.back()->set_name(fontname);
#endif
}

void Scene::LoadScript()
{
  std::string script_path;
  auto *metric = SceneManager::getMetrics(get_name());
  if (!metric) return;
  if (!metric->get("script", script_path)) return;
  rhythmus::LoadScript(script_path);
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
    SceneTask *task = new SceneTask("inputenable", [this] {
      EnableInput(true);
    });
    task->wait_for(theme_param_.begin_input_time);
    scenetask_.Enqueue(task);
    event_time += theme_param_.begin_input_time;
  }

  // set scenetime if necessary
  if (theme_param_.next_scene_time > 0)
  {
    EventManager::SendEvent("SceneChange");
    SceneTask *task = new SceneTask("timeout", [this] {
      this->CloseScene(true);
      EventManager::SendEvent("SceneTimeout");
    });
    task->wait_for(theme_param_.next_scene_time - event_time);
    scenetask_.Enqueue(task);
    event_time += theme_param_.next_scene_time;
  }
}

void Scene::FadeOutScene(bool next)
{
  float duration = theme_param_.fade_out_time;
  if (duration <= 0)
  {
    // immediate close scene
    CloseScene(next);
    return;
  }

  // fade effect
  if (fade_duration_ != 0) return;
  fade_duration_ = -duration;
  fade_time_ = 0;

  // disable input & disable event enqueue
  EnableInput(false);
  SceneTask *task = new SceneTask("fadeoutevent", [this, next] {
    this->CloseScene(next);
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

// XXX: change this method name to finishscene?
void Scene::CloseScene(bool next)
{
  SaveOptions();
  SceneManager::ChangeScene(
    next ? next_scene_ : prev_scene_
  );
}

void Scene::EnableInput(bool enable_input)
{
  is_input_available_ = enable_input;
}

void Scene::ProcessInputEvent(const InputEvent& e)
{
  if (e.KeyCode() == GLFW_KEY_ESCAPE)
  {
    FadeOutScene(false);
  }
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
  // Load necessary option list from userpref
  std::string scene_name(get_name());
  auto *metric = SceneManager::getMetrics(scene_name);
  if (!metric) return;
  int option_count = 0;

  // Load options from user option list
  // If not exist, then add option to user option list.
  if (metric->get("Option", option_count))
  {
    for (int i = 0; i < option_count; ++i)
    {
      std::string args_s;
      if (!metric->get("Option" + std::to_string(i), args_s))
        continue;
      CommandArgs args(args_s);
      auto &val = Setting::GetOption(scene_name, args.Get<std::string>(0));
      if (val.empty())
      {
        val = Setting::SetOption(scene_name,
          args.Get<std::string>(0), /* option name */
          args.Get<std::string>(1), /* option type (file, flag, val ...) */
          args.Get<std::string>(2), /* selection list */
          args.Get<std::string>(3)  /* default value */
        );
      }

      CommandArgs args_val(val);
      std::string opt_cmd(args_val.Get<std::string>(0));
      if (opt_cmd == "file")
      {
        ResourceManager::getInstance().MakePathHigherPriority(args_val.Get<std::string>(1));
      }
      else if (opt_cmd == "flag")
      {
        SceneManager::setVisible(args_val.Get<int>(1), 1);
      }
      else if (opt_cmd == "val")
      {
        metric->set(args_val.Get<std::string>(1), args_val.Get<std::string>(2));
      }
    }
  }
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


void Scene::LoadObjectMetrics(const ThemeMetrics &metrics)
{
  /* First, check is given object metrics is generic & able to be created to object.
   * if not, search for currently existing object. */

  BaseObject *target_object = CreateObjectFromMetrics(metrics);

  if (target_object)
  {
    RegisterChild(target_object);
  }
  else
  {
    if (target_object = FindChildByName(metrics.name()))
      target_object->Load(metrics);
    else
      std::cerr << "Warning: object metric " << metrics.name() <<
      " is invalid and cannot appliable." << std::endl;
  }
}

}