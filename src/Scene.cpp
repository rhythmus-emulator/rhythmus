#include "Scene.h"
#include "SceneManager.h"       /* preference */
#include "TaskPool.h"
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
    do_sort_objects_(false), enable_caching_(false), scene_loading_task_(nullptr)
{
}

Scene::~Scene()
{
}

void Scene::Load(const MetricGroup &m)
{
  int draw_index = 0;
  bool use_custom_layout = false;

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
  m.get_safe("usecustomlayout", use_custom_layout);

  // sort object if necessary.
  if (do_sort_objects_)
  {
    std::stable_sort(children_.begin(), children_.end(),
      [](BaseObject *o1, BaseObject *o2) {
      return o1->GetDrawOrder() < o2->GetDrawOrder();
    });
  }
}

class SceneLoadTask : public Task {
public:
  SceneLoadTask(Scene* s) : s(s) {}

  virtual void run()
  {
    // Load start event : Loading
    EVENTMAN->SendEvent("Loading");

    // Load metrics (e.g. Stepmania)
    s->LoadFromName();

    // Load script file
    std::string script_path = SCENEMAN->GetSceneScript(s);
    Script::Load(script_path, s);
  }

  virtual void abort() {}

private:
  Scene* s;
};

void Scene::LoadScene()
{
  if (scene_loading_task_ || IsLoading()) {
    Logger::Warn("Task is already in loading, or loaded.");
    return;
  }
  // TODO: scene load should be in main thread
  // (only resource loading in sub-thread)
  //scene_loading_task_ = new SceneLoadTask(this);
  //TASKMAN->EnqueueTask(scene_loading_task_);

    // Load start event : Loading
  EVENTMAN->SendEvent("Loading");

  // Load metrics (e.g. Stepmania)
  LoadFromName();

  // Load script file
  std::string script_path = SCENEMAN->GetSceneScript(this);
  Script::Load(script_path, this);
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
    EVENTMAN->SendEvent("SceneChange");
    SceneTask *task = new SceneTask("timeout", [this] {
      this->CloseScene(true);
      EVENTMAN->SendEvent("SceneTimeout");
    });
    task->wait_for(next_scene_time_ - event_time);
    scenetask_.Enqueue(task);
    event_time += next_scene_time_;
  }

  // Now trigger actual 'OnLoad' event.
  EVENTMAN->SendEvent("Load");
}

bool Scene::IsLoading() const
{
  // TODO: change into TaskGroup later
  return TASKMAN->is_idle() == false;
}

void Scene::RegisterPredefObject(BaseObject *obj)
{
  R_ASSERT(obj && !obj->get_name().empty());
  predef_objects_[obj->get_name()] = obj;
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
  SCENEMAN->ChangeScene(next ? next_scene_ : prev_scene_);
}

void Scene::EnableInput(bool enable_input)
{
  is_input_available_ = enable_input;
}

bool Scene::IsInputAvailable() const
{
  // TODO: check scene start time also
  return is_input_available_;
}

void Scene::SetInputStartTime(int time) { begin_input_time_ = time; }
void Scene::SetFadeOutTime(int time) { fade_out_time_ = time; }
void Scene::SetFadeInTime(int time) { fade_in_time_ = time; }
void Scene::SetTimeout(int time) { next_scene_time_ = time; }

void Scene::ProcessInputEvent(const InputEvent& e)
{
  if (e.type() == InputEvents::kOnKeyUp)
  {
    if (e.KeyCode() == RI_KEY_ESCAPE)
    {
      FadeOutScene(false);
    }
  }
}

void Scene::doUpdate(double delta)
{
  // scheduled events
  scenetask_.Update((float)delta);

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
    double fade_alpha_ = fade_duration_ > 0 ?
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
    vi[0].c.a = vi[1].c.a = vi[2].c.a = vi[3].c.a = (float)fade_alpha_;

    GRAPHIC->SetTexture(0, 0);
    GRAPHIC->SetBlendMode(1);
    //glColor3f(0, 0, 0);
    GRAPHIC->DrawQuad(vi);
  }
}

// -------------------------------------------------------------------- Loaders

class LR2CSVSceneHandlers
{
public:
  static bool image(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    std::string name, path;
    name = format_string("image%u", loader->get_image_index());
    path = ctx->get_str(1);
    if (path != "CONTINUE")
      PATH->SetSymbolLink(name, FilePath(path).get());
    return true;
  }
  static bool lr2font(void*, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto &fntstyle = METRIC->add_group(format_string("font%u", loader->get_font_index()));
    const char *path = ctx->get_str(1);
    fntstyle.set("path", path);
    return true;
  }
  static bool font(void*, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // --
    return true;
  }
  static bool information(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO -- "gamemode", "title", "artist", "previewimage"
    int gamemode = atoi(ctx->get_str(1));
    static const char *scenename_reserved[] = {
      // 7, 5, 14, 10, 9
      "PlayScene", 0, 0, 0, 0,
      "SelectScene",
      "DecideScene",
      "ResultScene",
    };
    const char* scenename = nullptr;
    if (gamemode > 14) return false;
    scenename = scenename_reserved[gamemode];
    if (scenename == nullptr) return false;
    SCENEMAN->SetSceneScript(scenename, ctx->get_path());
    return true;
  }
  static bool customoption(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO -- {"name", "id", "constraint:%3s,%4s,%5s,%6s,%7s,%8s,%9s,%10s", 0},
    return true;
  }
  static bool customfile(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO -- {"name", "constraint", "default", 0}
    return true;
  }
  static bool transcolor(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO: change GRAPHIC configuration
    return true;
  }
  static bool startinput(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *scene = (Scene*)loader->get_object("scene");
    if (!scene) return false;
    scene->SetInputStartTime(ctx->get_int(1));
    return true;
  }
  static bool ignoreinput(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // ignores all input except system keys (e.g. escape)
    // TODO
    return true;
  }
  static bool fadeout(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *scene = (Scene*)loader->get_object("scene");
    if (!scene) return false;
    scene->SetFadeOutTime(ctx->get_int(1));
    return true;
  }
  static bool fadein(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *scene = (Scene*)loader->get_object("scene");
    if (!scene) return false;
    scene->SetFadeInTime(ctx->get_int(1));
    return true;
  }
  static bool timeout(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    auto *scene = (Scene*)loader->get_object("scene");
    if (!scene) return false;
    scene->SetTimeout(ctx->get_int(1));
    return true;
  }
  static bool helpfile(void *, LR2CSVExecutor *loader, LR2CSVContext *ctx)
  {
    // TODO
    return true;
  }
  LR2CSVSceneHandlers()
  {
    LR2CSVExecutor::AddHandler("#IMAGE", (LR2CSVHandlerFunc)image);
    LR2CSVExecutor::AddHandler("#LR2FONT", (LR2CSVHandlerFunc)lr2font);
    LR2CSVExecutor::AddHandler("#FONT", (LR2CSVHandlerFunc)font);
    LR2CSVExecutor::AddHandler("#INFORMATION", (LR2CSVHandlerFunc)information);
    LR2CSVExecutor::AddHandler("#CUSTOMOPTION", (LR2CSVHandlerFunc)customoption);
    LR2CSVExecutor::AddHandler("#CUSTOMFILE", (LR2CSVHandlerFunc)customfile);
    LR2CSVExecutor::AddHandler("#TRANSCLOLR", (LR2CSVHandlerFunc)transcolor);
    LR2CSVExecutor::AddHandler("#STARTINPUT", (LR2CSVHandlerFunc)startinput);
    LR2CSVExecutor::AddHandler("#IGNOREINPUT", (LR2CSVHandlerFunc)ignoreinput);
    LR2CSVExecutor::AddHandler("#FADEOUT", (LR2CSVHandlerFunc)fadeout);
    LR2CSVExecutor::AddHandler("#FADEIN", (LR2CSVHandlerFunc)fadein);
    LR2CSVExecutor::AddHandler("#TIMEOUT", (LR2CSVHandlerFunc)timeout);
    LR2CSVExecutor::AddHandler("#HELPFILE", (LR2CSVHandlerFunc)helpfile);
  }
};

// register handlers
LR2CSVSceneHandlers _LR2CSVSceneHandlers;

}
