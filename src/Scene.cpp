#include "Scene.h"
#include "SceneManager.h"       /* preference */
#include "TaskPool.h"
#include "ScriptLR2.h"
#include "Util.h"
#include "Logger.h"
#include "common.h"

// objects
#include "Sprite.h"
#include "object/Text.h"
#include "object/Number.h"
#include "object/Button.h"
#include "object/Slider.h"
#include "object/Bargraph.h"
#include "object/OnMouse.h"

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
  : imgidx_(0), fntidx_(0), fade_time_(0), fade_duration_(0),
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
}

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
  Script::Load(script_path, this, get_name());

  // Finialize
  if (do_sort_objects_) SortChildren();
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

void Scene::AddImageSymbolLink(const std::string& path)
{
  std::string name = format_string("image%u", imgidx_++);
  if (path != "CONTINUE")
    PATH->SetSymbolLink(name, FilePath(path).get());
}

void Scene::AddFontSymbolLink(const std::string& path)
{
  std::string name = format_string("font%u", fntidx_++);
  if (path != "CONTINUE")
    PATH->SetSymbolLink(name, FilePath(path).get());
}

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

#define HANDLERLR2_OBJNAME const char

class ScenePreloadHandler : public LR2FnClass {
public:
  HANDLERLR2(INFORMATION) {
    // TODO -- "gamemode", "title", "artist", "previewimage"
    int gamemode = args.get_int(1);
    static const char* scenename_reserved[] = {
      // 7, 5, 14, 10, 9
      "PlayScene", 0, 0, 0, 0,
      "SelectScene",
      "DecideScene",
      "ResultScene",
    };
    const char* scenename = nullptr;
    if (gamemode > 14) return;
    scenename = scenename_reserved[gamemode];
    if (scenename == nullptr) return;
    SCENEMAN->SetSceneScript(scenename, o);
  }
  ScenePreloadHandler() : LR2FnClass("ScenePreload") {
    ADDSHANDLERLR2(INFORMATION);
  }
};

class SoundConfigHandler : public LR2FnClass {
public:
  HANDLERLR2(SELECT) {
    METRIC->set("SelectSceneBgm", FilePath(args.get_str(1)).get());
  }
  SoundConfigHandler() : LR2FnClass("SoundConfig") {
    ADDSHANDLERLR2(SELECT);
  }
};


#undef  HANDLERLR2_OBJNAME
#define HANDLERLR2_OBJNAME Scene
REGISTER_LR2OBJECT(Scene);

class BaseSceneHandler : public LR2FnClass {
public:
  HANDLERLR2(IMAGE) {
    o->AddImageSymbolLink(args.get_str(1));
  }
  HANDLERLR2(LR2FONT) {
    o->AddFontSymbolLink(args.get_str(1));
  }
  HANDLERLR2(FONT) {
  }
  HANDLERLR2(INFORMATION) {
  }
  HANDLERLR2(CUSTOMOPTION) {
    // TODO -- {"name", "id", "constraint:%3s,%4s,%5s,%6s,%7s,%8s,%9s,%10s", 0},
  }
  HANDLERLR2(CUSTOMFILE) {
    // TODO -- {"name", "constraint", "default", 0}
  }
  HANDLERLR2(TRANSCLOLR) {
    // TODO: change GRAPHIC configuration
  }
  HANDLERLR2(STARTINPUT) {
    o->SetInputStartTime(args.get_int(1));
  }
  HANDLERLR2(IGNOREINPUT) {
    // TODO: ignores all input except system keys (e.g. escape)
  }
  HANDLERLR2(FADEOUT) {
    o->SetFadeOutTime(args.get_int(1));
  }
  HANDLERLR2(FADEIN) {
    o->SetFadeInTime(args.get_int(1));
  }
  HANDLERLR2(TIMEOUT) {
    o->SetTimeout(args.get_int(1));
  }
  HANDLERLR2(HELPFILE) {
    // TODO
  }
  HANDLERLR2(SRC_IMAGE) {
    auto* obj = new Sprite();
    obj->set_name("#IMAGE");
    obj->RunCommand(args);
    obj->RunCommand("#SRC", args);
    o->AddChild(obj);
  }
  HANDLERLR2(DST_IMAGE) {
    auto* obj = o->GetLastChildWithName("#IMAGE");
    if (!obj) {
      Logger::Warn("[Warn] Invalid %s tag", args.get_str(0));
      return;
    }
    obj->RunCommand("#DST_IMAGE", args);
    obj->RunCommand("#DST", args);
  }
  HANDLERLR2(SRC_TEXT) {
    auto* obj = new Text();
    obj->set_name("#TEXT");
    obj->RunCommand(args);
    obj->RunCommand("#SRC", args);
    o->AddChild(obj);
  }
  HANDLERLR2(DST_TEXT) {
    auto* obj = o->GetLastChildWithName("#TEXT");
    if (!obj) {
      Logger::Warn("[Warn] Invalid %s tag", args.get_str(0));
      return;
    }
    obj->RunCommand("#DST_TEXT", args);
    obj->RunCommand("#DST", args);
  }
  HANDLERLR2(SRC_NUMBER) {
    auto* obj = new Number();
    obj->set_name("#NUMBER");
    obj->RunCommand(args);
    obj->RunCommand("#SRC", args);
    o->AddChild(obj);
  }
  HANDLERLR2(DST_NUMBER) {
    auto* obj = o->GetLastChildWithName("#NUMBER");
    if (!obj) {
      Logger::Warn("[Warn] Invalid %s tag", args.get_str(0));
      return;
    }
    obj->RunCommand("#DST_NUMBER", args);
    obj->RunCommand("#DST", args);
  }
  HANDLERLR2(SRC_BUTTON) {
    auto* obj = new Button();
    obj->set_name("#BUTTON");
    obj->RunCommand(args);
    obj->RunCommand("#SRC", args);
    o->AddChild(obj);
  }
  HANDLERLR2(DST_BUTTON) {
    auto* obj = o->GetLastChildWithName("#BUTTON");
    if (!obj) {
      Logger::Warn("[Warn] Invalid %s tag", args.get_str(0));
      return;
    }
    //obj->RunCommand("#DST_BUTTON", args);
    obj->RunCommand("#DST", args);
  }
  HANDLERLR2(SRC_SLIDER) {
    auto* obj = new Slider();
    obj->set_name("#SLIDER");
    obj->RunCommand(args);
    obj->RunCommand("#SRC", args);
    o->AddChild(obj);
  }
  HANDLERLR2(DST_SLIDER) {
    auto* obj = o->GetLastChildWithName("#SLIDER");
    if (!obj) {
      Logger::Warn("[Warn] Invalid %s tag", args.get_str(0));
      return;
    }
    obj->RunCommand("#DST_SLIDER", args);
    obj->RunCommand("#DST", args);
  }
  HANDLERLR2(SRC_BARGRAPH) {
    auto* obj = new Bargraph();
    obj->set_name("#BARGRAPH");
    obj->RunCommand(args);
    obj->RunCommand("#SRC", args);
    o->AddChild(obj);
  }
  HANDLERLR2(DST_BARGRAPH) {
    auto* obj = o->GetLastChildWithName("#BARGRAPH");
    if (!obj) {
      Logger::Warn("[Warn] Invalid %s tag", args.get_str(0));
      return;
    }
    //obj->RunCommand("#DST_BARGRAPH", args);
    obj->RunCommand("#DST", args);
  }
  HANDLERLR2(SRC_ONMOUSE) {
    auto* obj = new OnMouse();
    obj->set_name("#ONMOUSE");
    obj->RunCommand(args);
    obj->RunCommand("#SRC_IMAGE", args);
    obj->RunCommand("#SRC", args);
    o->AddChild(obj);
  }
  HANDLERLR2(DST_ONMOUSE) {
    auto* obj = o->GetLastChildWithName("#ONMOUSE");
    if (!obj) {
      Logger::Warn("[Warn] Invalid %s tag", args.get_str(0));
      return;
    }
    //obj->RunCommand("#DST_ONMOUSE", args);
    obj->RunCommand("#DST", args);
  }
  BaseSceneHandler() : LR2FnClass(GetTypename<Scene>())
  {
    ADDSHANDLERLR2(IMAGE);
    ADDSHANDLERLR2(LR2FONT);
    ADDSHANDLERLR2(FONT);
    ADDSHANDLERLR2(INFORMATION);
    ADDSHANDLERLR2(CUSTOMOPTION);
    ADDSHANDLERLR2(CUSTOMFILE);
    ADDSHANDLERLR2(TRANSCLOLR);
    ADDSHANDLERLR2(STARTINPUT);
    ADDSHANDLERLR2(IGNOREINPUT);
    ADDSHANDLERLR2(FADEOUT);
    ADDSHANDLERLR2(FADEIN);
    ADDSHANDLERLR2(TIMEOUT);
    ADDSHANDLERLR2(HELPFILE);
    ADDSHANDLERLR2(SRC_IMAGE);
    ADDSHANDLERLR2(DST_IMAGE);
    ADDSHANDLERLR2(SRC_TEXT);
    ADDSHANDLERLR2(DST_TEXT);
    ADDSHANDLERLR2(SRC_NUMBER);
    ADDSHANDLERLR2(DST_NUMBER);
    ADDSHANDLERLR2(SRC_BUTTON);
    ADDSHANDLERLR2(DST_BUTTON);
    ADDSHANDLERLR2(SRC_SLIDER);
    ADDSHANDLERLR2(DST_SLIDER);
    ADDSHANDLERLR2(SRC_BARGRAPH);
    ADDSHANDLERLR2(DST_BARGRAPH);
    ADDSHANDLERLR2(SRC_ONMOUSE);
    ADDSHANDLERLR2(DST_ONMOUSE);
  }
};

// register handlers
ScenePreloadHandler _ScenePreloadHandler;
SoundConfigHandler _SoundConfigHandler;
BaseSceneHandler _BaseSceneHandler;

}
