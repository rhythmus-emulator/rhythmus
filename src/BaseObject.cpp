#include "BaseObject.h"
#include "SceneManager.h"
#include "Setting.h"
#include "Util.h"
#include "Script.h"
#include <iostream>

namespace rhythmus
{

BaseObject::BaseObject()
  : parent_(nullptr), draw_order_(0), rot_center_(-1),
    resource_id_(-1), blending_(1),
    is_focusable_(false), is_focused_(false), is_hovered_(false)
{
  memset(&current_prop_, 0, sizeof(DrawProperty));
  current_prop_.sw = current_prop_.sh = 1.0f;
  current_prop_.display = true;
  SetRGB(1.0f, 1.0f, 1.0f);
  SetAlpha(1.0f);
  SetScale(1.0f, 1.0f);
  memset(visible_group_, 0, sizeof(visible_group_));
}

BaseObject::BaseObject(const BaseObject& obj)
  : name_(obj.name_), parent_(obj.parent_),
    draw_order_(obj.draw_order_), rot_center_(obj.rot_center_),
    resource_id_(obj.resource_id_), blending_(obj.blending_),
    is_focusable_(obj.is_focusable_), is_focused_(false), is_hovered_(false)
{
  // XXX: childrens won't copy by now
  tween_ = obj.tween_;
  current_prop_ = obj.current_prop_;
  memcpy(visible_group_, obj.visible_group_, sizeof(visible_group_));
}

BaseObject::~BaseObject()
{
  for (auto* p : owned_children_)
    delete p;
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
  children_.push_back(obj);
}

void BaseObject::RemoveChild(BaseObject* obj)
{
  auto it = std::find(children_.begin(), children_.end(), obj);
  if (it != children_.end())
    children_.erase(it);
}

void BaseObject::RemoveAllChild()
{
  children_.clear();
}

void BaseObject::RegisterChild(BaseObject* obj)
{
  owned_children_.push_back(obj);
  AddChild(obj);
}

void BaseObject::set_parent(BaseObject* obj)
{
  parent_ = obj;
}

BaseObject* BaseObject::get_parent()
{
  return parent_;
}

BaseObject* BaseObject::FindChildByName(const std::string& name)
{
  for (const auto& spr : children_)
    if (spr->get_name() == name) return spr;
  return nullptr;
}

BaseObject* BaseObject::FindRegisteredChildByName(const std::string& name)
{
  for (const auto& spr : owned_children_)
    if (spr->get_name() == name) return spr;
  return nullptr;
}

BaseObject* BaseObject::GetLastChild()
{
  if (children_.size() > 0)
    return children_.back();
  else return nullptr;
}

void BaseObject::RunCommandByName(const std::string &event_name)
{
  auto it = commands_.find(event_name);
  if (it != commands_.end())
    RunCommand(it->second);
}

void BaseObject::RunCommand(const std::string& command)
{
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
      it->second(this, CommandArgs(value));
    }
    catch (std::out_of_range& e)
    {
      std::cerr << "Error: Command parameter is not enough to execute " << command << std::endl;
    }
  }
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
  if (tween_.empty())
  {
    std::cerr << "Warning: tried to queue command without tween.";
    return;
  }

  // XXX: is it proper to execute command directly? refactoring necessary?
  if (tween_.size() == 1)
    RunCommand(command);
  tween_.back().commands = command;
}

void BaseObject::LoadCommand(const std::string &name, const std::string &command)
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

void BaseObject::LoadCommand(const Metric& metric)
{
  // process event handler registering
  // XXX: is this code is good enough?
  for (auto it = metric.cbegin(); it != metric.cend(); ++it)
  {
    if (it->first.size() >= 5 /* XXX: 'On' prefix? */
        && strnicmp(it->first.c_str(), "On", 2) == 0)
    {
      LoadCommand(it->first.substr(2), it->second);
    }
  }
}

void BaseObject::LoadCommandWithPrefix(const std::string &prefix, const Metric& metric)
{
  for (auto it = metric.cbegin(); it != metric.cend(); ++it)
  {
    if (strnicmp(it->first.c_str(), prefix.c_str(), prefix.size()) == 0)
    {
      LoadCommand(it->first.substr(prefix.size()), it->second);
    }
  }
}

void BaseObject::LoadCommandWithNamePrefix(const Metric& metric)
{
  if (get_name().empty())
    return;
  std::string prefix = get_name() + "On";
  LoadCommandWithPrefix(prefix, metric);
}

void BaseObject::AddCommand(const std::string &name, const std::string &command)
{
  LoadCommand(name, command);
}

void BaseObject::Load(const Metric& metric)
{
  if (metric.exist("zindex"))
    draw_order_ = metric.get<int>("zindex");
  if (metric.exist("lr2src"))
    LoadFromLR2SRC(metric.get<std::string>("lr2src"));

  LoadCommand(metric);
}

void BaseObject::LoadByText(const std::string &metric_text)
{
  Load(Metric(metric_text));
}

void BaseObject::LoadByName()
{
  if (get_name().empty())
    return;
  Metric *m = Setting::GetThemeMetricList().get_metric(get_name());
  if (!m)
    return;
  Load(*m);
}

void BaseObject::Initialize()
{
  if (get_name().empty()) return;
  auto *m = Setting::GetThemeMetricList().get_metric(get_name());
  if (m)
    Load(*m);
}

bool BaseObject::OnEvent(const EventMessage& msg)
{
  RunCommandByName(msg.GetEventName());
  return true;
}

void BaseObject::AddTweenState(const DrawProperty &draw_prop, uint32_t time_duration, int ease_type, bool loop)
{
  tween_.emplace_back(TweenState{
    draw_prop, time_duration, 0, 0, loop, ease_type
    });
}

void BaseObject::SetTweenTime(int time_msec)
{
  auto tween_length = GetTweenLength();
  if (tween_length > time_msec) return;
  SetDeltaTweenTime(time_msec - tween_length);
}

void BaseObject::SetDeltaTweenTime(int time_msec)
{
  tween_.push_back({
    GetDestDrawProperty(),
    (uint32_t)time_msec, 0, 0, false, EaseTypes::kEaseOut
    });
}

void BaseObject::StopTween()
{
  tween_.clear();
}

uint32_t BaseObject::GetTweenLength() const
{
  uint32_t r = 0;
  for (auto &t : tween_)
    r += t.time_duration - t.time_eclipsed;
  return r;
}

DrawProperty& BaseObject::GetDestDrawProperty()
{
  if (tween_.empty())
    return current_prop_;
  else
    return tween_.back().draw_prop;
}

DrawProperty& BaseObject::get_draw_property()
{
  return current_prop_;
}

void BaseObject::SetPos(int x, int y)
{
  auto& p = GetDestDrawProperty();
  p.pi.x = x;
  p.pi.y = y;
}

void BaseObject::MovePos(int x, int y)
{
  auto& p = GetDestDrawProperty();
  p.pi.x += x;
  p.pi.y += y;
}

void BaseObject::SetSize(int w, int h)
{
  auto& p = GetDestDrawProperty();
  p.w = w;
  p.h = h;
}

void BaseObject::SetAlpha(unsigned a)
{
  SetAlpha(a / 255.0f);
}

void BaseObject::SetAlpha(float a)
{
  auto& p = GetDestDrawProperty();
  p.aBL = p.aBR = p.aTL = p.aTR = a;
}

void BaseObject::SetRGB(unsigned r, unsigned g, unsigned b)
{
  SetRGB(r / 255.0f, g / 255.0f, b / 255.0f);
}

void BaseObject::SetRGB(float r, float g, float b)
{
  auto& p = GetDestDrawProperty();
  p.r = r;
  p.g = g;
  p.b = b;
}

void BaseObject::SetScale(float x, float y)
{
  auto& p = GetDestDrawProperty();
  p.pi.sx = x;
  p.pi.sy = y;
}

void BaseObject::SetRotation(float x, float y, float z)
{
  auto& p = GetDestDrawProperty();
  p.pi.rotx = x;
  p.pi.roty = y;
  p.pi.rotz = z;
}

void BaseObject::SetRotationAsRadian(float x, float y, float z)
{
  SetRotation(
    glm::radians(x),
    glm::radians(y),
    glm::radians(z)
  );
}

void BaseObject::SetRotationCenter(int rot_center)
{
  rot_center_ = rot_center;
}

void BaseObject::SetRotationCenterCoord(float x, float y)
{
  SetRotationCenter(-1);
  auto& p = GetDestDrawProperty();
  p.pi.tx = x;
  p.pi.ty = y;
}

void BaseObject::SetAcceleration(int acc)
{
  if (tween_.empty())
    return;
  auto& t = tween_.back();
  t.ease_type = acc;
}

void BaseObject::SetLR2DST(
  int time, int x, int y, int w, int h, int acc_type,
  int a, int r, int g, int b, int blend, int filter, int angle,
  int center)
{
  // DST specifies time information as the time of motion
  // So we need to calculate 'duration' of that motion.

  // calculate previous tween's duration
  if (!tween_.empty())
  {
    auto cur_motion_length = GetTweenLength();
    // to prevent overflow bug
    if (time < cur_motion_length)
      time = cur_motion_length;
    tween_.back().time_duration = time - cur_motion_length;
  }

  // If first tween and starting time is not zero,
  // then add dummy tween (invisible).
  if (tween_.empty() && time > 0)
  {
    // dummy tween should invisible
    Hide();
    SetDeltaTweenTime(time);
  }

  // make current tween
  SetDeltaTweenTime(0);

  // Now specify current tween.
  GetDestDrawProperty().display = true;
  SetPos(x, y);
  SetSize(w, h);
  SetRGB((unsigned)r, (unsigned)g, (unsigned)b);
  SetAlpha((unsigned)a);
  SetRotationAsRadian(0, 0, -angle);
  SetRotationCenter(center);
  switch (acc_type)
  {
  case 0:
    SetAcceleration(EaseTypes::kEaseLinear);
    break;
  case 1:
    SetAcceleration(EaseTypes::kEaseIn);
    break;
  case 2:
    SetAcceleration(EaseTypes::kEaseOut);
    break;
  case 3:
    SetAcceleration(EaseTypes::kEaseInOut);
    break;
  }

  // blend parameter should set by Sprite command.
  if (blend != 1)
    QueueCommand("blend:" + std::to_string(blend));
}

void BaseObject::SetLR2DSTCommand(const std::string &lr2dst)
{
  CommandArgs args(lr2dst);
  SetLR2DSTCommandInternal(args);
}

void BaseObject::SetLR2DSTCommandInternal(const CommandArgs &args)
{
  tween_.clear();
  CommandArgs la;
  la.set_separator('|');
  for (size_t i = 1; i < args.size(); ++i)
  {
    la.Set(args.Get<std::string>(i));
    SetLR2DST(
      la.Get<int>(0), /* time */
      la.Get<int>(1), la.Get<int>(2), la.Get<int>(3), la.Get<int>(4), /* xywh */
      la.Get<int>(5), /* acc_type */
      la.Get<int>(6), la.Get<int>(7), la.Get<int>(8), la.Get<int>(9), /* argb */
      la.Get<int>(10), la.Get<int>(11), la.Get<int>(12), /* blend, filter, angle */
      la.Get<int>(13) /* center */
    );
  }
  la.Set(args.Get<std::string>(0));
  SetVisibleGroup(
    la.Get<int>(0), la.Get<int>(1), la.Get<int>(2)
  );
  SetTweenLoopTime(la.Get<int>(3));
}

void BaseObject::SetVisibleGroup(int group0, int group1, int group2)
{
  visible_group_[0] = group0;
  visible_group_[1] = group1;
  visible_group_[2] = group2;
}

void BaseObject::Hide()
{
  current_prop_.display = false;
}

void BaseObject::Show()
{
  current_prop_.display = true;
}

void BaseObject::SetDrawOrder(int order)
{
  draw_order_ = order;
}

int BaseObject::GetDrawOrder() const
{
  return draw_order_;
}

void BaseObject::SetAllTweenPos(int x, int y)
{
  for (auto &t : tween_)
  {
    t.draw_prop.pi.x = x;
    t.draw_prop.pi.y = y;
  }
}

void BaseObject::SetAllTweenScale(float w, float h)
{
  for (auto &t : tween_)
  {
    t.draw_prop.w *= w;
    t.draw_prop.h *= h;
  }
}

void BaseObject::SetText(const std::string &value) {}

void BaseObject::SetNumber(int number) { SetText(std::to_string(number)); }

void BaseObject::SetNumber(double number) { SetText(std::to_string(number)); }

void BaseObject::Refresh() {}

void BaseObject::SetResourceId(int id) { resource_id_ = id; }

int BaseObject::GetResourceId() const { return resource_id_; }

void BaseObject::SetFocusable(bool is_focusable)
{
  is_focusable_ = is_focusable;
}

bool BaseObject::IsEntered(float x, float y)
{
  x -= current_prop_.pi.x + current_prop_.x;
  y -= current_prop_.pi.y + current_prop_.y;
  return (x >= 0 && x <= current_prop_.w
       && y >= 0 && y <= current_prop_.h);
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

void BaseObject::Click()
{
  if (!is_focusable_)
    return;

  // XXX: same as SetFocused(true) ...?
  SetFocused(true);

  RunCommandByName("click");
}

void BaseObject::SetBlend(int blend_mode) { blending_ = blend_mode; }

// milisecond
void BaseObject::UpdateTween(float delta_ms)
{
  if (!IsTweening())
    return;

  // time process
  while (1)
  {
    //
    // kind of trick: if single tween with loop,
    // It's actually useless. turn off loop attr.
    //
    // kind of trick: if current tween is last one,
    // Do UpdateTween here. We expect last tween state
    // should be same as current tween in that case.
    //
    // By this logic, two or more tweens exist
    // if we exit this loop with delta_ms <= 0.
    //
    if (tween_.size() == 1)
    {
      auto &t = tween_.front();
      current_prop_ = t.draw_prop;
      if (!t.commands.empty())
        RunCommand(t.commands);
      tween_.clear();
      return;
    }

    TweenState &t = tween_.front();
    if (delta_ms + t.time_eclipsed >= t.time_duration)  // tween end
    {
      delta_ms -= t.time_duration - t.time_eclipsed;

      // trigger next tween event if exist
      auto next = std::next(tween_.begin());
      if (next != tween_.end() && next->commands.empty())
        RunCommand(next->commands);

      // loop tween by push it again.
      // -> use more efficient method splice
      if (t.loop)
      {
        t.time_eclipsed = t.time_loopstart;
        tween_.splice(tween_.end(), tween_, tween_.begin());
      }
      else tween_.pop_front();
    }
    else  // ongoing tween
    {
      t.time_eclipsed += delta_ms;
      delta_ms = 0;  // actually same as exiting tween
    }

    //
    // actual loop condition
    //
    if (delta_ms <= 0)
      break;
  }

  //
  // Now calculate DrawState
  //
  DrawProperty& ti = current_prop_;
  const TweenState &t1 = tween_.front();
  const TweenState &t2 = *std::next(tween_.begin());

  ti.display = t1.draw_prop.display;

  // If not display, we don't need to calculate further away.
  if (ti.display)
  {
    float r = (float)t1.time_eclipsed / t1.time_duration;
    MakeTween(ti, t1.draw_prop, t2.draw_prop, r, t1.ease_type);

    // if rotation center need to be calculated
    if (rot_center_ >= 0 && rot_center_ < 10)
    {
      const float rel_pos_arr_x[] =
      { 0.5f, 0.f, 0.5f, 1.0f, 0.f, 0.5f, 1.0f, 0.f, 0.5f, 1.0f };
      const float rel_pos_arr_y[] =
      { 0.5f, 1.0f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 0.f, 0.f, 0.f };
      const float
        rel_pos_x = rel_pos_arr_x[rot_center_],
        rel_pos_y = rel_pos_arr_y[rot_center_];
      ti.pi.tx = ti.w * rel_pos_x;
      ti.pi.ty = ti.h * rel_pos_y;
    }
  }
}

/* This method should be called just after all fixed tween is appended. */
void BaseObject::SetTweenLoopTime(uint32_t time_msec)
{
  // all tweens should loop after a tween is marked to be looped.
  bool loopstart = false;

  for (auto& t : tween_)
  {
    if (loopstart || time_msec < t.time_duration)
    {
      t.time_loopstart = time_msec;
      t.loop = true;
      time_msec = 0;
      loopstart = true;
    }
    else {
      t.time_loopstart = 0;
      t.loop = false;
      time_msec -= t.time_duration;
    }
  }
}

bool BaseObject::IsVisible() const
{
  if (Script::getInstance().GetFlag(visible_group_[0]) == 0
    || Script::getInstance().GetFlag(visible_group_[1]) == 0
    || Script::getInstance().GetFlag(visible_group_[2]) == 0)
    return false;

  return current_prop_.display &&
    current_prop_.aBL > 0 &&
    current_prop_.aBR > 0 &&
    current_prop_.aTL > 0 &&
    current_prop_.aTR > 0;
}

bool BaseObject::IsTweening() const
{
  return tween_.size() > 0;
}


// milisecond
void BaseObject::Update(float delta)
{
  UpdateTween(delta);
  doUpdate(delta);
  for (auto* p : children_)
    p->Update(delta);
  doUpdateAfter(delta);
}

void BaseObject::Render()
{
  if (!IsVisible())
    return;

  Graphic::PushMatrix();

  // set matrix
  Graphic::getInstance().SetMatrix(get_draw_property().pi);

  // render vertices
  doRender();

  for (auto* p : children_)
    p->Render();

  doRenderAfter();

  Graphic::PopMatrix();
}

void BaseObject::doUpdate(float delta) {}
void BaseObject::doRender() {}
void BaseObject::doUpdateAfter(float delta) {}
void BaseObject::doRenderAfter() {}


#define TWEEN_ATTRS \
  TWEEN(x) \
  TWEEN(y) \
  TWEEN(w) \
  TWEEN(h) \
  TWEEN(r) \
  TWEEN(g) \
  TWEEN(b) \
  TWEEN(aTL) \
  TWEEN(aTR) \
  TWEEN(aBR) \
  TWEEN(aBL) \
  TWEEN(sx) \
  TWEEN(sy) \
  TWEEN(sw) \
  TWEEN(sh) \
  TWEEN(pi.rotx) \
  TWEEN(pi.roty) \
  TWEEN(pi.rotz) \
  TWEEN(pi.tx) \
  TWEEN(pi.ty) \
  TWEEN(pi.x) \
  TWEEN(pi.y) \
  TWEEN(pi.sx) \
  TWEEN(pi.sy) \


void MakeTween(DrawProperty& ti, const DrawProperty& t1, const DrawProperty& t2,
  double r, int ease_type)
{
  switch (ease_type)
  {
  case EaseTypes::kEaseLinear:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseIn:
  {
    // use cubic function
    r = r * r * r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseOut:
  {
    // use cubic function
    r = 1 - r;
    r = r * r * r;
    r = 1 - r;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseInOut:
  {
    // use cubic function
    r = 2 * r - 1;
    r = r * r * r;
    r = 0.5f + r / 2;
#define TWEEN(attr) \
  ti.attr = t1.attr * (1 - r) + t2.attr * r;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  case EaseTypes::kEaseNone:
  default:
  {
#define TWEEN(attr) \
  ti.attr = t1.attr;

    TWEEN_ATTRS;

#undef TWEEN
    break;
  }
  }
}

void BaseObject::LoadFromLR2SRC(const std::string &cmd)
{
  /* implemented later */
}

/* ---------------------------- Command related */

const CommandFnMap& BaseObject::GetCommandFnMap()
{
  static CommandFnMap cmdfnmap_;
  if (cmdfnmap_.empty())
  {
    static auto fn_Pos = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetPos(args.Get<int>(0), args.Get<int>(1));
    };
    static auto fn_SetLR2DST = [](void *o, CommandArgs& args) {
      auto *b = static_cast<BaseObject*>(o);
      b->SetLR2DSTCommandInternal(args);
    };
    static auto fn_Show = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->Show();
    };
    static auto fn_Hide = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->Hide();
    };
    static auto fn_Loop = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetTweenLoopTime((uint32_t)args.Get<int>(0));
    };
    static auto fn_Text = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetText(args.Get<std::string>(0));
    };
    static auto fn_Number = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetNumber(args.Get<int>(0));
    };
    static auto fn_NumberF = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetNumber(args.Get<double>(0));
    };
    static auto fn_Refresh = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->Refresh();
    };
    static auto fn_resid = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetResourceId(args.Get<int>(0));
    };
    static auto fn_blend = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetBlend(args.Get<int>(0));
    };
    static auto fn_Focusable = [](void *o, CommandArgs& args) {
      static_cast<BaseObject*>(o)->SetFocusable(args.Get<int>(0));
    };
    static auto fn_SendEvent = [](void *o, CommandArgs& args) {
      EventManager::SendEvent(args.Get<std::string>(0));
    };
    cmdfnmap_["pos"] = fn_Pos;
    cmdfnmap_["lr2cmd"] = fn_SetLR2DST;
    cmdfnmap_["show"] = fn_Show;
    cmdfnmap_["hide"] = fn_Hide;
    cmdfnmap_["loop"] = fn_Loop;
    cmdfnmap_["text"] = fn_Loop;
    cmdfnmap_["number"] = fn_Number;
    cmdfnmap_["numberf"] = fn_NumberF;
    cmdfnmap_["refresh"] = fn_Refresh;
    cmdfnmap_["resid"] = fn_resid;
    cmdfnmap_["blend"] = fn_blend;
    cmdfnmap_["focusable"] = fn_Focusable;
    cmdfnmap_["sendevent"] = fn_SendEvent;
  }

  return cmdfnmap_;
}

}