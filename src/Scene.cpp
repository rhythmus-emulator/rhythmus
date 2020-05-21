#include "Scene.h"
#include "SceneManager.h"       /* preference */
#include "Setting.h"
#include "Script.h"
#include "Util.h"
#include "Logger.h"
#include "common.h"

namespace rhythmus
{

// ---------------------------- SceneTask
SceneTask::SceneTask(const std::string& name, std::function<void()> func)
  : name_(name), func_(std::move(func)), wait_time_(0), is_ran_(false) {}

void SceneTask::wait_for(float wait_time)
{
  wait_time_ = wait_time;
}

void SceneTask::wait_for(int wait_time)
{
  wait_time_ = (float)wait_time;
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
  : fade_time_(0), fade_duration_(0),
    fade_in_time_(0), fade_out_time_(0),
    is_input_available_(true), begin_input_time_(0), next_scene_time_(0),
    do_sort_objects_(false), enable_caching_(false),
    focused_object_(nullptr)
{
}

void Scene::LoadScene()
{
  if (!get_name().empty())
  {
    // attempt to load custom scene
    auto *pref = PREFERENCE->GetFile(get_name());
    MetricGroup m;
    if (pref && m.Load(**pref))
    {
      Scene::Load(m);
      return;
    }
    else
    {
      Logger::Warn("Attempt to load scene %s but failed.", get_name().c_str());
    }
  }

  // fallback: default metric data
  Scene::Load(*METRIC);

  EventManager::SendEvent("Load");
}

void Scene::StartScene()
{
  int event_time = 0;

  // trigger fade_in if it exist
  TriggerFadeIn();

  // constraints for theme parameters
  if (next_scene_time_ > 0)
  {
    if (begin_input_time_ > next_scene_time_)
      begin_input_time_ = next_scene_time_;
  }

  // set input avail time
  if (begin_input_time_ > 0)
  {
    EnableInput(false);
    SceneTask *task = new SceneTask("inputenable", [this] {
      EnableInput(true);
    });
    task->wait_for(begin_input_time_);
    scenetask_.Enqueue(task);
    event_time += begin_input_time_;
  }

  // set scenetime if necessary
  if (next_scene_time_ > 0)
  {
    EventManager::SendEvent("SceneChange");
    SceneTask *task = new SceneTask("timeout", [this] {
      this->CloseScene(true);
      EventManager::SendEvent("SceneTimeout");
    });
    task->wait_for(next_scene_time_ - event_time);
    scenetask_.Enqueue(task);
    event_time += next_scene_time_;
  }
}

void Scene::Load(const MetricGroup& m)
{
  if (m.exist("sort"))
    do_sort_objects_ = m.get<bool>("sort");
  if (m.exist("Timeout"))
    next_scene_time_ = m.get<int>("Timeout");
  if (m.exist("InputStart"))
    begin_input_time_ = m.get<int>("InputStart");
  if (m.exist("FadeOut"))
    fade_out_time_ = m.get<int>("FadeOut");
  if (m.exist("FadeIn"))
    fade_in_time_ = m.get<int>("FadeIn");


  // Load scene specific script if necessary.
  if (m.exist("script"))
  {
    Script::Execute(m.get_str("script"));
  }


  // Create objects.
  SetOwnChildren(true);
  for (auto c = m.group_cbegin(); c != m.group_cend(); ++c)
  {
    BaseObject *o = CreateObject(*c);
    if (o)
      AddChild(o);
  }


  // sort object if necessary.
  if (do_sort_objects_)
  {
    std::sort(children_.begin(), children_.end(),
      [](BaseObject *o1, BaseObject *o2) {
      return o1->GetDrawOrder() < o2->GetDrawOrder();
    });
  }
}

void Scene::FadeOutScene(bool next)
{
  float duration = (float)fade_out_time_;
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
  task->wait_for(duration);
  scenetask_.DeleteAll();
  scenetask_.Enqueue(task);
  scenetask_.IsEnqueueable(false);
}

void Scene::TriggerFadeIn()
{
  float duration = (float)fade_in_time_;
  if (fade_duration_ != 0) return;
  fade_duration_ = duration;
  fade_time_ = 0;
}

// XXX: change this method name to finishscene?
void Scene::CloseScene(bool next)
{
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
  float x = (float)e.GetX();
  float y = (float)e.GetY();
  if (e.type() == InputEvents::kOnKeyUp)
  {
    if (e.KeyCode() == RI_KEY_ESCAPE)
    {
      FadeOutScene(false);
    }
  }
  else if (e.type() == InputEvents::kOnCursorMove)
  {
    // only single object can be focused.
    // bool is_hovered = false;
    for (auto i = children_.rbegin(); i != children_.rend(); ++i)
    {
      auto *obj = *i;
      obj->SetHovered(
        /**
         * Comment out hover-for-single-object,
         * as object hovering is not working as expected for LR2.
         *
         * is_hovered ? false : (is_hovered = obj->IsEntered(x, y))
         */
        obj->IsEntered(x, y)
      );
    }
  }
  else if (e.type() == InputEvents::kOnCursorClick)
  {
    // only single object can be focused.
    bool entered = false;
    bool is_focused = false;
    for (auto i = children_.rbegin(); i != children_.rend(); ++i)
    {
      auto *obj = *i;
      if (!obj->IsVisible())
        continue;
      if (is_focused)
        obj->SetFocused(false);
      else
      {
        entered = obj->IsEntered(x, y);
        if (entered)
          obj->Click();
        else
          obj->SetFocused(false);
        is_focused = obj->IsFocused();
      }
    }
  }
}

bool Scene::is_input_available() const
{
  return is_input_available_;
}

void Scene::doUpdate(float delta)
{
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
    {{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
    {{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }},
  };

  // implementation of fadeout effect
  if (fade_duration_ != 0)
  {
    float fade_alpha_ = fade_duration_ > 0 ?
      1.0f - fade_time_ / fade_duration_ :
      fade_time_ / -fade_duration_;
    if (fade_alpha_ > 1)
      fade_alpha_ = 1;

    float w = (float)GRAPHIC->width();
    float h = (float)GRAPHIC->height();
    vi[1].p.x = w;
    vi[2].p.x = w;
    vi[2].p.y = h;
    vi[3].p.y = h;
    vi[0].c.a = vi[1].c.a = vi[2].c.a = vi[3].c.a = fade_alpha_;

    GRAPHIC->SetTexture(0, 0);
    GRAPHIC->SetBlendMode(1);
    //glColor3f(0, 0, 0);
    GRAPHIC->DrawQuad(vi);
  }
}

}
