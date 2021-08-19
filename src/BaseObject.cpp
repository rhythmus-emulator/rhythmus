#include "BaseObject.h"
#include "SceneManager.h"
#include "Setting.h"
#include "Util.h"
#include "Script.h"
#include "ScriptLR2.h"
#include "Logger.h"
#include "KeyPool.h"
#include "common.h"
#include "config.h"

static std::map<std::string, void*> _fnLR2CommandMap;

namespace rhythmus
{

BaseObject::BaseObject()
  : parent_(nullptr), propagate_event_(false), draw_order_(0), position_prop_(0),
    set_xy_as_center_(false), visible_(true), hide_if_not_tweening_(false),
    ignore_visible_group_(true), is_draggable_(false), is_focusable_(false),
    is_focused_(false), is_hovered_(false), do_clipping_(false)
{
  memset(&frame_, 0, sizeof(DrawProperty));
  SetRGB(1.0f, 1.0f, 1.0f);
  SetAlpha(1.0f);
  SetScale(1.0f, 1.0f);
  SetCenter(0.5f, 0.5f);
  SetRotation(0, 0, 0);
  memset(visible_flag_, 0, sizeof(visible_flag_));
  memset(&bg_color_, 0, sizeof(Vector4));
}

BaseObject::BaseObject(const BaseObject& obj)
  : name_(obj.name_), propagate_event_(obj.propagate_event_),
    parent_(obj.parent_), ani_(obj.ani_), position_prop_(obj.position_prop_),
    draw_order_(obj.draw_order_), set_xy_as_center_(obj.set_xy_as_center_), visible_(obj.visible_),
    hide_if_not_tweening_(obj.hide_if_not_tweening_),
    ignore_visible_group_(obj.ignore_visible_group_),
    is_focusable_(obj.is_focusable_), is_draggable_(false), is_focused_(false),
    is_hovered_(false), do_clipping_(false)
{
  frame_ = obj.frame_;
  memcpy(visible_flag_, obj.visible_flag_, sizeof(visible_flag_));
  bg_color_ = obj.bg_color_;

  for (auto &d : obj.children_)
  {
    if (d.is_static)
      AddChild(d.p);
    else
      AddChild(d.p->clone());
  }
}

BaseObject::~BaseObject()
{
  SCENEMAN->ClearFocus(this);
  RemoveAllChild();
}

BaseObject *BaseObject::clone()
{
  return new BaseObject(*this);
}

void BaseObject::set_name(const std::string& name)
{
  name_ = name;
}

const std::string& BaseObject::get_name() const
{
  return name_;
}

void BaseObject::AddChild(BaseObject* obj)
{
  children_.push_back(ChildData{ obj, false });
  obj->parent_ = this;
}

void BaseObject::AddStaticChild(BaseObject* obj)
{
  children_.push_back(ChildData{ obj, true });
  obj->parent_ = this;
}

void BaseObject::RemoveChild(BaseObject* obj)
{
  auto it = std::find_if(children_.begin(), children_.end(),
    [obj](const ChildData& x) { return x.p == obj; });
  if (it != children_.end())
    children_.erase(it);
  if (!it->is_static) delete obj;
  else obj->parent_ = nullptr;
}

void BaseObject::RemoveAllChild()
{
  for (auto& d : children_)
    if (!d.is_static)
      delete d.p;
  children_.clear();
}

BaseObject* BaseObject::FindChildByName(const std::string& name)
{
  for (auto &d : children_)
    if (d.p->get_name() == name) return d.p;
  return nullptr;
}

void BaseObject::set_parent(BaseObject* obj)
{
  parent_ = obj;
}

BaseObject* BaseObject::get_parent()
{
  return parent_;
}

BaseObject* BaseObject::GetLastChild()
{
  if (children_.size() > 0)
    return children_.back().p;
  else return nullptr;
}

BaseObject* BaseObject::GetLastChildWithName(const std::string& name)
{
  for (auto i = children_.rbegin(); i != children_.rend(); ++i) {
    if (i->p->get_name() == name)
      return i->p;
  }
  return nullptr;
}

BaseObject *BaseObject::GetChildAtPosition(float x, float y)
{
  for (auto i = children_.rbegin(); i != children_.rend(); ++i)
  {
    auto* obj = i->p;
    if (/*obj->IsVisible() &&*/ obj->IsEntered(x, y))
    {
      auto *childobj = obj->GetChildAtPosition(x - obj->GetX(), y - obj->GetY());
      if (childobj) return childobj;
      else return obj;
    }
  }
  return nullptr;
}

void BaseObject::Load(const MetricGroup &m)
{
  // Load command
  LoadCommand(m);

  // Load values
  m.get_safe("name", name_);
  m.get_safe("zindex", draw_order_);
  m.get_safe("focus", is_focusable_);
  m.get_safe("clipping", do_clipping_);
  if (m.exist("background"))
    FillColorFromString(bg_color_, m.get_str("background"));
  m.get_safe("x", frame_.pos.x);
  m.get_safe("y", frame_.pos.y);
  if (m.exist("w"))
    frame_.pos.z = frame_.pos.x + m.get<float>("w");
  if (m.exist("h"))
    frame_.pos.w = frame_.pos.y + m.get<float>("h");
}

void BaseObject::LoadFromName()
{
  if (get_name().empty())
    return;

  MetricGroup *m = METRIC->get_group(get_name());
  if (!m)
    return;

  Load(*m);

  for (auto &d : children_)
    d.p->LoadFromName();
}

void BaseObject::OnReady()
{
  // propagate OnReady() to children
  for (auto &d : children_)
    d.p->OnReady();
}

void BaseObject::RunCommandByName(const std::string &event_name)
{
  auto it = commands_.find(event_name);
  if (it != commands_.end())
    RunCommand(it->second);
}

void BaseObject::RunCommand(std::string command)
{
  // @warn
  // RunCommand parameter must be passed by value, not by reference
  // since original command string might be destroyed while processing command
  // e.g. command 'flush' clears event early and it destroys original command.

  if (command.empty()) return;
  size_t ia = 0, ib = 0;
  std::string cmd_type, value;
  while (ib <= command.size())
  {
    if (command[ib] == ';' || command[ib] == 0)
    {
      Split(command.substr(ia, ib - ia), ':', cmd_type, value);
      RunCommand(cmd_type, value);
      ia = ib = ib + 1;
    }
    else ++ib;
  }
}

void BaseObject::RunCommand(const std::string &command, const std::string& value)
{
  auto &fnmap = GetCommandFnMap();
  auto it = fnmap.find(command);
  if (it == fnmap.end())
    return;
  else
  {
    try
    {
      CommandArgs args(value);
      it->second(this, args, value);
    }
    catch (std::out_of_range&)
    {
      std::cerr << "Error: Command parameter is not enough to execute " << command << std::endl;
    }
  }
  if (propagate_event_)
  {
    for (auto &d : children_)
      d.p->RunCommand(command, value);
  }
}

void BaseObject::RunCommandLR2(const LR2FnArgs& args)
{
  RunCommandLR2(args.get_str(0), args);
}

/* just clear out command, without event unregister. */
void BaseObject::ClearCommand(const std::string &name)
{
  auto it = commands_.find(name);
  if (it != commands_.end())
    commands_[name].clear();
}

void BaseObject::DeleteAllCommand()
{
  commands_.clear();
  UnsubscribeAll();
}

void BaseObject::QueueCommand(const std::string &command)
{
  qani_.SetCommand(command);
}

void BaseObject::AddCommand(const std::string &name, const std::string &command)
{
  // Append command if already exist.
  auto it = commands_.find(name);
  if (it == commands_.end())
  {
    commands_[name] = command;

    // add event handler if not registered
    SubscribeTo(name);
  }
  else
  {
    if (it->second.empty())
      it->second = command;
    else
      it->second += ";" + command;
  }
}

/* @warn Load() methods automatically calls LoadCommand(). */
void BaseObject::LoadCommand(const MetricGroup& metric)
{
  // process event handler registering
  // XXX: is this code is good enough?
  for (auto it = metric.cbegin(); it != metric.cend(); ++it)
  {
    if (it->first.size() >= 5 /* XXX: 'On' prefix? */
      && strnicmp(it->first.c_str(), "On", 2) == 0)
    {
      AddCommand(it->first.substr(2), it->second);
    }
  }
}

bool BaseObject::OnEvent(const EventMessage& msg)
{
  RunCommandByName(msg.GetEventName());
  return true;
}

DrawProperty& BaseObject::GetLastFrame()
{
  if (ani_.is_empty())
    return frame_;
  else
    return ani_.LastFrame();
}

DrawProperty& BaseObject::GetCurrentFrame()
{
  return frame_;
}

Animation& BaseObject::GetAnimation() { return ani_; }

QueuedAnimation& BaseObject::GetQueuedAnimation() { return qani_; }

void BaseObject::SetX(float x)
{
  float width = GetWidth();
  GetLastFrame().pos.x = x;
  GetLastFrame().pos.z = x + width;
}

void BaseObject::SetY(float y)
{
  float height = GetHeight();
  GetLastFrame().pos.y = y;
  GetLastFrame().pos.w = y + height;
}

void BaseObject::SetWidth(float w)
{
  GetLastFrame().pos.z = GetLastFrame().pos.x + w;
}

void BaseObject::SetHeight(float h)
{
  GetLastFrame().pos.w = GetLastFrame().pos.y + h;
}

void BaseObject::SetOpacity(float opa)
{
  GetLastFrame().color.a = opa;
}

void BaseObject::SetClip(bool clip)
{
  do_clipping_ = clip;
}

void BaseObject::SetPos(int x, int y)
{
  SetX(x);
  SetY(y);
}

void BaseObject::SetPos(const Vector4& pos)
{
  auto &p = GetLastFrame().pos;
  p = pos;
}

void BaseObject::MovePos(int x, int y)
{
  auto &p = GetLastFrame();
  p.pos.x += x;
  p.pos.y += y;
  p.pos.z += x;
  p.pos.w += y;
}

void BaseObject::SetSize(int w, int h)
{
  SetWidth(w);
  SetHeight(h);
}

void BaseObject::SetAlpha(unsigned a)
{
  SetAlpha(a / 255.0f);
}

void BaseObject::SetAlpha(float a)
{
  auto& p = GetLastFrame();
  p.color.a = a;
}

void BaseObject::SetRGB(unsigned r, unsigned g, unsigned b)
{
  SetRGB(r / 255.0f, g / 255.0f, b / 255.0f);
}

void BaseObject::SetRGB(float r, float g, float b)
{
  auto& p = GetLastFrame();
  p.color.r = r;
  p.color.g = g;
  p.color.b = b;
}

void BaseObject::SetScale(float x, float y)
{
  auto& p = GetLastFrame();
  p.scale.x = x;
  p.scale.y = y;
}

void BaseObject::SetRotation(float x, float y, float z)
{
  auto& p = GetLastFrame();
  p.rotate.x = x;
  p.rotate.y = y;
  p.rotate.z = z;
}

void BaseObject::SetRotationAsDegree(float x, float y, float z)
{
  SetRotation(
    glm::radians(x),
    glm::radians(y),
    glm::radians(z)
  );
}

void BaseObject::SetRepeat(bool repeat)
{
  if (repeat)
    ani_.SetLoop(0);
  else
    ani_.DeleteLoop();
}

void BaseObject::SetLoop(unsigned loop_start_time)
{
  ani_.SetLoop(loop_start_time);
}

void BaseObject::SetCenter(float x, float y)
{
  auto& p = GetLastFrame();
  p.align.x = x;
  p.align.y = y;
}

void BaseObject::SetCenter(int type)
{
  auto& p = GetLastFrame();
  float px[] = { 0.5f,
    1.0f,1.0f,1.0f,
    0.5f,0.5f,0.5f,
    0.0f,0.0f,0.0f
  };
  float py[] = { 0.5f,
    0.0f, 0.5f, 1.0f,
    0.0f, 0.5f, 1.0f,
    0.0f, 0.5f, 1.0f,
  };
  p.align.x = px[type];
  p.align.y = py[type];
}

void BaseObject::SetAcceleration(int acc)
{
  if (ani_.is_empty()) return;
  ani_.SetEase(acc);
}

float BaseObject::GetX() const
{
  return frame_.pos.x;
}

float BaseObject::GetY() const
{
  return frame_.pos.y;
}

float BaseObject::GetWidth() const { return frame_.pos.z - frame_.pos.x; }
float BaseObject::GetHeight() const { return frame_.pos.w - frame_.pos.y; }

void BaseObject::SetDebug(const std::string &debug_msg)
{
  debug_ = debug_msg;
}

void BaseObject::BringToTop()
{
  if (!parent_) return;
  auto b = parent_->children_.begin();
  auto e = parent_->children_.end();
  auto it = std::find_if(b, e,
    [this](const ChildData& x) { return x.p == this; });
  if (it != e) {
    std::rotate(it, it + 1, e);
  }
}

void BaseObject::SetVisibleFlag(const std::string& group0, const std::string& group1,
  const std::string& group2, const std::string& group3)
{
  static const int alwaystrue = 1;
  ignore_visible_group_ = false;
  visible_flag_[0] = &alwaystrue;
  visible_flag_[1] = &alwaystrue;
  visible_flag_[2] = &alwaystrue;
  visible_flag_[3] = &alwaystrue;
  if (!group0.empty())
  {
    KeyData<int> k1 = KEYPOOL->GetInt(group0);
    visible_flag_[0] = &*k1;
  }
  if (!group1.empty())
  {
    KeyData<int> k2 = KEYPOOL->GetInt(group1);
    visible_flag_[1] = &*k2;
  }
  if (!group2.empty())
  {
    KeyData<int> k3 = KEYPOOL->GetInt(group2);
    visible_flag_[2] = &*k3;
  }
  if (!group3.empty())
  {
    KeyData<int> k4 = KEYPOOL->GetInt(group3);
    visible_flag_[3] = &*k4;
  }
}

void BaseObject::UnsetVisibleFlag()
{
  ignore_visible_group_ = true;
}

void BaseObject::Hide()
{
  visible_ = false;
}

void BaseObject::Show()
{
  visible_ = true;
}

void BaseObject::SetDrawOrder(int order)
{
  draw_order_ = order;
}

int BaseObject::GetDrawOrder() const
{
  return draw_order_;
}

void BaseObject::SetText(const std::string &value) {}

void BaseObject::SetNumber(int number) { SetText(std::to_string(number)); }

void BaseObject::SetNumber(double number) { SetText(std::to_string(number)); }

void BaseObject::Refresh() {}

void BaseObject::SetFocusable(bool is_focusable)
{
  is_focusable_ = is_focusable;
}

void BaseObject::SetDraggable(bool is_draggable)
{
  is_draggable_ = is_draggable;
}

bool BaseObject::IsEntered(float x, float y)
{
  // TODO: in case of rotation?
  return (x >= frame_.pos.x && x <= frame_.pos.z
       && y >= frame_.pos.y && y <= frame_.pos.w);
}

void BaseObject::SetHovered(bool is_hovered)
{
  if (is_hovered_ == is_hovered)
    return;

  if (is_hovered)
    RunCommandByName("hover");
  else
    RunCommandByName("hoverout");
  is_hovered_ = is_hovered;
}

void BaseObject::SetFocused(bool is_focused)
{
  if (!is_focusable_ || is_focused_ == is_focused)
    return;

  if (is_focused)
    RunCommandByName("focus");
  else
    RunCommandByName("focusout");
  is_focused_ = is_focused;
}

bool BaseObject::IsHovered() const
{
  return is_hovered_;
}

bool BaseObject::IsFocused() const
{
  return is_focused_;
}

bool BaseObject::IsDragging() const
{
  return this == SCENEMAN->GetDraggingObject();
}

bool BaseObject::IsFocusable() const
{
  return is_focusable_;
}

bool BaseObject::IsDraggable() const
{
  return is_draggable_;
}

void BaseObject::Click()
{
  if (!is_focusable_)
    return;

  // XXX: same as SetFocused(true) ...?
  SetFocused(true);

  RunCommandByName("click");
}

void BaseObject::OnDrag(float dx, float dy)
{
  frame_.pos.x += dx;
  frame_.pos.z += dx;
  frame_.pos.y += dy;
  frame_.pos.w += dy;
}

void BaseObject::OnText(uint32_t codepoint)
{
}

void BaseObject::OnAnimation(DrawProperty &frame)
{
}

void BaseObject::Stop()
{
  ani_.Stop();
}

void BaseObject::Replay()
{
  ani_.Replay();
}

void BaseObject::Pause()
{
  ani_.Pause();
}

bool BaseObject::IsTweening() const
{
  return ani_.is_tweening();
}

bool BaseObject::IsVisible() const
{
  if (!ignore_visible_group_ && (*visible_flag_[0] == 0
    || *visible_flag_[1] == 0
    || *visible_flag_[2] == 0
    || *visible_flag_[3] == 0))
    return false;

  if (hide_if_not_tweening_ && ani_.is_tweening())
    return false;

  return visible_/* &&
    current_prop_.aBL > 0 &&
    current_prop_.aBR > 0 &&
    current_prop_.aTL > 0 &&
    current_prop_.aTR > 0*/;
}

// milisecond
void BaseObject::Update(double delta)
{
  bool is_tweening = ani_.is_tweening();
  ani_.Update(delta);
  if (is_tweening) {
    ani_.GetDrawProperty(frame_);
    OnAnimation(frame_);
  }
  doUpdate(delta);
  for (auto& d : children_)
    d.p->Update(delta);
  doUpdateAfter();
}

void BaseObject::Render()
{
  if (!IsVisible())
    return;

  Vector2 size(frame_.pos.z - frame_.pos.x,
               frame_.pos.w - frame_.pos.y);

  // set matrix for rotation/translation
  // remind vertex's center coord is (0,0).
  // (see BaseObject::FillVertexInfo(...) for more detail)
  GRAPHIC->PushMatrix();
  /*
  if (position_prop_ == 0)
  {
  }*/
  GRAPHIC->Translate(Vector3{
    frame_.pos.x + size.x * frame_.align.x,
    frame_.pos.y + size.y * frame_.align.y,
    0 });
  if (frame_.rotate.x != 0 || frame_.rotate.y != 0 || frame_.rotate.z != 0)
    GRAPHIC->Rotate(frame_.rotate);
  GRAPHIC->Scale(Vector3{ frame_.scale.x, frame_.scale.y, 1.0f });
  if (set_xy_as_center_)
  {
    if (frame_.align.x != 0.5f || frame_.align.y != 0.5f)
    {
      GRAPHIC->Translate(Vector3{
        (0.5f-frame_.align.x) * size.x,
        (0.5f-frame_.align.y) * size.y,
        0 });
    }
  }
  else
  {
    if (frame_.align.x != 0.0f || frame_.align.y != 0.0f)
    {
      GRAPHIC->Translate(Vector3{
        -frame_.align.x * size.x,
        -frame_.align.y * size.y,
        0 });
    }
  }

  // TODO: texture translate

  // clip implementation (prevent content overflowing)
  // XXX: nested clipping support?
  if (do_clipping_)
    GRAPHIC->ClipViewArea(frame_.pos);

  // render background if necessary.
  // XXX: you can check rendering area by using background property.
  if (bg_color_.a > 0)
  {
    VertexInfo vi[4];
    vi[0].p = Vector3(-size.x / 2, -size.y / 2, 0.0f);
    vi[1].p = Vector3(size.x / 2, -size.y / 2, 0.0f);
    vi[2].p = Vector3(size.x / 2, size.y / 2, 0.0f);
    vi[3].p = Vector3(-size.x / 2, size.y / 2, 0.0f);
    vi[0].t = Vector2(0.0f, 0.0f);
    vi[1].t = Vector2(1.0f, 0.0f);
    vi[2].t = Vector2(1.0f, 1.0f);
    vi[3].t = Vector2(0.0f, 1.0f);
    vi[0].c = vi[1].c = vi[2].c = vi[3].c = bg_color_;
    GRAPHIC->SetBlendMode(1);
    GRAPHIC->SetTexture(0, 1);
    GRAPHIC->DrawQuad(vi);
  }

  // render vertices
  doRender();

  for (auto& d : children_)
    d.p->Render();

  doRenderAfter();

  // reset clipping
  if (do_clipping_)
    GRAPHIC->ResetViewArea();

  GRAPHIC->PopMatrix();
}

void BaseObject::SortChildren()
{
  std::stable_sort(children_.begin(), children_.end(),
    [](const ChildData& o1, const ChildData& o2) {
      return o1.p->GetDrawOrder() < o2.p->GetDrawOrder();
    });
}

void BaseObject::FillVertexInfo(VertexInfo *vi)
{
  const DrawProperty &f = GetCurrentFrame();

  // we use center of vertex as base coord.
  // remind final drawing position will be determined by world matrix.

  float w = f.pos.z - f.pos.x;
  float h = f.pos.w - f.pos.y;

  vi[0].p = Vector3{ -w / 2, -h / 2, 0 };
  vi[0].c = f.color;

  vi[1].p = Vector3{ w / 2, -h / 2, 0 };
  vi[1].c = f.color;

  vi[2].p = Vector3{ w / 2, h / 2, 0 };
  vi[2].c = f.color;

  vi[3].p = Vector3{ -w / 2, h / 2, 0 };
  vi[3].c = f.color;
}

void BaseObject::doUpdate(double delta) {}
void BaseObject::doRender() {}
void BaseObject::doUpdateAfter() {}
void BaseObject::doRenderAfter() {}


/* ---------------------------- Command related */

const CommandFnMap& BaseObject::GetCommandFnMap()
{
  static CommandFnMap cmdfnmap_;
  if (cmdfnmap_.empty())
  {
    static auto fn_X = [](void *o, CommandArgs& args, const std::string &) {
      auto *obj = static_cast<BaseObject*>(o);
      auto vi = args.Get<float>(0);
      obj->SetX(vi);
    };
    static auto fn_Y = [](void *o, CommandArgs& args, const std::string &) {
      auto *obj = static_cast<BaseObject*>(o);
      auto vi = args.Get<float>(0);
      obj->SetY(vi);
    };
    static auto fn_W = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetWidth(args.Get<float>(0));
    };
    static auto fn_H = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetHeight(args.Get<float>(0));
    };
    static auto fn_Pos = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetPos(args.Get<int>(0), args.Get<int>(1));
    };
    static auto fn_Scale = [](void *o, CommandArgs& args, const std::string &) {
      if (args.size() == 1)
        static_cast<BaseObject*>(o)->SetScale(args.Get<float>(0), args.Get<float>(0));
      else
        static_cast<BaseObject*>(o)->SetScale(args.Get<float>(0), args.Get<float>(1));
    };
    static auto fn_Opacity = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetOpacity(args.Get<float>(0));
    };
    static auto fn_acc = [](void *o, CommandArgs& args, const std::string &) {
      if (args.size() == 0)
        return;
      std::string v = args.Get<std::string>(0);
      if (v[0] >= '0' && v[0] <= '9')
        static_cast<BaseObject*>(o)->SetAcceleration(args.Get<int>(0));
      else
      {
        const char *ease_names[] = {
          "none", "linear", "easein", "easeout", "easeinout", "easeinoutback", 0
        };
        for (int i = 0; ease_names[i]; ++i)
        {
          if (stricmp(v.c_str(), ease_names[i]) == 0)
          {
            static_cast<BaseObject*>(o)->SetAcceleration(i);
            return;
          }
        }
      }
    };
    static auto fn_time = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->GetAnimation().DuplicateFrame(args.Get<int>(0));
    };
    static auto fn_stop = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Stop();
    };
    static auto fn_loop = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetLoop(args.Get<int>(0));
    };
    static auto fn_repeat = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetRepeat(true);
    };
    static auto fn_rotate = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetRotationAsDegree(0.0f, 0.0f, args.Get<float>(0));
    };
    static auto fn_Show = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Show();
    };
    static auto fn_Hide = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Hide();
    };
    static auto fn_Replay = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Replay();
    };
    static auto fn_Pause = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Pause();
    };
    static auto fn_Text = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetText(args.Get<std::string>(0));
    };
    static auto fn_Number = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetNumber(args.Get<int>(0));
    };
    static auto fn_NumberF = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetNumber(args.Get<double>(0));
    };
    static auto fn_Refresh = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->Refresh();
    };
    static auto fn_name = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->set_name(args.Get<std::string>(0));
    };
    static auto fn_Focusable = [](void *o, CommandArgs& args, const std::string &) {
      static_cast<BaseObject*>(o)->SetFocusable(args.Get<int>(0));
    };
    static auto fn_SendEvent = [](void *o, CommandArgs& args, const std::string &) {
      EVENTMAN->SendEvent(args.Get<std::string>(0));
    };
    cmdfnmap_["x"] = fn_X;
    cmdfnmap_["y"] = fn_Y;
    cmdfnmap_["w"] = fn_W;
    cmdfnmap_["h"] = fn_H;
    cmdfnmap_["scale"] = fn_Scale;
    cmdfnmap_["opacity"] = fn_Opacity;
    cmdfnmap_["acc"] = fn_acc;
    cmdfnmap_["time"] = fn_time;
    cmdfnmap_["stop"] = fn_stop;
    cmdfnmap_["loop"] = fn_loop;
    cmdfnmap_["repeat"] = fn_repeat;
    cmdfnmap_["rotate"] = fn_rotate;
    cmdfnmap_["pos"] = fn_Pos;
    cmdfnmap_["show"] = fn_Show;
    cmdfnmap_["hide"] = fn_Hide;
    cmdfnmap_["replay"] = fn_Replay;
    cmdfnmap_["pause"] = fn_Pause;
    cmdfnmap_["text"] = fn_Text;
    cmdfnmap_["number"] = fn_Number;
    cmdfnmap_["numberf"] = fn_NumberF;
    cmdfnmap_["refresh"] = fn_Refresh;
    cmdfnmap_["name"] = fn_name;
    cmdfnmap_["focusable"] = fn_Focusable;
    cmdfnmap_["sendevent"] = fn_SendEvent;
  }

  return cmdfnmap_;
}

}

#include "Sprite.h"
#include "object/Text.h"
#include "object/Number.h"
#include "object/Slider.h"
#include "object/Bargraph.h"
#include "object/Button.h"
#include "object/OnMouse.h"
#include "object/Dialog.h"
#include "object/ListView.h"

namespace rhythmus
{

BaseObject* BaseObject::CreateObject(const std::string &type)
{
  BaseObject *object = nullptr;

  // create object from type string
  if (type == "image" || type == "sprite") {
    object = new Sprite();
  }
  else if (type == "text") {
    object = new Text();
  }
  else if (type == "number") {
    object = new Number();
  }
  else if (type == "slider") {
    object = new Slider();
  }
  else if (type == "bargraph") {
    object = new Bargraph();
  }
  else if (type == "onmouse") {
    object = new OnMouse();
  }
  else if (type == "button") {
    object = new Button();
  }
  else if (type == "dialog") {
    // TODO: add command process handler
    //object = new Dialog();
  }
  else if (type == "listview") {
    // TODO: add command process handler
    //object = new ListView();
  }
  else if (type == "frame") {
    // TODO: use BaseFrame() object
    object = new BaseObject();
  }
  else if (type == "line") {

  }

  return object;
}

const char* BaseObject::type() const { return "BaseObject"; }

std::string BaseObject::toString() const
{
  std::stringstream ss;
  ss << "type: " << type() << std::endl;
  if (commands_.empty())
    ss << "events: (empty)" << std::endl;
  else {
    ss << "events:" << std::endl;
    for (auto &i : commands_)
    {
      ss << " - " << i.first << " : " << i.second << std::endl;
    }
  }
  ss << "pos (rect) : " << frame_.pos.x << "," << frame_.pos.y << "," << frame_.pos.w << "," << frame_.pos.z << std::endl;
  ss << "draw_order : " << draw_order_ << std::endl;
  ss << "is_focusable? : " << (is_focusable_ ? "true" : "false") << std::endl;

  if (!debug_.empty())
    ss << "debug message: " << debug_ << std::endl;
  return ss.str();
}

// ------------------------------------------------------------------ Loader/Helper

// TODO: Number::OnStart �޼��� �����ϱ� (Load���� Async �ڵ� �κ� �����ϱ�)

class XMLObjectHandlers
{
public:
  static void sprite(XMLExecutor *e, XMLContext *ctx)
  {
    auto *o = BaseObject::CreateObject("sprite");
    auto *parent = (BaseObject*)e->GetParent();
    parent->AddChild(o);
    e->SetCurrentObject(o);
    o->Load(ctx->GetCurrentMetric());
  }
  static void text(XMLExecutor *e, XMLContext *ctx)
  {
    auto *o = BaseObject::CreateObject("text");
    auto *parent = (BaseObject*)e->GetParent();
    parent->AddChild(o);
    e->SetCurrentObject(o);
    o->Load(ctx->GetCurrentMetric());
  }
  static void slider(XMLExecutor *e, XMLContext *ctx)
  {
    auto *o = BaseObject::CreateObject("slider");
    auto *parent = (BaseObject*)e->GetParent();
    parent->AddChild(o);
    e->SetCurrentObject(o);
    o->Load(ctx->GetCurrentMetric());
  }
  XMLObjectHandlers()
  {
    XMLExecutor::AddHandler("sprite", (XMLCommandHandler)&sprite);
    XMLExecutor::AddHandler("image", (XMLCommandHandler)&sprite);
    XMLExecutor::AddHandler("text", (XMLCommandHandler)&text);
    XMLExecutor::AddHandler("slider", (XMLCommandHandler)&slider);
  }
};

// register handler
XMLObjectHandlers _XMLObjectHandlers;



#define HANDLERLR2_OBJNAME BaseObject
REGISTER_LR2OBJECT(BaseObject);

class BaseObjectLR2Handler : public LR2FnClass {
public:
  HANDLERLR2(SRC) {
    o->SetDebug(format_string("LR2SRC-%u", args.get_str(21)));
  }
  HANDLERLR2(DST) {
    const bool is_first_run = o->GetAnimation().is_empty();

    // these attributes are only affective for first run
    if (is_first_run) {
      const int center = args.get_int(15);
      const int loop = args.get_int(16);
      const int timer = args.get_int(17);

      o->SetCenter(center);

      // LR2 needs to keep its animation queue, so don't use stop.
      o->AddCommand(format_string("LR%d", timer), "replay");
      o->AddCommand(format_string("LR%dOff", timer), "hide");
      if (loop >= 0)
        o->SetLoop(loop);

      // XXX: LR2_TEXT object should not apply this attribute.
      o->SetVisibleFlag(
        format_string("F%s", args.get_str(18)),
        format_string("F%s", args.get_str(19)),
        format_string("F%s", args.get_str(20)),
        std::string()
      );
    }

    o->GetAnimation().AddFrame(args);
  }
  BaseObjectLR2Handler() : LR2FnClass( GetTypename<BaseObject>() ) {
    ADDSHANDLERLR2(SRC);
    ADDSHANDLERLR2(DST);
  }
};

BaseObjectLR2Handler _BaseObjectLR2Handler;

}
